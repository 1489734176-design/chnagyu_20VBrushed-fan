#include "charge.h"
#include "config.h"
#include "adc.h"
#include "battery.h"
#include "power.h"

/* 当前是否检测到充电输入。 */
static uint8_t charge_input_present = 0U;

/* 当前软件是否请求打开充电通路。 */
static uint8_t charge_enabled = 0U;

/*
 * @brief 初始化充电管理状态。
 * @note 无论宏开关状态如何，上电都先关闭 CH_EN。
 */
void Charge_Init(void)
{
    charge_input_present = 0U;
    charge_enabled = 0U;
    Power_SetChargeEnable(0U);
}

/*
 * @brief 请求设置充电通路。
 * @param enable 1=请求打开，0=关闭。
 * @note CHARGE_MANAGEMENT_ENABLE=0 时只记录关闭状态并保持硬件低电平。
 */
void Charge_SetEnable(uint8_t enable)
{
#if CHARGE_MANAGEMENT_ENABLE
    charge_enabled = (enable != 0U) ? 1U : 0U;
    Power_SetChargeEnable(charge_enabled);
#else
    (void)enable;
    charge_enabled = 0U;
    Power_SetChargeEnable(0U);
#endif
}

/*
 * @brief 执行充电输入检测和充电通路控制。
 * @note 当前宏关闭，代码保留供后续按电池和充电器规格启用。
 */
void Charge_Task(void)
{
#if CHARGE_MANAGEMENT_ENABLE
    uint32_t charge_voltage_mv;

    /* CH_VIN 由 ADC 模块按 11 倍分压换算。 */
    charge_voltage_mv = AppAdc_ReadChargeInputMv();
    charge_input_present = (charge_voltage_mv >= CHARGE_INPUT_PRESENT_MV) ? 1U : 0U;

    /* 只有检测到输入且电量未满时才允许打开通路。 */
    if ((charge_input_present != 0U) && (Battery_GetPercent() < 100U))
    {
        Charge_SetEnable(1U);
    }
    else
    {
        Charge_SetEnable(0U);
    }
#else
    /* 关闭期间不读取 ADC，不自动打开 CH_EN。 */
#endif
}

/* @brief 返回当前是否检测到充电输入。 */
uint8_t Charge_IsInputPresent(void)
{
    return charge_input_present;
}

/* @brief 返回当前软件是否允许充电。 */
uint8_t Charge_IsEnabled(void)
{
    return charge_enabled;
}
