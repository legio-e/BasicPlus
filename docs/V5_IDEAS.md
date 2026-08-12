# V5 — IDEAS

Charlas de diseño de V5. Lo que aquí se decide se implementa después; lo que se
descarta se queda escrito CON EL MOTIVO, que es la mitad del valor.

---

## El pack nativo se relocaliza AL GRABAR — y lleva un binario por familia

**Decidido 12-ago-2026 (opción B).** Criterio de Eduardo al elegirla:

> *«No podemos ir haciendo packs forzando las cosas. Construimos las
> herramientas, las verificamos que funcionen bien, y entonces el pack sale
> solo.»*

### El problema, medido

Al intentar meter `SQLite.mod` y `SQLite.mdn` dentro del pack que ya lleva el
motor nativo, salieron tres cosas:

1. **`pack.py` da por supuesto un pack de UNA entrada.** Lo tiene escrito:
   `+0` cabecera del pack (128 B), `+128` cabecera de la entrada (48 B),
   `+176` los datos. Con tres entradas la tabla mide 3×48 y los datos empiezan
   en **+272**. El sello (`linked_flash`) seguiría diciendo +176.
   *Confirmado en placa: el P4 encontró el pack en `0x401590b0` = zona + 176.*
2. **`out:pack` no recoge el `.npk`**: filtra el `outDir` a `.mod`/`.mdn`.
3. **Y aunque entrara por `resources/`, iría al final** y en orden alfabético.

No sería un fallo mudo —la escalera compara el sello con dónde encuentra el
pack y diría «realojado para OTRA dirección»— pero no arrancaría.

### La causa de fondo, y el hueco que la explica

Que la posición sea cargante es el SÍNTOMA. La causa es que relocalizamos **a
mano y por adelantado**, con un script.

`bpvm_npack.h:68` dice: *«La tabla de relocalizaciones NO viaja: la consume el
IDE al grabar»*. Eso es el modelo **previsto**, no el implementado:

| lado | estado |
|---|---|
| dispositivo: escalera de validación, sello, `E_SIN_RELOC` | ✅ hecho |
| IDE: relocalizar al grabar | ❌ **no existe** |

Buscando `npack` / `linked_flash` / `R_RISCV` / `reloc_count` en todo el árbol
Java —IDE, `pack/`, frontend— no hay nada. El único relocalizador del repo
entero es `notas/v5-sqlite-prueba/I/pack.py`, un prototipo en la carpeta de
notas. **Ese es el trabajo de B**: portarlo a producto.

No es empezar de cero: los dos relocalizadores (ARM y RISC-V) funcionan y están
verificados contra `ld`. Y `pack.py` lleva un **oráculo** —compara su resultado
con lo que produce el enlazador— que se porta como test. Lo que hoy es un
script sin red pasaría a tener la suya.

### El modelo

1. **El build produce un `.npk` SIN relocalizar POR DESTINO** (ver D1: uno por
   fichero, con doble extension), cada uno con su tabla de relocalizaciones.
   `reloc_count > 0`, sello a cero.
2. **El IDE, al grabar**: mira la placa conectada, elige el FICHERO que le
   toca, calcula la dirección REAL (base de la zona + offset de la entrada
   dentro del pack que acaba de montar), aplica las relocalizaciones, escribe
   el sello y graba.
3. **El dispositivo valida y ejecuta en sitio.** Esa parte ya está hecha.

Con esto la posición dentro del pack **deja de importar**: el IDE conoce el
offset de cada entrada porque él mismo montó el pack.

### La llave es UN DESTINO, no la arquitectura ni el par (arch, float_abi)

Primera versión de este documento: la llave era el par `(arch, float_abi)`.
**Está mal, y lo destapó Eduardo preguntando lo obvio:** *«la cuestión de verdad
es si hay un sólo ARM»*.

No lo hay:

| perfil | núcleos | qué cambia |
|---|---|---|
| ARMv6-M | Cortex-M0, M0+, M1 | Thumb reducido, **sin FPU**, sin DSP |
| ARMv7-M / v7E-M | M3 / M4, M7 | Thumb-2 completo, DSP y FPU opcionales |
| ARMv8-M Baseline | M23 | v6-M + TrustZone |
| **ARMv8-M Mainline** | **M33**, M35P | Thumb-2 + DSP + FPU opcional |
| ARMv8.1-M | M55, M85 | + Helium (vectorial) |

Nuestras dos placas ARM (Metro RP2350 y Nucleo STM32U575) son **las dos
Cortex-M33**, mismo perfil y misma FPU de simple precision. Un solo fichero ARM
vale hoy no porque ARM sea uno, sino porque tenemos dos placas del mismo.

