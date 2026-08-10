#include "Int_BQ769.h"
RegisterGroup bq_register;
// CRC计算
uint8_t crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0x00; // 初始值为0
    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc <<= 1;
            }
        }
        crc &= 0xFF; // 确保只保留8位
    }
    return crc;
}

// 唤醒功能
static void Int_BQ769_WakeUp(void);
// 进入SHIP模式,本质I2C向BQ769的SYS_CTRL1寄存器写入两遍数据
static void Int_BQ769_Ship(void);

// 1.初始化
void Int_BQ769_Init(void)
{
    Int_BQ769_Ship();
    HAL_Delay(300);
    Int_BQ769_WakeUp();
    COM_DEBUG_LN("BQ769 Start Up...");
}

// 2.向BQ769的某一个寄存器写入单个字节
void Int_BQ769_WriteRegister(uint16_t mem_addr, uint8_t wb)
{
    uint8_t raw_arr[3] = {DEVICE_ADDR, mem_addr, wb};
    uint8_t crc_res = crc8(raw_arr, 3);
    uint8_t res_dta[2] = {wb, crc_res};

    taskENTER_CRITICAL();
    // 8.3.1.4the CRC for the first data byte is calculated over the slave address, register address, and data.
    HAL_I2C_Mem_Write(&hi2c2, DEVICE_ADDR, mem_addr, I2C_MEMADD_SIZE_8BIT, res_dta, 2, 1000);
    taskEXIT_CRITICAL();
}

// 3.读取多个寄存器数据-连续读取
uint8_t Int_BQ769_ReadRegister(uint16_t mem_addr, uint8_t *buffer, uint16_t sizes)
{

    uint8_t *temp_array = pvPortMalloc(sizes * 2);
    taskENTER_CRITICAL();
    HAL_I2C_Mem_Read(&hi2c2, DEVICE_ADDR, mem_addr, I2C_MEMADD_SIZE_8BIT, temp_array, sizes * 2, 1000);
    taskEXIT_CRITICAL();

    for (uint8_t i = 0; i < sizes; i++)
    {
        if (i == 0)
        {
            /*calculated after the second start and uses the slave address and data byte. */
            uint8_t raw_arr[2] = {DEVICE_ADDR + 1, temp_array[0]};
            uint8_t crc_res = crc8(raw_arr, 2);
            if (crc_res != temp_array[1])
            {
                COM_DEBUG_LN("Read Data CRC ERROR!!");
                vPortFree(temp_array);
                return 0;
            }
        }
        else
        {
            /*In a single-byte read transaction, the CRC is calculated after the second start and uses the slave address
and data byte.*/
            uint8_t crc_res = crc8(&temp_array[i * 2], 1);
            if (crc_res != temp_array[i * 2 + 1])
            {
                COM_DEBUG_LN("Read Data CRC ERROR!");
                vPortFree(temp_array);
                return 0;
            }
        }
        buffer[i] = temp_array[2 * i];
    }
}

// SHIP模式,低功耗
static void Int_BQ769_Ship(void)
{
    /*
    To enter SHIP mode from NORMAL mode, the [SHUT_A] and [SHUT_B] bits in the SYS_CTRL1 register must
be written with specific patterns across two consecutive writes:
?
Write #1: [SHUT_A] = 0, [SHUT_B] = 1
?
Write #2: [SHUT_A] = 1, [SHUT_B] = 0
*/

    bq_register.SysCtrl1.SysCtrl1Bit.SHUT_A = 0;
    bq_register.SysCtrl1.SysCtrl1Bit.SHUT_B = 1;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL1, bq_register.SysCtrl1.SysCtrl1Byte);
    bq_register.SysCtrl1.SysCtrl1Bit.SHUT_A = 1;
    bq_register.SysCtrl1.SysCtrl1Bit.SHUT_B = 0;
    Int_BQ769_WriteRegister(BQ_SYS_CTRL1, bq_register.SysCtrl1.SysCtrl1Byte);
}

// 唤醒
static void Int_BQ769_WakeUp(void)
{
    // TS1拉高2ms

    HAL_GPIO_WritePin(BQ769_WAKEUP_GPIO_Port, BQ769_WAKEUP_Pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(BQ769_WAKEUP_GPIO_Port, BQ769_WAKEUP_Pin, GPIO_PIN_SET);
}
