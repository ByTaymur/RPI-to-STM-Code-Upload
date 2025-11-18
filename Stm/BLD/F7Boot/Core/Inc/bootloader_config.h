/**
  ******************************************************************************
  * @file    bootloader_config.h
  * @brief   STM32F7 specific bootloader configuration
  * @author  Production Team
  * @version 3.0
  * @date    2024-11-18
  ******************************************************************************
  */

#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Platform Identification ---------------------------------------------------*/
#define PLATFORM_NAME           "STM32F7"
#define PLATFORM_MCU_SPEED      "STM32F7xx @ 216MHz"

/* Memory Configuration ------------------------------------------------------*/
#define ETX_APP_START_ADDRESS   0x08008000      /**< Application starts after 32KB bootloader */
#define MAX_FIRMWARE_SIZE       (992 * 1024)    /**< 992KB maximum (Sectors 1-7) */

/* Flash Configuration (F7-specific) -----------------------------------------*/
#define FLASH_ERASE_FIRST_SECTOR    FLASH_SECTOR_1      /**< Start from Sector 1 */
#define FLASH_ERASE_SECTOR_COUNT    7                   /**< Erase Sectors 1-7 */
#define FLASH_VOLTAGE_RANGE_VAL     FLASH_VOLTAGE_RANGE_3  /**< 2.7V-3.6V */
#define FLASH_PROGRAMMING_TYPE      FLASH_TYPEPROGRAM_WORD  /**< F7 uses 32-bit */
#define FLASH_WORD_SIZE             4                   /**< 4 bytes per write (WORD) */

/* Flash Erase Configuration -------------------------------------------------*/
#define FLASH_ERASE_TYPE        FLASH_TYPEERASE_SECTORS
#define FLASH_ERASE_START       FLASH_ERASE_FIRST_SECTOR
#define FLASH_ERASE_COUNT       FLASH_ERASE_SECTOR_COUNT

/* Platform Features ---------------------------------------------------------*/
#define HAS_CACHE               1               /**< F7 has I-Cache and D-Cache */
#define REQUIRES_WORD_ALIGN     1               /**< F7 WORD requires 4-byte alignment */

/* GPIO Configuration --------------------------------------------------------*/
#define LED_GPIO_PORT           BLed_GPIO_Port
#define LED_GPIO_PIN            BLed_Pin

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CONFIG_H */