**El agujero:** `(arch, float_abi)` NO distingue ARMv6-M de ARMv8-M — los dos
son `EM_ARM = 40` y los dos pueden ser `softfp`. Un pack compilado para M33 en
una placa Cortex-M0+ **pasaría la escalera** y se estrellaría con una
instrucción indefinida. No diría «otra arquitectura»: reventaría. Exactamente
el modo de fallo que la escalera existe para evitar.

Y el ejemplo que lo hace incómodo: **el RP2350 lleva los dos** — dos Cortex-M33
*y* dos núcleos RISC-V (Hazard3), y se elige cuál arranca. «Una placa, una
familia» ya es falso hoy, en una placa que tenemos.

**Decisión: la llave es un `BP_TARGET_*`**, uno por destino, en la misma tabla
única donde viven los `MDN_ARCH_*`. La granularidad correcta ya existe en
`pack.py` (`target="arm-cortex-m33"`, `target="riscv32-esp-p4"`), sólo que hoy
es descriptiva y no es la llave. El `float_abi` deja de ser campo aparte: va
dentro de la definición del destino. Una llave en vez de un par — más simple y
más estricta.

**Y el micro declara qué destinos ACEPTA, en orden de preferencia.** Esto
devuelve lo que se pierde al hacer la llave estricta: una imagen ARMv6-M sí
corre en un M33. Si el firmware declara `[cortex-m33, cortex-m0]`, el cargador
coge el primer fichero que encaje — el optimizado si esta, el generico si no.
Quien sabe lo que puede ejecutar es el micro, no una tabla del PC. Encaja con
#378 («que cada micro DIGA lo que tiene»).

Hoy son DOS ficheros; el S3 hara tres cuando tenga toolchain, y un M0+ haria
cuatro. **El diseno no debe hornear el dos.**

Y el aviso que ya esta escrito en `pack.py`, que aqui vale doble: *«un desajuste
aqui no da error, da NUMEROS MAL»*. Por eso el VALIDADOR vive en el formato y lo
comprueba el dispositivo. El nombre de fichero (D1) es solo el SELECTOR con que
el IDE elige antes de grabar: si mintiera, la escalera lo caza con `E_ARCH`.

### Decisiones

**D1 — EL FORMATO NO SE TOCA. Un fichero por destino, con DOBLE EXTENSIÓN.**

```
sqlite.npk.ARMV8     sqlite.npk.RISCV
SQLite.mdn.ARMV8     SQLite.mdn.RISCV
```

Decisión de Eduardo (12-ago), y sustituye a lo primero que escribí aquí:

> *«Como eso no llega a los micros, no merece la pena hacer un cambio de
> formato. Lo que utilizamos es una doble extensión.»*

❌ **Descartado: una entrada `.npk` con N rebanadas dentro** (cabecera + un
directorio de N destinos, cada uno con su imagen y su tabla). Motivos:

1. **El micro pagaría por algo que sólo sirve en el PC.** El fichero gordo es
   un envase de DISTRIBUCIÓN: a la placa llega una sola familia. Meterle el
   directorio de N al formato de flash es hacerle cargar con una estructura
   cuyo único trabajo es descartar hermanas que nunca va a ver.
2. **El argumento que lo decide es de ESCALA, y es de Eduardo:** *«hoy tenemos
   2 familias, pero mañana pueden ser 4, 6 o n. Eso en el PC no es problema
   pero en un micro no.»* Con la doble extensión, n familias son n ficheros en
   el PC —gratis— y **siempre uno** en la placa. Con rebanadas, cada familia
   nueva ensancha el formato que valida el firmware.
3. **Coste real:** cero cambios en C, cero riesgo para lo que ya corre en
   placa. La alternativa tocaba `bpvm_npack.h`, la escalera y su test.

📌 **Y mi objeción a la doble extensión era floja: el nombre NO es una segunda
fuente de verdad.** Es un **selector** para que el IDE elija antes de grabar; el
**validador** sigue siendo la cabecera, que lleva `arch` y `float_abi` y que
comprueba el dispositivo. Si el nombre miente, la escalera lo caza con
`E_ARCH`. Dos papeles distintos, no dos verdades.

