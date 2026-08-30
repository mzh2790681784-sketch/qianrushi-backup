#include "key.h"

#define KB1  HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)
#define KB2  HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)
#define KB3  HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)
#define KB4  HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)
#define KEYPORT  KB1 | (KB2<<1) | (KB3<<2) | (KB4<<3) | 0xf0

u8 trg;	 	 // 全局变量，单次触发
u8 cont; 	 // 全局变量，长按
void key_read(void)
{
    u8 readdata = (KEYPORT)^0xff;   		   // 1
    trg=readdata&(readdata^cont);        // 2 
    cont=readdata;                           // 3
}
