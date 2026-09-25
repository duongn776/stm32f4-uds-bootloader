/*
 * stm32f407xx_systick.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  SysTick driver for STM32F407xx: 1 ms time base (tick counter + blocking delay).
 *  Reference: Cortex-M4 Generic User Guide 4.4 (SysTick), 4.3.10 (SHPR3),
 *             RM0090 6.2 (SysTick clock = HCLK/8 or HCLK).
 */

#ifndef INC_STM32F407XX_SYSTICK_H_
#define INC_STM32F407XX_SYSTICK_H_

#include "stm32f407xx.h"

/**
  * @brief SysTick register layout (Cortex-M4)
  */
typedef struct
{
    volatile uint32_t STK_CTRL;     /*!< Control and status register, offset 0x00 */
    volatile uint32_t STK_LOAD;     /*!< Reload value register,       offset 0x04 */
    volatile uint32_t STK_VAL;      /*!< Current value register,      offset 0x08 */
    volatile uint32_t STK_CALIB;    /*!< Calibration value register,  offset 0x0C */
} SysTick_Reg;

/* SysTick base address and register access */
#define SYSTICK_BASEADDRESS             0xE000E010UL
#define SYSTICK                         ((SysTick_Reg *)SYSTICK_BASEADDRESS)

/* System Handler Priority Register 3: PRI_15 (SysTick) = bits [31:24] */
#define SYSTICK_SHPR3                   ((volatile uint32_t *)0xE000ED20UL)
#define SYSTICK_SHPR3_PRI_POS           24U

/* STK_CTRL bit positions */
#define SYSTICK_CTRL_ENABLE_POS         0U
#define SYSTICK_CTRL_TICKINT_POS        1U
#define SYSTICK_CTRL_CLKSOURCE_POS      2U
#define SYSTICK_CTRL_COUNTFLAG_POS      16U

/* STK_LOAD valid range (24-bit, 0 has no effect) */
#define SYSTICK_LOAD_MAX                0x00FFFFFFUL

/* Clock source */
#define SYSTICK_CLKSOURCE_AHB_DIV_8     0U  /*!< HCLK / 8 */
#define SYSTICK_CLKSOURCE_AHB_NO_DIV    1U  /*!< HCLK     */

/* Counter enable */
#define SYSTICK_COUNTER_DISABLED        0U
#define SYSTICK_COUNTER_ENABLED         1U

/* Interrupt enable */
#define SYSTICK_INTERRUPT_DISABLED      0U
#define SYSTICK_INTERRUPT_ENABLED       1U

/* SysTick exception priority: lowest (4 priority bits implemented) */
#define SYSTICK_IRQ_PRIORITY            15U


void     SysTick_Init(void);
void     SysTick_ClearCounterValue(void);
void     SysTick_InterruptConfig(_Bool IsInterruptEnabled);
void     SysTick_SelectClockSource(_Bool ClockSource);
void     SysTick_EnableCounter(void);
void     SysTick_SetReloadValue(uint32_t ReloadValue);
uint32_t getTick(void);
void     Delay_ms(uint32_t ms);

#endif /* INC_STM32F407XX_SYSTICK_H_ */
