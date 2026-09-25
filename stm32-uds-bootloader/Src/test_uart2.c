/**
 * test_uart2.c - Test USART2 driver tren STM32F407G-DISC1
 *
 * Ket noi (dung USB-UART 3.3V, vd CP2102/CH340/FT232):
 *   PA2 (USART2_TX, AF7) ---> RX cua USB-UART
 *   PA3 (USART2_RX, AF7) <--- TX cua USB-UART
 *   GND                  ---- GND
 * Terminal (PuTTY, Tera Term, ...): 115200 baud, 8 data, no parity, 1 stop, no flow control.
 *
 * Kich ban test:
 *   Test 1: USART_Transmit (blocking) - in banner, PCLK1 va BRR
 *   Test 2: USART_Receive_IT + USART_Transmit_IT - echo lai moi ky tu go tu terminal
 *           (Enter -> xuong dong), LED xanh la doi trang thai moi byte nhan.
 *           Byte nhan duoc dua vao ring buffer ngay trong callback va Receive_IT
 *           duoc goi lai trong ngat, nen paste chuoi dai cung khong mat byte.
 *   Test 3: moi 1 s in "tick=<ms> rx=<n> tx=<n> err=<n>" (blocking, xen ke voi echo)
 *   Loi (ORE/FE/NE/PE) -> LED do sang, errCount tang
 *   LED cam doi trang thai moi lan TX interrupt hoan tat
 *
 * Clock: HSI 16 MHz, APB1 khong chia -> PCLK1 = 16 MHz, BRR = 0x8B (USARTDIV 8.6875).
 */
#include <stddef.h>
#include "stm32f407xx_drivers.h"
#include "test_uart2.h"

#define UART2_PORT      GPIOA
#define UART2_TX_PIN    GPIO_PIN_2
#define UART2_RX_PIN    GPIO_PIN_3

#define LED_PORT        GPIOD
#define LED_GREEN       GPIO_PIN_12
#define LED_ORANGE      GPIO_PIN_13
#define LED_RED         GPIO_PIN_14

static USART_HandleTypeDef hUart2;

#define RX_RING_SIZE    64U                   /* luy thua cua 2 */

static uint8_t rxByte;                        /* buffer nhan 1 byte (interrupt)  */
static uint8_t txBuf[2];                      /* buffer echo (interrupt)         */

static volatile uint8_t  rxRing[RX_RING_SIZE];
static volatile uint32_t rxHead   = 0U;       /* ghi trong ngat                  */
static volatile uint32_t rxTail   = 0U;       /* doc trong vong lap chinh        */
static volatile uint32_t rxCount  = 0U;       /* xem trong debugger              */
static volatile uint32_t txCount  = 0U;
static volatile uint32_t errCount = 0U;

/* Loi khong phuc hoi: LED do sang mai */
static void test_error(void)
{
    __disable_irq();
    GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
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
    const uint8_t pins[] = { LED_GREEN, LED_ORANGE, LED_RED };

    for (uint32_t i = 0U; i < sizeof(pins); i++)
    {
        hLed.Init.Pin = pins[i];
        (void)GPIO_Init(&hLed);
        GPIO_WritePin(LED_PORT, pins[i], GPIO_PIN_RESET);
    }
}

/* PA2 = USART2_TX, PA3 = USART2_RX, AF7 (datasheet, bang alternate function) */
static void uart2_gpio_init(void)
{
    GPIO_HandleTypeDef hPin = {
        .pGPIOx = UART2_PORT,
        .Init = {
            .Mode      = GPIO_MODE_AF,
            .OPType    = GPIO_OPTYPE_PP,
            .Speed     = GPIO_SPEED_VERY_HIGH,
            .Alternate = GPIO_AF7,
        },
    };

    hPin.Init.Pin  = UART2_TX_PIN;
    hPin.Init.Pull = GPIO_NOPULL;
    if (GPIO_Init(&hPin) != GPIO_OK)
    {
        test_error();
    }

    /* Pull-up tren RX: duong truyen o muc 1 (idle) khi chua cam USB-UART */
    hPin.Init.Pin  = UART2_RX_PIN;
    hPin.Init.Pull = GPIO_PULLUP;
    if (GPIO_Init(&hPin) != GPIO_OK)
    {
        test_error();
    }
}

static void uart2_init(void)
{
    hUart2.pUSARTx            = USART2;
    hUart2.Init.Mode          = USART_MODE_TX_RX;
    hUart2.Init.BaudRate      = USART_BAUDRATE_115200;
    hUart2.Init.WordLength    = USART_WORDLENGTH_8BITS;
    hUart2.Init.Oversampling  = USART_OVER8_DISABLE;
    hUart2.Init.StopBits      = USART_STOPBITS_1;
    hUart2.Init.ParityControl = USART_PARITY_NONE;
    hUart2.Init.HWFlowControl = USART_HW_NONE;

    if (USART_Init(&hUart2) != USART_OK)
    {
        test_error();
    }

    /* Bao loi FE/NE/ORE qua ngat (CR3.EIE) */
    USART2->CR3 |= USART_CR3_EIE;

    USART_IRQPriorityConfig(USART2_IRQn, 10U);
    USART_IRQInterruptConfig(USART2_IRQn, ENABLE);
}

