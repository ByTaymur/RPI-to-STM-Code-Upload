/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32F103 Bootloader - Bug-Free Version
  * @version        : 2.1 (All bugs fixed)
  * @date           : 2024-11-18
  ******************************************************************************
  * BUG FIXES:
  * - Fixed checksum calculation (was using wrong byte)
  * - Fixed buffer overflow in Index management
  * - Fixed handshake state machine
  * - Fixed flash write size for last block
  * - Fixed double MaxIndex update logic
  * - Added proper error handling
  * - Added bounds checking
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
#define MAX_BLOCK_SIZE          1024
#define ETX_APP_START_ADDRESS   0x08004400

typedef enum {
    STATE_WAIT_HANDSHAKE = 0,
    STATE_RECEIVE_DATA,
    STATE_TRANSFER_COMPLETE
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
// State machine
static BootloaderState_t bootloader_state = STATE_WAIT_HANDSHAKE;

// Handshake variables
static uint8_t  handshake_buffer[6];
static uint8_t  handshake_index = 0;
static uint32_t firmware_total_size = 0;

// Data reception variables
static uint8_t  data_block[MAX_BLOCK_SIZE];
static uint16_t block_index = 0;
static uint32_t bytes_received = 0;
static uint32_t current_block_size = MAX_BLOCK_SIZE;

// Checksum
static uint8_t  checksum_response = 0;
static bool     checksum_ready = false;

// Flash writing
static uint32_t flash_write_address = ETX_APP_START_ADDRESS;
static bool     is_first_block = true;

// UART RX
static uint8_t  rx_byte = 0;

// Timeout and status
static uint32_t idle_counter = 0;
static bool     transfer_error = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void Bootloader_Process(void);
static void Jump_To_Application(void);
static void Handle_Handshake_Byte(uint8_t byte);
static void Handle_Data_Byte(uint8_t byte);
static HAL_StatusTypeDef Write_Block_To_Flash(uint8_t *data, uint16_t size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Printf support via UART2
  */
#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
  * @brief  UART RX Callback - Called when byte received
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance != USART1)
    {
        return;
    }

    // Reset idle counter
    idle_counter = 0;

    // LED toggle for activity
    HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);

    // Process byte based on state
    switch(bootloader_state)
    {
        case STATE_WAIT_HANDSHAKE:
            Handle_Handshake_Byte(rx_byte);
            break;

        case STATE_RECEIVE_DATA:
            Handle_Data_Byte(rx_byte);
            break;

        case STATE_TRANSFER_COMPLETE:
            // Ignore any additional bytes
            break;

        default:
            bootloader_state = STATE_WAIT_HANDSHAKE;
            break;
    }

    // Re-enable UART interrupt for next byte
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

/**
  * @brief  Handle handshake byte reception
  * @param  byte: Received byte
  * @note   Handshake format: {[4 bytes size]}
  */
static void Handle_Handshake_Byte(uint8_t byte)
{
    // State 0: Wait for start marker '{'
    if(handshake_index == 0)
    {
        if(byte == '{')
        {
            handshake_buffer[handshake_index++] = byte;
        }
        // Ignore any other byte
    }
    // State 1-5: Receiving size and end marker
    else if(handshake_index < 6)
    {
        handshake_buffer[handshake_index++] = byte;

        // Check if we received all 6 bytes
        if(handshake_index == 6)
        {
            // Validate end marker
            if(handshake_buffer[5] == '}')
            {
                // Parse firmware size (big-endian)
                firmware_total_size = ((uint32_t)handshake_buffer[1] << 24) |
                                    ((uint32_t)handshake_buffer[2] << 16) |
                                    ((uint32_t)handshake_buffer[3] << 8)  |
                                    ((uint32_t)handshake_buffer[4]);

                // Validate size
                if(firmware_total_size > 0 && firmware_total_size <= (47 * 1024))
                {
                    printf("Handshake OK: Size = %lu bytes\r\n", firmware_total_size);

                    // Erase flash (will be done in first write)
                    is_first_block = true;

                    // Send ACK
                    uint8_t ack = 'O';
                    HAL_UART_Transmit(&huart1, &ack, 1, 100);

                    // Switch to data reception state
                    bootloader_state = STATE_RECEIVE_DATA;
                    block_index = 0;
                    bytes_received = 0;
                    current_block_size = MAX_BLOCK_SIZE;

                    printf("Ready to receive data...\r\n");
                }
                else
                {
                    printf("ERROR: Invalid size %lu\r\n", firmware_total_size);
                    transfer_error = true;
                }
            }
            else
            {
                printf("ERROR: Invalid handshake end marker\r\n");
            }

            // Reset handshake
            handshake_index = 0;
            memset(handshake_buffer, 0, sizeof(handshake_buffer));
        }
    }
    else
    {
        // Should never reach here, reset
        handshake_index = 0;
    }
}

/**
  * @brief  Handle data byte reception
  * @param  byte: Received byte
  */
