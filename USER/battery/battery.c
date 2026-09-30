#include "battery.h"
#include "config.h"
#include "adc.h"

/* 当前保存的电量百分比，功能关闭时不参与应用控制。 */
static uint8_t battery_percent = 0U;

/* 最近一次 VBus 换算结果，单位为毫伏。 */
static uint32_t battery_voltage_mv = 0UL;

/* 欠压锁存状态，功能关闭时始终为 0。 */
static uint8_t battery_under_voltage = 0U;

/*
 * @brief 初始化电量管理变量。
 * @note 电量管理关闭时不读取 ADC，也不触发欠压动作。
 */
void Battery_Init(void)
{
    battery_percent = 0U;
    battery_voltage_mv = 0UL;
    battery_under_voltage = 0U;
}

#if BATTERY_MANAGEMENT_ENABLE
/*
 * @brief 根据 VBus 电压线性计算一个基础电量百分比。
 * @param voltage_mv VBus 毫伏值。
 * @return 0~100 的电量百分比。
 * @note 真正产品版本应根据电芯放电曲线替换为标定表。
 */
static uint8_t Battery_ConvertPercent(uint32_t voltage_mv)
{
    uint32_t percent;

    if (voltage_mv <= BATTERY_EMPTY_VOLTAGE_MV)
    {
        return 0U;
    }
    if (voltage_mv >= BATTERY_FULL_VOLTAGE_MV)
    {
        return 100U;
    }

    percent = (voltage_mv - BATTERY_EMPTY_VOLTAGE_MV) * 100UL;
    percent /= (BATTERY_FULL_VOLTAGE_MV - BATTERY_EMPTY_VOLTAGE_MV);
    return (uint8_t)percent;
}
#endif

/*
 * @brief 执行电量采样和简单滤波。
 * @note BATTERY_MANAGEMENT_ENABLE=0 时保留代码但整个任务为空操作。
 */
void Battery_Task(void)
{
#if BATTERY_MANAGEMENT_ENABLE
    uint32_t new_voltage_mv;
    uint8_t new_percent;

    /* 读取 VBus，VBus 的分压倍率由 ADC 模块统一处理。 */
    new_voltage_mv = AppAdc_ReadVbusMv();
    battery_voltage_mv = (battery_voltage_mv * 3UL + new_voltage_mv) / 4UL;
    new_percent = Battery_ConvertPercent(battery_voltage_mv);
    battery_percent = (uint8_t)(((uint16_t)battery_percent * 3U + new_percent) / 4U);
    battery_under_voltage = (battery_voltage_mv <= BATTERY_UVP_VOLTAGE_MV) ? 1U : 0U;
#else
    /* 功能关闭期间不采样、不改变应用状态，也不控制电机。 */
#endif
}

/* @brief 返回最近一次电量百分比。 */
uint8_t Battery_GetPercent(void)
{
    return battery_percent;
}

/* @brief 返回最近一次 VBus 电压，单位为毫伏。 */
uint32_t Battery_GetVoltageMv(void)
{
    return battery_voltage_mv;
}

/* @brief 返回欠压状态。 */
uint8_t Battery_IsUnderVoltage(void)
{
    return battery_under_voltage;
}
