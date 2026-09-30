#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

/* K3：开关机按键，原理图网络 ON/OFF，连接 PA12。 */
#define KEY_EVENT_K3                       (0x01U)

/* K1：风扇档位按键，原理图网络 KEY1，连接 PA9。 */
#define KEY_EVENT_K1                       (0x02U)

/* K2：水泵档位按键，原理图网络 KEY2，连接 PA10。 */
#define KEY_EVENT_K2                       (0x04U)

/* 初始化三个按键的上拉输入。 */
void Key_Init(void);

/* 每 1 ms 扫描三个按键，返回本次确认的短按释放事件位图。 */
uint8_t Key_Scan(void);

/* 查询某个按键当前是否为低电平按下。 */
uint8_t Key_IsPressed(uint8_t event_bit);

#endif /* __KEY_H */
