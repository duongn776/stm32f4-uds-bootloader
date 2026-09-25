/*
 * stm32f407xx_gpio.c
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduong
 *
 *  GPIO driver for STM32F407xx (register level).
 *  Reference: RM0090 Rev 21, section 8 (GPIO), 9.2 (SYSCFG), 12 (EXTI).
 */

#include <stddef.h>
#include "stm32f407xx_gpio.h"

/* Number of bits of one pin field in MODER/OSPEEDR/PUPDR, and in AFRL/AFRH / EXTICR */
#define GPIO_2BIT_FIELD_MASK        0x3U
#define GPIO_4BIT_FIELD_MASK        0xFU

/**
  * @brief  Enables or disables the peripheral clock of the given GPIO port.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  clockState ENABLE or DISABLE.
  * @retval None
  */
void GPIO_PeriClockControl(GPIO_TypeDef *pGPIOx, uint8_t clockState)
{
    if (clockState == ENABLE)
    {
        if (pGPIOx == GPIOA)
        {
            GPIOA_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOB)
        {
            GPIOB_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOC)
        {
            GPIOC_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOD)
        {
            GPIOD_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOE)
        {
            GPIOE_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOF)
        {
            GPIOF_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOG)
        {
            GPIOG_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOH)
        {
            GPIOH_CLK_ENABLE();
        }
        else if (pGPIOx == GPIOI)
        {
            GPIOI_CLK_ENABLE();
        }

        /* Errata ES0182 2.1.13: dummy read to wait 2 cycles after enabling the clock */
        (void)RCC->AHB1ENR;
    }
    else
    {
        if (pGPIOx == GPIOA)
        {
            GPIOA_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOB)
        {
            GPIOB_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOC)
        {
            GPIOC_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOD)
        {
            GPIOD_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOE)
        {
            GPIOE_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOF)
        {
            GPIOF_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOG)
        {
            GPIOG_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOH)
        {
            GPIOH_CLK_DISABLE();
        }
        else if (pGPIOx == GPIOI)
        {
            GPIOI_CLK_DISABLE();
        }
    }
}

/**
  * @brief  Initializes one GPIO pin according to the parameters in hGPIO->Init.
  * @note   The port clock is enabled by this function.
  * @param  hGPIO pointer to a GPIO_HandleTypeDef structure that contains
  *         the port base address and the pin configuration.
  * @retval GPIO_OK on success, GPIO_ERROR if a parameter is invalid.
  */
