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
#include "led.h"
#include "key.h"
#include "adc.h"
#include "i2c_hal.h"
#include "dac.h"
#include "rtc.h"
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

// LED执行程序
__IO uint32_t led_Tick=0;
u8 led_ctrl=0xaa;
void led_process(void)
{

	if(uwTick-led_Tick<100)	return;
	led_Tick=uwTick;
	
	led_control(led_ctrl);
}


// 按键执行程序
__IO uint32_t keyTick = 0;
void key_process(void)
{
	if(uwTick - keyTick < 10) return ; 
	keyTick = uwTick; 
	
	key_read();
	if(trg&0x01)	//B1
	{

	}
	if(trg&0x02)	//B2
	{

	}
	if(trg&0x04)	//B3
	{
 
	}
	if(trg&0x08)	//B4
	{

	}
}


// ADC执行程序
u16 adc1_val,adc2_val;
float volt_r37,volt_r38,volt_mcp;
void adc_process(void)
{
	//RANK1 - CH5
	HAL_ADC_Start(&hadc1);
	volt_mcp = HAL_ADC_GetValue(&hadc1)/4096.0f*3.3f;
	//RANK2 - CH11
	HAL_ADC_Start(&hadc1);
	adc1_val = HAL_ADC_GetValue(&hadc1);
	volt_r38 = adc1_val/4096.0f*3.3f;
	
	//ADC2的采集
    HAL_ADC_Start(&hadc2);
	adc2_val = HAL_ADC_GetValue(&hadc2);
	volt_r37 = adc2_val/4096.0f*3.3f;
}


//DAC
u16 dac_ch1_val,dac_ch2_val;
void dac_process()
{  
	dac_ch1_val = (1.1f/3.3f*4095);
	dac_ch2_val = (2.5f/3.3f*4095);
	
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_ch1_val);	//0-->0v  4095--> 3.3V    1.1v --> 1365
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	  
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_ch2_val);	//0-->0v  4095--> 3.3V    2.2v --> 2730
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
}


//RTC
RTC_TimeTypeDef rtc_time;
RTC_DateTypeDef rtc_date;
void rtc_process()
{
	HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
}


// EEPROM
u8 eeprom_add=0x20,eeprom_data=0;


//MCP4017
u8 mcp4017_set=0x7f,mcp4017_data=0;


//串口发送
u8 tx_buf[]={"您好!\r\n"};


//串口接收
u8 uart_buf[2];
u8 rx_buf[10];
u8 rx_cnt = 0;
__IO uint32_t uartTick = 0;
void RxIdle_process()
{
	if(uwTick - uartTick < 50) return ; 
	uartTick = uwTick; 
	//50ms执行一次
	rx_cnt = 0;
	memset(rx_buf,'\0',sizeof(rx_buf));		//清空接收缓存数组
}


//PWM捕获

//u32 tim2_cnt1=0;
//u32 f40=0;
//void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{
//	tim2_cnt1 = __HAL_TIM_GetCounter(&htim2);//获取CNT，对应CNT(us)
//	__HAL_TIM_SetCounter(&htim2,0);			// 设置CNT为0，重新开始计时
//	
//	f40 = 1000000/tim2_cnt1;				//R40的调整的555频率
//	
//	HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);//开启TIM2_CH1的输入捕获中断
//}


u32 tim2_cnt1 = 0, tim2_cnt2 = 0;
u32 f40 = 0;
float d40 = 0;

u32 tim3_cnt1 = 0, tim3_cnt2 = 0;
u32 f39 = 0;
float d39= 0;

