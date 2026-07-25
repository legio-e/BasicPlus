#!/usr/bin/env bash
# compat/compat.sh — Arnes de compatibilidad V3 -> V2 (BasicPlus, H1)
#
# ############################################################################
# ## 'check' DESACTIVADO en V4 (decision de Eduardo, 17-jul).               ##
# ## La compatibilidad BINARIA con las versiones anteriores (V2 y V3) ya no ##
# ## se sostiene: estamos modificando el formato del .mod a proposito.      ##
# ## La red pasa a ser: PARIDAD VM-Java <-> VM-C + la bateria de tests.     ##
# ## El arnes NO se borra: V2 esta publicada y los goldens son su registro. ##
# ## Ver el mensaje de check() para el detalle. 'gen' sigue disponible.     ##
# ############################################################################
#
# El invariante que este arnes verificaba (principio 7 de V3): un programa de V2
# corre SIN CAMBIOS en V3. Se comprobaba en tres frentes:
#   - comportamiento  : la salida de cada .mod de V2 no cambia en las VMs V3.
#   - opcodes         : los ids de opcode de V2 no se mueven (Java y C).
#   - emision         : el frontend V3 emite .mod byte-identicos a los de V2.
# Los tres daban por contrato los ARTEFACTOS BINARIOS de V2 — y eso es justo lo
# que V4 rompe a proposito (ver check()).
#
#   ./compat.sh gen     Captura goldens con los binarios V2 (capsula v2/bin):
#                       v2/golden/*.mod + *.out + opcodes_java.txt + opcodes_c.txt.
#                       Solo se re-ejecuta cuando cambia V2 (raro: V2 esta congelada).
#
#   ./compat.sh check   DESACTIVADO: explica por que y no verifica nada.
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
SAMPLES="$ROOT/bpgenvm-c/samples"
STDLIB="$ROOT/bpstdlib"
GOLD="$HERE/v2/golden"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# --- Binarios V2 (capsula inmutable) ---
V2_FE="$HERE/v2/bin/basicplus-frontend.jar"
V2_JAVA="$HERE/v2/bin/bpgenvm-1.0.jar"
V2_C="$HERE/v2/bin/bpgenvm-c.exe"
# --- Binarios / fuentes V3 (actuales) ---
V3_FE="$ROOT/lexer-java/target/basicplus-frontend.jar"
V3_JAVA="$ROOT/miVM/target/bpgenvm-1.0.jar"
V3_C="$ROOT/bpgenvm-c/build/bpgenvm-c.exe"
JAVA_OPC="$ROOT/miVM/src/main/java/edu/bpgenvm/bytecode/OpCode.java"
C_OPC="$ROOT/bpgenvm-c/include/bpvm_opcodes.h"

# Corpus: nombres de fichero .bp (sin extension) en bpgenvm-c/samples/.
# Elegidos por cobertura de features V2 y por NO depender de stdlib mas alla de
# Core (import implicito de clases/excepciones). Se amplia en tandas posteriores.
CORPUS="hello arith strings concat charat counter MethodCall trycatch \
        bytetest longtest longarr doubletest casttest utf8test idxtest \
        convtest strops OverloadTest \
        samples/LocalArrTest.bp"

