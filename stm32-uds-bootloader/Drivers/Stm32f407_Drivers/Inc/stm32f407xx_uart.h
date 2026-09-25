/*
 * stm32f407xx_uart.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  USART/UART driver for STM32F407xx (asynchronous mode).
 *  Reference: RM0090 Rev 21, section 30 (USART).
 *
 *  Note: the GPIO pins (TX/RX in AF mode, e.g. AF7 for USART1..3, AF8 for
 *        UART4/5 and USART6) must be configured with the GPIO driver.
 */

#ifndef INC_STM32F407XX_UART_H_
#define INC_STM32F407XX_UART_H_

#include "stm32f407xx.h"
#include "stm32f407xx_rcc.h"

/**
  * @brief USART Init Structure definition
  */
typedef struct
{
    uint8_t     Mode;           /*!< Specifies whether the Receive or Transmit mode is enabled.
                                     This parameter can be a value of @ref USART_Mode                   */

    uint32_t    BaudRate;       /*!< Specifies the communication baud rate.
                                     This parameter can be a value of @ref USART_BaudRate               */

    uint8_t     WordLength;     /*!< Specifies the number of data bits (including parity) in a frame.
                                     This parameter can be a value of @ref USART_Word_Length            */

    uint8_t     Oversampling;   /*!< Specifies the oversampling mode.
                                     This parameter can be a value of @ref USART_Oversampling           */

    uint8_t     StopBits;       /*!< Specifies the number of stop bits transmitted.
                                     This parameter can be a value of @ref USART_Stop_Bits              */

    uint8_t     ParityControl;  /*!< Specifies the parity control mode.
                                     This parameter can be a value of @ref USART_Parity                 */

    uint8_t     HWFlowControl;  /*!< Specifies the hardware flow control mode.
                                     This parameter can be a value of @ref USART_HW_FlowControl         */
} USART_InitTypeDef;

/**
  * @brief USART Handle Structure definition
  */
typedef struct
{
    USART_TypeDef       *pUSARTx;       /*!< USART registers base address                       */

    USART_InitTypeDef   Init;           /*!< USART communication parameters                     */

    uint8_t             *pTxBuffer;     /*!< Pointer to the Tx transfer buffer                  */

    uint8_t             *pRxBuffer;     /*!< Pointer to the Rx transfer buffer                  */

    volatile uint32_t   TxLen;          /*!< Remaining Tx frames (modified in ISR)              */

    volatile uint32_t   RxLen;          /*!< Remaining Rx frames (modified in ISR)              */

    volatile uint8_t    TxState;        /*!< Tx transfer state, @ref USART_States               */

    volatile uint8_t    RxState;        /*!< Rx transfer state, @ref USART_States               */
} USART_HandleTypeDef;

/** @defgroup USART_Mode USART Mode
  */
#define USART_MODE_TX                   0U
#define USART_MODE_RX                   1U
#define USART_MODE_TX_RX                2U

/** @defgroup USART_BaudRate USART BaudRate
  */
#define USART_BAUDRATE_1200             1200U
#define USART_BAUDRATE_2400             2400U
#define USART_BAUDRATE_9600             9600U
#define USART_BAUDRATE_19200            19200U
#define USART_BAUDRATE_38400            38400U
#define USART_BAUDRATE_57600            57600U
#define USART_BAUDRATE_115200           115200U
#define USART_BAUDRATE_230400           230400U
#define USART_BAUDRATE_460800           460800U
#define USART_BAUDRATE_921600           921600U
#define USART_BAUDRATE_2M               2000000U
#define USART_BAUDRATE_3M               3000000U

/** @defgroup USART_Parity USART Parity
  * @brief    With parity, the MSB of the frame (bit 7 or bit 8) is the parity bit.
  */
#define USART_PARITY_NONE               0U
#define USART_PARITY_EVEN               1U
#define USART_PARITY_ODD                2U

/** @defgroup USART_Word_Length USART Word Length (CR1.M)
  */
#define USART_WORDLENGTH_8BITS          0U
#define USART_WORDLENGTH_9BITS          1U

/** @defgroup USART_Oversampling USART Oversampling (CR1.OVER8)
  */
