#include "charge.h"
#include "config.h"
#include "adc.h"
#include "power.h"
#include "battery.h"

static charge_input_state_t charge_input_state = CHARGE_INPUT_UNKNOWN;
static uint8_t charge_input_sample_valid = 0U;
static uint32_t charge_input_voltage_mv = 0UL;
static uint8_t charge_enabled = 0U;
static uint8_t charge_voltage_stopped = 0U;
#if CHARGE_MANAGEMENT_ENABLE && CHARGE_INPUT_DETECTION_ENABLE && BATTERY_PROTECTION_ENABLE
static uint8_t charge_power_available = 0U;
#endif

#if CHARGE_INPUT_DETECTION_ENABLE
/* 连续确认计数，门限之间保留已确认状态，启动/故障后需重新确认。 */
static uint16_t charge_insert_count = 0U;
static uint16_t charge_remove_count = 0U;
#endif

/* 只在输出变化时写 GPIO，避免正常充电每拍先低后高。 */
static void Charge_WriteOutput(uint8_t enable)
{
    if (charge_enabled != enable)
    {
        Power_SetChargeEnable(enable);
        charge_enabled = enable;
    }
}

static uint8_t Charge_IsAllowed(void)
{
#if CHARGE_MANAGEMENT_ENABLE && CHARGE_INPUT_DETECTION_ENABLE && BATTERY_PROTECTION_ENABLE
    /* 启动快速采样也可能更新电压；按最新样本判断，回落后不锁存截止。 */
    charge_voltage_stopped = ((Battery_IsSampleValid() != 0U) &&
                              (Charge_IsInputAbsent() == 0U) &&
                              (Battery_GetVoltageMv() >= CHARGE_STOP_VOLTAGE_MV)) ? 1U : 0U;
    return ((charge_power_available != 0U) &&
            (charge_input_state == CHARGE_INPUT_PRESENT) &&
            (charge_input_sample_valid != 0U) &&
            (charge_input_voltage_mv > CHARGE_INPUT_INSERT_MV) &&
            (Battery_IsSampleValid() != 0U) && (Battery_IsAdcFault() == 0U) &&
            (Battery_IsOverVoltage() == 0U) &&
            (charge_voltage_stopped == 0U)) ? 1U : 0U;
#else
    return 0U;
#endif
}

void Charge_Init(void)
{
#if CHARGE_INPUT_DETECTION_ENABLE
    charge_input_state = CHARGE_INPUT_UNKNOWN;
    charge_insert_count = 0U;
    charge_remove_count = 0U;
#else
    charge_input_state = CHARGE_INPUT_DISABLED;
#endif
    charge_input_sample_valid = 0U;
    charge_input_voltage_mv = 0UL;
    charge_enabled = 0U;
    charge_voltage_stopped = 0U;
#if CHARGE_MANAGEMENT_ENABLE && CHARGE_INPUT_DETECTION_ENABLE && BATTERY_PROTECTION_ENABLE
    charge_power_available = 0U;
#endif
    Power_SetChargeEnable(0U);
}

void Charge_SetEnable(uint8_t enable)
{
    if (enable == 0U)
    {
        Charge_WriteOutput(0U);
    }
    else
    {
        Charge_WriteOutput(Charge_IsAllowed());
    }
}

/* 只在应用完成启动检查和电源决策后，才允许由关闭切换为打开。 */
void Charge_UpdateOutput(uint8_t power_available)
{
#if CHARGE_MANAGEMENT_ENABLE && CHARGE_INPUT_DETECTION_ENABLE && BATTERY_PROTECTION_ENABLE
    charge_power_available = (power_available != 0U) ? 1U : 0U;
#else
    (void)power_available;
#endif
    Charge_SetEnable(1U);
}

void Charge_Task(void)
{
#if CHARGE_INPUT_DETECTION_ENABLE
    if (AppAdc_TryReadChargeInputMv(&charge_input_voltage_mv) == 0U)
    {
        charge_input_sample_valid = 0U;
        charge_input_state = CHARGE_INPUT_FAULT;
        charge_insert_count = 0U;
        charge_remove_count = 0U;
        Charge_SetEnable(0U);
        return;
    }
    charge_input_sample_valid = 1U;

    if (charge_input_voltage_mv > CHARGE_INPUT_INSERT_MV)
    {
        charge_remove_count = 0U;
        if (charge_insert_count < CHARGE_INPUT_FILTER_TICKS)
        {
            charge_insert_count++;
        }
        if (charge_insert_count >= CHARGE_INPUT_FILTER_TICKS)
        {
            charge_input_state = CHARGE_INPUT_PRESENT;
        }
    }
    else if (charge_input_voltage_mv <= CHARGE_INPUT_REMOVE_MV)
    {
        charge_insert_count = 0U;
        if (charge_remove_count < CHARGE_INPUT_FILTER_TICKS)
        {
            charge_remove_count++;
        }
        if (charge_remove_count >= CHARGE_INPUT_FILTER_TICKS)
        {
            charge_input_state = CHARGE_INPUT_ABSENT;
        }
    }
    else
    {
        charge_insert_count = 0U;
        charge_remove_count = 0U;
    }
#endif
    /* 低输入/无效样本/截止立即关断，不等待按键处理，更不先开再检查。 */
    if (Charge_IsAllowed() == 0U)
    {
        Charge_SetEnable(0U);
    }
}

charge_input_state_t Charge_GetInputState(void)
{
    return charge_input_state;
}

uint8_t Charge_IsInputPresent(void)
{
    return (charge_input_state == CHARGE_INPUT_PRESENT) ? 1U : 0U;
}

uint8_t Charge_IsInputAbsent(void)
{
#if CHARGE_INPUT_DETECTION_ENABLE
    return ((charge_input_state == CHARGE_INPUT_ABSENT) &&
            (charge_input_sample_valid != 0U) &&
            (charge_remove_count >= CHARGE_INPUT_FILTER_TICKS)) ? 1U : 0U;
#else
    return 0U;
#endif
}

uint8_t Charge_IsInputSampleValid(void)
{
    return charge_input_sample_valid;
}

uint32_t Charge_GetInputVoltageMv(void)
{
    return charge_input_voltage_mv;
}

uint8_t Charge_IsEnabled(void)
{
    return charge_enabled;
}

uint8_t Charge_IsVoltageStopped(void)
{
    return charge_voltage_stopped;
}
