/*
 * stm32f407xx_uart.c
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  USART/UART driver for STM32F407xx (asynchronous mode).
 *  Reference: RM0090 Rev 21, section 30 (USART).
 */

#include <stddef.h>
#include "stm32f407xx_uart.h"

static void USART_WriteFrame(USART_HandleTypeDef *husart);
static void USART_ReadFrame(USART_HandleTypeDef *husart);
static void USART_EndTxTransfer(USART_HandleTypeDef *husart);
static void USART_Transmit_TXE(USART_HandleTypeDef *husart);
static void USART_Receive_RXNE(USART_HandleTypeDef *husart);

/**
  * @brief  Returns 1 for UART4/UART5 (no CTS/RTS, no 0.5/1.5 stop bits).
  */
static uint8_t USART_IsUART(const USART_TypeDef *pUSARTx)
{
    return (pUSARTx == UART4 || pUSARTx == UART5) ? 1U : 0U;
}

/**
  * @brief  Enables or disables the clock for the specified USART peripheral.
  * @param  pUSARTx USART1, USART2, USART3, UART4, UART5 or USART6.
  * @param  state ENABLE to enable the clock, DISABLE to disable it.
  * @retval None
  */
void USART_PeriClockControl(USART_TypeDef *pUSARTx, uint8_t state)
{
    if (state == ENABLE)
    {
        if (pUSARTx == USART1)
        {
            USART1_CLK_ENABLE();
        }
        else if (pUSARTx == USART2)
        {
            USART2_CLK_ENABLE();
        }
        else if (pUSARTx == USART3)
        {
            USART3_CLK_ENABLE();
        }
        else if (pUSARTx == UART4)
        {
            UART4_CLK_ENABLE();
        }
        else if (pUSARTx == UART5)
        {
            UART5_CLK_ENABLE();
        }
        else if (pUSARTx == USART6)
        {
            USART6_CLK_ENABLE();
        }

        /* Errata ES0182 2.1.13: dummy read to wait 2 cycles after enabling the clock */
        (void)RCC->APB1ENR;
        (void)RCC->APB2ENR;
    }
    else
    {
        if (pUSARTx == USART1)
        {
            USART1_CLK_DISABLE();
        }
        else if (pUSARTx == USART2)
        {
            USART2_CLK_DISABLE();
        }
        else if (pUSARTx == USART3)
        {
            USART3_CLK_DISABLE();
        }
        else if (pUSARTx == UART4)
        {
            UART4_CLK_DISABLE();
        }
        else if (pUSARTx == UART5)
        {
            UART5_CLK_DISABLE();
        }
        else if (pUSARTx == USART6)
        {
            USART6_CLK_DISABLE();
        }
    }
}

/**
  * @brief  Configures the baud rate (USART_BRR) of the specified USART.
  * @note   Tx/Rx baud = fCK / (8 * (2 - OVER8) * USARTDIV)   (RM0090 30.3.4)
  *         => USARTDIV * 16 (OVER8 = 0) or USARTDIV * 8 (OVER8 = 1) = fCK / baud.
  *         div = round(fCK / baud) is therefore BRR directly for OVER8 = 0, the
  *         carry of the fraction into the mantissa is handled automatically.
  *         For OVER8 = 1 the 3-bit fraction is kept in BRR[2:0], BRR[3] = 0.
  *         Must be called after CR1.OVER8 is configured.
  * @param  pUSARTx USART1..3, UART4, UART5 or USART6.
  * @param  BaudRate desired baud rate.
  * @retval None
  */
