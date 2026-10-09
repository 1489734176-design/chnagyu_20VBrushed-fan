#ifndef __CHARGE_H
#define __CHARGE_H

#include <stdint.h>

/* 输入状态不等于充电状态，未确认/故障时不能当作已拔出。 */
typedef enum
{
    CHARGE_INPUT_UNKNOWN = 0U,
    CHARGE_INPUT_ABSENT,
    CHARGE_INPUT_PRESENT,
    CHARGE_INPUT_FAULT,
    CHARGE_INPUT_DISABLED
} charge_input_state_t;

/* 初始化输入检测，默认关闭 CH_EN。 */
void Charge_Init(void);

/* 采样并更新输入/截止状态，只能立即关闭输出；打开由周期末统一决策。 */
void Charge_Task(void);

/* 应用周期末调用：实际电源释放时传 0，否则传 1；按安全条件自动使能。 */
void Charge_UpdateOutput(uint8_t power_available);

/* 关闭立即生效；打开仍受输入、ADC、过压、截止电压及电源状态约束。 */
void Charge_SetEnable(uint8_t enable);

charge_input_state_t Charge_GetInputState(void);
uint8_t Charge_IsInputPresent(void);

/* 连续低电压已确认拔出，当前样本也有效，才允许逻辑关机释放电源。 */
uint8_t Charge_IsInputAbsent(void);

/* 最近一次采样是否有效；电压查询保留最近有效值，不代表故障时仍可使用。 */
uint8_t Charge_IsInputSampleValid(void);
uint32_t Charge_GetInputVoltageMv(void);

/* 通路的软件输出状态，不代表测得电池充电电流。 */
uint8_t Charge_IsEnabled(void);

/* 电压截止不是充满判定；有效电压低于截止值后清除，无需拔插充电器。 */
uint8_t Charge_IsVoltageStopped(void);

#endif /* __CHARGE_H */
