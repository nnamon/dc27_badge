/*
    Copyright (C) 2015 Stephen Caudle

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    NRF/LLD/hal_spi_lld.h
 * @brief   NRF5 low level SPI driver header.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef HAL_SPI_LLD_H
#define HAL_SPI_LLD_H

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Enable SPI controller 0
 */
#if !defined(NRF5_SPI_USE_SPI0) || defined(__DOXYGEN__)
#define NRF5_SPI_USE_SPI0                 FALSE
#endif

/**
 * @brief   Enable SPI controller 1
 */
#if !defined(NRF5_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define NRF5_SPI_USE_SPI1                 FALSE
#endif

/**
 * @brief   Enable SPI controller 2
 */
#if !defined(NRF5_SPI_USE_SPI2) || defined(__DOXYGEN__)
#define NRF5_SPI_USE_SPI2                 FALSE
#endif

/**
 * @brief   Enable SPI controller 3 (note: SPIM3 supports > 8MHz clock)
 */
#if !defined(NRF5_SPI_USE_SPI3) || defined(__DOXYGEN__)
#define NRF5_SPI_USE_SPI3                 FALSE
#endif

/**
 * @brief   Select SPIM with EasyDMA vs. legacy SPI
 */
#if !defined(NRF5_SPI_USE_DMA) || defined(__DOXYGEN__)
#define NRF5_SPI_USE_DMA                  FALSE
#endif

/**
 * @brief   Enable NRF52832 rev 1 anomaly 58 workaround
 */
#if !defined(NRF5_SPIM_USE_ANOM58_WAR) || defined(__DOXYGEN__)
#define NRF5_SPIM_USE_ANOM58_WAR          FALSE
#endif

/**
 * @brief   Set NRF52832 rev 1 anomaly 58 workaround PPI channel
 */
#if !defined(NRF5_ANOM58_PPI) || defined(__DOXYGEN__)
#define NRF5_ANOM58_PPI                  10
#endif

/**
 * @brief   Set NRF52832 rev 1 anomaly 58 workaround GPIOTE channel
 */
#if !defined(NRF5_ANOM58_GPIOTE) || defined(__DOXYGEN__)
#define NRF5_ANOM58_GPIOTE               7
#endif

/**
 * @brief   Set NRF52832 DMA chunk size
 */
#if !defined(NRF5_SPIM_SLOW_DMA_CHUNK) || defined(__DOXYGEN__)
#define NRF5_SPIM_SLOW_DMA_CHUNK              128
#endif

/**
 * @brief   Set NRF52840 DMA chunk size
 */
#if !defined(NRF5_SPIM_FAST_DMA_CHUNK) || defined(__DOXYGEN__)
#define NRF5_SPIM_FAST_DMA_CHUNK              128
#endif

/**
 * @brief   SPI0 interrupt priority level setting.
 */
#if !defined(NRF5_SPI_SPI0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define NRF5_SPI_SPI0_IRQ_PRIORITY    3
#endif

/**
 * @brief   SPI1 interrupt priority level setting.
 */
#if !defined(NRF5_SPI_SPI1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define NRF5_SPI_SPI1_IRQ_PRIORITY    3
#endif

/**
 * @brief   SPI2 interrupt priority level setting.
 */
#if !defined(NRF5_SPI_SPI2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define NRF5_SPI_SPI2_IRQ_PRIORITY    3
#endif

/**
 * @brief   SPI3 interrupt priority level setting.
 */
#if !defined(NRF5_SPI_SPI3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define NRF5_SPI_SPI3_IRQ_PRIORITY    3
#endif

/**
 * @brief   Overflow error hook.
 * @details The default action is to stop the system.
 */
#if !defined(NRF5_SPI_SPI_ERROR_HOOK) || defined(__DOXYGEN__)
#define NRF5_SPI_SPI_ERROR_HOOK()    chSysHalt()
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !NRF5_SPI_USE_SPI0 && !NRF5_SPI_USE_SPI1 &&				    \
    !NRF5_SPI_USE_SPI2 && !NRF5_SPI_USE_SPI3
#error "SPI driver activated but no SPI peripheral assigned"
#endif

