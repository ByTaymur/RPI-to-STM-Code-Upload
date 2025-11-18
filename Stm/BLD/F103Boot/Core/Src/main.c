/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Memory configuration
#define MAX_BLOCK_SIZE          1024                      // 1KB block size
#define ETX_APP_START_ADDRESS   0x08004400                // Application start address
#define MAX_FIRMWARE_SIZE       (47 * 1024)               // 47KB max for F103

// Protocol constants
#define PROTOCOL_START_BYTE     '{'
#define PROTOCOL_END_BYTE       '}'
#define PROTOCOL_ACK_BYTE       'O'
#define PROTOCOL_NACK_BYTE      'N'

// Handshake validation
#define HANDSHAKE_MIN_INDEX     4
#define HANDSHAKE_MAX_INDEX     8

// Flash configuration
#define FLASH_ERASE_PAGES       47                        // Number of pages to erase

// State machine - Clean enum instead of magic numbers
typedef enum {
    BOOTLOADER_STATE_IDLE = 0,
    BOOTLOADER_STATE_HANDSHAKE_RECEIVED = 10,
    BOOTLOADER_STATE_TRANSFERRING = 20,
    BOOTLOADER_STATE_COMPLETE = 30,
    BOOTLOADER_STATE_ERROR = 255
} BootloaderState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void Application( void );
static void Firmware_Update( void );

// Helper functions for better code organization
static bool Validate_Handshake_Byte(uint8_t byte);
static bool Parse_Firmware_Size(void);
static void Send_ACK_Response(void);
static void Handle_Block_Complete(void);
static uint8_t Calculate_Checksum(const uint8_t *block, uint32_t size);
static void Reset_Transfer_State(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ============================================================================
// BOOTLOADER STATE MACHINE VARIABLES
// ============================================================================
static BootloaderState_t bootloader_state = BOOTLOADER_STATE_IDLE;
static uint8_t ack_byte = PROTOCOL_ACK_BYTE;

// ============================================================================
// UART RECEPTION VARIABLES
// ============================================================================
static uint8_t uart_rx_byte = 0;

// ============================================================================
// DATA BLOCK RECEPTION VARIABLES
// ============================================================================
static uint8_t data_block[MAX_BLOCK_SIZE];
static uint16_t block_index = 0;
static uint32_t block_index_saved = 0;  // Saved before reset for checksum

// ============================================================================
// FIRMWARE SIZE TRACKING
// ============================================================================
static uint32_t firmware_total_size = 0;
static uint32_t firmware_received_size = 0;
static uint32_t bytes_remaining = MAX_BLOCK_SIZE;
static uint32_t current_block_size = MAX_BLOCK_SIZE;

// ============================================================================
// CHECKSUM VARIABLES
// ============================================================================
static uint8_t checksum_response[1] = {0};

// ============================================================================
// FLASH WRITE VARIABLES
// ============================================================================
static uint32_t flash_write_index = 0;

// ============================================================================
// TIMEOUT AND STATUS COUNTERS
// ============================================================================
static uint8_t handshake_count = 0;
static uint32_t idle_timeout_counter = 0;
static bool transfer_error_flag = false;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  // Print banner
  printf("\r\n");
  printf("╔════════════════════════════════════════════════════════════════╗\r\n");
  printf("║       STM32F103 Bootloader v3.0 (Production Quality)          ║\r\n");
  printf("╠════════════════════════════════════════════════════════════════╣\r\n");
  printf("║  Application: 0x%08lX                                      ║\r\n", ETX_APP_START_ADDRESS);
  printf("║  Max Size:    %d KB                                         ║\r\n", MAX_FIRMWARE_SIZE / 1024);
  printf("║  Block Size:  %d bytes                                      ║\r\n", MAX_BLOCK_SIZE);
  printf("╚════════════════════════════════════════════════════════════════╝\r\n");
  printf("\r\n");

  // Start UART reception in interrupt mode
  HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

  // Run firmware update process
  Firmware_Update();

  // Jump to user application
  Application();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLed_GPIO_Port, BLed_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BLed_Pin */
  GPIO_InitStruct.Pin = BLed_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BLed_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RpiBootEnb_Pin */
  GPIO_InitStruct.Pin = RpiBootEnb_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RpiBootEnb_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

  return ch;
}


/**
  * @brief  Write data block to application flash area
  * @param  data: Pointer to data buffer
  * @param  data_len: Length of data in bytes
  * @param  is_first_block: true if this is the first block (triggers erase)
  * @retval HAL_StatusTypeDef
  */
