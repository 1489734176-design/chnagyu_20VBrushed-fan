# Task Plan: 充电软件准备

## Goal
第一阶段软件准备已完成；第二阶段按用户确认的硬件恒流与20.0 V截止要求开启通路，不烧录。

## Next Step
交付已启用充电通路与20.0 V锁存截止的固件结果；烧录及板上充电验证需另行授权。

## Current Phase
Complete

## Phases
### Phase 1: Requirements & Discovery
- [x] 核对PDF、BOM和调用路径。
- [x] 确认5串/20 V/无BMS/4 A目标/边充边用及K3行为。
- **Status:** complete
### Phase 2: Planning & Structure
- [x] 用户批准计划：C:/Users/mym02/.claude/plans/transient-tumbling-hickey.md。
- [x] 限定为软件准备，实际充电不启用。
- **Status:** complete
### Phase 3: Implementation
- [x] 独立输入检测与迟滞。
- [x] 有界ADC及电池故障锁存。
- [x] 逻辑关机/拔出断电，文档更新。
- **Status:** complete
### Phase 4: Testing & Verification
- [x] 实际模块桩测试。
- [x] Keil检测开/关全量编译及资源检查。
- [x] 检查所有CH_EN输出及最终diff。
- **Status:** complete
### Phase 5: Delivery
- [x] 报告完成项、测试结果和未做硬件验证。
- **Status:** complete

### Phase 6: 启用通路及截止
- [x] 用户确认适配器恒流、软件20.0 V停止，批准第二阶段计划。
- [x] 通路输出统一决策、截止锁存、9/8 V输入存在检测。
- [x] 逻辑关机/欠压与充电解耦，文档更新。
- **Status:** complete
### Phase 7: 第二阶段验证与交付
- [x] 模拟开启、截止、故障及功能关闭组合。
- [x] Keil重编译与最终配置核对。
- [x] 交付并说明未烧录/未验证硬件。
- **Status:** complete

## Decisions Made
| Decision | Rationale |
|---|---|
| 第二阶段充电宏启用、20.0 V截止锁存 | 用户确认硬件恒流，选择软件截止值 |
| 第二阶段输入存在检测9/8 V | 兼容恒流源输出降压，须板上校准 |
| 第一阶段保持CHARGE_MANAGEMENT_ENABLE=0（历史） | 当时只做软件准备 |
| 4 A只作目标，不参与控制 | I_SENSE仅测电机 |
| 保留现有电压保护阈值 | 本次不擅自重定保护策略 |
| 测试文件放临时目录 | 不新增生产模块/Keil项目项 |

## Errors Encountered
| Error | Resolution |
|---|---|
| fitz未安装 | 使用pdftotext与标准库 |
| PATH无主机C编译器 | 检查Keil附带工具或隔离测试工具 |
