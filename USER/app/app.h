#ifndef __APP_H
#define __APP_H

#include <stdint.h>

/* OFF 为电机逻辑关机；插电或输入未确认时可继续保电监测，ON 工作，PROTECT 停机。 */
typedef enum
{
    APP_STATE_OFF = 0U,
    APP_STATE_ON = 1U,
    APP_STATE_PROTECT = 2U
} app_state_t;

/* 恢复风扇档位；K3 按住上电时直接启动，其他上电保持待机。 */
void App_Init(void);

/* 每 1 ms 执行一次按键、电机档位和电源状态处理。 */
void App_Task(void);

/* 返回当前应用状态。 */
app_state_t App_GetState(void);

#endif /* __APP_H */
