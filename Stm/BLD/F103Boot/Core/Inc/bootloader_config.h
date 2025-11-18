/**
  ******************************************************************************
  * @file    bootloader_config.h
  * @brief   STM32F103 specific bootloader configuration
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
#define PLATFORM_NAME           "STM32F103"
#define PLATFORM_MCU_SPEED      "STM32F103 @ 72MHz"

/* Memory Configuration ------------------------------------------------------*/
#define ETX_APP_START_ADDRESS   0x08004400      /**< Application starts after bootloader */
#define MAX_FIRMWARE_SIZE       (47 * 1024)     /**< 47KB maximum (47 pages) */

/* Flash Configuration (F103-specific) ---------------------------------------*/
#define FLASH_ERASE_PAGES       47              /**< Number of pages to erase */
#define FLASH_PROGRAMMING_TYPE  FLASH_TYPEPROGRAM_HALFWORD  /**< F103 uses 16-bit */
#define FLASH_WORD_SIZE         2               /**< 2 bytes per write (HALFWORD) */

/* Flash Erase Configuration -------------------------------------------------*/
#define FLASH_ERASE_TYPE        FLASH_TYPEERASE_PAGES
#define FLASH_ERASE_START       ETX_APP_START_ADDRESS
#define FLASH_ERASE_COUNT       FLASH_ERASE_PAGES

/* Platform Features ---------------------------------------------------------*/
#define HAS_CACHE               0               /**< F103 has no cache */
#define REQUIRES_WORD_ALIGN     0               /**< F103 HALFWORD, no strict align */

/* GPIO Configuration --------------------------------------------------------*/
#define LED_GPIO_PORT           BLed_GPIO_Port
#define LED_GPIO_PIN            BLed_Pin

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CONFIG_H */
