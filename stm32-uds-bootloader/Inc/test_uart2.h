/*
 * test_uart2.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  Test USART2 driver: PA2 = TX, PA3 = RX, 115200 8N1.
 */

#ifndef TEST_UART2_H_
#define TEST_UART2_H_

/* Runs the USART2 test, never returns. SysTick_Init() must be called before. */
void Test_UART2_Run(void);

#endif /* TEST_UART2_H_ */
