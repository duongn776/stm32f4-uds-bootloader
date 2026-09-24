/**
 * main.c - Test LED PD12..PD15 tren STM32F407G-DISC1
 * Chi dung CMSIS (stm32f4xx.h), khong dung HAL.
 *
 *   PD12 = LD4 xanh la | PD13 = LD3 cam | PD14 = LD5 do | PD15 = LD6 xanh duong
 *
 * Clock: HSI 16 MHz mac dinh sau reset (SystemCoreClock = 16000000).
 */
#include "stm32f4xx.h"

#define LED_GREEN   12U
#define LED_ORANGE  13U
#define LED_RED     14U
#define LED_BLUE    15U
#define LED_ALL     ((1UL << LED_GREEN) | (1UL << LED_ORANGE) | (1UL << LED_RED) | (1UL << LED_BLUE))

/* Delay chinh xac theo ms bang SysTick (polling, chua dung ngat) */
static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;   /* 1 ms */
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (ms-- > 0U) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) { }
    }
    SysTick->CTRL = 0U;
}

static void led_init(void)
{
    /* 1. Bat clock GPIOD (RCC_AHB1ENR bit 3) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;                                /* doc lai de cho clock on dinh */

    /* 2. PD12..PD15: output (MODER = 01) */
    GPIOD->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 |
                      GPIO_MODER_MODER14 | GPIO_MODER_MODER15);
    GPIOD->MODER |=  (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 |
                      GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0);

    /* 3. Push-pull, toc do thap, khong pull-up/down */
    GPIOD->OTYPER  &= ~LED_ALL;
    GPIOD->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR12 | GPIO_OSPEEDER_OSPEEDR13 |
                        GPIO_OSPEEDER_OSPEEDR14 | GPIO_OSPEEDER_OSPEEDR15);
    GPIOD->PUPDR   &= ~(GPIO_PUPDR_PUPDR12 | GPIO_PUPDR_PUPDR13 |
                        GPIO_PUPDR_PUPDR14 | GPIO_PUPDR_PUPDR15);

    /* 4. Tat het LED: BSRR nua cao = reset */
    GPIOD->BSRR = LED_ALL << 16;
}

static void led_on(uint32_t pin)  { GPIOD->BSRR = (1UL << pin); }          /* set   */
static void led_off(uint32_t pin) { GPIOD->BSRR = (1UL << (pin + 16U)); }  /* reset */

int main(void)
{
    const uint32_t order[4] = { LED_GREEN, LED_ORANGE, LED_RED, LED_BLUE };

    led_init();

    while (1) {
        /* Test 1: chay vong tung LED */
        for (uint32_t i = 0; i < 4U; i++) {
            led_on(order[i]);
            delay_ms(200);
            led_off(order[i]);
        }

        /* Test 2: bat / tat tat ca 3 lan */
        for (uint32_t i = 0; i < 3U; i++) {
            GPIOD->BSRR = LED_ALL;           /* bat het */
            delay_ms(300);
            GPIOD->BSRR = LED_ALL << 16;     /* tat het */
            delay_ms(300);
        }
    }
}
