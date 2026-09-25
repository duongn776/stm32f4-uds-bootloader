/*
 * stm32f407xx_flash.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 *
 *  Embedded flash driver for STM32F407xx (1 MB, single bank, 12 sectors).
 *  Reference: RM0090 Rev 21, section 3 (Embedded flash memory interface):
 *             3.3 organization (Table 5), 3.6 erase/program, 3.9 registers.
 *             STM32F407 datasheet: flash erase/program timing.
 *
 *  Design rules (SAD v3.1, WP2.6):
 *   - Program parallelism x32 (PSIZE = 10): VDD = 3 V on STM32F407G-DISC1
 *     (2.7 V - 3.6 V range, RM0090 Table 13). Programming is word only.
 *   - Sectors 0-1 (bootloader) are refused by every erase/program function.
 *   - Error flags are cleared before every operation, BSY is polled with timeout.
 *   - Every programmed word is read back and verified.
 *   - During an erase/program the CPU is stalled on any flash access (single
 *     bank, RM0090 3.6.1): no interrupt is served, SysTick does not advance.
 *     Timeouts must therefore NOT rely on getTick() (use a loop counter or
 *     DWT->CYCCNT), and IWDG timeout must be longer than a 128 KB sector erase.
 *
 *  Scope: register level only. Choosing APP/META areas, tracking erased
 *  sectors and UDS NRC mapping belong to FlashMgr / Meta (upper layers).
 */

#ifndef INC_STM32F407XX_FLASH_H_
#define INC_STM32F407XX_FLASH_H_

#include "stm32f407xx.h"

/******************************************************************************
 *                              Return status
 ******************************************************************************/
typedef enum
{
    FLASH_OK = 0,               /*!< Operation completed and verified                        */
	FLASH_ERROR,           		 /*!< Flash Error          									 */
    FLASH_ERR_PARAM,            /*!< Invalid sector / address out of main memory / not
                                     4-byte aligned / NULL pointer / length overflow         */
    FLASH_ERR_PROTECTED,        /*!< Target is in the bootloader sectors (0-1)               */
    FLASH_ERR_LOCKED,           /*!< FLASH_CR still locked (unlock failed or not called)     */
    FLASH_ERR_TIMEOUT,          /*!< BSY did not clear within the timeout                    */
    FLASH_ERR_WRP,              /*!< SR.WRPERR: sector write protected (option bytes nWRP)   */
    FLASH_ERR_PGA,              /*!< SR.PGAERR: alignment error (128-bit row crossed)        */
    FLASH_ERR_PGP,              /*!< SR.PGPERR: access size does not match PSIZE             */
    FLASH_ERR_PGS,              /*!< SR.PGSERR: programming sequence error                   */
    FLASH_ERR_OPERR,            /*!< SR.OPERR: operation error                               */
    FLASH_ERR_NOT_ERASED,       /*!< Target word is not 0xFFFFFFFF before programming        */
    FLASH_ERR_VERIFY            /*!< Read back value differs from the written value          */
} Flash_StatusTypeDef;

/******************************************************************************
 *                      Memory organization (RM0090 Table 5)
 ******************************************************************************/
#define FLASH_SECTOR_COUNT          12U

#define FLASH_SECTOR_0              0U      /*!< 0x0800_0000, 16 KB  - BOOT         */
#define FLASH_SECTOR_1              1U      /*!< 0x0800_4000, 16 KB  - BOOT         */
#define FLASH_SECTOR_2              2U      /*!< 0x0800_8000, 16 KB  - META         */
#define FLASH_SECTOR_3              3U      /*!< 0x0800_C000, 16 KB  - reserved     */
#define FLASH_SECTOR_4              4U      /*!< 0x0801_0000, 64 KB  - APP          */
#define FLASH_SECTOR_5              5U      /*!< 0x0802_0000, 128 KB - APP          */
#define FLASH_SECTOR_6              6U      /*!< 0x0804_0000, 128 KB - APP          */
#define FLASH_SECTOR_7              7U      /*!< 0x0806_0000, 128 KB - APP          */
#define FLASH_SECTOR_8              8U      /*!< 0x0808_0000, 128 KB - staging      */
#define FLASH_SECTOR_9              9U      /*!< 0x080A_0000, 128 KB - staging      */
#define FLASH_SECTOR_10             10U     /*!< 0x080C_0000, 128 KB - staging      */
#define FLASH_SECTOR_11             11U     /*!< 0x080E_0000, 128 KB - staging      */

#define FLASH_SECTOR_0_ADDR         0x08000000UL
#define FLASH_SECTOR_1_ADDR         0x08004000UL
#define FLASH_SECTOR_2_ADDR         0x08008000UL
#define FLASH_SECTOR_3_ADDR         0x0800C000UL
#define FLASH_SECTOR_4_ADDR         0x08010000UL
#define FLASH_SECTOR_5_ADDR         0x08020000UL
#define FLASH_SECTOR_6_ADDR         0x08040000UL
#define FLASH_SECTOR_7_ADDR         0x08060000UL
#define FLASH_SECTOR_8_ADDR         0x08080000UL
#define FLASH_SECTOR_9_ADDR         0x080A0000UL
#define FLASH_SECTOR_10_ADDR        0x080C0000UL
#define FLASH_SECTOR_11_ADDR        0x080E0000UL

#define FLASH_MAIN_START_ADDR       FLASH_SECTOR_0_ADDR
#define FLASH_MAIN_END_ADDR         0x080FFFFFUL        /*!< Last byte of main memory   */
#define FLASH_MAIN_SIZE             0x00100000UL        /*!< 1 MB                       */

#define FLASH_SECTOR_SIZE_16K       0x00004000UL
#define FLASH_SECTOR_SIZE_64K       0x00010000UL
#define FLASH_SECTOR_SIZE_128K      0x00020000UL

