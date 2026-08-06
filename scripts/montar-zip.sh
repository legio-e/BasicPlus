#!/usr/bin/env bash
# ============================================================================
# montar-zip.sh — arma el ZIP de distribución de BasicPlus para Windows.
#
# POR QUÉ UN SCRIPT Y NO A MANO: el ZIP es lo que se prueba en H13 y lo que se
# publica, y tiene que ser el MISMO. Montarlo a mano una vez para probar y otra
# para publicar es exactamente la forma de que difieran sin que nadie lo note.
# Aquí se monta una vez, se sella, y de ahí salen las dos cosas.
#
# La ESTRUCTURA no es un gusto mío: sale de lo que el código busca.
#   packs/       IdePrefs.installSubdir("packs")      → biblioteca de packs
#   bin/         SimRunner.locateExe                  → micro simulado + SDL2
#   bpgenvm-c/   AotBuild.autodetectBpgenvm           → include/ + src/ del AOT
#   docs/        IdePrefs.docsDir()                   → la ayuda (F1)
#   firmware/    (no lo busca nadie: es para el usuario)
# Si alguien cambia esas rutas en el código, este script deja de servir — y se
# nota en la prueba de instalación (bloque J del guión), que es su red.
#
#   bash scripts/montar-zip.sh
# ============================================================================
set -euo pipefail

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
VER="$(sed -n "s@.*<version>\(.*\)</version>.*@\1@p" "$RAIZ/BpIde/pom.xml" | head -1)"
NOMBRE="BasicPlus-${VER}-win"
OUT="$RAIZ/dist/$NOMBRE"
JAR="$RAIZ/BpIde/target/BpIde-${VER}.jar"

echo "== BasicPlus $VER =="

# El jar manda: si no está, no hay nada que empaquetar (y la versión sale del pom,
# así que un jar viejo con otro nombre se detecta aquí y no en la release).
[ -f "$JAR" ] || { echo "FALTA $JAR — 'mvn package' en BpIde primero"; exit 1; }

rm -rf "$OUT"; mkdir -p "$OUT"/{bin,packs,docs,firmware,bpstdlib,bpdevices,samples}

# --- el IDE -----------------------------------------------------------------
cp "$JAR" "$OUT/"
printf '@echo off\r\njava -jar "%%~dp0BpIde-%s.jar" %%*\r\n' "$VER" > "$OUT/bpide.bat"

# --- BpVM.cfg: cómo encuentra el compilador la stdlib -----------------------
# SIN esto el paquete no compila NADA que importe la stdlib. El ZIP lleva los
# .mod en bpstdlib/, pero `locateImportMod` sólo mira fromPath, outDir, el dir
# del fuente y `dependencyPaths` — y dependencyPaths se rellena leyendo este
# fichero. Sin él, `import Core` no resuelve, el compilador lo OMITE y los
# errores salen luego en el código del usuario (H13, Pico: ExcCatchTest daba 4
# errores que no eran suyos).
#
# Las rutas van RELATIVAS a propósito: VmConfig.resolveRelativo las resuelve
# contra el directorio de ESTE fichero, no contra el directorio de trabajo. Así
# la instalación se puede mover de sitio y arrancar desde donde sea.
cat > "$OUT/BpVM.cfg" <<'CFG'
{
  "stdlibDir":  "./bpstdlib",
  "devicesDir": "./bpdevices"
}
CFG

# --- micro simulado (el IDE lo busca en bin/) --------------------------------
cp "$RAIZ/bpgenvm-c/build/bpvm-sim.exe" "$OUT/bin/"
cp "$RAIZ/bpgenvm-c/build/SDL2.dll"     "$OUT/bin/"

# --- AOT: sólo include/ + src/, que es lo que comprueba looksLikeBpgenvm ------
mkdir -p "$OUT/bpgenvm-c"
cp -r "$RAIZ/bpgenvm-c/include" "$OUT/bpgenvm-c/"
cp -r "$RAIZ/bpgenvm-c/src"     "$OUT/bpgenvm-c/"

