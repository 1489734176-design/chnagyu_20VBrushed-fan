#include "power.h"
#include "config.h"

/*
 * @brief 初始化电源相关 GPIO。
 * @note 先写安全输出电平，再切换为推挽输出，减少上电毛刺。
 */
void Power_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* 使能 GPIOA 时钟，EN 和 CH_EN 都位于 GPIOA。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);

    /* 上电默认禁止充电，避免初始化过程误打开充电通路。 */
    GPIO_ResetBits(GPIOA, POWER_CHARGE_ENABLE_PIN);
    /* 先预置 EN 为高，接管硬件保电。 */
    GPIO_SetBits(GPIOA, POWER_EN_PIN);

    /* 配置 EN 和 CH_EN 为高速推挽输出。 */
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = POWER_EN_PIN | POWER_CHARGE_ENABLE_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio_init);
}

/*
 * @brief 设置主电源保持状态。
 * @param enable 1 表示保持供电，0 表示释放硬件保持。
 */
void Power_SetKeepAlive(uint8_t enable)
{
    if (enable != 0U)
    {
        GPIO_SetBits(GPIOA, POWER_EN_PIN);
    }
    else
    {
        GPIO_ResetBits(GPIOA, POWER_EN_PIN);
    }
}

/*
 * @brief 设置充电通路使能状态。
 * @param enable 1 表示请求打开充电，0 表示禁止充电。
 * @note 充电功能关闭期间无论调用参数如何都保持 CH_EN 为低。
 */
void Power_SetChargeEnable(uint8_t enable)
{
#if CHARGE_MANAGEMENT_ENABLE
    if (enable != 0U)
    {
        GPIO_SetBits(GPIOA, POWER_CHARGE_ENABLE_PIN);
    }
    else
    {
        GPIO_ResetBits(GPIOA, POWER_CHARGE_ENABLE_PIN);
    }
#else
    (void)enable;
    GPIO_ResetBits(GPIOA, POWER_CHARGE_ENABLE_PIN);
#endif
}

/*
 * @brief 执行整机安全关断。
 * @note 电机停止由 motor 模块先完成，此函数只处理电源和充电控制脚。
 */
void Power_SafePowerOff(void)
{
    /* 无论功能开关状态如何，都先禁止充电通路。 */
    GPIO_ResetBits(GPIOA, POWER_CHARGE_ENABLE_PIN);
    /* 最后释放 EN，允许硬件电源关闭。 */
    GPIO_ResetBits(GPIOA, POWER_EN_PIN);
}
