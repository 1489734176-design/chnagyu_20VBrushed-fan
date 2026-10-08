# Findings & Decisions

## Requirements
- 电池功能只保留欠压/过压，无电量计算或显示。
- 欠压 <=13500mV；欠压恢复 >=15500mV；过压 >=24000mV。
- 参考指定电钻工程，中文注释；不启用充电、不修改参考工程、不烧录。

## Research Findings
- 参考 USER/user_control.c:415 Volt_Handler_fast 上电立即判断；:473 Volt_Handler 使用独立饱和递增/递减计数。
- 参考 parameter-300N.h:333-349 运行欠压1、过压各滤波300ms。参考并无独立过压恢复阈值；本次降到24V以下滤波恢复。
- 当前 Battery_Task 被宏禁用，6V欠压及电量线性算法未接入App；删除百分比还需清理Charge_Task禁用分支中的引用。
- 当前 App_Init 的 App_PowerOn 实际被注释，保留上电不自动转动的现状。main 与 App_Init 重复初始化模块，初始化统一归main。
- 复用 AppAdc_ReadVbusMv、Motor_StopAll、Charge_SetEnable(0)、Led_AllOff；保护时EN保持以便采样，K3可真正关机，恢复不自动启动。
- 固定5V参考、11倍分压仍需实测校准；ADC轮询转换约百微秒，现有flag_1ms为合并标志而非排队计数。

## Technical Decisions
- 用户已批准 starry-wishing-puppy.md 方案。
- 新增APP_STATE_PROTECT、启动快速检查以及过压/汇总保护查询；独立饱和计数，欠压带15.5V恢复迟滞。
- 缺少充满/终止策略时自动充电任务关闭CH_EN，不把24V过压阈值用作充满判断。

## Issues Encountered
- Grep/Glob报uv_spawn，已改Python只读搜索。
- Python控制台GBK无法输出混合编码替换字符，使用PYTHONIOENCODING=utf-8。