# --- biblioteca + stdlib ----------------------------------------------------
cp "$RAIZ/packs/"*.pack        "$OUT/packs/"
cp "$RAIZ/bpstdlib/"*.mod      "$OUT/bpstdlib/"
cp "$RAIZ/bpdevices/"*.mod     "$OUT/bpdevices/"

# Los FUENTES de la stdlib también (hallazgo 28, 4-ago). No es un extra para
# curiosos: el IDE resuelve las dependencias del device de forma TRANSITIVA, y
# descubre los imports de cada módulo LEYENDO SU .bp (resolveDeviceDeps →
# parseImports). Sin los fuentes, `Gui` no delata que importa `Json` → Json no
# se sube → en un dispositivo VIRGEN no arranca NINGUNA demo gráfica. En el repo
# nunca se vio porque ahí los .bp están; se destapó en la P4 Waveshare recién
# particionada, la única placa de la campaña con /lib vacío.
cp "$RAIZ/bpstdlib/"*.bp       "$OUT/bpstdlib/"
cp "$RAIZ/bpdevices/"*.bp      "$OUT/bpdevices/" 2>/dev/null || true

# --- documentación: los volúmenes + sus imágenes + los anexos que enlazan ----
cp "$RAIZ/docs/"*.html "$OUT/docs/"
cp -r "$RAIZ/docs/img" "$OUT/docs/"
mkdir -p "$OUT/docs/en" && cp "$RAIZ/docs/en/"*.html "$OUT/docs/en/"

# Los anexos .md NO se eligen a mano: se sacan de lo que la ayuda enlaza. En docs/
# hay 44 .md y la mayoría son notas internas de desarrollo; elegir a ojo cuáles se
# publican es la forma de dejarse uno y que F1 dé un 404 en la instalación — que es
# exactamente lo que arregló #363.
for f in $(grep -ho 'href="[^"]*\.md"' "$RAIZ/docs/"*.html | sed 's/href="//;s/"//' | sort -u); do
    cp "$RAIZ/docs/$f" "$OUT/docs/"
done
for f in $(grep -ho 'href="[^"]*\.md"' "$RAIZ/docs/en/"*.html | sed 's/href="//;s/"//' | sort -u); do
    case "$f" in
        ../*) cp "$RAIZ/docs/${f#../}" "$OUT/docs/"    ;;
        *)    cp "$RAIZ/docs/en/$f"    "$OUT/docs/en/" ;;
    esac
done

# --- firmware: las imágenes SELLADAS, no las de los directorios de compilación
cp "$RAIZ/dist/firmware/"*.uf2 "$RAIZ/dist/firmware/"*.bin \
   "$RAIZ/dist/firmware/SHA256SUMS.txt" "$RAIZ/dist/firmware/README.md" "$OUT/firmware/"

# --- samples: los del REPO, no los de mi copia de trabajo --------------------
# `git ls-files` y no `samples/*.bp`: en la copia de trabajo viven los ficheros de
# depuración del día (NB2P0.bp, FsMinC30.bp, Hola.bp...), que no pintan nada en una
# publicación. Lo que está en el repo es lo que se ha revisado; lo demás es mío.
# OJO con el patrón entre comillas: es pathspec de git, no glob del shell, y ahí el
# `*` SÍ atraviesa `/` — sin el filtro se traía los .bp de los proyectos y los
# aplanaba aquí, pisando ficheros distintos con el mismo nombre.
# 1) LO SUELTO DE LA RAIZ — y ya no solo los .bp. El patron era 'samples/*.bp',
#    asi que un sample que se apoya en un asset a su lado viajaba MANCO: eso es
#    el hallazgo 25 (GuiImageDemo pide testimg.png, que se quedaba en el repo y
#    daba pantalla en blanco desde la instalacion). Ahora viaja todo lo que este
#    EN GIT en la raiz de samples/ — los .bp y lo que necesiten.
(cd "$RAIZ" && git ls-files -z -- 'samples/*') | while IFS= read -r -d '' f; do
    case "${f#samples/}" in */*) continue ;; esac
    cp "$RAIZ/$f" "$OUT/samples/"
done

