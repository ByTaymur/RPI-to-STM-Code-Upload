/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - STM32F7 Bootloader
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#define ETX_APP_START_ADDRESS   0x08008000                // Sector 1 start (after 32KB bootloader)
#define MAX_FIRMWARE_SIZE       (992 * 1024)              // 992KB max for F7 (Sectors 1-7)

// Protocol constants
#define PROTOCOL_START_BYTE     '{'
#define PROTOCOL_END_BYTE       '}'
#define PROTOCOL_ACK_BYTE       'O'
#define PROTOCOL_NACK_BYTE      'N'

// Handshake validation
#define HANDSHAKE_MIN_INDEX     4
#define HANDSHAKE_MAX_INDEX     8

// Flash configuration for STM32F7
#define FLASH_ERASE_FIRST_SECTOR    FLASH_SECTOR_1        // Start sector
#define FLASH_ERASE_SECTOR_COUNT    7                     // Sectors 1-7
#define FLASH_VOLTAGE_RANGE_VAL     FLASH_VOLTAGE_RANGE_3 // 2.7V-3.6V

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
static void CPU_CACHE_Enable(void);
/* USER CODE BEGIN PFP */
static void Application( void );
static void Firmware_Update( void );

// Helper functions for better code organization
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
// FLASH WRITE VARIABLES (32-bit for STM32F7)
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
  CPU_CACHE_Enable();
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

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
	printf("STM32F7 Bootloader Started...\r\n");
	Firmware_Update();
	Application();

  while (1)
  {


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration for STM32F7
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;      // 8MHz HSE / 8 = 1MHz
  RCC_OscInitStruct.PLL.PLLN = 432;    // 1MHz * 432 = 432MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 432MHz / 2 = 216MHz
  RCC_OscInitStruct.PLL.PLLQ = 9;      // 432MHz / 9 = 48MHz (USB)

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 216MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;    // 54MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;    // 108MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
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

/**
  * @brief  Enable CPU Cache
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART2 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

  return ch;
}

/**
  * @brief  Get Flash sector number for STM32F7
  * @param  Address: Flash address
  * @retval Sector number
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < 0x08008000) && (Address >= 0x08000000))
  {
    sector = FLASH_SECTOR_0;  // 32KB
  }
  else if((Address < 0x08010000) && (Address >= 0x08008000))
  {
    sector = FLASH_SECTOR_1;  // 32KB
  }
  else if((Address < 0x08018000) && (Address >= 0x08010000))
  {
    sector = FLASH_SECTOR_2;  // 32KB
  }
  else if((Address < 0x08020000) && (Address >= 0x08018000))
  {
    sector = FLASH_SECTOR_3;  // 32KB
  }
  else if((Address < 0x08040000) && (Address >= 0x08020000))
  {
    sector = FLASH_SECTOR_4;  // 128KB
  }
  else if((Address < 0x08080000) && (Address >= 0x08040000))
  {
    sector = FLASH_SECTOR_5;  // 256KB
  }
  else if((Address < 0x080C0000) && (Address >= 0x08080000))
  {
    sector = FLASH_SECTOR_6;  // 256KB
  }
  else if((Address < 0x08100000) && (Address >= 0x080C0000))
  {
    sector = FLASH_SECTOR_7;  // 256KB
  }

  return sector;
}

static HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,
                                        uint16_t data_len, bool is_first_block )
{
  HAL_StatusTypeDef ret;

  do
  {
    ret = HAL_FLASH_Unlock();
    if( ret != HAL_OK )
    {
      break;
    }

    //No need to erase every time. Erase only the first time.
    if( is_first_block )
    {
      //printf("Erasing the Flash memory...\r\n");
      //Erase the Flash sectors for application (Sector 1-7)
      FLASH_EraseInitTypeDef EraseInitStruct;
      uint32_t SectorError;

      EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
      EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3; // 2.7V to 3.6V
      EraseInitStruct.Sector        = FLASH_SECTOR_1;        // Start from Sector 1
      EraseInitStruct.NbSectors     = 7;                     // Erase Sectors 1-7

      // CRITICAL: Disable interrupts during flash erase to prevent corruption
      __disable_irq();
      ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
      __enable_irq();

      if( ret != HAL_OK )
      {
        break;
      }
      application_write_idx = 0;
    }

    // STM32F7 uses WORD (32-bit) programming
    for(int i = 0; i < data_len/4; i++)
    {
      uint32_t word_data = data[i * 4] |
                          (data[i * 4 + 1] << 8) |
                          (data[i * 4 + 2] << 16) |
                          (data[i * 4 + 3] << 24);

      ret = HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD,
                               (ETX_APP_START_ADDRESS + application_write_idx ),
                               word_data
                             );
      if( ret == HAL_OK )
      {
        //update the data count
        application_write_idx += 4;
      }
      else
      {
        printf("Flash Write Error...HALT!!!\r\n");
        break;
      }
    }

    if( ret != HAL_OK )
    {
      break;
    }

    ret = HAL_FLASH_Lock();
    if( ret != HAL_OK )
    {
      break;
    }
  }while( false );

  return ret;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	DataFlagCount=0;
	HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);

	// Buffer overflow protection
	if(Index >= MAX_BLOCK_SIZE)
	{
		printf("ERROR: Buffer overflow!\r\n");
		Index = 0;
		HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
		return;
	}

	Block[Index++]=RPiDataByte;

	if(IlkSifre!=10)
	{
			if(( '{' == RPiDataByte || Block[0]== 0x7B) && IlkSifre==0) IlkSifre=1;
			if( IlkSifre==1 &&( RPiDataByte == '}' || RPiDataByte ==  0x7D  ) && Index < 8  && Index > 4 )
			{
					Block[0]=0;	Block[5]=0;
					BLeng = Block[1]<<24 | Block[2]<<16 | Block[3]<<8 | Block[4];

					// Validate firmware size (max ~992KB for F7 - Sectors 1-7)
					// Sector 1-3: 3*32KB = 96KB
					// Sector 4: 128KB
					// Sector 5-7: 3*256KB = 768KB
					// Total: 992KB
					if(BLeng > (992 * 1024))
					{
						printf("ERROR: Firmware too large (%lu bytes). Max: 992KB\r\n", BLeng);
						IlkSifre = 0;
						Index = 0;
						HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
						return;
					}

					BlockLeng=BLeng;
					DataCount++;
					Index=0;
					IlkSifre=10;
			}
		}
  if( Index == MaxIndex  )
	{
	  	IndexSum=Index;
		if(BLeng>=1024)
		{
			BLeng=BLeng-MaxIndex;
			MaxIndex = 1024;
			current_app_size=MaxIndex+current_app_size;
		}
		else if(BLeng<1024)  // FIXED: Changed to else if
		{
			MaxIndex = BLeng;
			current_app_size=MaxIndex+current_app_size;
		}
		if( ( Index == MAX_BLOCK_SIZE ) || ( current_app_size >= BlockLeng) )
		{
			printf("\rTransfer %d \r\n", ( BlockLeng-BLeng ));
			// FIXED: Write actual block size (IndexSum) instead of always MAX_BLOCK_SIZE
			if( write_data_to_flash_app(Block, IndexSum, (current_app_size <= MAX_BLOCK_SIZE) ) != HAL_OK )
			{
				printf("HALT!!!\r\n");
			}
		}
		Index=0;
		// CRITICAL FIX: Use correct checksum calculation
		// Calculate checksum as (first_byte + last_byte) & 0xFF
		// The last byte is at position IndexSum-1 (before Index was reset)
		Sum[0] = (Block[0] + Block[IndexSum - 1]) & 0xFF;
		memset(Block, 0, sizeof(Block));
	}
	HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
}

static void Firmware_Update(void)
{
	  while (1)
  {
			if(IlkSifre==10)
			{
				HAL_Delay(1);
				HAL_UART_Transmit_IT(&huart1,DataGonder, 1);
				Index=0;
				IlkSifre=20;
			}
			if(IndexSum>0)
			{
				HAL_Delay(1);
				HAL_UART_Transmit_IT(&huart1,Sum, 1);
				IndexSum=0;
			}
			DataFlagCount++;
			if(DataFlagCount%10==0)HAL_GPIO_TogglePin(BLed_GPIO_Port, BLed_Pin);
			HAL_Delay(1);
			if((current_app_size >= BlockLeng && (IlkSifre==20 && DataFlagCount>1000) )|| DataFlagCount>5000)
			{
				HAL_UART_Transmit_IT(&huart1,Sum, 1);
				printf("Boot Finished...\r\n");
				break;

			}
		}
}

static void Application( void )
{
	printf("Jumping to Application...\n");
	void (*app_reset_handler)(void) = (void*)(*((volatile uint32_t*)(ETX_APP_START_ADDRESS + 4U)));

	if( app_reset_handler == (void*)0xFFFFFFFF )
	{
	  printf("Invalid Application... HALT!!!\r\n");
	  while(1);
	}

	// Disable interrupts
	__disable_irq();

	// Disable SysTick
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	// Set MSP
	__set_MSP(*(volatile uint32_t*) ETX_APP_START_ADDRESS);

	// Turn OFF the Led to tell the user that Bootloader is not running
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET );

	// Jump to application
	app_reset_handler();
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
