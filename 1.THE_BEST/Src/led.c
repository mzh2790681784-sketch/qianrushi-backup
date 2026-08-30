#include "led.h"

void led_control(u8 led_ctrl)
{
	//熄灭所有led
	HAL_GPIO_WritePin(GPIOC,0xff<<8,GPIO_PIN_SET);
	//根据led_ctrl点亮相应led
	HAL_GPIO_WritePin(GPIOC,led_ctrl<<8,GPIO_PIN_RESET);
	//控制锁存器
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}