# Un item del CORPUS es (a) un nombre suelto -> $SAMPLES/<n>.bp, o (b) una RUTA
# relativa a la raiz del repo (lleva '/') -> tal cual. La (b) existe para que los
# oraculos que viven FUERA de bpgenvm-c/samples/ tambien entren en la red: un .bp
# suelto que no esta en ninguna bateria se pudre en silencio. Paso justamente con
# samples/LocalArrTest.bp (oraculo de L8 v3, paridad byte a byte): estuvo roto
# desde el ensanchado 4->8B sin que nadie lo oyera, y despues estuvo ARREGLADO
# (b99529e) sin que nadie se enterara tampoco. Si un .bp es un oraculo, va aqui.
bp_path() {
  case "$1" in
    */*) echo "$ROOT/$1" ;;
    *)   echo "$SAMPLES/$1.bp" ;;
  esac
}

filt() { grep -vE 'INICIANDO|FIN DE|heapStart|^config:' | sed '/^[[:space:]]*$/d'; }

run_vm() {  # $1=bin(.jar|.exe) $2=mod
  # Se ejecuta DESDE el dir del .mod para que ambas VMs resuelvan las deps
  # (p.ej. Core.mod, dep implicita de try/catch en V3 desde #248) junto al
  # modulo raiz. La VM-C ya resuelve relativo al .mod; el CLI de la VM-Java
  # resuelve relativo al CWD, asi que sin este cd la Java no encontraria Core.
  # Subshell () para no alterar el CWD del propio arnes. Los binarios ($1) son
  # rutas absolutas, siguen resolviendo desde cualquier dir.
  local dir base; dir="$(dirname "$2")"; base="$(basename "$2")"
  case "$1" in
    *.jar) ( cd "$dir" && java -jar "$1" "$base" 2>/dev/null | filt ) ;;
    *)     ( cd "$dir" && "$1" "$base" 2>/dev/null | filt ) ;;
  esac
}

# Extrae "NOMBRE 0xID" de cada enum (ids a mayusculas para comparar).
extract_java() { sed -nE 's/^[[:space:]]*([A-Z][A-Z0-9_]*)[[:space:]]*\((0x[0-9A-Fa-f]+).*/\1 \2/p' "$JAVA_OPC" | tr 'a-f' 'A-F' | sort; }
extract_c()    { sed -nE 's/^#define[[:space:]]+OP_([A-Z0-9_]+)[[:space:]]+(0x[0-9A-Fa-f]+).*/\1 \2/p' "$C_OPC" | tr 'a-f' 'A-F' | sort; }

# ---------------------------------------------------------------- gen
gen() {
  echo "== gen: capturando goldens con los binarios V2 =="
  rm -rf "$GOLD"; mkdir -p "$GOLD"
  java -jar "$V2_FE" "$STDLIB/Core.bp" --compile "$GOLD" --backend=mivm >/dev/null 2>&1
  local ok=0 skip=0 s
  for s in $CORPUS; do
    rm -f "$WORK"/*.mod "$WORK"/*.bpi
    if ! java -jar "$V2_FE" "$SAMPLES/$s.bp" --compile "$WORK" --backend=mivm >/dev/null 2>&1; then
      echo "  SKIP $s  (no compila con frontend V2)"; skip=$((skip+1)); continue
    fi
    local mod; mod="$(ls "$WORK"/*.mod 2>/dev/null | head -1)"
    [ -z "$mod" ] && { echo "  SKIP $s  (sin .mod)"; skip=$((skip+1)); continue; }
    cp "$GOLD/Core.mod" "$WORK/" 2>/dev/null
    local oj oc; oj="$(run_vm "$V2_JAVA" "$mod")"; oc="$(run_vm "$V2_C" "$mod")"
    if [ "$oj" != "$oc" ]; then
      echo "  SKIP $s  (divergencia dual-VM en V2 -> fuera del corpus)"; skip=$((skip+1)); continue
    fi
    local base; base="$(basename "$mod" .mod)"
    cp "$mod" "$GOLD/$base.mod"; printf '%s\n' "$oj" > "$GOLD/$base.out"
    echo "  OK   $s  -> $base.mod"; ok=$((ok+1))
  done
  extract_java > "$GOLD/opcodes_java.txt"
  extract_c    > "$GOLD/opcodes_c.txt"
  echo "  opcodes V2: java=$(wc -l < "$GOLD/opcodes_java.txt"), c=$(wc -l < "$GOLD/opcodes_c.txt")"
  echo "== gen: $ok goldens de comportamiento, $skip omitidos =="
}

# ------------------------------------------------------- check: comportamiento
check_behaviour() {
  echo "-- comportamiento (3 ejes: V3-Java==golden, V3-C==golden, V3-Java==V3-C) --"
  local pass=0 fail=0 g base
  for g in "$GOLD"/*.mod; do
    base="$(basename "$g" .mod)"; [ "$base" = "Core" ] && continue
    [ -f "$GOLD/$base.out" ] || continue
    rm -f "$WORK"/*.mod; cp "$GOLD/Core.mod" "$WORK/" 2>/dev/null; cp "$g" "$WORK/"
    local mod="$WORK/$base.mod" gold oj oc
    gold="$(cat "$GOLD/$base.out")"
    oj="$(run_vm "$V3_JAVA" "$mod")"; oc="$(run_vm "$V3_C" "$mod")"
    if [ "$oj" = "$gold" ] && [ "$oc" = "$gold" ] && [ "$oj" = "$oc" ]; then
      pass=$((pass+1))
    else
      echo "  FAIL $base"; fail=$((fail+1))
      [ "$oj" != "$gold" ] && echo "       - V3-Java difiere del golden"
      [ "$oc" != "$gold" ] && echo "       - V3-C    difiere del golden"
      [ "$oj" != "$oc" ]   && echo "       - V3-Java != V3-C (paridad rota)"
    fi
  done
  echo "  comportamiento: $pass PASS, $fail FAIL"
  [ "$fail" -eq 0 ]
}

# ------------------------------------------------------- check: opcodes
check_opcodes() {
  echo "-- opcodes (ids V2 intactos en Java y C; Java==C en los compartidos) --"
  local fail=0
  extract_java > "$WORK/oj.txt"; extract_c > "$WORK/oc.txt"
  while read -r name id; do
    cur="$(awk -v n="$name" '$1==n{print $2}' "$WORK/oj.txt")"
    if [ -z "$cur" ]; then echo "  FAIL Java opcode $name BORRADO (era $id)"; fail=1
    elif [ "$cur" != "$id" ]; then echo "  FAIL Java opcode $name MOVIDO $id -> $cur"; fail=1; fi
  done < "$GOLD/opcodes_java.txt"
  while read -r name id; do
    cur="$(awk -v n="$name" '$1==n{print $2}' "$WORK/oc.txt")"
    if [ -z "$cur" ]; then echo "  FAIL C opcode $name BORRADO (era $id)"; fail=1
    elif [ "$cur" != "$id" ]; then echo "  FAIL C opcode $name MOVIDO $id -> $cur"; fail=1; fi
  done < "$GOLD/opcodes_c.txt"
  local disc; disc="$(join "$WORK/oj.txt" "$WORK/oc.txt" | awk '$2!=$3{print "  FAIL "$1": Java "$2" != C "$3}')"
  [ -n "$disc" ] && { echo "$disc"; fail=1; }
  [ "$fail" -eq 0 ] && echo "  opcodes: OK"
  return $fail
}

# ------------------------------------------------------- check: emision
check_emit() {
  echo "-- emision (frontend V3 emite .mod byte-identico al golden V2) --"
  local fail=0 s
  for s in $CORPUS; do
    rm -f "$WORK"/*.mod "$WORK"/*.bpi
    java -jar "$V3_FE" "$SAMPLES/$s.bp" --compile "$WORK" --backend=mivm >/dev/null 2>&1 || continue
    local mod; mod="$(ls "$WORK"/*.mod 2>/dev/null | head -1)"; [ -z "$mod" ] && continue
    local base; base="$(basename "$mod" .mod)"; [ -f "$GOLD/$base.mod" ] || continue
    if ! cmp -s "$mod" "$GOLD/$base.mod"; then
      echo "  EMIT-DIFF $base  (.mod del frontend V3 != golden V2 -> intencional? regenera golden)"; fail=1
    fi
  done
  [ "$fail" -eq 0 ] && echo "  emision: OK"
  return $fail
}

# ------------------------------------------------------- check: paridad dual-VM
# El unico frente vivo desde V4. No mira a V2: compila el corpus con el frontend
# ACTUAL y compara las dos VMs de hoy ENTRE SI. Es lo que sigue siendo cierto
# cuando el formato del .mod evoluciona a proposito, y es la red que pide
# docs/PUBLICAR.md ("paridad dual-VM en host").
check_parity() {
  echo "-- paridad dual-VM (frontend actual -> VM-Java == VM-C) --"
  local pass=0 fail=0 skip=0 s bp mod oj oc
  for s in $CORPUS; do
    bp="$(bp_path "$s")"
    if [ ! -f "$bp" ]; then
      echo "  SKIP $s (no existe $bp)"; skip=$((skip+1)); continue
    fi
    rm -f "$WORK"/*.mod
    if ! java -jar "$V3_FE" "$bp" --compile "$WORK" --backend=mivm >/dev/null 2>&1; then
      echo "  SKIP $s (no compila con el frontend actual)"; skip=$((skip+1)); continue
    fi
    # El .mod raiz es el que NO es Core (dep implicita desde #248).
    mod="$(ls "$WORK"/*.mod 2>/dev/null | grep -v '[/\]Core\.mod$' | head -1)"
    if [ -z "$mod" ]; then
      echo "  SKIP $s (el frontend no emitio .mod)"; skip=$((skip+1)); continue
    fi
    [ -f "$WORK/Core.mod" ] || cp "$STDLIB/Core.mod" "$WORK/" 2>/dev/null
    oj="$(run_vm "$V3_JAVA" "$mod")"; oc="$(run_vm "$V3_C" "$mod")"
    if [ "$oj" = "$oc" ]; then
      pass=$((pass+1))
    else
      echo "  FAIL $s — VM-Java != VM-C (paridad rota)"; fail=$((fail+1))
      diff <(printf '%s\n' "$oj") <(printf '%s\n' "$oc") 2>/dev/null | head -6 | sed 's/^/       /'
    fi
  done
  echo "  paridad: $pass PASS, $fail FAIL, $skip SKIP"
  [ "$fail" -eq 0 ]
}

# check_behaviour / check_opcodes / check_emit se conservan intactas arriba: son
# el registro de lo que se verificaba contra V2 y siguen siendo reutilizables si
# algun dia hay algo que comparar. Desde V4 NO se llaman (ver el aviso de abajo).
check() {
  cat <<'EOF'
== check: compatibilidad binaria con V2/V3 DESACTIVADA ==

Decision de Eduardo (17-jul, V4): se desactiva la compatibilidad BINARIA con las
versiones anteriores. Estamos modificando el formato del .mod a proposito, asi
que mantenerla es imposible — y fingir que se mantiene es peor que no tenerla:

  - el ensanchado de refs 4->8B (H1.2a, 9-jul) cambio el ABI del bytecode;
  - H6.a metio la interfaz DENTRO del .mod y subio el formato a v6;
  - #284 fijo la norma: el formato del .mod debe ser IGUAL al que habla la VM.
    Si no coincide -> recompilar (y ya lo tienes actualizado). Un .mod v5 de V2
    lo RECHAZAN las dos VMs, a proposito.

Los tres frentes de V2 daban por contrato sus ARTEFACTOS BINARIOS, y por eso se
retiran: 'comportamiento' ejecutaba los .mod v5 de V2 en las VMs de hoy (justo
lo que #284 prohibe); 'emision' exigia .mod byte-identicos a los de V2
(imposible en cuanto el formato evoluciona); 'opcodes' solo tenia sentido con
los otros dos. El arnes NO se borra: V2 esta publicada (Release v2.0) y
v2/golden + v2/bin son su registro historico; 'gen' sigue disponible.

Lo que SI seguimos garantizando, y es lo que verifica este check:
  - PARIDAD VM-Java <-> VM-C sobre la cadena ACTUAL (abajo);
  - la bateria de tests (fuera de este arnes).

EOF
  local fails=0
  check_parity || fails=$((fails+1))
  echo "== check: $([ "$fails" -eq 0 ] && echo 'paridad dual-VM VERDE' || echo 'paridad dual-VM en ROJO') =="
  [ "$fails" -eq 0 ]
}

case "${1:-}" in
  gen)   gen ;;
  check) check ;;
  *) echo "uso: $0 {gen|check}"; exit 2 ;;
esac
