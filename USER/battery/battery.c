#include "battery.h"
#include "config.h"
#include "adc.h"

/* 最近一次 VBus 换算结果，单位为毫伏，不再计算电量百分比。 */
static uint32_t battery_voltage_mv = 0UL;

/* 欠压、过压分别锁存；任一保护未解除，都禁止电机启动。 */
static uint8_t battery_under_voltage = 0U;
static uint8_t battery_over_voltage = 0U;

/* 无效样本不能用于电压判断，ADC 故障单独锁存并禁止电机。 */
static uint8_t battery_sample_valid = 0U;
static uint8_t battery_adc_fault = 0U;

#if BATTERY_PROTECTION_ENABLE
static uint16_t battery_adc_recover_count = 0U;
/* 每种保护独立计数：未锁存时累计异常，锁存后用作恢复倒计时。 */
static uint16_t battery_uvp_count = 0U;
static uint16_t battery_ovp_count = 0U;

/*
 * @brief 更新一路电压保护的计数和锁存状态。
 * @note 参考电钻 Volt_Handler：异常采样递增，正常采样递减，达到上限触发。
 *       已触发后，必须连续满足恢复条件才解除；中途不满足则重置恢复倒计时。
 *       每 1 ms 调用一次，计数饱和处理，避免长时间异常导致溢出。
 */
static void Battery_UpdateProtection(uint8_t fault, uint8_t recovered,
                                     uint16_t filter_ms, uint16_t *count,
                                     uint8_t *active)
{
    if (*active != 0U)
    {
        if (recovered != 0U)
        {
            if (*count > 0U)
            {
                (*count)--;
            }
            if (*count == 0U)
            {
                *active = 0U;
            }
        }
        else
        {
            *count = BATTERY_RECOVER_FILTER_TIME_MS;
        }
    }
    else if (fault != 0U)
    {
        if (*count < filter_ms)
        {
            (*count)++;
        }
        if (*count >= filter_ms)
        {
            *active = 1U;
            *count = BATTERY_RECOVER_FILTER_TIME_MS;
        }
    }
    else if (*count > 0U)
    {
        (*count)--;
    }
}

/* 失败时保留最近有效电压，但禁止把它作为新样本或连续恢复证据。 */
static uint8_t Battery_ReadVoltage(void)
{
    if (AppAdc_TryReadVbusMv(&battery_voltage_mv) == 0U)
    {
        battery_sample_valid = 0U;
        battery_adc_fault = 1U;
        battery_adc_recover_count = 0U;
        if (battery_under_voltage != 0U)
        {
            battery_uvp_count = BATTERY_RECOVER_FILTER_TIME_MS;
        }
        if (battery_over_voltage != 0U)
        {
            battery_ovp_count = BATTERY_RECOVER_FILTER_TIME_MS;
        }
        return 0U;
    }
    battery_sample_valid = 1U;
    return 1U;
}
#endif

/*
 * @brief 初始化电池电压保护。
 * @note 必须在 AppAdc_Init 后调用。参考电钻 Volt_Handler_fast，上电立即检查，
 *       不让零初值参与平均滤波，避免正常电池被误判为欠压。
 */
void Battery_Init(void)
{
    battery_voltage_mv = 0UL;
    battery_under_voltage = 0U;
    battery_over_voltage = 0U;
    battery_sample_valid = 0U;
    battery_adc_fault = 0U;
#if BATTERY_PROTECTION_ENABLE
    battery_adc_recover_count = 0U;
    battery_uvp_count = 0U;
    battery_ovp_count = 0U;
#endif
    (void)Battery_CheckBeforeStart();
}

/*
 * @brief 启动前重新采样，快速阻止异常电压下的电机启动。
 * @return 1=允许启动，0=仍有欠压或过压保护。
 * @note 这里只置位故障，绝不清除已有锁存；恢复只能由周期任务确认。
 */
uint8_t Battery_CheckBeforeStart(void)
{
#if BATTERY_PROTECTION_ENABLE
    if (Battery_ReadVoltage() == 0U)
    {
        return 0U;
    }
    if (battery_voltage_mv <= BATTERY_UVP_VOLTAGE_MV)
    {
        battery_under_voltage = 1U;
        battery_uvp_count = BATTERY_RECOVER_FILTER_TIME_MS;
    }
    if (battery_voltage_mv >= BATTERY_OVP_VOLTAGE_MV)
    {
        battery_over_voltage = 1U;
        battery_ovp_count = BATTERY_RECOVER_FILTER_TIME_MS;
    }
#endif
    return (Battery_IsProtected() == 0U) ? 1U : 0U;
}

/*
 * @brief 每 1 ms 执行一次欠压/过压检测及恢复滤波。
 * @note 欠压恢复使用 15.5 V 迟滞；过压恢复沿用参考工程的低于过压阈值判断。
 *       本模块只提供保护状态，实际电机停机由应用层统一执行。
 */
void Battery_Task(void)
{
#if BATTERY_PROTECTION_ENABLE
    if (Battery_ReadVoltage() == 0U)
    {
        return;
    }
    if (battery_adc_fault != 0U)
    {
        if (battery_adc_recover_count < BATTERY_ADC_RECOVER_TICKS)
        {
            battery_adc_recover_count++;
        }
        if (battery_adc_recover_count >= BATTERY_ADC_RECOVER_TICKS)
        {
            battery_adc_fault = 0U;
        }
    }
    Battery_UpdateProtection(
        (battery_voltage_mv <= BATTERY_UVP_VOLTAGE_MV) ? 1U : 0U,
        (battery_voltage_mv >= BATTERY_UVP_RECOVER_VOLTAGE_MV) ? 1U : 0U,
        BATTERY_UVP_FILTER_TIME_MS, &battery_uvp_count, &battery_under_voltage);
    Battery_UpdateProtection(
        (battery_voltage_mv >= BATTERY_OVP_VOLTAGE_MV) ? 1U : 0U,
        (battery_voltage_mv < BATTERY_OVP_VOLTAGE_MV) ? 1U : 0U,
        BATTERY_OVP_FILTER_TIME_MS, &battery_ovp_count, &battery_over_voltage);
#else
    /* 功能关闭时不采样，欠压/过压状态保持为 0。 */
#endif
}

/* @brief 返回最近一次 VBus 电压，单位为毫伏。 */
uint32_t Battery_GetVoltageMv(void)
{
    return battery_voltage_mv;
}

/* @brief 返回欠压锁存状态。 */
uint8_t Battery_IsUnderVoltage(void)
{
    return battery_under_voltage;
}

/* @brief 返回过压锁存状态。 */
uint8_t Battery_IsOverVoltage(void)
{
    return battery_over_voltage;
}

/* @brief 返回汇总电压保护状态。 */
uint8_t Battery_IsProtected(void)
{
    return ((battery_under_voltage != 0U) || (battery_over_voltage != 0U) ||
            (battery_adc_fault != 0U)) ? 1U : 0U;
}

uint8_t Battery_IsSampleValid(void)
{
    return battery_sample_valid;
}

uint8_t Battery_IsAdcFault(void)
{
    return battery_adc_fault;
}
