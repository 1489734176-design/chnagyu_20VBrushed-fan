#include "app.h"
#include "key.h"
#include "led.h"
#include "motor.h"
#include "power.h"
#include "battery.h"
#include "charge.h"

/* 当前应用状态，复位后先停机等待 K3，异常电压则进入保护状态。 */
static app_state_t app_state = APP_STATE_OFF;

/* 当前风扇档位，开机默认 1 档。 */
static uint8_t fan_gear = MOTOR_FAN_GEAR_MIN;

/* 当前水泵档位，开机默认 0 档。 */
static uint8_t pump_gear = MOTOR_PUMP_GEAR_MIN;

/*
 * 上电时 K3 仍可能被按住，这一次按压的释放不能当作新的开关机指令。
 * 1 表示还需忽略一次 K3 事件（即上电那次按压的释放）。
 */
static uint8_t app_ignore_first_k3 = 0U;

/*
 * @brief 根据当前风扇和水泵档位刷新指示灯。
 * @note 风扇 1/2/3 档分别点亮 LED6/LED5/LED4；水泵 1/2/3 档分别点亮 LED3/LED2/LED1；
 *       水泵 0 档不点亮任何水泵灯。掩码 bit0~bit5 对应 LED1~LED6。
 */
static void App_UpdateGearLeds(void)
{
    uint8_t led_mask = 0U;

    /* 风扇档位与指示灯一一对应，档位越高灯越靠前。 */
    switch (fan_gear)
    {
        case 1U:
            led_mask |= (uint8_t)(1U << LED_ID_6);
            break;
        case 2U:
            led_mask |= (uint8_t)(1U << LED_ID_5);
            break;
        case 3U:
            led_mask |= (uint8_t)(1U << LED_ID_4);
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
 * @brief 执行应用关机流程。
 * @note 先停电机和 LED，再禁止充电，最后释放 EN，避免关机过程中仍有驱动输出。
 */
static void App_PowerOff(void)
{
    Motor_StopAll();
    Charge_SetEnable(0U);
    Led_AllOff();
    app_state = APP_STATE_OFF;
    Power_SafePowerOff();
}

/*
 * @brief 电压保护停机，不释放 EN，以便继续采样并判断恢复条件。
 * @note 停止风扇、水泵和指示灯；用户仍可按 K3 真正关机。
 */
static void App_EnterProtection(void)
{
    Motor_StopAll();
    Charge_SetEnable(0U);
    Led_AllOff();
    app_state = APP_STATE_PROTECT;
}

/*
 * @brief 执行应用开机流程。
 * @note 启动前快速检查当前电压，已有保护锁存只能由 Battery_Task 滤波解除。
 */
static void App_PowerOn(void)
{
    if (Battery_CheckBeforeStart() == 0U)
    {
        App_EnterProtection();
        return;
    }

    Power_SetKeepAlive(1U);
    Charge_SetEnable(0U);
    fan_gear = MOTOR_FAN_GEAR_MIN;
    pump_gear = MOTOR_PUMP_GEAR_MIN;
    Motor_SetFanGear(fan_gear);
    Motor_SetPumpGear(pump_gear);
    app_state = APP_STATE_ON;

    /* 开机后立即按默认档位刷新指示灯。 */
    App_UpdateGearLeds();
}

/*
 * @brief 初始化应用，保持当前上电不自动启动电机的行为。
 * @note Power_Init 已接管 EN；main 中已经完成电池快速检查和充电初始化，
 *       这里不能再次初始化电池，否则会丢失已经锁存的保护状态。
 */
void App_Init(void)
{
    Motor_StopAll();
    Charge_SetEnable(0U);
    Led_AllOff();
    app_state = APP_STATE_OFF;
    fan_gear = MOTOR_FAN_GEAR_MIN;
    pump_gear = MOTOR_PUMP_GEAR_MIN;

    /* 若上电时 K3 仍被按住，则忽略这一次按压的释放事件。 */
    app_ignore_first_k3 = (Key_IsPressed(KEY_EVENT_K3) != 0U) ? 1U : 0U;

    if (Battery_IsProtected() != 0U)
    {
        App_EnterProtection();
    }
}

/*
 * @brief 处理一个 1 ms 应用周期。
 * @note 电压保护优先于档位控制；K3 可在运行或保护状态下关机。
 */
void App_Task(void)
{
    uint8_t key_events;

    /* 先检测电压，再处理按键，防止保护触发当拍仍调整电机输出。 */
    Battery_Task();
    Charge_Task();
    key_events = Key_Scan();

    if (((key_events & KEY_EVENT_K3) != 0U) && (app_ignore_first_k3 != 0U))
    {
        /* 上电那次 K3 释放不作为新指令，但本周期仍须执行电压保护。 */
        app_ignore_first_k3 = 0U;
        key_events = 0U;
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

/* @brief 返回当前应用状态。 */
app_state_t App_GetState(void)
{
    return app_state;
}
