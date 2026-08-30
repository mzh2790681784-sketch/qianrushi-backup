/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
float volt;
u8 state=1;
float min=6.4,max=8.4;
float min_set=6.4,max_set=8.4;
u8 jingbao=0;
u8 jiemian=0;
u8 xuanze=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void led_ctrl(u8 led)
{
	HAL_GPIO_WritePin(GPIOC,0xff<<8,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC,led<<8,GPIO_PIN_RESET);
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}

__IO uint32_t led_tick=0;
u8 led=0;
u8 shanshuo1=0,shanshuo2=0;
void led_process()
{
	if(uwTick-led_tick<=100)	return;
	led_tick=uwTick;
	
	led=0;
	
	if(state==0)
		led|=0x01;
	if(jingbao==1)
	{
		if(shanshuo1==0)
		{
			led|=0x02;
			shanshuo1=1;
		}
		else
		{
			shanshuo1=0;
		}
	}
	if(volt>8.4f)
	{
		if(shanshuo2==0)
		{
			led|=0x80;
			shanshuo2=1;
		}
		else
		{
			shanshuo2=0;
		}
	}
	led_ctrl(led);
}

#define	kb1	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)
#define	kb2	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)
#define	kb3	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)
#define	kb4	HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)

#define key_data ((kb4<<3)|(kb3<<2)|(kb2<<1)|kb1|0xf0)
u8 trg,cont;
void key_scan()
{
	u8 date=key_data^0xff;
	trg=date&(date^cont);
	cont=date;
}
__IO uint32_t key_tick=0;
void key_process()
{
	if(uwTick-key_tick<=10)	return;
	key_tick=uwTick;
	key_scan();
	
	if(trg&0x01)
	{
		jiemian++;
		if(jiemian==2)
		{
			jiemian=0;
			xuanze=0;
			min_set=min;
			max_set=max;
		}
	}
	if(trg&0x02)
	{
		if(jiemian==1)
		{
			xuanze++;
			if(xuanze==2)
				xuanze=0;
		}
	}
	if(trg&0x04)
	{
		if(jiemian==1)
		{
			if(xuanze==0)
			{
				if(min<(max-0.1f))
					min+=0.1f;
			}
			else if(xuanze==1)
			{
				if(max<8.4f)
					max+=0.1f;
			}
		}
	}
	if(trg&0x08)
	{
		if(jiemian==0)
		{
			state++;
			if(state==2)
				state=0;
		}
		else if(jiemian==1)
		{
			if(xuanze==0)
			{
				if(min>3.7f)
					min-=0.1f;
			}
			else if(xuanze==1)
			{
				if((max-0.1f)>min)
					max-=0.1f;
			}
		}
	}
}

__IO uint32_t adc_tick=0;
u16 adc_count;
float r37v;
void adc_process()
{
	if(uwTick-adc_tick<=50)	return;
	adc_tick=uwTick;
	
	HAL_ADC_Start(&hadc2);
	adc_count=HAL_ADC_GetValue(&hadc2);
	r37v=adc_count/4095.0f*3.3f;
	
	volt=r37v*3;
	if(volt<min_set||volt>max_set)
		jingbao=1;
	else
		jingbao=0;
}
int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(&huart1,(u8 *)&ch,1,50);
	return ch;
}
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}
u8 uart_buff[1];
u8 rx_buff[20];
u8 rx_count=0;
__IO uint32_t uart_tick=0;

void rx_release()
{
	if(uwTick-uart_tick<=50)	return;
	uart_tick=uwTick;
	
	if(rx_count!=0)
	{
		if(strcmp((char*)rx_buff,"Check_VOLT")==0)
			printf("BATTERY:%.1fV",volt);
		else if(strcmp((char*)rx_buff,"Check_START")==0)
		{
			if(state==0)
				printf("START:CHARGE");	
			else if(state==1)
				printf("START:IDLE");
		}
		else
			printf("error");
	}
	rx_count=0;
	memset(rx_buff,'\0',sizeof(rx_buff));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_tick=uwTick;
	rx_buff[rx_count++]=uart_buff[0];
	
	
	HAL_UART_Receive_IT(&huart1,uart_buff,1);
}
u8 dis_buff[20];
void lcd_shuju()
{
	LCD_DisplayStringLine(Line1, (unsigned char *)"      BATTERY      ");
	
	sprintf((char*)dis_buff,"     VOLT:%.1fV     ",volt);
	LCD_DisplayStringLine(Line3,dis_buff);

	if(state==0)
		LCD_DisplayStringLine(Line4, (unsigned char *)"    STATE:CHARGE      ");
	else if(state==1)
		LCD_DisplayStringLine(Line4, (unsigned char *)"    STATE:IDLE      ");
}
void lcd_shezhi()
{
	LCD_DisplayStringLine(Line1, (unsigned char *)"        SET       ");
	
	sprintf((char*)dis_buff,"      MIN:%.1fV     ",min);
	LCD_DisplayStringLine(Line3,dis_buff);
	
	sprintf((char*)dis_buff,"      MAX:%.1fV     ",max);
	LCD_DisplayStringLine(Line4,dis_buff);

	
}
__IO uint32_t lcd_tick=0;
void lcd_process()
{
	if(uwTick-lcd_tick<=100)	return;
	lcd_tick=uwTick;
	
	if(jiemian==0)
		lcd_shuju();
	else if(jiemian==1)
		lcd_shezhi();
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
	
    /* USER CODE BEGIN 2 */
	
	MX_ADC2_Init();
	MX_USART1_UART_Init();
	HAL_UART_Receive_IT(&huart1,uart_buff,1);
	
    LCD_Init();
	HAL_Delay(100);;
	/* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);
	LCD_Clear(Black);
	HAL_Delay(100);

    while (1)
    {
		led_process();
		key_process();
		adc_process();
		rx_release();
		lcd_process();
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
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0
                      | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4
                      | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8
                      | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
  
    /*Configure GPIO pins : PC13 PC14 PC15 PC0
                             PC1 PC2 PC3 PC4
                             PC5 PC6 PC7 PC8
                             PC9 PC10 PC11 PC12 */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0
                          | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4
                          | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8
                          | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : PA8 */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PB5 PB8 PB9 */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PA0 */
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PB0 PB1 PB2 */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PD2 */
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
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
    /* User can add his own implementation to report the HAL error return state */

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
       tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
