/**
  ******************************************************************************
  * @file    bootloader_hal_stm32.c
  * @brief   STM32 HAL Implementation for Universal Bootloader
  * @author  Production Team
  * @version 4.0
  * @date    2024-11-18
  ******************************************************************************
  * @attention
  *
  * This is the STM32-specific implementation of bootloader_hal.h
  *
  * For other processors (ESP32, nRF52, PIC32, etc.), create similar files:
  *   - bootloader_hal_esp32.c
  *   - bootloader_hal_nrf52.c
  *   - bootloader_hal_pic32.c
  *   - etc.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bootloader_hal.h"
#include "bootloader_config.h"

#ifdef PLATFORM_STM32

/* Include STM32 HAL library */
#include "main.h"  // Contains HAL includes

/* External handles (defined in main.c) */
extern UART_HandleTypeDef huart1;  // Communication UART
extern UART_HandleTypeDef huart2;  // Debug UART

/* ============================================================================
   FLASH IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Unlock flash memory for writing (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Unlock(void)
{
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Lock flash memory after writing (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Lock(void)
{
    HAL_StatusTypeDef status = HAL_FLASH_Lock();
    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Erase flash memory region (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Erase(uint32_t start_address,
                                            uint32_t size_bytes)
{
    HAL_StatusTypeDef status;
    uint32_t erase_error = 0;

#if defined(STM32F1)
    /* F1 uses Page-based erase */
    FLASH_EraseInitTypeDef erase_config;
    erase_config.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_config.PageAddress = start_address;
    erase_config.NbPages = FLASH_ERASE_COUNT;

#elif defined(STM32F4) || defined(STM32F7) || defined(STM32H7)
    /* F4/F7/H7 use Sector-based erase */
    FLASH_EraseInitTypeDef erase_config;
    erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_config.Sector = FLASH_ERASE_START;
    erase_config.NbSectors = FLASH_ERASE_COUNT;
    erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_VAL;

#elif defined(STM32L4) || defined(STM32G4)
    /* L4/G4 use Page-based erase with bank support */
    FLASH_EraseInitTypeDef erase_config;
    erase_config.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_config.Banks = FLASH_BANK_1;
    erase_config.Page = start_address;
    erase_config.NbPages = FLASH_ERASE_COUNT;

#else
    #error "Unsupported STM32 series"
#endif

    // CRITICAL: Disable interrupts during flash erase
    __disable_irq();
    status = HAL_FLASHEx_Erase(&erase_config, &erase_error);
    __enable_irq();

    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Write data to flash memory (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Write(uint32_t address,
                                            const uint8_t *data,
                                            uint32_t size)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t write_addr = address;

#if defined(STM32F1)
    /* F1: HALFWORD (16-bit) programming */
    for(uint32_t i = 0; i < size; i += 2)
    {
        uint16_t halfword;
        if(i + 1 < size)
        {
            halfword = data[i] | (data[i + 1] << 8);
        }
        else
        {
            halfword = data[i] | 0xFF00;  // Pad with 0xFF
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   write_addr, halfword);
        if(status != HAL_OK) break;
        write_addr += 2;
    }

#elif defined(STM32F4) || defined(STM32F7)
    /* F4/F7: WORD (32-bit) programming */
    for(uint32_t i = 0; i < size; i += 4)
    {
        uint32_t word;
        if(i + 3 < size)
        {
            word = data[i] |
                  (data[i + 1] << 8) |
                  (data[i + 2] << 16) |
                  (data[i + 3] << 24);
        }
        else
        {
            // Pad remaining bytes with 0xFF
            word = 0xFFFFFFFF;
            for(uint32_t j = 0; j < 4 && (i + j) < size; j++)
            {
                word = (word & ~(0xFF << (j * 8))) | (data[i + j] << (j * 8));
            }
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   write_addr, word);
        if(status != HAL_OK) break;
        write_addr += 4;
    }

#elif defined(STM32H7)
    /* H7: FLASHWORD (256-bit/32-byte) programming */
    for(uint32_t i = 0; i < size; i += 32)
    {
        uint32_t flashword[8] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
                                 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

        // Copy data to flashword (pad with 0xFF if needed)
        uint32_t copy_len = (i + 32 <= size) ? 32 : (size - i);
        memcpy(flashword, &data[i], copy_len);

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                   write_addr, (uint32_t)flashword);
        if(status != HAL_OK) break;
        write_addr += 32;
    }

#elif defined(STM32L4) || defined(STM32G4)
    /* L4/G4: DOUBLEWORD (64-bit) programming */
    for(uint32_t i = 0; i < size; i += 8)
    {
        uint64_t doubleword;
        if(i + 7 < size)
        {
            doubleword = ((uint64_t)data[i])       |
                        ((uint64_t)data[i + 1] << 8)  |
                        ((uint64_t)data[i + 2] << 16) |
                        ((uint64_t)data[i + 3] << 24) |
                        ((uint64_t)data[i + 4] << 32) |
                        ((uint64_t)data[i + 5] << 40) |
                        ((uint64_t)data[i + 6] << 48) |
                        ((uint64_t)data[i + 7] << 56);
        }
        else
        {
            // Pad with 0xFF
            doubleword = 0xFFFFFFFFFFFFFFFFULL;
            for(uint32_t j = 0; j < 8 && (i + j) < size; j++)
            {
                doubleword = (doubleword & ~(0xFFULL << (j * 8))) |
                            ((uint64_t)data[i + j] << (j * 8));
            }
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                   write_addr, doubleword);
        if(status != HAL_OK) break;
        write_addr += 8;
    }

#else
    #error "Unsupported STM32 series for flash programming"