/* Gui chuoi blocking, cho TX interrupt (neu co) xong truoc de khong tranh buffer */
static void uart2_puts(const char *str)
{
    uint32_t len = 0U;

    while (str[len] != '\0')
    {
        len++;
    }

    while (hUart2.TxState != USART_STATE_READY)
    {
    }

    USART_Transmit(&hUart2, (uint8_t *)str, len);
}

/* In so nguyen khong dau (thap phan), khong dung printf */
static void uart2_put_u32(uint32_t value)
{
    char buf[11];
    uint32_t i = sizeof(buf) - 1U;

    buf[i] = '\0';
    do
    {
        buf[--i] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && i > 0U);

    uart2_puts(&buf[i]);
}

/* In so hex 4 chu so, vd 0x008B */
static void uart2_put_hex16(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[7] = { '0', 'x', 0, 0, 0, 0, '\0' };

    for (uint32_t i = 0U; i < 4U; i++)
    {
        buf[5U - i] = hex[value & 0xFU];
        value >>= 4U;
    }

    uart2_puts(buf);
}

/* Vector USART2 (ten ham trung voi startup_stm32f407vgtx.s) */
void USART2_IRQHandler(void)
{
    USART_IRQHandler(&hUart2);
}

/* Override ham weak trong driver, chay trong ngu canh ngat */
void USART_ApplicationEventCallback(USART_HandleTypeDef *husart, uint8_t event)
{
    if (husart != &hUart2)
    {
        return;
    }

    switch (event)
    {
        case USART_EVENT_RX_CMPLT:
            rxCount++;
            if ((rxHead - rxTail) < RX_RING_SIZE)
            {
                rxRing[rxHead % RX_RING_SIZE] = rxByte;
                rxHead++;
            }
            else
            {
                errCount++;   /* ring buffer day */
            }
            /* Nhan byte tiep theo ngay trong ngat (RxState da ve READY) */
            (void)USART_Receive_IT(husart, &rxByte, 1U);
            GPIO_TogglePin(LED_PORT, LED_GREEN);
            break;

        case USART_EVENT_TX_CMPLT:
            txCount++;
            GPIO_TogglePin(LED_PORT, LED_ORANGE);
            break;

        case USART_ERR_ORE:
        case USART_ERR_FE:
        case USART_ERR_NE:
        case USART_EVENT_PE:
            errCount++;
            GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
            break;

        default:
            break;
    }
}

void Test_UART2_Run(void)
{
    uint32_t lastPrint;

    led_init();
    uart2_gpio_init();
    uart2_init();

    /* Test 1: blocking transmit */
    uart2_puts("\r\n==== STM32F407 USART2 test (PA2 TX / PA3 RX) ====\r\n");
    uart2_puts("PCLK1 = ");
    uart2_put_u32(RCC_GetPCLK1_Value());
    uart2_puts(" Hz, BRR = ");
    uart2_put_hex16(USART2->BRR);
    uart2_puts("\r\nGo ky tu bat ky, board se echo lai.\r\n");

    /* Test 2: bat dau nhan 1 byte bang interrupt */
    if (USART_Receive_IT(&hUart2, &rxByte, 1U) != USART_STATE_READY)
    {
        test_error();
    }

    lastPrint = getTick();

    while (1)
    {
        /* Test 2: echo */
        if (rxTail != rxHead)
        {
            uint32_t len = 1U;
            uint8_t  ch  = rxRing[rxTail % RX_RING_SIZE];

            rxTail++;

            /* Doi lan echo truoc xong roi moi ghi de txBuf */
            while (hUart2.TxState != USART_STATE_READY)
            {
            }

            txBuf[0] = ch;
            if (ch == '\r')
            {
                txBuf[1] = '\n';
                len = 2U;
            }
            (void)USART_Transmit_IT(&hUart2, txBuf, len);
        }

        /* Test 3: in trang thai moi 1 s */
        if ((uint32_t)(getTick() - lastPrint) >= 1000U)
        {
            lastPrint += 1000U;

            uart2_puts("tick=");
            uart2_put_u32(getTick());
            uart2_puts(" rx=");
            uart2_put_u32(rxCount);
            uart2_puts(" tx=");
            uart2_put_u32(txCount);
            uart2_puts(" err=");
            uart2_put_u32(errCount);
            uart2_puts("\r\n");
        }
    }
}
