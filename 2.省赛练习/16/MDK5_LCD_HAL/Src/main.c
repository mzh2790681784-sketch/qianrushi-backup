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
#include <stdio.h>
#include <string.h>
#include "adc.h"
#include "usart.h"
#include "tim.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint16_t CF=0,CF1=0;
uint16_t XF=0;
uint8_t CD=0,CD1=0;
uint8_t ST=0;

u8 jiemian=0,canshu=0;

u8 DS_SET=1,DR_SET=80;
u8 DS=1,DR=80;
u16 FS_SET=100,FR_SET=2000;
u16 FS=100,FR=2000;

u8 sec=0,min=0,hour=0;
u8 sec1=0,min1=0,hour1=0;

u8 yichang=0;

u8 n1=0,n2=0;
float pinlv,zhankongbi;
void lcd_process(void);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

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
	if(uwTick-led_tick<=200)	return;
	led_tick=uwTick;
	
	if(jiemian==0)
		led|=0x01;
	else
		led&=0xfe;
	
	if(ST==1)
		led|=0x02;
	else
		led&=0xfd;
	
	led_ctrl(led);
}

#define	kb1	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)
#define	kb2	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)
#define	kb3	HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)
#define	kb4	HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)
#define date	(kb4<<3) | (kb3<<2) | (kb2<<1) | kb1 | 0xf0
u8 trg,cont;
void key_scan()
{
	u8 read_dat=(date)^0xff;
	trg=read_dat&(read_dat^cont);
	cont=read_dat;
}

__IO uint32_t key_tick=0;
u32 key_2s=0;
u32 ceshi=0;
void key_process()
{
	if(uwTick-key_tick<=10)		return;
	key_tick=uwTick;
	ceshi++;
	key_scan();
	if(trg&0x01)	//k1
	{
		jiemian++;
		if(jiemian==3)
		{
			jiemian=0;
			canshu=0;
			if(10+DS<=DR&&1000+FS<=FR&&DR<=100&&DR>10&&FR>1000)
			{
				DS_SET=DS;
				DR_SET=DR;
				FS_SET=FS;
				FR_SET=FR;
			}
		}
	}
	if(trg&0x02)	//k2
	{
		if(jiemian==2)
		{
			canshu++;
			if(canshu==4)
				canshu=0;
		}
	}
	if(trg&0x04)	//k3
	{
		if(jiemian==2)
		{
			if(canshu==0)
			{
				if(DS<100)
					DS+=1;
			}
			else if(canshu==1)
			{
				if(DR<100)
					DR+=10;
			}
			else if(canshu==2)
				FS+=100;
			else if(canshu==3)
				FR+=1000;
		}
	}
	if(trg&0x08)	//k4
	{
		if(jiemian==2)
		{
			if(canshu==0)
			{
				if(DS>0)
					DS-=1;
			}
			else if(canshu==1)
			{	
				if(DR>10)
					DR-=10;
			}
			else if(canshu==2)
			{
				if(FS>=100)
					FS-=100;
			}
			else if(canshu==3)
			{
				if(FR>=1000)
					FR-=1000;
			}
		}
	}
	if(cont&0x02)	//长按key2检测
	{
		if(jiemian==0&&key_2s==0)
			key_2s=uwTick;
	}
	if(trg==0x00&&cont==0x00)
	{
		if((uwTick-key_2s)>=2000&&key_2s!=0)			//2s
		{
			uwTick=0;
			key_2s=0;
		}
		else if((uwTick-key_2s)<2000&&key_2s!=0)
		{
			ST=!ST;
			key_2s=0;
		}
	}
}

u16 adc1_data,adc2_data;
float r38v,r37v;
__IO uint32_t adc_tick=0;
void adc_process()
{
	if(uwTick-adc_tick<=20)	return;
	adc_tick=uwTick;
	
	HAL_ADC_Start(&hadc1);
	adc1_data=HAL_ADC_GetValue(&hadc1);
	r38v=adc1_data/4095.0f*3.3f;
	
	HAL_ADC_Start(&hadc2);
	adc2_data=HAL_ADC_GetValue(&hadc2);
	r37v=adc2_data/4095.0f*3.3f;
	
	if(ST==0)
	{
		n1=(DR_SET-10)/DS_SET;
		zhankongbi=r37v*(DR_SET-10)*DS_SET/3.3/DS_SET+10;
		CD=(uint8_t)zhankongbi;
		
		n2=(FR_SET-1000)/FS_SET;
		pinlv=r38v*(FR_SET-1000)*FS_SET/FS_SET/3.3+1000;
		CF=(uint16_t)pinlv;
		
		TIM17->ARR = 1000000/CF;	
		TIM17->CCR1 = CD*10000/CF; 
	}
	
//	printf("r37:%.1f,r38:%.1f\r\n",r37v,r38v);
}

