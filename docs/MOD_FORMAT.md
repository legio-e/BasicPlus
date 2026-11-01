# Formato binario `.mod` (v9)

Este documento es la **especificación canónica** del fichero `.mod` que
emite el frontend (`lexer-java`) y consumen las dos VMs (`miVM` en Java y
`bpgenvm-c` en C). Si una implementación discrepa del documento, **el
documento gana**.

Versión vigente: **9** (`"MOD9"`, el enlace tardío, §13: el compilador lo emite sin
flag desde el 17-sep-2026, V7/H2 `#523` V11; `--formato=8` sigue existiendo como
salida de emergencia hasta cerrar V7). Lo que sigue describe **v8** (`0x4D4F4438` =
`"MOD8"`, 15-sep-2026), que es la base de v9: el módulo lleva **contra qué se compiló** — su generación
`gen`, el CRC32 de su interfaz, y el sello de cada dependencia — y las VMs aplican
**la norma de versiones** de `#500` al cargar, §3.1). Las VMs de V7 cargan también
**v7** y **v6** (sin datos de versión: cuentan como generación 0 y CRC desconocido)
y **rechazan** v5 y anteriores con un mensaje (`#284`). El compilador emite **sólo
v8**. Las VMs de V6 rechazan un v8 con «MAGIC no reconocido»: hacia delante no hay
compatibilidad, hacia atrás sí (decisión de Eduardo, 14-sep).

> **v9 — el enlace tardío (`"MOD9"`, V7/H2 `#523`, 16-sep-2026).** Misma cabecera de
> 44 bytes; cambia lo que el módulo dice de las clases de OTROS módulos: en vez de
> números horneados (ranuras, offsets, descriptores) lleva **referencias simbólicas**
> (subtabla `refs` 3.3) que el cargador resuelve al enlazar, y publica su propia
> tabla de clases (subtabla `CLASES` 4.5) para que los demás resuelvan contra ella.
> Desde el 17-sep (V11) es lo que el compilador emite sin flag, y la stdlib, los
> blobs de las cinco imágenes, `Stdlib.pack` y el IDE van en v9. Todo en **§13**.

Convención general:
- Todos los enteros son **big-endian** salvo indicación expresa.
- Todos los strings se serializan con la convención `writeUTF` de Java
  `DataOutputStream`: `u16 length` seguido de `length` bytes UTF-8
  (modificado — sin BOM, sin terminador NUL). Cuando el documento dice
  "UTF-8 raw" se refiere a sólo los bytes, sin el length prefix.
- Offsets dentro de las secciones (`relativeOffset`, `csOffset`, etc.)
  son `i32` con signo. Los offsets CS-relative al data block son
  típicamente **negativos** porque el data block precede al código en
  memoria runtime.

---

## 1. Layout del fichero

> ⚠️ **Este documento estuvo DOS versiones desatrasado** — describía v5 cuando el
> compilador llevaba tiempo emitiendo v6 — y se puso al día el 23-ago-2026 al
> añadir v7. Queda escrito porque volverá a pasar y porque el fallo es caro: **el
> header CRECE con la versión**, así que un lector que se equivoque de tamaño
> desplaza *todas* las secciones y corrompe el módulo en silencio. Leer el header
> según el MAGIC no es una precaución: es el contrato.

```
+─────────────────────── header (44 bytes en v8) ───────────────────────────+
│  +00  MAGIC          i32  0x4D4F4438 ("MOD8")                            │
│  +04  dataSize       i32  bytes de la sección data                       │
│  +08  mainOffset     i32  offset del entrypoint dentro de code (-1 = no) │
│  +12  importsSize    i32  bytes de la sección imports                    │
│  +16  exportsSize    i32  bytes de la sección exports                    │
│  +20  codeSize       i32  bytes de la sección code                       │
│  +24  librarySize    i32  bytes de la sección library (0 si no hay)      │
│  +28  interfaceSize  i32  bytes de la sección interface   ← v6 y v7      │
│  +32  nativeSize     i32  bytes de la sección native      ← v7 y v8      │
│  +36  gen            i32  generación del módulo (`version` del proyecto; 0 si suelto) ← v8
│  +40  ifaceCrc       i32  CRC32 (IEEE) de los bytes de la sección interface        ← v8
+──────────────────────────────────────────────────────────────────────────+
+── library   ──+  librarySize   bytes, UTF-8 raw (sin length prefix)
+── imports   ──+  importsSize   bytes (ver §3)
+── exports   ──+  exportsSize   bytes (ver §4)
+── interface ──+  interfaceSize bytes (ver §4.5)   ← v6+
+── native    ──+  nativeSize    bytes (ver §4.6)   ← v7
+── data      ──+  dataSize      bytes (ver §5)
+── code      ──+  codeSize      bytes (ver §6)
```

**Header por versión:** v5 = 28 bytes (7 enteros) · v6 = 32 (8) · v7 = 36 (9).

📌 **Las secciones nuevas se insertan ANTES de `data`, nunca al final.** No es
estética: los offsets internos de `data` y `code` son relativos a su propia
sección, así que meter algo delante no obliga a recalcular nada — y meterlo
detrás convertiría el fichero en algo cuyo final hay que adivinar. Un `.mod` que
vive en un pack por XIP **no es un fichero**: es una región mapeada, y ahí "lo
que sobra al final" no es una señal que exista.

El fichero termina exactamente al final de la sección `code`. Sin
trailer, sin checksum, sin padding.


## 2. Library

String UTF-8 sin length prefix; `librarySize` lo determina. Si está
vacío (`librarySize == 0`), el módulo no pertenece a ninguna library.

Cuando hay library `L` y módulo `M`:
- Nombre de fichero por convención: `L.M.mod`.
- Símbolos en el `globalSymbolTable` se cualifican como `L.M.<symbol>`.

---

## 3. Sección IMPORTS

```
imports ::= count:i32  import_entry*  [ deps ]  [ refs ]   (deps: v8+; refs: sólo v9, §13.2)

import_entry ::= name:UTF  fromPath:UTF

deps       ::= count:i32  dep_entry*
dep_entry  ::= module:UTF  fromPath:UTF  gen:i32  ifaceCrc:i32  flags:i32
```

**La subtabla `deps` (v8, V7/H2)** va DETRÁS de las entradas de import, dentro
de `importsSize`, y se detecta por los bytes remanentes (aditivo). Es el
**sello** de cada dependencia contra la que el compilador compiló este módulo:
una entrada por **módulo** (nombre lógico cualificado, `Core` o `acme.LibHdr`),
no por import — un import repite el sello de su dueño, y una dependencia toda
intrínseca (`Math`) no deja ningún import. `gen` e `ifaceCrc` son los de la
cabecera del `.mod` de la dependencia tal como el compilador la leyó (para una
dependencia v6/v7, `gen` = 0 y `ifaceCrc` = CRC32 de su sección interface).
`flags` bit 0 = **EXTIENDE**: este módulo tiene una clase cuya base vive en esa
dependencia. Un lector que no conozca la subtabla la salta por `importsSize`.
Una subtabla vacía significa «no exige nada» (módulos escritos a mano con el
`ModWriter`, tests).

### 3.1 La norma de versiones (`#500`), igual en las dos VMs

Al resolver cada dependencia `D` de un módulo `M` (v8) — también si `D` ya estaba
cargada por otro módulo — con el sello `(gen_req, crc_req, flags)` que `M` lleva
para `D`:

1. `D.gen < gen_req` → **rechazo**: `'D' (gen X) es mas antiguo que el que 'M'
   necesita (gen Y): actualiza /lib o recompila`.
