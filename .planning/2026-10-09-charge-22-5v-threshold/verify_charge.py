"""Adapt the existing local module fixture for the 22.5 V, non-latching policy.

Usage: python -I verify_charge.py REPO EXISTING_FIXTURE EXISTING_EMULATOR_DEPS
Only existing local dependencies are used; no packages are installed.
Original fixture files and repository sources are never modified.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

root, fixture, deps = (pathlib.Path(arg).resolve() for arg in sys.argv[1:4])
if not (deps / "unicorn").is_dir():
    raise RuntimeError("Existing Unicorn dependency directory is unavailable")
work = pathlib.Path(tempfile.mkdtemp(prefix="nx32-charge-22500-tests-"))
print(f"Test workspace: {work}", flush=True)
for name in ("test_platform.h", "startup.s", "emulate.py"):
    shutil.copyfile(fixture / name, work / name)
source = (fixture / "tests.c").read_text(encoding="utf-8")


def replace_once(old, new):
    global source
    if source.count(old) != 1:
        raise RuntimeError(f"Expected one fixture match: {old[:100]}")
    source = source.replace(old, new)


# The old fixture used 8.5 V as the middle of the 9/8 V detection band.
source = source.replace("8500UL", "((CHARGE_INPUT_INSERT_MV + CHARGE_INPUT_REMOVE_MV) / 2UL)")
replace_once("""    set_mv(1U,14000UL); app_ticks(500U);
    CHECK(Charge_IsEnabled() && Charge_IsInputPresent());""", """    raw_values[1]=raw_below(CHARGE_INPUT_INSERT_MV); App_Task();
    CHECK(!Charge_IsEnabled() && Charge_IsInputPresent() && en_high());
    app_ticks(500U); CHECK(!Charge_IsEnabled() && en_high());
    raw_values[1]=raw_above(CHARGE_INPUT_INSERT_MV); App_Task();
    CHECK(Charge_GetInputVoltageMv()>CHARGE_INPUT_INSERT_MV && Charge_IsEnabled());""")
replace_once('passed("automatic path control, no per-tick GPIO pulses, motor-use and constant-current droop");',
             'passed("automatic path, stable GPIO, motor use and strict input eligibility");')
old_start = source.index("    reset(20000UL);\n    raw_values[0]=raw_above(CHARGE_STOP_VOLTAGE_MV)-1U;")
old_end = source.index("    reset(20000UL); set_mv(0U,20000UL); Battery_Init(); App_Init();", old_start)
source = source[:old_start] + """    reset(20000UL);
    raw_values[0]=raw_above(CHARGE_STOP_VOLTAGE_MV)-1U;
    app_ticks(100U);
    CHECK(Battery_GetVoltageMv()<CHARGE_STOP_VOLTAGE_MV && Charge_IsEnabled());
    CHECK(!Charge_IsVoltageStopped());
    set_mv(0U,20000UL); App_Task(); CHECK(Charge_IsEnabled());
    set_mv(0U,CHARGE_STOP_VOLTAGE_MV); App_Task();
    CHECK(Battery_GetVoltageMv()>=CHARGE_STOP_VOLTAGE_MV);
    CHECK(!Charge_IsEnabled() && Charge_IsVoltageStopped());
    highs=charge_high_writes;
    Charge_SetEnable(1U); CHECK(!Charge_IsEnabled() && charge_high_writes==highs);
    press(KEY_EVENT_K3); CHECK(App_GetState()==APP_STATE_ON && !Charge_IsEnabled());
    press(KEY_EVENT_K1); press(KEY_EVENT_K2); press(KEY_EVENT_K3);
    CHECK(!Charge_IsEnabled() && Charge_IsVoltageStopped());
    adc_fail[0]=1U; App_Task(); adc_fail[0]=0U; app_ticks(300U);
    CHECK(!Charge_IsEnabled() && Charge_IsVoltageStopped());
    raw_values[0]=raw_above(CHARGE_STOP_VOLTAGE_MV)-1U; App_Task();
    CHECK(Charge_IsEnabled() && !Charge_IsVoltageStopped());
    CHECK(charge_high_writes==highs+1U);
    highs=charge_high_writes; lows=charge_low_writes;
    app_ticks(500U); Charge_SetEnable(1U);
    CHECK(Charge_IsEnabled() && charge_high_writes==highs && charge_low_writes==lows);
    set_mv(0U,CHARGE_STOP_VOLTAGE_MV); App_Task();
    CHECK(!Charge_IsEnabled() && Charge_IsVoltageStopped());
    set_mv(0U,18000UL); App_Task();
    CHECK(Charge_IsEnabled() && !Charge_IsVoltageStopped());
    set_mv(1U,0UL); App_Task(); CHECK(!Charge_IsEnabled());
    app_ticks(99U); CHECK(!Charge_IsEnabled() && !Charge_IsVoltageStopped());
    set_mv(1U,20000UL); app_ticks(100U); CHECK(Charge_IsEnabled());
    passed("22.5 V boundary, immediate cutoff and resume without unplugging");

    reset(20000UL); app_ticks(99U);
    raw_values[1]=raw_below(CHARGE_INPUT_INSERT_MV); App_Task();
    CHECK(!Charge_IsEnabled() && !Charge_IsInputPresent());
    set_mv(1U,20000UL); app_ticks(99U); CHECK(!Charge_IsEnabled());
    App_Task(); CHECK(Charge_IsEnabled());
    passed("input below insertion threshold resets consecutive confirmation");

    reset(20000UL); app_ticks(100U);
    set_mv(0U,CHARGE_STOP_VOLTAGE_MV); App_Task(); CHECK(!Charge_IsEnabled());
    set_mv(0U,18000UL); adc_fail[0]=1U; App_Task();
    CHECK(!Charge_IsEnabled() && Battery_IsAdcFault());
    adc_fail[0]=0U; app_ticks(299U); CHECK(!Charge_IsEnabled());
    App_Task(); CHECK(Charge_IsEnabled() && !Charge_IsVoltageStopped());
    passed("voltage recovery cannot bypass latched ADC fault recovery");

""" + source[old_end:]
replace_once("reset(20000UL); set_mv(0U,20000UL); Battery_Init(); App_Init();",
             "reset(20000UL); set_mv(0U,CHARGE_STOP_VOLTAGE_MV); Battery_Init(); App_Init();")
replace_once("fast_read_inject=2U; fast_vbus_mv=20000UL; press(KEY_EVENT_K3);",
             "fast_read_inject=2U; fast_vbus_mv=CHARGE_STOP_VOLTAGE_MV; press(KEY_EVENT_K3);")
(work / "tests.c").write_text(source, encoding="utf-8", newline="\n")
runner = (fixture / "run_tests.py").read_text(encoding="utf-8")
old_deps = "'C:/Users/mym02/AppData/Local/Temp/nx32-emulator-deps-vnd5gi_l'"
if runner.count(old_deps) != 1:
    raise RuntimeError("Unexpected fixture dependency argument")
runner = runner.replace(old_deps, repr(deps.as_posix()))
(work / "run_tests.py").write_text(runner, encoding="utf-8", newline="\n")
result = subprocess.run(
    [sys.executable, "-I", str(work / "run_tests.py"), str(root), str(work)],
    cwd=root, capture_output=True, text=True, errors="replace", timeout=600,
)
output = result.stdout + result.stderr
print(output, end="", flush=True)
pathlib.Path(__file__).with_name("module-tests.log").write_text(
    f"Test workspace: {work}\n" + output, encoding="utf-8", newline="\n"
)
sys.exit(result.returncode)