//int fputc(int ch,FILE *f)
//{
//	HAL_UART_Transmit(&huart1,(u8*)&ch,1,50);
//	return ch;
//}

//void USART1_IRQHandler(void)
//{
//  /* USER CODE BEGIN USART1_IRQn 0 */

//  /* USER CODE END USART1_IRQn 0 */
//  HAL_UART_IRQHandler(&huart1);
//  /* USER CODE BEGIN USART1_IRQn 1 */

//  /* USER CODE END USART1_IRQn 1 */
//}

//u8 uart_buf[1];
//u8 rx_buf[100];
//u8 rx_cnt = 0;
//__IO uint32_t uartTick = 0;
//void RxIdle_process()
//{
//	if(uwTick - uartTick < 50) return ; 
//	uartTick = uwTick; 
//	
//	rx_cnt = 0;
//	memset(rx_buf,'\0',sizeof(rx_buf));
//}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)		//可在hal_uart.h的1552行找到
//{
//	uartTick = uwTick;
//	rx_buf[rx_cnt++] = uart_buf[0];	
//	
////	if(rx_cnt==3)
////	{
////		rx_cnt = 0;
////		led_ctrl(rx_buf[0]);
////	}
////	
//	if(uart_buf[0] == '\n')
//	{
//		
//		HAL_UART_Transmit(&huart1,(u8 *)rx_buf,rx_cnt-1, 50);	//以16进制形式将收到的数据再发回来
//		printf("%s",rx_buf);									//以字符串形式将收到的数据再发回来
//		if(rx_buf[0]>='0'&& rx_buf[0]<='9')			// '0'-'9' → 0-9
//            led_ctrl(rx_buf[0]- '0');
//        else if(rx_buf[0]>='a'&&rx_buf[0]<='f')		// 'a'-'f' → 10-15 (0x0a-0x0f)
//            led_ctrl(rx_buf[0]-'a'+10);  
//        else if(rx_buf[0]>='A'&&rx_buf[0]<='F')		// 'A'-'F' → 10-15 (0x0a-0x0f)
//            led_ctrl(rx_buf[0]-'A'+10); 
//		
//		rx_cnt = 0;
//	}
//	
//	HAL_UART_Receive_IT(&huart1,uart_buf,1);
//}

void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */

  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

//void TIM3_IRQHandler(void)
//{
//  /* USER CODE BEGIN TIM3_IRQn 0 */

//  /* USER CODE END TIM3_IRQn 0 */
//  HAL_TIM_IRQHandler(&htim3);
//  /* USER CODE BEGIN TIM3_IRQn 1 */

//  /* USER CODE END TIM3_IRQn 1 */
//}

u32 tim2_cnt1=0;
u32 DF=0,DF1=0;				//PA15
//u32 tim3_cnt1 = 0;
//u32 f39=0;				//PB4
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)	//可在hal_tim.h的2508行找到
{
	//如果仅仅是测量两路频率
	if(htim==&htim2)
	{
		tim2_cnt1 = __HAL_TIM_GetCounter(&htim2);//获取CNT，对应CNT(us)
		__HAL_TIM_SetCounter(&htim2,0);			// 设置CNT为0，重新开始计时
		DF = 1000000/tim2_cnt1;				//R40的调整的555频率
		HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);//开启TIM2_CH1的输入捕获中断
	}
	
	if((DF>CF&&(DF-CF)>=1000)||(CF>DF&&(CF-DF)>=1000))
	{
		if(yichang==0)
		{
			CF1=CF;CD1=CD;DF1=DF;sec1=sec;min1=min;hour1=hour;
			if(DF>CF)
				XF=DF-CF;
			else
				XF=CF-DF;
			yichang=1;
			led|=0x04;
		}
	}
	else
	{
		yichang=0;
		led&=0xfb;
	}
//	if(htim==&htim3)
//	{
//		tim3_cnt1 = __HAL_TIM_GetCounter(&htim3);//获取CNT，对应CNT(us)
//		__HAL_TIM_SetCounter(&htim3,0);			// 设置CNT为0，重新开始计时
//		f39 = 1000000/tim3_cnt1;				//R39的调整的555频率
//		HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);//开启TIM3_CH1的输入捕获中断
//	}
}

__IO uint32_t lcdTick = 0;
u8 display_buf[20];

