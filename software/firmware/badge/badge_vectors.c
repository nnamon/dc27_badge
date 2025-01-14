/*-
 * Copyright (c) 2019
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ch.h"
#include "hal.h"
#include "gfx.h"

#include <stdio.h>
#include <stdint.h>

/*
 * Information on fault handling comes from the Cortex-M4 Generic User Guide
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0553b/DUI0553.pdf
 */

#define HARD_FAULT	0
#define BUS_FAULT	1
#define USAGE_FAULT	2
#define MEMMANAGE_FAULT	3

/*
 * Cortex-M4 exception frame, including floating point state.
 */

typedef struct exc_frame {
	uint32_t		exc_R[4];
	uint32_t		exc_R12;
	uint32_t		exc_LR;
	uint32_t		exc_PC;
	uint32_t		exc_xPSR;
	uint32_t		exc_fpregs[16];
	uint32_t		exc_FPSCR;
	uint32_t		exc_dummy;
} EXC_FRAME;

/*
 * Cortex-M4 exception types.
 */

static char * exc_msg[] = {
	"Instruction access violation",
	"Data access violation",
	NULL,
	"Memory management fault on unstacking during exception return",
	"Memory management fault on stacking during exception entry",
	"Memory management fault during floating point context save",
	NULL,
	NULL,
	"Instruction bus error",
	"Precise data bus error",
	"Imprecise data bus error",
	"Bus error on unstacking during exception return",
	"Bus error on stacking during exception entry",
	"Bus error during floating point context save",
	NULL,
	NULL,
	"Illegal instruction",
	"Invalid EPSR state",
	"Invalid PC load via EXC_RETURN",
	"Unsupported coprocessor",
	NULL,
	NULL,
	NULL,
	NULL,
	"Unaligned access trap",
	"Division by zero",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static char exc_msgbuf[80];

/*
 * This global controls whether or not the idle thread will execute
 * a WFI instruction to put the CPU to sleep when we're idle. This
 * is generally a good idea for power saving. However, it takes a
 * certain amount of time for the CPU to begin executing instructions
 * again after you wake it up, and that can be bad for certain places
 * where we need low latency. For example, the music player app has
 * to read data from the SD card, draw the spectrograph on the screen,
 * update the LED array, and send audio samples through the I2S
 * controller. We need to keep writing samples into the I2S controller
 * on a regular basis in order to avoid the audio sounding warbly.
 * Sometimes it takes the CPU too long to wake up after sleeping and
 * we can't meet the deadline, so in those cases, we can set this
 * variable to false temporarily in order to prevent the CPU from
 * sleeping.
 */

static volatile bool_t badge_sleep = TRUE;

/*
 * FPU interrupt
 * Floating point exceptions can occur for things like overflow
 * and underflow, which are not treated as CPU exceptions like
 * divide by zero. When they occur, this triggers the FPU interrupt
 * in the NVIC. Normally this interrupt is not enabled, so we don't
 * notice when these events occur, but we really should handle them.
 * For one thing, it's not possible to put the CPU into low power
 * modes while FPU events are pending.
 *
 * We do not set the FPSCR register directly. When an FPU interrupt
 * occurs, the CPU pushes the current CPU state, including the FPU
 * context, onto the stack. The FPU context includes an FPSCR word,
 * and this word will be loaded back into the FPSCR register from
 * the stack when the interrupt service routine exits. So modify
 * the stashed FPSCR word on the stack and the FPSCR register instead
 * of just the FPSCR register itself.
 */

#define FPU_EXCEPTION_MASK 0x0000009F

OSAL_IRQ_HANDLER(VectorD8)
{
	EXC_FRAME * exc;

	OSAL_IRQ_PROLOGUE();

	/*
	 * The Cortex-M4F supports a feature called lazy stacking.
	 * FPU state is something which should, in some cases,
	 * be preserved when an interrupt occurs. However saving the
	 * FPU state to the stack adds some extra overhead which can
	 * affect interrupt latency. The rule is that if the CPU uses a
	 * floating point instruction and an interrupt occurs, extra
	 * space will be reserved on the stack to hold the floating
	 * point context. However with lazy stacking, that extra
	 * space is not actually filled with the FPU register state
	 * right away. (Allocating the extra space takes no extra
	 * work since all the CPU has to do is set the SP register
	 * to a slightly different value. Actually writing out the
	 * FPU register contents to the stack is another matter.)
	 *
	 * To force the CPU to write the FPU state, we need to perform
	 * some kind of floating point operation on entry to the ISR.
	 * Reading the FPSCR register counts as a "floating point
	 * operation" and has the added benefit that it doesn't also
	 * change the the FPU state. So reading it here forces the
	 * CPU to put the FPU registers onto the stack, including
	 * the current FPSCR contents itself.
	 */

	(void)__get_FPSCR();

	/*
	 * Get the exception stack frame pointer. Note that using
	 * the PSP assumes we will only get a floating point exception
	 * from thread-mode code. This should be true since we
	 * should never be using floating point instructions in
	 * an interrupt handler.
	 */

	exc = (EXC_FRAME *)__get_PSP();

	/*
	 * Clear floating point exceptions. We directly update the
	 * FPSCR register and the FPSCR word in the exception frame.
	 * This prevents the FPSCR status bits from possibly being
	 * changed back to their previous state later when we exit
	 * the ISR.
	 */

	exc->exc_FPSCR &= ~(FPU_EXCEPTION_MASK);
	__set_FPSCR(exc->exc_FPSCR);

	OSAL_IRQ_EPILOGUE();

	return;
}

void
badge_idle (void)
{
	if (badge_sleep == TRUE)
		__WFI();
}

void
badge_sleep_enable (void)
{
	badge_sleep = TRUE;
	return;
}

void
badge_sleep_disable (void)
{
	badge_sleep = FALSE;
	return;
}

static void
_putc (char c)
{
	NRF_UART0->EVENTS_TXDRDY = 0;
	(void)NRF_UART0->EVENTS_TXDRDY;
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->TXD = (uint32_t)c;
	(void)NRF_UART0->TXD;
	while (NRF_UART0->EVENTS_TXDRDY == 0)
		;
	NRF_UART0->TASKS_STOPTX = 1;

	return;
}

static void
_puts (char * str)
{
	char * p;
	p = str;
	while (*p != '\0') {
		_putc (*p);
		p++;
	}
	_putc ('\r');
	_putc ('\n');
	return;
}

static void
dumpFrame (int type, uint32_t lr, EXC_FRAME *p)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (SCB->CFSR & (1 << i) && exc_msg[i] != NULL)
			_puts (exc_msg[i]);
	}

	if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk)
		_puts ("Bus fault on vector table read during exception");

	if (type == HARD_FAULT && SCB->HFSR & SCB_HFSR_FORCED_Msk)
		_puts ("Forced fault due to configurable priority violation");

	if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) {
		sprintf (exc_msgbuf, "Memory fault address: 0x%08lX",
		    SCB->BFAR);
		_puts (exc_msgbuf);
	}

	if (SCB->CFSR & SCB_CFSR_BFARVALID_Msk) {
		sprintf (exc_msgbuf, "Bus fault address: 0x%08lX",
		    SCB->BFAR);
		_puts (exc_msgbuf);
	}

	sprintf (exc_msgbuf, "Fault while in %s mode", lr & 0x8 ?
	    "thread" : "handler");
	_puts (exc_msgbuf);

	sprintf (exc_msgbuf, "Floating point context %ssaved on stack",
	    lr & 0x10 ? "not " : "");
	_puts (exc_msgbuf);

	if (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) {
		sprintf (exc_msgbuf, "Interrupt is pending");
		_puts (exc_msgbuf);
	}

	if (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) {
		sprintf (exc_msgbuf, "Exception pending: %ld",
		    (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) >>
		    SCB_ICSR_VECTPENDING_Pos);
		_puts (exc_msgbuf);
	}

	if (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) {
		sprintf (exc_msgbuf, "Exception active: %ld",
		    (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >>
		    SCB_ICSR_VECTACTIVE_Pos);
		_puts (exc_msgbuf);
	}

	sprintf (exc_msgbuf, "PC: 0x%08lX LR: 0x%08lX "
	    "SP: 0x%08lX SR: 0x%08lX",
	    p->exc_PC, p->exc_LR,
	    (uint32_t)p + (lr & 0x10 ? 32 : sizeof(EXC_FRAME)),
	    p->exc_xPSR);
	_puts (exc_msgbuf);
	sprintf (exc_msgbuf, "R0: 0x%08lX R1: 0x%08lX "
	    "R2: 0x%08lX R3: 0x%08lX R12: 0x%08lX",
	    p->exc_R[0], p->exc_R[1], p->exc_R[2], p->exc_R[3], p->exc_R12);
	_puts (exc_msgbuf);

	return;
}

