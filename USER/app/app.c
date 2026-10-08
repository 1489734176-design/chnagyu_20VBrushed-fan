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
 * 上电开机时 K3 仍被按住，这一次按压的“释放”不能当作关机。
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

    /* 开机后立即按默认档位刷新指示灯。 */
    App_UpdateGearLeds();
}

/*
 * @brief 初始化应用。
 * @note 硬件为自锁存电源：不按 K3 时整机断电、功耗为 0；按下 K3 才给 MCU 上电。
 *       因此 MCU 一旦运行到这里，就代表用户刚按 K3 开机，直接进入开机状态、风扇立即转。
 *       上电那一次 K3 仍处于按下状态，需要忽略它的释放事件，避免刚开机就被当作关机。
 */
void App_Init(void)
{
    /* 关闭可选管理模块，Power_Init 已拉高 EN 完成自锁存保持。 */
    Charge_SetEnable(0U);
    Battery_Init();
    Charge_Init();

    /* 若上电时 K3 仍被按住，则忽略这一次按压的释放事件。 */
    app_ignore_first_k3 = (Key_IsPressed(KEY_EVENT_K3) != 0U) ? 1U : 0U;

    /* 上电即开机：风扇 1 档直接转，水泵 0 档，点亮对应指示灯。 */
//    App_PowerOn();
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
        if (app_ignore_first_k3 != 0U)
        {
            /* 忽略上电开机那一次按压的释放，保持开机状态、风扇继续转。 */
            app_ignore_first_k3 = 0U;
        }
        else if (app_state == APP_STATE_ON)
        {
            /* 开机状态再按 K3：关机并释放 EN，整机断电、功耗回到 0。 */
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