uint8_t GPIO_Init(GPIO_HandleTypeDef *hGPIO)
{
    GPIO_TypeDef *pGPIOx;
    uint32_t pin;
    uint32_t pinMask;

    if (hGPIO == NULL || hGPIO->pGPIOx == NULL)
    {
        return GPIO_ERROR;
    }

    if (!IS_GPIO_PIN(hGPIO->Init.Pin)       || !IS_GPIO_MODE(hGPIO->Init.Mode)   ||
        !IS_GPIO_PULL(hGPIO->Init.Pull)     || !IS_GPIO_SPEED(hGPIO->Init.Speed) ||
        !IS_GPIO_OPTYPE(hGPIO->Init.OPType) || !IS_GPIO_AF(hGPIO->Init.Alternate))
    {
        return GPIO_ERROR;
    }

    pGPIOx  = hGPIO->pGPIOx;
    pin     = hGPIO->Init.Pin;
    pinMask = (1U << pin);

    /* 1. Enable the port clock */
    GPIO_PeriClockControl(pGPIOx, ENABLE);

    /* 2. Configure the mode (MODER, 2 bits per pin) */
    pGPIOx->MODER &= ~(GPIO_2BIT_FIELD_MASK << (2U * pin));

    if (hGPIO->Init.Mode <= GPIO_MODE_ANALOG)
    {
        /* Non interrupt mode: Mode value matches the MODER encoding */
        pGPIOx->MODER |= ((uint32_t)hGPIO->Init.Mode << (2U * pin));

        /* Make sure the EXTI line is no longer active for this pin */
        EXTI->IMR  &= ~pinMask;
        EXTI->RTSR &= ~pinMask;
        EXTI->FTSR &= ~pinMask;
    }
    else
    {
        /* Interrupt mode: the pin stays in input mode (MODER = 00, cleared above) */
        uint32_t exticrIdx = pin / 4U;   /* EXTICR[0..3]           */
        uint32_t exticrPos = pin % 4U;   /* field in EXTICR (4 bit) */
        uint32_t portCode  = GPIO_BASEADDR_TO_CODE(pGPIOx);

        /* 2.1 Select the trigger edge (RTSR/FTSR) */
        if (hGPIO->Init.Mode == GPIO_MODE_IT_FALLING)
        {
            EXTI->FTSR |=  pinMask;
            EXTI->RTSR &= ~pinMask;
        }
        else if (hGPIO->Init.Mode == GPIO_MODE_IT_RISING)
        {
            EXTI->RTSR |=  pinMask;
            EXTI->FTSR &= ~pinMask;
        }
        else /* GPIO_MODE_IT_RISING_FALLING */
        {
            EXTI->RTSR |= pinMask;
            EXTI->FTSR |= pinMask;
        }

        /* 2.2 Connect the port to the EXTI line (SYSCFG_EXTICRx) */
        SYSCFG_CLK_ENABLE();
        (void)RCC->APB2ENR;   /* Errata ES0182 2.1.13 */

        SYSCFG->EXTICR[exticrIdx] &= ~(GPIO_4BIT_FIELD_MASK << (4U * exticrPos));
        SYSCFG->EXTICR[exticrIdx] |=  (portCode << (4U * exticrPos));

        /* 2.3 Clear a stale pending flag (rc_w1), then unmask the interrupt line */
        EXTI->PR   =  pinMask;
        EXTI->IMR |=  pinMask;
    }

    /* 3. Configure the output speed (OSPEEDR, 2 bits per pin) */
    pGPIOx->OSPEEDR &= ~(GPIO_2BIT_FIELD_MASK << (2U * pin));
    pGPIOx->OSPEEDR |=  ((uint32_t)hGPIO->Init.Speed << (2U * pin));

    /* 4. Configure the pull-up/pull-down (PUPDR, 2 bits per pin) */
    pGPIOx->PUPDR &= ~(GPIO_2BIT_FIELD_MASK << (2U * pin));
    pGPIOx->PUPDR |=  ((uint32_t)hGPIO->Init.Pull << (2U * pin));

    /* 5. Configure the output type (OTYPER, 1 bit per pin) */
    pGPIOx->OTYPER &= ~pinMask;
    pGPIOx->OTYPER |=  ((uint32_t)hGPIO->Init.OPType << pin);

    /* 6. Configure the alternate function (AFRL for pin 0..7, AFRH for pin 8..15, 4 bits per pin) */
    if (hGPIO->Init.Mode == GPIO_MODE_AF)
    {
        uint32_t afrIdx = pin / 8U;
        uint32_t afrPos = pin % 8U;

        pGPIOx->AFR[afrIdx] &= ~(GPIO_4BIT_FIELD_MASK << (4U * afrPos));
        pGPIOx->AFR[afrIdx] |=  ((uint32_t)hGPIO->Init.Alternate << (4U * afrPos));
    }

    return GPIO_OK;
}

/**
  * @brief  De-initializes the GPIOx peripheral registers to their reset values
  *         using RCC_AHB1RSTR.
  * @note   EXTI and SYSCFG configuration are not modified.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @retval None
  */
