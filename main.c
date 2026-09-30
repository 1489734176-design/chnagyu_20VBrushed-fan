#include "platform.h"
#include "main.h"
#include "power.h"
#include "motor.h"
#include "key.h"
#include "led.h"
#include "adc.h"
#include "battery.h"
#include "charge.h"
#include "timer.h"
#include "app.h"

/* TIM14 中断置位的 1 ms 应用节拍标志。 */
volatile uint8_t flag_1ms = 0U;

/*
 * @brief 固件主入口。
 * @note 初始化顺序优先保证 EN、LED 和电机输出安全，再启动按键和应用任务。
 */
int main(void)
{
    /* 初始化 SysTick 延时和平台基础配置。 */
    PLATFORM_Init();

    /* 最早接管 EN，避免新板因软件初始化耗时而掉电。 */
    Power_Init();

    /* 初始化所有安全输出和输入外设。 */
    Led_Init();
    Motor_Init();
    AppAdc_Init();
    Key_Init();
    AppTimer_Init();

    /* 初始化电量、充电和产品状态机。 */
    Battery_Init();
    Charge_Init();
    App_Init();

    /* 主循环只在 TIM14 产生新 1 ms 标志时运行一次应用任务。 */
    while (1)
    {
        if (flag_1ms != 0U)
        {
            flag_1ms = 0U;
            App_Task();
        }
    }
}
