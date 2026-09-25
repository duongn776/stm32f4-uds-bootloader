/*
 * stm32f407xx_rcc.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  RCC driver for STM32F407xx: read back the current clock frequencies.
 *  Reference: RM0090 Rev 21, section 7 (RCC for STM32F405xx/07xx).
 */

#ifndef INC_STM32F407XX_RCC_H_
#define INC_STM32F407XX_RCC_H_

#include "stm32f407xx.h"

/* Oscillator frequencies (Hz) */
#define RCC_HSI_FREQ                16000000U   /*!< Internal RC oscillator                   */
#define RCC_HSE_FREQ                8000000U    /*!< External crystal on STM32F407G-DISC1     */

/* RCC_CFGR fields (RM0090 7.3.3) */
#define RCC_CFGR_SWS_POS            2U          /*!< SWS[1:0]   bits 3:2   */
#define RCC_CFGR_HPRE_POS           4U          /*!< HPRE[3:0]  bits 7:4   */
#define RCC_CFGR_PPRE1_POS          10U         /*!< PPRE1[2:0] bits 12:10 */
#define RCC_CFGR_PPRE2_POS          13U         /*!< PPRE2[2:0] bits 15:13 */

/* SWS values */
#define RCC_SYSCLK_SRC_HSI          0U
#define RCC_SYSCLK_SRC_HSE          1U
#define RCC_SYSCLK_SRC_PLL          2U

/* RCC_PLLCFGR fields (RM0090 7.3.2) */
#define RCC_PLLCFGR_PLLM_POS        0U          /*!< PLLM[5:0]  bits 5:0   */
#define RCC_PLLCFGR_PLLN_POS        6U          /*!< PLLN[8:0]  bits 14:6  */
#define RCC_PLLCFGR_PLLP_POS        16U         /*!< PLLP[1:0]  bits 17:16 */
#define RCC_PLLCFGR_PLLSRC_POS      22U         /*!< PLLSRC     bit 22     */

/* Returns the system clock (SYSCLK) frequency in Hz */
uint32_t RCC_GetSysClock_Value(void);

/* Returns the AHB clock (HCLK) frequency in Hz */
uint32_t RCC_GetHCLK_Value(void);

/* Returns the APB1 clock (PCLK1) frequency in Hz */
uint32_t RCC_GetPCLK1_Value(void);

/* Returns the APB2 clock (PCLK2) frequency in Hz */
uint32_t RCC_GetPCLK2_Value(void);

/* Returns the main PLL output (PLLCLK) frequency in Hz */
uint32_t RCC_GetPLLOutputClock(void);

#endif /* INC_STM32F407XX_RCC_H_ */
