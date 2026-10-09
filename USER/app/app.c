#include "app.h"
#include "key.h"
#include "led.h"
#include "motor.h"
#include "power.h"
#include "battery.h"
#include "charge.h"
#include "adc.h"
#include "config.h"
#include "hal_flash.h"

/* Keil IROM 长度须为 0x3C00，将最后 1 KB 擦除页留给档位记录。 */
#define APP_FAN_MEMORY_START_ADDR          (0x08003C00UL)
#define APP_FAN_MEMORY_END_ADDR            (0x08004000UL)

/* K3 上电直接启动，其他上电保持待机；异常电压进入保护状态。 */
static app_state_t app_state = APP_STATE_OFF;

/* 无有效记录时默认 1 档，正常开机沿用上次关机档位。 */
static uint8_t fan_gear = MOTOR_FAN_GEAR_MIN;
static uint8_t fan_saved_gear = MOTOR_FAN_GEAR_MIN;
static uint32_t fan_memory_next_addr = APP_FAN_MEMORY_START_ADDR;

/* 当前水泵档位，开机默认 0 档。 */
static uint8_t pump_gear = MOTOR_PUMP_GEAR_MIN;

/* 上电按压已用于启动，忽略其释放，避免刚开机又关机。 */
static uint8_t app_ignore_first_k3 = 0U;

/* 用户逻辑关机后只为外部输入保电；电源已释放时不反复执行关断。 */
static uint8_t app_power_off_requested = 0U;
static uint8_t app_power_released = 0U;
static uint16_t fan_stall_count = 0U;
static uint8_t app_motor_fault_shutdown = 0U;

static void App_LoadFanGear(void)
{
    uint32_t address;
    uint16_t record;
    uint8_t gear;

    fan_saved_gear = MOTOR_FAN_GEAR_MIN;
    fan_memory_next_addr = APP_FAN_MEMORY_START_ADDR;
    for (address = APP_FAN_MEMORY_START_ADDR; address < APP_FAN_MEMORY_END_ADDR; address += 2UL)
    {
        record = *(volatile const uint16_t *)address;
        if (record != 0xFFFFU)
        {
            /* 跳过断电留下的不完整记录，避免再次编程已使用的半字。 */
            fan_memory_next_addr = address + 2UL;
            gear = (uint8_t)record;
            if ((gear >= MOTOR_FAN_GEAR_MIN) && (gear <= MOTOR_FAN_GEAR_MAX) &&
                ((uint8_t)(record >> 8) == (uint8_t)(gear ^ 0xFFU)))
            {
                fan_saved_gear = gear;
            }
        }
    }
    fan_gear = fan_saved_gear;
}

static void App_SaveFanGear(void)
{
    uint32_t primask;
    uint16_t record;
    FLASH_Status status = FLASH_COMPLETE;

    if (fan_gear == fan_saved_gear)
    {
        return;
    }

    /* Flash 操作会暂停采样，先关充电；此时电机已停且 EN 尚未释放。 */
    Charge_SetEnable(0U);
    record = (uint16_t)((uint16_t)fan_gear | ((uint16_t)(fan_gear ^ 0xFFU) << 8));
    primask = __get_PRIMASK();
    __disable_irq();
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /* 追加半字记录，整页写满后才擦除，减少 Flash 擦写次数。 */
    if (fan_memory_next_addr >= APP_FAN_MEMORY_END_ADDR)
    {
        status = FLASH_ErasePage(APP_FAN_MEMORY_START_ADDR);
        if (status == FLASH_COMPLETE)
        {
            fan_memory_next_addr = APP_FAN_MEMORY_START_ADDR;
        }
    }
    if (status == FLASH_COMPLETE)
    {
        status = FLASH_ProgramHalfWord(fan_memory_next_addr, record);
        if ((status == FLASH_COMPLETE) &&
            (*(volatile const uint16_t *)fan_memory_next_addr == record))
        {
            fan_saved_gear = fan_gear;
        }
        fan_memory_next_addr += 2UL;
    }

    FLASH_Lock();
    __set_PRIMASK(primask);
}