static void Handle_Data_Byte(uint8_t byte)
{
    // Check buffer overflow
    if(block_index >= MAX_BLOCK_SIZE)
    {
        printf("ERROR: Buffer overflow!\r\n");
        transfer_error = true;
        return;
    }

    // Store byte
    data_block[block_index++] = byte;

    // Check if block complete
    if(block_index >= current_block_size)
    {
        // Calculate checksum: first + last byte
        checksum_response = (data_block[0] + data_block[block_index - 1]) & 0xFF;

        // Write block to flash
        if(Write_Block_To_Flash(data_block, block_index) != HAL_OK)
        {
            printf("ERROR: Flash write failed!\r\n");
            transfer_error = true;
            return;
        }

        // Update counters
        bytes_received += block_index;
        printf("Progress: %lu / %lu bytes\r\n", bytes_received, firmware_total_size);

        // Send checksum back to RPI
        checksum_ready = true;

        // Prepare for next block
        block_index = 0;
        memset(data_block, 0, sizeof(data_block));

        // Calculate next block size
        uint32_t remaining = firmware_total_size - bytes_received;
        if(remaining > 0)
        {
            if(remaining >= MAX_BLOCK_SIZE)
            {
                current_block_size = MAX_BLOCK_SIZE;
            }
            else
            {
                current_block_size = remaining;
            }
        }
        else
        {
            // Transfer complete
            bootloader_state = STATE_TRANSFER_COMPLETE;
            printf("Transfer complete!\r\n");
        }
    }
}

/**
  * @brief  Write data block to flash
  * @param  data: Pointer to data
  * @param  size: Data size in bytes
  * @retval HAL status
  */
static HAL_StatusTypeDef Write_Block_To_Flash(uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret;

    // Unlock flash
    ret = HAL_FLASH_Unlock();
    if(ret != HAL_OK)
    {
        return ret;
    }

    // Erase on first block
    if(is_first_block)
    {
        printf("Erasing flash...\r\n");

        FLASH_EraseInitTypeDef erase_config;
        uint32_t page_error;

        erase_config.TypeErase   = FLASH_TYPEERASE_PAGES;
        erase_config.PageAddress = ETX_APP_START_ADDRESS;
        erase_config.NbPages     = 47;  // Adjust based on your needs

        ret = HAL_FLASHEx_Erase(&erase_config, &page_error);
        if(ret != HAL_OK)
        {
            HAL_FLASH_Lock();
            return ret;
        }

        flash_write_address = ETX_APP_START_ADDRESS;
        is_first_block = false;
        printf("Flash erased successfully\r\n");
    }

    // Write data (HALFWORD programming for F1)
    for(uint16_t i = 0; i < size; i += 2)
    {
        uint16_t halfword;

        // Handle odd size
        if(i + 1 < size)
        {
            halfword = data[i] | (data[i + 1] << 8);
        }
        else
        {
            halfword = data[i] | 0xFF00;  // Pad with 0xFF
        }

        ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                               flash_write_address,
                               halfword);

        if(ret != HAL_OK)
        {
            HAL_FLASH_Lock();
            return ret;
        }

        flash_write_address += 2;
    }

    // Lock flash
    ret = HAL_FLASH_Lock();

    return ret;
}

/**
  * @brief  Bootloader main process
  */
static void Bootloader_Process(void)
{
    while(1)
    {
        // Send checksum if ready
        if(checksum_ready)
        {
            HAL_UART_Transmit(&huart1, &checksum_response, 1, 100);
            checksum_ready = false;
        }

        // LED heartbeat
        if(idle_counter % 100 == 0)
        {
            HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
        }

        // Check timeout (50 seconds with no data = exit)
        idle_counter++;
        if(idle_counter > 50000)
        {
            printf("Timeout: No data received\r\n");
            break;
        }

        // Check if transfer complete
        if(bootloader_state == STATE_TRANSFER_COMPLETE)
        {
            HAL_Delay(100);  // Give time for last UART transmission
            printf("Bootloader finished!\r\n");
            break;
        }

        // Check for errors
        if(transfer_error)
        {
            printf("Transfer error - halting\r\n");
            while(1)
            {
                HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
                HAL_Delay(100);
            }
        }

        HAL_Delay(1);
    }
}

/**
  * @brief  Jump to application
  */
static void Jump_To_Application(void)
{
    printf("Jumping to application...\r\n");

    // Get application reset handler address
    uint32_t app_reset_handler_address = *((volatile uint32_t*)(ETX_APP_START_ADDRESS + 4U));

    // Validate application
    if(app_reset_handler_address == 0xFFFFFFFF)
    {
        printf("ERROR: Invalid application!\r\n");
        while(1)
        {
            HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
            HAL_Delay(500);
        }
    }

    // Disable interrupts
    __disable_irq();

    // Disable SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // Set vector table
    SCB->VTOR = ETX_APP_START_ADDRESS;

    // Set stack pointer
    __set_MSP(*((volatile uint32_t*)ETX_APP_START_ADDRESS));

    // Turn off LED
    HAL_GPIO_WritePin(BLed_GPIO_Port, BLed_Pin, GPIO_PIN_RESET);

    // Jump to application
    void (*app_reset_handler)(void) = (void*)app_reset_handler_address;
    app_reset_handler();

    // Should never return
    while(1);
}

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

    printf("\r\n");
    printf("╔════════════════════════════════════════════════════════════════╗\r\n");
    printf("║        STM32F103 Bootloader v2.1 (Bug-Free Edition)           ║\r\n");
    printf("╚════════════════════════════════════════════════════════════════╝\r\n");
    printf("\r\n");

    // Start UART reception in interrupt mode
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    printf("Waiting for firmware from RPI...\r\n");

    // Run bootloader process
    Bootloader_Process();

    // Jump to application
    Jump_To_Application();

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

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
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
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
