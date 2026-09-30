#include "motor.h"
#include "platform.h"
#include "config.h"

/* PWM1 的物理引脚：PA5，TIM1_CH1N，AF1。 */
#define MOTOR_FAN_PWM_PIN                  (GPIO_Pin_5)

/* PWM2 的物理引脚：PA6，TIM1_CH3，AF4。 */
#define MOTOR_PUMP_PWM_PIN                 (GPIO_Pin_6)

/* 用户指定的风扇档位比较值百分比，数值原样保留。 */
#define FAN_GEAR_1_PWM_PERCENT             (32U)
#define FAN_GEAR_2_PWM_PERCENT             (18U)
#define FAN_GEAR_3_PWM_PERCENT             (0U)

/* 用户指定的水泵档位比较值百分比，0 档由通道关闭实现安全停机。 */
#define PUMP_GEAR_0_PWM_PERCENT            (0U)
#define PUMP_GEAR_1_PWM_PERCENT            (30U)
#define PUMP_GEAR_2_PWM_PERCENT            (10U)
#define PUMP_GEAR_3_PWM_PERCENT            (0U)

/* TIM1 的 PWM 自动重装值，运行时按实际定时器时钟计算。 */
static uint16_t motor_pwm_period = 0U;

/* 当前风扇档位变量，0 表示通道关闭。 */
static uint8_t current_fan_gear = 0U;

/* 当前水泵档位变量，0 表示通道关闭。 */
static uint8_t current_pump_gear = 0U;

/*
 * @brief 把用户指定的百分比换算成 TIM1 CCR 值。
 * @param percent 比较值百分比，范围 0~100。
 * @return 对应的 CCR 值。
 * @note 这里保留用户给出的原始百分比，不在软件中反向改写档位表。
 */
static uint16_t Motor_CompareFromPercent(uint8_t percent)
{
    uint32_t compare;

    if (percent > 100U)
    {
        percent = 100U;
    }

    compare = ((uint32_t)motor_pwm_period + 1UL) * (uint32_t)percent;
    compare /= 100UL;

    if (compare > motor_pwm_period)
    {
        compare = motor_pwm_period;
    }

    return (uint16_t)compare;
}

/*
 * @brief 配置 TIM1 双通道 16 kHz 中心对齐 PWM。
 * @note PA5 使用 CH1N，PA6 使用 CH3；两个通道分别使用 CCR1 和 CCR3。
 */
void Motor_Init(void)
{
    GPIO_InitTypeDef gpio_init;
    TIM_TimeBaseInitTypeDef time_base_init;
    TIM_OCInitTypeDef oc_init;

    /* 先打开 GPIO 时钟并把 PWM 引脚置为低电平推挽，避免复用前悬空。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);
    GPIO_ResetBits(GPIOA, MOTOR_FAN_PWM_PIN | MOTOR_PUMP_PWM_PIN);
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = MOTOR_FAN_PWM_PIN | MOTOR_PUMP_PWM_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio_init);

    /* 配置 TIM1 基本计数器，中心对齐频率为时钟/(2*ARR 周期)。 */
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM1, ENABLE);
    motor_pwm_period = (uint16_t)(TIM_GetTIMxClock(TIM1) /
                                  (2UL * MOTOR_PWM_FREQUENCY_HZ) - 1UL);

    TIM_TimeBaseStructInit(&time_base_init);
    time_base_init.TIM_Prescaler = 0U;
    time_base_init.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
    time_base_init.TIM_Period = motor_pwm_period;
    time_base_init.TIM_ClockDivision = TIM_CKD_Div1;
    time_base_init.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM1, &time_base_init);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* 配置 TIM1_CH1N：关闭主输出，只开放互补 N 输出。 */
    TIM_OCStructInit(&oc_init);
    oc_init.TIM_OCMode = TIM_OCMode_PWM1;
    oc_init.TIM_OutputState = TIM_OutputState_Disable;
    oc_init.TIM_OutputNState = TIM_OutputNState_Enable;
    oc_init.TIM_Pulse = 0U;
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
    oc_init.TIM_OCNPolarity = TIM_OCNPolarity_Low;
    oc_init.TIM_OCIdleState = TIM_OCIdleState_Reset;
    oc_init.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &oc_init);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* 配置 TIM1_CH3：只开放普通输出，和 CH1N 使用同一 PWM 逻辑相位。 */
    oc_init.TIM_OutputState = TIM_OutputState_Enable;
    oc_init.TIM_OutputNState = TIM_OutputNState_Disable;
    oc_init.TIM_Pulse = 0U;
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC3Init(TIM1, &oc_init);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* 按 MM32G0001 官方 TIM1 样例设置两个引脚的 AF。 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_4);
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = MOTOR_FAN_PWM_PIN | MOTOR_PUMP_PWM_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio_init);

    /* 启动定时器但先关闭两个通道，默认状态必须是电机停机。 */
    TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Disable);
    TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Disable);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    current_fan_gear = 0U;
    current_pump_gear = 0U;
}

