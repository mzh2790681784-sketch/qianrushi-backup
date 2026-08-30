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
#include <stdio.h>
#include <string.h>


#include "adc.h"
#include "tim.h"
#include "usart.h"
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
u8 display_buf[20];

float temp=0;
u8 mode=1;
u16 speed=200;
u8 th=30;
u8 jiemian=0;
u8 shanshuo=0;
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
	HAL_GPIO_WritePin(GPIOC,0XFF<<8,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC,led<<8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}

__IO uint32_t led_tick=0;
u8 led=0;
void led_process()
{
	if(uwTick-led_tick<=100)	return;
	led_tick=uwTick;
	led=0;
	if(jiemian==0)
		led|=0x01;
	else if(jiemian==1)
		led|=0x02;

	if(mode==0)
		led|=0x04;

	if(speed>800)
	{
		if(shanshuo==0)
		{
			led|=0x80;
			shanshuo=1;
		}
		else if(shanshuo==1)
			shanshuo=0;
	}
	led_ctrl(led);
}

#define kb1 HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)
#define kb2 HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)
#define kb3 HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)
#define kb4 HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)
#define key_data	((kb4<<3)|(kb3<<2)|(kb2<<1)|kb1|0xf0)
u8 trg=0,cont=0;
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
		if(jiemian==0)
		{
		mode++;
		if(mode==2)
			mode=0;
		}
	}
	if(trg&0x02)
	{
		if(jiemian==0&&mode==1)
		{
			if(speed<=900)
			speed+=100;
		}
		if(jiemian==1)
		{
			if(th<60)
				th+=5;
		}
	}
	if(trg&0x04)
	{
		if(jiemian==0&&mode==1)
		{
			if(speed>=200)
				speed-=100;
		}
		if(jiemian==1)
		{
			if(th>30)
				th-=5;
		}
	}
	if(trg&0x08)
	{
		jiemian++;
		if(jiemian==2)
			jiemian=0;
	}
	
}

u16 adc2_data;
float r37v;
__IO uint32_t adc_tick=0;
void adc_process()
{
	if(uwTick-adc_tick<=20)		return;
	adc_tick=uwTick;
	
	HAL_ADC_Start(&hadc2);
	adc2_data=HAL_ADC_GetValue(&hadc2);
	r37v=adc2_data/4095.0f*3.3f;
	
	if(r37v>=3)
		temp=80;
	else if(r37v<=0.3)
		temp=0;
	else
		temp=80*(r37v-0.3)/2.7;
	
	if(mode==0)
	{
		if(temp>th)
		{
			speed=(temp-th)*20;
			if(speed<100)
				speed=100;
		}	
	}
	TIM17->CCR1 = speed; 
}

int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(&huart1,(u8*)&ch,1,50);
	return ch;
}
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
}
u8	uart_buf[1];
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(uart_buf[0]=='M')
	{
		if(mode==0)
			printf("Mode:AUTO");
		else if(mode==1)
			printf("Mode:MANU");
	}
	else if(uart_buf[0]=='T')
		printf("Temp:%.1f",temp);
	else if(uart_buf[0]=='S')
		printf("Speed:%drpm",speed);
	else
		printf("ERROR");
	HAL_UART_Receive_IT(&huart1,uart_buf,1);
}

void lcd_gongzuo()
{
	LCD_DisplayStringLine(Line1, (u8*)"        DATA       ");
	
	if(mode==0)
		LCD_DisplayStringLine(Line3, (u8*)"      MODE:AUTO    ");
	else if(mode==1)
		LCD_DisplayStringLine(Line3, (u8*)"      MODE:MANU    ");
	
	sprintf((char*)display_buf,"      TEMP:%.1f      ",temp);
	LCD_DisplayStringLine(Line4,display_buf);
	
	sprintf((char*)display_buf,"      SPEED:%d      ",speed);
	LCD_DisplayStringLine(Line5,display_buf);
}

void lcd_shezhi()
{
	LCD_DisplayStringLine(Line1, (u8*)"        SET       ");
	
	sprintf((char*)display_buf,"      TH:%d      ",th);
	LCD_DisplayStringLine(Line3,display_buf);
	LCD_DisplayStringLine(Line4, (u8*)"                       ");
	LCD_DisplayStringLine(Line5, (u8*)"                       ");
}

__IO uint32_t lcd_tick=0;
void lcd_process()
{
	if(uwTick-lcd_tick<=100)	return;
	lcd_tick=uwTick;
	if(jiemian==0)
		lcd_gongzuo();
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
	
	MX_TIM17_Init();
	HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);		//cubemx不会生成，要自己记住
	TIM17->ARR = 999;	// 周期是1000us，对应频率1000Hz
	
	MX_USART1_UART_Init();
	HAL_UART_Receive_IT(&huart1,uart_buf,1);		//cubemx不会生成，要自己记住
    
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
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);

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
    /*Configure GPIO pins : PB5 PB8 PB9 */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
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
