/*
 * stm32f407xx_gpio.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduong
 *
 *  GPIO driver for STM32F407xx (register level).
 *  Reference: RM0090 Rev 21, section 8 (GPIO), 9.2 (SYSCFG), 12 (EXTI).
 */

#ifndef INC_STM32F407XX_GPIO_H_
#define INC_STM32F407XX_GPIO_H_

#include "stm32f407xx.h"

/**
  * @brief GPIO Init structure definition
  */
typedef struct
{
    uint8_t Pin;        /*!< Specifies the GPIO pin to be configured.
                             This parameter can be a value of @ref GPIO_pins_define          */

    uint8_t Mode;       /*!< Specifies the operating mode for the selected pin.
                             This parameter can be a value of @ref GPIO_mode_define          */

    uint8_t Pull;       /*!< Specifies the Pull-up or Pull-down activation for the selected pin.
                             This parameter can be a value of @ref GPIO_pull_define          */

    uint8_t Speed;      /*!< Specifies the output speed for the selected pin.
                             This parameter can be a value of @ref GPIO_speed_define         */

    uint8_t OPType;     /*!< Specifies the output type for the selected pin.
                             This parameter can be a value of @ref GPIO_OPType_define        */

    uint8_t Alternate;  /*!< Peripheral to be connected to the selected pin (Mode = GPIO_MODE_AF).
                             This parameter can be a value of @ref GPIO_Alternate_define     */
} GPIO_InitTypeDef;

/**
  * @brief GPIO Handle structure definition
  */
typedef struct
{
    GPIO_TypeDef     *pGPIOx;   /*!< Base address of the GPIO port to which the pin belongs */

    GPIO_InitTypeDef Init;      /*!< GPIO pin configuration settings                         */
} GPIO_HandleTypeDef;


/** @defgroup GPIO_pins_define GPIO pins define
  * @brief    Pin number (0..15), not a bit mask.
  */
#define GPIO_PIN_0                      0U
#define GPIO_PIN_1                      1U
#define GPIO_PIN_2                      2U
#define GPIO_PIN_3                      3U
#define GPIO_PIN_4                      4U
#define GPIO_PIN_5                      5U
#define GPIO_PIN_6                      6U
#define GPIO_PIN_7                      7U
#define GPIO_PIN_8                      8U
#define GPIO_PIN_9                      9U
#define GPIO_PIN_10                     10U
#define GPIO_PIN_11                     11U
#define GPIO_PIN_12                     12U
#define GPIO_PIN_13                     13U
#define GPIO_PIN_14                     14U
#define GPIO_PIN_15                     15U

/** @defgroup GPIO_mode_define GPIO mode define
  * @brief    Values 0..3 match GPIOx_MODER encoding (RM0090 8.4.1).
  */
#define GPIO_MODE_INPUT                 0U  /*!< Input mode                                          */
#define GPIO_MODE_OUTPUT                1U  /*!< General purpose output mode                         */
#define GPIO_MODE_AF                    2U  /*!< Alternate function mode                             */
#define GPIO_MODE_ANALOG                3U  /*!< Analog mode                                         */
#define GPIO_MODE_IT_FALLING            4U  /*!< External interrupt, falling edge trigger detection  */
#define GPIO_MODE_IT_RISING             5U  /*!< External interrupt, rising edge trigger detection   */
#define GPIO_MODE_IT_RISING_FALLING     6U  /*!< External interrupt, both edges trigger detection    */

/** @defgroup GPIO_OPType_define GPIO output type define
  * @brief    GPIOx_OTYPER encoding (RM0090 8.4.2).
  */
#define GPIO_OPTYPE_PP                  0U  /*!< Output push-pull  */
#define GPIO_OPTYPE_OD                  1U  /*!< Output open-drain */

/** @defgroup GPIO_speed_define GPIO speed define
  * @brief    GPIOx_OSPEEDR encoding (RM0090 8.4.3).
  */
#define GPIO_SPEED_LOW                  0U  /*!< Low speed       */
#define GPIO_SPEED_MEDIUM               1U  /*!< Medium speed    */
#define GPIO_SPEED_HIGH                 2U  /*!< High speed      */
#define GPIO_SPEED_VERY_HIGH            3U  /*!< Very high speed */

/** @defgroup GPIO_pull_define GPIO pull define
  * @brief    GPIOx_PUPDR encoding (RM0090 8.4.4), value 3 is reserved.
  */
#define GPIO_NOPULL                     0U  /*!< No pull-up or pull-down */
#define GPIO_PULLUP                     1U  /*!< Pull-up                 */
#define GPIO_PULLDOWN                   2U  /*!< Pull-down               */

/** @defgroup GPIO_Alternate_define GPIO alternate function define
  * @brief    GPIOx_AFRL/AFRH encoding (RM0090 8.4.9, 8.4.10).
  *           See the datasheet alternate function mapping table for each pin.
  */
#define GPIO_AF0                        0U
#define GPIO_AF1                        1U
#define GPIO_AF2                        2U
#define GPIO_AF3                        3U
#define GPIO_AF4                        4U
#define GPIO_AF5                        5U
#define GPIO_AF6                        6U
#define GPIO_AF7                        7U
#define GPIO_AF8                        8U
#define GPIO_AF9                        9U
#define GPIO_AF10                       10U
#define GPIO_AF11                       11U
#define GPIO_AF12                       12U
#define GPIO_AF13                       13U
#define GPIO_AF14                       14U
#define GPIO_AF15                       15U

/** @defgroup GPIO_check_define Parameter check macros
  */
#define IS_GPIO_PIN(PIN)                ((PIN) <= GPIO_PIN_15)
#define IS_GPIO_MODE(MODE)              ((MODE) <= GPIO_MODE_IT_RISING_FALLING)
#define IS_GPIO_PULL(PULL)              ((PULL) <= GPIO_PULLDOWN)
#define IS_GPIO_SPEED(SPEED)            ((SPEED) <= GPIO_SPEED_VERY_HIGH)
#define IS_GPIO_OPTYPE(TYPE)            ((TYPE) <= GPIO_OPTYPE_OD)
#define IS_GPIO_AF(AF)                  ((AF) <= GPIO_AF15)

/** @defgroup GPIO_status_define GPIO function return status
  */
#define GPIO_OK                         0U
#define GPIO_ERROR                      1U


/* Peripheral clock setup *****************************************************/
void    GPIO_PeriClockControl(GPIO_TypeDef *pGPIOx, uint8_t clockState);

/* Initialization and de-initialization ***************************************/
uint8_t GPIO_Init(GPIO_HandleTypeDef *hGPIO);
void    GPIO_DeInit(GPIO_TypeDef *pGPIOx);

/* IO operation ***************************************************************/
uint8_t  GPIO_ReadPin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin);
uint16_t GPIO_ReadPort(GPIO_TypeDef *pGPIOx);
void     GPIO_WritePin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin, uint8_t pinState);
void     GPIO_WritePort(GPIO_TypeDef *pGPIOx, uint16_t value);
void     GPIO_TogglePin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin);
uint8_t  GPIO_LockPin(GPIO_TypeDef *pGPIOx, uint8_t GPIO_pin);

/* IRQ configuration and handling *********************************************/
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state);
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority);
void GPIO_IRQHandler(uint8_t GPIO_pin);
void GPIO_EXTI_Callback(uint8_t GPIO_pin);

#endif /* INC_STM32F407XX_GPIO_H_ */
