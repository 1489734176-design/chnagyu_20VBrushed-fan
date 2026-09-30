#ifndef __APP_ADC_H
#define __APP_ADC_H

#include <stdint.h>

/* 新板 ADC 通道映射：PB1=VBus/CH0，PB0=CH_VIN/CH1，PA2=I_SENSE/CH5。 */
#define ADC_CHANNEL_VBUS                   (ADC_Channel_0)
#define ADC_CHANNEL_CHARGE_INPUT           (ADC_Channel_1)
#define ADC_CHANNEL_CURRENT_SENSE           (ADC_Channel_5)

/* 初始化三路模拟输入和 ADC1。 */
void AppAdc_Init(void);

/* 读取指定 ADC 通道的 12 位原始计数。 */
uint16_t AppAdc_ReadRaw(uint8_t channel);

/* 读取 VBus，返回按 11 倍分压换算后的毫伏值。 */
uint32_t AppAdc_ReadVbusMv(void);

/* 读取充电输入 CH_VIN，返回按 11 倍分压换算后的毫伏值。 */
uint32_t AppAdc_ReadChargeInputMv(void);

/* 读取公共电机回流采样，返回按 5 mΩ 换算的毫安值。 */
uint32_t AppAdc_ReadCurrentMa(void);

#endif /* __APP_ADC_H */
