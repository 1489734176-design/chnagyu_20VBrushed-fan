"""用已安装 ARMCC 编译可选宏组合；仅验证编译，不冒充动态或实板测试。"""
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
ARMCC = Path("D:/Users/mym02/AppData/Local/Keil_v5/ARM/ARMCC/Bin/armcc.exe")
PROJECT = ROOT / "MDK-ARM/TIM3_TimeBase.uvprojx"
CONFIG = ROOT / "USER/config/config.h"
SOURCES = ["USER/battery/battery.c", "USER/charge/charge.c", "USER/app/app.c"]

project = ET.parse(PROJECT)
include_text = project.findtext(".//Cads/VariousControls/IncludePath")
assert include_text, "Missing Keil C include path"
includes = [(PROJECT.parent / part.replace("\\", "/")).resolve()
            for part in include_text.split(";") if part]
config = CONFIG.read_text(encoding="utf-8")

# 阈值必须与需求一致；完整固件使用的仍是原始配置文件。
for name, expected in {
    "BATTERY_PROTECTION_ENABLE": 1,
    "CHARGE_MANAGEMENT_ENABLE": 0,
    "BATTERY_UVP_VOLTAGE_MV": 13500,
    "BATTERY_UVP_RECOVER_VOLTAGE_MV": 15500,
    "BATTERY_OVP_VOLTAGE_MV": 24000,
    "BATTERY_UVP_FILTER_TIME_MS": 300,
    "BATTERY_OVP_FILTER_TIME_MS": 300,
    "BATTERY_RECOVER_FILTER_TIME_MS": 300,
}.items():
    match = re.search(r"#define\s+" + name + r"\s+\((\d+)[UL]*\)", config)
    assert match and int(match.group(1)) == expected, name
print("Production threshold and feature configuration: PASS")

with tempfile.TemporaryDirectory(prefix="fan-voltage-build-") as work:
    work = Path(work)
    for battery in (0, 1):
        for charge in (0, 1):
            folder = work / f"battery-{battery}-charge-{charge}"
            folder.mkdir()
            variant = config
            for name, value in (("BATTERY_PROTECTION_ENABLE", battery),
                                ("CHARGE_MANAGEMENT_ENABLE", charge)):
                variant, count = re.subn(
                    r"(#define\s+" + name + r"\s+)\(\d+U\)",
                    lambda m: m.group(1) + f"({value}U)", variant)
                assert count == 1, name
            (folder / "config.h").write_text(variant, encoding="utf-8", newline="\n")
            for source in SOURCES:
                command = [str(ARMCC), "--cpu", "Cortex-M0", "--c99", "-c", "-O0",
                           "-D__MICROLIB", "-DUSE_STDPERIPH_DRIVER", "-I", str(folder)]
                for include in includes:
                    command += ["-I", str(include)]
                command += [str(ROOT / source), "-o", str(folder / (Path(source).stem + ".o"))]
                result = subprocess.run(command, capture_output=True)
                output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
                if result.returncode or re.search(r"warning|error", output, re.I):
                    raise RuntimeError(f"{source}, battery={battery}, charge={charge}:\n{output}")
            print(f"battery={battery}, charge={charge}: all 3 modules compile without warnings")

print("PASS: 4 configurations, 12 ARMCC compilations; production config unchanged")
