/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * )
 ******************************************************************************
 * SOURCE OF THIS TUTORIAL:
 * 	PART1 & 2 (THIS REVISION) https://www.youtube.com/watch?v=jzo7z2gNBgg&list=PLArwqFvBIlwHRgPtsQAhgZavlp42qpkiG&index=1
 *
 */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "etx_ota_update.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#define DEMCR                 *((volatile uint32_t*) 0xE000EDFCu)

/* ITM register addresses */
#define ITM_STIMULUS_PORT0    *((volatile uint32_t*) 0xE0000000u)
#define ITM_TRACE_EN          *((volatile uint32_t*) 0xE0000E00u)
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAJOR		1		// Major revision
#define MINOR		0		//Minor revision

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
const uint8_t BL_Version[2]={MAJOR,MINOR};
char BOOTString[64];
uint16_t fwsize = 0;
uint16_t ota_fw_received_size=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void Set_BOOTMODE_LED(void);
void Set_APPMODE_LED(void);
static void goto_application(void);
static int write_data_to_flash_app( uint8_t *data,uint16_t data_len, bool is_first_block);
static void etx_ota_send_resp( uint8_t type );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint32_t index=0;

	uint16_t chunksize=0;
	int ex =0;

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
  /* USER CODE BEGIN 2 */
	printf("Starting Bootloader %d.%d \n\r",BL_Version[0],BL_Version[1]);
	Set_BOOTMODE_LED();

	/* Check the GPIO for 3 seconds */
	GPIO_PinState OTA_Pin_state;
	uint32_t end_tick = HAL_GetTick() + 3000;   // from now to 3 Seconds

	printf("Press the User Button PA0 to trigger OTA update...\r\n");
	do
	{
		OTA_Pin_state = HAL_GPIO_ReadPin( BTN_OTA_GPIO_Port, BTN_OTA_Pin);
		uint32_t current_tick = HAL_GetTick();

		/* Check the button is pressed or not for 3seconds */
		if( ( OTA_Pin_state != GPIO_PIN_RESET ) || ( current_tick > end_tick ) )
		{
			/* Either timeout or Button is pressed */
			break;
		}
	}while( 1 );

	/*Start the Firmware or Application update */
	if( OTA_Pin_state == GPIO_PIN_SET )
	{
		HAL_GPIO_WritePin(BOOTMODE_LED_GPIO_Port, BOOTMODE_LED_Pin, GPIO_PIN_RESET);
		HAL_Delay(500);
		HAL_GPIO_WritePin(BOOTMODE_LED_GPIO_Port, BOOTMODE_LED_Pin, GPIO_PIN_SET);
		//USART1->CR1 |= USART_CR1_RXNEIE;		/* Enable Receive interrupt */
		printf("Starting Firmware Download!!!\r\n");

		do
		{
			memset(data_in, 0, ETX_OTA_PACKET_MAX_SIZE);	//CLEAR THE ARRAY
			index=0;
			do
			{
				while( !( USART1->SR & USART_SR_RXNE ) ) {};	/* Timeout also has to be designed */
				data_in[index++]=USART1->DR;
				if (data_in[0] != ETX_OTA_SOF)
				{
					printf("Not valid Start packet, EXITING!!! \n\r");
					break;
				}
				//check chunk length
				if (index>=6)
				{
					chunksize = (data_in[3]<<8u) + (data_in[2])+7;
					fwsize = (data_in[5]<<8u) + (data_in[4]);
				}
				TotalCharsReceived =index;
			}
			while (data_in[chunksize-1] != ETX_OTA_EOF);	//this byte has to be received, else it will be stuck here

			/*
			 * This is required,
			 * But for now let us assume that if EOF is not matching then if will get stuck in above while loop
			 * verify EOF. */

			//ota_fw_received_size=chunksize-7 /*  1B-SOF + 1B-TYPE + 2B-CHUNKSIZE + 2B-FWSIZE + .... + 1B-EOF = 7B  */;
			//printf("[%d/%d]chunksize is %d, firmware size is %d \n\r",counter++,(fwsize/OTA_PACKET_MAX_SIZE),chunksize,fwsize);
			printf("Copying the chunk to flash\n\r");
			/* write the chunk to the Flash (App location) */
			write_data_to_flash_app( &data_in[6], chunksize-7, (ota_fw_received_size == 0));
			if( ex == HAL_OK )
			{
				printf("[%d of %d bytes received]\r\n", ota_fw_received_size, fwsize);

				/*
				 * Sending Acknowledgement to recieve another batch of FW chunk
				 */
				etx_ota_send_resp(ETX_OTA_ACK);
				//			while( !( USART1->SR & USART_SR_TXE ) ) {};
				//			USART2->DR = rxb;
			}
			else
			{
				printf("FLASH ERROR, EXITING!!!\n\r");
				break;
			}
		}
		while( !(ota_fw_received_size >= fwsize) );
		//received the full data. So, move to end
		printf("FLASH UPDATED, Rebooting...\n\r");
		HAL_NVIC_SystemReset();
	}
	else
	{
		printf("Button press karna \n\r");
	}

	HAL_Delay(2000);
	goto_application();
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
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
  HAL_GPIO_WritePin(GPIOA, APPMODE_LED_Pin|BOOTMODE_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_OTA_Pin */
  GPIO_InitStruct.Pin = BTN_OTA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_OTA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : APPMODE_LED_Pin BOOTMODE_LED_Pin */
  GPIO_InitStruct.Pin = APPMODE_LED_Pin|BOOTMODE_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Set_BOOTMODE_LED(void)
{
	HAL_GPIO_WritePin(BOOTMODE_LED_GPIO_Port, BOOTMODE_LED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(APPMODE_LED_GPIO_Port, APPMODE_LED_Pin, GPIO_PIN_RESET);
}

void Set_APPMODE_LED(void)
{
	HAL_GPIO_WritePin(BOOTMODE_LED_GPIO_Port, BOOTMODE_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(APPMODE_LED_GPIO_Port, APPMODE_LED_Pin, GPIO_PIN_SET);
}

static void etx_ota_send_resp( uint8_t type )
{
  ETX_OTA_RESP_ rsp =
  {
    .sof         = ETX_OTA_SOF,
    .packet_type = ETX_OTA_PACKET_TYPE_RESPONSE,
    .data_len    = 1u,
    .status      = type,
    .crc         = 0u,                //TODO: Add CRC
    .eof         = ETX_OTA_EOF
  };

  //send response
  HAL_UART_Transmit(&huart1, (uint8_t *)&rsp, sizeof(ETX_OTA_RESP_), HAL_MAX_DELAY);
}

/**
 * @brief Write data to the Application's actual flash location.
 * @param data data to be written
 * @param data_len data length
 * @is_first_block true - if this is first block, false - not first block
 * @retval HAL_StatusTypeDef
 */
static int write_data_to_flash_app( uint8_t *data,
		uint16_t data_len, bool is_first_block)
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
			printf("Erasing the Flash memory...\r\n");
			//Erase the Flash
			FLASH_EraseInitTypeDef EraseInitStruct;
			uint32_t SectorError;

			EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
			EraseInitStruct.Sector        = FLASH_SECTOR_5;
			EraseInitStruct.NbSectors     = 2;                    //erase 2 sectors(5,6)
			EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

			ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
			if( ret != HAL_OK )
			{
				break;
			}
		}
		printf("FLASH write at address %x \n\r", (ETX_APP_FLASH_ADDR + ota_fw_received_size));
		for(int i = 0; i < data_len; i++ )
		{
			ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
					(ETX_APP_FLASH_ADDR + ota_fw_received_size),
					data[i]
			);
			if( ret == HAL_OK )
			{
				//update the data count
				ota_fw_received_size += 1;

			}
			else
			{
				printf("Flash Write Error\r\n");
				break;
			}
		}
		//printf("FLASH written at address %x \n\r", (ETX_APP_FLASH_ADDR + ota_fw_received_size));
		//printf("%d bytes written in flash \n\r", ota_fw_received_size);
		ret = HAL_FLASH_Lock();
		if( ret != HAL_OK )
		{
			break;
		}
	}while( false );

	return ret;
}

static void goto_application(void)
{
	printf("Will jump to application\n\r");
	HAL_UART_DeInit(&huart1);	//DEINITIALIZE THE UART
	void (*app_reset_handler) (void) = (void*) ( *(volatile uint32_t *) (0x08020000 + 4));
	void Set_APPMODE_LED();
	app_reset_handler();		//call the application reset handler
}

/**
 * @brief Print the characters to USB OR SERIAL WIRE VIEWER TRACE (printf).
 * @retval int
 */

#ifdef ENABLE_TRACE
/* Override low-level _write system call */
int _write(int file, char *ptr, int len)
{
	int DataIdx;
	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		ITM_SendChar(*ptr++);
	}
	return len;
}
#else

int _write(int file, char *ptr, int len)
{
	static uint8_t rc = USBD_OK;

	do
	{
		rc = CDC_Transmit_FS(ptr, len);
	} while (USBD_BUSY == rc);

	if (USBD_FAIL == rc)
	{
		/// NOTE: Should never reach here.
		/// TODO: Handle this error.
		return 0;
	}
	return len;
}
#endif

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
