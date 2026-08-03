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
(cd "$RAIZ" && git ls-files -z -- 'samples/*.bp') | while IFS= read -r -d '' f; do
    case "${f#samples/}" in */*) continue ;; esac
    cp "$RAIZ/$f" "$OUT/samples/"
done
for p in sampleproject formdemo imageproject; do
    [ -d "$RAIZ/samples/$p" ] && cp -r "$RAIZ/samples/$p" "$OUT/samples/"
done

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
if command -v zip >/dev/null 2>&1; then
    zip -qr "$NOMBRE.zip" "$NOMBRE"
else   # Windows sin zip: PowerShell lo hace
    powershell -NoProfile -Command \
      "Compress-Archive -Path '$NOMBRE' -DestinationPath '$NOMBRE.zip' -Force"
fi
sha256sum "$NOMBRE.zip" > "$NOMBRE.zip.sha256"

echo
echo "  $NOMBRE.zip  —  $(du -h "$NOMBRE.zip" | cut -f1)"
cat "$NOMBRE.zip.sha256"
