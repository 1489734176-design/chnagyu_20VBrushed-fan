#ifndef __APP_H
#define __APP_H

#include <stdint.h>

/* 应用状态：OFF 等待开机，ON 正常工作，PROTECT 停机并等待电压恢复。 */
typedef enum
{
    APP_STATE_OFF = 0U,
    APP_STATE_ON = 1U,
    APP_STATE_PROTECT = 2U
} app_state_t;

/* 初始化应用状态和默认档位。 */
void App_Init(void);

/* 每 1 ms 执行一次按键、电机档位和电源状态处理。 */
void App_Task(void);

/* 返回当前应用状态。 */
app_state_t App_GetState(void);

#endif /* __APP_H */
