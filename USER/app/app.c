#include "app.h"
#include "key.h"
#include "led.h"
#include "motor.h"
#include "power.h"
#include "battery.h"
#include "charge.h"

/* 当前应用状态，复位后由 App_Init 设置为开机状态。 */
static app_state_t app_state = APP_STATE_OFF;

/* 当前风扇档位，开机默认 1 档。 */
static uint8_t fan_gear = MOTOR_FAN_GEAR_MIN;

/* 当前水泵档位，开机默认 0 档。 */
static uint8_t pump_gear = MOTOR_PUMP_GEAR_MIN;

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
 * @brief 执行应用开机流程。
 * @note K3 由硬件先启动电源，软件再拉高 EN 并恢复用户要求的默认档位。
 */
static void App_PowerOn(void)
{
    Power_SetKeepAlive(1U);
    Charge_SetEnable(0U);
    fan_gear = MOTOR_FAN_GEAR_MIN;
    pump_gear = MOTOR_PUMP_GEAR_MIN;
    Motor_SetFanGear(fan_gear);
    Motor_SetPumpGear(pump_gear);
    app_state = APP_STATE_ON;
}

/*
 * @brief 初始化应用。
 * @note 上电后默认开机，风扇为 1 档，水泵为 0 档。
 */
void App_Init(void)
{
    /* 先保持电源，再把可选管理模块置于关闭状态。 */
    Power_SetKeepAlive(1U);
    Charge_SetEnable(0U);
    Battery_Init();
    Charge_Init();

    /* 清除全部指示灯，避免在未定义业务映射前误显示档位。 */
    Led_AllOff();

    /* 写入用户指定的开机默认档位。 */
    fan_gear = MOTOR_FAN_GEAR_MIN;
    pump_gear = MOTOR_PUMP_GEAR_MIN;
    Motor_SetFanGear(fan_gear);
    Motor_SetPumpGear(pump_gear);
    app_state = APP_STATE_ON;
}

/*
 * @brief 处理一个 1 ms 应用周期。
 * @note K3 优先级最高；K1 只改变风扇，K2 只改变水泵，关机状态忽略档位键。
 */
void App_Task(void)
{
    uint8_t key_events;

    /* 电量和充电代码已写好，但当前两个宏关闭，不会影响主状态机。 */
    Battery_Task();
    Charge_Task();
    key_events = Key_Scan();

    /* K3 短按在开关机之间切换，优先于同一周期的其他按键。 */
    if ((key_events & KEY_EVENT_K3) != 0U)
    {
        if (app_state == APP_STATE_ON)
        {
            App_PowerOff();
        }
        else
        {
            App_PowerOn();
        }
        return;
    }

    /* 关机状态只等待 K3，不响应档位按键。 */
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
    }
}

/* @brief 返回当前应用状态。 */
app_state_t App_GetState(void)
{
    return app_state;
}