void GPIO_DeInit(GPIO_TypeDef *pGPIOx)
{
    if (pGPIOx == GPIOA)
    {
        GPIOA_REG_RESET();
    }
    else if (pGPIOx == GPIOB)
    {
        GPIOB_REG_RESET();
    }
    else if (pGPIOx == GPIOC)
    {
        GPIOC_REG_RESET();
    }
    else if (pGPIOx == GPIOD)
    {
        GPIOD_REG_RESET();
    }
    else if (pGPIOx == GPIOE)
    {
        GPIOE_REG_RESET();
    }
    else if (pGPIOx == GPIOF)
    {
        GPIOF_REG_RESET();
    }
    else if (pGPIOx == GPIOG)
    {
        GPIOG_REG_RESET();
    }
    else if (pGPIOx == GPIOH)
    {
        GPIOH_REG_RESET();
    }
    else if (pGPIOx == GPIOI)
    {
        GPIOI_REG_RESET();
    }
}

/**
  * @brief  Reads the specified input pin (IDR).
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  GPIO_pin specifies the pin to read.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @retval Pin state: GPIO_PIN_RESET (0) or GPIO_PIN_SET (1).
  */
uint8_t GPIO_ReadPin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin)
{
    return (uint8_t)((pGPIOx->IDR >> GPIO_pin) & 0x1U);
}

/**
  * @brief  Reads the entire input data register of the specified GPIO port.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @retval The 16-bit input port value (IDR).
  */
uint16_t GPIO_ReadPort(GPIO_TypeDef *pGPIOx)
{
    return (uint16_t)pGPIOx->IDR;
}

/**
  * @brief  Sets or clears the selected GPIO pin.
  * @note   Uses BSRR for an atomic write, other pins of the port are not affected
  *         (RM0090 8.4.7).
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  GPIO_pin specifies the pin to be written.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @param  pinState specifies the value to be written to the selected pin.
  *         This parameter can be one of the values:
  *           @arg GPIO_PIN_RESET: to clear the pin
  *           @arg GPIO_PIN_SET: to set the pin
  * @retval None
  */
void GPIO_WritePin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin, uint8_t pinState)
{
    if (pinState == GPIO_PIN_SET)
    {
        pGPIOx->BSRR = (1U << GPIO_pin);           /* BSy: set ODRy   */
    }
    else
    {
        pGPIOx->BSRR = (1U << (GPIO_pin + 16U));   /* BRy: reset ODRy */
    }
}

/**
  * @brief  Writes a 16-bit value to the output data register of the whole port.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  value 16-bit value written to ODR (bit y drives pin y).
  * @retval None
  */
void GPIO_WritePort(GPIO_TypeDef *pGPIOx, uint16_t value)
{
    pGPIOx->ODR = value;
}

/**
  * @brief  Toggles the specified GPIO pin.
  * @note   Uses BSRR so the write is atomic even if an interrupt modifies
  *         other pins of the same port.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  GPIO_pin specifies the pin to be toggled.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @retval None
  */
void GPIO_TogglePin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin)
{
    uint32_t pinMask = (1U << GPIO_pin);
    uint32_t odr     = pGPIOx->ODR;

    /* Pin currently high -> reset it (BRy), pin currently low -> set it (BSy) */
    pGPIOx->BSRR = ((odr & pinMask) << 16U) | (~odr & pinMask);
}

/**
  * @brief  Locks the configuration of the specified pin (RM0090 8.4.8).
  * @note   Once locked, MODER/OTYPER/OSPEEDR/PUPDR/AFR of this pin cannot be
  *         modified until the next MCU or peripheral reset.
  * @param  pGPIOx where x can be (A..I) to select the GPIO peripheral.
  * @param  GPIO_pin specifies the pin to be locked.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @retval GPIO_OK if the lock is active, GPIO_ERROR otherwise.
  */
