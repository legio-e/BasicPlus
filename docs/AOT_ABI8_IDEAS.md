# `#381` — que el AOT marshalle 8 bytes: **`long`** (y `double`, aparte)

> **Qué es esto.** El análisis previo de la ficha, antes de tocar código
> (15-ago-2026). Elección de Eduardo: *«me gusta más atacar los long y doubles
> de AOT; es una mejora y podemos aparcar un poco el tema de bugs»*.
>
> **Y una corrección suya nada más empezar, que es la que ordena el trabajo:**
> *«creo que estamos mezclando 2 cosas diferentes: long es una cosa y double
> otra. Podemos empezar por long a ver qué tal»*. La ficha original decía
> «long, double y float JUNTOS» y **la medida de abajo le da la razón**: `long`
> es casi todo código en línea y `double` es emulación de coma flotante entera.
> Comparten el marshalling y **nada más**.
>
> Así que: **`#381` = `long`**. `double` sale a ficha propia (`#426`) y espera.
>
> Lo escrito aquí es lo que se sabe HOY, con lo medido señalado como tal. El
> límite actual está documentado en `AOT_LIMITES.md` §1 y §3.

## El objetivo

Hoy una función `native` sólo maneja valores de 4 bytes: `integer`, `boolean`,
`float`, `string` y refs. Un `long` o un `double` —en parámetro, retorno **o
variable local**— aborta la compilación con un mensaje que ofrece dos salidas:
usar `float`, o quitar el `native` y que esa función corra interpretada.

El rodeo que existe hoy para traerse un valor de 8 bytes desde un pack es una
**caja de salida**: se declara el parámetro como `long[]`/`double[]` y el pack
escribe en el elemento 0 (`AotCEmitter.cTypePack`). Funciona, pero es un rodeo,
y su propio mensaje de error promete que se quitará con esta ficha.

## Lo que ya juega a favor (más de lo que parecía)

1. **La pila BP ya guarda 8 bytes.** El intérprete escribe `long` y `double`
   como 8 bytes big-endian con `bpvm_read_i64_be` / `bpvm_write_i64_be`
   (`bpvm_internal.h:568`), y esas funciones ya existen en el runtime. No hay
   que inventar una representación: ya está y es la misma en las dos VMs.
2. **El thunk ya sabe mover 8 bytes.** Lo hace desde #302 con las
   REFERENCIAS: `H->read_ref(mem + sp - 8); sp -= 8;`. O sea que el patrón de
   "este argumento ocupa el doble" está escrito, probado y en placa.
