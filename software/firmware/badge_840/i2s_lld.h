/*-
 * Copyright (c) 2018
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

#ifndef _I2S_LLDH_
#define _I2S_LLDH_

#define I2S_THREAD_PRIO		(NORMALPRIO + 1)
#define I2S_SAMPLES		1024
#define I2S_BYTES		(I2S_SAMPLES * sizeof(uint16_t))

#define I2S_PLAY_ONCE		0
#define I2S_PLAY_LOOP		1

#define I2S_STATE_IDLE		0
#define I2S_STATE_BUSY		1

#define I2S_PPI_CHAN		1
#define I2S_EGU_TASK		1

#define I2S_AMP_OFF		0
#define I2S_AMP_ON		1

extern void i2sStart (void);

extern void i2sAudioAmpCtl (uint8_t);

extern void i2sSamplesPlay (void *, int);
extern void i2sSamplesWait (void);
extern void i2sSamplesStop (void);

extern int i2sWait (void);
extern void i2sPlay (char *);
extern void i2sLoopPlay (char *, uint8_t);

extern uint16_t * i2sBuf;
extern uint8_t i2sEnabled;

#endif /* _I2S_LLDH_ */
