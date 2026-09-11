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
        bytetest longtest longarr doubletest powtest casttest utf8test idxtest \
        convtest strops OverloadTest OverloadMethod OverloadCtor SlotPropPriv SlotThreadSub \
        samples/LocalArrTest.bp samples/StrOps348.bp samples/MathOps348.bp samples/PathOps348.bp samples/EvFin.bp samples/ThreadTrasMain.bp         SciPar ArrLitAncho ObjArray CastExt ListaBp ListaHer CastSelf OwnerBp SuperExt samples/BusBug.bp samples/AdcDemo.bp samples/ArgDemo.bp samples/MathRango.bp samples/mathtest.bp samples/NeoCatch.bp samples/WdtCatch.bp samples/StubParidad.bp samples/IoPrompt.bp samples/IoPathAbs.bp samples/ThrowSinAtrapar.bp samples/MachineHost.bp samples/MachineAlias.bp samples/MachineId.bp samples/GuiParidad.bp samples/GuiParidad2.bp GuiChurn OwnerCascada GuiArbol samples/GuiWinJson.bp samples/GuiShot.bp"

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

# Quita el ruido de arranque/cierre y las lineas en blanco DE LOS BORDES. Las
# de en medio se respetan, y no es un detalle esteta: el volcado de la GUI mete
# el texto del widget tal cual, con sus saltos de linea crudos (un dropdown de
# tres opciones ocupa tres lineas). Borrando todos los blancos, una VM que
# emitiera una linea vacia de mas donde la otra no emite nada saldria VERDE.
filt() {
  grep -vE 'INICIANDO|FIN DE|heapStart|^config:' |
  awk '{
     if (!vista) { if ($0 ~ /^[[:space:]]*$/) next; vista=1 }
     if ($0 ~ /^[[:space:]]*$/) { pend[n++]=$0; next }
     for (i=0;i<n;i++) print pend[i]; n=0; print
   }'
}

# Tope de tiempo por ejecucion. NO es para cortar programas que bloquean a
# proposito: el corpus solo admite casos que TERMINAN SOLOS (los de GUI usan
# Gui.start()/stop(), no Gui.run()). Es la red de seguridad del propio arnes —
# un sample que se cuelgue por un bug deja de wedgear la tanda entera y pasa a
# ser un caso que tarda VM_TIMEOUT, falla y deja seguir a los demas.
# Si un caso se come el timeout, la salida sale cortada y el diff lo canta.
VM_TIMEOUT="${VM_TIMEOUT:-25}"

