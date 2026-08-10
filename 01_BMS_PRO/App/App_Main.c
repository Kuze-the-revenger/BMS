#include "App_Main.h"

// OLED
void oled_callBack(void *params);
TaskHandle_t oled_handler;

// BQ769
void battery_callBack(void *params);
TaskHandle_t battery_handler;

// 均衡管理参数
void balance_callBack(void *params);
TaskHandle_t balance_handler;

// 1.初始化
void App_Main_Init(void)
{
    // 创建动态任务-OLED
    xTaskCreate(oled_callBack, "oled", 128, NULL, 3, &oled_handler);
    // 创建动态任务-BQ769
    xTaskCreate(battery_callBack, "battery", 128, NULL, 5, &battery_handler);
    // 创建动态任务->BQ769开启均衡管理
    xTaskCreate(balance_callBack, "balance", 128, NULL, 4, &balance_handler);
    // 开启调度器
    vTaskStartScheduler();
}

// oled任务
void oled_callBack(void *params)
{
    // 1.oled初始化
    App_Display_Init();

    while (1)
    {
        // 2.每隔50刷新OLED内容
        // 进入临界区:在任务内部使用,底层原理,freeRTOS源码操作BASEPRI寄存器(中断屏蔽寄存器)
        taskENTER_CRITICAL();
        App_Display_ShowContent();
        // 退出临界区
        taskEXIT_CRITICAL();
        vTaskDelay(50);
    }
}
// BQ769任务
void battery_callBack(void *params)
{
    // 1.BQ769初始化
    App_Battery_Init();
    while (1)
    {
        // 获取电池组每一节电池的电压
        App_Battery_GetV();
        // 获取电池组总电压
        App_Battery_GetTotalV();
        // 获取电流大小
        App_Battery_GetCurrentA();
        // 获取电池组温度
        App_battery_GetTemp();
        // 获取电池组剩余电量
        App_Battery_getSoc();
        vTaskDelay(500);
    }
}

// 均衡管理任务
void balance_callBack(void *params)
{
    while (1)
    {
        // 均衡管理:需要依赖电池组的电压、温度.
        App_Battery_Balance();
        vTaskDelay(300);
    }
}