#include "timer.h"
#include "platform.h"

/*
 * @brief 初始化 TIM14 产生 1 ms 更新中断。
 * @note TIM14 不再连接灯光 PWM，也不再驱动 Charlieplex 数码管扫描。
 */
void AppTimer_Init(void)
{
    NVIC_InitTypeDef nvic_init;
    TIM_TimeBaseInitTypeDef time_base_init;
    uint32_t timer_clock_hz;

    /* 打开 TIM14 时钟，并根据运行时定时器时钟计算周期。 */
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM14, ENABLE);
    timer_clock_hz = TIM_GetTIMxClock(TIM14);

    TIM_TimeBaseStructInit(&time_base_init);
    time_base_init.TIM_Prescaler = 0U;
    time_base_init.TIM_CounterMode = TIM_CounterMode_Up;
    time_base_init.TIM_Period = (timer_clock_hz / 1000UL) - 1UL;
    time_base_init.TIM_ClockDivision = TIM_CKD_Div1;
    time_base_init.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM14, &time_base_init);

    /* 清除旧更新标志后打开 TIM14 更新中断。 */
    TIM_ClearFlag(TIM14, TIM_FLAG_Update);
    TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);

    /* 设置中断优先级并使能 TIM14 IRQ。 */
    nvic_init.NVIC_IRQChannel = TIM14_IRQn;
    nvic_init.NVIC_IRQChannelPriority = 0x01U;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    /* 启动 1 ms 应用节拍。 */
    TIM_Cmd(TIM14, ENABLE);
}