#endif

    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Read data from flash memory (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Read(uint32_t address,
                                           uint8_t *data,
                                           uint32_t size)
{
    // STM32 allows direct memory access to flash
    memcpy(data, (void*)address, size);
    return BOOTLOADER_HAL_OK;
}

/* ============================================================================
   UART IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Initialize UART (STM32)
  * @note   UART is already initialized in main(), so this is a no-op
  */
BootloaderHAL_StatusTypeDef HAL_UART_Init(uint32_t baudrate)
{
    // UART already initialized by CubeMX generated code
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Transmit single byte via UART (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_TransmitByte(uint8_t byte)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, &byte, 1, 100);
    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Transmit multiple bytes via UART (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_Transmit(const uint8_t *data,
                                              uint32_t size)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t*)data,
                                                 size, 1000);
    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/**
  * @brief  Receive single byte via UART (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_ReceiveByte(uint8_t *byte)
{
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, byte, 1, 100);
    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_TIMEOUT;
}

/* Static callback pointer */
static void (*uart_rx_callback)(uint8_t) = NULL;

/**
  * @brief  STM32 HAL UART RX Complete Callback
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rx_byte;

    if(huart->Instance == huart1.Instance && uart_rx_callback != NULL)
    {
        uart_rx_callback(rx_byte);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/**
  * @brief  Enable UART receive interrupt (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_UART_EnableRxInterrupt(void (*callback)(uint8_t))
{
    static uint8_t rx_byte;

    uart_rx_callback = callback;
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    return (status == HAL_OK) ? BOOTLOADER_HAL_OK : BOOTLOADER_HAL_ERROR;
}

/* ============================================================================
   GPIO IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Set GPIO pin state (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_WritePin(uint32_t pin_id, bool state)
{
    // Assume pin_id is encoded as: (PORT << 16) | PIN
    GPIO_TypeDef *port = (GPIO_TypeDef*)(GPIOA_BASE + ((pin_id >> 16) * 0x400));
    uint16_t pin = (uint16_t)(pin_id & 0xFFFF);

    HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Toggle GPIO pin state (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_TogglePin(uint32_t pin_id)
{
    GPIO_TypeDef *port = (GPIO_TypeDef*)(GPIOA_BASE + ((pin_id >> 16) * 0x400));
    uint16_t pin = (uint16_t)(pin_id & 0xFFFF);

    HAL_GPIO_TogglePin(port, pin);
    return BOOTLOADER_HAL_OK;
}

/**
  * @brief  Read GPIO pin state (STM32)
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_ReadPin(uint32_t pin_id, bool *state)
{
    GPIO_TypeDef *port = (GPIO_TypeDef*)(GPIOA_BASE + ((pin_id >> 16) * 0x400));
    uint16_t pin = (uint16_t)(pin_id & 0xFFFF);

    *state = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET);
    return BOOTLOADER_HAL_OK;
}

/* ============================================================================
   SYSTEM IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Delay for specified milliseconds (STM32)
  */
void HAL_Delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
  * @brief  Get system tick count (STM32)
  */
uint32_t HAL_GetTick_ms(void)
{
    return HAL_GetTick();
}

/**
  * @brief  Disable all interrupts (STM32 Cortex-M)
  */
void HAL_DisableInterrupts(void)
{
    __disable_irq();
}

/**
  * @brief  Enable all interrupts (STM32 Cortex-M)
  */
void HAL_EnableInterrupts(void)
{
    __enable_irq();
}

/**
  * @brief  System reset (STM32)
  */
void HAL_SystemReset(void)
{
    NVIC_SystemReset();
    while(1);  // Never reached
}

/* ============================================================================
   CACHE IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Disable instruction cache (STM32 F7/H7)
  */
void HAL_DisableICache(void)
{
#if defined(STM32F7) || defined(STM32H7)
    SCB_DisableICache();
#endif
    // F1/F4/L4/G4 have no cache - do nothing
}

/**
  * @brief  Disable data cache (STM32 F7/H7)
  */
void HAL_DisableDCache(void)
{
#if defined(STM32F7) || defined(STM32H7)
    SCB_DisableDCache();
#endif
    // F1/F4/L4/G4 have no cache - do nothing
}

/* ============================================================================
   APPLICATION JUMP IMPLEMENTATION (Cortex-M Specific)
   ============================================================================ */

/**
  * @brief  Jump to user application (Cortex-M)
  */
void HAL_JumpToApplication(uint32_t app_address)
{
    // Get reset handler address
    uint32_t app_reset_handler = *((volatile uint32_t*)(app_address + 4U));
    void (*reset_handler)(void) = (void*)app_reset_handler;

    // Disable all interrupts
    __disable_irq();

    // Disable SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Set Main Stack Pointer
    __set_MSP(*(volatile uint32_t*)app_address);

    // Jump to application
    reset_handler();

    while(1);  // Never reached
}

/**
  * @brief  Validate application exists (Cortex-M)
  */
bool HAL_ValidateApplication(uint32_t app_address)
{
    uint32_t app_reset_handler = *((volatile uint32_t*)(app_address + 4U));
    return (app_reset_handler != 0xFFFFFFFF);
}

/* ============================================================================
   DEBUG OUTPUT IMPLEMENTATION (STM32-Specific)
   ============================================================================ */

/**
  * @brief  Initialize debug output (STM32)
  */
void HAL_Debug_Init(void)
{
    // UART2 already initialized for debug output
}

/**
  * @brief  Print single character (STM32)
  */
void HAL_Debug_PutChar(char ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 10);
}

/**
  * @brief  Connect to printf (GCC)
  */
#ifdef __GNUC__
int __io_putchar(int ch)
{
    HAL_Debug_PutChar((char)ch);
    return ch;
}
#endif

#endif /* PLATFORM_STM32 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