u8 tim2_state = 0;	//0:开始计时，1：获取T1，2：获取T2
u8 tim3_state = 0;	//0:开始计时，1：获取T1，2：获取T2
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim2)
	{
		if(tim2_state == 0)							//第一个上升沿产生，开始计时
		{
			__HAL_TIM_SetCounter(&htim2,0);			// 设置CNT为0，重新开始计时
			TIM2->CCER |= 0x02;						//下降沿中断，CC1P置为1
			tim2_state = 1;							
		}
		else if(tim2_state == 1)					//获取T1，并改成上升沿中断
		{
			tim2_cnt1 = __HAL_TIM_GetCounter(&htim2);//获取T1(us)
			TIM2->CCER &= ~0x02;					//上升沿中断，CC1P置为0
			tim2_state = 2;
		}
		else if(tim2_state == 2)					//第二个上升沿中断，获取T2（周期）
		{
			tim2_cnt2 = __HAL_TIM_GetCounter(&htim2);//获取T2(us)
			f40 = 1000000/tim2_cnt2;				//R40的调整的555频率	
			d40 = tim2_cnt1*100.0f/tim2_cnt2;
			tim2_state = 0;
		}
		HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);//开启TIM2_CH1的输入捕获中断
	}
	
	if(htim == &htim3)
	{
		if(tim3_state == 0)							//第一个上升沿产生，开始计时
		{
			__HAL_TIM_SetCounter(&htim3,0);			// 设置CNT为0，重新开始计时
			TIM3->CCER |= 0x02;						//下降沿中断，CC1P置为1
			tim3_state = 1;							
		}
		else if(tim3_state == 1)					//获取T1，并改成上升沿中断
		{
			tim3_cnt1 = __HAL_TIM_GetCounter(&htim3);//获取T1(us)
			TIM3->CCER &= ~0x02;					//上升沿中断，CC1P置为0
			tim3_state = 2;
		}
		else if(tim3_state == 2)					//第二个上升沿中断，获取T2（周期）
		{
			tim3_cnt2 = __HAL_TIM_GetCounter(&htim3);//获取T2(us)
			f39 = 1000000/tim3_cnt2;				//R39的调整的555频率	
			d39 = tim3_cnt1*100.0f/tim3_cnt2;
			tim3_state = 0;
		}
		HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);//开启TIM3_CH1的输入捕获中断
	}
}


