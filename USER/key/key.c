#include "key.h"
#include "platform.h"
#include "config.h"

/* K3 的物理引脚：PA12，低电平表示按下。 */
#define KEY_K3_PIN                         (GPIO_Pin_12)

/* K1 的物理引脚：PA9，低电平表示按下。 */
#define KEY_K1_PIN                         (GPIO_Pin_9)

/* K2 的物理引脚：PA10，低电平表示按下。 */
#define KEY_K2_PIN                         (GPIO_Pin_10)

typedef struct
{
    /* 当前已经确认的稳定状态，1=按下，0=释放。 */
    uint8_t stable_pressed;
    /* 原始电平发生变化后的累计确认时间。 */
    uint16_t change_time_ms;
} key_state_t;

/* 三个按键各自独立保存去抖状态。 */
static key_state_t key_k3_state;
static key_state_t key_k1_state;
static key_state_t key_k2_state;

/*
 * @brief 读取一个按键并完成去抖。
 * @param pin 按键对应的 GPIOA 引脚掩码。
 * @param state 按键状态保存结构。
 * @return 本次是否产生“释放完成”的短按事件。
 */
static uint8_t Key_ScanOne(uint16_t pin, key_state_t *state)
{
    uint8_t raw_pressed;
    uint8_t event = 0U;

    /* 原理图三个按键均由 10 kΩ 上拉，低电平表示按下。 */
    raw_pressed = (GPIO_ReadInputDataBit(GPIOA, pin) == RESET) ? 1U : 0U;

    if (raw_pressed == state->stable_pressed)
    {
        /* 原始状态与稳定状态一致，清除变化计时。 */
        state->change_time_ms = 0U;
    }
    else
    {
        /* 原始状态持续变化，达到去抖时间后才更新稳定状态。 */
        if (state->change_time_ms < KEY_DEBOUNCE_TIME_MS)
        {
            state->change_time_ms++;
        }

        if (state->change_time_ms >= KEY_DEBOUNCE_TIME_MS)
        {
            state->change_time_ms = 0U;
            if ((state->stable_pressed != 0U) && (raw_pressed == 0U))
            {
                /* 从按下变为释放，报告一次短按。 */
                event = 1U;
            }
            state->stable_pressed = raw_pressed;
        }
    }

    return event;
}

/*
 * @brief 初始化按键 GPIO。
 * @note 三个按键均使用 GPIOA 内部上拉，外部原理图还提供了 10 kΩ 上拉。
 */
void Key_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* 使能 GPIOA 时钟。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);

    /* 配置 K3、K1、K2 为上拉输入。 */
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = KEY_K3_PIN | KEY_K1_PIN | KEY_K2_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio_init);

    /* 以当前电平初始化稳定状态，避免上电时误报短按。 */
    key_k3_state.stable_pressed =
        (GPIO_ReadInputDataBit(GPIOA, KEY_K3_PIN) == RESET) ? 1U : 0U;
    key_k1_state.stable_pressed =
        (GPIO_ReadInputDataBit(GPIOA, KEY_K1_PIN) == RESET) ? 1U : 0U;
    key_k2_state.stable_pressed =
        (GPIO_ReadInputDataBit(GPIOA, KEY_K2_PIN) == RESET) ? 1U : 0U;
    key_k3_state.change_time_ms = 0U;
    key_k1_state.change_time_ms = 0U;
    key_k2_state.change_time_ms = 0U;
}

/*
 * @brief 扫描三个按键并产生短按事件。
 * @return KEY_EVENT_K3、KEY_EVENT_K1、KEY_EVENT_K2 的组合位图。
 * @note 短按在按键释放并完成去抖后报告一次。
 */
uint8_t Key_Scan(void)
{
    uint8_t events = 0U;

    if (Key_ScanOne(KEY_K3_PIN, &key_k3_state) != 0U)
    {
        events |= KEY_EVENT_K3;
    }
    if (Key_ScanOne(KEY_K1_PIN, &key_k1_state) != 0U)
    {
        events |= KEY_EVENT_K1;
    }
    if (Key_ScanOne(KEY_K2_PIN, &key_k2_state) != 0U)
    {
        events |= KEY_EVENT_K2;
    }

    return events;
}

/*
 * @brief 查询按键是否处于按下状态。
 * @param event_bit 要查询的按键事件位。
 * @return 1=按下，0=释放或参数无效。
 */
uint8_t Key_IsPressed(uint8_t event_bit)
{
    if (event_bit == KEY_EVENT_K3)
    {
        return key_k3_state.stable_pressed;
    }
    if (event_bit == KEY_EVENT_K1)
    {
        return key_k1_state.stable_pressed;
    }
    if (event_bit == KEY_EVENT_K2)
    {
        return key_k2_state.stable_pressed;
    }
    return 0U;
}