2. `D.gen == gen_req` y `D.ifaceCrc != crc_req` → **rechazo**: `'D' (gen X, interfaz
   AAAAAAAA) no es la interfaz con la que se compilo 'M' (interfaz BBBBBBBB): recompila`.
3. `D.gen > gen_req` y `M` **extiende** una clase de `D` → **rechazo**: `'D' (gen X)
   es mas reciente que 'M' (compilado contra gen Y) y 'M' extiende una clase suya:
   recompila 'M'` — norma de Eduardo (15-sep): *«los hijos pueden ser más recientes
   que los padres; los padres no pueden ser más recientes que los hijos»*. Un hijo
   coloca sus campos y métodos detrás de los del padre: si el padre cambia de
   forma (aunque sólo añada al final), el hijo pisa sus ranuras.
4. `D.gen > gen_req` y `M` sólo la **usa** → se acepta **si enlaza**: los
   marcadores `Cls#m#slot` (§4.2) deciden; añadir al final no rompe a un cliente.
5. `D` es v6/v7 (sin datos): `gen` = 0 → si `gen_req > 0`, regla 1; si `gen_req` = 0,
   se anota «sin datos de version» por el diagnóstico (stderr en host, log en placa)
   y no se comprueba la interfaz.

El texto del rechazo sale por **OUTPUT** (el `stdout` del programa: mismo texto
byte a byte en miVM y bpgenvm-c, ASCII, sin rutas) y la carga falla con **exit 2**
(«no se pudo cargar»; `BPVM_ERR_DEP_VERSION` en C, `IOException` en Java, y
`load: <texto>` en el `errorMessage` del wire). Un símbolo que no enlaza sale como
`=== ERROR DE LINK: <texto> ===` con el texto de `link.c` en las dos VMs, exit 1.
Lo prueba `compat/compat.sh check-norma` con `Core` de verdad (9 casos).

- `name` es el qualifiedName del símbolo importado:
  - `"<Module>.<func>"` si el dueño no declara library.
  - `"<library>.<Module>.<func>"` si declara library.
- `fromPath`: el literal del `import X from "..."` del fuente (`""` si no lo
  hay). **Informativo desde V7/H2 (`#493`)**: es la pista del COMPILADOR para
  localizar la interfaz al compilar; ninguna VM lo usa para resolver el `.mod`
  en ejecución — el dueño se busca por su nombre canónico
  (`<library>.<Module>.mod` o `<Module>.mod`) en el orden único: pack en
  ejecución → `/app` → `/lib` → zona de packs. Hasta el 15-sep miVM lo honraba
  (relativo al cwd de la JVM) y la VM-C lo tiraba: mismo `.mod`, distinto
  resultado.

Una vez resuelto el dueño en el linker, el loader escribe la dirección
absoluta del símbolo en la **tabla de ext** (ver §7), entrada `i` → byte
`moduleBase + i*4`. El opcode `CALL_EXT` (ver `OPCODES.md`) lee de esa
tabla por índice.

---

## 4. Sección EXPORTS

Estructura con sub-secciones opcionales detectables por bytes
remanentes del `exportsSize` declarado en la cabecera. Esto permite
ampliar el formato manteniendo compatibilidad con loaders antiguos
(verlos sólo como `exportsSize` mayor del esperado).

### 4.1 Sub-sección de funciones exportadas (obligatoria)

```
funcs ::= count:i32  func_entry*

func_entry ::= name:UTF  relativeOffset:i32
```

`relativeOffset` es relativo al inicio del bloque `code` del módulo. El
loader lo absolutiza con `codeStart + relativeOffset` y lo registra en
`globalSymbolTable` con la clave `<library>.<module>.<name>` (o
`<module>.<name>` si no hay library).

**Varios nombres pueden apuntar al mismo offset** (V7/H2, 14-sep-2026): un
*alias* es una entrada más de esta sub-sección, con el offset de una función ya
emitida — cero bytecode, y un cargador anterior lo ve como una función exportada
normal. `relativeOffset` puede repetirse; **`name` NO**: miVM se quedaría con la
última entrada y la VM-C con la primera (paridad rota). El escritor lo rechaza
(`ModWriter.addFunction`/`addExportAlias`) y el compilador reserva al usuario los
nombres `__cls_*`/`__prop_*`.

**Los símbolos planos `__cls_*`.** El nombre de un import es `<Module>.<sym>`
y los dos cargadores derivan el módulo por el **penúltimo punto** (§3), así que
`<sym>` **no puede llevar puntos**. Como la función local de un miembro de clase
se llama `Clase.miembro`, lo que cruza el módulo es siempre un nombre plano que
el dueño exporta (`$` y `#` no son separadores):

| símbolo | qué es | desde |
|---|---|---|
| `__cls_new_<Cls>[$sufijo]` | factoría: `NEW_OBJECT` + `__init` del ctor **efectivo** (propio o heredado, `findConstructor`); una por sobrecarga, la cabeza sin sufijo | L2 · V7/H2 (`$sufijo`) |
| `__cls_init_<Cls>[$sufijo]` | `super(...)` cross-module: `this` + args → el `__init` de la clase que **declara** el ctor (local o `CALL_EXT`); una por ctor **efectivo** | L2 v3 · V7/H2 |
| `__cls_m_<Cls>#<firma>` | `super.metodo()` cross-module: salto NO virtual a la implementación de la base; uno por sobrecarga | `#451` · V7/H2 (`#`, firma) |
| `__cls_s_<Cls>#<firma>` | **alias** del método estático `Cls.m[$…]` | V7/H2 |
| `__cls_sg_<Cls>#<prop>` / `__cls_ss_<Cls>#<prop>` | **alias** de los accesores de la property estática `Cls.__prop_get_x` / `Cls.__prop_set_x` | V7/H2 |
| `<Cls>#<metodo>#<slot>` | marcador de vtable (§4.2, data): el linker lo resuelve por nombre; si la ranura cambió, no resuelve | `#324` |

Dos reglas de forma (14-sep-2026, medidas por la revisión): el separador
clase/miembro es **`#`** — `_` es un carácter legal de identificador y `A_b.c` y
`A.b_c` daban el mismo nombre —; y el miembro va **siempre por firma**
(`nombre$códigos`, `overloadMangle`: `cuantos$`, `cuantos$i`, `f$LBase;`), nunca
por su posición en la cadena de sobrecargas — si la interfaz omite la cabeza
(private, tipo no exportable) el consumidor renumeraba y llamaba a otra función.
El `$sufijo` de los constructores es el de H5.a: la cabeza sin sufijo (un `.mod`
con un solo ctor no cambia de símbolo al añadir otro), las demás `$códigos`.

### 4.2 Sub-sección de data exports (opcional — B3 v2 / L2 v3)

Presente sólo si quedan bytes después de leer la sub-sección 4.1.

```
data_exports ::= count:i32  data_entry*

data_entry ::= name:UTF  csOffset:i32
```

`csOffset` es CS-relative al `codeStart` del módulo (es decir, suele ser
negativo porque el data block está ANTES del code en memoria runtime).
La dirección absoluta es `codeStart + csOffset`.

Uso típico:
- `RuntimeError` (clase sintética para `try/catch e: RuntimeError`).
- Cualquier `public class X`: se exporta el descriptor de la clase
  como data symbol para que importadores con `extends` resuelvan
  `parentOff` en `linkAll` (ver §4.3).

### 4.3 Sub-sección de class fixups (opcional — L2 v3)

Presente sólo si quedan bytes después de leer 4.2. Si 4.3 está pero 4.2
no, **4.2 debe escribirse con count=0** (es un marcador necesario para
que el loader sepa dónde empieza 4.3).

