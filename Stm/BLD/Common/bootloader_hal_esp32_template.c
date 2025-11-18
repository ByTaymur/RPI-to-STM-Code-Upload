/**
  ******************************************************************************
  * @file    bootloader_hal_esp32_template.c
  * @brief   ESP32 HAL Template for Universal Bootloader
  * @author  Production Team
  * @version 4.0
  * @date    2024-11-18
  ******************************************************************************
  * @attention
  *
  * This is a TEMPLATE for ESP32 (Xtensa/RISC-V) implementation
  *
  * To use:
  * 1. Rename to bootloader_hal_esp32.c
  * 2. Add to your ESP-IDF project
  * 3. Implement the TODOs below
  * 4. Link with bootloader_common.h and bootloader_core.h
  *
  * ESP32 specific notes:
  *   - Uses ESP-IDF framework instead of HAL
  *   - Flash operations via esp_partition API
  *   - OTA (Over-The-Air) update support
  *   - Different boot mechanism (partition table)
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bootloader_hal.h"
#include "bootloader_config.h"

#ifdef PLATFORM_ESP32

/* ESP-IDF includes */
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Configuration */
#define UART_NUM            UART_NUM_0
#define UART_BAUD           115200
#define UART_BUF_SIZE       1024

/* OTA partition handle */
static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

/* ============================================================================
   FLASH IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Unlock flash memory (ESP32)
  * @note   ESP32 doesn't require explicit unlock
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Unlock(void)
{
    // ESP32: No unlock needed
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Lock flash memory (ESP32)
  * @note   ESP32 doesn't require explicit lock
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Lock(void)
{
    // ESP32: No lock needed
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Erase flash memory region (ESP32)
  * @note   ESP32 uses partition-based OTA
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Erase(uint32_t start_address,
                                            uint32_t size_bytes)
{
    // Get OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if(update_partition == NULL)
    {
        return BOOTLOADER_HAL_ERROR;
    }

    // Begin OTA update (this will erase the partition)
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN,
                                  &update_handle);

    return (err == ESP_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Write data to flash memory (ESP32 OTA)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Write(uint32_t address,
                                            const uint8_t *data,
                                            uint32_t size)
{
    // ESP32: Write via OTA API (handles alignment automatically)
    esp_err_t err = esp_ota_write(update_handle, data, size);

    return (err == ESP_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Read data from flash memory (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Read(uint32_t address,
                                           uint8_t *data,
                                           uint32_t size)
{
    esp_err_t err = esp_partition_read(update_partition, address, data, size);
    return (err == ESP_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/* ============================================================================
   UART IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Initialize UART (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_Init(uint32_t baudrate)
{
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t err = uart_param_config(UART_NUM, &uart_config);
    if(err != ESP_OK) return BOOTLOADER_HAL_ERROR;

    err = uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if(err != ESP_OK) return BOOTLOADER_HAL_ERROR;

    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Transmit single byte via UART (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_TransmitByte(uint8_t byte)
{
    int written = uart_write_bytes(UART_NUM, (const char*)&byte, 1);
    return (written == 1) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Transmit multiple bytes via UART (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_Transmit(const uint8_t *data,
                                              uint32_t size)
{
    int written = uart_write_bytes(UART_NUM, (const char*)data, size);
    return (written == size) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Receive single byte via UART (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_ReceiveByte(uint8_t *byte)
{
    int len = uart_read_bytes(UART_NUM, byte, 1, 100 / portTICK_PERIOD_MS);
    return (len == 1) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_TIMEOUT;
}

/**
  * @brief  UART RX Task (ESP32 FreeRTOS)
  */
static void (*uart_rx_callback)(uint8_t) = NULL;

static void uart_rx_task(void *pvParameters)
{
    uint8_t data;
    while(1)
    {
        int len = uart_read_bytes(UART_NUM, &data, 1,
                                  portMAX_DELAY);
        if(len > 0 && uart_rx_callback != NULL)
        {
            uart_rx_callback(data);
        }
    }
}

