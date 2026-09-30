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

/* 电量管理代码开关：0=保留代码但不参与应用运行，1=启用电量计算和欠压处理。 */
#define BATTERY_MANAGEMENT_ENABLE          (0U)

/* 充电管理代码开关：0=禁止检测和 CH_EN，1=允许充电状态机控制 CH_EN。 */
#define CHARGE_MANAGEMENT_ENABLE           (0U)

/* ADC 参考电压，实际值应在新芯片资料和实测后校准。 */
#define APP_ADC_VREF_MV                    (5000UL)

/* ADC 满量程计数，当前配置为 12 位。 */
#define APP_ADC_FULL_SCALE                 (4095UL)

/* VBus 分压倍率：10 kΩ 上臂、1 kΩ 下臂，电阻网络倍率为 11。 */
#define VBUS_DIVIDER_RATIO                 (11UL)

/* CH_VIN 分压倍率：10 kΩ 上臂、1 kΩ 下臂，电阻网络倍率为 11。 */
#define CHARGE_INPUT_DIVIDER_RATIO         (11UL)

/* 电流采样电阻为 5 mΩ，单位使用微欧以避免浮点数。 */
#define MOTOR_SHUNT_MICRO_OHM              (5000UL)

/* 按键去抖时间，主任务按 TIM14 产生的 1 ms 节拍调用。 */
#define KEY_DEBOUNCE_TIME_MS               (20U)

/* 电池电压曲线的默认参数，启用电量管理前必须结合电芯规格复核。 */
#define BATTERY_FULL_VOLTAGE_MV            (8400UL)
#define BATTERY_EMPTY_VOLTAGE_MV           (6000UL)
#define BATTERY_UVP_VOLTAGE_MV             (6000UL)

#endif /* __APP_CONFIG_H */