```
class_fixups ::= count:i32  fixup_entry*

fixup_entry ::= childClassName:UTF  childCsOffset:i32  parentQualified:UTF
```

- `childClassName`: nombre simple de la clase del child (que vive en
  ESTE módulo).
- `childCsOffset`: offset del descriptor del child relativo al
  `codeStart` de este módulo. Negativo (data block precede).
- `parentQualified`: nombre cualificado del parent en `globalSymbolTable`,
  e.g. `"L2Lib.Counter"`.

Aplicación en `linkAll` (tras resolver imports):

```
for each fixup:
    parentAbs = globalSymbolTable[parentQualified]
    childAbs  = moduleCodeStart + childCsOffset
    parentOff = parentAbs - moduleCodeStart        // CS-relative al child
    writeI32(childAbs + 8, parentOff)              // CLS_OFF_PARENT_OFF
```

Esto deja la cadena de herencia navegable cross-module por
`isDescendantOf` e `INVOKE_VIRTUAL` (que usan
`moduleManager.getCSForDataAddr` para saber qué CS sumar).

### 4.4 Sub-sección de eh-class fixups (opcional — BUG-2)

Presente sólo si quedan bytes después de leer 4.3. Si 4.4 está pero 4.3
no, **4.3 debe escribirse con count=0** (marcador, igual que 4.2→4.3).
Parchea el operando `clsOff` (i32) de cada `TRY_BEGIN_EXT` (0xAB) usado para
un `catch` de una clase de excepción definida en OTRO módulo.

```
eh_class_fixups ::= count:i32  eh_entry*

eh_entry ::= codeOffset:i32  parentQualified:UTF
```

- `codeOffset`: offset del operando `clsOff` (i32) a parchear, relativo al
  `codeStart` de este módulo (positivo; dentro del code block).
- `parentQualified`: nombre cualificado de la clase de excepción en
  `globalSymbolTable`, e.g. `"HoleLibA.MyError"`.

Aplicación en `linkAll` (tras resolver imports y class fixups):

```
for each eh_fixup:
    parentAbs = globalSymbolTable[parentQualified]
    clsOff    = parentAbs - moduleCodeStart        // CS-relative
    writeI32(moduleCodeStart + codeOffset, clsOff) // parchea el operando
```

En runtime `TRY_BEGIN_EXT` computa `expectedClass = cs + clsOff = parentAbs`,
de modo que `isDescendantOf` casa el `catch` con la clase del otro módulo.

---

## 4.5 Sección INTERFACE (v6+)

El texto de la interfaz del módulo — lo que hasta V4 era un fichero `.bpi`
aparte. UTF-8 crudo, sin length prefix; `interfaceSize` lo delimita. Vacía
(`interfaceSize == 0`) es legítimo: los módulos emitidos a mano por los tests no
la llevan.

**Por qué se fundió** (V4/H6.a): dos ficheros que tienen que ir juntos son dos
ficheros que se pueden desparejar. Al meterla dentro desaparecieron **71 `.bpi`**
y con ellos una clase entera de fallo. Es el mismo argumento que justifica §4.6.

Las VMs la **saltan**: ejecutan sin ella. La usa el compilador, para resolver
imports sin recompilar el módulo dueño.

**Las directivas (`bpi 8`, V7/H2).** Texto, una por línea; la escribe y la lee
`ModuleInterface.java` (`writeTo`/`fromBytes`), y la ida y vuelta es byte a byte
(`InterfazIdaYVueltaTest`). A nivel de módulo: `bpi <ver>`, `module`, `library`,
`interface`, `implements`, `extends`, `func <n>(<params>):<ret> [public] [static] [intrinsic] [inline <bytecode>]`
(H1, 20-sep: `inline` va el último y se lleva el resto de la línea, `CALL_BUILTIN 24; …` — el cuerpo de una
intrínseca, que el consumidor inlina en cada llamada),
`const`, `enum`, `prop`. Sólo las clases **propias** del módulo (los stubs de Core
que el compilador aliasa no se re-publican). Dentro de `class <n> public [extends <Base>] … end class`:

| directiva | qué publica |
|---|---|
| `layout <nF> <nM> <fbm> <obm>` | el layout binario del descriptor (campos y ranuras de vtable, con los bitmaps de refs y owners) — lo que un hijo de otro módulo necesita para colocar lo suyo detrás |
| `field <n>:<tipo> slot <s> [owner]` | un campo **no privado** con su ranura absoluta (V7/H2): el hijo emite `GET_FIELD`/`SET_FIELD` por ranura |
| `ctor(<params>)` · `ctor+(<params>)` | el constructor **efectivo** (propio o heredado: el mismo que usa la factoría) — la cabeza y, desde V7/H2, cada sobrecarga más, en orden de cadena |
| `prop <n>:<tipo> [public] [sync] [final] [private]` | una property de instancia (siempre se publica: sus dos accesores ocupan ranura; publicar NO es hacer público, `#316`) |
| `staticconst <n>:<tipo>=<literal> public` | constante estática: el consumidor la inlinea |
| `staticprop <n>:<tipo> [public] [final]` | property estática (V7/H2): sin ranura; el consumidor llama por `CALL_EXT` a los alias `__cls_sg_/__cls_ss_` |
| `event <n>(<params>) recv <slot> [opaque]` | evento propio con la ranura de su campo receptor |
| `method <n>(<params>):<ret> [public] [final]` | método de instancia (todo menos lo `private`: lo protegido ocupa ranura y la hija lo ve, `#390`) |
| `staticmethod <n>(<params>):<ret> [public] [final]` | método estático (V7/H2): sin ranura; `CALL_EXT` al alias `__cls_s_` |

**Las interfaces de clase** (`bpi 10`, V8 H7.3): `iface <n> public [extends A,B]` … `end iface`, con sus `method` y
`prop`; y en una clase, `implements A,B` (de otro módulo, cualificadas: `Lib.Forma`). Las escribe sólo un módulo que las
usa: los demás siguen con su `bpi 9` y su `ifaceCrc` de siempre.

`staticmethod`, `staticprop` y `ctor+` son directivas **nuevas** y no flags de
`method`/`prop`/`ctor` a propósito: un lector anterior (`tolerant`, versión mayor
que la suya) las **salta**. Un flag que el lector anterior ya conociera (`static`
en `method`: `parseFunc` lo acepta desde V4) lo habría contado como método de
instancia y corrido las ranuras de todo lo declarado detrás — en silencio. Los
flags `final`/`private` de `method`/`prop`, en cambio, el compilador de V6 no los
conoce y **aborta** con «flag desconocido»: ruidoso, y asumido — un compilador
anterior no lee módulos de un formato posterior; lo que se garantiza es que las
**VMs** ejecutan los módulos anteriores (MOD7).

## 4.6 Sección NATIVE (v7)

El código nativo AOT del módulo — lo que hasta V5 era un fichero `.mdn` aparte.
Vacía (`nativeSize == 0`) mientras el módulo no lleve nativo, que es el caso de
casi todos.

### Su contenido: N blobs `.mdn` concatenados, uno por familia

```
+── blob 0 ──+  cabecera mdn_header_t + N symbols + code   (alineado a 4)
+── blob 1 ──+  idem, otra arquitectura
+──   …    ──+
```

### ⚠️ La sección se ALINEA A SÍ MISMA

Las secciones del `.mod` **no están alineadas**: se empaquetan seguidas, y hoy
`data` y `code` empiezan en offsets impares. Da igual para ellas, porque el
cargador las **copia** a `memory[]` en direcciones alineadas — la alineación del
fichero no se hereda.

