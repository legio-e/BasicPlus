#!/usr/bin/env bash
# ============================================================================
# h13-lista.sh — comprueba la LISTA de programas de H13 antes de llevarla a las
# placas: cada uno compila con el frontend de hoy y, si no es de pantalla, corre
# en la VM-C del PC.
#
# POR QUÉ: una lista de pruebas con programas que ya no compilan es peor que no
# tener lista — se gasta el tiempo de placa depurando el sample, no el firmware.
# Esto es la Puerta 0 del guión (docs/H13_PRUEBAS.md) aplicada a los programas.
#
#   bash scripts/h13-lista.sh          todo
#   bash scripts/h13-lista.sh M F      sólo los grupos M y F
#
# Los de GUI se COMPILAN pero no se ejecutan: abrirían ventana. Su prueba real
# es en placa (o en el micro simulado), que es justo de lo que va H13.
# ============================================================================
set -u

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
FE="$RAIZ/lexer-java/target/basicplus-frontend.jar"
VMC="$RAIZ/bpgenvm-c/build/bpgenvm-c.exe"
STD="$RAIZ/bpstdlib"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT

for f in "$FE" "$VMC"; do
    [ -f "$f" ] || { echo "FALTA $f"; exit 1; }
done

# --- la lista: grupo | fichero .bp | qué demuestra -------------------------
# El grupo manda el orden de la tanda en placa. `gui` en la 4ª columna = SÓLO SE
# COMPILA aquí: necesita hardware (pantalla, PIO...) que el PC no tiene. El
# nombre viene de que al principio eran todos GUI.
LISTA="
M|samples/MemT1_Oo.bp|escalon 1: objetos y campos
M|samples/MemT2_StrField.bp|escalon 2: campo string (referencia) en objeto
M|samples/MemT3_Inherit.bp|escalon 3: herencia + super() + virtual
M|samples/MemT4_Concat.bp|escalon 4: concat en bucle (realloc de string)
M|samples/MemT5_Gc.bp|escalon 5: churn de GC (~440 KB)
M|samples/MemStress.bp|el aislante completo: OO + concat + GC
M|samples/StrGcTest.bp|regresion #1 del censo: StringBuilder.chars
M|samples/AnyGcHard.bp|#14: el objeto vive SOLO en un campo Object
M|samples/ConcatObjTest.bp|auto-toString de objetos al concatenar
M|samples/smp_heap_stress_pico.bp|estres de heap dimensionado a la Pico
F|samples/FileTest.bp|read/write/append basicos
F|samples/FileOpsTest.bp|mkdir/rmdir/copyFile/isDirectory
F|samples/FileBytesTest.bp|I/O binario con byte[]
F|samples/FsPowerCut.bp|torturador de corte de corriente
F|samples/FsImportTest.bp|import de un modulo subido al FS
F|samples/JsonDemo.bp|Json de la stdlib (parse + serialize)
F|samples/LogTest.bp|modulo Log a fichero
F|samples/CompressFileTest.bp|descompresion desde fichero
G|samples/GuiDemo.bp|el primer programa GUI|gui
G|samples/GuiColorDemo.bp|color (el que dio guerra en el P4)|gui
G|samples/GuiGeomDemo.bp|geometria explicita x/y/w/h|gui
G|samples/GuiValueDemo.bp|switch, slider, bar|gui
G|samples/GuiCheckDemo.bp|checkbox + onChange|gui
G|samples/GuiInputDemo.bp|dropdown + textarea|gui
G|samples/GuiListKbd.bp|list + keyboard|gui
G|samples/GuiTabDemo.bp|tabview|gui
G|samples/GuiMsgDemo.bp|msgbox modal|gui
G|samples/GuiTableDemo.bp|tabla de celdas|gui
G|samples/GuiImageDemo.bp|imagen (asset + control)|gui
G|samples/GuiFontDemo.bp|catalogo de fuentes|gui
G|samples/GuiRotDemo.bp|rotacion del display|gui
G|samples/GuiClickDemo.bp|eventos de clic|gui
G|samples/GuiAsyncDemo.bp|trabajo largo sin congelar la UI|gui
G|bpgenvm-c/samples/ChartDemo.bp|widget Chart (NUEVO en V4)|gui
E|samples/EvFull.bp|el ciclo completo de un evento
E|samples/EvOrder.bp|se atienden en el orden en que se disparan
E|samples/EvNest.bp|un handler dispara otro evento (reentrada)
E|samples/EvThrow.bp|un handler que atrapa lo suyo: el despacho sigue y acaba bien
E|samples/EvFin.bp|#342: el evento del thread que muere no se pierde
E|samples/h5cevuse.bp|eventos CROSS-MODULE
T|samples/AsyncDemo.bp|#325: Thread(obj::metodo(args))
T|samples/ThreadTrasMain.bp|#346: acaba cuando acaban TODOS los threads
T|samples/mutextest.bp|mutex bajo contencion
T|samples/synclisttest.bp|SyncList productor/consumidor
T|samples/preempttest.bp|worker CPU-bound: la consola sigue viva
T|samples/synctest.bp|sync property de CLASE (get/set envueltos en Mutex)
T|samples/modpropsync.bp|sync property de MODULO (el Mutex vive en un global)
T|samples/l2app.bp|sync property CROSS-MODULE (el lock, en el modulo dueno)
T|samples/paralleltest.bp|bloque parallel/case/default/endpar
T|samples/paralleltest_sugar.bp|el azucar del parallel, con default
L|samples/TupleFirstClass.bp|tuplas first-class
L|samples/TupCrossTest.bp|tuplas cruzando modulo
L|samples/DefaultParams.bp|parametros con valor por defecto
L|samples/StaticPropTest.bp|static property de clase
L|samples/narrowtypes.bp|tipos enteros estrechos
L|samples/Field8Test.bp|campos de 8 bytes (long/double)
L|samples/Wrap8Test.bp|envoltorios de 8 bytes cross-module
L|samples/PropLongTest.bp|properties long/double
X|samples/ExcCatchTest.bp|try/catch y jerarquia de excepciones
X|samples/stacktrace.bp|una excepcion sube 3 niveles y se atrapa arriba, con su mensaje
X|samples/OoSmoke.bp|humo de OO
X|samples/StrOps348.bp|operaciones de cadena (#348)
X|samples/MathOps348.bp|operaciones de Math (#348)
X|samples/PathOps348.bp|rutas, siempre con '/'
X|samples/RandomTest.bp|aleatorios contra su CONTRATO: rango, limite excluido, reparto
D|samples/blink.bp|GPIO en GP25 — SOLO a1: en la Metro GP25 es el NeoPixel|gui
D|samples/NeoDemo.bp|WS2812 por PIO — en la METRO sustituye a blink (mismo GP25)|gui
D|samples/BoardTest.bp|identificacion de la placa|gui
"

GRUPOS="${*:-M F G E T L X D}"

ok=0; nocomp=0; falla=0; solocomp=0
grupo_actual=""
printf '\n'
while IFS='|' read -r g bp desc modo; do
    [ -z "${g:-}" ] && continue
    case " $GRUPOS " in *" $g "*) ;; *) continue ;; esac
    if [ "$g" != "$grupo_actual" ]; then grupo_actual="$g"; printf '  == grupo %s ==\n' "$g"; fi

    nombre="$(basename "$bp" .bp)"
    if [ ! -f "$RAIZ/$bp" ]; then
        printf '  NO ESTA  %-22s %s\n' "$nombre" "$bp"; nocomp=$((nocomp+1)); continue
    fi

    rm -rf "$WORK"; mkdir -p "$WORK"
    if ! java -jar "$FE" "$RAIZ/$bp" --compile "$WORK" --backend=mivm \
            --dependencyPaths "$STD" >"$WORK/fe.log" 2>&1; then
        printf '  NOCOMPILA %-21s %s\n' "$nombre" "$(grep -m1 -iE 'error' "$WORK/fe.log" | cut -c1-60)"
        nocomp=$((nocomp+1)); continue
    fi
    mod="$(ls "$WORK"/*.mod 2>/dev/null | grep -viE '/(Core|Str|Math|IO|Collections|Json|Log|Compress|Gui|Time)\.mod$' | head -1)"
    [ -z "$mod" ] && mod="$(ls "$WORK"/*.mod 2>/dev/null | head -1)"
    if [ -z "$mod" ]; then
        printf '  NOCOMPILA %-21s (sin .mod)\n' "$nombre"; nocomp=$((nocomp+1)); continue
    fi

    if [ "${modo:-}" = "gui" ]; then
        printf '  compila   %-21s %s\n' "$nombre" "$desc"; solocomp=$((solocomp+1)); continue
    fi

    # La stdlib AL LADO del .mod: es lo que la VM busca, y en la placa está en
    # /lib. Sin esto un programa que importe Json/Log/Compress muere al cargar y
    # parece un fallo del sample cuando el que se ha equivocado es el arnés.
    # -n: NO pisar lo que acaba de compilar el frontend. bpstdlib/ arrastra
    # .mod rancios (v5) de módulos que NO son stdlib —L2Lib entre ellos—, y un
    # `cp` a secas sustituía el módulo recién generado por el caducado. El
    # sample parecía roto y el roto era el arnés.
    cp -n "$STD"/*.mod "$WORK/" 2>/dev/null

    salida="$(cd "$WORK" && timeout 90 "$VMC" "$(basename "$mod")" 2>&1)"; rc=$?
    # Los que TERMINAN mal a propósito (excepción sin atrapar) aprueban al revés.
    if [ "${modo:-}" = "peta" ]; then
        if [ $rc -ne 0 ]; then
            printf '  OK(peta)  %-21s %s\n' "$nombre" "$desc"; ok=$((ok+1))
        else
            printf '  FALLA     %-21s tenia que terminar mal y termino bien\n' "$nombre"
            falla=$((falla+1))
        fi
        continue
    fi
    if [ $rc -eq 0 ]; then
        printf '  OK        %-21s %s\n' "$nombre" "$desc"; ok=$((ok+1))
    else
        printf '  FALLA(%-3s)%-21s %s\n' "$rc" "$nombre" "$(printf '%s' "$salida" | tail -1 | cut -c1-52)"
        falla=$((falla+1))
    fi
done <<EOF
$LISTA
EOF

printf '\n  %d corren  ·  %d compilan (de pantalla)  ·  %d NO compilan  ·  %d fallan\n\n' \
       "$ok" "$solocomp" "$nocomp" "$falla"
[ "$nocomp" -eq 0 ] && [ "$falla" -eq 0 ]
