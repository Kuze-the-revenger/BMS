#ifndef __INT_BQ769_H__
#define __INT_BQ769_H__
#include "i2c.h"
#include "Com_Delay.h"
#include "Int_BQ769_BSP.h"
#include "Com_Debug.h"
#include "FreeRTOS.h"
#include "task.h"
// 从机地址
#define DEVICE_ADDR 0x10

// 1.BQ769初始化
void Int_BQ769_Init(void);

// 2.向BQ769的某一个寄存器写入单个字节
void Int_BQ769_WriteRegister(uint16_t mem_addr, uint8_t wb);

// 3.读取BQ769多个寄存器的数据
// 读取的寄存器的数据,通过CRC校验才使用.没通过不使用！ 1通过 0没通过
uint8_t Int_BQ769_ReadRegister(uint16_t mem_addr, uint8_t *buffer, uint16_t sizes);


extern RegisterGroup bq_register;

#endif /* __INT_BQ769_H__ */
