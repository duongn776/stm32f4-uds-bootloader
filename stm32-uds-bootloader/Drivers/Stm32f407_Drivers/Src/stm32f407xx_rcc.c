/*
 * stm32f407xx_rcc.c
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  RCC driver for STM32F407xx: read back the current clock frequencies.
 *  Reference: RM0090 Rev 21, section 7 (RCC for STM32F405xx/07xx).
 */

#include "stm32f407xx_rcc.h"

/* HPRE = 1000..1111 -> /2, /4, /8, /16, /64, /128, /256, /512 (no /32) */
static const uint16_t AHB_Prescaler[8] = { 2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U };

/* PPREx = 100..111 -> /2, /4, /8, /16 */
static const uint8_t APB_Prescaler[4] = { 2U, 4U, 8U, 16U };

/**
  * @brief  Returns the APB prescaler division factor from a PPREx field.
  * @param  ppre 3-bit PPRE1 or PPRE2 field value.
  * @retval Division factor (1, 2, 4, 8 or 16).
  */
static uint32_t RCC_GetAPBDivider(uint32_t ppre)
{
    /* 0xx: AHB clock not divided */
    if (ppre < 4U)
    {
        return 1U;
    }

    return APB_Prescaler[ppre - 4U];
}

/**
  * @brief  Calculates the main PLL output frequency (PLLCLK).
  * @note   f(VCO)    = f(PLL input) * PLLN / PLLM
  *         f(PLLCLK) = f(VCO) / PLLP, PLLP = 2, 4, 6 or 8   (RM0090 7.3.2)
  * @retval PLLCLK frequency in Hz, 0 if PLLM is an invalid configuration (0).
  */
uint32_t RCC_GetPLLOutputClock(void)
{
    uint32_t pllcfgr = RCC->PLLCFGR;
    uint32_t pllInput;
    uint32_t pllm;
    uint32_t plln;
    uint32_t pllp;

    /* PLLSRC: 0 = HSI, 1 = HSE */
    pllInput = ((pllcfgr >> RCC_PLLCFGR_PLLSRC_POS) & 0x1U) ? RCC_HSE_FREQ : RCC_HSI_FREQ;

    pllm = (pllcfgr >> RCC_PLLCFGR_PLLM_POS) & 0x3FU;
    plln = (pllcfgr >> RCC_PLLCFGR_PLLN_POS) & 0x1FFU;
    pllp = ((((pllcfgr >> RCC_PLLCFGR_PLLP_POS) & 0x3U) + 1U) * 2U);

    if (pllm == 0U)
    {
        return 0U;
    }

    /* Divide first: f(input)/PLLM is 1..2 MHz, so f * PLLN (max 432) fits in 32 bits */
    return ((pllInput / pllm) * plln) / pllp;
}

/**
  * @brief  Calculates the system clock frequency (SYSCLK) from RCC_CFGR.SWS.
  * @retval SYSCLK frequency in Hz.
  */
uint32_t RCC_GetSysClock_Value(void)
{
    uint32_t clockSrc = (RCC->CFGR >> RCC_CFGR_SWS_POS) & 0x3U;
    uint32_t sysClock;

    if (clockSrc == RCC_SYSCLK_SRC_HSI)
    {
        sysClock = RCC_HSI_FREQ;
    }
    else if (clockSrc == RCC_SYSCLK_SRC_HSE)
    {
        sysClock = RCC_HSE_FREQ;
    }
    else /* RCC_SYSCLK_SRC_PLL (11 is not applicable) */
    {
        sysClock = RCC_GetPLLOutputClock();
    }

    return sysClock;
}

/**
  * @brief  Calculates the AHB clock frequency (HCLK).
  * @retval HCLK frequency in Hz.
  */
uint32_t RCC_GetHCLK_Value(void)
{
    uint32_t hpre = (RCC->CFGR >> RCC_CFGR_HPRE_POS) & 0xFU;
    uint32_t ahbDiv;

    /* 0xxx: system clock not divided */
    if (hpre < 8U)
    {
        ahbDiv = 1U;
    }
    else
    {
        ahbDiv = AHB_Prescaler[hpre - 8U];
    }

    return RCC_GetSysClock_Value() / ahbDiv;
}

/**
  * @brief  Calculates the APB1 peripheral clock frequency (PCLK1).
  * @note   PCLK1 must not exceed 42 MHz on STM32F407.
  * @retval PCLK1 frequency in Hz.
  */
uint32_t RCC_GetPCLK1_Value(void)
{
    uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_POS) & 0x7U;

    return RCC_GetHCLK_Value() / RCC_GetAPBDivider(ppre1);
}

/**
  * @brief  Calculates the APB2 peripheral clock frequency (PCLK2).
  * @note   PCLK2 must not exceed 84 MHz on STM32F407.
  * @retval PCLK2 frequency in Hz.
  */
uint32_t RCC_GetPCLK2_Value(void)
{
    uint32_t ppre2 = (RCC->CFGR >> RCC_CFGR_PPRE2_POS) & 0x7U;

    return RCC_GetHCLK_Value() / RCC_GetAPBDivider(ppre2);
}