void USART_SetBaudRate(USART_TypeDef *pUSARTx, uint32_t BaudRate)
{
    uint32_t pclk;
    uint32_t div;

    if (BaudRate == 0U)
    {
        return;
    }

    /* USART1 and USART6 are on APB2, the others on APB1 */
    if (pUSARTx == USART1 || pUSARTx == USART6)
    {
        pclk = RCC_GetPCLK2_Value();
    }
    else
    {
        pclk = RCC_GetPCLK1_Value();
    }

    div = (pclk + (BaudRate / 2U)) / BaudRate;   /* rounded fCK / baud */

    if ((pUSARTx->CR1 & USART_CR1_OVER8) != 0U)
    {
        /* Mantissa = div / 8 in BRR[15:4], fraction = div % 8 in BRR[2:0] */
        pUSARTx->BRR = ((div & ~0x7U) << 1U) | (div & 0x7U);
    }
    else
    {
        /* Mantissa = div / 16 in BRR[15:4], fraction = div % 16 in BRR[3:0] */
        pUSARTx->BRR = div & 0xFFFFU;
    }
}

/**
  * @brief  Initializes the USART according to husart->Init and enables it (UE = 1).
  * @param  husart pointer to a USART_HandleTypeDef structure that contains
  *         the configuration information for the specified USART.
  * @retval USART_OK on success, USART_ERROR if a parameter is invalid.
  */
uint8_t USART_Init(USART_HandleTypeDef *husart)
{
    USART_TypeDef *pUSARTx;
    uint32_t tempreg = 0U;

    if (husart == NULL || husart->pUSARTx == NULL || husart->Init.BaudRate == 0U)
    {
        return USART_ERROR;
    }

    pUSARTx = husart->pUSARTx;

    /* UART4/UART5: no hardware flow control, no 0.5/1.5 stop bits (RM0090 30.6.5) */
    if (USART_IsUART(pUSARTx) &&
        (husart->Init.HWFlowControl != USART_HW_NONE ||
         husart->Init.StopBits == USART_STOPBITS_0_5 ||
         husart->Init.StopBits == USART_STOPBITS_1_5))
    {
        return USART_ERROR;
    }

    /* Enable the peripheral clock */
    USART_PeriClockControl(pUSARTx, ENABLE);

    /* Disable the USART while configuring */
    pUSARTx->CR1 &= ~USART_CR1_UE;

    /******************************** CR1 ********************************/
    /* Transfer direction */
    if (husart->Init.Mode == USART_MODE_RX)
    {
        tempreg |= USART_CR1_RE;
    }
    else if (husart->Init.Mode == USART_MODE_TX)
    {
        tempreg |= USART_CR1_TE;
    }
    else /* USART_MODE_TX_RX */
    {
        tempreg |= (USART_CR1_TE | USART_CR1_RE);
    }

    /* Word length: M = 0 -> 8 data bits, M = 1 -> 9 data bits */
    if (husart->Init.WordLength == USART_WORDLENGTH_9BITS)
    {
        tempreg |= USART_CR1_M;
    }

    /* Parity: PCE = 1 enables parity, PS = 0 even / 1 odd */
    if (husart->Init.ParityControl == USART_PARITY_EVEN)
    {
        tempreg |= USART_CR1_PCE;
    }
    else if (husart->Init.ParityControl == USART_PARITY_ODD)
    {
        tempreg |= (USART_CR1_PCE | USART_CR1_PS);
    }

    /* Oversampling: OVER8 = 0 -> by 16, OVER8 = 1 -> by 8 */
    if (husart->Init.Oversampling == USART_OVER8_ENABLE)
    {
        tempreg |= USART_CR1_OVER8;
    }

    pUSARTx->CR1 = tempreg;

    /******************************** CR2 ********************************/
    pUSARTx->CR2 = ((uint32_t)(husart->Init.StopBits & 0x3U) << USART_CR2_STOP_Pos);

    /******************************** CR3 ********************************/
    tempreg = 0U;

    if (husart->Init.HWFlowControl == USART_HW_CTS)
    {
        tempreg |= USART_CR3_CTSE;
    }
    else if (husart->Init.HWFlowControl == USART_HW_RTS)
    {
        tempreg |= USART_CR3_RTSE;
    }
    else if (husart->Init.HWFlowControl == USART_HW_CTS_RTS)
    {
        tempreg |= (USART_CR3_CTSE | USART_CR3_RTSE);
    }

    pUSARTx->CR3 = tempreg;

    /******************************** BRR ********************************/
    USART_SetBaudRate(pUSARTx, husart->Init.BaudRate);

    /* Handle state */
    husart->pTxBuffer = NULL;
    husart->pRxBuffer = NULL;
    husart->TxLen     = 0U;
    husart->RxLen     = 0U;
    husart->TxState   = USART_STATE_READY;
    husart->RxState   = USART_STATE_READY;

    /* Enable the USART */
    pUSARTx->CR1 |= USART_CR1_UE;

    return USART_OK;
}