run_vm() {  # $1=bin(.jar|.exe) $2=mod
  # Se ejecuta DESDE el dir del .mod para que ambas VMs resuelvan las deps
  # (p.ej. Core.mod, dep implicita de try/catch en V3 desde #248) junto al
  # modulo raiz. La VM-C ya resuelve relativo al .mod; el CLI de la VM-Java
  # resuelve relativo al CWD, asi que sin este cd la Java no encontraria Core.
  # Subshell () para no alterar el CWD del propio arnes. Los binarios ($1) son
  # rutas absolutas, siguen resolviendo desde cualquier dir.
  local dir base; dir="$(dirname "$2")"; base="$(basename "$2")"
  # V6/C1 (12-sep) — miVM SIN VENTANA (-Djava.awt.headless): el arnes compara
  # stdout, y un sample con Gui.start() abriria un JFrame en el PC de quien lo
  # corre; ademas asi miVM y la VM-C sin LVGL dicen lo mismo ante Gui.shot
  # («sin pantalla»), que es lo que GuiShot.bp comprueba.
  # V6/G2-4 (11-sep) — el stderr NO se tira entero: el guardian del GC de la
  # miVM ("[gc] !! HEAP INCONSISTENTE") llevaba desde el 15-jul cantando por ahi
  # que el recorrido del heap descarrilaba, y este arnes lo tapaba con 2>/dev/null
  # (57 PASS con el GC de la VM de referencia roto). Un aviso de heap se vuelca
  # como linea de salida: rompe la paridad y se VE en el diff.
  local err="$WORK/.stderr.$$"
  case "$1" in
    *.jar) ( cd "$dir" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$1" "$base" 2>"$err" | filt ) ;;
    *)     ( cd "$dir" && timeout "$VM_TIMEOUT" "$1" "$base" 2>"$err" | filt ) ;;
  esac
  grep -E "HEAP INCONSISTENTE" "$err" 2>/dev/null | head -2 | sed 's/^/!! stderr: /'
  rm -f "$err"
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
  # V6/C1 — el arnes compara contra el host SIN LVGL (sabor gui1-lvgl0): con LVGL
  # la VM-C abre ventana y Gui.shot captura de verdad, y su stdout ya no es el de
  # miVM sin ventana («sin pantalla»). Un FAIL por sabor no es paridad rota: se
  # corta aqui, con nombre, en vez de dejar un rojo que hay que interpretar.
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    echo "  ⚠ el host esta compilado CON LVGL: el arnes necesita 'make GUI=1 LVGL=0' (sabor gui1-lvgl0)"
    return 1
  fi
  local pass=0 fail=0 skip=0 s bp mod oj oc
  for s in $CORPUS; do
    bp="$(bp_path "$s")"
    if [ ! -f "$bp" ]; then
      echo "  SKIP $s (no existe $bp)"; skip=$((skip+1)); continue
    fi
    rm -rf "$WORK"/*.mod "$WORK/src" "$WORK/p.bpbuild"
    # V6/G2-4 — un sample puede necesitar un fichero al lado (GuiWinJson carga
    # el main.win REAL de formdemo). Lo declara el en su cabecera, con
    # `// recurso: ruta/desde/la/raiz`, igual que el modulo raiz se lee del
    # propio .bp: el arnes no sabe de samples concretos.
    sed -nE 's|^//[[:space:]]*recurso:[[:space:]]*([^[:space:]]+).*|\1|p' "$bp" | while read -r rec; do
      cp "$ROOT/$rec" "$WORK/" 2>/dev/null || echo "  (aviso) $s: recurso '$rec' no encontrado"
    done
    # La stdlib FRESCA va al outDir ANTES de compilar, y no es un detalle: el
    # resolutor de imports mira outDir PRIMERO y el directorio del fuente
    # despues. Sin esto, un sample de bpgenvm-c/samples/ resolvia `Core` contra
    # el Core.mod HERMANO de ese directorio — una copia del 18-ago con otra
    # vtable — y luego ejecutaba contra el de la stdlib actual: compilar contra
    # una era y ejecutar contra otra. Asi estuvo el arnes 3 dias en rojo
    # (CastExt/ListaBp/ListaHer pidiendo Integer#value#2 donde la stdlib
    # exporta #7) con un diagnostico equivocado de "stdlib rancia". La stdlib
    # estaba bien; el arnes mezclaba las dos.
    cp "$STDLIB"/*.mod "$WORK/" 2>/dev/null
    if ! java -jar "$V3_FE" "$bp" --compile "$WORK" --backend=mivm >/dev/null 2>&1; then
      # 2o intento POR PROYECTO. Un sample que importa stdlib (Math, IO, ...) no
      # compila con --compile a secas: el frontend no sabe donde buscar, y eso
      # hoy solo se le dice con un .bpbuild. Antes esos samples caian a SKIP, o
      # sea que se quedaban fuera de la red por una limitacion del ARNES y no
      # del sample — justo lo que el arnes existe para no dejar pasar.
      # El `main` sale del `module` de dentro del .bp, NO del nombre del fichero
      # (counter.bp declara CounterSample: usar el fichero daria un falso SKIP).
      local mname
      mname="$(sed -nE 's/^[[:space:]]*module[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\1/p' "$bp" | head -1)"
      if [ -z "$mname" ]; then
        echo "  SKIP $s (no compila y no se le ve el 'module')"; skip=$((skip+1)); continue
      fi
      mkdir -p "$WORK/src"; cp "$bp" "$WORK/src/"
      printf '{ "sourceDir": "src", "outDir": ".", "main": "%s", "dependencies": ["%s"] }\n' \
             "$mname" "$STDLIB" > "$WORK/p.bpbuild"
      if ! ( cd "$WORK" && java -jar "$V3_FE" --project p.bpbuild --backend=mivm ) >/dev/null 2>&1; then
        echo "  SKIP $s (no compila con el frontend actual, ni suelto ni por proyecto)"
        skip=$((skip+1)); continue
      fi
      # Las deps de stdlib se resuelven en EJECUCION junto al .mod (es lo que
      # hace el dispositivo, que las tiene instaladas), asi que van al lado.
      cp "$STDLIB"/*.mod "$WORK/" 2>/dev/null
    fi
    # El .mod raiz se localiza POR NOMBRE DE MODULO, leido del propio .bp.
    #
    # Antes era "el primer .mod que no sea Core", y eso era una MINA: en cuanto
    # la stdlib fresca se copia al WORK antes de compilar (el arreglo de arriba),
    # el primero alfabetico pasa a ser Adc.mod — una libreria sin main — y las
    # dos VMs fallan IGUAL al ejecutarla: 38/38 en verde sin haber ejecutado NI
    # UN sample. El falso-PAR exacto contra el que este arnes advierte.
    local rootname
    rootname="$(sed -nE 's/^[[:space:]]*module[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\1/p' "$bp" | head -1)"
    mod="$WORK/$rootname.mod"
    if [ -z "$rootname" ] || [ ! -f "$mod" ]; then
      echo "  SKIP $s (no se encontro $rootname.mod tras compilar)"; skip=$((skip+1)); continue
    fi
    [ -f "$WORK/Core.mod" ] || cp "$STDLIB/Core.mod" "$WORK/" 2>/dev/null
    oj="$(run_vm "$V3_JAVA" "$mod")"; oc="$(run_vm "$V3_C" "$mod")"
    # Una salida VACIA en las dos VMs no es paridad: es un caso que no ejecuto
    # nada y coincide por casualidad. Importa desde que hay GUI en el corpus,
    # porque esos samples se cortan por VM_TIMEOUT: si el corte llegase antes
    # del dump, las dos saldrian vacias y el arnes lo cantaria VERDE. Es el
    # mismo falso-PAR de "0 PASS no es verde", un piso mas abajo: por caso.
    if [ -z "$oj" ] && [ -z "$oc" ]; then
      echo "  FAIL $s — las dos VMs no imprimieron NADA (no es paridad, es un caso mudo)"
      fail=$((fail+1)); continue
    fi
    if [ "$oj" = "$oc" ]; then
      pass=$((pass+1))
    else
      echo "  FAIL $s — VM-Java != VM-C (paridad rota)"; fail=$((fail+1))
      diff <(printf '%s\n' "$oj") <(printf '%s\n' "$oc") 2>/dev/null | head -6 | sed 's/^/       /'
    fi
  done
  echo "  paridad: $pass PASS, $fail FAIL, $skip SKIP"
  # CERO ejecutados no es verde. Se vio el 25-ago: un fallo del arnes convirtio
  # los 38 en SKIP y el resumen decia VERDE sin haber ejecutado ni un sample.
  # (Ya estaba apuntado de antes: «un SKIP no es un PASS».)
  if [ "$pass" -eq 0 ]; then
    echo "  ⚠ 0 PASS: no se ha ejecutado NADA — eso no es paridad, es un arnes roto"
    return 1
  fi
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
