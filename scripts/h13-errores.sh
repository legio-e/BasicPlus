#!/usr/bin/env bash
# ============================================================================
# h13-errores.sh — comprueba que los ejemplos de `samples/errores/` SIGUEN
# fallando, y con el mensaje que toca.
#
# La carpeta de errores no es un cajón: es una prueba. Falla en los DOS sentidos:
#   - si uno empieza a COMPILAR, hemos perdido una regla del lenguaje sin enterarnos;
#   - si el mensaje CAMBIA, el diagnóstico ha empeorado (o mejorado, y hay que
#     actualizar la expectativa a conciencia, no de rebote).
#
# Lo segundo es lo que de verdad se escapa: un refactor puede dejar el error pero
# convertirlo en una traza ilegible, y sin esto nadie se entera hasta que lo sufre
# alguien que acaba de instalar.
#
#   bash scripts/h13-errores.sh
# ============================================================================
set -u

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
FE="$RAIZ/lexer-java/target/basicplus-frontend.jar"
DIR="$RAIZ/samples/errores"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT

[ -f "$FE" ] || { echo "FALTA $FE"; exit 1; }

# fichero | trozo del mensaje que DEBE aparecer
# El trozo se elige por lo que NO debería cambiar aunque se reescriba la frase:
# el nombre del símbolo, el número fuera de rango, la regla. Nada de acentos, que
# la consola de Windows los destroza y el grep dejaría de casar por el terminal.
CASOS="
BadSyntax|se esperaba una expresi
DefaultParamsBad|debe ser una constante literal
DefaultParamsBad|no puede ir tras uno con valor por defecto
classleak|no puede llamar a 'helperPrivado'
narrowtypes_errors|literal 300 fuera del rango de byte
narrowtypes_errors|fuera del rango de int8
paralleltest_scope_violate|no puede acceder a la variable global 'globalVar'
"

ok=0; mal=0
printf '\n'
while IFS='|' read -r nombre esperado; do
    [ -z "${nombre:-}" ] && continue
    bp="$DIR/$nombre.bp"
    if [ ! -f "$bp" ]; then
        printf '  NO ESTA  %-28s\n' "$nombre"; mal=$((mal+1)); continue
    fi
    rm -rf "$WORK"; mkdir -p "$WORK"
    salida="$(java -jar "$FE" "$bp" --compile "$WORK" --backend=mivm 2>&1)"

    if [ -n "$(ls "$WORK"/*.mod 2>/dev/null)" ]; then
        printf '  COMPILA  %-28s <- tenia que FALLAR: se ha perdido la regla\n' "$nombre"
        mal=$((mal+1)); continue
    fi
    if printf '%s' "$salida" | grep -qF "$esperado"; then
        printf '  OK       %-28s %s\n' "$nombre" "$esperado"; ok=$((ok+1))
    else
        printf '  CAMBIO   %-28s falta: %s\n' "$nombre" "$esperado"
        printf '           lo que dice ahora: %s\n' \
               "$(printf '%s' "$salida" | grep -m1 -E '^\[[0-9]+:[0-9]+\] error' | cut -c1-72)"
        mal=$((mal+1))
    fi
done <<EOF
$CASOS
EOF

printf '\n  %d comprobaciones OK  ·  %d mal\n\n' "$ok" "$mal"
[ "$mal" -eq 0 ]
