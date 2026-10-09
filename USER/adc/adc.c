#include "adc.h"
#include "platform.h"
#include "config.h"

/*
 * @brief 初始化新原理图的三路 ADC 输入。
 * @note PA2、PB0、PB1 必须保持模拟模式，避免数字输入漏电影响采样。
 */
void AppAdc_Init(void)
{
    GPIO_InitTypeDef gpio_init;
    ADC_InitTypeDef adc_init;

    /* 使能 ADC 使用的 GPIOA、GPIOB 时钟。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA | RCC_AHBPERIPH_GPIOB, ENABLE);

    /* PA2 为 I_SENSE，配置为模拟输入。 */
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = GPIO_Pin_2;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio_init);

    /* PB0 为 CH_VIN，PB1 为 VBus，两个输入均配置为模拟模式。 */
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &gpio_init);

    /* 打开 ADC1，并配置为 12 位单次转换。 */
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_ADC1, ENABLE);
    ADC_StructInit(&adc_init);
    adc_init.ADC_Resolution = ADC_Resolution_12b;
    adc_init.ADC_Prescaler = ADC_Prescaler_16;
    adc_init.ADC_Mode = ADC_Mode_Imm;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_Init(ADC1, &adc_init);
    ADC_SampleTimeConfig(ADC1, ADC_SampleTime_240_5);
    ADC_Cmd(ADC1, ENABLE);
}

/*
 * @brief 有界读取 ADC，成功返回 1 并写入 raw，失败返回 0 且不改输出值。
 * @note 超时停止转换并清理标志，避免永久阻塞；只在前台串行调用。
 */
uint8_t AppAdc_TryReadRaw(uint8_t channel, uint16_t *raw)
{
    uint32_t polls;
    uint8_t success = 0U;

    if (raw == 0)
    {
        return 0U;
    }

    ADC_SoftwareStartConvCmd(ADC1, DISABLE);
    ADC_AnyChannelNumCfg(ADC1, 0U);
    ADC_AnyChannelSelect(ADC1, ADC_AnyChannel_0, channel);
    ADC_AnyChannelCmd(ADC1, ENABLE);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    for (polls = 0UL; polls < APP_ADC_EOC_POLL_LIMIT; polls++)
    {
        if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != RESET)
        {
            *raw = ADC_GetConversionValue(ADC1);
            success = 1U;
            break;
        }
    }

    ADC_SoftwareStartConvCmd(ADC1, DISABLE);
    ADC_AnyChannelCmd(ADC1, DISABLE);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    return success;
}

/* 兼容旧接口：失败返回 0，安全决策必须使用带成功标志的 TryRead 接口。 */
uint16_t AppAdc_ReadRaw(uint8_t channel)
{
    uint16_t value = 0U;

    (void)AppAdc_TryReadRaw(channel, &value);
    return value;
}

/*
 * @brief 将 ADC 原始值换算为外部电压。
 * @param raw ADC 原始值。
 * @param divider_ratio 输入分压倍率。
 * @return 外部信号的毫伏值。
 */
static uint32_t AppAdc_RawToMv(uint16_t raw, uint32_t divider_ratio)
{
    uint32_t voltage_mv;

    /* 当前参数最大乘积为 4095 * 5000 * 11，32 位可容纳；最后再除减少截断。 */
    voltage_mv = (uint32_t)raw * APP_ADC_VREF_MV * divider_ratio;
    voltage_mv /= APP_ADC_FULL_SCALE;
    return voltage_mv;
}

static uint8_t AppAdc_TryReadMv(uint8_t channel, uint32_t divider_ratio,
                               uint32_t *voltage_mv)
{
    uint16_t raw;

    if ((voltage_mv == 0) || (AppAdc_TryReadRaw(channel, &raw) == 0U))
    {
        return 0U;
    }
    *voltage_mv = AppAdc_RawToMv(raw, divider_ratio);
    return 1U;
}

uint8_t AppAdc_TryReadVbusMv(uint32_t *voltage_mv)
{
    return AppAdc_TryReadMv(ADC_CHANNEL_VBUS, VBUS_DIVIDER_RATIO, voltage_mv);
}

uint8_t AppAdc_TryReadChargeInputMv(uint32_t *voltage_mv)
{
    return AppAdc_TryReadMv(ADC_CHANNEL_CHARGE_INPUT,
                          CHARGE_INPUT_DIVIDER_RATIO, voltage_mv);
}

/* @brief 读取 VBus 外部电压，分压倍率为 11。 */
uint32_t AppAdc_ReadVbusMv(void)
{
    return AppAdc_RawToMv(AppAdc_ReadRaw(ADC_CHANNEL_VBUS),
                          VBUS_DIVIDER_RATIO);
}

/* @brief 读取充电输入外部电压，分压倍率为 11。 */
uint32_t AppAdc_ReadChargeInputMv(void)
{
    return AppAdc_RawToMv(AppAdc_ReadRaw(ADC_CHANNEL_CHARGE_INPUT),
                          CHARGE_INPUT_DIVIDER_RATIO);
}

/*
 * @brief 读取公共电机回流电流。
 * @return 按 R18=5 mΩ 计算的毫安值。
 * @note 当前仅提供换算接口，未启用任何未经标定的过流保护。
 */
uint32_t AppAdc_ReadCurrentMa(void)
{
    uint32_t sense_mv;
    uint32_t current_ma;

    sense_mv = AppAdc_RawToMv(AppAdc_ReadRaw(ADC_CHANNEL_CURRENT_SENSE), 1UL);
    current_ma = sense_mv * 1000UL;
    current_ma /= (MOTOR_SHUNT_MICRO_OHM / 1000UL);
    return current_ma;
}