/**
  * @brief  De-initializes the USART registers to their reset values (RCC reset).
  * @param  pUSARTx USART1..3, UART4, UART5 or USART6.
  * @retval None
  */
void USART_DeInit(USART_TypeDef *pUSARTx)
{
    if (pUSARTx == USART1)
    {
        USART1_REG_RESET();
    }
    else if (pUSARTx == USART2)
    {
        USART2_REG_RESET();
    }
    else if (pUSARTx == USART3)
    {
        USART3_REG_RESET();
    }
    else if (pUSARTx == UART4)
    {
        UART4_REG_RESET();
    }
    else if (pUSARTx == UART5)
    {
        UART5_REG_RESET();
    }
    else if (pUSARTx == USART6)
    {
        USART6_REG_RESET();
    }
}

/**
  * @brief  Enables or disables the USART (CR1.UE).
  * @param  pUSARTx USART1..3, UART4, UART5 or USART6.
  * @param  state ENABLE or DISABLE.
  * @retval None
  */
void USART_PeripheralControl(USART_TypeDef *pUSARTx, uint8_t state)
{
    if (state == ENABLE)
    {
        pUSARTx->CR1 |= USART_CR1_UE;
    }
    else
    {
        pUSARTx->CR1 &= ~USART_CR1_UE;
    }
}

/**
  * @brief  Checks the status of a flag in USART_SR.
  * @param  pUSARTx USART1..3, UART4, UART5 or USART6.
  * @param  FlagName a value of @ref USART_Flags.
  * @retval FLAG_SET or FLAG_RESET.
  */
uint8_t USART_GetFlagStatus(USART_TypeDef *pUSARTx, uint32_t FlagName)
{
    return ((pUSARTx->SR & FlagName) != 0U) ? FLAG_SET : FLAG_RESET;
}

/**
  * @brief  Clears rc_w0 flags in USART_SR.
  * @note   Only CTS, LBD, TC and RXNE can be cleared by writing 0.
  *         IDLE, ORE, NE, FE and PE are cleared by a read of SR followed by
  *         a read of DR (RM0090 30.6.1). A plain write is used: writing 1 to
  *         the other bits has no effect, so no pending flag is lost.
  * @param  pUSARTx USART1..3, UART4, UART5 or USART6.
  * @param  FlagName USART_FLAG_CTS, USART_FLAG_LBD, USART_FLAG_TC or USART_FLAG_RXNE.
  * @retval None
  */
void USART_ClearFlag(USART_TypeDef *pUSARTx, uint32_t FlagName)
{
    pUSARTx->SR = ~(FlagName & (USART_SR_CTS | USART_SR_LBD | USART_SR_TC | USART_SR_RXNE));
}

/**
  * @brief  Writes one frame from husart->pTxBuffer to DR and advances the buffer.
  * @note   9-bit without parity: 2 bytes (uint16_t) per frame, otherwise 1 byte.
  *         With parity, the MSB is replaced by the parity bit by hardware.
  */
