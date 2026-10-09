#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

/* ============================================================================
 * 新原理图应用配置
 * 本文件集中保存本板的时钟、PWM 和可选功能开关，便于后续联调时统一修改。
 * ========================================================================== */

/* 系统运行时钟，当前工程的 system_mm32g0001.c 配置为 48 MHz HSI。 */
#define APP_SYSTEM_CLOCK_HZ                 (48000000UL)

/* 两路电机共用的 PWM 载波频率，用户要求为 16 kHz。 */
#define MOTOR_PWM_FREQUENCY_HZ              (16000UL)

/* 电池电压保护开关：0=关闭保护，1=启用欠压/过压保护，不计算或显示电量。 */
#define BATTERY_PROTECTION_ENABLE          (1U)

/* 适配器输入检测独立于充电使能：可以检测插拔，但不代表允许充电。 */
#define CHARGE_INPUT_DETECTION_ENABLE      (1U)

/* 硬件适配器负责恒流，软件启用通路并在 VBus 达到截止电压后锁存关闭。 */
#define CHARGE_MANAGEMENT_ENABLE           (1U)
#define CHARGE_STOP_VOLTAGE_MV             (20000UL)

/* 外部供电存在检测初值，兼容恒流时输出降压；须实测最低输出并校准。 */
#define CHARGE_INPUT_INSERT_MV             (18500UL)
#define CHARGE_INPUT_REMOVE_MV             (18000UL)
#define CHARGE_INPUT_FILTER_TICKS          (100U)

/* 硬件充电电流目标，软件不设置/限制电流，也不使用电机 I_SENSE 控制充电。 */
#define CHARGE_TARGET_CURRENT_MA           (4000UL)

/* ADC 参考电压，实际值应在新芯片资料和实测后校准。 */
#define APP_ADC_VREF_MV                    (5000UL)

/* ADC 满量程计数，当前配置为 12 位。 */
#define APP_ADC_FULL_SCALE                 (4095UL)

/* ADC EOC 最大轮询次数（不是毫秒），实际最坏耗时须在板上测量。 */
#define APP_ADC_EOC_POLL_LIMIT             (1024UL)

/* ADC 故障后须连续有效采样才解除；按实际服务任务次数计数。 */
#define BATTERY_ADC_RECOVER_TICKS          (300U)

/* VBus 分压倍率：10 kΩ 上臂、1 kΩ 下臂，电阻网络倍率为 11。 */
#define VBUS_DIVIDER_RATIO                 (11UL)

/* CH_VIN 分压倍率：10 kΩ 上臂、1 kΩ 下臂，电阻网络倍率为 11。 */
#define CHARGE_INPUT_DIVIDER_RATIO         (11UL)

/* 电流采样电阻为 5 mΩ，单位使用微欧以避免浮点数。 */
#define MOTOR_SHUNT_MICRO_OHM              (5000UL)

/* 按键去抖时间，主任务按 TIM14 产生的 1 ms 节拍调用。 */
#define KEY_DEBOUNCE_TIME_MS               (20U)

/* 欠压触发值为 13.5 V，锁存后须恢复到 15.5 V，避免停机回弹反复启停。 */
#define BATTERY_UVP_VOLTAGE_MV             (13500UL)
#define BATTERY_UVP_RECOVER_VOLTAGE_MV     (15500UL)

/* 过压触发值为 24 V；低于此值并通过恢复滤波后解除过压保护。 */
#define BATTERY_OVP_VOLTAGE_MV             (24000UL)

/* 参考电钻 Volt_Handler 的独立计数滤波，以下计数均按 1 ms 任务节拍。 */
#define BATTERY_UVP_FILTER_TIME_MS         (300U)
#define BATTERY_OVP_FILTER_TIME_MS         (300U)

/* 电压须连续满足恢复条件 300 ms 才解除锁存，不会自动启动电机。 */
#define BATTERY_RECOVER_FILTER_TIME_MS     (300U)

#endif /* __APP_CONFIG_H */
