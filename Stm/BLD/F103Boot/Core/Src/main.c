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
#define MAX_BLOCK_SIZE          ( 1024 )                  //1KB
#define ETX_APP_START_ADDRESS   0x08004400
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
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t RPiDataByte=0;
uint8_t BlockNumber1=0;
uint8_t BlockNumber2=0;
uint8_t BlockNumber3=0;
uint8_t BlockLeng1=0;
uint8_t BlockLeng2=0;
uint8_t BlockLeng3=0;
uint8_t BlockLeng4=0;
uint8_t Block[1024];
uint16_t Index=0;
uint8_t BlockOk=0;
uint8_t DataGonder[]={'O'};
uint16_t BNumber=0;
uint32_t BLeng=1024;

uint32_t BlockLeng=1024;
uint8_t   IndexCount=0;
uint8_t   DataCount=0;
uint16_t   Conter=0;
char BlockTest[1024];
uint8_t IlkSifre=0;

uint32_t MaxIndex=1024;

uint32_t BLengC=0;
uint8_t Sum[1]={0};
uint32_t IndexSum=0;
uint8_t Sum1=0;
uint8_t Sum2=0;

uint16_t application_size = 0;
uint16_t application_write_idx = 0;

uint16_t current_app_size=0;
uint32_t DataFlagCount=0;
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

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
	printf("Program Start...\r\n");
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
      //Erase the Flash
      FLASH_EraseInitTypeDef EraseInitStruct;
      uint32_t SectorError;

      EraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
      EraseInitStruct.PageAddress   = ETX_APP_START_ADDRESS;
      EraseInitStruct.NbPages       = 47;                     //47 Pages

      ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
      if( ret != HAL_OK )
      {
        break;
      }
      application_write_idx = 0;
    }

    for(int i = 0; i < data_len/2; i++)
    {
      uint16_t halfword_data = data[i * 2] | (data[i * 2 + 1] << 8);
      ret = HAL_FLASH_Program( FLASH_TYPEPROGRAM_HALFWORD,
                               (ETX_APP_START_ADDRESS + application_write_idx ),
                               halfword_data
                             );
      if( ret == HAL_OK )
      {
        //update the data count
        application_write_idx += 2;
      }
      else
      {
       // printf("Flash Write Error...HALT!!!\r\n");
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
	Block[Index++]=RPiDataByte;
	if(IlkSifre!=10)
	{
			if(( '{' == RPiDataByte || Block[0]== 0x7B) && IlkSifre==0) IlkSifre=1;
			if( IlkSifre==1 &&( RPiDataByte == '}' || RPiDataByte ==  0x7D  ) && Index < 8  && Index > 4 )
			{
					Block[0]=0;	Block[5]=0;
					BLeng = Block[1]<<24 | Block[2]<<16 | Block[3]<<8 | Block[4];
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
		if(BLeng<1024)
		{
			MaxIndex = BLeng;
			current_app_size=MaxIndex+current_app_size;
		}
		if( ( Index == MAX_BLOCK_SIZE ) || ( current_app_size >= BlockLeng) )
		{
			printf("\rTransfer %d \r\n", ( BlockLeng-BLeng ));
			if( write_data_to_flash_app(Block, MAX_BLOCK_SIZE, (current_app_size <= MAX_BLOCK_SIZE) ) != HAL_OK )
			{
				printf("HALT!!!\r\n");
			}
		}
		Index=0;
		Sum[0] =( Block[0] + RPiDataByte ) & 0xFF;
		memset(Block, 0, sizeof(Block));
	}
	HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
	//HAL_UART_Receive_IT(&huart1, &RPiDataByte, 1);
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
	printf("Application...\n");
	void (*app_reset_handler)(void) = (void*)(*((volatile uint32_t*)(ETX_APP_START_ADDRESS + 4U)));

	if( app_reset_handler == (void*)0xFFFFFFFF )
	{
	  printf("Invalid Application... HALT!!!\r\n");
	  while(1);
	}

	__set_MSP(*(volatile uint32_t*) ETX_APP_START_ADDRESS);

	// Turn OFF the Led to tell the user that Bootloader is not running
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET );

	app_reset_handler();    //call the app reset handler
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