static HAL_StatusTypeDef write_data_to_flash_app(uint8_t *data,
                                                  uint16_t data_len,
                                                  bool is_first_block)
{
    HAL_StatusTypeDef status = HAL_OK;

    // Validate input parameters
    if(data == NULL || data_len == 0)
    {
        printf("ERROR: Invalid flash write parameters\r\n");
        return HAL_ERROR;
    }

    // Unlock flash for write operations
    status = HAL_FLASH_Unlock();
    if(status != HAL_OK)
    {
        printf("ERROR: Flash unlock failed\r\n");
        return status;
    }

    // Erase flash on first block
    if(is_first_block)
    {
        printf("Erasing application flash area...\r\n");

        FLASH_EraseInitTypeDef erase_config;
        uint32_t erase_error = 0;

        erase_config.TypeErase = FLASH_TYPEERASE_PAGES;
        erase_config.PageAddress = ETX_APP_START_ADDRESS;
        erase_config.NbPages = FLASH_ERASE_PAGES;

        // CRITICAL: Disable interrupts during erase to prevent corruption
        __disable_irq();
        status = HAL_FLASHEx_Erase(&erase_config, &erase_error);
        __enable_irq();

        if(status != HAL_OK)
        {
            printf("ERROR: Flash erase failed (error: 0x%08lX)\r\n", erase_error);
            HAL_FLASH_Lock();
            return status;
        }

        flash_write_index = 0;
        printf("Flash erase complete\r\n");
    }

    // Write data (F103 uses HALFWORD programming)
    uint32_t write_address = ETX_APP_START_ADDRESS + flash_write_index;

    for(uint16_t i = 0; i < data_len; i += 2)
    {
        uint16_t halfword;

        // Handle odd data length
        if(i + 1 < data_len)
        {
            halfword = data[i] | (data[i + 1] << 8);
        }
        else
        {
            halfword = data[i] | 0xFF00;  // Pad with 0xFF
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   write_address,
                                   halfword);

        if(status != HAL_OK)
        {
            printf("ERROR: Flash write failed at 0x%08lX\r\n", write_address);
            break;
        }

        write_address += 2;
        flash_write_index += 2;
    }

    // Lock flash
    HAL_FLASH_Lock();

    return status;
}

/**
  * @brief  Calculate checksum (first byte + last byte) & 0xFF
  * @param  block: Pointer to data block
  * @param  size: Size of block
  * @retval Calculated checksum
  */
static uint8_t Calculate_Checksum(const uint8_t *block, uint32_t size)
{
    if(block == NULL || size == 0)
    {
        return 0;
    }

    return (block[0] + block[size - 1]) & 0xFF;
}

/**
  * @brief  Reset transfer state to idle
  * @retval None
  */
static void Reset_Transfer_State(void)
{
    bootloader_state = BOOTLOADER_STATE_IDLE;
    block_index = 0;
    firmware_total_size = 0;
    firmware_received_size = 0;
    bytes_remaining = MAX_BLOCK_SIZE;
    current_block_size = MAX_BLOCK_SIZE;
    transfer_error_flag = false;
    memset(data_block, 0, sizeof(data_block));
}

/**
  * @brief  UART RX Callback - Called on each byte received
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // Reset idle counter
    idle_timeout_counter = 0;

    // LED toggle for activity indication
    HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);

    // Buffer overflow protection - CRITICAL SAFETY CHECK
    if(block_index >= MAX_BLOCK_SIZE)
    {
        printf("ERROR: Buffer overflow detected!\r\n");
        transfer_error_flag = true;
        block_index = 0;
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
        return;
    }

    // Store received byte
    data_block[block_index++] = uart_rx_byte;

    // ===========================================================================
    // HANDSHAKE STATE: Wait for {SIZE} packet
    // ===========================================================================
    if(bootloader_state != BOOTLOADER_STATE_TRANSFERRING)
    {
        // Look for start byte '{'
        if((PROTOCOL_START_BYTE == uart_rx_byte || data_block[0] == 0x7B) &&
           bootloader_state == BOOTLOADER_STATE_IDLE)
        {
            bootloader_state = BOOTLOADER_STATE_HANDSHAKE_RECEIVED;
        }

        // Check for complete handshake packet
        if(bootloader_state == BOOTLOADER_STATE_HANDSHAKE_RECEIVED &&
           (PROTOCOL_END_BYTE == uart_rx_byte || uart_rx_byte == 0x7D) &&
           block_index < HANDSHAKE_MAX_INDEX && block_index > HANDSHAKE_MIN_INDEX)
        {
            // Parse firmware size (big-endian)
            data_block[0] = 0;  // Clear markers
            data_block[5] = 0;
            firmware_total_size = ((uint32_t)data_block[1] << 24) |
                                 ((uint32_t)data_block[2] << 16) |
                                 ((uint32_t)data_block[3] << 8)  |
                                 ((uint32_t)data_block[4]);

            // Validate firmware size
            if(firmware_total_size > MAX_FIRMWARE_SIZE)
            {
                printf("ERROR: Firmware too large (%lu bytes). Max: %d KB\r\n",
                       firmware_total_size, MAX_FIRMWARE_SIZE / 1024);
                Reset_Transfer_State();
                HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
                return;
            }

            // Handshake successful
            printf("Handshake OK: Firmware size = %lu bytes\r\n", firmware_total_size);
            bytes_remaining = firmware_total_size;
            firmware_received_size = 0;
            handshake_count++;
            block_index = 0;
            bootloader_state = BOOTLOADER_STATE_TRANSFERRING;
        }
    }

    // ===========================================================================
    // DATA TRANSFER STATE: Receive firmware blocks
    // ===========================================================================
    if(block_index == current_block_size)
    {
        // Save block index before reset (needed for checksum)
        block_index_saved = block_index;

        // Update remaining bytes
        if(bytes_remaining >= MAX_BLOCK_SIZE)
        {
            bytes_remaining -= current_block_size;
            current_block_size = MAX_BLOCK_SIZE;
            firmware_received_size += current_block_size;
        }
        else if(bytes_remaining < MAX_BLOCK_SIZE)
        {
            current_block_size = bytes_remaining;
            firmware_received_size += current_block_size;
        }

        // Write to flash if block is complete or transfer is done
        if((block_index == MAX_BLOCK_SIZE) ||
           (firmware_received_size >= firmware_total_size))
        {
            printf("\rProgress: %lu / %lu bytes\r\n",
                   firmware_received_size, firmware_total_size);

            // Write block to flash
            if(write_data_to_flash_app(data_block, block_index_saved,
                                      (firmware_received_size <= MAX_BLOCK_SIZE)) != HAL_OK)
            {
                printf("ERROR: Flash write failed!\r\n");
                transfer_error_flag = true;
            }
        }

        // Reset block buffer
        block_index = 0;

        // Calculate and prepare checksum response
        checksum_response[0] = Calculate_Checksum(data_block, block_index_saved);
        memset(data_block, 0, sizeof(data_block));
    }

    // Re-enable UART interrupt for next byte
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}
/**
  * @brief  Firmware update main loop
  * @retval None
  */
