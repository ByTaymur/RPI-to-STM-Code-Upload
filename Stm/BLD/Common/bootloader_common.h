/**
  ******************************************************************************
  * @file    bootloader_common.h
  * @brief   Common definitions for STM32 bootloader (Universal)
  * @author  Production Team
  * @version 3.0
  * @date    2024-11-18
  ******************************************************************************
  * @attention
  *
  * This file contains universal definitions that work across all STM32
  * platforms (F1, F4, F7, H7, etc.)
  *
  * Platform-specific settings should go in bootloader_config.h
  *
  ******************************************************************************
  */

#ifndef BOOTLOADER_COMMON_H
#define BOOTLOADER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Universal Protocol Constants ----------------------------------------------*/
#define PROTOCOL_START_BYTE     '{'     /**< Handshake start marker (0x7B) */
#define PROTOCOL_END_BYTE       '}'     /**< Handshake end marker (0x7D) */
#define PROTOCOL_ACK_BYTE       'O'     /**< Acknowledgment byte (0x4F) */
#define PROTOCOL_NACK_BYTE      'N'     /**< Negative acknowledgment (0x4E) */

/* Universal Block Configuration ---------------------------------------------*/
#define MAX_BLOCK_SIZE          1024    /**< Standard block size (1KB) */
#define HANDSHAKE_MIN_INDEX     4       /**< Min handshake packet size */
#define HANDSHAKE_MAX_INDEX     8       /**< Max handshake packet size */

/* Universal Bootloader States -----------------------------------------------*/
/**
  * @brief Bootloader state machine enumeration
  *
  * This enum is universal across all STM32 platforms
  */
typedef enum {
    BOOTLOADER_STATE_IDLE               = 0,    /**< Waiting for handshake */
    BOOTLOADER_STATE_HANDSHAKE_RECEIVED = 10,   /**< Handshake detected */
    BOOTLOADER_STATE_TRANSFERRING       = 20,   /**< Data transfer in progress */
    BOOTLOADER_STATE_COMPLETE           = 30,   /**< Transfer successful */
    BOOTLOADER_STATE_ERROR              = 255   /**< Error occurred */
} BootloaderState_t;

/* Universal Helper Function Prototypes --------------------------------------*/

/**
  * @brief  Calculate checksum (first byte + last byte) & 0xFF
  * @param  block: Pointer to data block
  * @param  size: Size of block
  * @retval Calculated checksum
  * @note   This is the universal checksum algorithm used by RPI and all STM32s
  */
static inline uint8_t Calculate_Checksum(const uint8_t *block, uint32_t size)
{
    if(block == NULL || size == 0)
    {
        return 0;
    }
    return (block[0] + block[size - 1]) & 0xFF;
}

/**
  * @brief  Parse firmware size from handshake packet (big-endian)
  * @param  packet: Handshake packet buffer
  * @retval Firmware size in bytes
  * @note   Universal for all platforms (RPI sends big-endian)
  */
static inline uint32_t Parse_Firmware_Size(const uint8_t *packet)
{
    return ((uint32_t)packet[1] << 24) |
           ((uint32_t)packet[2] << 16) |
           ((uint32_t)packet[3] << 8)  |
           ((uint32_t)packet[4]);
}

/* Universal Validation Macros -----------------------------------------------*/

/**
  * @brief  Validate handshake packet format
  * @param  buf: Buffer containing handshake
  * @param  idx: Current index
  * @retval true if valid, false otherwise
  */
#define IS_VALID_HANDSHAKE(buf, idx) \
    (((buf)[0] == PROTOCOL_START_BYTE || (buf)[0] == 0x7B) && \
     ((idx) >= HANDSHAKE_MIN_INDEX && (idx) <= HANDSHAKE_MAX_INDEX))

/**
  * @brief  Check if received byte is start marker
  */
#define IS_START_BYTE(byte) \
    ((byte) == PROTOCOL_START_BYTE || (byte) == 0x7B)

/**
  * @brief  Check if received byte is end marker
  */
#define IS_END_BYTE(byte) \
    ((byte) == PROTOCOL_END_BYTE || (byte) == 0x7D)

/* Universal Error Messages --------------------------------------------------*/
#define ERROR_MSG_BUFFER_OVERFLOW   "ERROR: Buffer overflow detected!\r\n"
#define ERROR_MSG_FIRMWARE_TOO_LARGE "ERROR: Firmware too large (%lu bytes). Max: %d KB\r\n"
#define ERROR_MSG_FLASH_UNLOCK      "ERROR: Flash unlock failed (0x%02X)\r\n"
#define ERROR_MSG_FLASH_ERASE       "ERROR: Flash erase failed (error: 0x%08lX)\r\n"
#define ERROR_MSG_FLASH_WRITE       "ERROR: Flash write failed at 0x%08lX (0x%02X)\r\n"
#define ERROR_MSG_NO_APP            "ERROR: No valid application found!\r\n"

/* Universal Progress Messages -----------------------------------------------*/
#define MSG_HANDSHAKE_OK            "Handshake OK: Firmware size = %lu bytes\r\n"
#define MSG_BOOTLOADER_READY        "Bootloader ready - waiting for firmware...\r\n"
#define MSG_TRANSFER_COMPLETE       "Firmware transfer complete!\r\n"
#define MSG_JUMP_TO_APP             "Preparing to jump to application...\r\n"
#define MSG_FLASH_ERASE             "Erasing flash...\r\n"
#define MSG_FLASH_ERASE_COMPLETE    "Flash erase complete\r\n"
#define MSG_PROGRESS                "\rProgress: %lu / %lu bytes\r\n"

/* Universal Timeout Configuration -------------------------------------------*/
#define BOOTLOADER_TIMEOUT_MS       50000   /**< 50 seconds overall timeout */
#define TRANSFER_COMPLETE_DELAY_MS  1000    /**< 1 second wait after transfer */

/* Universal Version Information ---------------------------------------------*/
#define BOOTLOADER_VERSION_MAJOR    3
#define BOOTLOADER_VERSION_MINOR    0
#define BOOTLOADER_VERSION_PATCH    0
#define BOOTLOADER_VERSION_STRING   "v3.0 (Universal)"

/* Universal Banner Template -------------------------------------------------*/
#define BANNER_TOP    "╔════════════════════════════════════════════════════════════════╗\r\n"
#define BANNER_BOTTOM "╚════════════════════════════════════════════════════════════════╝\r\n"
#define BANNER_SEP    "╠════════════════════════════════════════════════════════════════╣\r\n"

/**
  * @brief  Macro to print bootloader banner
  * @param  mcu_name: MCU name string
  * @param  mcu_freq: MCU frequency string
  * @param  app_addr: Application start address
  * @param  max_size: Maximum firmware size in bytes
  */
#define PRINT_BOOTLOADER_BANNER(mcu_name, mcu_freq, app_addr, max_size) \
    do { \
        printf("\r\n"); \
        printf(BANNER_TOP); \
        printf("║       %s Bootloader %s                        ║\r\n", mcu_name, BOOTLOADER_VERSION_STRING); \
        printf(BANNER_SEP); \
        printf("║  MCU:         %-48s║\r\n", mcu_freq); \
        printf("║  Application: 0x%08lX                                      ║\r\n", (unsigned long)(app_addr)); \
        printf("║  Max Size:    %-10d KB                                 ║\r\n", (int)((max_size) / 1024)); \
        printf("║  Block Size:  %-10d bytes                              ║\r\n", MAX_BLOCK_SIZE); \
        printf(BANNER_BOTTOM); \
        printf("\r\n"); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_COMMON_H */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