#define USART_OVER8_DISABLE             0U  /*!< Oversampling by 16 */
#define USART_OVER8_ENABLE              1U  /*!< Oversampling by 8  */

/** @defgroup USART_Stop_Bits USART Number of Stop Bits (CR2.STOP encoding)
  * @brief    0.5 and 1.5 stop bits are not available for UART4 and UART5.
  */
#define USART_STOPBITS_1                0U
#define USART_STOPBITS_0_5              1U
#define USART_STOPBITS_2                2U
#define USART_STOPBITS_1_5              3U

/** @defgroup USART_HW_FlowControl USART Hardware Flow control
  * @brief    Not available for UART4 and UART5.
  */
#define USART_HW_NONE                   0U
#define USART_HW_CTS                    1U
#define USART_HW_RTS                    2U
#define USART_HW_CTS_RTS                3U

/** @defgroup USART_Flags USART Flags (USART_SR masks)
  */
#define USART_FLAG_CTS                  USART_SR_CTS    /*!< rc_w0                               */
#define USART_FLAG_LBD                  USART_SR_LBD    /*!< rc_w0                               */
#define USART_FLAG_TXE                  USART_SR_TXE    /*!< cleared by a write to DR            */
#define USART_FLAG_TC                   USART_SR_TC     /*!< rc_w0                               */
#define USART_FLAG_RXNE                 USART_SR_RXNE   /*!< rc_w0 or cleared by a read of DR    */
#define USART_FLAG_IDLE                 USART_SR_IDLE   /*!< cleared by read SR then read DR     */
#define USART_FLAG_ORE                  USART_SR_ORE    /*!< cleared by read SR then read DR     */
#define USART_FLAG_NE                   USART_SR_NE     /*!< cleared by read SR then read DR     */
#define USART_FLAG_FE                   USART_SR_FE     /*!< cleared by read SR then read DR     */
#define USART_FLAG_PE                   USART_SR_PE     /*!< cleared by read SR then read DR     */

/** @defgroup USART_States USART States
  */
#define USART_STATE_READY               0U
#define USART_STATE_BUSY_TX             1U
#define USART_STATE_BUSY_RX             2U

/** @defgroup USART_Event_Error USART Event and Error
  */
#define USART_EVENT_TX_CMPLT            0U
#define USART_EVENT_RX_CMPLT            1U
#define USART_EVENT_IDLE                2U
#define USART_EVENT_CTS                 3U
#define USART_EVENT_PE                  4U
#define USART_ERR_FE                    5U
#define USART_ERR_NE                    6U
#define USART_ERR_ORE                   7U

/** @defgroup USART_Status USART function return status
  */
#define USART_OK                        0U
#define USART_ERROR                     1U

/******************************************************************************************
 *                              APIs supported by this driver
 *       For more information about the APIs check the function definitions
 ******************************************************************************************/

/*
 * Peripheral Clock setup
 */
void    USART_PeriClockControl(USART_TypeDef *pUSARTx, uint8_t state);

/*
 * Init and De-init
 */
uint8_t USART_Init(USART_HandleTypeDef *husart);
void    USART_DeInit(USART_TypeDef *pUSARTx);

/*
 * Data Send and Receive
 * Len is the number of frames: 1 byte per frame, or 2 bytes (uint16_t) per
 * frame for 9-bit data without parity.
 */
void    USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len);
void    USART_Receive(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len);
uint8_t USART_Transmit_IT(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len);
uint8_t USART_Receive_IT(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len);

/*
 * IRQ Configuration and ISR handling
 */
void    USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state);
void    USART_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority);
void    USART_IRQHandler(USART_HandleTypeDef *husart);

/*
 * Other Peripheral Control APIs
 */
uint8_t USART_GetFlagStatus(USART_TypeDef *pUSARTx, uint32_t FlagName);
void    USART_ClearFlag(USART_TypeDef *pUSARTx, uint32_t FlagName);
void    USART_PeripheralControl(USART_TypeDef *pUSARTx, uint8_t state);
void    USART_SetBaudRate(USART_TypeDef *pUSARTx, uint32_t BaudRate);

/*
 * Application Callbacks
 */
void    USART_ApplicationEventCallback(USART_HandleTypeDef *husart, uint8_t event);

#endif /* INC_STM32F407XX_UART_H_ */
