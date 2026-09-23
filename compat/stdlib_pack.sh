#!/usr/bin/env bash
# compat/stdlib_pack.sh — LA COPIA DE LA STDLIB QUE VA EN EL PACK, VIGILADA.
#
#   bash compat/stdlib_pack.sh [PACK]      comprueba que PACK (por defecto packs/Stdlib.pack)
#                                          lleva EXACTAMENTE los bpstdlib/*.mod, byte a byte
#   bash compat/stdlib_pack.sh --regenerar lo reconstruye con la receta de siempre (el proyecto
#                                          bpstdlib/Stdlib.bpbuild), lo copia a packs/ y comprueba
#
# POR QUE EXISTE. packs/Stdlib.pack es lo que el IDE graba en el micro simulado (SimRunner:
# un --pack= por cada pack de la libreria) y lo que montar-zip.sh mete en el ZIP. Su README
# promete «los MISMOS .mod que lleva el firmware en /lib, byte a byte», y nada lo comprobaba:
# se quedo rancio TRES veces — el MOD6 del 20-ago que viajo en el primer ZIP de V6; y el del
# 18-sep, que la cascada de H1 P3 (20-sep) no rehizo: sus 28 modulos eran de antes de que
# Object/Thread/Mutex/StringBuilder vivieran en Core, y en el micro simulado un programa con
# StringBuilder daba LINK_ERROR. La guarda del ZIP solo buscaba MOD6 dentro del pack; un pack
# MOD9 viejo la pasaba.
#
# Lo que comprueba, con el lector CANONICO (PackCli, que ademas verifica los CRC del pack):
#   1. cada .mod del pack es BYTE A BYTE el de bpstdlib/;
#   2. no falta ninguno: todo bpstdlib/*.mod esta en el pack (un modulo nuevo sin su import
#      en Stdlib.bp no entra en el pack, y eso tambien es un pack rancio);
#   3. no sobra ninguno.
# Sale 0 si cuadra; 1 con la lista de lo que no cuadra y la orden que lo arregla; 2 si no
# se puede comprobar (sin java o sin el frontend): una guarda que no mira no da verde.
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
FE="$ROOT/lexer-java/target/basicplus-frontend.jar"
STD="$ROOT/bpstdlib"
REGEN=0
PACK="$ROOT/packs/Stdlib.pack"
for a in "$@"; do
  case "$a" in
    --regenerar) REGEN=1 ;;
    *)           PACK="$a" ;;
  esac
done

command -v java >/dev/null 2>&1 || { echo "  ✗ stdlib del pack: no hay java — no se puede comprobar"; exit 2; }
[ -f "$FE" ] || { echo "  ✗ stdlib del pack: falta $FE (mvn -f lexer-java/pom.xml package) — no se puede comprobar"; exit 2; }

if [ "$REGEN" -eq 1 ]; then
  # La receta de packs/README.md, tal cual: el pack sale del PROYECTO de la stdlib (el mismo
  # compilador que construye lo que distribuimos), no de un empaquetado aparte.
  echo "-- regenerando packs/Stdlib.pack (bpstdlib/Stdlib.bpbuild) --"
  mkdir -p "$STD/out"
  ( cd "$ROOT" && java -jar "$FE" --project bpstdlib/Stdlib.bpbuild --backend=mivm --prune-bpi ) \
      > "$STD/out/.stdlib_pack.log" 2>&1 \
    || { echo "  ✗ el proyecto de la stdlib no compila: $STD/out/.stdlib_pack.log"; exit 1; }
  cp "$STD/out/Stdlib.pack" "$ROOT/packs/Stdlib.pack"
  PACK="$ROOT/packs/Stdlib.pack"
fi

[ -f "$PACK" ] || { echo "  ✗ stdlib del pack: no existe $PACK"; exit 1; }

T="$(mktemp -d)"
trap 'rm -rf "$T"' EXIT
java -cp "$FE" basicplus.pack.PackCli extract "$PACK" "$T" > "$T/.extract.log" 2>&1 \
  || { echo "  ✗ stdlib del pack: $PACK no se puede leer:"; sed 's/^/       /' "$T/.extract.log" | head -3; exit 1; }

malos=""; n=0
for m in "$T"/*.mod; do
  [ -f "$m" ] || continue
  b="$(basename "$m")"
  if [ ! -f "$STD/$b" ]; then
    malos="$malos
       sobra $b (esta en el pack y no en bpstdlib/)"
  elif ! cmp -s "$m" "$STD/$b"; then
    malos="$malos
       ≠ $b (pack $(wc -c < "$m") B, bpstdlib $(wc -c < "$STD/$b") B)"
  else
    n=$((n+1))
  fi
done
for m in "$STD"/*.mod; do
  b="$(basename "$m")"
  [ -f "$T/$b" ] || malos="$malos
       falta $b (esta en bpstdlib/ y no en el pack: ¿su import en Stdlib.bp?)"
done

rel="${PACK#$ROOT/}"
if [ -n "$malos" ]; then
  echo "  ✗ $rel NO es la stdlib de bpstdlib/ ($n modulos iguales):$malos"
  echo "     Un pack rancio lo graba el IDE en el micro simulado y viaja en el ZIP."
  echo "     Se arregla con:  bash compat/stdlib_pack.sh --regenerar"
  exit 1
fi
echo "  ok  $rel == bpstdlib/*.mod ($n modulos, byte a byte)"
