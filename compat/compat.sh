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
#
# V7/H7 (#599, 28-sep-2026) — DOS WORKERS. Eduardo: «tenemos micros con 1 nucleo y
# con 2 nucleos asi que ejecutar solamente con una opcion no se si es lo
# conveniente». La paridad principal corre con UN worker (lo que hacen hoy todas
# las placas); esta pasada corre el mismo corpus con DOS en las dos VMs:
#
#   ./compat.sh check-smp   VM-C --smp=2 == miVM --workers=2, y el stdout IGUAL al
#                           de la pasada con un worker: el mismo programa dice lo
#                           mismo en un micro de un nucleo y en uno de dos. Un
#                           sample cuyo orden entre hilos no fija el programa (dos
#                           hilos que imprimen a la vez) lo declara en su cabecera
#                           con `// smp: orden libre`, y se compara ORDENADO; con
#                           `// smp: fuera — por que`, se queda fuera de esta pasada
#                           (un SKIP a la vista, con su razon). Entra en check. La
#                           primera vez saco #599: miVM colgada en el GC con
#                           HeapReposo, que la pasada con uno no ve; y la segunda,
#                           #600: con dos workers, la linea de un hilo se partia con
#                           la de otro (EvDos). Desde #600 un `print` sale entero
#                           (PrintEntero: tres hilos y Main imprimiendo a la vez).
#
# V8 #698 (18-oct) — JC, BP con la ropa de Java y C#, al mismo arbol:
#
#   ./compat.sh check-jc    Cada sample del corpus, ademas, escrito en JC
#                           (--escribir-jc), compilado desde el .jc y ejecutado en
#                           las dos VMs: el stdout y el codigo de salida, los de BP.
#                           El .mod no (el `print` de JC es otro arbol). No entra en
#                           check: JC es un experimento de V8.
#
# V8 #726 (23-oct) — LA RED EN TRES GRUPOS. Eduardo: «Si podemos dividir por 3 y cada uno lleva 15 min, pasamos de
# 45 min a 15 min» y «De momento con 3 grupos es suficiente». Medida de una en una, eran 22:51 min:
#
#   ./compat.sh check         A la vez: el corpus (MOD9, MOD8 y dos workers), las pruebas de C, y lo demas; cada
#                             grupo en su $WORK, con `make` antes y, despues, solos, lo que cronometra: el sim,
#                             la sombra y el estres (ver check()).
#   ./compat.sh check-serie   La misma red de una en una, como hasta el 23-oct: para comparar, o si se sospecha
#                             de los grupos.
set -u
# V8/H4 (#708) — la red, EN SILENCIO: el sonido de las dos VMs, al mismo ritmo pero sin tocar el audio del sistema
# (sonido_host.c y Sonido.java), para que no suene nada mientras corre. Lo heredan las VMs y todo lo que se lanza.
export BPVM_SONIDO=mudo

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
PAR_C_ARGS=""    # opciones de la VM-C en la pasada ("--smp=2" en check-smp)
PAR_J_ARGS=""    # opciones de miVM en la pasada ("--workers=2" en check-smp)
PAR_JC=""        # V8 #698: no vacio -> cada sample, ademas, escrito en JC y ejecutado (check-jc)
PAR_REF_QUE="la pasada MOD9"   # de donde sale $PAR_REF, para el mensaje de un FAIL
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
CORPUS="hello arith strings concat charat counter MethodCall trycatch BreakTry CampoFijo FirmaUsa \
        bytetest longtest longarr doubletest powtest casttest utf8test idxtest \
        convtest strops OverloadTest OverloadMethod OverloadCtor SlotPropPriv SlotThreadSub \
        samples/LocalArrTest.bp samples/StrOps348.bp samples/MathOps348.bp samples/PathOps348.bp samples/EvFin.bp samples/ThreadTrasMain.bp samples/ExcDosCatch.bp samples/NullRecv.bp samples/ThreadPrint.bp EnumFirma samples/EnumCadena.bp samples/SyncHijo.bp SyncUsa HomUsa samples/PropRedecl.bp samples/FinallySolo.bp samples/SelfNew.bp samples/ForStep.bp samples/CtorInit.bp VisUsa samples/SyncSinCtor.bp samples/CastBaja.bp samples/MapTipado.bp samples/Destructor.bp samples/TimeDemo.bp samples/ArrayFijo.bp samples/FileCursor.bp samples/PathLargo.bp samples/Caps.bp samples/PinMapa.bp         SciPar ArrLitAncho ObjArray CastExt ListaBp ListaHer CastSelf OwnerBp SuperExt samples/BusBug.bp samples/AdcDemo.bp samples/ArgDemo.bp samples/MathRango.bp samples/mathtest.bp samples/NeoCatch.bp samples/WdtCatch.bp samples/StubParidad.bp samples/BusCompartido.bp samples/IoPrompt.bp samples/IoPathAbs.bp samples/ThrowSinAtrapar.bp samples/MachineHost.bp samples/MachineAlias.bp samples/MachineId.bp samples/GuiParidad.bp samples/GuiParidad2.bp GuiChurn OwnerCascada GuiArbol samples/GuiWinJson.bp samples/GuiShot.bp CamposHijo StrNulo EstaticosHijo CtorHijo LibMarcaUsa samples/diamondtest.bp samples/frompathtest.bp samples/fromtest.bp samples/libimport.bp samples/BusCount.bp samples/SpiDevice.bp samples/MoveRango.bp HiloUsa samples/SwitchTipos.bp samples/PropField.bp samples/ContinueFor.bp samples/NpShowTope.bp samples/FloatNat.bp samples/ArrayNegativo.bp NatArr NatEstrecha NatTuplas CatCadena SbPiezas GlobalesIni samples/ModInit.bp EvDos HeapInfo HeapCompact HeapReposo HeapCompactEv PrintEntero PrintSinSalto OwnerSmp HilosMuchos PilaMuro HiloExc TablaHandles FormsSetter CedeCero DescarteAncho TempRetiene FinallyAnidado RtcPc RtcEnHora FicheroOCarpeta GuiOrdenHijos OwnerReasigna GuiArbolG2 GcHondo OwnerHondo EvReceptorMuerto InvocaRanura BusDosHilos FicheroYaAbierto DestruyeArbol DestruyeLanza DestruyeGc DestruyeFinal DestruyeTrasError SueltaRecursos OwnerHondoDtor bpgenvm-c/samples/importhondo/ImportHondo.bp StrDosCaminos StrCursor StrPorBytes MapDesplaza StrOrden MoveSoloLectura Utf8Descodifica Utf8Gui ArrayEnorme TcpCloseSuelto ParseNumero CharCodeAtFuera JsonErrores NatDoContinue UnaLineaAlFinal RaiseEnSet SonidoParalelo PilasDelHeap PilaOperandos DestruyeBoton IntATexto Compuestos Iif NatCompuestos Genericos EnumEntero Eol Interfaces InterfacesUsa ClaseAbajo"

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

