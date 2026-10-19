# El entorno del ESP-IDF con el que se compilan los firmwares ESP de BasicPlus: la S3 (esp32), la C3, la C5, la C6 y
# la P4. Desde el 19-oct-2026 (#707) es el IDF 6.1, el mismo para toda la familia («es para no mezclar librerías»,
# Eduardo), instalado con el instalador de Espressif (EIM) en C:\esp\v6.1, con sus herramientas y su Python en
# C:\Espressif\tools.
#
# Hace lo mismo que el script de EIM (C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1) salvo dos cosas:
#   - no hace su «eim select v6.1» del final, que cambia el IDF elegido para los IDE (eim_idf.json);
#   - no registra el autocompletado de idf.py.
# El export.ps1 del propio IDF no sirve con una instalación de EIM: busca un Python que EIM no crea.
#
# Uso, en PowerShell (desde bash el entorno se pierde y idf.py «no se reconoce»):
#   . C:\lenguajes\pm\bpgenvm-c\tools\entorno_idf.ps1
#   idf -C C:\lenguajes\pm\bpgenvm-c\esp32p4 build
#
# Ojo: si se cambia sdkconfig.defaults, hay que borrar la sdkconfig del proyecto y recompilar; si no, el cambio no
# entra.
$idfRaiz = "C:\esp\v6.1\esp-idf"
$herr = "C:\Espressif\tools"
$venv = "$herr\python\v6.1\venv"

$env:IDF_PATH = $idfRaiz
$env:IDF_TOOLS_PATH = $herr
$env:IDF_PYTHON_ENV_PATH = $venv
$env:IDF_COMPONENT_LOCAL_STORAGE_URL = "file://$herr"
$env:ESP_ROM_ELF_DIR = "$herr\esp-rom-elfs\20241011/"
$env:OPENOCD_SCRIPTS = "$herr\openocd-esp32\v0.12.0-esp32-20260703/openocd-esp32/share/openocd/scripts"
$env:ESP_CLANG_LIBS_PATH = "$herr\esp-clang-libs\esp-21.1.3_20260408/esp-clang/lib"
$env:IDF_CCACHE_ENABLE = "1"
$env:ESP_IDF_VERSION = "6.1"
$env:IDF_VERSION = "6.1.0"
$env:VIRTUAL_ENV = $venv
$env:PATH = @(
    "$herr\ccache\4.12.1\ccache-4.12.1-windows-x86_64",
    "$herr\cmake\4.0.3\bin",
    "$herr\esp-rom-elfs\20241011",
    "$herr\idf-exe\1.0.3",
    "$herr\ninja\1.12.1",
    "$herr\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin",
    "$herr\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin",
    "$herr\esp32ulp-elf\2.38_20240113\esp32ulp-elf\bin",
    "$venv\Scripts",
    $env:PATH
) -join ";"

function global:idf {
    & "$env:IDF_PYTHON_ENV_PATH\Scripts\python.exe" "$env:IDF_PATH\tools\idf.py" @args
}