static void App_UpdateGearLeds(void)
{
    uint8_t led_mask = 0U;

    switch (fan_gear)
    {
        case 1U:
            led_mask |= (uint8_t)(1U << LED_ID_6);
            break;
        case 2U:
            led_mask |= (uint8_t)((1U << LED_ID_6) | (1U << LED_ID_5));
            break;
        case 3U:
            led_mask |= (uint8_t)((1U << LED_ID_6) | (1U << LED_ID_5) | (1U << LED_ID_4));
            break;
        default:
            break;
    }

    /* 水泵 0 档不亮灯，1/2/3 档依次点亮 LED3/LED2/LED1。 */
    switch (pump_gear)
    {
        case 1U:
            led_mask |= (uint8_t)(1U << LED_ID_3);
            break;
        case 2U:
            led_mask |= (uint8_t)(1U << LED_ID_2);
            break;
        case 3U:
            led_mask |= (uint8_t)(1U << LED_ID_1);
            break;
        default:
            break;
    }

    Led_SetMask(led_mask);
}

/*
 * @brief 用户关机后，确认外部输入已拔出才释放电源。
 * @note 未确认或 ADC 故障保持监测，不能把输入状态不可信当作拔出。
 */
static void App_UpdatePowerOff(void)
{
    charge_input_state_t input_state = Charge_GetInputState();

    if ((app_power_off_requested == 0U) || (app_power_released != 0U))
    {
        return;
    }
    if ((Charge_IsInputAbsent() != 0U) ||
        (input_state == CHARGE_INPUT_DISABLED))
    {
        Charge_UpdateOutput(0U);
        Power_SafePowerOff();
        app_power_released = 1U;
    }
    else
    {
        Power_SetKeepAlive(1U);
    }
}

/* 插电时停电机和灯，保存档位后保留监测及符合条件的充电。 */
static void App_PowerOff(void)
{
    Motor_StopAll();
    Led_AllOff();
    App_SaveFanGear();
    app_state = APP_STATE_OFF;
    app_power_off_requested = 1U;
    app_power_released = 0U;
    App_UpdatePowerOff();
    fan_stall_count = 0U;
}

static void App_CheckFanStall(void)
{
    uint32_t current_ma;
    uint8_t sample_valid;

    if ((app_state != APP_STATE_ON) || (Motor_GetFanGear() == 0U))
    {
        fan_stall_count = 0U;
        return;
    }

    sample_valid = AppAdc_TryReadCurrentMa(&current_ma);
    if (sample_valid != 0U)
    {
        if (current_ma <= FAN_STALL_CURRENT_MA)
        {
            fan_stall_count = 0U;
            return;
        }
        if (fan_stall_count < FAN_STALL_FILTER_TIME_MS)
        {
            fan_stall_count++;
        }
        if (fan_stall_count < FAN_STALL_FILTER_TIME_MS)
        {
            return;
        }
    }

    /* 采样故障同样关断；外部供电未掉电时禁止按键或充电逻辑重新保电。 */
    app_motor_fault_shutdown = 1U;
    Motor_StopAll();
    Led_AllOff();
    Charge_UpdateOutput(0U);
    App_SaveFanGear();
    app_state = APP_STATE_OFF;
    app_power_off_requested = 1U;
    app_power_released = 1U;
    fan_stall_count = 0U;
    Power_SafePowerOff();
}

/*
 * @brief 电压保护停机，不释放 EN，以便继续采样并判断恢复条件。
 * @note 停止风扇、水泵和指示灯；仅放电欠压不禁止充电，ADC/过压由充电模块阻断。
 */
static void App_EnterProtection(void)
{
    Motor_StopAll();
    Led_AllOff();
    app_state = APP_STATE_PROTECT;
    fan_stall_count = 0U;
}

/*
 * @brief 执行应用开机流程。
 * @note 启动前快速检查当前电压，已有保护锁存只能由 Battery_Task 滤波解除。
 */
static void App_PowerOn(void)
{
    fan_stall_count = 0U;
    app_power_off_requested = 0U;
    app_power_released = 0U;
    Power_SetKeepAlive(1U);
    if (Battery_CheckBeforeStart() == 0U)
    {
        App_EnterProtection();
        return;
    }

    pump_gear = MOTOR_PUMP_GEAR_MIN;
    Motor_SetFanGear(fan_gear);
    Motor_SetPumpGear(pump_gear);
    app_state = APP_STATE_ON;

    App_UpdateGearLeds();
}