run_vm() {  # $1=bin(.jar|.exe) $2=mod [$3...=opciones de la VM, p. ej. --smp=2]
  # Se ejecuta DESDE el dir del .mod para que ambas VMs resuelvan las deps
  # (p.ej. Core.mod, dep implicita de try/catch en V3 desde #248) junto al
  # modulo raiz. La VM-C ya resuelve relativo al .mod; el CLI de la VM-Java
  # resuelve relativo al CWD, asi que sin este cd la Java no encontraria Core.
  # Subshell () para no alterar el CWD del propio arnes. Los binarios ($1) son
  # rutas absolutas, siguen resolviendo desde cualquier dir.
  local bin="$1" dir base; dir="$(dirname "$2")"; base="$(basename "$2")"; shift 2
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
  # V7/H9 (2-oct, resto de #481) — el CÓDIGO DE SALIDA de cada VM, a un fichero por VM (.rc.java / .rc.c): el
  # stdout va por `filt` y el código se perdía en la tubería. La paridad del corpus lo compara.
  # (cada llamada borra SÓLO el suyo: la paridad llama dos veces seguidas, Java y C, y lee los dos después)
  case "$bin" in
    *.jar) rm -f "$WORK/.rc.java"
           ( cd "$dir" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$bin" "$@" "$base" 2>"$err" | filt
             echo "${PIPESTATUS[0]}" > "$WORK/.rc.java" ) ;;
    *)     rm -f "$WORK/.rc.c"
           ( cd "$dir" && timeout "$VM_TIMEOUT" "$bin" "$@" "$base" 2>"$err" | filt
             echo "${PIPESTATUS[0]}" > "$WORK/.rc.c" ) ;;
  esac
  grep -E "HEAP INCONSISTENTE" "$err" 2>/dev/null | head -2 | sed 's/^/!! stderr: /'
  # V7/H7 #605 (29-sep) — la MEDIDA de la pila, si se pide: con PILA_LOG=<fichero>, cuanta uso cada programa en la
  # VM-C (la linea «[bpvm] pila:» del final del RUN) y si alguno desbordo, en las dos VMs. No toca lo que se compara.
  if [ -n "${PILA_LOG:-}" ]; then grep -E "\[bpvm\] pila:|desbordamiento de pila" "$err" 2>/dev/null | sed "s|^|$base: |" >> "$PILA_LOG"; fi
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
  local pass=0 fail=0 skip=0 s bp mod oj oc fmt="" libres=0 smp_marca="" ordena="" ref_txt="" hechos=0 total
  total=$(echo $CORPUS | wc -w)
  [ -n "$PAR_FORMATO" ] && fmt="--formato=$PAR_FORMATO"
  for s in $CORPUS; do
    # V8 #726 — por dónde va, cada 20 samples (sólo en `check`, la de tres grupos: ver progreso())
    [ "$hechos" -gt 0 ] && [ $(( hechos % 20 )) -eq 0 ] && progreso "$hechos/$total"
    hechos=$(( hechos + 1 ))
    bp="$(bp_path "$s")"
    if [ ! -f "$bp" ]; then
      echo "  SKIP $s (no existe $bp)"; skip=$((skip+1)); continue
    fi
    # V8/H7.2 (#748, 31-oct) — un sample que usa lo que MOD8 no sabe expresar (lo que pide la tabla de clases de MOD9:
    # el INSTANCEOF de una clase de otro modulo) lo DECLARA en su cabecera, `// mod8: fuera — <por que>`, y la pasada
    # MOD8 lo deja fuera, a la vista: un SKIP que dice la razon. Igual que la de smp, una marca que no conoce es ROJO.
    # V8/H7.3 (#756, 1-nov) — ANTES de compilar: lo de MOD9 puede no compilar en MOD8 (las interfaces no compilan), y su
    # SKIP tiene que decir su razon, no «no compila».
    if [ "$PAR_FORMATO" = "8" ]; then
      local mod8_marca
      mod8_marca="$(sed -nE 's|^//[[:space:]]*mod8:[[:space:]]*(.*)$|\1|p' "$bp" | head -1 | tr -d '\r')"
      case "$mod8_marca" in
        "") ;;
        "fuera"|"fuera"[[:space:]]*)
          echo "  SKIP $s — // mod8: $(printf '%s' "$mod8_marca" | cut -c1-110)"; skip=$((skip+1)); continue ;;
        *)
          echo "  FAIL $s — la marca '// mod8: $mod8_marca' no la conoce el arnes (es 'fuera')"
          fail=$((fail+1)); continue ;;
      esac
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
    # V7/H7 (#599, 28-sep) — con dos workers, un sample cuyo orden entre hilos no
    # fija el programa (dos hilos que imprimen a la vez) lo DECLARA en su cabecera,
    # `// smp: orden libre`, y sus lineas se comparan ORDENADAS: las mismas, en
    # cualquier orden, contra la otra VM y contra la referencia. `// smp: fuera`
    # (con su por que) lo deja fuera de ESTA pasada, a la vista: un SKIP que dice la
    # razon. El arnes no adivina cuales son; y una marca que no conoce es ROJO: una
    # errata dejaria el sample comparado de otra manera que la que dice su cabecera.
    smp_marca=""; ordena=""
    if [ -n "$PAR_C_ARGS$PAR_J_ARGS" ]; then
      smp_marca="$(sed -nE 's|^//[[:space:]]*smp:[[:space:]]*(.*)$|\1|p' "$bp" | head -1 | tr -d '\r')"
      case "$smp_marca" in
        "") ;;
        "orden libre"|"orden libre"[[:space:]]*) ordena=1; libres=$((libres+1)) ;;
        "fuera"|"fuera"[[:space:]]*)
          echo "  SKIP $s — // smp: $(printf '%s' "$smp_marca" | cut -c1-110)"; skip=$((skip+1)); continue ;;
        *)
          echo "  FAIL $s — la marca '// smp: $smp_marca' no la conoce el arnes (son 'orden libre' y 'fuera')"
          fail=$((fail+1)); continue ;;
      esac
    fi
    oj="$(run_vm "$V3_JAVA" "$mod" $PAR_J_ARGS)"; oc="$(run_vm "$V3_C" "$mod" $PAR_C_ARGS)"
    # V7/H9 (2-oct, resto de #481) — el CÓDIGO DE SALIDA también es paridad: un programa que acaba bien en una VM y
    # con error en la otra (o con otro error) no es el mismo programa aunque imprima lo mismo. Hasta hoy se perdía en
    # la tubería de `filt` (los `1` de ThrowSinAtrapar, medidos a mano). Antes de exigirlo, el censo: 0 de 125.
    local rcj rcc
    rcj="$(cat "$WORK/.rc.java" 2>/dev/null)"; rcc="$(cat "$WORK/.rc.c" 2>/dev/null)"
    if [ "$rcj" != "$rcc" ]; then
      echo "  FAIL $s — el codigo de salida no casa: VM-Java ${rcj:-?}, VM-C ${rcc:-?}"; fail=$((fail+1)); continue
    fi
    if [ -n "$ordena" ]; then
      oj="$(printf '%s\n' "$oj" | LC_ALL=C sort)"; oc="$(printf '%s\n' "$oc" | LC_ALL=C sort)"
    fi
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
      # V7 (#546/#547, 24-sep) — la paridad no ve lo que las dos VMs hacen MAL por igual: un
      # switch sobre un string no casaba nunca y `field := 7` no convertía, en las dos, idéntico.
      # Un sample puede comprobarse a sí mismo e imprimir una línea `FALLO …` cuando un valor no
      # es el esperado: eso es rojo aunque las VMs coincidan.
      if printf '%s\n' "$oj" | grep -q '^FALLO'; then
        echo "  FAIL $s — paridad OK, pero el sample dice que algo sale mal:"; fail=$((fail+1))
        printf '%s\n' "$oj" | grep '^FALLO' | head -4 | sed 's/^/       /'
        continue
      fi
      # V7/H2 (#523 V5) — paridad no basta: en MOD9 el stdout tiene que ser EL DE
      # MOD8. Un sample que en MOD9 imprimiera otra cosa con las dos VMs de
      # acuerdo seria un falso-PAR de otro piso: el enlace tardio cambiando la
      # semantica sin que nadie lo oiga.
      local outname="${s//\//_}.out"
      if [ -n "$PAR_REF" ] && [ -f "$PAR_REF/$outname" ]; then
        if [ -n "$ordena" ]; then ref_txt="$(LC_ALL=C sort "$PAR_REF/$outname")"; else ref_txt="$(cat "$PAR_REF/$outname")"; fi
        if [ "$oj" != "$ref_txt" ]; then
          echo "  FAIL $s — paridad OK pero el stdout NO es el de la referencia ($PAR_REF_QUE)"; fail=$((fail+1))
          diff <(printf '%s\n' "$ref_txt") <(printf '%s\n' "$oj") 2>/dev/null | head -6 | sed 's/^/       /'
          continue
        fi
      fi
      [ -n "$PAR_SAVE" ] && printf '%s\n' "$oj" > "$PAR_SAVE/$outname"
      # V8 #698 (18-oct) — JC: el mismo programa escrito en JC (`--escribir-jc`), compilado desde el `.jc` y
      # ejecutado en las dos VMs, imprime LO MISMO que el de BP y acaba con el mismo código. El `.mod` no es el
      # mismo: el `print` de JC es otro árbol (`print a, b` se escribe `print(a, " ", b, "\n")`, #700), y por eso
      # se compara la salida. Lo que el escritor no escribe —lo acordado: el paso de un `for` en una variable, el
      # nombre `base`— es un SKIP con su motivo, a la vista. El `.jc` se compila en el mismo $WORK que el de BP:
      # los módulos hermanos que importe ya están ahí, compilados desde su `.bp`.
      if [ -n "$PAR_JC" ]; then
        local jcd="$WORK/srcjc" jcf ojj ocj rcjj rccj
        rm -rf "$jcd" "$WORK/pjc.bpbuild"; mkdir -p "$jcd"
        jcf="$jcd/$rootname.jc"
        java -jar "$V3_FE" "$bp" --escribir-jc --stdlibDir="$WORK" > "$jcf" 2> "$WORK/.jc.err"
        case $? in
          0) ;;
          2) echo "  SKIP $s — JC: $(tr -d '\r' < "$WORK/.jc.err" | head -1 | cut -c1-120)"; skip=$((skip+1)); continue ;;
          *) echo "  FAIL $s — JC: el escritor falla:"; tr -d '\r' < "$WORK/.jc.err" | head -3 | sed 's/^/       /'
             fail=$((fail+1)); continue ;;
        esac
        if ! java -jar "$V3_FE" "$jcf" --compile "$WORK" --backend=mivm --stdlibDir="$WORK" $fmt > "$WORK/.jc.log" 2>&1; then
          # El mismo 2o intento que el de BP: por proyecto (el main, `srcjc/<modulo>.jc`).
          printf '{ "sourceDir": "srcjc", "outDir": ".", "main": "%s", "dependencies": ["%s"] }\n' \
                 "$rootname" "$STDLIB" > "$WORK/pjc.bpbuild"
          if ! ( cd "$WORK" && java -jar "$V3_FE" --project pjc.bpbuild --backend=mivm --stdlibDir="$WORK" $fmt ) >> "$WORK/.jc.log" 2>&1; then
            echo "  FAIL $s — JC: el .jc no compila:"; grep -i 'error' "$WORK/.jc.log" | head -3 | sed 's/^/       /'
            fail=$((fail+1)); continue
          fi
        fi
        ojj="$(run_vm "$V3_JAVA" "$mod" $PAR_J_ARGS)"; ocj="$(run_vm "$V3_C" "$mod" $PAR_C_ARGS)"
        rcjj="$(cat "$WORK/.rc.java" 2>/dev/null)"; rccj="$(cat "$WORK/.rc.c" 2>/dev/null)"
        if [ -n "$ordena" ]; then
          ojj="$(printf '%s\n' "$ojj" | LC_ALL=C sort)"; ocj="$(printf '%s\n' "$ocj" | LC_ALL=C sort)"
        fi
        if [ "$rcjj" != "$rcj" ] || [ "$rccj" != "$rcc" ]; then
          echo "  FAIL $s — JC: el codigo de salida no es el de BP (VM-Java ${rcjj:-?} y no ${rcj:-?}, VM-C ${rccj:-?} y no ${rcc:-?})"
          fail=$((fail+1)); continue
        fi
        if [ "$ojj" != "$oj" ] || [ "$ocj" != "$oj" ]; then
          echo "  FAIL $s — JC: la salida no es la de BP"; fail=$((fail+1))
          diff <(printf '%s\n' "$oj") <(printf '%s\n' "$ojj") 2>/dev/null | head -6 | sed 's/^/       /'
          [ "$ojj" = "$ocj" ] || echo "       (y el de JC, VM-Java != VM-C)"
          continue
        fi
      fi
      pass=$((pass+1))
    else
      echo "  FAIL $s — VM-Java != VM-C (paridad rota)"; fail=$((fail+1))
      diff <(printf '%s\n' "$oj") <(printf '%s\n' "$oc") 2>/dev/null | head -6 | sed 's/^/       /'
    fi
  done
  echo "  paridad: $pass PASS, $fail FAIL, $skip SKIP"
  [ "$libres" -gt 0 ] && echo "  ($libres con '// smp: orden libre': sus lineas, comparadas ordenadas)"
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

