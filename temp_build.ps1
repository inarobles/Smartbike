$env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.5.1"
$env:PATH = "C:\Espressif\python_env\idf5.5_py3.11_env\Scripts;C:\Espressif\tools\riscv32-esp-elf\esp-14.2.0_20241119\riscv32-esp-elf\bin;C:\Espressif\tools\cmake\3.30.2\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\ccache\4.11.2;C:\Espressif\tools\idf-git\2.44.0\cmd;" + $env:PATH
$env:CCACHE_DISABLE = "1"
$env:PYTHONIOENCODING = "utf-8"
$env:ESP_IDF_VERSION = "5.5"
$env:IDF_TARGET = "esp32p4"
if (Test-Path build_fix) { rm -r -force build_fix }
if (Test-Path sdkconfig) { rm sdkconfig }
python $env:IDF_PATH\tools\idf.py -B build_fix -DSDKCONFIG_DEFAULTS="sdkconfig.defaults" build -- -j 1 *>&1 > idf_build.log
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
