#include "App_Battery.h"
// 保存增益值-mv【0.365-0.396】
float gain = 0;

// 保存偏移量-mv
//-128到127
int8_t offset = 0;

// 定义一个数组,保存电池组每一节电池的电压
#define BATTERY_NUMBER 5
float battery_array[BATTERY_NUMBER];

// 定义变量存储电池组总电压
float battery_totalV = 0.0;

// 定义变量存储电流大小
float battery_A = 0;

// 定义变量存储温度
int8_t battery_temp = 0;

// 定义变量存储剩余电量
uint8_t battery_soc = 0;

// 0.获取BQ7692003PWR芯片增益值与偏移量
static void App_Battery_GetGainAndOffset(void);
// 1.BQ769其他寄存器需要配置
static void App_Battery_OtherConfig(void);

// 1.BQ769初始化
void App_Battery_Init(void)
{
    // 1.BQ769初始化方法
    // 进入SHIP模式,发送I2C命令,Sys_CTRL1低二位设置
    // TS1引脚进行唤醒
    Int_BQ769_Init();

    // 2.获取增益值与偏移量
    App_Battery_GetGainAndOffset();

    // 3.其他寄存器配置
    App_Battery_OtherConfig();
}

// 2.获取电池组每一节电池的电压
void App_Battery_GetV(void)
{
    uint8_t tmp_array[BATTERY_NUMBER * 2] = {0};
    // 添加判断
    if (Int_BQ769_ReadRegister(BQ_VC1_HI, tmp_array, BATTERY_NUMBER * 2) == 0)
    {
        printf("获取电池组的电压CRC校验失败\r\n");
        return;
    }

    // 获取全部ADC寄存器的数据->CRC成功,根据公式计算
    for (uint8_t i = 0; i < BATTERY_NUMBER; i++)
    {
        battery_array[i] = (gain * (tmp_array[2 * i + 1] | tmp_array[2 * i] << 8) + offset) / 1000.0;
        printf("第%d个电池,电池为%fv\r\n", i + 1, battery_array[i]);
    }
}

// 3.获取电池组的总电压
void App_Battery_GetTotalV(void)
{
    uint8_t tmp_array[2] = {0};
    // 读取电池组总电压寄存器ADC数值
    if (Int_BQ769_ReadRegister(BQ_BAT_HI, tmp_array, 2) == 0)
    {
        printf("电池组总电压CRC校验失败\r\n");
        return;
    }

    // 存储电池组总电压
    battery_totalV = (4 * gain * (tmp_array[1] | tmp_array[0] << 8) + (BATTERY_NUMBER * offset)) / 1000.0;

    printf("电池组五节电池总电压:%fv\r\n", battery_totalV);
}

// 4.获取电池组充电、放电电流的大小
void App_Battery_GetCurrentA(void)
{
    uint8_t tmp_array[2] = {2};
    if (Int_BQ769_ReadRegister(BQ_CC_HI, tmp_array, 2) == 0)
    {
        printf("读取电流CRC校验失败\r\n");
        return;
    }

    // 获取ADC数值,不要负数
    int16_t adc_val = tmp_array[1] | (tmp_array[0] << 8);
    if (adc_val < 0)
    {
        adc_val = -adc_val;
    }

    // 计算出电流大小
    battery_A = ((adc_val * 8.44) / 1000.0) / 5;
    printf("电流大小:%f\r\n", battery_A);
}

// 5.获取电池组的温度
void App_battery_GetTemp(void)
{

    // 读取温度寄存器
    uint8_t tmp_array[2] = {0};
    if (Int_BQ769_ReadRegister(BQ_TS1_HI, tmp_array, 2) == 0)
    {
        printf("读取温度寄存器CRC校验失败\r\n");
        return;
    }
    // ts1引脚电压
    float ts1_vtsx = ((tmp_array[1] | tmp_array[0] << 8) * 382) / 1000000.0; // 转换成V
    // 热敏电阻的阻值
    float thermistor_rtx = (10000 * ts1_vtsx) / (3.3 - ts1_vtsx);

    // 获取电池组温度
    battery_temp = Com_BQ769_GetTemperatureByRes((int)thermistor_rtx);
    printf("电池组温度:%d", battery_temp);
}

