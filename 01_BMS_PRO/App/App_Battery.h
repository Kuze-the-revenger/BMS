#ifndef __APP_BATTERY_H__
#define __APP_BATTERY_H__
#include "Int_BQ769.h"
#include "Com_BQ769.h"
// 过压保护的阈值
#define OV_PROTECT 4.25
// 欠压保护阈值
#define UV_PROTECT 2.8

// 1.BQ769初始化方法
void App_Battery_Init(void);

// 2.获取电池组每一节电池的电压(v)
void App_Battery_GetV(void);

// 3.获取电池组总电压
void App_Battery_GetTotalV(void);

// 4.获取电池组充电、放电电流的大小
void App_Battery_GetCurrentA(void);

// 5.获取电池组的温度
void App_battery_GetTemp(void);

// 6.获取电池组电量
void App_Battery_getSoc(void);

// 7.电池组均衡管理
void App_Battery_Balance(void);

// 8.BMS电池组充电开启与关闭方法
void App_Battery_Charge(uint8_t status);

// 9.BMS电池组放电开启与关闭方法
void App_Battery_DisCharge(uint8_t status);
#endif /* __APP_BATTERY_H__ */