static void Firmware_Update(void)
{
    const uint32_t TIMEOUT_MS = 50000;  // 50 second timeout
    bool checksum_sent = false;

    printf("Bootloader ready - waiting for firmware...\r\n");

    while(1)
    {
        // Send ACK after successful handshake
        if(bootloader_state == BOOTLOADER_STATE_TRANSFERRING && !checksum_sent)
        {
            HAL_Delay(1);
            HAL_UART_Transmit_IT(&huart1, &ack_byte, 1);
            checksum_sent = true;
        }

        // Send checksum response if block complete
        if(block_index_saved > 0)
        {
            HAL_Delay(1);
            HAL_UART_Transmit_IT(&huart1, checksum_response, 1);
            block_index_saved = 0;
        }

        // LED heartbeat
        idle_timeout_counter++;
        if(idle_timeout_counter % 10 == 0)
        {
            HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
        }

        HAL_Delay(1);

        // Check transfer complete
        if((firmware_received_size >= firmware_total_size) &&
           (bootloader_state == BOOTLOADER_STATE_TRANSFERRING) &&
           (idle_timeout_counter > 1000))
        {
            // Send final checksum
            HAL_UART_Transmit_IT(&huart1, checksum_response, 1);
            printf("Firmware transfer complete!\r\n");
            bootloader_state = BOOTLOADER_STATE_COMPLETE;
            break;
        }

        // Check timeout
        if(idle_timeout_counter > TIMEOUT_MS)
        {
            printf("Timeout: No data received\r\n");
            break;
        }

        // Check for errors
        if(transfer_error_flag)
        {
            printf("Transfer error detected\r\n");
            bootloader_state = BOOTLOADER_STATE_ERROR;
            break;
        }
    }
}
/**
  * @brief  Jump to user application
  * @retval None
  */
static void Application(void)
{
    printf("Preparing to jump to application...\r\n");

    // Get application reset handler address
    uint32_t app_reset_handler_address = *((volatile uint32_t*)(ETX_APP_START_ADDRESS + 4U));
    void (*app_reset_handler)(void) = (void*)app_reset_handler_address;

    // Validate application exists
    if(app_reset_handler_address == 0xFFFFFFFF)
    {
        printf("ERROR: No valid application found!\r\n");
        printf("Flash appears empty at 0x%08lX\r\n", ETX_APP_START_ADDRESS);
        while(1)
        {
            HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
            HAL_Delay(200);  // Fast blink = error
        }
    }

    printf("Valid application found at 0x%08lX\r\n", ETX_APP_START_ADDRESS);
    printf("Reset handler: 0x%08lX\r\n", app_reset_handler_address);

    // Disable interrupts before jump
    __disable_irq();

    // Disable SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // Set vector table offset
    SCB->VTOR = ETX_APP_START_ADDRESS;

    // Set stack pointer
    __set_MSP(*((volatile uint32_t*)ETX_APP_START_ADDRESS));

    // Turn off LED to indicate bootloader exit
    HAL_GPIO_WritePin(BLed_GPIO_Port, BLed_Pin, GPIO_PIN_RESET);

    // Jump to application
    app_reset_handler();

    // Should never reach here
    while(1);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf('Wrong parameters value: file %s on line %d\r\n', file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