# ------------------------------------------------------------- JC (V8 #698)
# Cada sample del corpus, ademas de en BP, escrito en JC (`--escribir-jc`), compilado desde el `.jc` y ejecutado
# en las dos VMs: tiene que imprimir lo mismo que el de BP (ver check_parity). No entra en `check`: JC es un
# experimento de V8, y lo que se sube al final se decide al final.
check_jc() {
  echo "-- JC: el corpus escrito en JC imprime lo mismo que en BP, en las dos VMs --"
  local rc
  PAR_JC=1; check_parity; rc=$?; PAR_JC=""
  echo "  jc: $([ "$rc" -eq 0 ] && echo 'el corpus dice en JC lo mismo que en BP' || echo 'JC en ROJO')"
  return "$rc"
}

# ------------------------------------------------------------- dos workers (V7/H7 #599)
# El corpus con DOS workers en las dos VMs (VM-C --smp=2, miVM --workers=2): paridad
# Java == C y el stdout de la pasada con UNO, sample a sample. Hoy todas las placas
# corren un worker —tambien las de dos nucleos—, pero el mismo programa tiene que
# decir lo mismo en las dos maneras (Eduardo, 28-sep). Los samples con
# `// smp: orden libre` se comparan ordenados (ver check_parity).
check_smp() {   # $1 = "sin-referencia" cuando check ya hizo la pasada principal guardando en $REF_PAR
  echo "-- el corpus con DOS workers (VM-C --smp=2 == miVM --workers=2, y el stdout de la pasada con uno) --"
  local ref="$REF_PAR" rc; mkdir -p "$ref"
  if [ "${1:-}" != "sin-referencia" ]; then
    echo "  [un worker: la referencia]"
    PAR_SAVE="$ref"; check_parity; rc=$?; PAR_SAVE=""
    [ "$rc" -eq 0 ] || return 1
  elif [ -z "$(ls "$ref"/*.out 2>/dev/null)" ]; then
    echo "  no hay referencia en $ref: la pasada principal no guardo nada"; return 1
  fi
  echo "  [dos workers]"
  PAR_C_ARGS="--smp=2"; PAR_J_ARGS="--workers=2"; PAR_REF="$ref"; PAR_REF_QUE="la pasada con un worker"
  check_parity; rc=$?
  PAR_C_ARGS=""; PAR_J_ARGS=""; PAR_REF=""; PAR_REF_QUE="la pasada MOD9"
  echo "  smp: $([ "$rc" -eq 0 ] && echo 'el corpus dice lo mismo con dos workers que con uno' || echo 'dos workers en ROJO')"
  return "$rc"
}

