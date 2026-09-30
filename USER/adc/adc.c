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
 * @brief 读取一个 ADC 通道的原始值。
 * @param channel ADC 通道号，例如 ADC_Channel_0、ADC_Channel_1、ADC_Channel_5。
 * @return 12 位 ADC 原始计数。
 */
uint16_t AppAdc_ReadRaw(uint8_t channel)
{
    uint16_t value;

    /* 使用芯片自带的任意通道单次转换接口。 */
    ADC_AnyChannelNumCfg(ADC1, 0U);
    ADC_AnyChannelSelect(ADC1, ADC_AnyChannel_0, channel);
    ADC_AnyChannelCmd(ADC1, ENABLE);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 等待本次转换结束，前台调用频率低，不在中断中使用此函数。 */
    while (RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
    {
    }

    value = ADC_GetConversionValue(ADC1);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_AnyChannelCmd(ADC1, DISABLE);
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

    voltage_mv = (uint32_t)raw * APP_ADC_VREF_MV;
    voltage_mv /= APP_ADC_FULL_SCALE;
    voltage_mv *= divider_ratio;
    return voltage_mv;
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
