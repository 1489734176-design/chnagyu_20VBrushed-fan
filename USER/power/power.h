#ifndef __POWER_H
#define __POWER_H

#include "platform.h"

/* PA4 连接原理图的 EN，输出高电平时保持主电源。 */
#define POWER_EN_PIN                       (GPIO_Pin_4)

/* PA15 连接原理图的 CH_EN，输出高电平时允许充电通路。 */
#define POWER_CHARGE_ENABLE_PIN            (GPIO_Pin_15)

/* 初始化电源保持和充电通路控制脚。 */
void Power_Init(void);

/* 设置 EN 保电状态，enable=1 保持供电，enable=0 释放保持。 */
void Power_SetKeepAlive(uint8_t enable);

/* 设置 CH_EN，充电功能宏关闭时该接口仍强制输出禁止电平。 */
void Power_SetChargeEnable(uint8_t enable);

/* 同时停止充电通路并释放 EN，作为整机安全关断接口。 */
void Power_SafePowerOff(void);

#endif /* __POWER_H */
