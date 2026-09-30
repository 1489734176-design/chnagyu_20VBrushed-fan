#ifndef __APP_H
#define __APP_H

#include <stdint.h>

/* 应用状态：ON 为正常工作，OFF 为关机安全状态。 */
typedef enum
{
    APP_STATE_OFF = 0U,
    APP_STATE_ON = 1U
} app_state_t;

/* 初始化应用状态和默认档位。 */
void App_Init(void);

/* 每 1 ms 执行一次按键、电机档位和电源状态处理。 */
void App_Task(void);

/* 返回当前应用状态。 */
app_state_t App_GetState(void);

#endif /* __APP_H */
