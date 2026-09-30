#ifndef __BATTERY_H
#define __BATTERY_H

#include <stdint.h>

/* 初始化电量管理模块；功能宏关闭时仍保留接口。 */
void Battery_Init(void);

/* 执行一次电量采样和滤波任务，建议每 1 ms 调用。 */
void Battery_Task(void);

/* 返回最近一次计算出的电量百分比。 */
uint8_t Battery_GetPercent(void);

/* 返回最近一次采样到的 VBus 毫伏值。 */
uint32_t Battery_GetVoltageMv(void);

/* 返回欠压保护状态，1=欠压，0=正常。 */
uint8_t Battery_IsUnderVoltage(void);

#endif /* __BATTERY_H */
