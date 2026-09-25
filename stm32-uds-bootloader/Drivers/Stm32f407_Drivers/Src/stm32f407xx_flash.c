/*
 * stm32f407xx_flash.h
 *
 *  Created on: Sep 25, 2026
 *      Author: nhduo
 */

#ifndef SRC_STM32F407XX_FLASH_C_
#define SRC_STM32F407XX_FLASH_C_


#include "stm32f407xx_flash.h"

/*
 * Lock / unlock
 */

/**
  * @brief  Unlocks FLASH_CR (KEY1 then KEY2).
  * @note   Does nothing if already unlocked (writing the keys again while
  *         unlocked would cause a bus error and lock FLASH_CR until reset).
  * @retval FLASH_OK, or FLASH_ERR_LOCKED if CR.LOCK is still set.
  */
Flash_StatusTypeDef Flash_Unlock(void);

/**
  * @brief  Locks FLASH_CR (CR.LOCK = 1). Call it after every erase/program session.
  */
void Flash_Lock(void);

/*
 * Memory organization helpers
 */

/* Start address of each sector + end of main memory (RM0090 Table 5).
 * Size of sector n = s_sectorStart[n + 1] - s_sectorStart[n]. */
static const uint32_t s_sectorStart[FLASH_SECTOR_COUNT + 1U] =
{
    FLASH_SECTOR_0_ADDR,  FLASH_SECTOR_1_ADDR,  FLASH_SECTOR_2_ADDR,  FLASH_SECTOR_3_ADDR,
    FLASH_SECTOR_4_ADDR,  FLASH_SECTOR_5_ADDR,  FLASH_SECTOR_6_ADDR,  FLASH_SECTOR_7_ADDR,
    FLASH_SECTOR_8_ADDR,  FLASH_SECTOR_9_ADDR,  FLASH_SECTOR_10_ADDR, FLASH_SECTOR_11_ADDR,
    FLASH_MAIN_END_ADDR + 1UL                   /* 0x0810_0000: end of sector 11 */
};

/**
  * @brief  Returns the sector number containing an address.
  * @param  address any address in main memory.
  * @retval Sector number 0..11, or -1 if the address is outside main memory.
  */
int32_t Flash_GetSector(uint32_t address)
{
    int32_t sector;

    if ((address < FLASH_MAIN_START_ADDR) || (address > FLASH_MAIN_END_ADDR))
    {
        return -1;
    }

    /* Highest sector whose start address is <= address */
    for (sector = (int32_t)FLASH_SECTOR_COUNT - 1; sector > 0; sector--)
    {
        if (address >= s_sectorStart[sector])
        {
            break;
        }
    }

    return sector;
}

/**
  * @brief  Returns the start address of a sector.
  * @param  sector 0..11.
  * @retval Start address, or 0 if the sector number is invalid.
  */
uint32_t Flash_GetSectorAddress(uint8_t sector)
{
    if (sector >= FLASH_SECTOR_COUNT)
    {
        return 0U;
    }

    return s_sectorStart[sector];
}

/**
  * @brief  Returns the size of a sector in bytes.
  * @param  sector 0..11.
  * @retval FLASH_SECTOR_SIZE_16K, FLASH_SECTOR_SIZE_64K or FLASH_SECTOR_SIZE_128K,
  *         or 0 if the sector number is invalid.
  */
uint32_t Flash_GetSectorSize(uint8_t sector)
{
    if (sector >= FLASH_SECTOR_COUNT)
    {
        return 0U;
    }

    return s_sectorStart[sector + 1U] - s_sectorStart[sector];
}

/**
  * @brief  Returns 1 if [address, address + len) touches a protected sector (0-1).
  * @note   Two ranges [a1, b1] and [a2, b2] overlap when a1 <= b2 and b1 >= a2.
  *         A wrapped-around range (address + len overflows) is reported as
  *         protected, so an invalid range can never pass this check.
  */
uint8_t Flash_IsProtected(uint32_t address, uint32_t len)
{
    uint32_t last;

    if (len == 0U)
    {
        return 0U;
    }

    last = address + len - 1U;       /* last byte of the range */

    if (last < address)
    {
        return 1U;                   /* 32-bit overflow */
    }

    return ((address <= FLASH_PROTECTED_END_ADDR) &&
            (last    >= FLASH_MAIN_START_ADDR)) ? 1U : 0U;
}


/*
 * Erase / program
 */

/**
  * @brief  Erases one sector (RM0090 3.6.3), blocking.
  * @note   Sequence: check BSY, clear SR errors, CR: PSIZE = x32, SER = 1, SNB = sector,
  *         then STRT = 1, wait BSY with FLASH_TIMEOUT_ERASE_xxx, check SR errors,
  *         clear SER/SNB, flush the instruction/data caches (FLASH_ACR ICRST/DCRST).
  *         The CPU is stalled for up to ~2 s (128 KB sector).
  *         FLASH_CR must be unlocked before.
  * @param  sector 2..11 (FLASH_SECTOR_x).
  * @retval FLASH_OK, FLASH_ERR_PARAM, FLASH_ERR_PROTECTED, FLASH_ERR_LOCKED,
  *         FLASH_ERR_TIMEOUT or an SR error (FLASH_ERR_WRP, FLASH_ERR_OPERR, ...).
  */
