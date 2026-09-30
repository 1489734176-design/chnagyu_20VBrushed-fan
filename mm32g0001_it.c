#include "platform.h"
#include "main.h"
#include "mm32g0001_it.h"

/*
 * @brief 非屏蔽中断处理函数。
 * @note 当前产品没有额外 NMI 业务，保留空处理入口。
 */
void NMI_Handler(void)
{
}

/*
 * @brief 硬件错误处理函数。
 * @note 出现不可恢复异常时停在此处，避免继续驱动电机。
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/* @brief SVC 异常处理函数，当前无 RTOS 服务。 */
void SVC_Handler(void)
{
}

/* @brief PendSV 异常处理函数，当前无任务切换业务。 */
void PendSV_Handler(void)
{
}

/*
 * @brief SysTick 毫秒中断。
 * @note 只服务 PLATFORM_DelayMS，不负责 App_Task 调度。
 */
void SysTick_Handler(void)
{
    if (PLATFORM_DelayTick != 0U)
    {
        PLATFORM_DelayTick--;
    }
}

/*
 * @brief TIM3 更新中断。
 * @note 新板电机使用 TIM1，TIM3 不再承载产品业务，保留清标志入口。
 */
void TIM3_IRQHandler(void)
{
    if (RESET != TIM_GetITStatus(TIM3, TIM_IT_Update))
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

/*
 * @brief TIM14 1 ms 应用节拍中断。
 * @note 已删除旧 Charlieplex 数码管扫描和四次分频逻辑。
 */
void TIM14_IRQHandler(void)
{
    if (RESET != TIM_GetITStatus(TIM14, TIM_IT_Update))
    {
        flag_1ms = 1U;
        TIM_ClearITPendingBit(TIM14, TIM_IT_Update);
    }
}

/*
 * @brief EXTI0/1 中断入口。
 * @note 当前版本未把按键配置为 EXTI，保留清标志代码供后续低功耗扩展。
 */
void EXTI0_1_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_Line0))
    {
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line1))
    {
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

/*
 * @brief EXTI4~15 中断入口。
 * @note 当前版本按键采用 1 ms 轮询，未启用 PA9/PA10/PA12 EXTI。
 */
void EXTI4_15_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_Line4))
    {
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line9))
    {
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line10))
    {
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line11))
    {
        EXTI_ClearITPendingBit(EXTI_Line11);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line12))
    {
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line13))
    {
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line14))
    {
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
    if (RESET != EXTI_GetITStatus(EXTI_Line15))
    {
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
}
