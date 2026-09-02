#!/usr/bin/env bash
# ============================================================
# regen_esp32_mods.sh — regenera esp32/common/esp32_mods.c embebiendo la
# stdlib core de BasicPlus en el firmware del ESP32-S3.
#
# Gemelo de stm32/scripts/regen_stm32_mods.sh: cada <Name>.mod de bpstdlib/
# se convierte a un array C con `xxd -i` y se agrega a una tabla;
# esp32_mods_install() los pre-instala en /lib del FS (RAM) al arrancar
# (idéntico patrón al firmware de la Pico y del STM32).
#
# Uso:   bash bpgenvm-c/esp32/scripts/regen_esp32_mods.sh
# Tras cambiar la API de un módulo stdlib: recompílalo (IDE), re-ejecuta
# este script y reflashea el firmware (idf.py build flash).
#
# El set embebido = EMBEDDED_CORE_MODS del IDE (FrmMain.java): los módulos
# que el IDE da por pre-instalados en el device y por tanto NO sube.
# ============================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
STDLIB="$(cd "$HERE/../../.." && pwd)/bpstdlib"
OUT="$HERE/../common/esp32_mods.c"

# Mismo conjunto que EMBEDDED_CORE_MODS en el IDE. Nombre tal cual (la
# resolución de imports busca "<Module>.mod" preservando mayúsculas).
MODS=(Core Math IO Gpio I2c Spi Uart Pulse Pwm Pico Rtc Adc Wdt Timer)

for m in "${MODS[@]}"; do
    if [ ! -f "$STDLIB/$m.mod" ]; then
        echo "ERROR: falta $STDLIB/$m.mod (compila la stdlib primero)" >&2
        exit 1
    fi
done

var_of() { echo "$1" | tr '[:upper:]' '[:lower:]'; }

{
    echo "/*"
    echo " * esp32_mods.c — GENERADO por scripts/regen_esp32_mods.sh. NO EDITAR A MANO."
    echo " *"
    echo " * stdlib core de BasicPlus embebida en flash. esp32_mods_install() la"
    echo " * pre-instala en /lib del FS (RAM) al boot, igual que EMBEDDED_CORE_MODS"
    echo " * de la Pico / stm32_mods.c → los programas que importan stdlib resuelven"
    echo " * sin que el IDE tenga que subir las dependencias."
    echo " */"
    echo '#include "esp32_mods.h"'
    echo '#include "fs.h"'
    echo '#include "log.h"        /* log_printf: lo que decidió el instalador, al log del boot */'
    echo '#include "bpvm_mods.h"  /* #466: LA REGLA del /lib vive en src/, no aquí (esto es generado) */'
    echo '#include <stdint.h>'
    echo ""
    for m in "${MODS[@]}"; do
        var="$(var_of "$m")_mod"
        # xxd: `unsigned char X[] = {...}; unsigned int X_len = N;`
        # → lo hacemos `static const` para no exportar símbolos.
        xxd -i -n "$var" "$STDLIB/$m.mod" | sed 's/^unsigned /static const unsigned /'
        echo ""
    done
    echo "typedef struct { const char* path; const unsigned char* data; unsigned len; } mod_entry_t;"
    echo ""
    echo "static const mod_entry_t s_mods[] = {"
    for m in "${MODS[@]}"; do
        var="$(var_of "$m")_mod"
        printf '    { "/lib/%s.mod", %s, %s_len },\n' "$m" "$var" "$var"
    done
    echo "};"
    echo ""
    # OJO: install en LOTE (021fdbf) — sin el suspend/resume, cada fs_put
    # auto-persiste la partición entera y el primer boot del P4 tardaba ~46 s.
    # #466 — la DECISIÓN (falta / versión anterior / CRC distinto → se repone; más
    # nuevo → se deja) NO está aquí: está en src/bpvm_mods.c. Aquí sólo el bucle y
    # el lote. Lo que se puso a mano en este generado (#422) lo borró la siguiente
    # regeneración (#446); por eso no se vuelve a poner nada aquí.
    echo "static int mods_put(const char* p, const uint8_t* d, uint32_t n) {"
    echo "    return fs_put(p, d, n) == FS_OK ? 0 : -1;"
    echo "}"
    echo ""
    echo "void esp32_mods_install(void) {"
    echo "    unsigned n = (unsigned) (sizeof(s_mods) / sizeof(s_mods[0]));"
    echo "    unsigned escritos = 0;"
    echo "    /* LOTE: sin suspender, cada fs_put auto-persiste reescribiendo la partición"
    echo "     * entera (~3 s en la bpfs de 10 MB del P4) → el primer boot tardaba ~46 s."
    echo "     * Suspender + UN save al final lo deja en ~3 s; si no se escribe nada"
    echo "     * (boots siguientes), ni siquiera se guarda. */"
    echo "    fs_autosave_suspend();"
    echo "    for (unsigned i = 0; i < n; i++) {"
    echo "        char linea[160];"
    echo "        bpvm_mods_res_t r = bpvm_mods_sincronizar(s_mods[i].path, s_mods[i].data,"
    echo "                                                  s_mods[i].len, mods_put,"
    echo "                                                  linea, sizeof linea);"
    echo "        if (r == BPVM_MODS_INSTALADO || r == BPVM_MODS_REPUESTO) escritos++;"
    echo "        if (linea[0]) log_printf(\"%s\", linea);"
    echo "    }"
    echo "    fs_autosave_resume(escritos > 0);"
    echo "}"
} > "$OUT"

echo "generado: $OUT"
echo "modulos embebidos: ${MODS[*]}"
