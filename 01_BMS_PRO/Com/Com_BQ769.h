#ifndef __COM_BQ769__
#define __COM_BQ769__

#include "main.h"

/**
 * @brief 电压转换为温度
 *
 * @param uiADCV 热敏电阻阻值
 * @return int8_t 温度,单位是℃
 */
int8_t Com_BQ769_GetTemperatureByRes(int res);

/**
 * @brief 锂电池开路电压(OCV)与荷电状态(SOC)对应查找表
 * @note 电压单位：mV，SOC范围：0%~100%，每1%一个采样点
 *       数组索引直接对应SOC百分比值
 *       例如：索引0对应0% SOC，索引50对应50% SOC，索引100对应100% SOC
 *
 * 典型电压点：
 * 0% SOC:   3282mV    50% SOC:  3679mV    100% SOC: 4185mV
 * 10% SOC:  3477mV    60% SOC:  3748mV
 * 20% SOC:  3561mV    80% SOC:  3940mV
 */
uint8_t Com_BQ769_GetSocByVol(uint16_t vol);

#endif // __COM_BQ769__