# 2) LAS SUBCARPETAS. Antes habia aqui un `case ... */*) continue` que DESCARTABA
#    en silencio todo lo que viviera en un subdirectorio, mas tres proyectos
#    copiados a mano. Consecuencia (hallazgo 35): `samples/benchmarks/` entero se
#    quedaba fuera y con el `Bench.bp` — o sea que el AOT, el hito grande de V4,
#    se publicaba SIN UN SOLO EJEMPLO. Y `aottest/`, que es el proyecto que
#    genera el .mdn, tampoco salia.
#
#    El arreglo NO es copiarlo todo: aqui dentro hay material interno que no
#    pinta nada en una publicacion (reproducciones minimas, diagnostico de GC).
#    Hace falta ELEGIR. Lo que no puede ser es que olvidarse sea SILENCIOSO —
#    por eso, debajo, el guardian.
PUBLICAR_DIRS="aottest benchmarks errores external formdemo imageproject plugins sampleproject"
#    external/ y plugins/ no son demos: son las DEPENDENCIAS de tres samples de
#    la raiz que importan por ruta (appwithfromimpl -> plugins/filelogger.mod,
#    frompathtest -> external/helper.mod). Sin ellas esos tres no compilan desde
#    la instalacion. Misma familia que el hallazgo 6.
INTERNAS_DIRS="formev formmin holes resources"
#    formev/formmin  = reproducciones minimas de Forms, andamio de depuracion.
#    holes/          = diagnostico de agujeros de GC (21 ficheros).
#    resources/      = copia duplicada de testimg.png; el bueno es el de la raiz.

for p in $PUBLICAR_DIRS; do
    [ -d "$RAIZ/samples/$p" ] || continue
    cp -r "$RAIZ/samples/$p" "$OUT/samples/"
    # Y FUERA su out/. Un proyecto se publica con sus FUENTES; el .mod/.pack lo
    # hace `Build Project` en la máquina del usuario. Copiarlo tal cual metía en
    # el ZIP el out/ de mi copia de trabajo: en formdemo/out/ vivían 26 .mod v5
    # —los 23 demos de Gui y L2Lib, los mismos que se colaron en bpstdlib— y de
    # ahí salían. Ojo: esto NO lo arregla .gitignore, porque aquí se copia del
    # disco, no del índice de git.
    # A CUALQUIER profundidad, no sólo en la raíz del proyecto: en formdemo había
    # además un src/out/ de una compilación antigua. Ir quitándolos de uno en uno
    # es cómo se te escapa el siguiente.
    find "$OUT/samples/$p" -type d -name out -prune -exec rm -rf {} +
done

