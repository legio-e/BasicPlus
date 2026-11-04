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
EnumMal|'Color' es un enum: no se llama ni se convierte desde un entero
EnumMal|'string' y 'integer[]' (no hay conversi
EnumMal|'string' y '(integer, string)' (no hay conversi
EnumMal|'string' y 'null' (no hay conversi
EnumMal|'string' y 'void' (no hay conversi
TopeClase|la clase 'Ancha' ocupa 258 ranuras de campos
TopeClase|la clase 'Larga' tiene 257 m
StepCero|no puede ser 0
StepCero|debe ser integer, no 'float'
CtorMal|no puede delegar en s
CtorMal|se delegan en ciclo
CtorMal|no puede llamar adem
CtorMal|'Cerrada()' es private
CtorMal|'Familiar()' es protegido
CtorMal|'Base()' es private
CtorMal|no define constructor; super() no puede llevar argumentos
ClaseDelLenguaje|'Thread' es una clase del lenguaje
ClaseDelLenguaje|'StringBuilder' es una clase del lenguaje
ClaseDelLenguaje|'root class' s
PrefijoSistema|'__contador' empieza por
PrefijoSistema|'__dentro' empieza por
PrefijoSistema|'__privada' empieza por
PrefijoSistema|'__a' empieza por
PrefijoSistema|'__tmp' empieza por
DestructorMal|se llama como su clase: '~Caja'
DestructorMal|el destructor no lleva par
DestructorMal|no puede escapar: se destruye
DestructorMal|no puede escapar a un campo
DestructorMal|el destructor lo llama el lenguaje
ArrayFijoMal|puede ser una variable local o un campo
ArrayFijoMal|de clase tiene que ser
ArrayFijoMal|'buf' es un array fijo: no es una referencia
NativeCampo|en el metodo native 'Caja.doble'
NativeCampo|lee el campo 'n'
NativeConversion|en la funcion native 'patasDe'
NativeConversion|convertir a 'Perro'
ProtegidoMal|desde 'B' se usa sobre un 'B' o una hija suya, y este objeto es un 'A'
ProtegidoMal|si tiene que usarse sobre otros objetos
ProtegidoMal|'campo' es protegido de 'A': lo ven esa clase y sus descendientes, no desde fuera
ProtegidoMal|un campo no puede ser public
LocalSinValor|'total' se usa sin tener valor
LocalSinValor|'nombre' se usa sin tener valor
SwitchMal|switch sobre 'double': un real no se compara
SwitchMal|valor de 'case' de tipo 'float'
SwitchMal|para comparar referencias, usa if
PropFieldTipo|no se puede asignar 'string' a 'integer'
FinallySale|un 'finally' no puede salir con 'return'
FinallySale|un 'finally' no puede salir con 'break'
FinallySale|un 'finally' no puede salir con 'continue'
FirmaLarga|caracteres (maximo 160)
UnaLineaConFin|no lleva 'endif'
UnaLineaConFin|no lleva 'endwh'
UnaLineaConFin|no lleva 'next'
CompuestoMal|que no se guarda solo en 'integer'
CompuestoMal|'-=' requiere num
CompuestoMal|'&=' requiere integer/long
CompuestoMal|que no se guarda solo en 'byte'
IifMal|es el ternario de BP
IifMal|se esperaba boolean, encontrado 'integer'
IifMal|las dos ramas tienen que dar el mismo tipo
IifMal|se esperaban 3 argumentos, se pasaron 2
IifMal|no mueve un owner
IifMal|es un valor, no una sentencia
GenericosMal|una lista es List<Object>
GenericosMal|la llave de un Map es integer o string, no 'long'
GenericosMal|el valor de un Map es Object, no 'integer'
GenericosMal|'Cosa' no es gen
GenericosMal|una llave integer necesita Collections.numericMap()
GenericosMal|una llave string necesita Collections.Map()
GenericosMal|la llave de este Map es integer, no 'string' (put)
GenericosMal|la llave de este Map es integer, no 'string' (getString)
GenericosMal|se esperaba un array o una List en 'for in'
InterfacesMal|la interfaz 'ConCampo' no tiene campos
InterfacesMal|ciclo de herencia entre interfaces en 'A'
InterfacesMal|'SinArea' implementa 'Forma' y no tiene 'area(): double' (tiene 'area(): integer')
InterfacesMal|implementa 'Forma.area' y tiene que ser public
InterfacesMal|'area' implementa 'Forma.area': m
InterfacesMal|'Otra' no es una interfaz de 'MalMarcada'
InterfacesMal|lleva @override y no redefine nada
InterfacesMal|'@implement' repetida
InterfacesMal|'Forma' es una interfaz: no se extiende
InterfacesMal|'MalMarcada' es una clase, no una interfaz
InterfacesMal|'Hilo' desciende de Thread y un Thread no es un Object
InterfacesMal|'Forma' es una interfaz: no se construye
ArgLargo|no cabe: 130 bytes (el tope del argumento es 127)
EventoValor|'onClick' es un evento, no un valor
EventoValor|compararlo con null
ConstanteNula|la constante 'NADA' no puede ser null
ConstanteNula|la constante 'ETIQUETA' no puede ser null
ConstanteNula|la constante 'LOCAL' no puede ser null
AnnMal|'@XY'; las que existen son '@BD', '@implement' y '@override'
AnnMal|'@BD' sobre la clase 'Dos' tiene que decir a
AnnMal|la entidad 'Dos' no dice
AnnMal|'@BD' sobre una clase no conoce la clave 'esquema'; hoy admite: tabla
AnnMal|'tabla' tiene que ser una cadena
AnnMal|'@BD' sobre 'id' no tiene sentido: su clase 'Cuatro'
AnnMal|'@BD' sobre una property no conoce la clave 'clave'; hoy admite: columna, pk
AnnMal|'columna' no puede estar
AnnMal|(la entidad) o sobre una
ConstCalculada|una const requiere un valor literal
BoolBitsMal|'&' requiere integer/long, o dos boolean
BoolBitsMal|'|' requiere integer/long, o dos boolean
BoolBitsMal|'shl' requiere integer/long
SobrecargaAmbigua|encajan f(Perro, Object) y f(Object, Perro), y ninguna es
SobrecargaAmbigua|encajan h(Perro, Object) y h(Animal, Animal), y ninguna es
SobrecargaAmbigua|pasa el argumento en una variable del tipo de su par
EnumNombreMal|Enum.getName(): se esperaba el valor de un enum, no 'integer'
EnumNombreMal|Enum.getName(): se esperaba 1 argumento, el valor de un enum
EnumNombreNativa|no se traduce en una native; pide el nombre fuera de la native
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

# V8 #764 (2-nov) — el ARGUMENTO DE EJECUCION que no cabe (más de 127 bytes de UTF-8) es un error en la línea de
# órdenes de las DOS VMs: el mismo texto y el código 2, sin ejecutar nada. Antes la VM-C lo cortaba sin decirlo.
VMJ="$RAIZ/miVM/target/bpgenvm-1.0.jar"
VMC="$RAIZ/bpgenvm-c/build/bpgenvm-c"; [ -f "$VMC.exe" ] && VMC="$VMC.exe"
largo="$(printf '%0128d' 0 | tr 0 x)"   # 128 bytes
esperado="el argumento no cabe: 128 bytes (el tope es 127)"
rm -rf "$WORK"; mkdir -p "$WORK"
java -jar "$FE" "$RAIZ/samples/ArgDemo.bp" --compile "$WORK" --backend=mivm >/dev/null 2>&1
for vm in VM-Java VM-C; do
    if [ "$vm" = VM-Java ]; then salida="$(cd "$WORK" && java -jar "$VMJ" ArgDemo.mod "$largo" 2>&1)"; rc=$?
    else                         salida="$(cd "$WORK" && "$VMC" ArgDemo.mod "$largo" 2>&1)"; rc=$?; fi
    if [ "$rc" -eq 2 ] && printf '%s' "$salida" | grep -qF "$esperado"; then
        printf '  OK       %-28s %s\n' "argumento largo, $vm" "$esperado"; ok=$((ok+1))
    else
        printf '  CAMBIO   %-28s codigo %s; falta: %s\n' "argumento largo, $vm" "$rc" "$esperado"
        printf '           lo que dice ahora: %s\n' "$(printf '%s' "$salida" | head -1 | cut -c1-72)"
        mal=$((mal+1))
    fi
done

printf '\n  %d comprobaciones OK  ·  %d mal\n\n' "$ok" "$mal"
[ "$mal" -eq 0 ]