# ------------------------------------------------------------- el estres del heap
# V7/H7 #610 (29-sep) — lo que la paridad no ve porque casi siempre sale BIEN: la prueba de estres de #593
# (V7_IDEAS §H7). TablaHandles en la VM-C con el GC APAGADO y cuatro workers, varias veces: cuatro hilos guardan
# cadenas vivas, asi que la tabla de handles crece mientras los otros la leen. Crecia MOVIENDOSE con ellos corriendo
# (un memmove dentro del heap): con el codigo de antes de #610, un use-after-free falso casi una vez de cada dos.
# Ahora crece con el mundo parado. Sin GC a proposito: con el GC en marcha la tabla crece justo tras una colecta,
# con los demas parados, y no se veria. ESTRES_VECES=n cambia las pasadas.
# V7/H7 #611 — y HeapEstres con el heap JUSTO (~1.000 colectas por ejecucion) y 1, 2 y 4 workers, en las DOS VMs:
# cuatro hilos se pasan cadenas por un buzon y comprueban lo que reciben. El split de miVM reservaba los trozos antes
# que el array, donde el GC no los ve: con el heap justo fallaba SIEMPRE, tambien con un solo worker.
# V7/H7 #613 (30-sep) — y CedeCero con CUATRO workers, varias veces, en las dos VMs: cuatro hilos que ceden con
# sleep(0) en cada vuelta. miVM reclamaba un hilo que su worker aun no habia soltado (el sleep(0) lo re-encola
# desde dentro del builtin): dos workers sobre el mismo contexto, 9 de cada 30 con cuatro. La VM-C, nunca
# (sched_owner); desde #613 miVM tampoco.
# V7 #616 (1-oct) — y LA RUEDA: tres hilos que calculan sin ceder (Rueda). Con un worker, la VM-C con su planificador
# de varios (--smp=1) tiene que decir lo MISMO que con el de uno y que miVM: los tres empiezan antes de que acabe
# ninguno. El de varios buscaba siempre desde el primer hilo de la lista: A entero, luego B, luego C. Y con dos y
# cuatro workers, en las dos VMs, los tres tienen que empezar antes de que acabe el primero.
ESTRES_VECES="${ESTRES_VECES:-8}"
check_estres() {
  echo "-- el estres del heap (#610, #611): la tabla de handles que crece (VM-C sin GC, x$ESTRES_VECES); el heap justo con 1, 2 y 4 workers (las dos VMs); y un hilo que cede, con 4 workers (#613, x$ESTRES_VECES, las dos VMs) --"
  local E="$WORK/estres" fails=0 i rc sal w op
  rm -rf "$E"; mkdir -p "$E"; cp "$STDLIB"/*.mod "$E/"
  ( cd "$ROOT" && java -jar "$V3_FE" "$SAMPLES/TablaHandles.bp" --compile "$E" --backend=mivm \
      && java -jar "$V3_FE" "$SAMPLES/HeapEstres.bp" --compile "$E" --backend=mivm \
      && java -jar "$V3_FE" "$SAMPLES/CedeCero.bp" --compile "$E" --backend=mivm \
      && java -jar "$V3_FE" "$SAMPLES/Rueda.bp" --compile "$E" --backend=mivm ) >"$E/compila.log" 2>&1 \
    || { echo "  ✗ TablaHandles, HeapEstres, CedeCero o Rueda no compilan: $E/compila.log"; return 1; }
  # El heap justo: en miVM, ~38 KB para objetos (~80 colectas); en la VM-C, ~32 KB (~1.000). V8 #740 (29-oct): las
  # pilas de los cuatro hilos (8 KB cada una) salen ahora del heap, y la region fija es solo la del main (16 KB); para
  # el mismo apreton, el heap crece en esos 32 KB y el total baja: miVM, de 112 a 96 KB, con stackBase de 48 a 80; la
  # VM-C, de 96 a 80 KB. Medido con las VMs de antes y de despues: miVM 79 y 80 colectas; la VM-C 1085 y 1089.
  printf '{\n  "memorySize": 98304,\n  "stackBase": 81920\n}\n' > "$E/justo.cfg"
  local justo="creados:  1600|errores:  0|leidos de otros:  true|muchas colectas:  true"
  for w in 1 2 4; do
    ( cd "$E" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$V3_JAVA" --config=justo.cfg --workers=$w HeapEstres.mod >"$E/j.$w" 2>"$E/je.$w" ); rc=$?
    sal="$(filt < "$E/j.$w" | paste -sd'|')"
    if [ "$rc" -ne 0 ] || [ "$sal" != "$justo" ]; then
      fails=$((fails+1))
      echo "  ✗ HeapEstres miVM --workers=$w: salida $rc, «$sal» — $(grep -m1 -vE '^\[(gc|miVM)\]|^VM \[GC\]' "$E/je.$w")"
    fi
  done
  for w in 0 1 2 4; do
    op=""; [ "$w" -gt 0 ] && op="--smp=$w"
    ( cd "$E" && timeout "$VM_TIMEOUT" "$V3_C" --mem=81920 $op HeapEstres.mod >"$E/c.$w" 2>"$E/ce.$w" ); rc=$?
    sal="$(filt < "$E/c.$w" | paste -sd'|')"
    if [ "$rc" -ne 0 ] || [ "$sal" != "$justo" ]; then
      fails=$((fails+1))
      echo "  ✗ HeapEstres VM-C ${op:-sin --smp}: salida $rc, «$sal» — $(grep -m1 -E 'UAF|mensaje que traia|DESCARRIL' "$E/ce.$w")"
    fi
  done
  for i in $(seq 1 "$ESTRES_VECES"); do
    ( cd "$E" && timeout "$VM_TIMEOUT" "$V3_C" --nogc --smp=4 TablaHandles.mod >"$E/out.$i" 2>"$E/err.$i" ); rc=$?
    sal="$(filt < "$E/out.$i" | grep -v 'GC DESACTIVADO' | paste -sd'|')"   # el aviso de --nogc no es del programa
    if [ "$rc" -ne 0 ] || [ "$sal" != "errores:  0" ]; then
      fails=$((fails+1))
      echo "  ✗ pasada $i: salida $rc, «$sal» — $(grep -m1 -E 'UAF|mensaje que traia|SIN MEMORIA' "$E/err.$i")"
    fi
  done
  for i in $(seq 1 "$ESTRES_VECES"); do
    ( cd "$E" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$V3_JAVA" --workers=4 CedeCero.mod >"$E/cj.$i" 2>"$E/cje.$i" ); rc=$?
    sal="$(filt < "$E/cj.$i" | paste -sd'|')"
    if [ "$rc" -ne 0 ] || [ "$sal" != "errores:  0" ]; then
      fails=$((fails+1))
      echo "  ✗ CedeCero miVM --workers=4, pasada $i: salida $rc, «$sal» — $(grep -m1 -E '^\[B1\]|HALT|excep' "$E/cje.$i")"
    fi
    ( cd "$E" && timeout "$VM_TIMEOUT" "$V3_C" --smp=4 CedeCero.mod >"$E/cc.$i" 2>"$E/cce.$i" ); rc=$?
    sal="$(filt < "$E/cc.$i" | paste -sd'|')"
    if [ "$rc" -ne 0 ] || [ "$sal" != "errores:  0" ]; then
      fails=$((fails+1))
      echo "  ✗ CedeCero VM-C --smp=4, pasada $i: salida $rc, «$sal»"
    fi
  done
  local rueda="empieza A|empieza B|empieza C|acaba A|acaba B|acaba C|fin" quien
  for quien in "c:" "c:--smp=1" "j:--workers=1"; do
    op="${quien#*:}"
    if [ "${quien%%:*}" = c ]; then ( cd "$E" && timeout "$VM_TIMEOUT" "$V3_C" $op Rueda.mod >"$E/r.out" 2>"$E/r.err" ); rc=$?
    else ( cd "$E" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$V3_JAVA" $op Rueda.mod >"$E/r.out" 2>"$E/r.err" ); rc=$?; fi
    sal="$(filt < "$E/r.out" | paste -sd'|')"
    if [ "$rc" -ne 0 ] || [ "$sal" != "$rueda" ]; then
      fails=$((fails+1)); echo "  ✗ Rueda (#616) ${quien%%:*} ${op:-sin opciones}: salida $rc, «$sal»"
    fi
  done
  for quien in "c:--smp=2" "c:--smp=4" "j:--workers=2" "j:--workers=4"; do
    op="${quien#*:}"
    if [ "${quien%%:*}" = c ]; then ( cd "$E" && timeout "$VM_TIMEOUT" "$V3_C" $op Rueda.mod >"$E/r.out" 2>"$E/r.err" ); rc=$?
    else ( cd "$E" && timeout "$VM_TIMEOUT" java -Djava.awt.headless=true -jar "$V3_JAVA" $op Rueda.mod >"$E/r.out" 2>"$E/r.err" ); rc=$?; fi
    sal="$(filt < "$E/r.out" | paste -sd'|')"
    # las tres «empieza» antes de la primera «acaba»; el orden entre ellas, libre
    if [ "$rc" -ne 0 ] || [ "$(filt < "$E/r.out" | head -3 | grep -c '^empieza')" != 3 ]; then
      fails=$((fails+1)); echo "  ✗ Rueda (#616) ${quien%%:*} $op: un hilo no empieza hasta que acaba otro — salida $rc, «$sal»"
    fi
  done
  echo "  estres: $([ "$fails" -eq 0 ] && echo "la tabla crece sin corromper nada ($ESTRES_VECES de $ESTRES_VECES), el heap justo aguanta, un hilo que cede no se ejecuta dos veces y los hilos se turnan en rueda (#616), en las dos VMs" || echo "$fails pasadas en ROJO")"
  [ "$fails" -eq 0 ]
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
# ---------------------------------------------------------------- check-sim
# V7 (#537, 23-sep) — EL MICRO SIMULADO, EN LA RED. Sus cuatro smokes vivian fuera
# de toda red y se pudrieron sin que nadie lo viera: `sim-smoke` rojo desde H1 P3
# (20-sep: el `import Core` implicito, con el sim arrancado sin libreria), el caso
# GUI de `io-smoke` sin cargar desde el 11-sep y contado como «saltado», y el
# control de `test-listtrunc` rojo desde U3.24 (31-ago). Y `packs/Stdlib.pack` —lo
# que el IDE graba en el sim y lo que viaja en el ZIP— era la stdlib del 18-sep.
# Aqui: la guarda del pack + los cuatro smokes, contra un sim construido AQUI y en
# su propio binario (build/bpvm-sim-red.exe): el de siempre puede tenerlo abierto
# el IDE, o ser LVGL/SQLite. Un sim que no compila o un smoke que no acaba en
# [status=OK] es ROJO; «saltado» solo vale lo que el smoke declara por sabor (el
# caso GUI de io-smoke, que sin LVGL no bloquea).
SIM_RED=""   # lo fija check_sim al construirlo; check_sombra lo usa si esta
check_sim() {
  echo "-- el micro simulado (#537): la libreria del pack y los smokes del wire (y la SD, V7/H3) --"
  local fails=0 sal rc t n sim="$ROOT/bpgenvm-c/build/bpvm-sim-red.exe"
  # sim_dbg_smoke y run_smoke: V7/H10, el RUN común, paso 0 (3-oct) —el depurador por el cable (con #634: un KILL en
  # pausa), que llevaba meses con un .mod MOD5 que ya nadie cargaba; y lo que HOY hace el RUN en los casos que nada más
  # mira (un fallo de carga, un NUL, 512 controles, un carácter en el byte 512, un KILL sin programa)—.
  # stat_base_smoke: V7 #654 (7-oct) —una pregunta del cable (STAT por nombre con otra `base`) no mueve la
  # carpeta del programa en marcha: 300 preguntas mientras corre, y «cambios: 0»—.
  # run_tarea_smoke: V7/H12, etapas 0 y 1 (4-oct) —la tarea `run`: mil RUN seguidos, la salida tardía, el KILL barriendo
  # el final del programa, la ventana ensanchada, el sabotaje que tiene que cazar y el interruptor (`--run-viejo`)—.
  local smokes="sim_smoke boardsim_smoke io_smoke list_trunc_smoke sd_smoke reset_smoke env_smoke sim_dbg_smoke run_smoke run_tarea_smoke stat_base_smoke" total   # reset_smoke: V7/H8 (28-sep); env_smoke: #626
  total=$(( $(echo $smokes | wc -w) + 1 ))
  bash "$HERE/stdlib_pack.sh" || fails=$((fails+1))
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    echo "  ⚠ el host esta compilado CON LVGL: el sim de la red necesita 'make GUI=1 LVGL=0' (sabor gui1-lvgl0)"
    return 1
  fi
  if ! make -C "$ROOT/bpgenvm-c" sim SIM_OUT=build/bpvm-sim-red.exe >"$WORK/.sim.make.log" 2>&1; then
    echo "  ✗ el simulador no compila (make sim):"
    grep -iE "error" "$WORK/.sim.make.log" | head -4 | sed 's/^/       /'
    return 1
  fi
  SIM_RED="$sim"
  for t in $smokes; do
    sal="$( cd "$ROOT/bpgenvm-c" && BPVM_SIM="$sim" timeout 300 python "tools/$t.py" 2>&1 )"; rc=$?
    n="$(printf '%s\n' "$sal" | grep -c '^  ok')"
    if [ "$rc" -eq 0 ] && printf '%s\n' "$sal" | grep -q '^\[status=OK\]'; then
      echo "  ok  $t: $n comprobaciones$(printf '%s\n' "$sal" | grep -q 'SALTADA' && printf ' (y una SALTADA que el smoke declara: el sim de la red no lleva LVGL)')"
    else
      echo "  ✗ $t (salida $rc, $n ok):"
      printf '%s\n' "$sal" | grep -E "FAIL|Traceback|Error" | head -6 | sed 's/^/       /'
      fails=$((fails+1))
    fi
  done
  echo "  sim: $((total - fails))/$total (el pack y los smokes)"
  [ "$fails" -eq 0 ]
}

# ---------------------------------------------------------------- check-testc
# V7/H9 (#541, 24-sep) — PRUEBAS DE C que no estaban en ninguna red y se pudrieron
# sin que nadie lo viera: el AOT EJECUTADO (test-aotgc, test-aotnew: el .c del
# emisor compilado con el gcc del host y corrido con el GC en cada alocacion) y el
# nucleo del depurador (test-debug, test-debug-mt, test-listen). NatNew.bp no
# compilaba desde #19; las del depurador cargaban un Arith.mod MOD5 de mayo —y en
# un checkout limpio se SALTABAN con salida 0—; y el AOT de las dos primeras no
# veia Core fuera del checkout principal. Cada una compila ya su .mod (y su .c) en
# cada ejecucion. Verde = salida 0 Y su linea de OK: un «skip», o una salida 0 sin
# la señal buena, es ROJO.
# #629 — la señal de verde de cada prueba de C: casi todas acaban en [status=OK]; las de antes de esa costumbre, no.
# Junto con la salida 0 (las que se repiten, como test-fslfsmt, salen con 1 si una vuelta falla).
senal_testc() {
  case "$1" in
    test-oom|test-excmsggc|test-gchole|test-gchondo|test-ownerhondo|test-coresiempre|test-anidadalarga|test-trazasbus|test-importhondo|test-hilos|test-rand|test-runguard|test-nat2runs) echo "^PASS $1\$" ;;
    test-lfs)        echo '^littlefs smoke OK' ;;
    test-lfsfile)    echo '^filebd read OK' ;;
    test-lfsfault)   echo '^littlefs fault-injection OK' ;;
    test-fslfs)      echo '^fachada bpvm_fs sobre littlefs OK' ;;
    test-fscursor)   echo '^[0-9]+/[0-9]+, 0 fallos$' ;;
    test-fsvfs)      echo '^VFS B1\.3 OK' ;;
    test-fslfsmt)    echo '^fs-mt: threads=8 .*corrupciones=0 \(VERDE\)' ;;
    test-fspos)      echo '^--- [0-9]+/[0-9]+ OK, 0 FALLO\(S\) ---$' ;;
    test-sd-turnos-control) echo 'el control falla, como debe \[status=OK\]$' ;;
    check-builtins)  echo '^  [0-9]+ builtins en Core\.bp .* generados hasta hoy' ;;
    *)               echo '^\[status=OK\]|^OK: ' ;;
  esac
}
check_testc() {
  echo "-- pruebas de C (#541): el AOT ejecutado, el switch en una native (#546), el float en una native por la FPU y por los helpers (#559), el EXITED que sale siempre (#557), la stdlib que cede a la zona de packs (#466, #468), la TF soltando el bus (H3 SD.2) y el nucleo del depurador --"
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    echo "  ⚠ el host esta compilado CON LVGL: estas pruebas necesitan 'make GUI=1 LVGL=0' (sabor gui1-lvgl0)"
    return 1
  fi
  local fails=0 t sal rc ok n
  # V7/H7 (30-sep): y test-part, test-bmgr y test-pack, que no estaban en ninguna red: test-bmgr llevaba roto (no
  # enlazaba) desde #468 sin que nadie lo viera, por segunda vez (la primera, del 13-ago al 23-sep).
  # V7/H9 #619 (2-oct): y test-guipack —la costura VM <-> pack de la GUI—, que tampoco estaba en ninguna red: el
  # «no hay pack» guardado para toda la sesión (la C6) se le escapó por no tener el caso, no por estar fuera.
  # V7/H10 #626 (2-oct): y test-env —el bloque A/B del ENV, y bpvm_env_anadir, la escritura del arranque—, que
  # tampoco estaba en ninguna red.
  # V7/H10 #629 (2-oct): y TODAS las del Makefile que ninguna red corría —el censo de ese día: 32 sin nombrar; de
  # las 31 ejecutadas, 30 en verde y test-crc, que no enlazaba desde el 31-ago (#628)—, y test-fsfallo (#628). Fuera, con su porqué: test-compressbench
  # (una medida: su prueba es test-compressnat), gui-smoke (abre una ventana SDL) y boardsim-smoke (check-sim ya corre
  # su boardsim_smoke.py). No todas acaban en [status=OK]: cada una dice su señal de verde en senal_testc.
  # V7/H10 #634 (3-oct): y test-dbgkill —un KILL con el programa en pausa: el Stop del IDE en un breakpoint—.
  # V7/H10, el RUN común, paso 1 (3-oct): y test-salida —la salida del programa, común: sin partir nada y sin tirar nada—.
  # #639 (3-oct): y test-tablasim —la tabla de símbolos con poca memoria: crece menos, y si no cabe, lo dice—.
  # V7/H10, el board_mgr común (#637, 3-oct): y test-flash —la zona de la flash que se puede escribir—.
  # #648 (4-oct): y test-gchondo —el GC sin recursión, con la pila acotada en las dos VMs—.
  # #656 (7-oct): y test-ownerhondo —soltar un owner sin recursión, igual, y con el repaso de la VM-C recorrido—.
  # #652 (7-oct): y test-coresiempre —todo programa lleva Core: el Hello de la cápsula de V6, que no lo importa—.
  # #664 (7-oct): y test-anidadalarga —una llamada anidada larga con dos workers: la VM-Java, aparcada para el GC—.
  # #659 (7-oct): y test-trazasbus —una traza de bus es una línea, también con dos runs— y test-runtrasrun
  # —un RUN tras otro empieza de cero: el ADC del stub y los avisos de un throw—.
  # #649 (4-oct): y test-importhondo —la carga sin recursión: una cadena de 13 imports no cuesta más pila que un módulo—.
  # H12, etapa 2 (4-oct): y test-hilos —la cola de hilos sin array ni tope, y que decrece: un hilo acabado sale de ella—.
  # H12, etapa 2a.2 (4-oct): y test-cola —el planificador de la cola: el interbloqueo con uno y dos runs, ninguno falso, y el fin del
  # último run que sale; con sus dos sabotajes— (y test-kill, con el KILL que despierta a un programa dormido).
  # #653 (4-oct): y test-logmt —el log con dos escritores no se sale de su región; con su sabotaje—.
  # #355, H12 2b (5-oct): y test-reserva —la reserva de emergencia, sólo para construir un error; con su sabotaje—.
  # H12 2b (5-oct): y test-frcond —la condición de FreeRTOS con el kernel de verdad; con la de antes, roja—.
  # #660/#661 (5-oct): y test-palabras —el camino fino: palabras de una vez, el número de vuelta en el GC, la
  # referencia que no puede existir, muerta; con sus tres sabotajes—.
  # V7 #669 (8-oct): y test-bus y test-arbitro —el bus compartido de G4 y su árbitro con dos hilos—, que no estaban en
  # ninguna red (pasaban sueltas); test-bus lleva además los pines de la pantalla de la imagen (#669).
  # V7/H12 (6-oct): y test-nat2runs —las natives con dos runs: el contexto, el depósito de ejecutores, la pila de C de
  # una native que espera al mundo dentro de sí y la línea del bucle aparcada; con sus tres sabotajes—.
  # V8 #724 (27-oct): y test-packtexto —el texto que devuelve un pack, con uno de mentira: entero y descodificado— y
  # test-sqltexto —el de SQLite de punta a punta, con el puente que escribe el compilador de hoy—.
  # V8/H4 (#708, 27-oct): y test-sonido —lo común del sonido, con una cintura que apunta cada muestra—.
  local lista="test-ejecutor test-aotdirecta test-mdnv3 test-aotgc test-aotnew test-aotswitch test-aotstr test-aotarr test-aotanida test-aotpuente test-aottryvm test-aottry test-aotnombres test-aotestrecha test-aottuplas test-aotglobales test-aotcede test-aotexpropia test-aotkill test-aotpesada test-aotbuiltin test-aotcompara test-aotdo test-aotcompuestos test-packtexto test-sqltexto test-sonido test-aotfloat test-wirerecorte test-salida test-mods test-guilleno test-guipack test-sd-turnos test-debug test-debug-mt test-listen test-kill test-dbgkill test-tablasim test-flash test-heapdescarrilado test-compactsmp test-smpfallo test-oqentero test-net test-iomt test-smphandles test-freelist test-packhelp test-mem test-utf8 test-part test-bmgr test-pack test-npack test-sqlmem test-boot2 test-env test-mdnreceta test-callbp test-xmodule test-throwmsg test-throwuser test-method test-bytenat test-xmodnat test-compressnat test-oom test-excmsggc test-lfs test-lfsfile test-lfsfault test-fslfs test-fscursor test-fsvfs test-fslfsmt test-fspos test-blk test-sdio test-listdir test-boot test-crc test-fsfallo test-gchole test-gchondo test-ownerhondo test-coresiempre test-anidadalarga test-trazasbus test-runtrasrun test-importhondo test-hilos test-cola test-nat2runs test-logmt test-reserva test-palabras test-frcond test-rand test-runguard test-sd-turnos-control test-suelta test-destruirpoca test-destruyefinal test-censo test-bus test-arbitro check-builtins"
  n=$(echo $lista | wc -w)
  local hechos=0
  for t in $lista; do
    [ "$hechos" -gt 0 ] && [ $(( hechos % 20 )) -eq 0 ] && progreso "$hechos/$n"   # V8 #726: por dónde va, cada 20
    hechos=$(( hechos + 1 ))
    sal="$(timeout 300 make -C "$ROOT/bpgenvm-c" "$t" 2>&1)"; rc=$?
    ok="$(printf '%s\n' "$sal" | grep -E "$(senal_testc "$t")" | head -1)"
    if [ "$rc" -eq 0 ] && [ -n "$ok" ]; then
      echo "  ok  $t: $(printf '%s' "$ok" | cut -c1-90)"
    else
      echo "  ✗ $t (salida $rc):"
      printf '%s\n' "$sal" | grep -E "ROJO|FAIL|FALLOS|[Ee]rror|skip|✗|^  - |^  \| " | grep -v "warning" | head -8 | sed 's/^/       /'
      fails=$((fails+1))
    fi
  done
  echo "  pruebas de C: $((n - fails))/$n"
  [ "$fails" -eq 0 ]
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
    # V7 #650 (8-oct) — ...pero si /lib tiene los MISMOS bytes que el pack (tamaño y CRC), la del PACK: XIP, sin
    # copiarla a la RAM (Eduardo: «Misma versión, de momento mismo tamaño y CRC»).
    mkdir -p "$S/lib3"; cp "$S/lib2/"*.mod "$S/lib3/"; cp "$S/var/Str.mod" "$S/lib3/Str.mod"
    o="$( cd "$S" && "$V3_C" --libdir=lib3 --pack=StrPk.pack app/Sombra.mod 2>"$S/igual.err" | filt )"
    if printf '%s' "$o" | grep -q "VARIANTE:ff" && grep -q "del FS es igual al del pack" "$S/igual.err" && grep -q "cargado XIP desde pack" "$S/igual.err" && ! grep -q "eclipsa al del pack" "$S/igual.err"; then echo "  ok  zona (VM-C): /lib con la MISMA Str que el pack: la del pack, XIP (#650)"
    else echo "  ✗ zona: con la misma Str en /lib y en el pack, esperaba la del pack (#650):"; grep -E "dep 'Str'|XIP|eclipsa|igual" "$S/igual.err" | sed 's/^/       /'; fails=$((fails+1)); fi
    # ...y un principal que SOLO vive en la zona arranca XIP con su pack primero.
    o="$( cd "$S/vacio" && "$V3_C" --libdir=../lib2 --pack=../AppPk.pack Sombra.mod 2>"$S/zona.err" | filt )"
    if printf '%s' "$o" | grep -q "VARIANTE:ff" && grep -q "cargado XIP desde el pack 'AppPk'" "$S/zona.err" && grep -q "dep 'Str' -> pack EN EJECUCION" "$S/zona.err"; then echo "  ok  zona (VM-C): el principal que solo vive en la zona corre XIP, su pack primero"
    else echo "  ✗ zona: el principal de la zona:"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; grep -E "dep |XIP|load" "$S/zona.err" | sed 's/^/       /'; fails=$((fails+1)); fi
    # V7/H7 #606 — ...y con DOS workers: bpvm_run_smp buscaba el modulo del `main` con el rango viejo, vacio en XIP
    # («PC fuera de rango»); desde #606 la puesta a punto del RUN es una para los dos planificadores.
    o="$( cd "$S/vacio" && "$V3_C" --smp=2 --libdir=../lib2 --pack=../AppPk.pack Sombra.mod 2>"$S/zona2.err" | filt )"
    if printf '%s' "$o" | grep -q "VARIANTE:ff"; then echo "  ok  zona (VM-C): el principal de la zona corre XIP tambien con dos workers (#606)"
    else echo "  ✗ zona: el principal de la zona con --smp=2 (#606):"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; tail -2 "$S/zona2.err" | sed 's/^/       /'; fails=$((fails+1)); fi
    # V7/H7 #606 — ...y el manejador por RANURA (Forms: GUI_INVOKE_BY_SLOT) de una clase de la zona salta a SU
    # codigo: la direccion se calculaba sin la base `cb` del XIP y saltaba a basura.
    cat > "$S/src/SlotZona.bp" <<'EOF'
module SlotZona
  import Gui
  class Mando
    var nombre: string
    public function Mando(nombre: string)
      this.nombre := nombre
    end Mando
    public function pulsado(sender: Object)
      print "manejador de " + this.nombre + " llamado"
    end pulsado
  end Mando
  public function main()
    var m: Mando := Mando("la zona")
    Gui.__guiInvokeBySlot(m, Gui.__guiSlotOf(m, "pulsado"), null)
    print "fin"
  end main
end SlotZona
EOF
    if ( cd "$ROOT" && java -jar "$V3_FE" "$S/src/SlotZona.bp" --compile "$S/app" --backend=mivm ) >"$S/slot.log" 2>&1 \
       && ( cd "$S/app" && java -cp "$PACKJAR" basicplus.pack.PackCli build --out "$S/SlotPk.pack" --name SlotPk --version 1 --date 0 SlotZona.mod ) >>"$S/slot.log" 2>&1; then
      o="$( cd "$S/vacio" && "$V3_C" --libdir=../lib --pack=../SlotPk.pack SlotZona.mod 2>"$S/slot.err" | filt )"
      if printf '%s' "$o" | grep -q "manejador de la zona llamado" && printf '%s' "$o" | grep -qx "fin"; then echo "  ok  zona (VM-C): el manejador por ranura de una clase de la zona salta a su codigo (#606)"
      else echo "  ✗ zona: el manejador por ranura de una clase de la zona (#606):"; printf '%s\n' "$o" | head -3 | sed 's/^/       /'; tail -2 "$S/slot.err" | sed 's/^/       /'; fails=$((fails+1)); fi
    else echo "  ✗ SlotZona no compila o no se empaqueta: $S/slot.log"; fails=$((fails+1)); fi
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
    # Dentro de `check`, el sim que acaba de construir check_sim (#537): en un
    # worktree el de siempre no existe y este caso se saltaba; suelto, el de siempre.
    local SIMEXE="${SIM_RED:-$ROOT/bpgenvm-c/build/bpvm-sim.exe}"
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
  # El total, a mano: al añadir un caso, súmalo (8-oct: decía 13 y ya eran 14; con el de #650, 15).
  echo "  sombra: $((15 - fails))/15 casos"
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

# V7/H6 G5.b0 (#575, 26-sep) — los `.mdn` de los packs en la ABI que habla la VM. Subir
# MDN_ABI_VERSION sin regenerarlos deja SQLite (y hasta #623, la sonda Gui2) sin sus natives en TODAS las placas con
# imagen nueva (sus `native` externas no tienen otro cuerpo que uno que lanza): paso el 20-ago
# (#504) y otra vez el 26-sep con la 9, aunque el LEEME de SQLite lo decia. Mira las dos puntas
# (mdn_format.h y MdnPack), los ingredientes versionados y cada .pack grabable. Ver packs_abi.py.
check_packs_abi() {
  echo "-- los .mdn de los packs en la ABI de la VM (#575) --"
  python "$HERE/packs_abi.py"
}

# V7 #668 (8-oct) — la stdlib EMBEBIDA de las imágenes (pico_mods.c, esp32_mods.c, stm32_mods.c) es la de bpstdlib: la
# imagen la repone en /lib al arrancar, y la del 2-oct habría devuelto las placas a la stdlib de antes de #668.
check_mods_embebidos() {
  echo "-- la stdlib embebida de las imagenes, la de bpstdlib (#668: la imagen la repone en /lib al arrancar) --"
  python "$ROOT/bpgenvm-c/scripts/check_mods_embebidos.py"
}

# check_behaviour / check_opcodes / check_emit se conservan intactas arriba: son
# el registro de lo que se verificaba contra V2 y siguen siendo reutilizables si
# algun dia hay algo que comparar. Desde V4 NO se llaman (ver el aviso de abajo).
aviso_v2() {
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
}

# La red de UNA EN UNA (`check-serie`): la de siempre hasta el 23-oct. Desde V8 #726, `check` corre en tres grupos a
# la vez (abajo); esta se queda para comparar, o si se sospecha de los grupos.
check_serie() {
  aviso_v2
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
  # #537 — el micro simulado ANTES que la sombra: construye el sim de la red, y el
  # caso del wire de la sombra lo usa.
  check_sim || fails=$((fails+1))
  # #541 — las pruebas de C del AOT y del depurador (despues del sim, que deja
  # construidos los objetos de la biblioteca).
  check_testc || fails=$((fails+1))
  check_sombra || fails=$((fails+1))
  check_mod8 sin-referencia || fails=$((fails+1))
  # V7/H7 (#599) — el mismo corpus con dos workers, contra la misma referencia.
  check_smp sin-referencia || fails=$((fails+1))
  # V7/H7 #610, #611 — el estres del heap: lo que casi siempre sale bien (la tabla de handles que crece, sin GC;
  # el heap justo con varios workers, en las dos VMs); y #613, un hilo que cede con cuatro workers.
  check_estres || fails=$((fails+1))
  check_packs_abi || fails=$((fails+1))
  check_mods_embebidos || fails=$((fails+1))
  echo "== check: $([ "$fails" -eq 0 ] && echo 'paridad dual-VM (MOD9) + capsula V6 + norma + sim + pruebas de C + sombra + MOD8 + dos workers + estres del heap + packs-abi + stdlib embebida VERDE' || echo "$fails frente(s) en ROJO") =="
  [ "$fails" -eq 0 ]
}

# ------------------------------------------------------------- la red en tres grupos (V8 #726)
# Eduardo, 23-oct: «Si podemos dividir por 3 y cada uno lleva 15 min, pasamos de 45 min a 15 min» y «De momento con 3
# grupos es suficiente [...] nos permite seguir trabajando en paralelo con lo siguiente pendiente». La red de una en una,
# medida ese día con la hora en cada línea: 22:51 min —las tres pasadas del corpus, 3:34 + 3:35 + 3:42; las pruebas de
# C, 8:52; la cápsula, la norma, el sim, la sombra y lo demás, 2:44; el estrés, 0:24—. Aquí, tres grupos A LA VEZ, cada
# uno en su propio $WORK (el `.rc.java` de run_vm y los .mod de la paridad son de su grupo):
#   A  el corpus: MOD9, MOD8 y dos workers, en ese orden —las dos últimas comparan su stdout con el de MOD9 sample a
#      sample, y por eso van detrás y en el mismo grupo—;
#   B  las pruebas de C: comparten bpgenvm-c/build y las fixtures de bpgenvm-c/samples/, y siguen de una en una;
#   C  lo demás: la cápsula V6, hacia delante, la norma, packs-abi y la stdlib embebida.
# ANTES, en serie, `make`: los .o de la VM-C los usan el sim (C) y las pruebas de C (B) —si hubiera que compilarlos, a
# la vez se pisarían—, y diez pruebas de C dependen del binario de la VM que el corpus (A) está ejecutando: tiene que
# estar hecho antes. Con el host compilado con LVGL no se compila nada (cada frente lo dice, como siempre). DESPUÉS,
# SOLOS, lo que cronometra, que con los otros grupos corriendo podría fallar sin razón: el sim —sus smokes miden
# tiempos: el caso G de run_tarea_smoke compara 200 RUN de una manera y de otra, y el 24-oct, con la carga de los otros
# grupos, no cuadró (1,38 s contra 0,52)—, la sombra, que usa el sim que acaba de construir check_sim, y el estrés, que
# mide hilos y el heap justo con varios workers.
# Lo que se comparte y no se pisa: los puertos (los smokes del sim, 5107-5131; la sombra, 5461; test-net, 47391;
# test-listen, uno libre) y los .mod de las fixtures en samples/, que el corpus no lee —un hermano de un sample del
# corpus («InvocaRanuraBase») el compilador lo toma de su fuente: «es un resto fuera de outDir»—.
# Cada frente deja su salida en $LOGS/<n>-<frente>.log; mientras corren sale una línea por frente acabado, y al final
# las salidas enteras, en el orden de siempre. `check-serie` es la red de antes, de una en una (y se queda).
# Y EL PROGRESO (24-oct; Eduardo: «cuando ejecutas el worker sería posible ver una barra de progreso o algo
# similar?»): en los frentes largos —las pasadas del corpus y las pruebas de C, de 3 a 9 minutos—, una línea cada 20
# casos con por dónde van, como la del frente acabado. Va por el descriptor 3, que `check` abre sobre su salida, y no al
# registro del frente: el informe del final sale como siempre. Fuera de `check` (check-serie, check-paridad…), nada.
LOGS=""
PROG_G=""; PROG_NOM=""; PROG_T0=0
progreso() {   # $1=lo hecho, «60/164»
  [ -n "$PROG_G" ] || return 0
  printf '  %s  %s  %-16s %-7s %s\n' "$(date +%H:%M:%S)" "$PROG_G" "$PROG_NOM" "$1" "$(mmss $(( SECONDS - PROG_T0 )))" >&3
}
frente() {   # $1=grupo $2=fichero, sin extension $3=nombre para el progreso $4...=la funcion y sus argumentos
  local g="$1" f="$2" nom="$3" t=$SECONDS rc; shift 3
  PROG_G="$g"; PROG_NOM="$nom"; PROG_T0=$SECONDS
  "$@" > "$LOGS/$f.log" 2>&1; rc=$?
  echo "$rc" > "$LOGS/$f.rc"
  printf '  %s  %s  %-16s %s  %s\n' "$(date +%H:%M:%S)" "$g" "$nom" "$([ "$rc" -eq 0 ] && echo 'ok  ' || echo 'ROJO')" \
         "$(mmss $(( SECONDS - t )))"
}
mmss() { printf '%d:%02d' $(( $1 / 60 )) $(( $1 % 60 )); }
seg_de() { cat "$LOGS/$1.seg" 2>/dev/null || echo 0; }
FRENTES="01-paridad 02-mod7 03-fwd 04-norma 05-sim 06-testc 07-sombra 08-mod8 09-smp 10-estres 11-packs-abi 12-embebidos"
check() {
  aviso_v2
  exec 3>&1   # el progreso (progreso()): a la salida de `check`, no al registro de cada frente
  local fails=0 f t0=$SECONDS pa pb pc
  LOGS="$WORK/red"; mkdir -p "$LOGS" "$REF_PAR"
  if ls "$ROOT"/bpgenvm-c/build/.flavor-*lvgl1 >/dev/null 2>&1; then
    :   # con LVGL no se compila nada: cada frente que lo necesita lo dice (⚠), como en check-serie
  elif ! make -C "$ROOT/bpgenvm-c" >"$WORK/.make.log" 2>&1; then
    echo "  ✗ la VM-C no compila (make), y sin ella la red no prueba nada:"
    grep -iE "error" "$WORK/.make.log" | head -6 | sed 's/^/       /'
    echo "== check: ROJO (la VM-C no compila) =="
    return 1
  fi
  echo "-- la red en tres grupos a la vez (#726): A, el corpus; B, las pruebas de C; C, lo demas. Y al final, solos, el sim, la sombra y el estres --"
  ( WORK="$WORK/gA"; mkdir -p "$WORK"
    PAR_SAVE="$REF_PAR"; frente A 01-paridad "paridad MOD9" check_parity; PAR_SAVE=""
    frente A 08-mod8 "MOD8" check_mod8 sin-referencia
    frente A 09-smp "dos workers" check_smp sin-referencia
    echo $(( SECONDS - t0 )) > "$LOGS/grupoA.seg" ) & pa=$!
  ( WORK="$WORK/gB"; mkdir -p "$WORK"
    frente B 06-testc "pruebas de C" check_testc
    echo $(( SECONDS - t0 )) > "$LOGS/grupoB.seg" ) & pb=$!
  ( WORK="$WORK/gC"; mkdir -p "$WORK"
    frente C 02-mod7 "capsula V6" check_mod7
    frente C 03-fwd "hacia delante" check_fwd
    frente C 04-norma "norma" check_norma
    frente C 11-packs-abi "packs-abi" check_packs_abi
    frente C 12-embebidos "stdlib embebida" check_mods_embebidos
    echo $(( SECONDS - t0 )) > "$LOGS/grupoC.seg" ) & pc=$!
  # Un Ctrl-C para los tres: en un script sin control de trabajos, lo que va con `&` no recibe el SIGINT.
  trap 'kill $pa $pb $pc 2>/dev/null; rm -rf "$WORK"; exit 130' INT TERM
  wait $pa $pb $pc
  trap - INT TERM
  frente - 05-sim "sim" check_sim
  frente - 07-sombra "sombra" check_sombra
  frente - 10-estres "estres" check_estres
  echo ""
  for f in $FRENTES; do
    if [ -f "$LOGS/$f.log" ]; then cat "$LOGS/$f.log"; else echo "  ✗ $f: NO ha corrido (su grupo acabo antes)"; fi
    [ "$(cat "$LOGS/$f.rc" 2>/dev/null)" = 0 ] || fails=$((fails+1))
  done
  echo "  (en tres grupos a la vez: A, el corpus, $(mmss "$(seg_de grupoA)"); B, las pruebas de C, $(mmss "$(seg_de grupoB)"); C, lo demas, $(mmss "$(seg_de grupoC)"); y despues, solos, el sim, la sombra y el estres. En total, $(mmss $(( SECONDS - t0 ))))"
  echo "== check: $([ "$fails" -eq 0 ] && echo 'paridad dual-VM (MOD9) + capsula V6 + norma + sim + pruebas de C + sombra + MOD8 + dos workers + estres del heap + packs-abi + stdlib embebida VERDE' || echo "$fails frente(s) en ROJO") =="
  [ "$fails" -eq 0 ]
}

case "${1:-}" in
  gen)        gen ;;
  gen-v6)     gen_v6 ;;
  check)      check ;;         # V8 #726: en tres grupos a la vez
  check-serie) check_serie ;;  # la de antes, de una en una
  check-mod7) check_mod7 ;;
  check-fwd)  check_fwd ;;
  check-norma) check_norma ;;
  check-sombra) check_sombra ;;
  check-sim)  check_sim ;;
  check-testc) check_testc ;;
  check-mod8) check_mod8 ;;
  check-smp)  check_smp ;;
  check-estres) check_estres ;;
  check-packs-abi) check_packs_abi ;;
  check-mods-embebidos) check_mods_embebidos ;;
  check-paridad) PAR_SAVE="${PARIDAD_GUARDA:-}"; [ -n "$PAR_SAVE" ] && mkdir -p "$PAR_SAVE"
              check_parity ;;   # V7/H9 (2-oct): sólo la paridad del corpus. V8 (#711): con PARIDAD_GUARDA=<dir>, el stdout
                                # de cada sample (el de la VM-Java) queda en <dir>: el antes y el después de un cambio, que
                                # la paridad sola no ve (si las dos VMs cambian igual, sigue verde)
  check-jc)   check_jc ;;          # V8 #698 (18-oct): el corpus, también en JC
  *) echo "uso: $0 {gen|gen-v6|check|check-serie|check-mod7|check-fwd|check-norma|check-sim|check-testc|check-sombra|check-mod8|check-smp|check-estres|check-packs-abi|check-paridad|check-jc}"; exit 2 ;;
esac