static void USART_WriteFrame(USART_HandleTypeDef *husart)
{
    if (husart->Init.WordLength == USART_WORDLENGTH_9BITS &&
        husart->Init.ParityControl == USART_PARITY_NONE)
    {
        husart->pUSARTx->DR = (*(uint16_t *)husart->pTxBuffer & 0x01FFU);
        husart->pTxBuffer += 2U;
    }
    else
    {
        husart->pUSARTx->DR = (*husart->pTxBuffer & 0xFFU);
        husart->pTxBuffer++;
    }
}

/**
  * @brief  Reads one frame from DR to husart->pRxBuffer and advances the buffer.
  * @note   The parity bit (MSB) is masked out of the user data.
  */
static void USART_ReadFrame(USART_HandleTypeDef *husart)
{
    if (husart->Init.WordLength == USART_WORDLENGTH_9BITS)
    {
        if (husart->Init.ParityControl == USART_PARITY_NONE)
        {
            /* 9 data bits */
            *(uint16_t *)husart->pRxBuffer = (uint16_t)(husart->pUSARTx->DR & 0x01FFU);
            husart->pRxBuffer += 2U;
        }
        else
        {
            /* 8 data bits + parity */
            *husart->pRxBuffer = (uint8_t)(husart->pUSARTx->DR & 0xFFU);
            husart->pRxBuffer++;
        }
    }
    else
    {
        if (husart->Init.ParityControl == USART_PARITY_NONE)
        {
            /* 8 data bits */
            *husart->pRxBuffer = (uint8_t)(husart->pUSARTx->DR & 0xFFU);
        }
        else
        {
            /* 7 data bits + parity */
            *husart->pRxBuffer = (uint8_t)(husart->pUSARTx->DR & 0x7FU);
        }
        husart->pRxBuffer++;
    }
}

/**
  * @brief  Transmits data in blocking mode.
  * @param  husart pointer to the USART handle.
  * @param  pTxBuffer pointer to the data to be transmitted.
  * @param  Len number of frames to send (see USART_WriteFrame for the buffer layout).
  * @retval None
  */
void USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len)
{
    husart->pTxBuffer = pTxBuffer;

    while (Len > 0U)
    {
        /* Wait until the transmit data register is empty */
        while (USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_TXE) == FLAG_RESET)
        {
        }

        USART_WriteFrame(husart);
        Len--;
    }

    /* Wait until the last frame is completely shifted out */
    while (USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_TC) == FLAG_RESET)
    {
    }

    husart->pTxBuffer = NULL;
}

/**
  * @brief  Receives data in blocking mode.
  * @param  husart pointer to the USART handle.
  * @param  pRxBuffer pointer to the reception buffer.
  * @param  Len number of frames to receive (see USART_ReadFrame for the buffer layout).
  * @retval None
  */
void USART_Receive(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len)
{
    husart->pRxBuffer = pRxBuffer;

    while (Len > 0U)
    {
        /* Wait until a frame is received */
        while (USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_RXNE) == FLAG_RESET)
        {
        }

        USART_ReadFrame(husart);
        Len--;
    }

    husart->pRxBuffer = NULL;
}

/**
  * @brief  Starts a transmission in interrupt mode.
  * @note   TXE interrupt feeds DR, then the TC interrupt ends the transfer and
  *         calls USART_ApplicationEventCallback(USART_EVENT_TX_CMPLT).
  * @param  husart pointer to the USART handle.
  * @param  pTxBuffer pointer to the data (must stay valid until TX complete).
  * @param  Len number of frames to send.
  * @retval Previous Tx state: USART_STATE_READY if the transfer was started.
  */
uint8_t USART_Transmit_IT(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len)
{
    uint8_t state = husart->TxState;

    if (state == USART_STATE_READY && Len > 0U)
    {
        husart->pTxBuffer = pTxBuffer;
        husart->TxLen     = Len;
        husart->TxState   = USART_STATE_BUSY_TX;

        /* Enable the TXE interrupt, TCIE is enabled after the last frame */
        husart->pUSARTx->CR1 |= USART_CR1_TXEIE;
    }

    return state;
}

