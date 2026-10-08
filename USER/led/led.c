#include "led.h"
#include "platform.h"

/* LED1~LED6 的实际 GPIO 映射，全部为 PA 端口。 */
#define LED1_PIN                           (GPIO_Pin_11)
#define LED2_PIN                           (GPIO_Pin_1)
#define LED3_PIN                           (GPIO_Pin_0)
#define LED4_PIN                           (GPIO_Pin_7)
#define LED5_PIN                           (GPIO_Pin_8)
#define LED6_PIN                           (GPIO_Pin_3)

/* 逻辑 LED 编号到物理 GPIO 掩码的查找表。 */
static const uint16_t led_pin_table[6] =
{
    LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN, LED5_PIN, LED6_PIN
};

/*
 * @brief 初始化六个独立 LED。
 * @note LED 阳极通过 1 kΩ 接 +5V，因此 MCU 输出低电平点亮，高电平熄灭。
 */
void Led_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* 使能 GPIOA 时钟，先将所有 LED 预置为高电平。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);
    GPIO_SetBits(GPIOA, LED1_PIN | LED2_PIN | LED3_PIN |
                        LED4_PIN | LED5_PIN | LED6_PIN);

    /* 配置六个 LED 为推挽输出。 */
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = LED1_PIN | LED2_PIN | LED3_PIN |
                         LED4_PIN | LED5_PIN | LED6_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_High;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio_init);
}

/*
 * @brief 设置单个 LED。
 * @param led_id LED1~LED6 的逻辑编号。
 * @param on 1 点亮，0 熄灭。
 */
void Led_Set(uint8_t led_id, uint8_t on)
{
    uint16_t pin;

    if (led_id >= 6U)
    {
        return;
    }

    pin = led_pin_table[led_id];
    if (on != 0U)
    {
        /* 低电平灌电流，LED 点亮。 */
        GPIO_ResetBits(GPIOA, pin);
    }
    else
    {
        /* 高电平关闭 LED。 */
        GPIO_SetBits(GPIOA, pin);
    }
}

/*
 * @brief 按六位掩码设置所有 LED。
 * @param led_mask bit0~bit5 对应 LED1~LED6，1=点亮。
 */
void Led_SetMask(uint8_t led_mask)
{
	
    uint8_t led_id;

    for (led_id = 0U; led_id < 6U; led_id++)
    {
        Led_Set(led_id, (led_mask & (uint8_t)(1U << led_id)) != 0U);
    }
}

/* @brief 关闭全部 LED，统一用于初始化和关机安全路径。 */
void Led_AllOff(void)
{
    GPIO_SetBits(GPIOA, LED1_PIN | LED2_PIN | LED3_PIN |
                        LED4_PIN | LED5_PIN | LED6_PIN);
}