/**
  * @brief  Enable UART receive interrupt (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_EnableRxInterrupt(void (*callback)(uint8_t))
{
    uart_rx_callback = callback;

    // Create UART RX task
    BaseType_t ret = xTaskCreate(uart_rx_task, "uart_rx", 2048,
                                 NULL, 10, NULL);

    return (ret == pdPASS) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/* ============================================================================
   GPIO IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Set GPIO pin state (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_WritePin(uint32_t pin_id, bool state)
{
    gpio_set_level((gpio_num_t)pin_id, state ? 1 : 0);
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Toggle GPIO pin state (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_TogglePin(uint32_t pin_id)
{
    int level = gpio_get_level((gpio_num_t)pin_id);
    gpio_set_level((gpio_num_t)pin_id, !level);
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Read GPIO pin state (ESP32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_ReadPin(uint32_t pin_id, bool *state)
{
    *state = (gpio_get_level((gpio_num_t)pin_id) == 1);
    return BOOTLOADER_HAL_OK;
}

/* ============================================================================
   SYSTEM IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Delay for specified milliseconds (ESP32)
  */
void HAL_Delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

/**
  * @brief  Get system tick count (ESP32)
  */
uint32_t HAL_GetTick_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
  * @brief  Disable all interrupts (ESP32)
  */
void HAL_DisableInterrupts(void)
{
    portDISABLE_INTERRUPTS();
}

/**
  * @brief  Enable all interrupts (ESP32)
  */
void HAL_EnableInterrupts(void)
{
    portENABLE_INTERRUPTS();
}

/**
  * @brief  System reset (ESP32)
  */
void HAL_SystemReset(void)
{
    esp_restart();
    while(1);  // Never reached
}

/* ============================================================================
   CACHE IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Disable instruction cache (ESP32)
  * @note   ESP32: Cache managed by ESP-IDF
  */
void HAL_DisableICache(void)
{
    // ESP32: Cache management handled by bootloader
    // No action needed in application
}

/**
  * @brief  Disable data cache (ESP32)
  * @note   ESP32: Cache managed by ESP-IDF
  */
void HAL_DisableDCache(void)
{
    // ESP32: Cache management handled by bootloader
    // No action needed in application
}

/* ============================================================================
   APPLICATION JUMP IMPLEMENTATION (ESP32 OTA)
   ============================================================================ */

/**
  * @brief  Jump to user application (ESP32 OTA)
  */
void HAL_JumpToApplication(uint32_t app_address)
{
    // ESP32: Finalize OTA update and mark partition as valid
    esp_err_t err = esp_ota_end(update_handle);
    if(err != ESP_OK)
    {
        // OTA failed
        esp_restart();
    }

    // Set boot partition to newly updated partition
    err = esp_ota_set_boot_partition(update_partition);
    if(err != ESP_OK)
    {
        // Failed to set boot partition
        esp_restart();
    }

    // Restart to boot into new firmware
    esp_restart();

    while(1);  // Never reached
}

/**
  * @brief  Validate application exists (ESP32 OTA)
  */
bool HAL_ValidateApplication(uint32_t app_address)
{
    // ESP32: Check if update partition is valid
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);

    return (update != NULL);
}

/* ============================================================================
   DEBUG OUTPUT IMPLEMENTATION (ESP32-Specific)
   ============================================================================ */

/**
  * @brief  Initialize debug output (ESP32)
  */
void HAL_Debug_Init(void)
{
    // ESP32: printf goes to UART0 by default
}

/**
  * @brief  Print single character (ESP32)
  */
void HAL_Debug_PutChar(char ch)
{
    putchar(ch);
}

#endif /* PLATFORM_ESP32 */

/**
  * @}
  */

/************************ ESP32 HAL Template ******************************/