/**
  * @brief  Starts a reception in interrupt mode.
  * @note   USART_ApplicationEventCallback(USART_EVENT_RX_CMPLT) is called when
  *         Len frames have been received.
  * @param  husart pointer to the USART handle.
  * @param  pRxBuffer pointer to the reception buffer.
  * @param  Len number of frames to receive.
  * @retval Previous Rx state: USART_STATE_READY if the reception was started.
  */
uint8_t USART_Receive_IT(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len)
{
    uint8_t state = husart->RxState;

    if (state == USART_STATE_READY && Len > 0U)
    {
        husart->pRxBuffer = pRxBuffer;
        husart->RxLen     = Len;
        husart->RxState   = USART_STATE_BUSY_RX;

        /* Enable the RXNE interrupt (also signals ORE) */
        husart->pUSARTx->CR1 |= USART_CR1_RXNEIE;
    }

    return state;
}

/**
  * @brief  Enables or disables the specified IRQ in the NVIC.
  * @note   ISERx/ICERx are write-1-to-set/clear, a plain write is used so that
  *         the other IRQs are not affected.
  * @param  IRQNumber USART1_IRQn (37), USART2_IRQn (38), USART3_IRQn (39),
  *         UART4_IRQn (52), UART5_IRQn (53) or USART6_IRQn (71).
  * @param  state ENABLE or DISABLE.
  * @retval None
  */
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state)
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
  * @param  IRQNumber specifies the IRQ number.
  * @param  IRQPriority priority level 0..15 (lower value = higher priority).
  * @retval None
  */
void USART_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority)
{
    uint32_t iprx        = IRQNumber / 4U;
    uint32_t iprSection  = IRQNumber % 4U;
    uint32_t shiftAmount = (8U * iprSection) + (8U - NO_PR_BITS_IMPLEMENTED);
    uint32_t prio        = (uint32_t)IRQPriority & ((1U << NO_PR_BITS_IMPLEMENTED) - 1U);

    *(NVIC_PR_BASEADDR + iprx) &= ~(0xFFU << (8U * iprSection));
    *(NVIC_PR_BASEADDR + iprx) |=  (prio << shiftAmount);
}

/**
  * @brief  Ends the interrupt mode transmission (called on TC).
  * @param  husart USART handle.
  * @retval None
  */
static void USART_EndTxTransfer(USART_HandleTypeDef *husart)
{
    /* Disable the TC interrupt and clear TC (rc_w0) */
    husart->pUSARTx->CR1 &= ~USART_CR1_TCIE;
    husart->pUSARTx->SR   = ~USART_SR_TC;

    husart->pTxBuffer = NULL;
    husart->TxLen     = 0U;
    husart->TxState   = USART_STATE_READY;

    USART_ApplicationEventCallback(husart, USART_EVENT_TX_CMPLT);
}

/**
  * @brief  Handles the TXE interrupt: sends the next frame.
  * @param  husart USART handle.
  * @retval None
  */
static void USART_Transmit_TXE(USART_HandleTypeDef *husart)
{
    if (husart->TxState != USART_STATE_BUSY_TX)
    {
        husart->pUSARTx->CR1 &= ~USART_CR1_TXEIE;
        return;
    }

    USART_WriteFrame(husart);
    husart->TxLen--;

    if (husart->TxLen == 0U)
    {
        /* Last frame written: stop TXE, wait for TC to end the transfer */
        husart->pUSARTx->CR1 &= ~USART_CR1_TXEIE;
        husart->pUSARTx->CR1 |=  USART_CR1_TCIE;
    }
}

/**
  * @brief  Handles the RXNE interrupt: stores the received frame.
  * @param  husart USART handle.
  * @retval None
  */