// LCD执行程序
void LCD_Process(void)
{
	u8 display_buf[20];
//	
//	//【问题】长数据对短数据覆盖问题
//	//--> 解决方案：加空格,针对字符串
////	LCD_DisplayStringLine(Line2,"hello");
//	LCD_DisplayStringLine(Line2,"hi   ");
//	//--> 解决方案：格式化输出，针对数据
////	sprintf((char*) display_buf,"%5d",4000);			//显示5位，默认右对齐
////	LCD_DisplayStringLine(Line3,display_buf);
//	sprintf((char*) display_buf,"%5d",10);
//	LCD_DisplayStringLine(Line3,display_buf);
//	
//	//格式化输出例子
//	sprintf((char*) display_buf,"%-5d",10);				//左对齐
//	LCD_DisplayStringLine(Line4,display_buf);
//	
//	sprintf((char*) display_buf,"%05d",500);			//前面补0
//	LCD_DisplayStringLine(Line5,display_buf);
//	
//	sprintf((char*) display_buf,"%5.3f",3.1415926);		//显示小数，总长是5位数（小数点算1位），小数点后是2位
//	LCD_DisplayStringLine(Line6,display_buf);
//	
//	sprintf((char*) display_buf,"%x",15);				//%x显示16进制,%o显示8进制
//	LCD_DisplayStringLine(Line7,display_buf);
//	
//	sprintf((char*) display_buf,"%c",'a');				//%s字符串，%c字符
//	LCD_DisplayStringLine(Line8,display_buf);
//	
//	sprintf((char*) display_buf,"%d %% ",10);			//输出百分号：%
//	LCD_DisplayStringLine(Line9,display_buf);	
	LCD_SetTextColor(Red);
	sprintf((char*)display_buf," THE BEST TEMPLATE! ");
	LCD_DisplayStringLine(Line0,display_buf);
	
	sprintf((char*)display_buf,"           --BY MZH ");
	LCD_DisplayStringLine(Line1,display_buf);
	
	
	LCD_SetTextColor(Black);
	sprintf((char*)display_buf,"MCP:%5.2f(%02x)",volt_mcp,mcp4017_data);
	LCD_DisplayStringLine(Line2,display_buf);
	
	sprintf((char*)display_buf,"R37:%5.2f R38:%5.2f",volt_r37,volt_r38);
	LCD_DisplayStringLine(Line3,display_buf);
	
	sprintf((char*)display_buf,"Time: %02d-%02d-%02d",rtc_time.Hours,rtc_time.Minutes,rtc_time.Seconds);
	LCD_DisplayStringLine(Line4,display_buf);
	
	sprintf((char*)display_buf,"T3(R39):%05d, %4.1f",f39, d39);
	LCD_DisplayStringLine(Line5,display_buf);
	
	sprintf((char*)display_buf,"T2(R40):%05d, %4.1f",f40, d40);
	LCD_DisplayStringLine(Line6,display_buf);
	
	sprintf((char*)display_buf,"24c02(%02x):%02x(%03d)",eeprom_add,eeprom_data,eeprom_data);
	LCD_DisplayStringLine(Line7,display_buf);
	
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
	MX_ADC1_Init();
	MX_ADC2_Init();
	MX_DAC1_Init();
	MX_RTC_Init();
	MX_USART1_UART_Init();
	HAL_UART_Receive_IT(&huart1,uart_buf,1);	//开启串口接收中断
	MX_TIM2_Init();
	HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);//开启TIM2_CH1的输入捕获中断
	MX_TIM3_Init();
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);//开启TIM3_CH1的输入捕获中断
	
	MX_TIM17_Init();
	HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);
	TIM17->ARR = 499;	// 周期是500us，对应频率2kHz
	TIM17->CCR1 = 400; // 80%占空比
    
	MX_TIM16_Init();
	HAL_TIM_PWM_Start(&htim16,TIM_CHANNEL_1);
	TIM16->ARR = 49;	// 周期是50us，对应频率20kHz
	TIM16->CCR1 = 10; // 20%占空比
	/* USER CODE BEGIN 2 */
    LCD_Init();
	HAL_Delay(100);
	
	I2CInit();
	eeprom_write(eeprom_add,0x55);
	eeprom_data= eeprom_read(eeprom_add);
	
	
	mcp4017_write(mcp4017_set);
	mcp4017_data = mcp4017_read();
	/* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
	LCD_Clear(White);
	
    LCD_SetBackColor(White);
    LCD_SetTextColor(Blue);
	
	//串口发送
//	HAL_UART_Transmit(&huart1,(unsigned char *)"Hello World!\r\n",sizeof("Hello World!\r\n")-1,50);
//	HAL_UART_Transmit(&huart1,(unsigned char *)"Hello World!\r\n",sizeof("Hello World!\r\n")-1,50);
//	HAL_UART_Transmit(&huart1,(unsigned char *)tx_buf,sizeof(tx_buf)-1,50);
	
	printf("Hello World!\r\n");
	printf("N: %d \r\n",123);
	printf("N: %f \r\n",3.1415);
	
	
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
		led_process();
		key_process();
		adc_process();
		LCD_Process();
		dac_process();
		rtc_process();
		RxIdle_process();
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

//串口接收回调函数

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//	led_ctrl=uart_buf[0];
//	HAL_UART_Receive_IT(&huart1,uart_buf,1);	//开启下一次串口接收
//}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uartTick = uwTick; 		//重新开始计时50ms
	//接收3个字节长度数据，rx_buf[0]写入EEPROM，rx_buf[1]控制LED，...
	rx_buf[rx_cnt++] = uart_buf[0];	
	
	if(rx_cnt==3)				//定长
	{
		rx_cnt = 0;
		led_ctrl=rx_buf[1];
	}
	
//	if(uart_buf[0] == '\n')		//不定长，固定帧尾接收到换行符 \r \n
//	{
//		rx_cnt = 0;
//		led_ctrl=rx_buf[1];
//	}
	HAL_UART_Receive_IT(&huart1,uart_buf,1);	//开启下一次串口接收
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