/* Battery_Init 已完成，不能重新初始化并清除已有保护锁存。 */
void App_Init(void)
{
    fan_stall_count = 0U;
    app_motor_fault_shutdown = 0U;
    Power_SetKeepAlive(1U);
    Charge_UpdateOutput(0U);
    app_power_off_requested = 0U;
    app_power_released = 0U;
    Motor_StopAll();
    Led_AllOff();
    app_state = APP_STATE_OFF;
    App_LoadFanGear();
    pump_gear = MOTOR_PUMP_GEAR_MIN;

    app_ignore_first_k3 = (Key_IsPressed(KEY_EVENT_K3) != 0U) ? 1U : 0U;

    if (app_ignore_first_k3 != 0U)
    {
        App_PowerOn();
    }
    else if (Battery_IsProtected() != 0U)
    {
        App_EnterProtection();
    }
}

/*
 * @brief 处理一个 1 ms 应用周期。
 * @note 电压保护优先于档位控制；K3 可在运行或保护状态下关机。
 */
static void App_ProcessTask(void)
{
    uint8_t key_events;

    if (app_motor_fault_shutdown != 0U)
    {
        return;
    }

    /* 先检测电压，再处理按键，防止保护触发当拍仍调整电机输出。 */
    Battery_Task();
    Charge_Task();
    App_CheckFanStall();
    if (app_motor_fault_shutdown != 0U)
    {
        return;
    }
    key_events = Key_Scan();

    if (((key_events & KEY_EVENT_K3) != 0U) && (app_ignore_first_k3 != 0U))
    {
        /* 上电那次 K3 释放不作为新指令，但本周期仍须执行电压保护。 */
        app_ignore_first_k3 = 0U;
        key_events = 0U;
    }

    /* 用户关机优先于重新进入保护；监测继续，K3 仍可尝试开机。 */
    if (app_power_off_requested != 0U)
    {
        /* 若硬件仍有供电，新一轮确认接入可重新保电，仍不自动启动电机。 */
        if ((app_power_released != 0U) && (Charge_IsInputPresent() != 0U))
        {
            Power_SetKeepAlive(1U);
            app_power_released = 0U;
        }
        if ((key_events & KEY_EVENT_K3) != 0U)
        {
            App_PowerOn();
        }
        else
        {
            App_UpdatePowerOff();
        }
        return;
    }

    if (Battery_IsProtected() != 0U)
    {
        if (app_state != APP_STATE_PROTECT)
        {
            App_EnterProtection();
        }
    }
    else if (app_state == APP_STATE_PROTECT)
    {
        /* 恢复只解除停机锁定，不自动转动；丢弃当拍按键，等待新的 K3 操作。 */
        app_state = APP_STATE_OFF;
        return;
    }

    /* K3 优先于档位键；保护状态也允许释放 EN 进行真正关机。 */
    if ((key_events & KEY_EVENT_K3) != 0U)
    {
        if ((app_state == APP_STATE_ON) || (app_state == APP_STATE_PROTECT))
        {
            App_PowerOff();
        }
        else
        {
            App_PowerOn();
        }
        return;
    }

    /* 关机和保护状态均不响应档位按键。 */
    if (app_state != APP_STATE_ON)
    {
        return;
    }

    /* K1 短按循环风扇 1 档、2 档、3 档，然后回到 1 档。 */
    if ((key_events & KEY_EVENT_K1) != 0U)
    {
        fan_gear++;
        if (fan_gear > MOTOR_FAN_GEAR_MAX)
        {
            fan_gear = MOTOR_FAN_GEAR_MIN;
        }
        Motor_SetFanGear(fan_gear);
        App_UpdateGearLeds();
    }

    /* K2 短按循环水泵 0 档、1 档、2 档、3 档，然后回到 0 档。 */
    if ((key_events & KEY_EVENT_K2) != 0U)
    {
        pump_gear++;
        if (pump_gear > MOTOR_PUMP_GEAR_MAX)
        {
            pump_gear = MOTOR_PUMP_GEAR_MIN;
        }
        Motor_SetPumpGear(pump_gear);
        App_UpdateGearLeds();
    }
}

/* 所有按键/保护分支结束后统一决策，快速启动采样的异常也在当拍关闭。 */
void App_Task(void)
{
    App_ProcessTask();
    Charge_UpdateOutput((app_power_released == 0U) ? 1U : 0U);
}

/* @brief 返回当前应用状态。 */
app_state_t App_GetState(void)
{
    return app_state;
}