#if NRF5_SPI_USE_SPI0 &&						    \
    !OSAL_IRQ_IS_VALID_PRIORITY(NRF5_SPI_SPI0_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI0"
#endif

#if NRF5_SPI_USE_SPI1 &&						    \
    !OSAL_IRQ_IS_VALID_PRIORITY(NRF5_SPI_SPI1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI1"
#endif

#if NRF5_SPI_USE_SPI2 &&						    \
    !OSAL_IRQ_IS_VALID_PRIORITY(NRF5_SPI_SPI2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI2"
#endif

#if NRF5_SPI_USE_SPI3 &&						    \
    !OSAL_IRQ_IS_VALID_PRIORITY(NRF5_SPI_SPI3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI3"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an SPI driver.
 */
typedef struct SPIDriver SPIDriver;

/**
 * @brief   SPI notification callback type.
 *
 * @param[in] spip      pointer to the @p SPIDriver object triggering the
 *                      callback
 */
typedef void (*spicallback_t)(SPIDriver *spip);

/**
 * @brief   SPI frequency
 */
typedef enum {
  NRF5_SPI_FREQ_125KBPS = (SPI_FREQUENCY_FREQUENCY_K125 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_250KBPS = (SPI_FREQUENCY_FREQUENCY_K250 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_500KBPS = (SPI_FREQUENCY_FREQUENCY_K500 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_1MBPS = (SPI_FREQUENCY_FREQUENCY_M1 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_2MBPS = (SPI_FREQUENCY_FREQUENCY_M2 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_4MBPS = (SPI_FREQUENCY_FREQUENCY_M4 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_8MBPS = (SPI_FREQUENCY_FREQUENCY_M8 << SPI_FREQUENCY_FREQUENCY_Pos),
#ifdef SPIM_FREQUENCY_FREQUENCY_M16
  NRF5_SPI_FREQ_16MBPS = (SPIM_FREQUENCY_FREQUENCY_M16 << SPI_FREQUENCY_FREQUENCY_Pos),
  NRF5_SPI_FREQ_32MBPS = (SPIM_FREQUENCY_FREQUENCY_M32 << SPI_FREQUENCY_FREQUENCY_Pos),
#endif
} spifreq_t;

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief Operation complete callback or @p NULL.
   */
  spicallback_t         end_cb;
  /**
   * @brief The frequency of the SPI peripheral
   */
  spifreq_t             freq;
  /**
   * @brief The SCK pad
   */
  uint16_t              sckpad;
  /**
   * @brief The MOSI pad
   */
  uint16_t              mosipad;
  /**
   * @brief The MOSI pad
   */
  uint16_t              misopad;
  /* End of the mandatory fields.*/
  /**
   * @brief The chip select line pad number.
   */
  uint16_t              sspad;
  /**
   * @brief Shift out least significant bit first
   */
  uint8_t               lsbfirst;
  /**
   * @brief SPI mode
   */
  uint8_t               mode;
  /**
   * @brief dummy data for SPI ignore
   */
  uint8_t               dummy;
} SPIConfig;

/**
 * @brief   Structure representing a SPI driver.
 */
struct SPIDriver {
  /**
   * @brief Driver state.
   */
  spistate_t            state;
  /**
   * @brief Current configuration data.
   */
  const SPIConfig       *config;
#if SPI_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief Waiting thread.
   */
  thread_reference_t    thread;
#endif /* SPI_USE_WAIT */
#if SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief Mutex protecting the bus.
   */
  mutex_t               mutex;
#elif CH_CFG_USE_SEMAPHORES
  semaphore_t           semaphore;
#endif
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#if defined(SPI_DRIVER_EXT_FIELDS)
  SPI_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the SPI port.
   */
#if NRF5_SPI_USE_DMA == TRUE
  NRF_SPIM_Type          *port;
#else
  NRF_SPI_Type          *port;
#endif
  /**
   * @brief Number of bytes yet to be received.
   */
  uint32_t              rxcnt;
  /**
   * @brief Receive pointer or @p NULL.
   */
  void                  *rxptr;
  /**
   * @brief Number of bytes yet to be transmitted.
   */
  uint32_t              txcnt;
  /**
   * @brief Transmit pointer or @p NULL.
   */
  const void            *txptr;
  /**
   * @brief Maximum DMA chunk size
   */
  uint32_t		chunk;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if NRF5_SPI_USE_SPI0 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif
#if NRF5_SPI_USE_SPI1 && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
#endif
#if NRF5_SPI_USE_SPI2 && !defined(__DOXYGEN__)
extern SPIDriver SPID3;
#endif
#if NRF5_SPI_USE_SPI3 && !defined(__DOXYGEN__)
extern SPIDriver SPID4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void spi_lld_init(void);
  void spi_lld_start(SPIDriver *spip);
  void spi_lld_stop(SPIDriver *spip);
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
  void spi_lld_ignore(SPIDriver *spip, size_t n);
  void spi_lld_exchange(SPIDriver *spip, size_t n,
                        const void *txbuf, void *rxbuf);
  void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
  uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI */

#endif /* HAL_SPI_LLD_H */

/** @} */
