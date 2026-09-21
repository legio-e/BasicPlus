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
#
# V7/H2 (14-sep-2026) — LA CAPSULA V6, y esta vez la compatibilidad binaria SI se
# sostiene: la subida MOD7 -> MOD8 es ADITIVA por decision de Eduardo («los V7
# tendrian que poderse ejecutar sin problemas»). Se verifica con el ARTEFACTO
# PUBLICADO como oraculo — dist/BasicPlus-6.0-win.zip lleva el compilador y la
# miVM de V6 dentro de BpIde-6.0.jar, y la stdlib MOD7 —, no con lo que haya en
# el arbol de trabajo:
#
#   ./compat.sh gen-v6      Captura la capsula v6/golden: el corpus compilado
#                           por el COMPILADOR PUBLICADO de V6 (MOD7 congelado),
#                           la stdlib MOD7 del ZIP, y la salida de cada .mod con
#                           las dos VMs de hoy (que coinciden entre si o el caso
#                           no entra). Solo se re-ejecuta si cambia el corpus.
#   ./compat.sh check-mod7  Las VMs ACTUALES ejecutan los .mod MOD7 congelados
#                           (sin recompilar nada) y reproducen el .out byte a
#                           byte. Es la prueba de «hacia atras». Entra en check.
#   ./compat.sh check-fwd   Un .mod del compilador ACTUAL en la miVM PUBLICADA
#                           de V6: o corre igual (mismo formato) o lo rechaza
#                           con mensaje; nunca peta. Es la prueba de «hacia
#                           delante» (cuando el compilador emita MOD8, aqui se
#                           exige el rechazo). Entra en check.
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
SAMPLES="$ROOT/bpgenvm-c/samples"
STDLIB="$ROOT/bpstdlib"
GOLD="$HERE/v2/golden"
# V7/H2 (#523 V5/V11, 17-sep) — la paridad principal es MOD9 (lo que emite el
# compilador sin flag, contra la stdlib MOD9 de bpstdlib/) y exige el magic MOD9 en
# cada .mod raiz. check-mod8 es la foto «antes»: la stdlib construida como MOD8 en
# el temporal y el corpus con --formato=8, con paridad Java == C y el stdout IGUAL
# al de la pasada MOD9 (que check guarda). Vive mientras exista --formato=8.
PAR_FORMATO=""   # vacio = el del compilador (9); "8" -> --formato=8 en las dos compilaciones
PAR_SAVE=""      # dir donde guardar <s>.out (la salida de la VM-Java) como referencia
PAR_REF=""       # dir con <s>.out de referencia: un stdout distinto es FAIL (MOD8 != MOD9)
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
# --- Capsula V6: el ZIP publicado (dist/, versionado a proposito) ---
V6_ZIP="$ROOT/dist/BasicPlus-6.0-win.zip"
V6_GOLD="$HERE/v6/golden"
C_OPC="$ROOT/bpgenvm-c/include/bpvm_opcodes.h"

# Corpus: nombres de fichero .bp (sin extension) en bpgenvm-c/samples/.
# Elegidos por cobertura de features V2 y por NO depender de stdlib mas alla de
# Core (import implicito de clases/excepciones). Se amplia en tandas posteriores.
CORPUS="hello arith strings concat charat counter MethodCall trycatch \
        bytetest longtest longarr doubletest powtest casttest utf8test idxtest \
        convtest strops OverloadTest OverloadMethod OverloadCtor SlotPropPriv SlotThreadSub \
        samples/LocalArrTest.bp samples/StrOps348.bp samples/MathOps348.bp samples/PathOps348.bp samples/EvFin.bp samples/ThreadTrasMain.bp samples/ExcDosCatch.bp samples/NullRecv.bp samples/ThreadPrint.bp EnumFirma samples/EnumCadena.bp samples/SyncHijo.bp SyncUsa HomUsa samples/PropRedecl.bp samples/FinallySolo.bp samples/SelfNew.bp samples/ForStep.bp samples/CtorInit.bp VisUsa samples/SyncSinCtor.bp samples/CastBaja.bp samples/MapTipado.bp samples/Destructor.bp samples/TimeDemo.bp         SciPar ArrLitAncho ObjArray CastExt ListaBp ListaHer CastSelf OwnerBp SuperExt samples/BusBug.bp samples/AdcDemo.bp samples/ArgDemo.bp samples/MathRango.bp samples/mathtest.bp samples/NeoCatch.bp samples/WdtCatch.bp samples/StubParidad.bp samples/IoPrompt.bp samples/IoPathAbs.bp samples/ThrowSinAtrapar.bp samples/MachineHost.bp samples/MachineAlias.bp samples/MachineId.bp samples/GuiParidad.bp samples/GuiParidad2.bp GuiChurn OwnerCascada GuiArbol samples/GuiWinJson.bp samples/GuiShot.bp CamposHijo StrNulo EstaticosHijo CtorHijo LibMarcaUsa samples/diamondtest.bp samples/frompathtest.bp samples/fromtest.bp samples/libimport.bp"

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
  # V7/H2 (#493) — el banner se quita CON las lineas en blanco que lo preceden
  # (miVM imprime una antes de «=== INICIANDO»): con un aviso por OUTPUT antes
  # del banner (la carga habla antes de arrancar), ese blanco era una linea de
  # diferencia entre las VMs que no es del programa.
  awk '{
     if ($0 ~ /INICIANDO|FIN DE|heapStart|^config:/) { n=0; next }
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
  local pass=0 fail=0 skip=0 s bp mod oj oc fmt=""
  [ -n "$PAR_FORMATO" ] && fmt="--formato=$PAR_FORMATO"
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
    # V11 (17-sep) — `--stdlibDir=$WORK`: la stdlib es la copiada a $WORK, dicha a mano.
    # Sin esto el frontend autodescubre BpVM.cfg subiendo desde el FUENTE y toma la de
    # bpstdlib/ (#467: la stdlib PRIMERO): en la pasada MOD8 sellaba la interfaz MOD9
    # de bpstdlib/ y la norma rechazaba con razon («no es la interfaz con la que se
    # compilo»). (Compilar una COPIA del .bp en $WORK no vale: siete samples importan
    # a sus hermanos del directorio del fuente.)
    if ! java -jar "$V3_FE" "$bp" --compile "$WORK" --backend=mivm --stdlibDir="$WORK" $fmt >/dev/null 2>&1; then
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
      if ! ( cd "$WORK" && java -jar "$V3_FE" --project p.bpbuild --backend=mivm --stdlibDir="$WORK" $fmt ) >/dev/null 2>&1; then
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
    local rootname rootlib
    rootname="$(sed -nE 's/^[[:space:]]*module[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\1/p' "$bp" | head -1)"
    # V7/H2 (#518) - un modulo con `library "x.y"` se escribe como x.y.Modulo.mod.
    rootlib="$(sed -nE 's/^[[:space:]]*library[[:space:]]+"([A-Za-z0-9_.]+)".*/\1/p' "$bp" | head -1)"
    mod="$WORK/$rootname.mod"
    [ -n "$rootlib" ] && mod="$WORK/$rootlib.$rootname.mod"
    if [ -z "$rootname" ] || [ ! -f "$mod" ]; then
      echo "  SKIP $s (no se encontro $rootname.mod tras compilar)"; skip=$((skip+1)); continue
    fi
    [ -f "$WORK/Core.mod" ] || cp "$STDLIB/Core.mod" "$WORK/" 2>/dev/null
    # V7/H2 (#523 V5) — el ARTEFACTO, no el log: el .mod raiz tiene que llevar el
    # magic del formato de la pasada (el del compilador, 9, o el pedido con $fmt).
    # Un $fmt ignorado por el frontend daria una pasada «MOD8» que en realidad
    # repite la de MOD9, en verde.
    if [ "$(head -c 4 "$mod")" != "MOD${PAR_FORMATO:-9}" ]; then
      echo "  FAIL $s — el .mod es $(head -c 4 "$mod") y no MOD${PAR_FORMATO:-9}: el arnes no compilo lo que dice"
      fail=$((fail+1)); continue
    fi
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
      # V7/H2 (#523 V5) — paridad no basta: en MOD9 el stdout tiene que ser EL DE
      # MOD8. Un sample que en MOD9 imprimiera otra cosa con las dos VMs de
      # acuerdo seria un falso-PAR de otro piso: el enlace tardio cambiando la
      # semantica sin que nadie lo oiga.
      local outname="${s//\//_}.out"
      if [ -n "$PAR_REF" ] && [ -f "$PAR_REF/$outname" ] && [ "$oj" != "$(cat "$PAR_REF/$outname")" ]; then
        echo "  FAIL $s — paridad OK pero el stdout NO es el de la referencia (la pasada MOD9)"; fail=$((fail+1))
        diff "$PAR_REF/$outname" <(printf '%s\n' "$oj") 2>/dev/null | head -6 | sed 's/^/       /'
        continue
      fi
      [ -n "$PAR_SAVE" ] && printf '%s\n' "$oj" > "$PAR_SAVE/$outname"
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