uint8_t GPIO_LockPin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin)
{
    const uint32_t lckk = (1U << 16U);
    uint32_t tmp = lckk | (1U << GPIO_pin);

    /* LOCK key write sequence, LCKR[15:0] must not change during the sequence */
    pGPIOx->LCKR = tmp;                 /* WR LCKR[16] = 1 + LCKR[15:0] */
    pGPIOx->LCKR = (1U << GPIO_pin);    /* WR LCKR[16] = 0 + LCKR[15:0] */
    pGPIOx->LCKR = tmp;                 /* WR LCKR[16] = 1 + LCKR[15:0] */
    (void)pGPIOx->LCKR;                 /* RD LCKR                      */

    /* RD LCKR[16] = 1 confirms the lock is active */
    return ((pGPIOx->LCKR & lckk) != 0U) ? GPIO_OK : GPIO_ERROR;
}

/**
  * @brief  Enables or disables the specified IRQ in the NVIC.
  * @note   ISERx/ICERx are write-1-to-set/clear: writing 0 has no effect, so a
  *         plain write is used (a read-modify-write on ICERx would disable
  *         every IRQ currently enabled in that register).
  * @param  IRQNumber specifies the IRQ number (0..81 on STM32F407),
  *         e.g. EXTI0_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn.
  * @param  state ENABLE or DISABLE.
  * @retval None
  */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state)
{
    uint32_t bit = (1U << (IRQNumber % 32U));

    if (state == ENABLE)
    {
        if (IRQNumber < 32U)
        {
            *NVIC_ISER0 = bit;
        }
        else if (IRQNumber < 64U)
        {
            *NVIC_ISER1 = bit;
        }
        else if (IRQNumber < 96U)
        {
            *NVIC_ISER2 = bit;
        }
    }
    else
    {
        if (IRQNumber < 32U)
        {
            *NVIC_ICER0 = bit;
        }
        else if (IRQNumber < 64U)
        {
            *NVIC_ICER1 = bit;
        }
        else if (IRQNumber < 96U)
        {
            *NVIC_ICER2 = bit;
        }
    }
}

/**
  * @brief  Configures the priority of an IRQ in the NVIC.
  * @note   Each IPRx register holds 4 IRQs (8 bits each). Only the upper
  *         NO_PR_BITS_IMPLEMENTED (4) bits of each byte are implemented.
  * @param  IRQNumber specifies the IRQ number.
  * @param  IRQPriority specifies the priority level (0..15, lower value = higher priority).
  * @retval None
  */
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
    uint32_t iprx        = IRQNumber / 4U;
    uint32_t iprSection  = IRQNumber % 4U;
    uint32_t shiftAmount = (8U * iprSection) + (8U - NO_PR_BITS_IMPLEMENTED);
    uint32_t prio        = (uint32_t)IRQPriority & ((1U << NO_PR_BITS_IMPLEMENTED) - 1U);

    *(NVIC_PR_BASEADDR + iprx) &= ~(0xFFU << (8U * iprSection));
    *(NVIC_PR_BASEADDR + iprx) |=  (prio << shiftAmount);
}

/**
  * @brief  Handles the EXTI interrupt of a specific pin.
  * @note   Call this function from the EXTIx_IRQHandler of the vector table.
  *         EXTI_PR is rc_w1: only the bit of this pin is written, so pending
  *         flags of other lines sharing the same vector are kept.
  * @param  GPIO_pin specifies the pin number connected to the EXTI line.
  * @retval None
  */
void GPIO_IRQHandler(uint8_t GPIO_pin)
{
    uint32_t pinMask = (1U << GPIO_pin);

    if ((EXTI->PR & pinMask) != 0U)
    {
        EXTI->PR = pinMask;   /* clear the pending flag */
        GPIO_EXTI_Callback(GPIO_pin);
    }
}

/**
  * @brief  EXTI line detection callback.
  * @note   This function must not be modified here, when the callback is
  *         needed it can be implemented in the user file (weak symbol).
  * @param  GPIO_pin specifies the pin number connected to the EXTI line.
  * @retval None
  */
__WEAK void GPIO_EXTI_Callback(uint8_t GPIO_pin)
{
    (void)GPIO_pin;
}
