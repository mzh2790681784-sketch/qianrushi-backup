#ifndef __I2C_HAL_H
#define __I2C_HAL_H

#include "main.h"

void I2CStart(void);
void I2CStop(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(void);
void I2CSendNotAck(void);
void I2CSendByte(unsigned char cSendByte);
unsigned char I2CReceiveByte(void);
void I2CInit(void);

void eeprom_write(u8 add,u8 dat);
u8 eeprom_read(u8 add);

void mcp4017_write(u8 val);
u8 mcp4017_read(void);

#endif