**Con `native` no da igual.** Un `.mod` dentro de un pack se ejecuta por **XIP,
en su sitio**, así que la posición del blob en el fichero **es su dirección de
ejecución**. Un blob RISC-V en offset impar no arranca, y el propio `.mdn` ya
pide sus datos alineados a 4.

Por eso la sección empieza con **0–3 bytes de relleno** hasta el múltiplo de 4, y
`nativeSize` **los incluye**. Ninguna otra sección se toca; el lector hace:

```
inicio_del_primer_blob = align4(inicio_de_la_seccion)
```

📌 Lo señaló Eduardo el 23-ago al revisar el formato, **antes** de que hubiera un
solo blob emitido. Sin eso, el fallo sólo se habría visto en placa, con XIP, y en
una arquitectura sí y en otra no.

**No hay tabla de contenidos, y no hace falta**: un `.mdn` ya se describe a sí
mismo. Su cabecera (`src/mdn_format.h`) trae `code_size` y `sym_count`, así que
el tamaño total de un blob se calcula desde él:

```
total = mdn_header_size(version) + sym_count * sizeof(mdn_symbol_t) + code_size
```

redondeado a 4. Con eso un lector camina de blob en blob hasta agotar
`nativeSize`. Y cada blob dice **para qué arquitectura es**, en su campo `arch`
(`MDN_ARCH_ARM`, `_RISCV`, `_XTENSA`), así que el cargador se queda con el suyo y
salta los demás.

### La cabecera del blob (`.mdn`), por versión — `src/mdn_format.h`

Little-endian (al revés que el `.mod`: el consumidor es la CPU del micro).

```
+00  magic[4]     "MDN\0"
+04  version      u16   1 (cabecera de 20 B) · 2 (24 B, desde V7/H2 15-sep-2026)
+06  abi_version  u16   MDN_ABI_VERSION: la tabla de helpers que el código espera (#284: igual, o rechazo)
+08  code_size    u32
+12  sym_count    u32   N × { name[32], thunk_offset u32 } detrás de la cabecera
+16  arch         u32   e_machine del ELF: 40 ARM · 243 RISC-V · 94 Xtensa · 0 = legacy ARM
+20  flags_crc    u32   SÓLO v2 (`#441`): CRC32 IEEE de los bytes del literal de la RECETA
```

**La receta (`#441`, V7/H2).** Los flags con que se compila un `.mdn` viven en
**un solo sitio**, `bpgenvm-c/include/bpvm_aot_recetas.h`, una receta por juego de
instrucciones y ABI —no por placa—: `ARMV8M_MAIN_SOFTFP` (Cortex-M33: Pico 2 y
STM32U5 comparten `.mdn`), `RV32IMAFC_ILP32F` (ESP32-P4), `RV32IMC_ILP32` (C3/C6,
sin destino en el IDE todavía). El IDE parsea el `.h`, compila con esa receta y
estampa en `flags_crc` el CRC32 de sus bytes; el firmware elige la suya por las
macros de su ISA (`__ARM_ARCH_8M_MAIN__`, `__riscv_float_abi_*`), hashea el mismo
literal y **rechaza** (`MDN_ERR_RECETA`, la función sigue interpretada, mensaje al
log) el blob que traiga otra: misma arquitectura y otros flags es lo que ni `arch`
ni las fechas ven (`-mcmodel=medany`, 17-ago, dejó inservibles todos los `.mdn`
RISC-V). Un blob **v1** no lleva huella y se carga «a ciegas» con aviso, como un
MOD7 sin datos de versión. La placa publica su receta en el INFO (`aotReceta`,
`aotFlagsCrc`) y el IDE no sube un `.mdn` de otra. Sólo entran los flags de
COMPILACIÓN: los de enlace y el guión `aot/mdn.ld` los vigila el guardián de
relocalizaciones de `MdnPack` en el build.

📌 **Por eso el formato de N familias es el mismo que el de una.** Hoy se emite
N=1 —la familia de la placa conectada— y el multifamilia no pide ningún cambio de
formato: sólo que el compilador emita más blobs. La poda del IDE se vuelve
*«quédate con el blob cuyo `arch` coincide»*, que es tirar bytes, no reformatear.

### Lo que mata

Igual que §4.5, pero peor, porque el `.mdn` se puede desparejar **de dos maneras**
y las dos nos han pasado:

- **En el tiempo** — *«el `.mdn` es MÁS VIEJO que su `.mod`»*. Existe un guardián
  en el IDE porque hacía falta; con un solo fichero no hay de qué guardarse.
- **De familia** — se subió un `.mdn` de ARM a una P4. El `arch` estaba en el
  fichero y la placa decía el suyo, pero nadie los comparaba. Con bloques
  etiquetados **dentro**, elegir el equivocado deja de ser posible por
  construcción.

### Estado

- **23-ago-2026**: el formato cerrado y los dos cargadores lo leen; la sección se
  emitía vacía.
- **25-ago-2026** (N1.4): el IDE funde el blob en el `.mod` y las familias lo
  cargan desde ahí; el `.mdn` suelto de la app ya no se escribe.
- **15-sep-2026** (V7/H2, `#441`): blob **v2** con la huella de la receta; los
  lectores (cargadores, `Disasm`, `MdnInspect`, `test_modhdr`) caminan por la
  versión de cada blob. Fixture `test/fixtures/modhdr/NatNew8.mod` (MOD8 + blob v2).
---

## 5. Sección DATA

`dataSize` bytes opacos al loader. Contiene:
- Constantes (`const X: integer := 42` → 4 bytes con el valor).
- Globals (`var g: T` → tamaño según `T`).
- Class descriptors (ver §8).
- String literals: `[u32 byte_len BE][bytes UTF-8]` (V2 H2). Hasta V1 era
  `[u32 cp_count][i32 codepoint]×N`; ahora son bytes UTF-8 (≈4× más compacto
  para ASCII). Es solo un cambio de **codificación del contenido** del símbolo
  (dato), no de la estructura del contenedor. El layout coincide con un heap
  string `TYPE_ARRAY_I8` (sin el tag), así que `LEA_GLOBAL` empuja la dirección
  y el string se usa in-situ.

**Alineación a 4 (v3.0.1).** Cada símbolo ocupa su tamaño **natural**, y no todos
son múltiplo de 4: `integer`/`float`/`long`/`double` y los arrays numéricos sí lo
son, pero un **string** es `4 + N` bytes (N = longitud UTF-8, arbitraria), un
`int8`/`byte` const ocupa **1** byte, un `int16`/`short` **2**, y un
`byte[]`/`int16[]` const `4 + N` / `4 + 2n`. Si se empaquetaran a pelo, un símbolo
impar dejaría a los que le siguen (y al propio CS) en direcciones **no 4-alineadas**.
Por eso el frontend **redondea el tamaño de CADA símbolo al siguiente múltiplo de
4** (`slot = (len + 3) & ~3`): el dato va al extremo **bajo** del slot (donde apunta
su offset CS-relativo) y los bytes sobrantes quedan al alto, a cero. Como todos los
slots son múltiplo de 4, **cada símbolo cae en un offset 4-alineado** y el bloque
entero —y con él `codeStart = dataStart + dataSize` (= **CS**)— queda alineado sin
padding extra al final. Importa porque el barrido conservador del GC —en **ambas**
VMs— solo lee palabras **alineadas a 4**: un símbolo de referencia (un global) en
dirección no alineada sería invisible al GC → recolección de un objeto vivo (el bug
**GC-2** de v3.0.1). Como extra, evita accesos a `integer` no alineados, que
**fallan** en ARM/RISC-V. (Los campos DENTRO de un objeto no sufren esto: van en
slots de 4 bytes indexados —ver §8— y el compilador prohíbe arrays de tamaño fijo
como campo de clase, así que un campo de referencia siempre queda 4-alineado.)