// 6.获取电池组剩余电量
void App_Battery_getSoc(void)
{
    // 1.思路,电量看电压 4.2->100%   3.3->0%

    // 先计算出电池组的平局电压
    float total = 0; // 电池组总电压
    float avg = 0;   // 电池组平局电压
    for (uint8_t i = 0; i < BATTERY_NUMBER; i++)
    {
        total += battery_array[i];
    }
    // 平局电压
    avg = total / BATTERY_NUMBER;

    battery_soc = Com_BQ769_GetSocByVol((uint16_t)(avg * 1000.0));

    // 判断温度->纠正剩余电量
    if (battery_temp >= 0 && battery_temp <= 40)
    {
        battery_soc = battery_soc * (1 + -0.005 * (battery_temp - 25));
    }

    // 极端情况>40
    if (battery_temp > 40)
    {
        battery_soc = battery_soc * (1 + -0.005 * (40 - 25) + -0.008 * (battery_temp - 40));
    }
    // 极端情况<0
    if (battery_temp < 0)
    {
        battery_soc = battery_soc * (1 + -0.005 * (0 - 25) + -0.003 * (battery_temp - 0));
    }
    // 温度过高,计算出电量被高估,纠正降低
    // 温度过低,计算出电量被低估,纠正升高
    printf("电池剩余电量:%d\r\n", battery_soc);
}

// 均衡管理
void App_Battery_Balance(void)
{
    // 1.计算出电池组中最小的电压
    float min_val = battery_array[0];
    for (uint8_t i = 1; i < BATTERY_NUMBER; i++)
    {
        if (battery_array[i] < min_val)
        {
            min_val = battery_array[i];
        }
    }
    // 1.符合条件均衡管理  2.不符合条件不处理
    // 标志位:表示五节电池谁需要均衡管理,默认都不需要管理
    uint8_t balance_array[5] = {0};
    for (uint8_t i = 0; i < BATTERY_NUMBER; i++)
    {
        // 均衡管理
        if (battery_array[i] - min_val >= 0.05 && battery_array[i] > 3.3 && battery_temp >= 0 && battery_temp <= 40)
        {
            balance_array[i] = 1;
        }
        else
        {
            balance_array[i] = 0;
        }
    }

    // 2.相邻的电池不同同时均衡管理
    for (uint8_t i = 0; i < BATTERY_NUMBER - 1; i++)
    {
        if (balance_array[i] == 1 && balance_array[i + 1] == 1)
        {

            if (battery_array[i] >= battery_array[i + 1])
            {
                balance_array[i + 1] = 0;
            }
            else
            {
                balance_array[i] = 0;
            }
        }
    }

    // 均衡管理
    bq_register.CellBal1.CellBal1Bit.CB1 = balance_array[0];
    bq_register.CellBal1.CellBal1Bit.CB2 = balance_array[1];
    bq_register.CellBal1.CellBal1Bit.CB3 = balance_array[2];
    bq_register.CellBal1.CellBal1Bit.CB4 = balance_array[3];
    bq_register.CellBal1.CellBal1Bit.CB5 = balance_array[4];
    Int_BQ769_WriteRegister(BQ_CELLBAL1, bq_register.CellBal1.CellBal1Byte);

    // 测试代码:查兰节电池做均衡管理了
    uint8_t cell_val = 0;
    if (Int_BQ769_ReadRegister(BQ_CELLBAL1, &cell_val, 1) != 0)
    {
        for (uint8_t i = 0; i < BATTERY_NUMBER; i++)
        {
            printf("第%d节电池,均衡管理%s\r\n", i + 1, ((cell_val >> i) & 0x01) ? "ON" : "OFF");
        }
    }
}

// 8.BMS电池组充电开启与关闭方法
void App_Battery_Charge(uint8_t status)
{
    bq_register.SysCtrl2.SysCtrl2Bit.CHG_ON = status;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL2, bq_register.SysCtrl2.SysCtrl2Byte);
}

// 9.BMS电池组放电开启与关闭方法
void App_Battery_DisCharge(uint8_t status)
{
    bq_register.SysCtrl2.SysCtrl2Bit.DSG_ON = status;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL2, bq_register.SysCtrl2.SysCtrl2Byte);
}