Flash_StatusTypeDef Flash_EraseSector(uint8_t sector)
{

}
/**
  * @brief  Programs one 32-bit word (RM0090 3.6.4), blocking, with read-back verify.
  * @note   Sequence: check BSY, clear SR errors, CR: PSIZE = x32, PG = 1,
  *         one 32-bit write to address, wait BSY with FLASH_TIMEOUT_PROGRAM_MS,
  *         check SR errors, PG = 0, compare *(volatile uint32_t *)address with data.
  *         The target word must be erased (0xFFFFFFFF).
  *         FLASH_CR must be unlocked before.
  * @param  address word aligned address, outside sectors 0-1.
  * @param  data value to program.
  * @retval FLASH_OK, FLASH_ERR_PARAM, FLASH_ERR_PROTECTED, FLASH_ERR_LOCKED,
  *         FLASH_ERR_NOT_ERASED, FLASH_ERR_TIMEOUT, an SR error, or FLASH_ERR_VERIFY.
  */
Flash_StatusTypeDef Flash_ProgramWord(uint32_t address, uint32_t data);

/**
  * @brief  Program byte, halfword, word or double word at a specified address
  * @param  TypeProgram  Indicate the way to program at a specified address.
  *                           This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address  specifies the address to be programmed.
  * @param  Data specifies the data to be programmed
  *
  * @retval Flash_StatusTypeDef Flash Status
  */
Flash_StatusTypeDef FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
{

}

/*
 * Read-back checks
 */

/**
  * @brief  Checks that [address, address + len) is erased (all bytes 0xFF).
  * @retval 1 if blank, 0 otherwise (or if the range is outside main memory).
  */
uint8_t  Flash_IsBlank(uint32_t address, uint32_t len);

/**
  * @brief  Compares flash content with a buffer.
  * @retval FLASH_OK if equal, FLASH_ERR_VERIFY if different, FLASH_ERR_PARAM if
  *         the range is outside main memory or pData is NULL.
  */
Flash_StatusTypeDef Flash_Verify(uint32_t address, const uint8_t *pData, uint32_t len);

/**
  * @brief  Unlock the FLASH control register access
  * @retval Flash Status
  */
Flash_StatusTypeDef FLASH_Unlock(void)
{
	Flash_StatusTypeDef status = FLASH_OK;

	if ((FLASH->CR & FLASH_CR_LOCK) != RESET)
	{
		/* Authorize the FLASH Registers access */
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;

		/* Verify Flash is unlocked */
		if ((FLASH->CR & FLASH_CR_LOCK) != RESET)
		{
			status = FLASH_ERROR;
		}
	}

	return status;
}


/**
  * @brief  Locks the FLASH control register access
  * @retval Flash Status
  */
Flash_StatusTypeDef FLASH_Lock(void)
{
  /* Set the LOCK Bit to lock the FLASH Registers access */
  FLASH->CR |= FLASH_CR_LOCK;

  return FLASH_OK;
}


/**
  * @brief  Unlock the FLASH Option Control Registers access.
  * @retval Flash Status
  */
Flash_StatusTypeDef FLASH_OB_Unlock(void)
{
  if ((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) != RESET)
  {
    /* Authorizes the Option Byte register programming */
    FLASH->OPTKEYR = FLASH_OPT_KEY1;
    FLASH->OPTKEYR = FLASH_OPT_KEY2;
  }
  else
  {
    return FLASH_ERROR;
  }

  return FLASH_OK;
}

/**
  * @brief  Lock the FLASH Option Control Registers access.
  * @retval Flash Status
  */
Flash_StatusTypeDef FLASH_OB_Lock(void)
{
  /* Set the OPTLOCK Bit to lock the FLASH Option Byte Registers access */
  FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;

  return FLASH_OK;
}



/**
  * @brief  Wait for a FLASH operation to complete.
  * @param  Timeout maximum flash operationtimeout
  * @retval Flash Status
  */
Flash_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout)
{
  uint32_t tickstart  = DWT->CYCCNT;
  uint32_t cycles = Timeout * (SystemCoreClock / 1000U);


  /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
     Even if the FLASH operation fails, the BUSY flag will be reset and an error
     flag will be set */
  while ((FLASH->SR & FLASH_SR_BSY) != 0U)
  {
	if ((uint32_t)(DWT->CYCCNT - tickstart) >= cycles)
    {
       return FLASH_ERR_TIMEOUT;
    }
  }

  /* If there is no error flag set */
  return FLASH_OK;

}









#endif /* SRC_STM32F407XX_FLASH_C_ */