void
trapHandle (int type, uint32_t exc_lr, EXC_FRAME * exc_sp)
{
	int i;

	/* Reset the serial port. */

	NRF_UART0->INTENCLR = 0xFFFFFFFF;
	(void)NRF_UART0->INTENCLR;
	NRF_UART0->TASKS_STOPTX = 1;

	for (i = 0; i < 1000; i++)
		__DSB();

	_puts ("");
	_puts ("");

	switch (type) {
		case HARD_FAULT:
			_puts ("********** HARD FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case BUS_FAULT:
			_puts ("********** BUS FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case USAGE_FAULT:
			_puts ("********** USAGE FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case MEMMANAGE_FAULT:

			/*
			 * Disable the MPU before handling a memory manager
			 * fault. If the fault occurs during exception
			 * stacking, it means the CPU tried to push an
			 * exception frame onto a protected area of the
			 * stack. That protection will remain in effect
			 * if we try to decode the stack frame in the
			 * trap handler, and we'll trigger a hard fault.
			 * Decoding the stack frame in this case may not
			 * actually yield valid results, but it's wrong
			 * to trigger another fault too.
			 */
			mpuDisable ();

			_puts ("********** MEMMANAGE FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		default:
			_puts ("********** unknown fault **********");
			break;
	}

	/* Break into the debugger */

	__asm__ ("bkpt #0");

	while (1) {
	}

	/* NOTREACHED */
}
