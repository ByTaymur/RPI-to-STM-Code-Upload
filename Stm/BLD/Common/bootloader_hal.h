/**
  ******************************************************************************
  * @file    bootloader_hal.h
  * @brief   Hardware Abstraction Layer for Universal Bootloader
  * @author  Production Team
  * @version 4.0
  * @date    2024-11-18
  ******************************************************************************
  * @attention
  *
  * This HAL makes the bootloader COMPLETELY PROCESSOR-INDEPENDENT.
  *
  * Supports: STM32 (Cortex-M), ESP32 (Xtensa/RISC-V), nRF52 (Cortex-M),
  *           PIC32 (MIPS), RISC-V, AVR, etc.
  *
  * Just implement these functions for your target processor!
  *
  ******************************************************************************
  */

#ifndef BOOTLOADER_HAL_H
#define BOOTLOADER_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
   PROCESSOR-INDEPENDENT HAL INTERFACE
   ============================================================================ */

/**
  * @brief  Bootloader HAL Result Codes (universal)
  */
typedef enum {
    BOOTLOADER_HAL_OK       = 0x00,     /**< Success */
    BOOTLOADER_HAL_ERROR    = 0x01,     /**< Generic error */
    BOOTLOADER_HAL_BUSY     = 0x02,     /**< Resource busy */
    BOOTLOADER_HAL_TIMEOUT  = 0x03      /**< Timeout occurred */
} BootloaderHAL_StatusTypeDef;

/* ============================================================================
   FLASH ABSTRACTION (Processor-Independent)
   ============================================================================ */

/**
  * @brief  Unlock flash memory for writing
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Unlock(void);

/**
  * @brief  Lock flash memory after writing
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Lock(void);

/**
  * @brief  Erase flash memory region
  * @param  start_address: Start address to erase
  * @param  size_bytes: Number of bytes to erase
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  *         Implementation should:
  *         - Disable interrupts
  *         - Erase required sectors/pages
  *         - Re-enable interrupts
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Erase(uint32_t start_address,
                                            uint32_t size_bytes);

/**
  * @brief  Write data to flash memory
  * @param  address: Destination address in flash
  * @param  data: Pointer to data buffer
  * @param  size: Number of bytes to write
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  *         Implementation should handle:
  *         - Alignment requirements (8/16/32/64/256-bit)
  *         - Padding if needed
  *         - Error checking
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Write(uint32_t address,
                                            const uint8_t *data,
                                            uint32_t size);

/**
  * @brief  Read data from flash memory
  * @param  address: Source address in flash
  * @param  data: Pointer to destination buffer
  * @param  size: Number of bytes to read
  * @retval HAL status
  * @note   OPTIONAL - can use direct memory access on most platforms
  */
BootloaderHAL_StatusTypeDef HAL_Flash_Read(uint32_t address,
                                           uint8_t *data,
                                           uint32_t size);

/* ============================================================================
   UART ABSTRACTION (Processor-Independent)
   ============================================================================ */

/**
  * @brief  Initialize UART for bootloader communication
  * @param  baudrate: Baud rate (typically 115200)
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_UART_Init(uint32_t baudrate);

/**
  * @brief  Transmit single byte via UART
  * @param  byte: Byte to transmit
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_UART_TransmitByte(uint8_t byte);

/**
  * @brief  Transmit multiple bytes via UART
  * @param  data: Pointer to data buffer
  * @param  size: Number of bytes to transmit
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_UART_Transmit(const uint8_t *data,
                                              uint32_t size);

/**
  * @brief  Receive single byte via UART
  * @param  byte: Pointer to store received byte
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_UART_ReceiveByte(uint8_t *byte);

/**
  * @brief  Enable UART receive interrupt
  * @param  callback: Function to call on byte received
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_UART_EnableRxInterrupt(void (*callback)(uint8_t byte));

/* ============================================================================
   GPIO ABSTRACTION (Processor-Independent)
   ============================================================================ */

/**
  * @brief  Set GPIO pin state
  * @param  pin_id: Pin identifier (platform-specific)
  * @param  state: true = HIGH, false = LOW
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_WritePin(uint32_t pin_id, bool state);

/**
  * @brief  Toggle GPIO pin state
  * @param  pin_id: Pin identifier (platform-specific)
  * @retval HAL status
  * @note   MUST BE IMPLEMENTED for each processor
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_TogglePin(uint32_t pin_id);

/**
  * @brief  Read GPIO pin state
  * @param  pin_id: Pin identifier (platform-specific)
  * @param  state: Pointer to store pin state
  * @retval HAL status
  * @note   OPTIONAL - for button input
  */
BootloaderHAL_StatusTypeDef HAL_GPIO_ReadPin(uint32_t pin_id, bool *state);

/* ============================================================================
   SYSTEM ABSTRACTION (Processor-Independent)
   ============================================================================ */

/**
  * @brief  Delay for specified milliseconds
  * @param  ms: Milliseconds to delay
  * @retval None
  * @note   MUST BE IMPLEMENTED for each processor
  */
void HAL_Delay_ms(uint32_t ms);