El loader inyecta el bloque entero en memoria a partir de
`dataStart = moduleBase + extTableSize`. Los offsets dentro del bloque
los conoce el frontend al emitir (no se reportan al loader).

---

## 6. Sección CODE

`codeSize` bytes de bytecode. Ver `OPCODES.md` para la semántica de
cada byte y sus operandos.

El loader inyecta el bloque a partir de `codeStart = dataStart + dataSize`.
Después del último byte, `codeStart + codeSize` es la frontera donde
empieza el siguiente módulo (o `nextFreeAddress` para el próximo
`loadModuleToMemory`).

---

## 7. Layout runtime: organización del `memory[]`

Cuando el loader carga un módulo, el `memory[]` global de la VM queda:

```
   ↓ moduleBase (= nextFreeAddress alineado)
   ┌────────────────────────────────┐
   │ ext-table  (extCount * 4)      │  ← rellenado por linkAll con addr de imports
   ├────────────────────────────────┤
   │ data block (dataSize bytes)    │  ← ↑ dataStart
   ├────────────────────────────────┤
   │ code block (codeSize bytes)    │  ← ↑ codeStart  (= CS del módulo)
   └────────────────────────────────┘
   ↓ moduleBase + total = nextFreeAddress (del siguiente módulo)
```

Convenciones clave:
- **CS** del módulo = `codeStart`. Todos los opcodes (jumps, NEW_OBJECT,
  CALL/RET) trabajan con offsets relativos a CS.
- Los offsets a símbolos del data block son **negativos** desde CS
  (`dataStart - codeStart < 0`).
- La ext-table es per-module; cada import resuelto ocupa 4 bytes con
  la dirección absoluta. `CALL_EXT idx` lee `mem[extTableAddress +
  idx*4]` para obtener la dirección destino.

Detrás de todos los módulos cargados viene el **heap** (a partir de
`vm.heapStart = nextFreeAddress` tras el último módulo). Ver
`HEAP_LAYOUT.md`.

---

## 8. Layout del Class Descriptor (en el data block)

Cada `public class X` registra un símbolo en el data block con este
layout binario. La dirección del descriptor (CS-relative o absoluta
según el contexto) es lo que `NEW_OBJECT csOff` empuja al stack como
el `class_ptr` de la nueva instancia.

```
offset  size  campo
─────────────────────────────────────────────────────────────────────
  +0    u16   num_fields
  +2    u16   num_methods
  +4    u16   bitmap_words   = ceil(num_fields / 32)
  +6    u16   _pad           (= 0)
  +8    i32   parent_offset  CS-relative al descriptor del padre.
                              0 = sin padre.
                              Cross-module: se escribe 0 al emitir y
                              `linkAll` lo PARCHA usando class_fixups (§4.3).
 +12    bw*4  field_bitmap   bit k=1 ⇒ field[k] es ref (GC trace)
 +12+bw*4  bw*4  owner_bitmap  bit k=1 ⇒ field[k] es owner (FREE recursivo)
 +12+2*bw*4  num_methods*4  vtable  i32 offsets relativos a CS del módulo
                                     dueño. Sentinel -1 = método heredado
                                     cross-module no implementado localmente
                                     (la VM sube por parent_offset).
```

Notas:
- `bw = bitmap_words`. Si `num_fields == 0`, `bw == 0` y las dos
  bitmaps ocupan 0 bytes.
- `parent_offset` es CS-relative al CS del MÓDULO del descriptor que se
  está leyendo. Para navegar a un parent cross-module la VM hace
  `parentAbs = getCSForDataAddr(child) + parent_offset` (resolved via
  `loadedModules`).
- La vtable nunca incluye constructores; el `__init` de cada clase es
  una función normal accesible por nombre.

---

## 9. Header de objeto en heap

Aunque vive en el heap (no en el `.mod`), se documenta aquí por su
relación con `NEW_OBJECT`. Cuando `NEW_OBJECT csOff` ejecuta:

```
ref := heapAlloc(num_fields * 4, TYPE_OBJECT)
writeI32(ref, classPtr)         ; primer field = class_ptr
zero los siguientes (num_fields-1)*4 bytes
push ref
```

El `class_ptr` ocupa el slot `[0]` de la instancia; los fields del
usuario empiezan en el slot `[1]` (= ref+4). `GET_FIELD/SET_FIELD slot`
acceden a `mem[ref + 4 + slot*4]`.

El header del bloque heap (4 bytes ANTES del `ref` que ve el código BP)
guarda type+size — ver `HEAP_LAYOUT.md` §2 para detalle.

---

## 10. Detección de versión

El loader debe:
1. Leer 4 bytes y mirar el MAGIC. **No hay byte de versión separado**: el MAGIC
   ES la versión, y también la declaración de ABI (ver `#284`).
2. **El tamaño del header depende del MAGIC** — 28 / 32 / 36 / 44 bytes para v5 /
   v6 / v7 / v8. Leerlo mal desplaza todas las secciones. En Java está resuelto en
   `ModFormat.headerSizeDe(magic)`; en C, `BPVM_HEADER_SIZE_V8`.
3. **Qué se ejecuta hoy: v6, v7 y v8.** v5 se **rechaza** — es anterior al
   ensanchado de refs 4→8B y su ABI no se puede garantizar, así que correrlo
   corrompería en silencio.
4. 📌 **v7 y v8 NO obligan a regenerar nada.** Sólo añaden secciones y campos; el
   ABI (el ancho de referencia) no cambia, y por eso v6 sigue siendo ejecutable.
   El gate de `#284` vigila el ABI, no el número; la norma de §3.1 vigila las
   VERSIONES, y sólo actúa cuando el módulo lleva los datos (v8).

Para añadir un campo sin romper compat se usa el truco de subsección
opcional detectable por bytes remanentes (ver §4.2 / §4.3). Para
cambios incompatibles se sube el número.

---

## 11. Tabla de tamaños fijos

| Símbolo | Valor | Significado |
|---|---|---|
| `MAGIC` | `0x4D4F4435` | "MOD5" — rechazado (ABI ambiguo) |
| `MAGIC_V6` | `0x4D4F4436` | "MOD6" — con sección `interface` |
| `MAGIC_V7` | `0x4D4F4437` | "MOD7" — con sección `native` |
| `MAGIC_V8` | `0x4D4F4438` | "MOD8" — con `gen`, `ifaceCrc` y la subtabla `deps` (§3) |
| `HEADER_SIZE` / `_V6` / `_V7` / `_V8` | 28 / 32 / 36 / 44 | bytes del header, según MAGIC |
| `FORMAT_VERSION` | 8 | versión lógica (informativa) |
| `CLS_OFF_NUM_FIELDS` | 0 | offset del campo en el class descriptor |
| `CLS_OFF_NUM_METHODS` | 2 | |
| `CLS_OFF_BITMAP_WORDS` | 4 | |
| `CLS_OFF_PARENT_OFF` | 8 | |
| `CLS_OFF_FIELD_BITMAP` | 12 | (los bitmaps empiezan aquí) |
| `EXT_TABLE_ENTRY_SIZE` | 4 | bytes por entrada de la ext-table |

---

## 12. Cambios entre versiones

