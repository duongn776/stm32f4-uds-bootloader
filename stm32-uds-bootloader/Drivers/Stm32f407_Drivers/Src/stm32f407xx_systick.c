/*
 * stm32f407xx_systick.c
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  SysTick driver for STM32F407xx: 1 ms time base (tick counter + blocking delay).
 *  Reference: Cortex-M4 Generic User Guide 4.4 (SysTick), 4.3.10 (SHPR3),
 *             RM0090 6.2 (SysTick clock = HCLK/8 or HCLK).
 */

#include "stm32f407xx_systick.h"

/* Millisecond counter, incremented by SysTick_Handler.
 * volatile: modified in ISR, without it the compiler turns Delay_ms into an endless loop. */
static volatile uint32_t ticks = 0U;

/**
  * @brief  Sets the SysTick reload value.
  * @note   Valid range is 1..0x00FFFFFF, a value of 0 has no effect
  *         (Cortex-M4 Generic User Guide 4.4.2). Out of range values are clamped.
  * @param  ReloadValue value to load into STK_LOAD.
  * @retval None
  */
void SysTick_SetReloadValue(uint32_t ReloadValue)
{
    if (ReloadValue == 0U)
    {
        ReloadValue = 1U;
    }
    else if (ReloadValue > SYSTICK_LOAD_MAX)
    {
        ReloadValue = SYSTICK_LOAD_MAX;
    }

    SYSTICK->STK_LOAD = ReloadValue;
}

/**
  * @brief  Enables the SysTick counter.
  * @retval None
  */
void SysTick_EnableCounter(void)
{
    SYSTICK->STK_CTRL |= (1U << SYSTICK_CTRL_ENABLE_POS);
}

/**
  * @brief  Selects the SysTick clock source.
  * @param  ClockSource SYSTICK_CLKSOURCE_AHB_DIV_8 (HCLK/8) or
  *         SYSTICK_CLKSOURCE_AHB_NO_DIV (HCLK).
  * @retval None
  */
void SysTick_SelectClockSource(_Bool ClockSource)
{
    SYSTICK->STK_CTRL &= ~(1U << SYSTICK_CTRL_CLKSOURCE_POS);
    SYSTICK->STK_CTRL |=  ((uint32_t)ClockSource << SYSTICK_CTRL_CLKSOURCE_POS);
}

/**
  * @brief  Enables or disables the SysTick exception request.
  * @param  IsInterruptEnabled SYSTICK_INTERRUPT_ENABLED or SYSTICK_INTERRUPT_DISABLED.
  * @retval None
  */
void SysTick_InterruptConfig(_Bool IsInterruptEnabled)
{
    SYSTICK->STK_CTRL &= ~(1U << SYSTICK_CTRL_TICKINT_POS);
    SYSTICK->STK_CTRL |=  ((uint32_t)IsInterruptEnabled << SYSTICK_CTRL_TICKINT_POS);
}

/**
  * @brief  Clears the SysTick current value.
  * @note   Writing any value clears STK_VAL to 0 and also clears COUNTFLAG
  *         (Cortex-M4 Generic User Guide 4.4.3).
  * @retval None
  */
void SysTick_ClearCounterValue(void)
{
    SYSTICK->STK_VAL = 0U;
}

/**
  * @brief  Initializes SysTick to generate an interrupt every 1 ms.
  * @note   Clock source is HCLK/8. The reload value is computed from
  *         SystemCoreClock, so call SystemCoreClockUpdate() and then this
  *         function again after changing the system clock (e.g. PLL).
  * @retval None
  */
void SysTick_Init(void)
{
    uint32_t clockFreq = SystemCoreClock / 8U;   /* SysTick input clock (HCLK/8) */

    /* Stop the counter while configuring */
    SYSTICK->STK_CTRL = 0U;

    SysTick_SetReloadValue((clockFreq / 1000U) - 1U);

    /* STK_VAL reset value is unknown: clear it so the first tick is exactly 1 ms */
    SysTick_ClearCounterValue();

    /* Lowest priority, so SysTick does not preempt the other interrupts */
    *SYSTICK_SHPR3 &= ~(0xFFU << SYSTICK_SHPR3_PRI_POS);
    *SYSTICK_SHPR3 |=  ((SYSTICK_IRQ_PRIORITY << (8U - __NVIC_PRIO_BITS)) << SYSTICK_SHPR3_PRI_POS);

    SysTick_SelectClockSource(SYSTICK_CLKSOURCE_AHB_DIV_8);
    SysTick_InterruptConfig(SYSTICK_INTERRUPT_ENABLED);
    SysTick_EnableCounter();
}

/**
  * @brief  Returns the number of milliseconds since SysTick_Init().
  * @retval Current tick value (ms), wraps around after ~49.7 days.
  */
uint32_t getTick(void)
{
    return ticks;
}

/**
  * @brief  SysTick exception handler, called every 1 ms.
  * @retval None
  */
void SysTick_Handler(void)
{
    ticks++;
}

/**
  * @brief  Blocking delay in milliseconds.
  * @note   One tick is added so the delay is at least ms milliseconds even if
  *         it starts just before a tick. The unsigned subtraction handles the
  *         wrap-around of the tick counter.
  * @param  ms number of milliseconds to wait.
  * @retval None
  */
void Delay_ms(uint32_t ms)
{
    uint32_t start = getTick();
    uint32_t wait  = ms;

    if (wait < 0xFFFFFFFFU)
    {
        wait++;
    }

    while ((uint32_t)(getTick() - start) < wait)
    {
    }
}
