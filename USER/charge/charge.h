#ifndef __CHARGE_H
#define __CHARGE_H

#include <stdint.h>

/* 充电输入判定阈值，启用充电功能前需按充电器规格校准。 */
#define CHARGE_INPUT_PRESENT_MV            (9000UL)

/* 初始化充电管理模块，默认关闭 CH_EN。 */
void Charge_Init(void);

/* 执行充电检测和通路控制任务，功能宏关闭时为空操作。 */
void Charge_Task(void);

/* 请求打开或关闭充电通路，功能宏关闭时始终保持关闭。 */
void Charge_SetEnable(uint8_t enable);

/* 返回当前是否检测到外部充电输入。 */
uint8_t Charge_IsInputPresent(void);

/* 返回当前软件是否允许充电通路。 */
uint8_t Charge_IsEnabled(void);

#endif /* __CHARGE_H */