- **v9 + las interfaces** (V8 H7.3, `#756`, 1-nov-2026): la subtabla `CLASES` en su **versión 2** (cada fila con sus
  interfaces; el bit 2 de una clase es interfaz, y el de un método, implementa), la ref `IFAZ_MET` (7), los opcodes
  `0xBF..0xC1`, E11 y E12, y la interfaz del módulo en `bpi 10` (`iface`, `implements`). **Sólo en los módulos que usan
  interfaces**: todos los demás salen byte a byte como antes (medido: los 663 programas del repo y los 31 módulos de la
  stdlib) y corren en los firmwares de V7. El MAGIC no cambia.
- **v9** (actual: diseñado el 16-sep-2026, el formato del compilador desde el 17-sep, V7/H2 `#523`): **enlace tardío**.
  Subtabla `refs` (3.3) en `imports`, subtabla `CLASES` (4.5) en `exports` (y ya no
  hay 4.3/4.4), la interfaz sin números (`bpi 9`: sin `layout`, `slot`, `recv`),
  once opcodes `_REF` (`0xB1..0xBB`) y el campo `+6` del descriptor (§13). Misma
  cabecera de 44 bytes, mismo ABI de refs: las VMs de V7 cargan v6/v7/v8/v9 y un
  MOD9 con un dueño MOD7/8 se rechaza con E9 (§13.7).
- **v8** (15-sep-2026, V7/H2): el módulo lleva **contra qué se compiló**:
  `gen` e `ifaceCrc` en la cabecera (44 bytes) y la subtabla `deps` en `imports`
  (§3); las VMs aplican la norma de versiones (§3.1). La interfaz se escribe con
  `\n` fijo (el CRC es la identidad de esos bytes: igual en Windows y Linux).
  **Aditivo**: v6 y v7 se siguen ejecutando (sin datos de versión). El mismo día,
  el blob `.mdn` de la sección `native` pasa a **v2** (24 B, `flags_crc`: la
  huella de la receta, `#441`, §4.6); los blobs v1 se siguen cargando con aviso.
- **v7** (23-ago-2026): sección **`native`** entre `interface` y `data`
  (§4.6) — el `.mdn` deja de ser un fichero aparte. Header de 36 bytes.
  **Aditivo**: no cambia el ABI, así que los `.mod` v6 se siguen ejecutando y no
  hubo que regenerar ninguno.