/* Bootloader sectors, never erased/programmed by this driver */
#define FLASH_PROTECTED_SECTOR_FIRST    FLASH_SECTOR_0
#define FLASH_PROTECTED_SECTOR_LAST     FLASH_SECTOR_1
#define FLASH_PROTECTED_END_ADDR        (FLASH_SECTOR_2_ADDR - 1UL)   /*!< 0x0800_7FFF  */

/* Value of an erased word */
#define FLASH_ERASED_WORD           0xFFFFFFFFUL

/******************************************************************************
 *                      Register values (RM0090 3.6, 3.9)
 ******************************************************************************/
/* FLASH_KEYR unlock sequence (a wrong sequence locks FLASH_CR until reset) */
#define FLASH_KEY1                  0x45670123UL
#define FLASH_KEY2                  0xCDEF89ABUL

/* FLASH_CR.PSIZE values (use with FLASH_CR_PSIZE_Pos) */
#define FLASH_PSIZE_X8              0x0U
#define FLASH_PSIZE_X16             0x1U
#define FLASH_PSIZE_X32             0x2U    /*!< Used by this driver (2.7 V - 3.6 V)  */
#define FLASH_PSIZE_X64             0x3U    /*!< Requires external VPP                */

/* All FLASH_SR error flags (rc_w1: cleared by writing 1) */
#define FLASH_SR_ERRORS             (FLASH_SR_OPERR  | FLASH_SR_WRPERR | FLASH_SR_PGAERR | \
                                     FLASH_SR_PGPERR | FLASH_SR_PGSERR)

/******************************************************************************
 *                              Timeouts
 * Worst case from the datasheet (x32): 16 KB ~0.5 s, 64 KB ~1.1 s,
 * 128 KB ~2 s erase, ~100 us word program. Margin x2.
 * Values in ms: convert with SystemCoreClock (loop count or DWT->CYCCNT),
 * NOT with getTick() (SysTick is stalled during the operation).
 ******************************************************************************/
#define FLASH_TIMEOUT_ERASE_16K_MS      1000U
#define FLASH_TIMEOUT_ERASE_64K_MS      2200U
#define FLASH_TIMEOUT_ERASE_128K_MS     4000U
#define FLASH_TIMEOUT_PROGRAM_MS        1U

/** @defgroup FLASH_Flag_definition FLASH Flag definition
  * @brief Flag definition
  * @{
  */
#define FLASH_FLAG_EOP                 FLASH_SR_EOP            /*!< FLASH End of Operation flag               */
#define FLASH_FLAG_OPERR               FLASH_SR_SOP            /*!< FLASH operation Error flag                */
#define FLASH_FLAG_WRPERR              FLASH_SR_WRPERR         /*!< FLASH Write protected error flag          */
#define FLASH_FLAG_PGAERR              FLASH_SR_PGAERR         /*!< FLASH Programming Alignment error flag    */
#define FLASH_FLAG_PGPERR              FLASH_SR_PGPERR         /*!< FLASH Programming Parallelism error flag  */
#define FLASH_FLAG_PGSERR              FLASH_SR_PGSERR         /*!< FLASH Programming Sequence error flag     */
#define FLASH_FLAG_BSY                 FLASH_SR_BSY            /*!< FLASH Busy flag                           */

#define FLASH_OPT_KEY1           0x08192A3BU
#define FLASH_OPT_KEY2           0x4C5D6E7FU


/**
  * @brief  Get the specified FLASH flag status.
  * @param  __FLAG__ specifies the FLASH flags to check.
  *          This parameter can be any combination of the following values:
  *            @arg FLASH_FLAG_EOP   : FLASH End of Operation flag
  *            @arg FLASH_FLAG_OPERR : FLASH operation Error flag
  *            @arg FLASH_FLAG_WRPERR: FLASH Write protected error flag
  *            @arg FLASH_FLAG_PGAERR: FLASH Programming Alignment error flag
  *            @arg FLASH_FLAG_PGPERR: FLASH Programming Parallelism error flag
  *            @arg FLASH_FLAG_PGSERR: FLASH Programming Sequence error flag
  *            @arg FLASH_FLAG_RDERR : FLASH Read Protection error flag (PCROP) (*)
  *            @arg FLASH_FLAG_BSY   : FLASH Busy flag
  *           (*) FLASH_FLAG_RDERR is not available for STM32F405xx/407xx/415xx/417xx devices
  * @retval The new state of __FLAG__ (SET or RESET).
  */
#define __FLASH_GET_FLAG(__FLAG__)   ((FLASH->SR & (__FLAG__)))


/******************************************************************************
 *                      APIs supported by this driver
 ******************************************************************************/

int32_t  Flash_GetSector(uint32_t address);
uint32_t Flash_GetSectorAddress(uint8_t sector);
uint32_t Flash_GetSectorSize(uint8_t sector);
uint8_t  Flash_IsProtected(uint32_t address, uint32_t len);
Flash_StatusTypeDef Flash_EraseSector(uint8_t sector);
Flash_StatusTypeDef Flash_ProgramWord(uint32_t address, uint32_t data);
Flash_StatusTypeDef FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data);
uint8_t  Flash_IsBlank(uint32_t address, uint32_t len);
Flash_StatusTypeDef Flash_Verify(uint32_t address, const uint8_t *pData, uint32_t len);
Flash_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout);




/* Peripheral Control functions  **********************************************/
Flash_StatusTypeDef FLASH_Unlock(void);
Flash_StatusTypeDef FLASH_Lock(void);
Flash_StatusTypeDef FLASH_OB_Unlock(void);
Flash_StatusTypeDef FLASH_OB_Lock(void);



#endif /* INC_STM32F407XX_FLASH_H_ */