static void USART_Receive_RXNE(USART_HandleTypeDef *husart)
{
    if (husart->RxState != USART_STATE_BUSY_RX)
    {
        /* Unexpected frame: read DR to clear RXNE and avoid an endless interrupt */
        (void)husart->pUSARTx->DR;
        return;
    }

    USART_ReadFrame(husart);
    husart->RxLen--;

    if (husart->RxLen == 0U)
    {
        husart->pUSARTx->CR1 &= ~USART_CR1_RXNEIE;
        husart->pRxBuffer = NULL;
        husart->RxState   = USART_STATE_READY;

        USART_ApplicationEventCallback(husart, USART_EVENT_RX_CMPLT);
    }
}

/**
  * @brief  Handles the USART event and error interrupts.
  * @note   Call this function from USARTx_IRQHandler / UARTx_IRQHandler.
  *         SR is read once at the beginning: for IDLE/ORE/NE/FE/PE the
  *         following read of DR completes the clear sequence (RM0090 30.6.1).
  * @param  husart pointer to the USART handle.
  * @retval None
  */
void USART_IRQHandler(USART_HandleTypeDef *husart)
{
    USART_TypeDef *pUSARTx = husart->pUSARTx;
    uint32_t sr  = pUSARTx->SR;
    uint32_t cr1 = pUSARTx->CR1;
    uint32_t cr3 = pUSARTx->CR3;
    uint32_t errors = sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE);

    /*************************** RXNE: data received ***************************/
    if ((sr & USART_SR_RXNE) && (cr1 & USART_CR1_RXNEIE))
    {
        USART_Receive_RXNE(husart);   /* reads DR: also clears the error flags */
    }
    else if (errors != 0U && ((cr1 & (USART_CR1_RXNEIE | USART_CR1_PEIE)) || (cr3 & USART_CR3_EIE)))
    {
        /* Error without RXNE handling: read DR to complete the clear sequence */
        (void)pUSARTx->DR;
    }

    /*************************** TXE: DR empty *********************************/
    if ((sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE))
    {
        USART_Transmit_TXE(husart);
    }

    /*************************** TC: transmission complete *********************/
    if ((sr & USART_SR_TC) && (cr1 & USART_CR1_TCIE))
    {
        USART_EndTxTransfer(husart);
    }

    /*************************** CTS (not on UART4/UART5) **********************/
    if ((sr & USART_SR_CTS) && (cr3 & USART_CR3_CTSIE))
    {
        pUSARTx->SR = ~USART_SR_CTS;   /* rc_w0 */
        USART_ApplicationEventCallback(husart, USART_EVENT_CTS);
    }

    /*************************** IDLE line detected ****************************/
    if ((sr & USART_SR_IDLE) && (cr1 & USART_CR1_IDLEIE))
    {
        (void)pUSARTx->DR;             /* read SR (done) then DR clears IDLE */
        USART_ApplicationEventCallback(husart, USART_EVENT_IDLE);
    }

    /*************************** Errors ****************************************/
    if ((sr & USART_SR_PE) && (cr1 & USART_CR1_PEIE))
    {
        USART_ApplicationEventCallback(husart, USART_EVENT_PE);
    }

    if ((sr & USART_SR_ORE) && ((cr1 & USART_CR1_RXNEIE) || (cr3 & USART_CR3_EIE)))
    {
        USART_ApplicationEventCallback(husart, USART_ERR_ORE);
    }

    if (cr3 & USART_CR3_EIE)
    {
        if (sr & USART_SR_FE)
        {
            USART_ApplicationEventCallback(husart, USART_ERR_FE);
        }

        if (sr & USART_SR_NE)
        {
            USART_ApplicationEventCallback(husart, USART_ERR_NE);
        }
    }
}

/**
  * @brief  USART event callback.
  * @note   Weak symbol, implement it in the application file when needed.
  * @param  husart pointer to the USART handle.
  * @param  event a value of @ref USART_Event_Error.
  * @retval None
  */
__WEAK void USART_ApplicationEventCallback(USART_HandleTypeDef *husart, uint8_t event)
{
    (void)husart;
    (void)event;
}