- **v6** (V4/H6.a): sección **`interface`** entre `exports` y `data` (§4.5) — el
  `.bpi` deja de ser un fichero aparte; se borraron 71. Header de 32 bytes. Y
  marca la era de refs de 8 bytes, que es lo que hace ejecutable un módulo (#284).
- **v5**: cada import lleva `fromPath` además de `name`.
- **v4**: se añade `librarySize` al header + sección `library`.
- **v3**: layout actual del class descriptor (con `bitmap_words` y
  `parent_offset` en sus posiciones actuales).
- **v2**: introducción de `bitmap_words`.
- **v1**: formato inicial mínimo.

Los .mod v4 e inferiores ya no se cargan (el MAGIC cambió). El frontend
emite siempre la versión actual.

---

## 13. v9 — el enlace tardío (`MOD9`, V7/H2 `#523`)

**El problema que resuelve.** Hasta v8, un módulo que usa una clase de otro lleva
horneados los números que dependen de la forma de esa clase: la ranura de vtable de
cada método, el offset de cada campo, el descriptor que construye, el layout del
padre que extiende. Un cambio de forma en la librería (un método nuevo en medio, un
campo más) invalida esos números y la norma `#500` (§3.1) obliga a recompilar al
consumidor — y lo que la norma no ve (misma ranura, otra firma) corre en silencio con
la ranura equivocada. **Criterio decidido (Eduardo, 15-sep): el de Java.** Un cambio
de FORMA corre; un miembro usado que desaparece o cambia de firma se rechaza al
enlazar nombrándolo; nada en silencio; y el `stdout` es byte-idéntico en las dos VMs.
**La norma de Eduardo**: *«lo que puede resolver el compilador es estático y lo que
puede resolver el cargador es dinámico»* — el módulo lleva referencias y el cargador
construye vtables, offsets y descriptores al cargar.

### 13.1 Qué lleva referencias (R1–R4)

- **R2 — quién va por ref**: toda clase de OTRO módulo, y toda clase **abierta** (R1:
  alguna de sus `extends` cruza módulo), también desde su propio módulo. Lo demás
  (clases cerradas del propio módulo) sigue horneado como en v8.
- **R4 — el nombre lleva la firma**: la clave canónica de un método es
  `nombre(tipos)retorno` (`k()s`, `m(i)i`, `suma(LLib/Base;)i`), la de un campo
  `nombre:tipo` (`n:i`, `s:Ls;`), la de un evento `nombre(tipos)`. Cambiar la firma
  o el retorno cambia la clave → el consumidor no la encuentra → rechazo E3/E4/E5.
  Las funciones de módulo importadas (3.1) también llevan la firma (`Lib.f()i`).
- **La clase estática del receptor**: la ref de un miembro se hace contra la clase
  estática del receptor (no contra la que lo declara): `Hija.k()s` aunque `k` viva en
  `Base`. El cargador la busca subiendo por la cadena real.
- **Límites**: identificador ≤ 63 caracteres (error del lexer), clave ≤ 160 bytes.

### 13.2 Subtabla `refs` (3.3) — al final de `imports`

```
refs      ::= count:u16  ref_entry*
ref_entry ::= kind:u8  depIdx:i16  clase:UTF  clave:UTF
```

| `kind` | nombre | `clase` / `clave` | lo que el cargador escribe en `ext[count3.1 + i]` |
|---|---|---|---|
| 0 | `CLASE` | `Mod.Cls` / `""` | dirección **absoluta** del descriptor (`NEW_OBJECT_REF`, `INSTANCEOF_REF`, `CHECKCAST_REF`, `TRY_BEGIN_REF`) |
| 1 | `MET` | clase estática / `nombre(tipos)ret` | ranura de vtable (`INVOKE_VIRTUAL_REF`) |
| 2 | `CAMPO` | clase estática / `nombre:tipo` | ranura de campo (`GET/SET_FIELD[_LONG\|_OWNER]_REF`); la clave `__syncMutex:LMutex;` es la ranura del mutex de `sync` (la de la raíz de la cadena) |
| 3 | `EV_RECV` | clase / `evento(tipos)` | ranura receptora del evento |
| 4 | `EV_DEST` | clase / `evento(tipos)` | ranura receptora + 2 (el `$dest` del handler, `PUSH_REF`) |
| 5 | `SUBTIPO` | `S` / `T` | `1` si `T` sigue en la cadena efectiva de `S`; si no, E10. Ocupa una entrada de la ext-table aunque ningún opcode la lea: es la **asunción** `S <: T` que el consumidor hizo al compilar (una asignación, un `throw`, un ensanchado) |
| 6 | `SUPER` | clase estática del padre / `nombre(tipos)ret` | dirección **absoluta** de código del método EFECTIVO del padre (la primera fila con esa clave subiendo desde el padre por la cadena real; un privado de otro módulo no cuenta). Es el operando de `CALL_EXT` para `super.m()`: la única llamada no virtual de BP (J-3, 16-sep) |
| 7 | `IFAZ_MET` | interfaz estática del receptor / `nombre(tipos)ret` | V8 H7.3 (`#756`): la **posición** de la clave en la lista aplanada de la interfaz (§13.9). Es el 2.º operando de `INVOKE_IFAZ_REF`; el 1.º, la interfaz, va por una ref `CLASE` (que, contra una interfaz, da la dirección de su bloque) |

Con las interfaces (V8 H7.3), `CLASE` contra una interfaz da su **bloque** (su identidad, §13.9), y `SUBTIPO` con
una interfaz de supertipo mira las interfaces de la clase (o, si `S` es una interfaz, las de las que hereda): si ya no
está, el texto es `'App' asume que 'Lib.Punto' implementa 'Lib.Forma' y 'Lib' (gen 8) ya no: recompila 'App'`.

`depIdx` indexa la subtabla `deps` (§3); **`-2` = local** (`clase` va sin cualificar:
una clase abierta del propio módulo). La ref `i` vive en la ext-table del módulo en
`ext[count3.1 + i]` — la misma tabla que `CALL_EXT`; los opcodes `_REF` llevan ese
índice `u16` y **nada del código se parchea** (vale para XIP).

### 13.3 Subtabla `CLASES` (4.5) — en `exports`, tras 4.1 y 4.2

En v9 **no hay 4.3 ni 4.4** (los class fixups eran la forma v8 de parchear el
`parent_offset`; ahora lo escribe el cargador al construir).

```
clases    ::= ver:u16 (=1 o 2)  nClases:u16  clase*
clase     ::= name:UTF  flags:u8  csOff:i32  parentDepIdx:i16  parentName:UTF
              nFieldsOwn:u16  nMethodsOwn:u16  bwOwn:u16
              ownFieldBitmap[bwOwn]:i32  ownOwnerBitmap[bwOwn]:i32
              nMet:u16 (clave:UTF slot:u16 codeOff:i32 flags:u8)*
              nCam:u16 (clave:UTF slot:u16 ancho:u8 flags:u8)*
              nEv:u16  (clave:UTF recvSlot:u16)*
              [ver 2]  nIf:u16 (depIdx:i16 nombre:UTF)*
```

**La versión 2** (V8 H7.3, `#756`, 1-nov) es la de las **interfaces de clase**, y la escribe **sólo un módulo que
las usa** (declara una, o una clase suya implementa alguna): los demás siguen escribiendo la 1, idénticos byte a byte,
y corren en los firmwares de antes (decidido por Eduardo). Un módulo con la 2 en un firmware anterior se rechaza al
cargar: `'App': subtabla CLASES de version 2 (esperada 1)`. En la 2, cada fila acaba con sus interfaces: las de la
cabecera de una clase (`implements A, Lib.B`), o las de las que hereda una interfaz; `depIdx` es el de la subtabla
deps, o `-2` (local: el nombre va sin cualificar).

- **Una interfaz es una fila más**, con el **bit 2** de sus `flags` (el «id» de Eduardo: *«una clase y una interfaz se
  guardan igual o casi igual»*): `csOff` = 0 (no tiene descriptor), `parentDepIdx` = -1, sin campos ni eventos, y sus
  métodos con su clave, `slot` = su posición y `codeOff` = -1 (no tienen código). Una property suya son las claves de
  sus dos accesores (`getX()t`, `setX(t)V`).
- El **bit 2** de los `flags` de un **método**: **implementa** uno de una interfaz de la cabecera de su clase (lo que en
  el fuente lleva `@implement{ … }`); si al enlazar ninguna interfaz de la clase tiene ya esa clave, E12.

- `flags` de clase: bit 0 **abierta** (R1), bit 1 pública. `csOff`: offset del
  descriptor en la sección `data` del módulo (las cerradas lo traen hecho; en una
  abierta es el descriptor *provisional*, que no se usa: el cargador construye otro).
- `parentDepIdx` / `parentName`: el padre (`-1` = la raíz `Object`, `-2` = local).
- `nFieldsOwn`, `nMethodsOwn`, `bwOwn = ceil(nFieldsOwn/32)` y los dos bitmaps son
  **sólo los propios** (los del padre los aporta el padre efectivo al construir).
- Método: `slot` (ranura propia, relativa), `codeOff` (relativo al `codeStart` del
  módulo), `flags` bit 0 **override** (marcado al compilar: si el padre efectivo ya no
  tiene esa clave → E2), bit 1 **privado** (ocupa ranura, no se publica: otro módulo no
  la ve, y una property privada del padre no se sobreescribe desde el hijo).
- Campo: `slot`, `ancho` (4 u 8 bytes), `flags` bit 0 ref, bit 1 owner, bit 2 privado.
- Evento: `recvSlot` (ranura receptora propia).

El cargador **valida la subtabla al cargar** (versión, coherencia de `bwOwn`,
`parentDepIdx` dentro de `deps`, `csOff` dentro de `data`, ranuras dentro de rango) y
rechaza el `.mod` como corrupto antes de enlazar (`bpvm_link_validar_clases`).

### 13.4 Lo que hace el cargador (las dos VMs, el mismo algoritmo)

1. **Registrar** todas las clases de los módulos v9 (`Mod.Cls → fila 4.5`).
2. **Construir** las abiertas, memoizado padre→hijo: `P` = el padre real (E1 si no está;
   E9 si su módulo es MOD7/8; E7 si hay ciclo); `nF = P.nF + nFieldsOwn`; por cada
   método propio, `buscarClave` en la cadena de `P` ignorando las filas privadas de
   otro módulo → si existe, **override en esa ranura**; si no existe y estaba marcado
   override → E2; si no, ranura nueva `nM++`. E6 si `nF` o `nM` > 65535. El descriptor
   nuevo va a la **arena** (detrás del último módulo; el heap arranca detrás): cabecera,
   `+6 = índice del módulo del hijo + 1` (§8), `parent_offset` hacia el descriptor
   efectivo del padre, bitmaps = los efectivos del padre OR los propios desplazados
   `P.nF`, vtable = `-1` en `[0, P.nM)` y `codeOff` en las propias y los overrides.
   Los privados: **gana la declaración más cercana** (una clave privada en el padre y
   pública en el hijo es pública para el hijo).
2b. **Las interfaces** (V8 H7.3, `#756`; §13.9): el bloque y la lista aplanada de cada interfaz (al construirla en el
   paso 2, que la memoiza como a una clase: las de las que hereda, primero; un ciclo es E7), y después, clase a clase
   en el orden de los módulos y de sus filas, la **lista de interfaces** de cada clase con la **tabla** de cada una, y
   la tabla de las clases que tienen alguna, ordenada por descriptor. Aquí salen E11 y E12. Si ninguna clase tiene
   interfaces, este paso no escribe nada: la arena queda como antes.
3. **Resolver las refs** 3.3 de cada módulo (§13.2) escribiendo la ext-table.
4. **Verificar**: ninguna ref sin resolver; cada descriptor construido coherente.

Antes de todo eso, los **imports 3.1** y, antes aún, **E9** (§13.7): un consumidor v9
con una ref a un módulo sin 4.5 se rechaza nombrando al dueño (medido el 17-sep:
si no, el import del constructor con firma caía antes con «no exporta … version
vieja?» y el mensaje útil no se veía nunca).

**Coste** (Pico 2, tres módulos, una clase construida, 11 refs): 995 µs de enlace en
MOD8 → 1.655 µs en MOD9, 66 B de arena, 4× menos `strcmp` (menos marcadores).

### 13.5 La interfaz (`bpi 9`) y la norma de versiones

La sección `interface` de un v9 no lleva números: sin `layout`, sin `field … slot n`,
sin `event … recv n` (el lector acepta las dos formas; el `layout` de una base
externa es vacío). `gen` e `ifaceCrc` siguen en la cabecera y en `deps`, pero para un
**dueño v9** la norma `#500` sólo aplica la regla 1 (**E0**: gen más antigua que la
sellada → rechazo); un `ifaceCrc` distinto es un diag informativo («compilado contra
otra interfaz … enlace tardío») y el enlace decide. Para un dueño MOD7/8 la norma
sigue entera (§3.1).

### 13.6 Los opcodes `_REF` (`0xB1..0xBB`)

Cada uno es idéntico a su hermano sin `_REF` salvo en el operando: `idx u16` de la
ext-table en vez del número horneado. Tabla completa en `OPCODES.md` §`0xB1..0xBB`.
`super.m()` a un padre por ref es `CALL_EXT` con el índice de una ref `SUPER`. Y los de las
interfaces (V8 H7.3), `0xBF..0xC1`, también por ref (§13.9).

### 13.7 Los rechazos del enlace (E0–E12), byte-idénticos en las dos VMs

| | cuándo | texto |
|---|---|---|
| E0 | el dueño es más antiguo que el sellado | el de la norma (§3.1, regla 1) |
| E1 | el padre (o la clase) ya no existe | `'App' extiende la clase 'Lib.Base' y 'Lib' (gen 8) no la tiene: recompila 'App'` · `'App' necesita la clase 'Lib.Base' y …` |
| E2 | override huérfano | `'App.Hija' redefine 'k()s' y 'Lib' (gen 8) ya no lo tiene: recompila 'App'` |
| E3 | método usado sin esa clave | `'App' necesita 'Lib.Base.k()s' y 'Lib' (gen 8) no lo tiene (tiene 'k()i'): recompila 'App'` — cita hasta 8 candidatas con el mismo nombre |
| E4 | campo | `'App' necesita el campo 'Lib.Base.p:i' y 'Lib' (gen 8) no lo tiene (tiene 'p:s'): recompila 'App'` |
| E5 | evento | `'App' necesita el evento 'Lib.Base.cambio(i)' y 'Lib' (gen 8) no lo tiene: recompila 'App'` |
| E6 | más de 65535 ranuras | `'App.Hija' tendria N ranuras (maximo 65535)` |
| E7 | ciclo de herencia | `ciclo de herencia en 'App.Hija'` |
| E8 | la verificación tras construir falla (una ranura sin dueño, dos claves iguales) | `la clase 'App.Hija' construida es inconsistente (ranura 7 sin dueno): .mod corrupto o bug del cargador` |
| E9 | dueño MOD7/8 de un consumidor v9 | `'Lib' es MOD8 y 'App' necesita su tabla de clases: recompila 'Lib'` |
| E10 | asunción de subtipo rota | `'App' asume que 'Lib.Derived' extiende 'Lib.Base' y 'Lib' (gen 8) ya no: recompila 'App'` · con una interfaz (V8 H7.3): `'App' asume que 'Lib.Punto' implementa 'Lib.Forma' y …` |
| E11 | V8 H7.3: la interfaz tiene un método que la clase no tiene (otra versión de la interfaz) | `'App.Rect' implementa 'Lib.Medible' y no tiene 'perimetro()d': recompila 'App'` |
| E12 | V8 H7.3: la clase dice implementar (bit 2) algo que ya no tiene ninguna de sus interfaces | `'App.Rect' implementa 'getEscala()i' y ninguna de sus interfaces lo tiene ya: recompila 'App'` |

Y con las interfaces, las dos formas de E1: `'App.Rect' implementa la interfaz 'Lib.Medible' y 'Lib' (gen 8) no la
tiene: recompila 'App'` (o `hereda de la interfaz`, si es una interfaz la que la nombra) y `… y en 'Lib' (gen 8) es una
clase: recompila 'App'`. Todas, byte a byte las mismas en las dos VMs (`compat/mezcla.py`, bloque F).

Por el wire llegan en `EXITED` con `status: "LINK_ERROR"` y `errorMessage`
(`BPVM_WIRE_PROTOCOL.md` §6.3); por el CLI, `=== ERROR DE LINK: … ===`.

### 13.8 Mezcla de formatos (D10, decisión de Eduardo)

- **Consumidor v9 + dueño MOD7/8**: **E9**. Sin camino degradado (sintetizar claves
  desde los marcadores reabriría los cuatro silenciosos medidos).
- **Consumidor MOD7/8 + dueño v9**: sus marcadores (`__cls_m_Base#k$`, `Base#setP#3`)
  no existen en v9 → el «no exporta … version vieja?» de siempre. Hacia atrás sí,
  hacia delante no.
- **Parque instalado**: el `.mod` MOD8 de una app en `/app` de una placa con firmware
  V7 (stdlib v9 embebida) muere en el enlace nombrando el símbolo: se recompila.

La red que lo sostiene: `compat/mezcla.py --formato=9` (194 filas, predicción escrita
antes de correr, las dos VMs; bloque B = la librería barajada con `--barajar`/
`--relleno`, el guardián contra cualquier número horneado que se escape), `compat/
compat.sh check-mod9` (el corpus entero y la stdlib en v9 con el `stdout` de v8),
`EnlaceTardioFixturesTest` (miVM, vtables y bitmaps escritos de antemano) y
`compat/v10_casos.py` (las reproducciones de la revisión adversarial).

### 13.9 Las interfaces en la arena (V8 H7.3, `#756`, 1-nov)

Lo que la VM mira al ejecutar lo escribe el cargador en la arena (detrás de los descriptores construidos), igual en las
dos VMs (`EnlaceTardio.construirInterfaces` y `lt_construir_interfaces`), y **sólo si alguna clase tiene interfaces**:
ni los descriptores ni la arena de un programa sin ellas cambian. Todo en big-endian y alineado a 4.

```
bloque de una interfaz   u16 n  u16 0                         4 B: su dirección es su identidad
lista de una clase       u16 n  u16 0  n × { u32 bloque  u32 tabla }
tabla (clase, interfaz)  u16 ranura[m]  (relleno a 4)         m = las claves de la lista aplanada de la interfaz
tabla de clases          u32 n  n × { u32 descriptor  u32 lista }   ordenada por descriptor (sin signo)
```

- **La lista aplanada** de una interfaz: las claves de las interfaces de las que hereda, en el orden en que las
  declara, y después las suyas; sin repetir. Es sólo del enlace (no va a la arena): de ella sale la posición de una
  ref `IFAZ_MET` y el orden de las tablas.
- **La lista de una clase**: las interfaces de su padre, tal cual —una hija conserva las ranuras de lo que sobrescribe,
  así que las tablas del padre le valen, y si no añade ninguna, comparte la lista del padre—, y detrás las de su
  cabecera, cada una con las de las que hereda (ella primero, en profundidad, sin repetir). Por cada interfaz nueva,
  su tabla: por cada clave de la lista aplanada, la ranura de la clase con esa clave (propia o heredada, no privada);
  si no la tiene, **E11**.
- **La tabla de clases** (`vm->ifaz_tabla`/`ifaz_n` en la VM-C; `tablaIfaz`/`nTablaIfaz` en la VM-Java): las clases
  con lista, para buscarla por descriptor con una búsqueda dicotómica.

Las tres instrucciones (`OPCODES.md`, `0xBF..0xC1`): **`INVOKE_IFAZ_REF`** busca la lista de la clase del objeto, en
ella la tabla de la interfaz, la ranura en la posición del método, y sigue como `INVOKE_VIRTUAL` (con su subida por los
padres); **`INSTANCEOF_IFAZ_REF`** y **`CHECKCAST_IFAZ_REF`**, si la interfaz está en la lista (la conversión, con el
`RuntimeError` de siempre, que nombra el tipo). Una clase con interfaces la construye siempre el cargador: en MOD9 toda
clase de un módulo de usuario ya es abierta (desciende de `Core.Object`); una de `Core` con interfaces, todavía no (el
compilador lo dice). La interfaz del módulo (§4.5) las publica en `bpi 10`, también sólo si las hay: `iface <n> public
[extends A,B]` … `end iface` (con sus `method` y `prop`) y, dentro de una clase, `implements A,B`.