/**
  * @brief  Get system tick count (milliseconds since boot)
  * @retval Current tick count
  * @note   MUST BE IMPLEMENTED for each processor
  */
uint32_t HAL_GetTick_ms(void);

/**
  * @brief  Disable all interrupts
  * @retval None
  * @note   MUST BE IMPLEMENTED for each processor
  */
void HAL_DisableInterrupts(void);

/**
  * @brief  Enable all interrupts
  * @retval None
  * @note   MUST BE IMPLEMENTED for each processor
  */
void HAL_EnableInterrupts(void);

/**
  * @brief  System reset
  * @retval None (never returns)
  * @note   MUST BE IMPLEMENTED for each processor
  */
void HAL_SystemReset(void) __attribute__((noreturn));

/* ============================================================================
   CACHE ABSTRACTION (Processor-Dependent - Optional)
   ============================================================================ */

/**
  * @brief  Disable instruction cache
  * @retval None
  * @note   OPTIONAL - implement only if processor has I-Cache
  *         Leave empty if not applicable
  */
void HAL_DisableICache(void);

/**
  * @brief  Disable data cache
  * @retval None
  * @note   OPTIONAL - implement only if processor has D-Cache
  *         Leave empty if not applicable
  */
void HAL_DisableDCache(void);

/* ============================================================================
   APPLICATION JUMP ABSTRACTION (Architecture-Specific)
   ============================================================================ */

/**
  * @brief  Jump to user application
  * @param  app_address: Application start address
  * @retval None (never returns)
  * @note   MUST BE IMPLEMENTED for each architecture
  *
  * Implementation varies by architecture:
  *
  * Cortex-M (STM32, nRF52):
  *   void HAL_JumpToApplication(uint32_t app_address) {
  *       uint32_t msp = *(volatile uint32_t*)app_address;
  *       uint32_t reset_handler = *(volatile uint32_t*)(app_address + 4);
  *       __set_MSP(msp);
  *       ((void(*)())reset_handler)();
  *   }
  *
  * RISC-V:
  *   void HAL_JumpToApplication(uint32_t app_address) {
  *       asm volatile("jr %0" : : "r"(app_address));
  *   }
  *
  * AVR:
  *   void HAL_JumpToApplication(uint32_t app_address) {
  *       asm volatile("jmp 0x0000");
  *   }
  *
  * ESP32 (Xtensa):
  *   void HAL_JumpToApplication(uint32_t app_address) {
  *       esp_restart();  // or partition boot
  *   }
  */
void HAL_JumpToApplication(uint32_t app_address) __attribute__((noreturn));

/**
  * @brief  Validate application exists at address
  * @param  app_address: Application start address
  * @retval true if valid application found
  * @note   MUST BE IMPLEMENTED for each architecture
  *
  * Implementation varies by architecture:
  *
  * Cortex-M:
  *   bool HAL_ValidateApplication(uint32_t addr) {
  *       uint32_t reset_handler = *(uint32_t*)(addr + 4);
  *       return (reset_handler != 0xFFFFFFFF);
  *   }
  *
  * RISC-V/AVR:
  *   bool HAL_ValidateApplication(uint32_t addr) {
  *       uint8_t first_byte = *(uint8_t*)addr;
  *       return (first_byte != 0xFF);
  *   }
  */
bool HAL_ValidateApplication(uint32_t app_address);

/* ============================================================================
   PRINTF ABSTRACTION (Optional but recommended)
   ============================================================================ */

/**
  * @brief  Initialize debug output (UART/ITM/SWO/etc.)
  * @retval None
  * @note   OPTIONAL - implement for printf support
  */
void HAL_Debug_Init(void);

/**
  * @brief  Print single character to debug output
  * @param  ch: Character to print
  * @retval None
  * @note   OPTIONAL - implement for printf support
  *         Connect to _write() or __io_putchar()
  */
void HAL_Debug_PutChar(char ch);

/* ============================================================================
   PLATFORM DETECTION (Compile-time)
   ============================================================================ */

/**
  * @brief  Platform detection and sanity checks
  */
#if defined(STM32F1) || defined(STM32F4) || defined(STM32F7) || \
    defined(STM32H7) || defined(STM32L4) || defined(STM32G4)
    #define PLATFORM_STM32
    #define ARCH_CORTEX_M
#elif defined(ESP32)
    #define PLATFORM_ESP32
    #ifdef CONFIG_IDF_TARGET_ESP32C3
        #define ARCH_RISCV
    #else
        #define ARCH_XTENSA
    #endif
#elif defined(NRF52) || defined(NRF52832) || defined(NRF52840)
    #define PLATFORM_NRF52
    #define ARCH_CORTEX_M
#elif defined(__AVR__)
    #define PLATFORM_AVR
    #define ARCH_AVR
#elif defined(__riscv)
    #define PLATFORM_GENERIC_RISCV
    #define ARCH_RISCV
#else
    #warning "Unknown platform - please define HAL functions manually"
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_HAL_H */

/**
  * @}
  */

/************************ Processor-Independent HAL ***********************/
