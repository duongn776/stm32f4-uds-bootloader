/**
 * main.c - Test GPIO driver (stm32f407xx_gpio) tren STM32F407G-DISC1
 *
 *   PD12 = LD4 xanh la | PD13 = LD3 cam | PD14 = LD5 do | PD15 = LD6 xanh duong
 *   PA0  = B1 (nut USER, co dien tro pull-down ngoai, nhan = muc 1)
 *
 * Clock: HSI 16 MHz mac dinh sau reset (SystemCoreClock = 16000000).
 *
 * Kich ban test (lap vo han):
 *   Test 1: GPIO_WritePin  - chay vong tung LED
 *   Test 2: GPIO_ReadPin   - ghi roi doc lai tung LED, sai -> LED do sang mai
 *   Test 3: GPIO_TogglePin - nhay ca 4 LED 3 lan
 *   Test 4: EXTI (nut B1)  - moi lan nhan nut, LED xanh duong doi trang thai
 *                            (chay song song voi cac test tren qua ngat EXTI0)
 *
 * Delay dung driver SysTick (ngat 1 ms, Delay_ms/getTick).
 */
#include "stm32f407xx_gpio.h"
#include "stm32f407xx_systick.h"

#define LED_PORT    GPIOD
#define LED_GREEN   GPIO_PIN_12
#define LED_ORANGE  GPIO_PIN_13
#define LED_RED     GPIO_PIN_14
#define LED_BLUE    GPIO_PIN_15

#define BTN_PORT    GPIOA
#define BTN_PIN     GPIO_PIN_0

static volatile uint32_t btnCount = 0U;   /* so lan nhan nut, xem trong debugger */

/* Loi khong phuc hoi: tat het, LED do sang mai */
static void error_handler(void)
{
    __disable_irq();
    GPIO_WritePin(LED_PORT, LED_GREEN,  GPIO_PIN_RESET);
    GPIO_WritePin(LED_PORT, LED_ORANGE, GPIO_PIN_RESET);
    GPIO_WritePin(LED_PORT, LED_BLUE,   GPIO_PIN_RESET);
    GPIO_WritePin(LED_PORT, LED_RED,    GPIO_PIN_SET);
    while (1) { }
}

static void led_init(void)
{
    GPIO_HandleTypeDef hLed = {
        .pGPIOx = LED_PORT,
        .Init = {
            .Mode   = GPIO_MODE_OUTPUT,
            .OPType = GPIO_OPTYPE_PP,
            .Pull   = GPIO_NOPULL,
            .Speed  = GPIO_SPEED_LOW,
        },
    };
    const uint8_t pins[] = { LED_GREEN, LED_ORANGE, LED_RED, LED_BLUE };

    for (uint32_t i = 0U; i < sizeof(pins); i++)
    {
        hLed.Init.Pin = pins[i];
        if (GPIO_Init(&hLed) != GPIO_OK)
        {
            error_handler();
        }
        GPIO_WritePin(LED_PORT, pins[i], GPIO_PIN_RESET);
    }
}

static void button_init(void)
{
    GPIO_HandleTypeDef hBtn = {
        .pGPIOx = BTN_PORT,
        .Init = {
            .Pin  = BTN_PIN,
            .Mode = GPIO_MODE_IT_RISING,   /* nhan nut: 0 -> 1 */
            .Pull = GPIO_NOPULL,           /* board da co pull-down ngoai */
        },
    };

    if (GPIO_Init(&hBtn) != GPIO_OK)
    {
        error_handler();
    }

    GPIO_IRQPriorityConfig(EXTI0_IRQn, 15U);
    GPIO_IRQInterruptConfig(EXTI0_IRQn, ENABLE);
}

/* Vector EXTI0 (ten ham trung voi startup_stm32f407vgtx.s) */
void EXTI0_IRQHandler(void)
{
    GPIO_IRQHandler(BTN_PIN);
}

/* Override ham weak trong driver */
void GPIO_EXTI_Callback(uint8_t GPIO_pin)
{
    if (GPIO_pin == BTN_PIN)
    {
        btnCount++;
        GPIO_TogglePin(LED_PORT, LED_BLUE);
    }
}

int main(void)
{
    const uint8_t order[] = { LED_GREEN, LED_ORANGE, LED_RED };

    SysTick_Init();   /* time base 1 ms, phai goi truoc Delay_ms */
    led_init();
    button_init();

    while (1)
    {
        /* Test 1: GPIO_WritePin - chay vong tung LED (khong dung LED xanh duong vi danh cho nut) */
        for (uint32_t i = 0U; i < sizeof(order); i++)
        {
            GPIO_WritePin(LED_PORT, order[i], GPIO_PIN_SET);
            Delay_ms(200);
            GPIO_WritePin(LED_PORT, order[i], GPIO_PIN_RESET);
        }

        /* Test 2: GPIO_ReadPin - o che do output, IDR phan anh muc thuc te tren chan */
        for (uint32_t i = 0U; i < sizeof(order); i++)
        {
            GPIO_WritePin(LED_PORT, order[i], GPIO_PIN_SET);
            Delay_ms(1);   /* IDR can vai chu ky dong bo sau khi ghi */
            if (GPIO_ReadPin(LED_PORT, order[i]) != GPIO_PIN_SET)
            {
                error_handler();
            }

            GPIO_WritePin(LED_PORT, order[i], GPIO_PIN_RESET);
            Delay_ms(1);
            if (GPIO_ReadPin(LED_PORT, order[i]) != GPIO_PIN_RESET)
            {
                error_handler();
            }
        }

        /* Test 3: GPIO_TogglePin - nhay 3 LED 3 lan, LED xanh duong khong bi anh huong */
        for (uint32_t i = 0U; i < 6U; i++)
        {
            GPIO_TogglePin(LED_PORT, LED_GREEN);
            GPIO_TogglePin(LED_PORT, LED_ORANGE);
            GPIO_TogglePin(LED_PORT, LED_RED);
            Delay_ms(300);
        }
    }
}
