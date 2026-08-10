#include "App_Display.h"
// 定义变量存储电池组总电压
extern float battery_totalV;
// 定义变量存储电流大小
extern float battery_A;
// 定义变量存储温度
extern int8_t battery_temp;
// 定义变量存储剩余电量
extern uint8_t battery_soc;

// 展示标题
static void App_Display_ShowTitle(void);
// 刷新内容
static void App_Display_Refresh(void);

// 1.初始化
void App_Display_Init(void)
{
    Inf_OLED_Init();
}

// 2.展示内容
void App_Display_ShowContent(void)
{

    // 1.展示汉字标题
    App_Display_ShowTitle();

    // 2.刷新内容
    App_Display_Refresh();
}
// 标题
static void App_Display_ShowTitle(void)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        // 1.坐标x,y  字模索引  字体大小   是否要背景颜色
        Inf_OLED_ShowChinese(24 + 16 * i, 0, i, 16, 1);
    }
}
// 刷新
static void App_Display_Refresh(void)
{

    // 显示电压
    char vstr[6];
    sprintf(vstr, "V:%4.1f", battery_totalV);
    Inf_OLED_ShowString(2, 30, (uint8_t *)vstr, 16, 1);

    // 显示电流
    char cstr[6];
    sprintf(cstr, "C:%4.1f", battery_A);
    Inf_OLED_ShowString(64, 30, (uint8_t *)cstr, 16, 1);
    // 显示温度
    char tstr[6];
    sprintf(tstr, "T:%d", battery_temp);
    Inf_OLED_ShowString(2, 48, (uint8_t *)tstr, 16, 1);
    // 显示电量
    char sstr[6];
    sprintf(sstr, "S:%4.1f%%", (float)battery_soc);
    Inf_OLED_ShowString(64, 48, (uint8_t *)sstr, 16, 1);

    Inf_OLED_Refresh();
}
