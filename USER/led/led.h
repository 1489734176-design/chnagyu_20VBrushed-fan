#ifndef __LED_H
#define __LED_H

#include <stdint.h>

/* 六个 LED 的逻辑编号，掩码 bit0~bit5 分别对应 LED1~LED6。 */
#define LED_ID_1                           (0U)
#define LED_ID_2                           (1U)
#define LED_ID_3                           (2U)
#define LED_ID_4                           (3U)
#define LED_ID_5                           (4U)
#define LED_ID_6                           (5U)
#define LED_MASK_ALL                       (0x3FU)

/* 初始化六个静态 LED 输出，默认全部熄灭。 */
void Led_Init(void);

/* 设置单个 LED，on=1 点亮，on=0 熄灭。 */
void Led_Set(uint8_t led_id, uint8_t on);

/* 通过六位掩码设置 LED，bit=1 表示点亮。 */
void Led_SetMask(uint8_t led_mask);

/* 关闭全部 LED。 */
void Led_AllOff(void);

#endif /* __LED_H */
