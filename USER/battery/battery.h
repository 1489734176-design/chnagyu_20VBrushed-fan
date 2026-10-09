#ifndef __BATTERY_H
#define __BATTERY_H

#include <stdint.h>

/* 初始化电压保护，并在 ADC 初始化后立即检查上电电压。 */
void Battery_Init(void);

/* 每 1 ms 调用一次，执行欠压/过压检测及恢复滤波。 */
void Battery_Task(void);

/* 启动前重新采样并快速锁存异常；1=允许启动，0=禁止，不清除已有保护。 */
uint8_t Battery_CheckBeforeStart(void);

/* 返回最近一次采样到的 VBus 毫伏值，不计算电量百分比。 */
uint32_t Battery_GetVoltageMv(void);

/* 返回欠压保护状态，1=欠压锁存，0=正常。 */
uint8_t Battery_IsUnderVoltage(void);

/* 返回过压保护状态，1=过压锁存，0=正常。 */
uint8_t Battery_IsOverVoltage(void);

/* 电压或 ADC 故障仍未解除时返回 1。 */
uint8_t Battery_IsProtected(void);

/* 电压查询保留最近有效值，使用前应核对采样有效性及故障锁存。 */
uint8_t Battery_IsSampleValid(void);
uint8_t Battery_IsAdcFault(void);

#endif /* __BATTERY_H */
