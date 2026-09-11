#!/usr/bin/env bash
# ============================================================
# regen_mods.sh — EL generador de la stdlib embebida, para las tres familias (V6/U4).
#
# Emite UN fichero de SÓLO DATOS y su cabecera:
#   <dir>/<fam>_mods.c   static const unsigned char <mod>_mod[] = {…};   (xxd -i)
#                        const bpvm_mod_embebido_t <fam>_mods[] = { {"/lib/X.mod", x_mod, sizeof x_mod}, … };
#                        const unsigned <fam>_mods_n;
#   <dir>/<fam>_mods.h   extern const bpvm_mod_embebido_t <fam>_mods[]; extern const unsigned <fam>_mods_n;
#
# NI UNA LÍNEA DE CÓDIGO dentro del generado: el bucle (bpvm_mods_instalar_tabla) y
# la regla (#466) viven en src/bpvm_mods.c. Lo que se puso a mano en un generado
# murió en la siguiente regeneración (#422 → #446). La longitud va por `sizeof`
# (una expresión constante de verdad), no por la variable `_len` de xxd.
#
# Uso: regen_mods.sh <familia> <dir_salida> MOD... [--extra /ruta/en/placa=fichero.mod ...]
#      Los MOD salen de bpstdlib/<MOD>.mod y van a /lib/<MOD>.mod. Los --extra son
#      entradas sueltas (el Hello.mod de muestra de la Pico, en /app).
# ============================================================
set -euo pipefail

INVOCACION="$*"     # V6/G2: se guarda en el generado. La lista de modulos y los --extra
                    # vivian en la cabeza de quien lo lanzo, y el Hello embebido ya no existia
                    # en el arbol cuando hubo que regenerar (11-sep).
FAM="$1"; OUT="$2"; shift 2
STDLIB="$(cd "$(dirname "$0")/../.." && pwd)/bpstdlib"
MODS=(); EXTRA=()
while [ $# -gt 0 ]; do
    case "$1" in
        --extra) EXTRA+=("$2"); shift 2 ;;
        *)       MODS+=("$1");  shift ;;
    esac
done
for m in "${MODS[@]}"; do
    [ -f "$STDLIB/$m.mod" ] || { echo "ERROR: falta $STDLIB/$m.mod (compila la stdlib primero)" >&2; exit 1; }
done
for e in ${EXTRA[@]+"${EXTRA[@]}"}; do
    [ -f "${e#*=}" ] || { echo "ERROR: falta ${e#*=}" >&2; exit 1; }
done

var_of() { echo "$1" | tr '[:upper:]' '[:lower:]' | tr -c 'a-z0-9\n' '_'; }
blob()   { xxd -i -n "$1" "$2" | sed 's/^unsigned char /static const unsigned char /; /^unsigned int /d'; }

C="$OUT/${FAM}_mods.c"; H="$OUT/${FAM}_mods.h"; GUARD="$(echo "${FAM}_MODS_H" | tr '[:lower:]' '[:upper:]')"
{
    echo "/*"
    echo " * ${FAM}_mods.c — GENERADO por scripts/regen_mods.sh. NO EDITAR A MANO."
    echo " *"
    echo " * La stdlib embebida de la imagen ${FAM}: SÓLO DATOS (los blobs y la tabla)."
    echo " * El bucle y la regla viven en src/bpvm_mods.c; lo que se ponga aquí a mano"
    echo " * muere en la siguiente regeneración (#422 → #446)."
    echo " *"
    echo " * Invocación (reproducible): scripts/regen_mods.sh $INVOCACION"
    echo " */"
    echo "#include \"${FAM}_mods.h\""
    echo ""
    for m in "${MODS[@]}"; do
        blob "$(var_of "$m")_mod" "$STDLIB/$m.mod"
        echo ""
    done
    i=0
    for e in ${EXTRA[@]+"${EXTRA[@]}"}; do
        blob "extra${i}_mod" "${e#*=}"; i=$((i+1))
        echo ""
    done
    echo "const bpvm_mod_embebido_t ${FAM}_mods[] = {"
    for m in "${MODS[@]}"; do
        v="$(var_of "$m")_mod"
        printf '    { "/lib/%s.mod", %s, sizeof %s },\n' "$m" "$v" "$v"
    done
    i=0
    for e in ${EXTRA[@]+"${EXTRA[@]}"}; do
        v="extra${i}_mod"; i=$((i+1))
        printf '    { "%s", %s, sizeof %s },\n' "${e%%=*}" "$v" "$v"
    done
    echo "};"
    echo "const unsigned ${FAM}_mods_n = (unsigned) (sizeof ${FAM}_mods / sizeof ${FAM}_mods[0]);"
} > "$C"
{
    echo "/*"
    echo " * ${FAM}_mods.h — GENERADO por scripts/regen_mods.sh. NO EDITAR A MANO."
    echo " * La tabla de la stdlib embebida de la imagen ${FAM}; la recorre"
    echo " * bpvm_mods_instalar_tabla (src/bpvm_mods.c) con el put de la familia."
    echo " */"
    echo "#ifndef ${GUARD}"
    echo "#define ${GUARD}"
    echo "#include \"bpvm_mods.h\""
    echo "extern const bpvm_mod_embebido_t ${FAM}_mods[];"
    echo "extern const unsigned ${FAM}_mods_n;"
    echo "#endif"
} > "$H"
echo "generado: ${C#$PWD/} ($(wc -c < "$C") B) + $(basename "$H") — ${#MODS[@]} módulos${EXTRA[@]+ + ${#EXTRA[@]} extra}"
