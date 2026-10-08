# Task Plan: 电池欠压与过压保护

## Goal
移除电量计算/显示，参考电钻工程接入电池欠压 13.5V、欠压恢复 15.5V、过压 24V 保护，保留中文注释并验证 Keil 编译。

## Next Step
代码与编译验证已完成；交付结果，等待用户进行实板电压和输出验证。

## Current Phase
Complete

## Phases

### Phase 1: 调查与方案
- [x] 检查现有电池、ADC、应用和充电模块。
- [x] 阅读用户指定电钻工程的欠压/过压实现。
- [x] 明确阈值、滤波与停机/恢复策略。
- **Status:** complete

### Phase 2: 实现
- [x] 移除电量功能，启用电压保护。
- [x] 接入启动与运行期间的电机保护，并添加中文注释。
- [x] 同步受影响的接口和项目说明。
- **Status:** complete

### Phase 3: 验证与交付
- [x] 阈值、恢复、防重启等逻辑审阅；动态与实板测试未执行，单独说明限制。
- [x] Keil 全量编译与差异检查。
- [x] 说明结果及硬件验证限制。
- **Status:** complete

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| 只读取参考电钻工程 | 用户要求参考，不修改另一个产品固件。 |
| 不启用充电管理、不烧录硬件 | 本次范围仅电池欠压/过压保护。 |
| 保留开始时已有生成文件/IDE 状态修改 | 用户现有改动不可覆盖或回退；构建会正常更新产物。 |

## Errors Encountered
| Error | Resolution |
|-------|------------|
| Grep/Glob uv_spawn | 改用Python只读搜索。 |
| Python默认GBK输出UnicodeEncodeError | 显式设置PYTHONIOENCODING=utf-8。 |
| ARMCLANG查询目标要求显式ARM目标；无主机C编译器 | 使用ARMCC验证全部宏组合编译；未运行主机动态测试。 |
| 全树diff --check发现构建产物尾空格 | 单独对USER/main/CLAUDE源文件检查通过，不修改生成器产物格式。 |