# ------------------------------------------------------------- MOD8 (V7/H2 #523 V5/V11)
# La foto «antes». Desde el 17-sep (V11) el compilador emite MOD9 y la paridad
# principal ya es MOD9 contra la stdlib MOD9 de bpstdlib/; este frente construye
# la stdlib ENTERA como MOD8 en $WORK y compila el corpus con --formato=8,
# exigiendo paridad Java == C y el mismo stdout que la pasada MOD9, sample a
# sample: «lo que corre en MOD9 corre igual en MOD8» — el emisor MOD8 sigue vivo
# mientras exista --formato=8 (salida de emergencia hasta cerrar V7).
build_stdlib_fmt() {   # $1 = formato
  local d="$WORK/std$1"; rm -rf "$d"; mkdir -p "$d/src"
  cp "$STDLIB"/*.bp "$d/src/"
  printf '{ "sourceDir": "src", "outDir": ".", "main": "Stdlib", "dependencies": [] }\n' > "$d/p.bpbuild"
  if ! ( cd "$d" && java -jar "$V3_FE" --project p.bpbuild --backend=mivm --formato=$1 ) > "$d/build.log" 2>&1; then
    echo "  la stdlib NO compila como MOD$1:"; tail -5 "$d/build.log" | sed 's/^/       /'; return 1
  fi
  echo "  stdlib MOD$1: $(ls "$d"/*.mod | wc -l) modulos (magic $(head -c 4 "$d/Core.mod"))"
}
REF_PAR="$WORK/ref_par"   # el stdout de cada sample en la pasada principal (MOD9); lo guarda check_parity con PAR_SAVE
check_mod8() {   # $1 = "sin-referencia" cuando check ya hizo la pasada MOD9 guardando en $REF_PAR
  echo "-- el corpus en MOD8 (stdlib MOD8 + --formato=8: paridad Java == C y el stdout de la pasada MOD9) --"
  local ref="$REF_PAR" rc std_old; mkdir -p "$ref"
  if [ "${1:-}" != "sin-referencia" ]; then
    echo "  [MOD9: la referencia]"
    PAR_SAVE="$ref"; check_parity; rc=$?; PAR_SAVE=""
    [ "$rc" -eq 0 ] || return 1
  elif [ -z "$(ls "$ref"/*.out 2>/dev/null)" ]; then
    echo "  no hay referencia en $ref: la pasada MOD9 no guardo nada"; return 1
  fi
  build_stdlib_fmt 8 || return 1
  echo "  [MOD8]"
  std_old="$STDLIB"; STDLIB="$WORK/std8"; PAR_FORMATO=8; PAR_REF="$ref"
  check_parity; rc=$?
  STDLIB="$std_old"; PAR_FORMATO=""; PAR_REF=""
  echo "  mod8: $([ "$rc" -eq 0 ] && echo 'el corpus corre en MOD8 como en MOD9' || echo 'MOD8 en ROJO')"
  return "$rc"
}

# ------------------------------------------------------------- capsula V6
# Saca de dist/BasicPlus-6.0-win.zip lo que hace falta: BpIde-6.0.jar (lleva el
# compilador basicplus.frontend.Main y la miVM edu.bpgenvm.Main de V6) y la
# stdlib MOD7. Se extrae a $WORK/v6 cada vez: el ZIP es el oraculo, no una copia.
v6_extraer() {
  [ -f "$V6_ZIP" ] || { echo "  no existe $V6_ZIP (el ZIP publicado de V6 es el oraculo)"; return 1; }
  mkdir -p "$WORK/v6"
  ( cd "$WORK/v6" && unzip -q -o "$V6_ZIP" 'BasicPlus-6.0-win/BpIde-6.0.jar' 'BasicPlus-6.0-win/bpstdlib/*.mod' ) || return 1
  V6_JAR="$WORK/v6/BasicPlus-6.0-win/BpIde-6.0.jar"
  V6_STD="$WORK/v6/BasicPlus-6.0-win/bpstdlib"
  [ -f "$V6_JAR" ] && ls "$V6_STD"/*.mod >/dev/null 2>&1
}

# Compila un item del corpus con el compilador de V6 (suelto o por proyecto,
# como check_parity) y deja el .mod raiz en $1. Devuelve el nombre del modulo.
v6_compilar() {  # $1=dir de trabajo  $2=item del corpus
  local dir="$1" s="$2" bp mname
  bp="$(bp_path "$s")"
  [ -f "$bp" ] || return 1
  rm -rf "$dir"/*.mod "$dir/src" "$dir/p.bpbuild"
  sed -nE 's|^//[[:space:]]*recurso:[[:space:]]*([^[:space:]]+).*|\1|p' "$bp" | while read -r rec; do
    cp "$ROOT/$rec" "$dir/" 2>/dev/null
  done
  mname="$(sed -nE 's/^[[:space:]]*module[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\1/p' "$bp" | head -1)"
  [ -n "$mname" ] || return 1
  cp "$V6_STD"/*.mod "$dir/" 2>/dev/null
  if ! java -cp "$V6_JAR" basicplus.frontend.Main "$bp" --compile "$dir" --backend=mivm >/dev/null 2>&1; then
    mkdir -p "$dir/src"; cp "$bp" "$dir/src/"
    printf '{ "sourceDir": "src", "outDir": ".", "main": "%s", "dependencies": ["%s"] }\n' "$mname" "$V6_STD" > "$dir/p.bpbuild"
    ( cd "$dir" && java -cp "$V6_JAR" basicplus.frontend.Main --project p.bpbuild --backend=mivm ) >/dev/null 2>&1 || return 1
  fi
  [ -f "$dir/$mname.mod" ] || return 1
  echo "$mname"
}

gen_v6() {
  echo "== gen-v6: capsula V6 — el corpus compilado por el compilador PUBLICADO (MOD7), salida con las VMs de hoy =="
  v6_extraer || return 1
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    echo "  ⚠ el host esta compilado CON LVGL: hace falta 'make GUI=1 LVGL=0' (sabor gui1-lvgl0)"; return 1
  fi
  rm -rf "$V6_GOLD"; mkdir -p "$V6_GOLD/stdlib"
  cp "$V6_STD"/*.mod "$V6_GOLD/stdlib/"
  local ok=0 skip=0 s mname oj oc dir="$WORK/g6"
  mkdir -p "$dir"
  for s in $CORPUS; do
    mname="$(v6_compilar "$dir" "$s")" || { echo "  SKIP $s (no compila con el compilador de V6)"; skip=$((skip+1)); continue; }
    oj="$(run_vm "$V3_JAVA" "$dir/$mname.mod")"; oc="$(run_vm "$V3_C" "$dir/$mname.mod")"
    if [ -z "$oj" ] && [ -z "$oc" ]; then echo "  SKIP $s (caso mudo)"; skip=$((skip+1)); continue; fi
    if [ "$oj" != "$oc" ]; then echo "  SKIP $s (las dos VMs de hoy no coinciden: fuera de la capsula)"; skip=$((skip+1)); continue; fi
    cp "$dir/$mname.mod" "$V6_GOLD/$mname.mod"; printf '%s\n' "$oj" > "$V6_GOLD/$mname.out"
    # Las DEPENDENCIAS hermanas que el compilador produjo al lado (SuperExt importa
    # SuperExtBase, CastExt su base...) viajan tambien, sin .out: son librerias.
    # Sin esto el golden corre en gen (el dir las acumula) y falla en check.
    local dep
    for dep in "$dir"/*.mod; do
      [ -f "$V6_STD/$(basename "$dep")" ] && continue
      [ -f "$V6_GOLD/$(basename "$dep")" ] || cp "$dep" "$V6_GOLD/"
    done
    # los recursos que el sample declara viajan con el golden (GuiWinJson carga main.win)
    sed -nE 's|^//[[:space:]]*recurso:[[:space:]]*([^[:space:]]+).*|\1|p' "$(bp_path "$s")" | while read -r rec; do
      cp "$ROOT/$rec" "$V6_GOLD/" 2>/dev/null
    done
    ok=$((ok+1))
  done
  echo "  magic de la capsula: $(head -c 4 "$V6_GOLD/stdlib/Core.mod")  ($(ls "$V6_GOLD"/*.out | wc -l) casos, $(ls "$V6_GOLD"/*.mod | wc -l) modulos, $(ls "$V6_GOLD"/stdlib/*.mod | wc -l) de stdlib)"
  echo "== gen-v6: $ok goldens, $skip omitidos =="
}

# Las VMs de HOY ejecutan cada .mod CONGELADO (compilado por V6, MOD7) con la
# stdlib MOD7 congelada al lado, y tienen que reproducir el .out byte a byte.
# Cuando las VMs hablen MOD8, esto es exactamente «los V7 corren sin problemas».
check_mod7() {
  echo "-- hacia atras: las VMs actuales ejecutan los .mod MOD7 congelados de la capsula V6 --"
  if ! ls "$V6_GOLD"/*.mod >/dev/null 2>&1; then
    echo "  no hay capsula: ./compat.sh gen-v6 primero"; return 1
  fi
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    echo "  ⚠ el host esta compilado CON LVGL: hace falta 'make GUI=1 LVGL=0' (sabor gui1-lvgl0)"; return 1
  fi
  local pass=0 fail=0 g base dir="$WORK/c7" oj oc esperado
  rm -rf "$dir"; mkdir -p "$dir"
  # la stdlib congelada y TODOS los .mod de la capsula (los que no tienen .out son
  # librerias hermanas de algun caso); los recursos (.win) tambien
  cp "$V6_GOLD"/stdlib/*.mod "$V6_GOLD"/*.mod "$dir/"; cp "$V6_GOLD"/*.win "$dir/" 2>/dev/null
  for g in "$V6_GOLD"/*.out; do
    base="$(basename "$g" .out)"
    esperado="$(cat "$g")"
    oj="$(run_vm "$V3_JAVA" "$dir/$base.mod")"; oc="$(run_vm "$V3_C" "$dir/$base.mod")"
    if [ "$oj" = "$esperado" ] && [ "$oc" = "$esperado" ]; then
      pass=$((pass+1))
    else
      fail=$((fail+1)); echo "  FAIL $base — un MOD7 congelado ya no da lo mismo:"
      [ "$oj" != "$esperado" ] && diff <(printf '%s\n' "$esperado") <(printf '%s\n' "$oj") | head -4 | sed 's/^/       java: /'
      [ "$oc" != "$esperado" ] && diff <(printf '%s\n' "$esperado") <(printf '%s\n' "$oc") | head -4 | sed 's/^/       c:    /'
    fi
  done
  echo "  hacia atras: $pass PASS, $fail FAIL (capsula $(head -c 4 "$V6_GOLD/stdlib/Core.mod"))"
  [ "$pass" -gt 0 ] && [ "$fail" -eq 0 ]
}

# Un .mod del compilador ACTUAL en la miVM PUBLICADA de V6. Con el mismo formato
# corre igual; con un formato mas nuevo la VM vieja tiene que RECHAZARLO con un
# mensaje que lo diga — nunca petar ni cargarlo mal. Se mira el codigo de salida
# y que, si no corre, la salida nombre el formato.
check_fwd() {
  echo "-- hacia delante: un .mod del compilador actual en la miVM publicada de V6 --"
  v6_extraer || return 1
  local dir="$WORK/fw" out rc magic
  rm -rf "$dir"; mkdir -p "$dir"
  cp "$V6_STD"/*.mod "$dir/"
  java -jar "$V3_FE" "$SAMPLES/hello.bp" --compile "$dir" --backend=mivm >/dev/null 2>&1 || { echo "  hello.bp no compila con el compilador actual"; return 1; }
  magic="$(head -c 4 "$dir/Hello.mod")"
  # V7/H2 (15-sep) — EL .mod recien compilado, POR NOMBRE. Antes se elegia «el primer
  # .mod que no sea Core/Str/IO/Math» sobre un directorio que lleva los 28 de la
  # stdlib copiada: salia Adc.mod (de V6, sin main), la miVM de V6 lo «ejecutaba» y
  # el check estaba verde sin probar nada (lo cazo un refutador del mapa de MOD8).
  out="$( cd "$dir" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -cp "$V6_JAR" edu.bpgenvm.Main Hello.mod 2>&1 )"; rc=$?
  if [ "$rc" -eq 0 ]; then
    echo "  la miVM de V6 EJECUTA el .mod actual (magic $magic): mismo formato, hacia delante no hay nada que rechazar"
    [ "$magic" = "MOD7" ] && return 0
    echo "  ✗ pero el magic no es MOD7: una VM vieja NO deberia ejecutar un formato que no conoce"; return 1
  fi
  if [ "$magic" = "MOD7" ]; then
    echo "  ✗ la miVM de V6 RECHAZA un MOD7 (rc=$rc), que es su propio formato:"; printf '%s\n' "$out" | head -3 | sed 's/^/       /'; return 1
  fi
  if printf '%s' "$out" | grep -qiE 'MOD[0-9]|formato|magic|version'; then
    echo "  la miVM de V6 RECHAZA el .mod actual (magic $magic) con mensaje: $(printf '%s' "$out" | grep -iE 'MOD[0-9]|formato|magic|version' | head -1)"
    return 0
  fi
  echo "  ✗ la miVM de V6 falla (rc=$rc) SIN decir que es el formato:"; printf '%s\n' "$out" | head -5 | sed 's/^/       /'
  return 1
}

# ── V7/H2 (#500) — LA NORMA DE VERSIONES, con Core de verdad, en las DOS VMs ────
# Cuatro stdlibs fabricadas como proyecto a partir de bpstdlib/*.bp (version 6, 7,
# 7 con un metodo mas al final de List, 8, y 8 con un metodo INSERTADO en medio de
# List), dos apps compiladas contra la de version 7 (NormaUsa: usa List; NormaExt:
# extiende List), y cada combinacion ejecutada en miVM y en la VM-C: el stdout
# filtrado tiene que ser IDENTICO entre VMs Y contener el texto esperado (o la
# salida normal cuando se acepta). Es la prueba de que «compilar contra una era y
# ejecutar contra otra» ya no pasa en silencio (V7_IDEAS §H2, apartado 4.3).
norma_stdlib() {  # $1=dir destino  $2=version  [$3=sed sobre Core.bp]
  local d="$1"
  rm -rf "$d"; mkdir -p "$d/src"
  cp "$STDLIB"/*.bp "$d/src/"
  [ -n "${3:-}" ] && sed -i "$3" "$d/src/Core.bp"
  printf '{ "sourceDir": "src", "outDir": ".", "main": "Stdlib", "version": %s, "dependencies": [] }\n' "$2" > "$d/p.bpbuild"
  ( cd "$d" && java -jar "$V3_FE" --project p.bpbuild --backend=mivm --formato=8 ) >"$d/build.log" 2>&1 || { echo "  ✗ la stdlib de prueba (version $2) no compila: $d/build.log"; return 1; }
}
norma_caso() {  # $1=etiqueta $2=dir con app+stdlib $3=modulo $4=texto esperado en stdout (regex) | "" = debe correr
  local et="$1" dir="$2" mod="$3" esperado="$4" oj oc
  oj="$(run_vm "$V3_JAVA" "$dir/$mod.mod")"
  oc="$(run_vm "$V3_C"    "$dir/$mod.mod")"
  if [ "$oj" != "$oc" ]; then
    echo "  ✗ $et: las VMs DIFIEREN"; echo "     java: $(printf '%s' "$oj" | head -3 | tr '\n' '|')"; echo "     c:    $(printf '%s' "$oc" | head -3 | tr '\n' '|')"; return 1
  fi
  if [ -n "$esperado" ]; then
    printf '%s' "$oj" | grep -qE "$esperado" || { echo "  ✗ $et: no dice lo esperado (/$esperado/):"; printf '%s\n' "$oj" | head -3 | sed 's/^/       /'; return 1; }
    echo "  ok  $et: $(printf '%s' "$oj" | grep -E "$esperado" | head -1 | cut -c1-110)"
  else
    printf '%s' "$oj" | grep -qE "mas antiguo|no es la interfaz|mas reciente|ERROR DE LINK" && { echo "  ✗ $et: deberia correr y rechaza:"; printf '%s\n' "$oj" | head -3 | sed 's/^/       /'; return 1; }
    echo "  ok  $et: corre ($(printf '%s' "$oj" | tr '\n' ' ' | cut -c1-60))"
  fi
}
# ---------------------------------------------------------------- check-sombra
# V7/H2 (#493) — LA SOMBRA: el orden de resolucion de modulos, UNO en las dos VMs
# (Eduardo, 15-sep: «/app -> /lib -> packs; si el principal esta en un pack,
# pack del principal -> /app -> /lib -> packs»), y el aviso por OUTPUT cuando
# un modulo de /app TAPA a uno de /lib (criterio «gemelo tapado»). En el PC,
# /app es la carpeta del programa y /lib es --stdlibDir (miVM) / --libdir (VM-C).
# El cwd NO cuenta (rompia la paridad: un .mod homonimo en el directorio actual
# lo cogia miVM y la VM-C no). La zona de packs solo la tiene la VM-C: ahi se
# comprueba que el modulo de un pack se carga XIP, que /lib lo eclipsa, y que un
# principal que solo vive en la zona arranca con su pack primero.
sombra_vm() {  # $1=jar|exe $2=cwd $3=libdir $4=mod [$5=args extra VM-C]
  local bin="$1" cwd="$2" lib="$3" mod="$4" extra="${5:-}" err="$WORK/.sombra.err"
  case "$bin" in
    *.jar) ( cd "$cwd" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$bin" --no-config ${lib:+--stdlibDir=$lib} "$mod" 2>"$err" | filt ) ;;
    *)     ( cd "$cwd" && timeout "$VM_TIMEOUT" "$bin" ${lib:+--libdir=$lib} $extra "$mod" 2>"$err" | filt ) ;;
  esac
}
sombra_caso() {  # $1=etiqueta $2=cwd $3=libdir $4=mod $5=regex que DEBE salir $6=regex que NO debe salir
  local et="$1" cwd="$2" lib="$3" mod="$4" si="$5" no="$6" oj oc
  oj="$(sombra_vm "$V3_JAVA" "$cwd" "$lib" "$mod")"
  oc="$(sombra_vm "$V3_C"    "$cwd" "$lib" "$mod")"
  if [ "$oj" != "$oc" ]; then
    echo "  ✗ $et: las VMs DIFIEREN"; echo "     java: $(printf '%s' "$oj" | head -3 | tr '\n' '|')"; echo "     c:    $(printf '%s' "$oc" | head -3 | tr '\n' '|')"; return 1
  fi
  printf '%s' "$oj" | grep -qE "$si" || { echo "  ✗ $et: falta /$si/:"; printf '%s\n' "$oj" | head -4 | sed 's/^/       /'; return 1; }
  if [ -n "$no" ] && printf '%s' "$oj" | grep -qE "$no"; then echo "  ✗ $et: sale /$no/ y no debia:"; printf '%s\n' "$oj" | head -4 | sed 's/^/       /'; return 1; fi
  echo "  ok  $et: $(printf '%s' "$oj" | grep -E "$si" | head -1 | cut -c1-100)"
}
check_sombra() {
  echo "-- la sombra (#493): /app -> /lib -> packs, igual en las dos VMs; el aviso «tapa al de /lib» --"
  local S="$WORK/sombra" fails=0 PACKJAR="$ROOT/pack/target/pack-1.0.jar"
  rm -rf "$S"; mkdir -p "$S/src" "$S/var" "$S/app" "$S/lib" "$S/lib2" "$S/cwd" "$S/vacio"
  cp "$STDLIB"/*.mod "$S/lib/"; cp "$STDLIB/Core.mod" "$S/lib2/"; cp "$STDLIB"/*.mod "$S/cwd/"
  # La Str VARIANTE: el mismo fuente con toHex marcado. Se compila a var/ y se
  # copia junto al programa que la importa con `from`, para que el compilador
  # la tome a ella (y el sello de la app sea el suyo) sin depender de mtimes.
  sed 's/^    return sb.toString()$/    return "VARIANTE:" + sb.toString()/' "$STDLIB/Str.bp" > "$S/var/Str.bp"
  ( cd "$ROOT" && java -jar "$V3_FE" "$S/var/Str.bp" --compile "$S/var" --backend=mivm ) >"$S/var.log" 2>&1 \
    || { echo "  ✗ la Str variante no compila: $S/var.log"; return 1; }
  cp "$S/var/Str.mod" "$S/src/Str.mod"
  cat > "$S/src/Sombra.bp" <<'EOF'
module Sombra
  import Str from "Str.mod"
  public function main()
    print "toHex(255) = " + Str.toHex(255L)
  end main
end Sombra
EOF
  cat > "$S/src/Limpio.bp" <<'EOF'
module Limpio
  import Str
  public function main()
    print "toHex(255) = " + Str.toHex(255L)
  end main
end Limpio
EOF
  ( cd "$ROOT" && java -jar "$V3_FE" "$S/src/Sombra.bp" --compile "$S/app" --backend=mivm && java -jar "$V3_FE" "$S/src/Limpio.bp" --compile "$S/app" --backend=mivm ) >"$S/app.log" 2>&1 \
    || { echo "  ✗ los programas no compilan: $S/app.log"; return 1; }
  rm -f "$S/app/Str.mod"
  local AV="aviso: 'Str' cargado desde /app, tapa al de /lib"
  # 1) sin gemelo: Str solo en /lib -> corre, sin aviso, en las dos.
  sombra_caso "solo /lib" "$S" lib app/Limpio.mod "toHex\(255\) = ff" "aviso" || fails=$((fails+1))
  # 2) la sombra: la variante en /app y la real en /lib -> gana /app y las DOS avisan igual.
  cp "$S/var/Str.mod" "$S/app/Str.mod"
  sombra_caso "app tapa a lib (aviso)" "$S" lib app/Sombra.mod "$AV" "" || fails=$((fails+1))
  sombra_caso "app tapa a lib (corre la de /app)" "$S" lib app/Sombra.mod "toHex\(255\) = VARIANTE:ff" "" || fails=$((fails+1))
  # 3) el mismo caso desde OTRO cwd (el de /lib): mismo resultado — el cwd no cambia nada.
  sombra_caso "mismo resultado desde otro cwd" "$S/lib" . ../app/Sombra.mod "$AV" "" || fails=$((fails+1))
  # 4) sin /lib configurado, la stdlib ENTERA en el cwd NO cuenta: las dos dicen que falta Str.
  rm -f "$S/app/Str.mod"
  { local oj oc
    oj="$( cd "$S/cwd" && java -Djava.awt.headless=true -jar "$V3_JAVA" --no-config ../app/Limpio.mod 2>&1 | grep -oE "falta el modulo 'Str'" | head -1 )"
    oc="$( cd "$S/cwd" && "$V3_C" ../app/Limpio.mod 2>&1 | grep -oE "falta el modulo 'Str'" | head -1 )"
    if [ "$oj" = "falta el modulo 'Str'" ] && [ "$oc" = "$oj" ]; then echo "  ok  el cwd no cuenta: las dos dicen «$oj»"
    else echo "  ✗ el cwd no cuenta: java=[$oj] c=[$oc]"; fails=$((fails+1)); fi
  }
  # 5) la zona de packs (solo VM-C): la Str del pack se carga XIP cuando /lib no la tiene...
  if [ -f "$PACKJAR" ]; then
    ( cd "$S/var" && java -cp "$PACKJAR" basicplus.pack.PackCli build --out "$S/StrPk.pack" --name StrPk --version 1 --date 0 Str.mod \
      && java -cp "$PACKJAR" basicplus.pack.PackCli build --out "$S/AppPk.pack" --name AppPk --version 1 --date 0 "$S/app/Sombra.mod" Str.mod ) >"$S/pack.log" 2>&1 \
      || { echo "  ✗ no se pudo construir el pack: $S/pack.log"; return 1; }
    local o e
    o="$( cd "$S" && "$V3_C" --libdir=lib2 --pack=StrPk.pack app/Sombra.mod 2>"$S/xip.err" | filt )"
    if printf '%s' "$o" | grep -q "VARIANTE:ff" && grep -q "cargado XIP desde pack" "$S/xip.err"; then echo "  ok  zona (VM-C): sin /lib/Str.mod, la Str del pack se carga XIP"
    else echo "  ✗ zona: esperaba XIP y VARIANTE:"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; grep -E "dep 'Str'|XIP" "$S/xip.err" | sed 's/^/       /'; fails=$((fails+1)); fi
    # ...y /lib la ECLIPSA cuando la tiene (sin aviso: /lib es la stdlib), y lo dice por el diag.
    o="$( cd "$S" && "$V3_C" --libdir=lib --pack=StrPk.pack app/Limpio.mod 2>"$S/ecl.err" | filt )"
    if printf '%s' "$o" | grep -q "toHex(255) = ff" && ! printf '%s' "$o" | grep -q "aviso" && grep -q "eclipsa al del pack" "$S/ecl.err"; then echo "  ok  zona (VM-C): /lib eclipsa al pack (y el diag lo dice)"
    else echo "  ✗ zona: /lib deberia eclipsar al pack:"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; fails=$((fails+1)); fi
    # ...y un principal que SOLO vive en la zona arranca XIP con su pack primero.
    o="$( cd "$S/vacio" && "$V3_C" --libdir=../lib2 --pack=../AppPk.pack Sombra.mod 2>"$S/zona.err" | filt )"
    if printf '%s' "$o" | grep -q "VARIANTE:ff" && grep -q "cargado XIP desde el pack 'AppPk'" "$S/zona.err" && grep -q "dep 'Str' -> pack EN EJECUCION" "$S/zona.err"; then echo "  ok  zona (VM-C): el principal que solo vive en la zona corre XIP, su pack primero"
    else echo "  ✗ zona: el principal de la zona:"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; grep -E "dep |XIP|load" "$S/zona.err" | sed 's/^/       /'; fails=$((fails+1)); fi
  else
    echo "  (sin $PACKJAR: los casos de la zona no se prueban)"
  fi
  # 9) DOS incumplimientos de la norma (#500) en cadena: Sombra -> Str (otra
  #    interfaz: la variante con una funcion mas) y Str -> Core (un Core con
  #    otra interfaz en /lib). Las dos VMs tienen que nombrar el PRIMERO, en el
  #    mismo orden de descubrimiento: miVM recurria antes de comprobar y
  #    nombraba el mas profundo (medido el 15-sep en el mapa de #493).
  mkdir -p "$S/var2" "$S/lib8/src"
  sed 's/^end Str$/  public function sombraExtra(): integer\n    return 9\n  end sombraExtra\nend Str/' "$S/var/Str.bp" > "$S/var2/Str.bp"
  cp "$STDLIB/Core.bp" "$S/lib8/src/Core.bp"
  sed -i 's/^  end List$/    public function normaNuevo(): integer\n      return 1\n    end normaNuevo\n  end List/' "$S/lib8/src/Core.bp"
  cp "$S/lib"/*.mod "$S/lib8/"
  # V11 (17-sep): los dos duenos de la cadena se compilan como MOD8 a proposito — el caso es
  # de la regla 2 de la norma #500 (misma gen, otra interfaz), que solo rige para duenos MOD7/8;
  # con duenos MOD9 no hay incumplimiento que nombrar: decide el enlace tardio.
  ( cd "$ROOT" && java -jar "$V3_FE" "$S/var2/Str.bp" --compile "$S/var2" --backend=mivm --formato=8 && java -jar "$V3_FE" "$S/lib8/src/Core.bp" --compile "$S/lib8" --backend=mivm --formato=8 ) >"$S/cadena.log" 2>&1 \
    || { echo "  ✗ la cadena no compila: $S/cadena.log"; return 1; }
  cp "$S/var2/Str.mod" "$S/app/Str.mod"
  sombra_caso "dos incumplimientos en cadena: el primero" "$S" lib8 app/Sombra.mod "'Str' \(gen 0, interfaz [0-9a-f]+\) no es la interfaz con la que se compilo 'Sombra'" "compilo 'Str'" || fails=$((fails+1))
  rm -f "$S/app/Str.mod"
  # 10) el PRINCIPAL vive en /lib (o el /lib del PC es la carpeta del programa): la
  #     copia es LA MISMA, no tapa nada, y no se avisa en ninguna de las dos.
  cp "$S/app/Limpio.mod" "$S/lib/Limpio.mod"
  sombra_caso "el principal en /lib: sin aviso" "$S" lib lib/Limpio.mod "toHex\(255\) = ff" "aviso" || fails=$((fails+1))
  rm -f "$S/lib/Limpio.mod"
  # 11) LOS RECURSOS DE LA ZONA por los cuatro lectores (VM-C): readFile, fileExists,
  #     IO.fileSize — la revision cazo que readFile los veia y fileExists no.
  if [ -f "$PACKJAR" ]; then
    mkdir -p "$S/res"
    printf 'DE-LA-ZONA' > "$S/res/datos.txt"
    cat > "$S/src/Recurso.bp" <<'EOF'
module Recurso
  import Core
  import IO
  public function main()
    print "existe " + fileExists("datos.txt")
    print "tam " + IO.fileSize("datos.txt")
    print "lee " + readFile("datos.txt")
  end main
end Recurso
EOF
    ( cd "$ROOT" && java -jar "$V3_FE" "$S/src/Recurso.bp" --compile "$S/app" --backend=mivm ) >"$S/rec.log" 2>&1 \
      || { echo "  ✗ Recurso.bp no compila: $S/rec.log"; return 1; }
    ( cd "$S/res" && java -cp "$PACKJAR" basicplus.pack.PackCli build --out "$S/ResPk.pack" --name ResPk --version 1 --date 0 datos.txt ) >>"$S/pack.log" 2>&1 \
      || { echo "  ✗ no se pudo construir ResPk.pack"; return 1; }
    local o
    o="$( cd "$S" && "$V3_C" --libdir=lib --pack=ResPk.pack app/Recurso.mod 2>/dev/null | filt )"
    if printf '%s' "$o" | grep -q "existe true" && printf '%s' "$o" | grep -q "tam 10" && printf '%s' "$o" | grep -q "lee DE-LA-ZONA"; then
      echo "  ok  recursos de la zona (VM-C): fileExists, fileSize y readFile ven el mismo fichero"
    else echo "  ✗ recursos de la zona:"; printf '%s\n' "$o" | head -4 | sed 's/^/       /'; fails=$((fails+1)); fi
    # 12) EL WIRE contra el micro simulado (la cintura de las placas): STAT name= del
    #     principal que solo vive en la zona, RUN por nombre y por pack:, y el overlay
    #     de un pack que no arranca. Ocho comprobaciones en compat/sim_sombra.py.
    local SIMEXE="$ROOT/bpgenvm-c/build/bpvm-sim.exe"
    if [ -x "$SIMEXE" ]; then
      local sal
      sal="$( python "$HERE/sim_sombra.py" "$SIMEXE" "$S/AppPk.pack" "$STDLIB/Core.mod" AppPk Sombra "VARIANTE:ff" --port=5461 2>&1 )"
      if printf '%s' "$sal" | grep -q "^\[status=OK\]"; then echo "  ok  el wire (sim): $(printf '%s' "$sal" | grep -c '  ok  ') comprobaciones (STAT/RUN del principal de la zona, pack:, overlay muerto)"
      else echo "  ✗ el wire (sim):"; printf '%s\n' "$sal" | grep -E "FAIL|Traceback|Error" | head -4 | sed 's/^/       /'; fails=$((fails+1)); fi
    else
      echo "  (sin $SIMEXE: el caso del wire no se prueba — make sim)"
    fi
  fi
  # V7/#520 (a), 18-sep — el ESQUEMA `pack:<Pack>/x` en un pack EJECUTABLE: el recurso
  # cualificado del propio pack se lee; el de OTRO pack (`pack:Otro/x`) NO es mio y sigue
  # al FS, donde no existe (fileExists false; readFile «no se pudo abrir»); el pelado sale
  # del pack. miVM ignoraba el esquema (contestaba por el basename) y ademas reventaba
  # en Windows con el ':' («Illegal char <:>»). Mismo stdout en las dos VMs.
  local R="$S/packres"; rm -rf "$R"; mkdir -p "$R"; cp "$STDLIB/Core.mod" "$R/"
  printf 'module PackRes\nimport Core\nfunction main()\n  print "propio: " + readFile("pack:PackRes/hola.txt")\n  print "otro existe: " + fileExists("pack:Otro/hola.txt")\n  print "pelado: " + readFile("hola.txt")\n  try\n    print readFile("pack:Otro/hola.txt")\n  catch e: RuntimeError\n    print "otro: " + e.msg\n  endtry\n  print "fin"\nend main\nend PackRes\n' > "$R/PackRes.bp"
  printf 'hola desde el pack' > "$R/hola.txt"; printf 'main=PackRes\n' > "$R/manifest.mft"
  if ( cd "$R" && java -jar "$V3_FE" PackRes.bp --compile . --backend=mivm --stdlibDir=. && java -cp "$PACKJAR" basicplus.pack.PackCli build --out PackRes.pack --name PackRes --version 1 --date 0 PackRes.mod Core.mod hola.txt manifest.mft ) >"$R/build.log" 2>&1; then
    rm -f "$R/hola.txt" "$R/PackRes.mod" "$R/manifest.mft"
    local pj pc esp
    pj="$(run_vm "$V3_JAVA" "$R/PackRes.pack")"; pc="$(run_vm "$V3_C" "$R/PackRes.pack")"
    esp="$(printf 'propio: hola desde el pack\notro existe: false\npelado: hola desde el pack\notro: readFile('"'"'pack:Otro/hola.txt'"'"'): no se pudo abrir\nfin')"
    if [ "$pj" = "$pc" ] && [ "$pj" = "$esp" ]; then echo "  ok  el esquema pack:<Pack>/x (propio, ajeno, pelado): mismo stdout en las dos VMs"
    else echo "  ✗ el esquema pack:"; echo "     java: $(printf '%s' "$pj" | tr '\n' '|' | cut -c1-160)"; echo "     c:    $(printf '%s' "$pc" | tr '\n' '|' | cut -c1-160)"; fails=$((fails+1)); fi
  else
    echo "  ✗ el esquema pack: no se pudo construir el pack ($R/build.log)"; fails=$((fails+1))
  fi
  echo "  sombra: $((13 - fails))/13 casos"
  [ "$fails" -eq 0 ]
}

# V7/H2 (#523 V11, 17-sep) — la norma de las TRES reglas es la de los duenos
# MOD7/MOD8 (con un dueno MOD9 solo queda la regla 1, E0, y decide el enlace
# tardio: eso lo mide compat/mezcla.py). Por eso este frente compila todo con
# --formato=8 mientras el emisor MOD8 exista; el dia que se quite, se va con el.
check_norma() {
  echo "-- la norma de versiones (#500): Core de verdad, dos VMs, mismo texto --"
  local N="$WORK/norma" fails=0
  rm -rf "$N"; mkdir -p "$N"
  # La app: USA List (NormaUsa) y EXTIENDE List (NormaExt).
  cat > "$N/NormaUsa.bp" <<'EOF'
module NormaUsa
  import Core
  public function main()
    var l: Core.List := Core.List()
    l.add(Core.Integer(7))
    print "usa len=" + l.length()
  end main
end NormaUsa
EOF
  cat > "$N/NormaExt.bp" <<'EOF'
module NormaExt
  import Core
  public class Mia extends Core.List
    var extra: integer := 0
    public function Mia()
      super()
    end Mia
    public function marca(): string
      return "mia/" + this.length()
    end marca
  end Mia
  public function main()
    var m: Mia := Mia()
    m.add(Core.Integer(1))
    print "ext " + m.marca()
  end main
end NormaExt
EOF
  # Las stdlibs: 6, 7, 7 con un metodo mas al FINAL de List (misma gen, otra
  # interfaz), 8 (misma fuente), 8 con un metodo INSERTADO al PRINCIPIO de List.
  local INS_FINAL='s/^  end List$/    public function normaNuevo(): integer\n      return 1\n    end normaNuevo\n  end List/'
  local INS_MEDIO='0,/^  public class List/s//  public class List\n    public function normaPrimero(): integer\n      return 0\n    end normaPrimero/'
  norma_stdlib "$N/s6" 6 || return 1
  norma_stdlib "$N/s7" 7 || return 1
  norma_stdlib "$N/s7b" 7 "$INS_FINAL" || return 1
  norma_stdlib "$N/s8" 8 || return 1
  norma_stdlib "$N/s8m" 8 "$INS_MEDIO" || return 1
  grep -q "normaNuevo" "$N/s7b/src/Core.bp" || { echo "  ✗ el sed de INS_FINAL no ha entrado"; return 1; }
  grep -q "normaPrimero" "$N/s8m/src/Core.bp" || { echo "  ✗ el sed de INS_MEDIO no ha entrado"; return 1; }
  # Compilar las dos apps contra la 7 (desde $N: sin BpVM.cfg a la vista, resuelve por outDir).
  local A="$N/app"; rm -rf "$A"; mkdir -p "$A"; cp "$N/s7"/*.mod "$A/"
  ( cd "$N" && java -jar "$V3_FE" NormaUsa.bp --compile app --backend=mivm --formato=8 && java -jar "$V3_FE" NormaExt.bp --compile app --backend=mivm --formato=8 ) >"$N/app.log" 2>&1 || { echo "  ✗ las apps no compilan contra la stdlib 7: $N/app.log"; return 1; }
  # Un directorio por caso: la app + la stdlib de ese caso.
  caso_dir() { local d="$N/c_$1"; rm -rf "$d"; mkdir -p "$d"; cp "$N/$2"/*.mod "$d/"; cp "$A/NormaUsa.mod" "$A/NormaExt.mod" "$d/"; echo "$d"; }
  local d
  d="$(caso_dir ok7 s7)"
  norma_caso "gen igual, misma interfaz: usa"     "$d" NormaUsa "" || fails=$((fails+1))
  norma_caso "gen igual, misma interfaz: extiende" "$d" NormaExt "" || fails=$((fails+1))
  d="$(caso_dir viejo s6)"
  norma_caso "gen menor (Core 6 < 7): rechazo"     "$d" NormaUsa "'Core' \(gen 6\) es mas antiguo que el que 'NormaUsa' necesita \(gen 7\)" || fails=$((fails+1))
  d="$(caso_dir iface s7b)"
  norma_caso "misma gen, otra interfaz: rechazo"   "$d" NormaUsa "'Core' \(gen 7, interfaz [0-9a-f]{8}\) no es la interfaz con la que se compilo 'NormaUsa'" || fails=$((fails+1))
  d="$(caso_dir nuevo s8)"
  norma_caso "gen mayor (8 > 7), usa: corre"       "$d" NormaUsa "" || fails=$((fails+1))
  norma_caso "gen mayor (8 > 7), extiende: rechazo" "$d" NormaExt "'Core' \(gen 8\) es mas reciente que 'NormaExt' \(compilado contra gen 7\) y 'NormaExt' extiende una clase suya" || fails=$((fails+1))
  d="$(caso_dir medio s8m)"
  norma_caso "gen mayor con ranura movida: no enlaza" "$d" NormaUsa "ERROR DE LINK: lib 'Core' presente pero no exporta 'Core.List#" || fails=$((fails+1))
  # La TRANSICION: una dependencia MOD7 (sin datos: gen 0, CRC desconocido). Con el Core
  # MOD7 congelado de la capsula V6 — si existe; si no, se fabrica sin `version`.
  local V6C="$V6_GOLD/stdlib"
  if [ -f "$V6C/Core.mod" ] && [ "$(head -c 4 "$V6C/Core.mod")" = "MOD7" ]; then
    d="$N/c_mod7"; rm -rf "$d"; mkdir -p "$d"; cp "$V6C"/*.mod "$d/"; cp "$A/NormaUsa.mod" "$d/"
    norma_caso "dep MOD7 con app que exige gen 7: rechazo (mas antiguo)" "$d" NormaUsa "'Core' \(gen 0\) es mas antiguo que el que 'NormaUsa' necesita \(gen 7\)" || fails=$((fails+1))
    # Y una app compilada SIN exigir gen (proyecto sin version), en MOD8 (layouts cerrados, marcadores
    # Cls#m#slot), contra el Core MOD7 de V6. Corria hasta el 21-sep porque los layouts COINCIDIAN;
    # desde que Object lleva el destructor (V7/H1: `__dtor()V`, ranura 2) toda vtable corre una ranura y
    # el enlace por marcador falla CON NOMBRE («no exporta 'Core.List#add#3'»): errores si, silenciosos
    # no. Es la leccion de #324 («quitar un metodo de una base CORRE los slots») al reves, y es
    # inherente a MOD8; en MOD9 las ranuras son del cargador y no pasa.
    norma_stdlib "$N/s0" 0 || return 1
    local A0="$N/app0"; rm -rf "$A0"; mkdir -p "$A0"; cp "$N/s0"/*.mod "$A0/"
    ( cd "$N" && java -jar "$V3_FE" NormaUsa.bp --compile app0 --backend=mivm --formato=8 ) >"$N/app0.log" 2>&1 || { echo "  ✗ NormaUsa no compila contra la stdlib 0"; return 1; }
    d="$N/c_mod7b"; rm -rf "$d"; mkdir -p "$d"; cp "$V6C"/*.mod "$d/"; cp "$A0/NormaUsa.mod" "$d/"
    norma_caso "dep MOD7 con app MOD8 sin gen: no enlaza, con nombre (Object lleva el destructor)" "$d" NormaUsa "presente pero no exporta 'Core.List#" || fails=$((fails+1))
    echo "  norma: $((9-fails))/9 casos"
  else
    echo "  (sin capsula V6: los dos casos de dependencia MOD7 no se prueban — ejecuta gen-v6)"
    echo "  norma: $((7-fails))/7 casos"
  fi
  [ "$fails" -eq 0 ]
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
  # V7/H2 (#523 V5/V11) — la pasada principal (MOD9) guarda su stdout: es la
  # referencia del frente MOD8 de abajo (el mismo corpus, compilado como MOD8,
  # tiene que decir lo mismo).
  mkdir -p "$REF_PAR"; PAR_SAVE="$REF_PAR"
  check_parity || fails=$((fails+1))
  PAR_SAVE=""
  # V7/H2 — la compatibilidad binaria VUELVE, esta vez con V6 y aditiva: los .mod
  # MOD7 congelados corren en las VMs de hoy, y los de hoy no rompen la VM de V6.
  check_mod7 || fails=$((fails+1))
  check_fwd  || fails=$((fails+1))
  check_norma || fails=$((fails+1))
  check_sombra || fails=$((fails+1))
  check_mod8 sin-referencia || fails=$((fails+1))
  echo "== check: $([ "$fails" -eq 0 ] && echo 'paridad dual-VM (MOD9) + capsula V6 + norma + sombra + MOD8 VERDE' || echo "$fails frente(s) en ROJO") =="
  [ "$fails" -eq 0 ]
}

case "${1:-}" in
  gen)        gen ;;
  gen-v6)     gen_v6 ;;
  check)      check ;;
  check-mod7) check_mod7 ;;
  check-fwd)  check_fwd ;;
  check-norma) check_norma ;;
  check-sombra) check_sombra ;;
  check-mod8) check_mod8 ;;
  *) echo "uso: $0 {gen|gen-v6|check|check-mod7|check-fwd|check-norma|check-sombra|check-mod8}"; exit 2 ;;
esac
