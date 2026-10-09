#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

/* 风扇档位编号：K1 在 1/2/3 档之间循环，开机档位由应用层恢复。 */
#define MOTOR_FAN_GEAR_MIN                 (1U)
#define MOTOR_FAN_GEAR_MAX                 (3U)

/* 水泵档位编号：开机默认 0 档，K2 在 0/1/2/3 档之间循环。 */
#define MOTOR_PUMP_GEAR_MIN                (0U)
#define MOTOR_PUMP_GEAR_MAX                (3U)

/* 初始化 TIM1 双路 PWM 和 PA5、PA6 复用输出。 */
void Motor_Init(void);

/* 设置风扇档位，gear=0 用于安全停机，1/2/3 使用用户给出的 PWM 百分比。 */
void Motor_SetFanGear(uint8_t gear);

/* 设置水泵档位，gear=0 表示停机，1/2/3 使用用户给出的 PWM 百分比。 */
void Motor_SetPumpGear(uint8_t gear);

/* 关闭两个 PWM 通道并把电机输出置于安全状态。 */
void Motor_StopAll(void);

/* 读取当前风扇档位，便于应用层保存或显示状态。 */
uint8_t Motor_GetFanGear(void);

/* 读取当前水泵档位，便于应用层保存或显示状态。 */
uint8_t Motor_GetPumpGear(void);

/* 读取当前 TIM1 自动重装值，便于调试 PWM 计算。 */
uint16_t Motor_GetPwmPeriod(void);

#endif /* __MOTOR_H */