⚠️ **El sufijo sale de la TABLA DE DESTINOS, no se teclea.** Si `ARMV8` se
escribe a mano en el build, otra vez en el IDE al elegir y otra en los docs, son
tres sitios para el mismo dato — que es el error que sí cuesta caro (#299,
#315). `NpackReloc.Destino` ya existe: que lleve su sufijo y que todos tiren de
ahí.

⚠️ **La doble extensión es SÓLO del PC.** Dentro de un `.pack` de BP el tipo de
entrada es un fourcc de **4 caracteres, minúsculas `[a-z0-9]`**
(`PackFormat.TYPE_LEN = 4` + `isLowerFourcc`) y se deriva de la ÚLTIMA
extensión: `sqlite.npk.RISCV` daría tipo `riscv`, cinco caracteres y en
mayúsculas — **lo rechaza al construir**. Lo que entra en el pack es el
ELEGIDO, ya renombrado a `sqlite.npk`.

**D2 — Un mecanismo, dos problemas.** La misma convención resuelve el `.mdn`,
que también es por arquitectura y hoy viaja suelto (era la idea de Eduardo del
11-ago para `miModulo.mdn.arm`). No hay que inventar nada aparte.

**D3 — Y el sufijo lleva el PERFIL, no sólo la familia.** `ARMV6` y `ARMV8` son
destinos distintos porque un Cortex-M0+ no ejecuta código de un M33 (ver abajo).
La convención lo soporta sin cambios; una llave `(arch, float_abi)` no.

**D4 — El `.npk` es un tipo de PRIMERA, no un recurso.**
Añadirlo a `OUTDIR_TYPES` de `PackStep`, junto a `.mod` y `.mdn`. Meterlo por
`resources/` "funcionaría", pero un motor no es un recurso: esa carpeta acepta
cualquier cosa sin validar y por contrato va detrás de los módulos. Como tipo
de primera entra además en las comprobaciones de «este pack se puede ejecutar»
que `PackStep` ya hace al construir (#361).

**D5 — Y va el PRIMERO.** No hace falta para que funcione (ver el modelo), pero
sí para que el reparto sea predecible: de primero su offset es `128 + N×48` y
sólo se mueve si cambia el NÚMERO de entradas; en medio se mueve si cambia el
TAMAÑO de cualquier cosa anterior. Diagnósticos más fáciles.

**D6 — UN PACK PARA CUALQUIER PLACA: lo distribuido es universal, lo grabado
es de una placa. Y quien poda es el IDE, al grabar.**

Idea de Eduardo (12-ago). Un solo `.pack` que se instala en cualquier sitio:

```
sqlite.pack  (lo que se reparte)      →  grabado en el P4
  npk  sqlite.RISCV                        npk  sqlite    ← relocalizado
  npk  sqlite.ARMV8                        mod  SQLite
  mod  SQLite          (portable)          mdn  SQLite    ← el de RISC-V
  mdn  SQLite.RISCV                     (los otros NO viajan)
  mdn  SQLite.ARMV8
```

⚠️ **Y `.npk` y `.mdn` son DISPARADORES INDEPENDIENTES** — matiz de Eduardo, que
corrige una simplificación mía («si tiene código nativo, el IDE lo transforma»).
No es uno, son dos, y un pack puede necesitar cualquiera de ellos, los dos, o
ninguno:

| lleva | qué obliga |
|---|---|
| `.npk` | relocalizar al grabar + quedarse con el de la familia |
| `.mdn` | **rehacer el pack quitando los `.mdn` de las otras familias** |
| ninguno | nada: viaja tal cual, universal LITERAL |

Un pack puede tener `.mdn` **sin** `.npk`: una librería con funciones `native`
que se apoyan en el puente AOT de la VM, sin motor externo que realojar. Ahí no
hay nada que relocalizar pero sí que podar.

**Por qué el IDE y no el micro.** El micro también podría buscar
`<Modulo>.<DESTINO>` y caer a `<Modulo>` — cabe en el nombre de entrada
(`NAME_LEN = 32`) y es poco código. Pero entonces cada placa cargaría en su
flash con los `.mdn` de las demás, y la tabla de destinos entraría en el
firmware. Criterio: **en la placa, lo que no está no puede fallar.**

Y el coste es el bueno: *«es una operación que se hace de vez en cuando, aquí el
coste de transformar es un poco de tiempo extra pero que no afecta a nada»*
(Eduardo). Además el IDE YA tiene que rehacer el pack por el `.npk`, así que la
poda del `.mdn` se apunta a un viaje que ya se hacía.

📌 **Lo que esto NO es**: una capacidad nueva. El `.mdn` dentro de un pack ya
funciona y está verificado en la Metro (11-ago): `AOT: SQLite.mdn cargado del
pack (3292 B, 16 thunks)`. El dispositivo lo busca por tipo y nombre
(`bpvm_pack_find(zona, len, "mdn", mod, ...)`), y con la poda sigue encontrando
exactamente lo de siempre. **Cero cambios en C.**

**D7 — El contrato del formato, en UN sitio y clavado por un test.**
Como `mdn_format.h`, y como ya hace `pack.py` hoy: aserto de tamano en el lado
Python + `offsetof` en `test_npack.c`. ⚠️ Con D1 el formato de flash NO cambia,
asi que esta pinza no hay que rehacerla — hay que NO ROMPERLA al portar a Java — Java y C tienen que romper la compilación, no la placa, cuando alguien
mueva un campo. Ver [[mdn-depende-del-layout-de-bpvm]] para lo que cuesta
cuando el contrato no está clavado.

### Orden de construcción

Herramientas primero; el pack es la consecuencia, no el objetivo.

1. **El relocalizador en Java**, con el oráculo contra `ld` como test. Sin
   tocar nada más: entra un ELF y una dirección, sale una imagen. Verificable
   en el PC, sin placa.
2. **El `.npk` sin relocalizar** como salida del build, UNO POR DESTINO con
   su tabla (D1: doble extension, cero cambios de formato). El test: que el dispositivo lo RECHACE con `E_SIN_RELOC` — que
   es lo que debe hacer con un pack sin grabar.
3. **`PackStep`**: `npk` como tipo de primera, colocado el primero.
4. **El grabado del IDE**: elegir FICHERO preguntandole a la placa que
   destinos acepta (y en que orden), relocalizar, sellar. Al `.pack` entra el
   elegido, ya renombrado a `sqlite.npk` (ver D1: el fourcc no admite el sufijo).
5. Y entonces el pack de SQLite sale solo — en ARM y en RISC-V, del mismo
   proyecto.

**D8 — UN BUILD, UN `.mod`, N `.mdn`. Y el C se emite UNA VEZ.**

El criterio es de Eduardo, y es más estrecho de lo que yo iba a hacer:

> «Un build, un `.bp`, un `.mod`, un código C, múltiples `.mdn`. Lo único que
> se ha de compilar varias veces es el código C intermedio.»

O sea que el proyecto declara sus familias —

```json
"aot": { "enabled": true, "targets": ["arm", "riscv"] }
```

— y de ahí sale:

```
    aot_SQLite.c          <- UNO, compartido por todas
      arm   -> aot_SQLite.arm.o                 -> SQLite.mdn.ARMV8
      riscv -> aot_SQLite.riscv.o -> .elf       -> SQLite.mdn.RISCV
```

**Por qué importa que el `.c` sea uno.** No es ahorrar tiempo: emitirlo por
familia serían dos pasadas del emisor, y dos pasadas son dos ocasiones de
divergir. Con una sola emisión es *imposible por construcción* que dos `.mdn`
del mismo pack no se correspondan entre sí ni con el bytecode que llevan al
lado. En la corrida real del SQLite los dos declaran **16 thunks**, que es esa
propiedad hecha número.

Consecuencias de diseño, todas pequeñas y todas con motivo:

- **El sufijo sólo aparece al construir un pack.** En un Run al dispositivo el
  `.mdn` sale pelado, porque el firmware busca `<Modulo>.mdn` en su FS. Son dos
  productos distintos con dos consumidores distintos, no una heurística.
- **`target` y `targets` a la vez es un error**, no una preferencia a adivinar:
  cualquier regla de precedencia que pusiéramos sería una que nadie recuerda.
- **Los nombres se validan al LEER el `.bpbuild`.** Escribir `risc-v` se ve en
  el fichero, que es donde se arregla — no tres pasos después, en forma de una
  familia que falta en el pack.
- **Los intermedios llevan la familia en el nombre siempre** (`aot_X.arm.o`).
  Con una da igual; con dos, el segundo gcc pisaría el `.o` del primero y el
  `.mdn` saldría con el código de la otra ISA. Eso no daría error en el PC:
  daría un cuelgue en la placa.
- **El constructor del pack no se entera de nada.** `PackStep` ya sabía leer la
  doble extensión (D1), así que no se tocó. Era la señal de que el reparto de
  responsabilidades estaba bien puesto.

El hueco donde encaja: `Main.PasoAntesDelPack`, entre compilar y empaquetar.
El AOT no puede vivir en el frontend —necesita las rutas de los toolchain, que
son de cada máquina y las guarda el IDE—, así que el frontend deja el hueco y
quien sabe compilar lo rellena. La alternativa era partir el build en dos y
duplicar el paso de empaquetado en los dos llamantes.

**Lo que el AOT NO hace: bloquear el pack.** Un pack sin `.mdn` corre
interpretado — más lento, no roto —, y ése es el criterio desde H12. Lo que no
puede es fallar callando, y por eso los avisos van a la consola.
