# Findings & Decisions

## Requirements
5串锂离子、20 V普通适配器、无保护板、4 A仅目标。允许边充边用，插电K3关机保留监测。只做软件准备，CH_EN低，不烧录/不实充。

## Research Findings
- Q2=NX3407 P-MOS、Q4=MMBT5551，CH_EN高有效。
- R28/R29=10k/20k，20 V输入VGS约−6.7 V。
- D1=SS56串联到B+，没有充电CC/CV控制级。
- VBus测锁存后的VCC，CH_VIN测C+，均需标定。
- R18=5毫欧在电机回流，不测充电电流。
- C+经R6到Q3控制网络，插电可保电；EN低不能保证断电。
- ADC_GetFlagStatus为厂商HAL；失败清理可使用ADC_SoftwareStartConvCmd(DISABLE)、ADC_AnyChannelCmd(DISABLE)、ADC_ClearFlag。
- ARMCC5及Keil可用；PATH没有gcc/clang/cl。Keil附带ARMCLANG/avh-fvp待检查测试用途。

## 第二阶段确认与完成
用户确认外部适配器硬件恒流，要求打开通路；软件截止选择VBus 20.0 V，并批准停止锁存至拔插规则。
- 实际代码已充电启用；不实现软件4 A限流或尾电流/充满判断。
- 存在检测9/8 V兼容恒流降压；GPIO周期末统一输出，ADC/过压/截止优先。
- 输入确认、有效采样、未截止且电源可用时通路打开；仅放电欠压不误阻充。
- 20.0 V输入扣二极管/锁存压降可能到不了20.0 V VBus，明确不保证截止一定可达。
- 四组合模拟140/59/92/17断言通过；Keil四组合0错误0警告。最终Code6636、RO236、RW92、ZI516；未烧录/未板上验证。

## Resources
- ../资料目录原理图PDF和BOM.xlsx（只读核实）
- C:/Users/mym02/.claude/plans/transient-tumbling-hickey.md：批准的完整实施计划

## Issues Encountered
fitz不存在，已改用pdftotext。现有power.c行末空白以及IDE/生成文件修改应保留。