# 3) EL GUARDIAN — esto es el arreglo de verdad del hallazgo 35. Las dos listas
#    de arriba hay que mantenerlas a mano (elegir que ve el usuario es una
#    DECISION, no se puede automatizar), pero olvidarse ya no sale gratis: si
#    aparece en samples/ una subcarpeta CON FICHEROS EN GIT que no este en
#    ninguna de las dos, el paquete NO SE MONTA.
#    Sin esto, `benchmarks/` llevaba meses cayendose del ZIP sin que nadie lo
#    notara — y no se noto hasta que Eduardo fue a buscar el Bench en la
#    instalacion y no estaba.
#    Filtro "con ficheros en git": las carpetas de build (out/) y la basura de un
#    comando mal escrito no tienen nada versionado y no molestan.
DESCONOCIDAS=""
for d in "$RAIZ"/samples/*/; do
    n="$(basename "$d")"
    [ -n "$(cd "$RAIZ" && git ls-files -- "samples/$n")" ] || continue
    case " $PUBLICAR_DIRS $INTERNAS_DIRS " in
        *" $n "*) ;;
        *) DESCONOCIDAS="$DESCONOCIDAS $n" ;;
    esac
done
if [ -n "$DESCONOCIDAS" ]; then
    echo "  ERROR: subcarpeta(s) de samples/ sin decidir:$DESCONOCIDAS"
    echo "  Anadelas a PUBLICAR_DIRS (van al ZIP) o a INTERNAS_DIRS (se quedan),"
    echo "  en scripts/montar-zip.sh. Que este script NO decida por ti es aposta."
    exit 1
fi


# --- red: ni un solo .mod caducado dentro del paquete ------------------------
# En H13 (a1 Pico) el IDE subió a la placa un L2Lib.mod v5 y el sample l2app murió
# con exit 3. No era el compilador: dentro de bpstdlib/ vivían 24 .mod rancios que
# nadie había puesto ahí a propósito —23 demos de Gui y L2Lib, restos de una
# compilación vieja—, y `resolveDeviceDeps` da prioridad de stdlib a CUALQUIER
# módulo presente en stdlibDir. O sea que el intruso no era adorno: mandaba sobre
# el módulo recién generado.
#
# La regla es una y no hay lista que mantener: si un .mod del paquete no es MOD6,
# el paquete no sale. Vale igual para un intruso que para una stdlib que se quede
# atrás en la próxima subida de formato.
# Y el invariante de bpstdlib/, que es donde vivían los intrusos: un módulo de la
# librería estándar TIENE su fuente al lado. Un .mod suelto ahí no es stdlib —
# y no es inocuo: `resolveDeviceDeps` da prioridad de stdlib a cualquier módulo
# presente en stdlibDir, así que taparía al recién compilado. Esto no se puede
# poner en .gitignore (los .mod buenos SÍ se versionan): tiene que ser un check.
INTRUSOS=""
for m in "$RAIZ"/bpstdlib/*.mod; do
    b="$(basename "$m" .mod)"
    [ -f "$RAIZ/bpstdlib/$b.bp" ] || INTRUSOS="$INTRUSOS $b.mod"
done
if [ -n "$INTRUSOS" ]; then
    echo "  INTRUSOS en bpstdlib/ (.mod sin su .bp al lado):$INTRUSOS"
    echo "  Un .mod ahí manda sobre el que acabas de compilar. Sácalo de bpstdlib."
    rm -rf "$OUT"; exit 1
fi

# El MISMO invariante, pero sobre el PAQUETE MONTADO (hallazgo 28). El de arriba
# mira el repo; éste mira lo que se va a publicar, que es donde estaba roto: el
# ZIP llevaba 26 .mod y CERO .bp, y con eso la resolución transitiva del IDE se
# queda muda (descubre los imports leyendo el .bp de cada módulo). Consecuencia
# real medida en placa: en un device virgen no arranca ninguna demo gráfica,
# porque nadie sube Json y Gui depende de él.
# Es la 3ª vez en la campaña que algo funciona en el repo y está roto desde el
# ZIP (hallazgos 6, 25 y 28) — por eso el check va aquí y no en la confianza.
FALTAN=""
for m in "$OUT"/bpstdlib/*.mod; do
    b="$(basename "$m" .mod)"
    [ -f "$OUT/bpstdlib/$b.bp" ] || FALTAN="$FALTAN $b.bp"
done
if [ -n "$FALTAN" ]; then
    echo "  El PAQUETE no lleva los fuentes de la stdlib:$FALTAN"
    echo "  Sin ellos el IDE no resuelve las deps transitivas y las demos"
    echo "  gráficas no arrancan en un dispositivo virgen (hallazgo 28)."
    rm -rf "$OUT"; exit 1
fi

MALOS=""
while IFS= read -r m; do
    [ "$(head -c4 "$m")" = "MOD6" ] || MALOS="$MALOS
  $(printf '%-6s %s' "$(head -c4 "$m")" "${m#$OUT/}")"
done < <(find "$OUT" -name '*.mod')
if [ -n "$MALOS" ]; then
    echo "  .mod CADUCADOS en el paquete (se esperaba MOD6):$MALOS"
    echo "  Un .mod que la VM rechaza no se publica. Regenéralo o quítalo."
    rm -rf "$OUT"; exit 1
fi

# --- red: ningún enlace de la ayuda puede quedar roto DENTRO del paquete -----
# Se comprueba sobre el árbol YA MONTADO, que es lo que va a ver el usuario: el
# repo puede tener el fichero y el ZIP no llevarlo (pasó con los 10 anexos .md).
# Los enlaces externos (http:, mailto:) no se tocan: no son cosa del paquete.
roto=0
for html in "$OUT/docs/"*.html "$OUT/docs/en/"*.html; do
    dir="$(dirname "$html")"
    for ref in $(grep -ho 'href="[^"#][^"]*"\|src="[^"#][^"]*"' "$html" \
                 | sed 's/^[a-z]*="//;s/"$//;s/#.*//' | grep -v '^[a-z][a-z]*:' | sort -u); do
        [ -e "$dir/$ref" ] || { echo "  ENLACE ROTO  ${html#$OUT/} -> $ref"; roto=1; }
    done