/*
 * @brief 设置风扇档位。
 * @param gear 0=停机，1=32%，2=18%，3=0%。
 */
void Motor_SetFanGear(uint8_t gear)
{
    uint8_t percent = FAN_GEAR_3_PWM_PERCENT;

    if (gear > MOTOR_FAN_GEAR_MAX)
    {
        gear = MOTOR_FAN_GEAR_MAX;
    }

    if (gear == 1U)
    {
        percent = FAN_GEAR_1_PWM_PERCENT;
    }
    else if (gear == 2U)
    {
        percent = FAN_GEAR_2_PWM_PERCENT;
    }

    TIM_SetCompare1(TIM1, Motor_CompareFromPercent(percent));
    current_fan_gear = gear;

    if (gear == 0U)
    {
        /* 关闭 CH1N，停机不依赖某个 CCR 数值。 */
        TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Disable);
    }
    else
    {
        /* 先写 CCR，再开放风扇 PWM 通道，避免开启时产生旧占空比。 */
        TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Enable);
    }
}

/*
 * @brief 设置水泵档位。
 * @param gear 0=停机，1=30%，2=10%，3=0%。
 */
void Motor_SetPumpGear(uint8_t gear)
{
    uint8_t percent = PUMP_GEAR_0_PWM_PERCENT;

    if (gear > MOTOR_PUMP_GEAR_MAX)
    {
        gear = MOTOR_PUMP_GEAR_MAX;
    }

    if (gear == 1U)
    {
        percent = PUMP_GEAR_1_PWM_PERCENT;
    }
    else if (gear == 2U)
    {
        percent = PUMP_GEAR_2_PWM_PERCENT;
    }
    else if (gear == 3U)
    {
        percent = PUMP_GEAR_3_PWM_PERCENT;
    }

    TIM_SetCompare3(TIM1, Motor_CompareFromPercent(percent));
    current_pump_gear = gear;

    if (gear == 0U)
    {
        /* 水泵 0 档使用通道关闭，确保 0 档不是悬空输出。 */
        TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Disable);
    }
    else
    {
        /* 先写 CCR，再开放水泵 PWM 通道。 */
        TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Enable);
    }
}

/*
 * @brief 关闭全部电机输出。
 * @note 两个 CCR 都清零，并关闭通道；再次设置档位时会重新开放对应通道。
 */
void Motor_StopAll(void)
{
    TIM_SetCompare1(TIM1, 0U);
    TIM_SetCompare3(TIM1, 0U);
    TIM_CCxNCmd(TIM1, TIM_Channel_1, TIM_CCxN_Disable);
    TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Disable);
    current_fan_gear = 0U;
    current_pump_gear = 0U;
}

/* @brief 返回当前风扇档位。 */
uint8_t Motor_GetFanGear(void)
{
    return current_fan_gear;
}

/* @brief 返回当前水泵档位。 */
uint8_t Motor_GetPumpGear(void)
{
    return current_pump_gear;
}

/* @brief 返回当前 PWM 自动重装值。 */
uint16_t Motor_GetPwmPeriod(void)
{
    return motor_pwm_period;
}