3. **La tabla de helpers está hecha para esto.** `aot_helpers_v2` dice
   literalmente *«AÑADIR AQUÍ slots futuros, NUNCA EN MEDIO»*, y `float` entró
   así en su día (H3 #166: `read_f32_be`/`write_f32_be`). Añadir cuatro slots al
   final es el camino que el diseño ya previó.
4. **El emisor ya trata un tipo que no es `int32_t`**: `float`. El camino de
   "tipo con helper propio" existe; no se abre de cero.

## El obstáculo de verdad, y está MEDIDO

Un `.mdn` es código **relocatable puro**: se carga sin enlazador y no puede
resolver nada por nombre — por eso los thunks acceden a los helpers a través de
`vm->aot_helpers` en vez de llamar al runtime directamente.

Compilando lo que emitiría el AOT, con los flags reales del pipeline
(`build_mdn.sh`: `-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16
-fpic -fno-jump-tables -Os`), estos son los símbolos que quedan **sin definir**:

| operación | ¿deja símbolo? |
|---|---|
| `long` + `-` | no — GCC lo hace en línea (`adds`/`adcs`) |
| `long` `*` | **no** — en línea (`umull`/`mla`) |
| `long` `/` y `mod` | sí — `__aeabi_ldivmod` |
| `double` + `*` `/` | sí — `__aeabi_dadd`, `__aeabi_dmul`, `__aeabi_ddiv` |
| `double` comparación | sí — `__aeabi_dcmplt` |
| `integer` → `double` | sí — `__aeabi_i2d` |

Y el pipeline del Pico **no enlaza**: compila a `.o` con `gcc -c` y `MdnPack`
empaqueta ese objeto. Sin resolver esos símbolos, el `.mdn` saltaría a la nada
— y el modo de fallo sería el peor conocido de esta casa: **cuelgue mudo en
placa** (es exactamente lo que costó la mañana del 11-ago cuando se movió el
offset de `aot_helpers`).

**La conclusión que ordena el trabajo**: `long` está casi regalado y `double`
exige resolver la emulación de coma flotante. Son dos problemas distintos con
un marshalling común.

## Opciones para las rutinas de libgcc

1. **Enlazar libgcc dentro del `.mdn`** (recomendada). Un paso `ld -r` contra
   `libgcc.a` mete en el objeto SÓLO las rutinas que se usan, y pasan a ser
   locales — el `.mdn` sigue siendo autónomo y relocatable. Hay precedente en la
   casa: el `.text` de RISC-V ya necesita un paso de enlace (V5/H4). Coste: unos
   KB por `.mdn` que use `double`.
2. **Exponerlas por la tabla de helpers.** No sirve: GCC emite esas llamadas por
   su cuenta al traducir el C; no hay dónde interceptarlas sin tocar el codegen.
3. **Recortar lo que no sea inline.** Aceptar `long` salvo `/` y `mod`. Es un
   recorte rarísimo de explicar («puedes multiplicar pero no dividir») y va
   contra el criterio de que los recortes se anuncian y se entienden.

## Plan — `long` primero, y `long` solo

Cada fase deja algo verificable, y ninguna rompe lo anterior.

- **F1 — el marshalling de 8 bytes.** Dos helpers al FINAL de `aot_helpers_v2`
  (`read_i64_be`, `write_i64_be` — las funciones ya existen en el runtime, sólo
  hay que exponerlas), `cType` devolviendo `int64_t` para `long`, y `emitThunk`
  avanzando `sp` de 8 en 8: el mismo patrón que ya usan las refs desde #302.
- **F2 — el cuerpo**: literales de 64 bits, aritmética, comparaciones y las
  conversiones `integer`↔`long` (que en BP ya son opcodes: `OP_I32_TO_I64`).
  Verificable ENTERA en el host, y luego en la Metro.
- **F3 — `/` y `mod`**, lo único de `long` que llama a libgcc
  (`__aeabi_ldivmod`). Dos salidas: enlazar libgcc dentro del `.mdn` (ver
  arriba), o —si F1+F2 ya dan lo que se buscaba— dejarlas fuera **con un mensaje
  que lo diga**, como el resto de recortes del AOT. Se decide con el número
  delante, no antes.

### Lo que NO entra aquí

- **`double` → ficha `#426`.** Comparte F1 (el marshalling) y nada más: necesita
  la emulación de coma flotante entera enlazada dentro del `.mdn`, y arrastra el
  riesgo serio de paridad. Además, el aviso que ya está en `AOT_LIMITES.md` §1
  sigue siendo verdad: **la FPU de estos micros es de precisión simple**
  (FPv5-SP en Cortex-M33 → RP2350 y STM32U5), así que un `double` no toca la FPU
  ni compilado. Marcar `native` una función con `double` es pedir velocidad y
  elegir el camino lento a la vez.
- **El puente `native`→BP** (`isBridgeI32Type`, `AOT_LIMITES.md` §3): hoy sólo
  cruzan valores de 4 bytes. Es independiente y va después.

## Los riesgos, por orden de gravedad

1. ⚠️ **EL INVARIANTE SAGRADO.** Con `long` el riesgo es MENOR que con
   `double` —los enteros de 64 bits no tienen redondeo— pero no es cero:
   el desbordamiento con signo es *comportamiento indefinido* en C, así que
   `-Os` puede tomarse libertades que el intérprete no se toma. Un caso de
   desbordamiento deliberado tiene que entrar en las pruebas desde el primer
   día, y conviene compilar el `.mdn` con `-fwrapv`.
   *(Con `double` el riesgo sería otro y peor —la contracción `a*b+c` en una
   instrucción con redondeo distinto—, y es una de las razones de que vaya en
   su propia ficha. Ya hay además una divergencia conocida: `GAP-4`.)*
2. ⚠️ **El layout de `bpvm_t` NO se toca.** Los cuatro helpers van al final de
   `aot_helpers_v2`; el prefijo congelado de `struct bpvm` se queda quieto. Si
   algo obligara a moverlo, se para y se piensa: eso es lo que cuelga los `.mdn`
   ya generados, en silencio.
3. **Los `.mdn` ya distribuidos** (`SQLite.mdn.ARMV8`/`.RISCV`) no usan los
   slots nuevos, así que crecer por el final no les afecta. Conviene
   comprobarlo de todas formas: reconstruir el `SQLite.pack` y ver que sale
   igual, como se hizo con #365.
4. **Es por arquitectura.** En RISC-V los símbolos serán otros (`__adddf3`,
   `__divdi3`…). F3 hay que hacerla en las dos.

## Cómo se comprueba cada fase

El arnés de siempre: un `.bp` con una `native` que use `long`/`double`,
compilado y ejecutado en **las dos VMs**, con la salida byte-idéntica; después
`compat/compat.sh check`; y en placa, la Metro antes que el P4. Un caso con
números feos (muy grandes, negativos, `0.1 + 0.2`) tiene que entrar desde el
primer día: es donde aparecen las divergencias de coma flotante.
