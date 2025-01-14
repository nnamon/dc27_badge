ifeq ($(USE_SMART_BUILD),yes)
HALCONF := $(strip $(shell cat halconf.h halconf_community.h 2>/dev/null | egrep -e "define"))

# List of all the NRF52x platform files.
PLATFORMSRC  = ${CHIBIOS}/os/hal/ports/common/ARMCMx/nvic.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_st_lld.c

ifneq ($(findstring HAL_USE_PAL TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_pal_lld.c
endif
ifneq ($(findstring HAL_USE_SERIAL TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_serial_lld.c
endif
ifneq ($(findstring HAL_USE_SPI TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_spi_lld.c
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_spim_lld.c
endif
ifneq ($(findstring HAL_USE_QSPI TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_qspi_lld.c
endif
ifneq ($(findstring HAL_USE_EXT TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_ext_lld_isr.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_ext_lld.c
endif
ifneq ($(findstring HAL_USE_I2C TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_i2c_lld.c
endif
ifneq ($(findstring HAL_USE_GPT TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_gpt_lld.c
endif
ifneq ($(findstring HAL_USE_WDG TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_wdg_lld.c
endif
ifneq ($(findstring HAL_USE_RNG TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_rng_lld.c
endif
ifneq ($(findstring HAL_USE_QEI TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_qei_lld.c
endif
else
PLATFORMSRC  = ${CHIBIOS}/os/hal/ports/common/ARMCMx/nvic.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_ext_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840/hal_ext_lld_isr.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_pal_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_serial_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_spi_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_spim_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_qspi_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_i2c_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_st_lld.c \
	       ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_gpt_lld.c \
	       ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_wdg_lld.c \
	       ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_rng_lld.c \
               ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD/hal_qei_lld.c
endif

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/ports/common/ARMCMx \
              ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/LLD \
              ${CHIBIOS_CONTRIB}/os/hal/ports/NRF5/NRF52840