// 获取增益值与偏移量
static void App_Battery_GetGainAndOffset(void)
{
    // 1.计算出增益值-读取ADCGAIN1与ADCGAIN2只读寄存器内部数据,按照公式计算
    uint8_t adc_gain1_val = 0;
    uint8_t adc_gain2_val = 0;
    Int_BQ769_ReadRegister(BQ_ADCGAIN1, &adc_gain1_val, 1);
    Int_BQ769_ReadRegister(BQ_ADCGAIN2, &adc_gain2_val, 1);
    gain = (365 + (adc_gain2_val >> 5) | ((adc_gain1_val & 0x0C) << 1)) / 1000.0;

    // 2.获取偏移量
    Int_BQ769_ReadRegister(BQ_ADCOFFSET, (uint8_t *)&offset, 1);
    COM_DEBUG_LN("gain:%f,offset:%#x", gain, offset);
}

// BQ769其他寄存器配置
static void App_Battery_OtherConfig(void)
{

    // 1.系统控制器 SYS_CTRL1
    // 这位选择位:0,使用BQ769内部温度传感器,测芯片自身温度   1:使用外部的热敏电阻测电池组的温度
    bq_register.SysCtrl1.SysCtrl1Bit.TEMP_SEL = 1;
    // BQ769测点电池组的电压、电流、温度。都需要使用片上外设ADC,使能打开！
    bq_register.SysCtrl1.SysCtrl1Bit.ADC_EN = 1;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL1, bq_register.SysCtrl1.SysCtrl1Byte);

    // 2.系统控制器 SYS_CTRL2
    // 测量库仑计(计数器:电荷数量)使能位,不能测电流
    bq_register.SysCtrl2.SysCtrl2Bit.CC_EN = 1;
    // 充电与放电的使能位
    bq_register.SysCtrl2.SysCtrl2Bit.CHG_ON = 1;
    bq_register.SysCtrl2.SysCtrl2Bit.DSG_ON = 1;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL2, bq_register.SysCtrl2.SysCtrl2Byte);

    // 3.保护寄存器1
    // RSNS:用于设备电流检测大小范围    1:用于测量电流范围较大(5mΩ)   0.用于测量电流范围较小(20mΩ)
    bq_register.Protect1.Protect1Bit.RSNS = 1;
    // 配置短路保护(电流)阈值,保护延迟时间.用于过滤电流尖峰，避免短路保护误触发
    bq_register.Protect1.Protect1Bit.SCD_THRESH = 0x7;
    bq_register.Protect1.Protect1Bit.SCD_DELAY = BMS_SCD_DELAY_400us;
    Int_BQ769_WriteRegister(BQ_PROTECT1, bq_register.Protect1.Protect1Byte);

    // 4.保护寄存器2
    // 主要配置放电的时候电流过大的保护相关配置
    // 放电过流保护阈值配置
    bq_register.Protect2.Protect2Bit.OCD_THRESH = 0xF;
    // 放电过流保护触发延迟配置
    bq_register.Protect2.Protect2Bit.OCD_DELAY = BMS_OCD_DELAY_1280ms;
    Int_BQ769_WriteRegister(BQ_PROTECT2, bq_register.Protect2.Protect2Byte);

    // 5.保护寄存器3
    // 设置充电过压保护的延迟时间   放电欠压的时候保护延迟时间
    // 欠压延迟
    bq_register.Protect3.Protect3Bit.UV_DELAY = BMS_UV_DELAY_16s;
    bq_register.Protect3.Protect3Bit.OV_DELAY = BMS_OV_DELAY_8s;
    Int_BQ769_WriteRegister(BQ_PROTECT3, bq_register.Protect3.Protect3Byte);

    // 计算出过压与欠压的阈值
    // 过压阈值
    uint16_t ov = (OV_PROTECT * 1000.0 - offset) / gain;
    uint8_t ov_result = (ov >> 4) & 0xFF;

    // 欠压阈值
    uint16_t uv = (UV_PROTECT * 1000.0 - offset) / gain;
    uint8_t uv_result = (uv >> 4) & 0xFF;

    Int_BQ769_WriteRegister(BQ_OV_TRIP, ov_result);
    Int_BQ769_WriteRegister(BQ_UV_TRIP, uv_result);

    // CC_CFG
    Int_BQ769_WriteRegister(BQ_CC_CFG, 0x19);
}
