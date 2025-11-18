/**
  ******************************************************************************
  * @file    bootloader_core.h
  * @brief   Universal bootloader core functions (platform-independent)
  * @author  Production Team
  * @version 3.0
  * @date    2024-11-18
  ******************************************************************************
  * @attention
  *
  * This file contains universal bootloader logic that works across all
  * STM32 platforms. Include this after bootloader_common.h and
  * bootloader_config.h
  *
  ******************************************************************************
  */

#ifndef BOOTLOADER_CORE_H
#define BOOTLOADER_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bootloader_common.h"
#include "bootloader_config.h"
#include <stdio.h>

/* Universal State Variables -------------------------------------------------*/
/**
  * @brief  Bootloader state management structure (universal)
  */
typedef struct {
    BootloaderState_t state;            /**< Current bootloader state */
    uint8_t           uart_rx_byte;     /**< Last received UART byte */
    uint8_t           ack_byte;         /**< ACK response byte */
    uint8_t           data_block[MAX_BLOCK_SIZE];  /**< Data buffer */
    uint16_t          block_index;      /**< Current index in data_block */
    uint32_t          block_index_saved;/**< Saved index for checksum */
    uint32_t          firmware_total_size;     /**< Total firmware size */
    uint32_t          firmware_received_size;  /**< Received so far */
    uint32_t          bytes_remaining;  /**< Bytes left to receive */
    uint32_t          current_block_size; /**< Current block size */
    uint8_t           checksum_response[1]; /**< Checksum to send back */
    uint8_t           handshake_count;  /**< Number of handshakes */
    uint32_t          idle_timeout_counter; /**< Timeout counter */
    bool              transfer_error_flag;  /**< Error flag */
    uint32_t          flash_write_index;    /**< Flash write position */
} BootloaderContext_t;

/**
  * @brief  Initialize bootloader context to default values
  * @param  ctx: Pointer to bootloader context
  * @retval None
  */
static inline void Bootloader_InitContext(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return;

    memset(ctx, 0, sizeof(BootloaderContext_t));
    ctx->state = BOOTLOADER_STATE_IDLE;
    ctx->ack_byte = PROTOCOL_ACK_BYTE;
    ctx->bytes_remaining = MAX_BLOCK_SIZE;
    ctx->current_block_size = MAX_BLOCK_SIZE;
}

/**
  * @brief  Reset transfer state to idle (universal)
  * @param  ctx: Pointer to bootloader context
  * @retval None
  */
static inline void Bootloader_ResetTransfer(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return;

    ctx->state = BOOTLOADER_STATE_IDLE;
    ctx->block_index = 0;
    ctx->firmware_total_size = 0;
    ctx->firmware_received_size = 0;
    ctx->bytes_remaining = MAX_BLOCK_SIZE;
    ctx->current_block_size = MAX_BLOCK_SIZE;
    ctx->transfer_error_flag = false;
    memset(ctx->data_block, 0, sizeof(ctx->data_block));
}

/**
  * @brief  Process handshake packet (universal)
  * @param  ctx: Pointer to bootloader context
  * @retval true if handshake valid and accepted, false otherwise
  */
static inline bool Bootloader_ProcessHandshake(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return false;

    // Parse firmware size
    ctx->data_block[0] = 0;  // Clear markers
    ctx->data_block[5] = 0;
    ctx->firmware_total_size = Parse_Firmware_Size(ctx->data_block);

    // Validate size
    if(ctx->firmware_total_size > MAX_FIRMWARE_SIZE)
    {
        printf(ERROR_MSG_FIRMWARE_TOO_LARGE,
               ctx->firmware_total_size,
               MAX_FIRMWARE_SIZE / 1024);
        return false;
    }

    // Initialize transfer
    printf(MSG_HANDSHAKE_OK, ctx->firmware_total_size);
    ctx->bytes_remaining = ctx->firmware_total_size;
    ctx->firmware_received_size = 0;
    ctx->handshake_count++;
    ctx->block_index = 0;
    ctx->state = BOOTLOADER_STATE_TRANSFERRING;

    return true;
}

/**
  * @brief  Update block size tracking (universal)
  * @param  ctx: Pointer to bootloader context
  * @retval None
  */
static inline void Bootloader_UpdateBlockSize(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return;

    // Save current block index
    ctx->block_index_saved = ctx->block_index;

    // Update remaining bytes
    if(ctx->bytes_remaining >= MAX_BLOCK_SIZE)
    {
        ctx->bytes_remaining -= ctx->current_block_size;
        ctx->current_block_size = MAX_BLOCK_SIZE;
        ctx->firmware_received_size += ctx->current_block_size;
    }
    else if(ctx->bytes_remaining < MAX_BLOCK_SIZE)
    {
        ctx->current_block_size = ctx->bytes_remaining;
        ctx->firmware_received_size += ctx->current_block_size;
    }
}

/**
  * @brief  Check if transfer is complete (universal)
  * @param  ctx: Pointer to bootloader context
  * @retval true if complete, false otherwise
  */
static inline bool Bootloader_IsTransferComplete(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return false;

    return (ctx->firmware_received_size >= ctx->firmware_total_size) &&
           (ctx->state == BOOTLOADER_STATE_TRANSFERRING) &&
           (ctx->idle_timeout_counter > TRANSFER_COMPLETE_DELAY_MS);
}

/**
  * @brief  Check for timeout (universal)
  * @param  ctx: Pointer to bootloader context
  * @retval true if timeout occurred, false otherwise
  */
static inline bool Bootloader_IsTimeout(BootloaderContext_t *ctx)
{
    if(ctx == NULL) return false;

    return ctx->idle_timeout_counter > BOOTLOADER_TIMEOUT_MS;
}

/**
  * @brief  Validate application exists in flash (universal)
  * @param  app_addr: Application start address
  * @retval true if valid application found, false otherwise
  */
static inline bool Bootloader_ValidateApplication(uint32_t app_addr)
{
    uint32_t app_reset_handler = *((volatile uint32_t*)(app_addr + 4U));
    return (app_reset_handler != 0xFFFFFFFF);
}

/**
  * @brief  Prepare to jump to application (universal - no cache)
  * @param  app_addr: Application start address
  * @retval None
  * @note   Call platform-specific cache disable before this if needed
  */
static inline void Bootloader_PrepareJump(uint32_t app_addr)
{
    // Disable all interrupts
    __disable_irq();

    // Disable SysTick timer
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Set Main Stack Pointer
    __set_MSP(*(volatile uint32_t*)app_addr);
}

/**
  * @brief  Execute jump to application (universal)
  * @param  app_addr: Application start address
  * @retval None (never returns)
  */
static inline void Bootloader_JumpToApplication(uint32_t app_addr)
{
    // Get reset handler address
    uint32_t app_reset_handler_addr = *((volatile uint32_t*)(app_addr + 4U));
    void (*app_reset_handler)(void) = (void*)app_reset_handler_addr;

    // Jump
    app_reset_handler();
}

/**
  * @brief  Print banner (universal wrapper)
  * @retval None
  */
static inline void Bootloader_PrintBanner(void)
{
    PRINT_BOOTLOADER_BANNER(PLATFORM_NAME,
                           PLATFORM_MCU_SPEED,
                           ETX_APP_START_ADDRESS,
                           MAX_FIRMWARE_SIZE);
}

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CORE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