void lcd_jiankong()
{
	LCD_DisplayStringLine(Line1,"       PWM      ");
	
	sprintf((char*)display_buf,"   CF=%dHz        ",CF);
	LCD_DisplayStringLine(Line3,display_buf);
	
	sprintf((char*)display_buf,"   CD=%d%%      ",CD);
	LCD_DisplayStringLine(Line4,display_buf);
	
	sprintf((char*)display_buf,"   DF=%dHz      ",DF);
	LCD_DisplayStringLine(Line5,display_buf);
	
	if(ST==1)
		LCD_DisplayStringLine(Line6,"   ST=LOCK     ");
	else if(ST==0)
		LCD_DisplayStringLine(Line6,"   ST=UNLOCK      ");
	
	sec=(uwTick/1000)%60;
    min=(uwTick/60000)%60;
    hour=(uwTick/3600000)%24;
	sprintf((char*)display_buf,"   %02dH%02dM%02dS     ",hour,min,sec);
	LCD_DisplayStringLine(Line7,display_buf);
	
}

void lcd_tongji()
{
	LCD_DisplayStringLine(Line1,"       RECD      ");
	
	sprintf((char*)display_buf,"   CF=%dHz      ",CF1);
	LCD_DisplayStringLine(Line3,display_buf);
	
	sprintf((char*)display_buf,"   CD=%d%%        ",CD1);
	LCD_DisplayStringLine(Line4,display_buf);
	
	sprintf((char*)display_buf,"   DF=%d       ",DF1);
	LCD_DisplayStringLine(Line5,display_buf);
	
	sprintf((char*)display_buf,"   XF=%dHz        ",XF);
	LCD_DisplayStringLine(Line6,display_buf);
	
	sec=(uwTick/1000)%60;
    min=(uwTick/60000)%60;
    hour=(uwTick/3600000)%24;
	sprintf((char*)display_buf,"   %02dH%02dM%02dS       ",hour,min,sec);
	LCD_DisplayStringLine(Line7,display_buf);
	
}

void lcd_canshu()
{
	LCD_DisplayStringLine(Line1,"       PARA       ");
	
	sprintf((char*)display_buf,"   DS=%d%%         ",DS);
	LCD_DisplayStringLine(Line3,display_buf);
	
	sprintf((char*)display_buf,"   DR=%d%%         ",DR);
	LCD_DisplayStringLine(Line4,display_buf);
	
	sprintf((char*)display_buf,"   FS=%dHz         ",FS);
	LCD_DisplayStringLine(Line5,display_buf);
	
	sprintf((char*)display_buf,"   FR=%dHz         ",FR);
	LCD_DisplayStringLine(Line6,display_buf);
	
	LCD_DisplayStringLine(Line7,"                           ");
}

void lcd_process()
{
	if(uwTick-lcdTick<=100)		return;
	lcdTick=uwTick;
		
	if(jiemian==0)
		lcd_jiankong();
	else if(jiemian==1)
		lcd_tongji();
	else if(jiemian==2)
		lcd_canshu();
	
//	sprintf((char*)display_buf,"   key=%dms",ceshi/100);
//	LCD_DisplayStringLine(Line8,display_buf);
//	sprintf((char*)display_buf,"   cont2=%dms",key_2s);
//	LCD_DisplayStringLine(Line9,display_buf);
	
	
//	u8 display_buf[20];
//	sprintf((char*)display_buf,"r37:%.1f,r38:%.1f",r37v,r38v);
//	LCD_DisplayStringLine(Line0,display_buf);
//	sprintf((char*)display_buf,"f40(t2_1_PA15):%5d",f40);
//	LCD_DisplayStringLine(Line1,display_buf);
////	sprintf((char*)display_buf,"f39(t3_1_PB4):%5d",f39);
////	LCD_DisplayStringLine(Line2,display_buf);
//	sec=(uwTick/1000)%60;
//    min=(uwTick/60000)%60;
//    hour=(uwTick/3600000)%24;
//	sprintf((char*)display_buf,"   %02dH%02dM%02dS",hour,min,sec);
//	LCD_DisplayStringLine(Line7,display_buf);
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
	MX_ADC1_Init();
	MX_ADC2_Init();
	
//	MX_USART1_UART_Init();
//	HAL_UART_Receive_IT(&huart1,uart_buf,1);		//cubemx不会生成，要自己记住
	
	MX_TIM2_Init();
	HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);		//cubemx不会生成，要自己记住
	
//	MX_TIM3_Init();
//	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);		//cubemx不会生成，要自己记住
	
	MX_TIM17_Init();
	HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);		//cubemx不会生成，要自己记住
			
	
    LCD_Init();
	HAL_Delay(100);
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
		adc_process();
		key_process();
		lcd_process();

//		RxIdle_process();
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
	
	/*Configure GPIO pin : PD2 */
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
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