done
[ "$roto" -eq 0 ] || { echo "La ayuda tiene enlaces rotos en el paquete — no se monta el ZIP"; exit 1; }

# --- el ZIP + su sello ------------------------------------------------------
cd "$RAIZ/dist"
rm -f "$NOMBRE.zip"
# NO se usa Compress-Archive de PowerShell: escribe los nombres con `\` (bug conocido
# de ZipFile en .NET Framework, que es lo que trae PS 5.1). El estándar ZIP manda `/`.
# Windows lo tolera, pero quien lo descomprima en Linux o Mac —para leer los docs o
# coger las imágenes de firmware— se encuentra ficheros llamados "BasicPlus-4.0-win\bin\
# bpvm-sim.exe", todos en el mismo montón. El `jar` del JDK siempre escribe `/`, y el
# JDK está garantizado: sin él no hay IDE que empaquetar.
if command -v zip >/dev/null 2>&1; then
    zip -qr "$NOMBRE.zip" "$NOMBRE"
else
    jar cfM "$NOMBRE.zip" "$NOMBRE"
fi

# Guardián de lo anterior: un solo `\` dentro de un nombre y el paquete no vale.
if jar tf "$NOMBRE.zip" | grep -q '\\'; then
    echo "El ZIP lleva '\\' en los nombres — se descomprime mal fuera de Windows"; exit 1
fi

# --- red: el paquete tiene que COMPILAR, no sólo tener los ficheros ---------
# El bloque J comprobaba que el IDE ENCUENTRA sus carpetas; nunca compiló nada
# de verdad desde una instalación, y por eso se coló que faltaba BpVM.cfg: el
# ZIP llevaba bpstdlib/ pero el compilador no sabía dónde estaba, omitía el
# `import Core` y los errores salían en el código del usuario (H13, Pico).
#
# Se hace sobre el ZIP EXTRAÍDO EN UN TEMPORAL, no sobre dist/: el árbol montado
# vive dentro del repo, y VmConfig sube directorios buscando BpVM.cfg — o sea que
# encontraba el del checkout y la comprobación pasaba SIEMPRE. Un guardián con la
# red de seguridad puesta no comprueba nada.
echo "-- comprobando que el paquete compila con su propia stdlib --"
PRUEBA="$(mktemp -d)"
( cd "$PRUEBA" && jar xf "$RAIZ/dist/$NOMBRE.zip" ) || { echo "  no se pudo extraer"; exit 1; }
CASO="$PRUEBA/$NOMBRE/samples/ExcCatchTest.bp"     # importa Core + un módulo propio
if ! ( cd / && java -cp "$PRUEBA/$NOMBRE/BpIde-$VER.jar" basicplus.frontend.Main \
         "$CASO" --compile "$PRUEBA/out" --backend=mivm > "$PRUEBA/log" 2>&1 ) \
   || [ -z "$(ls "$PRUEBA/out"/*.mod 2>/dev/null)" ]; then
    echo "  EL PAQUETE NO COMPILA un programa con stdlib:"
    grep -E '^\[[0-9]+:|no se localiz|sin interfaz' "$PRUEBA/log" | head -5 | sed 's/^/    /'
    rm -rf "$PRUEBA" "$NOMBRE.zip"; exit 1
fi
rm -rf "$PRUEBA"

sha256sum "$NOMBRE.zip" > "$NOMBRE.zip.sha256"

echo
echo "  $NOMBRE.zip  —  $(du -h "$NOMBRE.zip" | cut -f1)"
cat "$NOMBRE.zip.sha256"
