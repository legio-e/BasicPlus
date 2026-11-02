# Catálogo de builtins (`CALL_BUILTIN id`)

Catálogo de los builtins de la VM BP: qué hace cada uno. **La fuente de la tabla es
`bpstdlib/Core.bp`** (V7/H1 P2, 20-sep): cada primitiva es una función de `Core` cuyo
cuerpo es `inline CALL_BUILTIN n`, y `scripts/gen_builtins.py` GENERA de ahí el enum
`Builtin` de miVM y `bpgenvm-c/src/builtin_ids.h` —con la firma BP al lado de cada
id—. Hoy son **287** (0–286). Cada builtin tiene:
- `id` = `ordinal()` del enum `Builtin` (estable; **se añade siempre al
  final** para no romper compat con `.mod` ya emitidos).
- `bpName`: el nombre de su función en `Core`. El compilador ya no resuelve
  builtins por nombre (fuera `Intrinsics.java`): una llamada a `strlen` es una
  llamada a la función de la raíz, y en su sitio el compilador mete su cuerpo
  `inline`, el `CALL_BUILTIN` (manual §8.3b). Las públicas de
  `Core` (sin prefijo `__`) se ven sin cualificar en todo programa; las `__*` son
  para la stdlib, y módulos como Math/IO las exponen con nombres limpios (a veces
  con su propia declaración `inline CALL_BUILTIN n`, como `Machine`).
- Stack effect y semántica.

No todos tienen aquí su fila: los que no, por rangos, en «Los que no tienen fila
aquí», al final; su firma exacta, en `builtin_ids.h`.

Convención general:
- Cada builtin **pop** sus argumentos del stack del thread (en orden:
  primer arg más abajo, último arg en el top) y **push** exactamente
  un valor (el return) al terminar. Si la signature BP es void, push
  un `0` dummy — el frontend emite `SET_LOCAL __discard` para
  consumirlo. Esto unifica el dispatcher.
- Los builtins que pueden fallar lanzan `RuntimeError` BP atrapable
  vía `throwBpRuntimeError(tc, msg)`. El doc lo señala con "*RTErr*".

---

## 0..15 — Strings utilitarios

> **V2 (H2):** los strings son `byte[]` **UTF-8** (`TYPE_ARRAY_I8`), no arrays
> de codepoints i32. Los builtins con índice (`strlen`, `substring`, `charAt`,
> `charCodeAt`, `indexOf`) operan **por codepoint** (decodifican UTF-8); para
> ASCII es idéntico a V1. `print` emite los bytes UTF-8 directos (sin truncar).
> Ver HEAP_LAYOUT §6.

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 0 | `strlen` | `(s: string)` | `int` | Length del array de chars. |
| 1 | `parseInt` | `(s: string)` | `int` | `[+-] dígitos` (ASCII), con espacios alrededor (los bytes de 0 a 32), y que quepa en 32 bits. Si no, RTErr: `parseInt: '12abc' no es un número entero`, `parseInt: '99999999999' no cabe en un integer` (V8, `#716`: la misma regla en las dos VMs; antes, `strtol` en la VM-C y `Long.parseLong` en miVM, que mataba el hilo). |
| 2 | `parseFloat` | `(s: string)` | `float` | `[+-] (dígitos [. dígitos] \| . dígitos) [(e\|E) [+-] dígitos]`, con espacios alrededor; sin hexadecimal, `inf` ni `nan`. Lo válido lo convierte `strtod` / `Double.parseDouble` (iguales con un decimal limpio). Si no, RTErr: `parseFloat: '1e' no es un número`; lo que pasa del máximo de un `float`, `parseFloat: '1e39' no cabe en un float` (V8, `#716`). |
| 3 | `intToString` | `(i: int)` | `string` | Decimal con signo. |
| 4 | `floatToString` | `(f: float)` | `string` | Java `Float.toString`. |
| 5 | `boolToString` | `(b: bool)` | `string` | `"true"`/`"false"`. |
| 6 | `upper` | `(s: string)` | `string` | Locale default. |
| 7 | `lower` | `(s: string)` | `string` | |
| 8 | `trim` | `(s: string)` | `string` | Espacios al inicio y fin (whitespace). |
| 9 | `substring` | `(s, from, to)` | `string` | `[from, to)`. RTErr out-of-range. |
| 10 | `indexOf` | `(s, needle)` | `int` | Primera aparición; -1 si no está. |
| 11 | `startsWith` | `(s, prefix)` | `bool` | |
| 12 | `endsWith` | `(s, suffix)` | `bool` | |
| 13 | `contains` | `(s, needle)` | `bool` | |
| 14 | `charAt` | `(s, i)` | `string` | String de 1 char. RTErr out-of-range. |
| 15 | `replace` | `(s, find, replacement)` | `string` | Todas las ocurrencias. |

## 16..18 — Numéricas enteras

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 16 | `abs` | `(i: int)` | `int` | Valor absoluto. |
| 17 | `min` | `(a, b: int)` | `int` | |
| 18 | `max` | `(a, b: int)` | `int` | |

## 19..28 — Numéricas float + constantes

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 19 | `sqrt` | `(f: float)` | `float` | Java `Math.sqrt`. RTErr si `f<0` (NaN). |
| 20 | `pow` | `(base, exp: float)` | `float` | `Math.pow`. |
| 21 | `log` | `(f: float)` | `float` | Logaritmo natural (`ln`). |
| 22 | `log10` | `(f: float)` | `float` | |
| 23 | `exp` | `(f: float)` | `float` | `e^f`. |
| 24 | `sin` | `(f: float)` | `float` | Radianes. |
| 25 | `cos` | `(f: float)` | `float` | |
| 26 | `tan` | `(f: float)` | `float` | |
| 27 | `random` | `()` | `float` | `[0, 1)`, como el `RND` de siempre. Nunca devuelve 1.0. |
| 28 | `randomInt` | `(min, max: int)` | `int` | `int(random() * (max-min)) + min` → `[min, max)`, el alto **NO** entra. `max <= min` devuelve `min`. |

> **#347 — `randomInt` no es un generador aparte: es `random()` escalado.**
> Esa es la definición, y de ella sale sola la respuesta a "¿entra el extremo
> alto?" — no entra, igual que en el `INT(RND*n)` de toda la vida. Definirlo
> como la fórmula y no como prosa quita la ambigüedad de raíz: no hay dos
> conceptos que puedan divergir, hay uno. Y hace que `randomInt(0, len(a))` sea
> **siempre** un índice válido, que es su uso más frecuente.
>
> Un dado, por tanto, es `randomInt(1, 7)`.
>
> **No son builtins portables**: necesitan entropía, y la pone cada familia en
> `bpvm_platform_random_u32()` — TRNG del RP2350, `esp_random()` en el ESP32,
> PRNG sembrado del reloj en host y en el STM32 (ver `bpvm_platform.h`). La
> conversión a `[0,1)` y a rango de enteros la hace el núcleo, **igual para
> todas**: si cada cintura convirtiera a su manera, el mismo programa daría
> distribuciones distintas según la placa.
>
> Por lo mismo, **la paridad dual-VM no puede cubrirlos** (dos fuentes distintas
> nunca dan la misma salida). Lo que los vigila es `make test-rand`, que
> comprueba lo que sí es invariante: rango, exclusión del alto y cobertura.

## 29..30 — Constantes

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 29 | `pi` | `()` | `float` | `Math.PI`. |
| 30 | `e` | `()` | `float` | `Math.E`. |

## 31..33 — Conversión float→int

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 31 | `floor` | `(f: float)` | `int` | `Math.floor`. |
| 32 | `ceil` | `(f: float)` | `int` | `Math.ceil`. |
| 33 | `round` | `(f: float)` | `int` | Half-up. |

## 34..35 — Tiempo

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 34 | `now` | `()` | `int` | `System.currentTimeMillis()` truncado a i32. |
| 35 | `sleep` | `(ms: int)` | `void` | **Bloquea** el thread BP `ms` milisegundos. Cede el scheduler (`tc.status=BLOCKED_SLEEP`, `tc.wakeAt=now()+ms`). Setea `tc.yieldRequested=true` para abandonar el inner loop. |

## 36 — String que devuelve array

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 36 | `split` | `(s: string, sep: string)` | `string[]` | Split por sep. |

## 37..42 — I/O

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 37 | `input` | `()` | `string` | **Sólo VM-Java**: con IDE abre su ventana; sin IDE lee una línea de stdin (sin newline final). En la VM-C (placas y micro simulado) lanza RTErr `builtin 37 no soportado en esta VM (subconjunto C)` — decidido, ver `PENDIENTES.md`. |
| 38 | `readFile` | `(path: string)` | `string` | Lee fichero entero como UTF-8. RTErr si no existe / sandbox. Si no es UTF-8 válido, `EncodingError` (`readFile('x'): UTF-8 mal formado en el byte 5 (0xF1)`; V8/H5, `#711`); con el 285, «�». |
| 39 | `writeFile` | `(path, content: string)` | `void` | Sobreescribe. Crea el fichero si no existe. RTErr sandbox. |
| 40 | `appendFile` | `(path, content: string)` | `void` | Append. RTErr sandbox. |
| 41 | `fileExists` | `(path: string)` | `bool` | Sólo FICHEROS: una carpeta da `false` (V7/H10 #630, 2-oct, Eduardo; la VM-Java decía `true`, la VM-C `false`). Para una carpeta, `IO.dirExists`. RTErr sandbox. |
| 42 | `listDir` | `(path: string)` | `string[]` | Nombres de las entradas (no recursivo), **ordenados por bytes**, en las dos VMs y en placa (V7/H3, 21-sep: la VM-C no lo tenía; el orden de un listado es del FS, y el mismo programa tiene que imprimir lo mismo en el PC y en la placa). RTErr `listDir('p'): no se pudo abrir` (antes miVM devolvía una lista vacía); en la VM-C, también `demasiadas entradas` si los nombres no caben en la zona de rascar. |
| 126 | `readFileBytes` | `(path: string)` | `byte[]` | Lee fichero entero como `byte[]` crudo (binario sin pérdidas: NUL/>127/UTF-8 inválido). #247. |
| 127 | `writeFileBytes` | `(path, data: byte[])` | `void` | Escribe los bytes crudos del `byte[]` (sobreescribe). #247. |

## 43 — Debug

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 43 | `gc` | `()` | `void` | Dispara GC manual. Imprime stats. |

## 44..48 — Soporte para clases stdlib (intrínsecos `__*`)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 44 | `__newRefArray` | `(cap: int)` | `T[]` | Aloca `TYPE_ARRAY_REF` con `cap` slots a 0. Bajo allocAnchor. |
| 45 | `__growRefArray` | `(old: T[], newCap: int)` | `T[]` | Aloca nuevo + copia `min(oldLen, newCap)` refs. |
| 46 | `__growIntArray` | `(old: int[], newCap: int)` | `int[]` | Idem para arrays i32. |

Los que crean un array (estos, `newIntArray` y los opcodes `NEWARRAY*`) multiplican sus bytes en 64 bits, y lo que no cabe
en la memoria de la VM entera es «No space in heap», atrapable (V8, `#712`: en 32 bits, `newIntArray(2^30 + 1)` daba un
bloque de 4 B con una longitud de mil millones). En la VM-C pasan todos por `bpvm_prim_nuevo_array`.
| 47 | `__charsToString` | `(chars: int[], len: int)` | `string` | Aloca string copiando `chars[0..len)`. Un código que no es un carácter (un sustituto, o fuera de 0..10FFFF), `EncodingError`: `appendChar: el código 55296 no es un carácter` (V8/H5, `#711`). |
| 48 | `charCodeAt` | `(s: string, i: int)` | `int` | Codepoint del char `i` (sin alocar). Fuera de rango, el RTErr de `charAt`: `charCodeAt: idx fuera de rango 9 (len=4)` (V8, `#717`; antes, la VM-C devolvía 0 en silencio). |

## 49..51 — Threading

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 49 | `__threadStart` | `(t: Thread)` | `void` | Crea un nuevo `ThreadContext` con stackBase nuevo, inicializa frame para llamar a `t.run()`, lo encola en `runQueue`. `run()` es la ranura 0 y se resuelve como cualquier llamada virtual, subiendo por la cadena de herencia (`#530`, V7: heredado de un padre de otro módulo, la VM-C no arrancaba el hilo y miVM saltaba a `cs - 1`). |
| 50 | `__threadJoin` | `(t: Thread)` | `void` | Bloquea el caller (`tc.status=BLOCKED_JOIN`, `blockedOnJoin = t.tid`) hasta que el thread target termine. Yield al scheduler. |
| 51 | `yield` | `()` | `void` | `tc.yieldRequested = true`. El scheduler re-encola en runQueue y elige otro tc. |

## 52..54 — Mutex

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 52 | `__mutexCreate` | `()` | `int` | Crea un `JavaMutex` con id nuevo. Devuelve el id (no un ref BP). |
| 53 | `__mutexLock` | `(m: Mutex)` | `void` | Si `owner == myTid` → RTErr "re-entrada". Si `owner == -1` → toma. Sino: `tc.status=BLOCKED_MUTEX, blockedOnMutex=mid`, yield. |
| 54 | `__mutexUnlock` | `(m: Mutex)` | `void` | Si `owner != myTid` → RTErr. Sino: `owner=-1`, despierta primer waiter (lo pone RUNNABLE y le da el lock). |

## 55 — Arrays

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 55 | `move` | `(src, dst, srcStart, dstStart, count)` | `void` | Copia `count` elementos (1, 2, 4 u 8 B cada uno según el array: `byte[]`, `integer[]`, `long[]`/`double[]`, referencias). Soporta overlapping en el mismo array. RTErr si: src/dst null o no son arrays; dst de solo lectura (una cadena, `#714`, V8: `move: dst es de solo lectura`); tipos distintos (i8/i16/i32/i64/ref); count negativo; offset negativo; rangos fuera de length — en ese orden y con los mismos textos en las dos VMs (`#535`, V7: la VM-C no comprobaba nada y copiaba 4 B por elemento; miVM rechazaba `long[]`/`double[]`). Leer de una cadena (como `src`) sí se puede. Paridad: `samples/MoveRango.bp` y `bpgenvm-c/samples/MoveSoloLectura.bp`. |

## 56..63 — Math intrínsecos (vía `import Math`)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 56 | `__sign_i` | `(i: int)` | `int` | -1/0/1. |
| 57 | `__sign_f` | `(f: float)` | `int` | -1/0/1. NaN → 0. |
| 58 | `__asin` | `(f: float)` | `float` | `Math.asin`. |
| 59 | `__acos` | `(f: float)` | `float` | `Math.acos`. |
| 60 | `__atan` | `(f: float)` | `float` | |
| 61 | `__atan2` | `(y, x: float)` | `float` | |
| 62 | `__factorial_i` | `(n: int)` | `int` | `n!` exacto i32. RTErr si `n<0` o `n>12` (desborda). |
| 63 | `__gamma_f` | `(x: float)` | `float` | Lanczos approximation (g=7, n=9). Reflection para `0<x<0.5`. |

## 64..76 — IO intrínsecos (vía `import IO`)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 64 | `__pathJoin` | `(a, b: string)` | `string` | Con separador del SO. No resuelve `..`. |
| 65 | `__pathParent` | `(p: string)` | `string` | `"" `si no tiene padre. |
| 66 | `__pathBasename` | `(p: string)` | `string` | Último componente. |
| 67 | `__pathExtension` | `(p: string)` | `string` | Sin punto. `""` si no. |
| 68 | `__pathAbsolute` | `(p: string)` | `string` | **V6 (#484): el nombre que el FS va a usar**, función pura de `(p, App.projectPath())`: empieza por `/` → tal cual · tiene esquema `x:` → tal cual · hay proyecto → `<projectPath>/<p>` · si no → tal cual (lo relativo se queda relativo). **NO** normaliza `..` ni `.` y no mira el disco: `pathAbsolute("a/../b")` = `a/../b`, igual en las dos VMs. Antes devolvía la ruta del SO del PC. |
| 69 | `__mkdir` | `(p: string)` | `void` | Recursive. No falla si existe. |
| 70 | `__rmdir` | `(p: string)` | `void` | Sólo si vacío. |
| 71 | `__removeFile` | `(p: string)` | `void` | |
| 72 | `__rename` | `(src, dst: string)` | `void` | Sobreescribe destino. |
| 73 | `__copyFile` | `(src, dst: string)` | `void` | Sobreescribe destino. |
| 74 | `__fileSize` | `(p: string)` | `int` | RTErr si >2GB (no representable en i32). |
| 75 | `__isDirectory` | `(p: string)` | `bool` | También `IO.dirExists` (#630): el mismo builtin, con el nombre que acompaña a `fileExists`. |
| 76 | `__lastModified` | `(p: string)` | `int` | Epoch ms truncado a i32. |

## 77 — UI (N20)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 77 | `__prompt` | `(spec: string)` | `string` | Si no hay `PromptSender` registrado (= no IDE): RTErr BP atrapable `prompt: no hay IDE conectado` — el mismo texto en la VM-C, que nunca tiene IDE para esto (#484). Si lo hay: bloquea el thread BP (`tc.status=BLOCKED_PROMPT`, registrado en `pendingPrompts[promptId]=tc`), envía `{"type":"PROMPT_REQUEST","promptId":N,"spec":...}` al IDE. El IDE responde con `{"type":"PROMPT_RESPONSE","id":M,"promptId":N,"values":...}` (wire v1) y la VM despierta el thread con el JSON empujado al stack. |

---

## 119..120 — Conversión string ↔ byte[] (V2 H2)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 119 | `toBytes` | `(s: string)` | `byte[]` | Copia los bytes UTF-8 del string a un `byte[]` nuevo (copia defensiva: el string es inmutable). |
| 120 | `fromBytes` | `(b: byte[])` | `string` | Copia los bytes de `b` a un string nuevo (interpretados como UTF-8). Es el DESCODIFICADOR: si no es UTF-8 válido, `EncodingError` (`fromBytes: UTF-8 mal formado en el byte 5 (0xF1)`; V8/H5, `#711`); con el 284, «�». |

> `string` y `byte[]` comparten el layout `TYPE_ARRAY_I8`; la conversión es una
> copia del payload con distinto tipo estático.
>
> **Nota de paridad C/Java (V7):** la VM-C (host/MCU) implementa hoy **todos** los
> builtins salvo tres: `input` (37), `heapFrag` (121) y `heapMap` (122) —`listDir` (42)
> está en las dos desde V7/H3—. Ver **§ Paridad VM-C** para el fallo limpio de esos tres.
> Y los de la GUI (`__gui*`) sólo existen donde hay GUI: el PC y las imágenes con
> pantalla.

---

## Paridad VM-C — subconjunto de builtins (GAP-1)

> *2026-06-07.* La VM-C (`bpgenvm-c`, host + MCU) implementa **un subconjunto**
> de los builtins de la VM-Java. No es un bug: es alcance (el device no necesita
> el filesystem del PC ni los diagnósticos del heap de Java). GAP-1 **acota y
> endurece** ese subconjunto en tres frentes:

**1. Fallo limpio (lo importante).** Un builtin fuera del subconjunto ya **no**
aborta la VM con `BAD_OPCODE` (crash duro, no diagnosticable). El `default` del
dispatcher (`builtins.c`) lanza ahora un **`RuntimeError` BP atrapable**:

    RuntimeError: builtin <id> no soportado en esta VM (subconjunto C)

— capturable con `try/catch e: RuntimeError`; si no se atrapa, termina el thread
con un mensaje claro (status `RuntimeError BP no atrapado`, **exit 1** desde V6/#481:
el código de «excepción BP no atrapada», igual en las dos VMs), nunca un crash
silencioso.

**2. Portados byte-exactos.** Las numéricas **enteras** `abs`/`min`/`max`
(ids 16/17/18) se portan a la VM-C. Aritmética `int32` pura → **paridad
byte-idéntica por construcción** (incl. `abs(INT_MIN)=INT_MIN`, vía `unsigned`).

**3. Caveat de paridad float (por qué NO se portan en masa).** Los builtins de
**coma flotante** (`sqrt`, `sin`, `cos`, `pow`, `log`…) NO se portan a la ligera:
`java.lang.Math` y la `libm` de C pueden diferir en el **último ULP** → el
`floatToString` resultante imprimiría dígitos distintos → **rompería el stdout
byte-idéntico**, invariante central del proyecto dual-VM. Portarlos exige
verificar la igualdad bit-a-bit del resultado (o compartir implementación):
trabajo de **H10**.

### Clasificación (estado en V7, octubre de 2026)

Lo de arriba es la historia de GAP-1 (junio). Desde entonces se portó casi todo — los
float con el formateador canónico GAP-4 (L13) y los string por codepoint — y el FS del
device es littlefs (V4/H2), así que «IO filesystem host-only» dejó de ser verdad. Medido
contra `builtins.c` (el `enum Builtin` de la VM-Java tiene 287 entradas; en V6, 240):

| Clase | Builtins | En VM-C |
|---|---|---|
| **Portados** | strings (0..15, `split`, `charCodeAt`, `toBytes`/`fromBytes`, `__strCopy` 263), numéricas y float/math (16..33, `random*`, Math `__*` 56..63 y `clamp`/`wrap`/`hypot`/`remap`), `now`/`sleep`/`gc`, arrays y threading/Mutex, **ficheros** (`readFile`/`writeFile`/`appendFile`/`fileExists`, `readFileBytes`/`writeFileBytes`, `listDir` desde V7, y el cursor de `IO.File` 240..249) y **rutas/FS de IO** (`__path*` 64..68, `__mkdir`…`__lastModified` 69..76), `__throwRte`, `__longToString`/`__doubleToString`, packs, eventos, `App.*`, `Machine.*` (con la tabla de capacidades 250..253 y `getEnv` 271), el módulo `Heap` y `Heap.compact` (264..270), y las de liberar es destruir (272..277) | ✅ todos |
| **GUI** (`__gui*`: 135..199, 206..207, 210, 214..220, 222..223, 234..239) | widgets, eventos de la GUI, rotación, `Chart`, `Gui.shot` | ✅ en el PC y en las imágenes con pantalla (la C5, la C6, las P4 y la DK2; LVGL llega por el pack). En las demás imágenes no están: lanzan `builtin N no soportado en esta VM (subconjunto C)` |
| **HW (device)** | Gpio/I2c/Spi/Uart/Pwm/Adc/Rtc/Wdt/Pulse/Machine/NeoPixel, y en V7 los buses con dispositivo y transacción (254..262) y el `release()` de cada recurso (278..281) | ✅ sí. **Sin backend ya no se inventan valores (V6)**: en el PC, Gpio/Pwm/Pulse/Uart/Spi/Machine escriben la misma línea de simulación en las dos VMs; NeoPixel donde no hay driver (STM32 y PC; la familia ESP32 lo tiene desde V7/H4, por el RMT) y Wdt en STM32/PC lanzan `RuntimeError`; Adc lanza si la conversión falla. **Los de bus (I2c 83–84, Spi 87–89, Uart 91–92, #534)**: array `null`, `count` < 0 o `count` > longitud → `RuntimeError` con el mismo texto en las dos VMs; SPI y UART sin tope, en tramos de 256 (una llamada al driver y una línea de simulación por tramo; en `Uart.read` el plazo cuenta por tramo); I2C es una transacción, no se parte: tope de 256 con error. Detalle por familia en `referencia.html` §14. |
| **Red (`Net`, 131..134)** | `tcpConnect`/`tcpSend`/`tcpRecv`/`tcpClose` | ✅ en las dos VMs del PC; **en placa no hay backend** (WiFi = V8): lanzan `Net: sin red en esta plataforma` |
| **Sólo VM-Java, por decisión** | `input` (37), `__prompt` (77 — en la VM-C lanza `prompt: no hay IDE conectado`) | ❌ decidido (`PENDIENTES.md`) |
| **Java-only (diagnóstico)** | `heapFrag`/`heapMap` (121/122) | ❌ intencional (herramientas de la VM-Java) |

> `input` **compila** sin import en cualquier módulo: el fallo llega en ejecución, en la
> placa. Con `heapFrag`/`heapMap` (diagnóstico), es el único punto donde el mismo `.mod`
> da salida distinta en las dos VMs a propósito.

---

## Reglas para implementadores

1. **Estabilidad del `id`**: el byte que va al `.mod` es `ordinal()`.
   Una vez asignado, NO se reordena. Nuevos builtins van **al final**.
2. **Stack effect uniforme**: pop args, push exactamente 1 valor de
   return. Para builtins void, push `0` dummy.
3. **Errores**: usar `throwBpRuntimeError(tc, msg)` para errores
   atrapables. Reservar `BpThreadFault` para errores no-recuperables
   (overflow de stack interno, opcode no implementado, etc.).
4. **Sandboxing**: los builtins de filesystem (`READ_FILE`,
   `WRITE_FILE`, `MKDIR`, etc.) DEBEN pasar el path por
   `ModuleManager.resolveInWorkdir(path)` si hay workdir configurado.
   Esto previene path traversal a fuera del sandbox.
5. **Threading**: builtins que ceden CPU (`SLEEP`, `THREAD_JOIN`,
   `MUTEX_LOCK` bloqueante, `__prompt`) deben:
   - Setear `tc.status` apropiado.
   - Setear `tc.yieldRequested = true` antes de retornar.
   - Empujar el "return placeholder" sólo cuando el thread despierte
     (en `MUTEX_UNLOCK`, `wakeUpJoinWaiters`, `deliverPromptResponse`).
6. **GC awareness**: builtins que alocan deben:
   - Llamar `heapAlloc` dentro de `synchronized(vmLock)` (o usar el
     helper `allocVm*` que ya lo hace).
   - Setear `tc.allocAnchor = ref` antes de soltar el lock para evitar
     que un GC concurrente libere el ref antes del push al stack BP.
7. **Un `string` null** (`#774`, V8): una cadena nunca es `null`, pero una variable `string` sí puede no referenciar
   ninguna. Si un parámetro `string` de un builtin llega a `null`, el builtin no llega a entrar: las dos VMs lanzan
   antes «referencia nula: `<nombre>` de un string null» (un `RuntimeError` atrapable, como un objeto `null`). No se
   escribe en cada builtin: `scripts/gen_builtins.py` saca de la firma de `Core.bp` dónde está cada parámetro `string`
   en la pila (bytes bajo `sp`: un `integer`, un `float` o un `boolean`, 4; un `long`, un `double` o una referencia,
   8) y lo genera en `Builtin.cadenas` (miVM) y en `bpgenvm-c/src/builtin_cadenas.h` (VM-C), que miran
   `dispatchBuiltin` y `bpvm_call_builtin`; así un builtin nuevo con un `string` lo trae puesto. El nombre del mensaje
   es el BP sin los `__` (o el de `NOMBRE_DE_ERROR`: `File`, `readFile`). Fuera, los que tratan el `null` como `""` a
   propósito (`NULL_ES_VACIA`): `__strCopy` (concatenar) y `__avisaDestructor` (escribe un mensaje). Los cuatro atajos
   del AOT que no pasan por el despacho (`string_length`, `string_char_at`, `string_char_code_at`,
   `string_substring`) lo miran a mano, con el mismo texto. `make check-builtins` (en la red) avisa si lo generado no
   está al día.

---

## 128 — `__throwRte(msg: string)` → no retorna  *(#248)*

Lanza el **RuntimeError nativo** de la VM con el mensaje dado — exactamente el
mismo path que los faults nativos (división por cero, null deref, índice fuera
de rango): construye el objeto RuntimeError, escribe `msg` en el slot 0 y hace
unwind hacia el handler BP más cercano (`try/catch e: RuntimeError`). Si no hay
handler, el thread termina con el error.

Uso interno: el `compareTo` por defecto de `Object` lo llama para señalar
"compareTo() no implementado" sin que cada módulo dependa del descriptor local
de RuntimeError (que #248 retira en favor de la clase única `Core.RuntimeError`).
Cualquier código BP puede llamarlo también.

- VM-Java: `case THROW_RTE` → `throwBpRuntimeError(tc, msg)`.
- VM-C: `case BUILTIN_THROW_RTE` → `builtin_throw(vm, tc, msg)`.

---

## 129 — `__longToString(v: long)` → string  *(L13)*

Decimal con signo de un i64 (8 bytes en pila). Lo emite `coerceToString`
para el concat `string + long` — antes no existía y el concat corrompía la
pila. VM-Java: `Long.toString`. VM-C: `%PRId64` (idéntico para todo valor).

## 130 — `__doubleToString(v: double)` → string  *(L13)*

Conversión a string con el **formateo canónico GAP-4** (el mismo de DPRINT:
punto fijo a 6 decimales con ceros recortados; científica fuera de rango) —
byte-idéntico entre VMs y, deliberadamente, idéntico a `print v`. Lo emite
`coerceToString` para el concat `string + double`.

> L13 también portó **FLOAT_TO_STRING (4)** a la VM-C y lo unificó en ambas
> VMs al formateador GAP-4 (antes la VM-Java usaba `Float.toString`, así que
> `"" + f` y `print f` podían diferir ENTRE SÍ para valores de más de 6
> decimales).

## 240–249 — el cursor de `IO.File`  *(V7/H3 #491, 21-sep)*

Lo que usan `IO.File` e `IO.TextFile`: un fichero abierto con su cursor, y TODO su estado en el objeto. El objeto
reserva un `byte[]` de estado (`newByteArray(__fstateSize(path))`) y cada primitiva recibe ese bloque como primer
argumento; la VM enhebra por esos bloques su registro de abiertos, así que el GC cierra lo inalcanzable y el fin de
RUN cierra lo que quede. No hay tope de ficheros abiertos. La regla de los abiertos es la de los verbos de fichero
entero (#657): un fichero abierto para escribir no se vuelve a abrir; para leer, los que se quiera. Mismos textos de
error en las dos VMs.

| id | firma | qué hace |
|----|-------|----------|
| 240 | `__fstateSize(path: string): integer` | Los bytes del bloque de estado para esa ruta: los que pide el motor del FS (littlefs o FAT), lo mayor entre leerla (resuelta como `readFile`) y escribirla (tal cual, como `writeFile`). En miVM, 0: el estado vive en Java. |
| 241 | `__fopen(st: byte[], path: string, mode: integer)` | Abre `path` sobre el bloque `st`. Modos: 0 `"r"` (existe; sólo lectura), 1 `"w"` (crea o trunca), 2 `"a"` (crea si falta; el cursor al final), 3 `"r+"` (existe; lectura y escritura). La lectura resuelve la ruta como `readFile`; la escritura la toma tal cual. RTErr: `File: modo no válido`, `File: sin bloque de estado`, `File: el objeto ya tiene un fichero abierto`, `File('p', "w"): ya está abierto` (otro lo tiene abierto y la regla no lo deja), `File('p', "r"): no se pudo abrir`. |
| 242 | `__fclose(st: byte[])` | Cierra. Idempotente: cerrar lo cerrado no hace nada. RTErr `File.close(): error al cerrar` si falla el motor. |
| 243 | `__fread(st: byte[], dst: byte[], off: integer, n: integer): integer` | Lee hasta `n` bytes desde el cursor en `dst[off..]`; devuelve los leídos (0 = fin). RTErr: `File.read(): fichero cerrado`, `… buffer nulo`, `… fuera del buffer`, `… error de lectura`. |
| 244 | `__fwrite(st: byte[], src: byte[], off: integer, n: integer)` | Escribe `n` bytes de `src[off..]` en el cursor. RTErr: los de `__fread` (con `File.write()`) y `File.write(): abierto sólo para lectura`. |
| 245 | `__fseek(st: byte[], pos: integer)` | Pone el cursor en `pos`, dentro de `[0, tamaño]` (extender es escribir al final). Fuera: RTErr `File.seek(N): fuera del fichero (tamaño M)`. |
| 246 | `__ftell(st: byte[]): integer` | La posición del cursor. |
| 247 | `__fsize(st: byte[]): integer` | El tamaño del fichero abierto. |
| 248 | `__fflush(st: byte[])` | Lleva al medio lo escrito (vuelca la caché del motor). RTErr `File.flush(): error de escritura`. |
| 249 | `__freadLine(st: byte[]): string` | `TextFile.readLine`, en nativo: la línea siguiente sin su `\n` (ni el `\r` de un `\r\n`), y el cursor tras ella; `""` al final (lo dice `eof()`). En BP salía 15× más lenta que `readFile` + `split` en la Pico (`FileBench`). Si la línea no es UTF-8 válido, `EncodingError` (`File.readLine(): UTF-8 mal formado en el byte N (0xNN)`), y queda leída (V8/H5, `#711`); con el 286, «�». |

En las placas, `IO.File` funciona igual en la flash (littlefs) y en la SD (`/sd`, FAT).

## 250–253 — la tabla de capacidades del micro  *(V7/H4, 22-sep)*

Una sola tabla por micro, la que publica su imagen, para el INFO del IDE, para BP y para validar las fachadas. Las
usa `Machine` (`uarts()`, `i2cs()`, `spis()`, `pins()`, `analogPins()`, `pwmPins()`, `pulsePins()`,
`openDrainPins()`, `hasPin()`, `isAnalog()`, `isInputOnly()`, `pwmUnit()`, `manufacturer()`, `package()`,
`adcUnits()`, `hasBreadcrumb()`, `MAX_CPU_MHZ()`, `MIN_CPU_MHZ()`), con su propia declaración privada de cada una.
Los pines del sistema —los de la flash o la PSRAM, los del cable del IDE y los de la pantalla que pone la imagen
(#574, #669)— no aparecen. En el PC, las dos VMs dan el mismo perfil, el del RP2350A (30 GPIO, el ADC en 26..29,
UART, I2C y SPI 0 y 1, 12 slices de PWM). Una clase que no existe: RTErr `Machine: clase de capacidad desconocida`.

| id | firma | qué hace |
|----|-------|----------|
| 250 | `__capsArray(kind: integer): integer[]` | Un array: 0, las UART válidas (lo que acepta `Uart.Port(id, …)`); 1, las I2C; 2, las SPI; 3, los pines que existen; 4, los analógicos; 5, los de PWM; 6, los de contador de pulsos; 7, los de colector abierto. |
| 251 | `__capsInt(kind: integer): integer` | Un número: 0, la frecuencia máxima de la CPU (Hz); 1, la mínima; 2, si se puede ajustar (0/1); 3, las salidas PWM; 4, las unidades ADC; 5, si hay migas (`setMark`, 0/1); 6, el tamaño del mapa de pines. |
| 252 | `__capsStr(kind: integer): string` | Un texto: 0, el micro; 1, el paquete (la variante: `QFN-60`/`QFN-80` en el RP2350); 2, el fabricante; 3, la unidad en que cuenta el PWM (`slice` o `canal`). |
| 253 | `__capsPin(pin: integer): integer` | La máscara del pin (0 = no existe, o es del sistema): 1 digital, 2 analógico, 4 PWM, 8 pulsos, 16 colector abierto, 32 sólo entrada. |

## 254–262 — pines y buses: soltar, el dispositivo y la transacción  *(V7/H4 y H6.c, 22–24 sep)*

El estado de cada pin y de cada bus lo lleva el común (la HAL BP), igual en todas las familias. Un bus es
COMPARTIDO —el programa, los packs y la TF pueden usar el mismo— y lo exclusivo es el dispositivo, su CS: un árbitro
da el bus por turnos, y cada llamada es UNA transacción en un turno. La configuración del bus la fija el ENV
(`spiN=`, `i2cN=`) o el primero que lo abre; otra distinta es un error con nombre. Una transacción se calcula antes
contra el tope de tiempo del bus —el `ms:` de su línea del ENV; 20 ms si no lo dice, nunca más de 100—: si no cabe,
RTErr sin tocar el cable (`SPI 2: la transacción ocuparía el bus N ms a V (máximo M ms)`). En el PC, las dos VMs
escriben la misma línea de simulación.

| id | firma | qué hace |
|----|-------|----------|
| 254 | `__gpioRelease(pin: integer)` | `Gpio.release(pin)`, lo que llama `release()` (y el destructor) de `Gpio.Pin`, `Gpio.AnPin` y `Adc.Channel`: el pin vuelve a libre y, desde #667, también en el hardware, como en el reset, si era del programa y estaba en uso (el de un bus o un pack no se toca; un LED de la placa vuelve al firmware). PC: `[gpio] release pin=N`. |
| 255 | `__adcInitPin(pin: integer): integer` | La vista del ADC POR PIN (`Gpio.AnPin`): busca su (unidad, canal) en el mapa, reserva el pin como analógico, inicia el canal y devuelve el canal. RTErr: el pin no existe o es del sistema (#574), `el pin N de <micro> no es analógico`, `el ADC del pin N es la unidad U y esta imagen sólo atiende la 1`, `ADC: el pin N no se pudo iniciar en esta imagen`. |
| 256 | `__spiRelease(bus: integer)` | `Spi.Bus.release()`: el programa suelta el bus; si nadie más lo usa, se cierra y sus pines quedan libres (otro puede abrirlo distinto). RTErr: `el SPI N no lo tienes abierto`, `el SPI N es de la placa (…): no se suelta desde un programa` (el del ENV), `el SPI N lo usa también <un pack, el firmware>: no se puede soltar`. |
| 257 | `__i2cRelease(bus: integer)` | `I2c.Bus.release()`: lo mismo para un I2C. |
| 258 | `__spiDevOpen(bus: integer, cs: integer, baudrate: integer, mode: integer)` | `Spi.Device(bus, cs, baudrate, mode)`: se engancha al bus, que ya tiene que estar establecido (por el ENV o por un `Spi.Bus`), y reserva su CS como salida, en reposo alto. Cada dispositivo lleva su velocidad y su modo (0..3). RTErr: velocidad o modo no válidos, el bus sin establecer, el CS ocupado. |
| 259 | `__spiDevXfer(bus, cs, baudrate, mode, op, tx: integer[], ntx, rx: integer[], nrx): integer` | Una operación del `Spi.Device`, en una transacción con su CS bajado: `op` 1 `write` (`ntx` bytes de `tx`), 2 `read` (`nrx` en `rx`), 3 `transfer` (`ntx` en los dos sentidos a la vez), 4 `writeRead` (escribe `ntx` y lee `nrx` sin soltar el CS). Devuelve los bytes (`nrx` en `read` y `writeRead`; `ntx` en los demás), o -1 si el driver falla. RTErr: `spi.write: el CS N no está reservado como salida (¿release()?)`, `… el SPI N no está abierto`, los de los arrays y el del tope. |
| 260 | `__spiDevExec(bus, cs, baudrate, mode, ops: integer[], nops, tx: integer[], rx: integer[]): integer` | `Spi.Device.execute(t)`: la `Spi.Transaction` que reúne `Spi.bp` —`ops`, pares (tipo, bytes): 1 escribir, 2 leer, 3 transferir; `tx`, todo lo que se escribe, seguido; `rx`, todo lo que se lee— en UNA transacción, con el CS bajado de principio a fin. Se valida entera antes (cada operación, los arrays, el tope). Devuelve el total de bytes, o -1. Si otro hilo cambia la transacción mientras corre: `spi.execute: la transacción cambió mientras se ejecutaba (¿compartida entre hilos sin Mutex?)`. |
| 261 | `__i2cWriteRead(bus, addr, tx: integer[], ntx, rx: integer[], nrx): integer` | `I2c.Bus.writeRead` (e `I2c.writeRead`): escribe `ntx` bytes y lee `nrx` con un *repeated start* entre los dos, sin soltar el bus. Hasta 256 bytes por transacción. Devuelve `nrx`, o -1 si el dispositivo no contesta (es su respuesta, como en `read`/`write`); lo que no la deja empezar —el tope, el bus, el turno— es RTErr. |
| 262 | `__i2cExec(bus, addr, ops: integer[], nops, tx: integer[], rx: integer[]): integer` | `I2c.Bus.execute(addr, t)`: la `I2c.Transaction` —de 1 a 3 mensajes, `ops` en pares (1 escribir, 2 leer; bytes)—, con *repeated start* entre ellos; hasta 256 bytes en total. Devuelve el total de bytes, o -1 si el dispositivo no contesta. RTErr: `i2c.execute: una transacción lleva de 1 a 3 mensajes (tiene N)`, una operación no válida, un `count` negativo, el tope. |

## 263 — `__strCopy(src: string, dst: byte[], pos: integer)` → integer  *(#554, V7/H7)*

La concatenación. Copia **todos** los bytes de la cadena `src` en el array de bytes `dst` a partir de `pos` y
devuelve la posición siguiente (`pos` + los bytes de `src`), así que las copias se encadenan. No la escribe nadie a
mano: la usan los helpers `__strconcat(a, b)` y `__strconcatN(s0 … sN-1)` que el compilador sintetiza en cada módulo
(una reserva del tamaño final —la suma de los `ALEN`, en BYTES— y un `__strCopy` por operando). Antes copiaban byte a
byte en bytecode: ~10 ms por concatenación de una cadena de 2 KB en placa.

`src` es una cadena en cualquiera de sus **dos** formas: un handle del heap o la referencia cruda de un literal del
bloque de datos (`LEA_GLOBAL`) —de cualquier módulo, compilado antes o después—. Una variable `string` a `null` se
concatena como la cadena vacía (`#512`; desde `#774`, V8, el `null` se escribe —una global o un campo sin valor ya
nacen `""`— y éste es de los dos builtins que no lo comprueban: regla 7). Por eso es un builtin propio de las cadenas y
no `move`, que rechaza lo que no tiene cabecera de heap.
`dst` tiene que ser un array de bytes del heap. Errores (con el mismo texto en las dos VMs): `strCopy: dst es null`,
`dst no es ref a heap`, `dst apunta a bloque libre`, `dst no es un array de bytes (type=N)`, `fuera de rango (pos=…
bytes=… dst=…)` — ninguno sale del helper, que reserva justo lo que copia.

## 264–269 — el diagnóstico del heap: `__heapSize`, `__heapFree`, `__heapLargestFree`, `__heapFreeBlocks`, `__heapHandles`, `__heapCollections`() → integer  *(V7/H7, 28-sep)*

Lo que un programa puede preguntar del heap, en las DOS VMs (el pedido de Eduardo del 5-ago; `V7_IDEAS` §H7). Se
usan por el módulo `Heap` (`Heap.getSize`, `getFree`, `getLargestFree`, `getFreeBlocks`, `getHandles`,
`getCollections`). LEEN contadores: no reservan nada ni recorren el heap, sólo su lista de libres (`heap.c`,
`bpvm_heap_cuentas`; `VirtualMachine.cuentasDelHeap`). El tamaño es lo que un programa puede llegar a ocupar (en la
VM-C, hasta `heap_top` menos la reserva de emergencia); lo libre, el final sin usar más la lista de libres; el mayor
libre, el mayor de los dos; los huecos, los bloques de la lista; los handles, los slots de la tabla en uso (vivos y
muertos sin recoger); las colectas, las de este RUN.

**No son números de paridad**: cada VM tiene su heap (la VM-C guarda la tabla de handles y la reserva de emergencia
dentro del bloque, y su tamaño depende de la memoria de cada placa), así que el tamaño y lo libre difieren entre VMs
—en el PC, 254753 y 376424 al arrancar—; lo usado, los handles y las colectas coinciden (medido: 108 B, 4 y 0 al
arrancar). Una prueba compara RELACIONES (`bpgenvm-c/samples/HeapInfo.bp`). `heapFrag` (121) y `heapMap` (122)
siguen como estaban: texto y sólo miVM.

## 270 — `__heapCompact`() → void — COMPACTAR el heap  *(V7/H7 #595, 28-sep)*

Lo usa `Heap.compact()` (pedido de Eduardo el 28-sep: *«que el usuario pudiese pedir compactar el heap. Siempre en
condiciones controladas»*). Una colecta completa y, con el mundo aún parado, deslizar lo vivo al principio del heap:
todo lo libre queda en UN bloque. Posible porque una referencia es un handle (V4): mover un objeto es cambiar su
entrada de la tabla. El MISMO algoritmo en las dos VMs (`heap.c`, `compactar`/`bpvm_heap_compactar`;
`VirtualMachine.compactarLocked`), sin memoria aparte: la tabla se ENHEBRA en las cabeceras —la cabecera guarda el
último índice que apunta al bloque (el bit 31, el de la marca del GC) y la entrada, lo que había en la cabecera—; se
recorre el heap en orden de dirección y cada bloque enhebrado baja al primer sitio libre, con sus índices puestos
al día. No se mueven, y dejan delante un hueco en la lista de libres: un bloque vivo sin handle y uno CLAVADO (el
bit 23, de los reservados: el estado de un fichero abierto —en la VM-C, el motor del FS lo guarda ahí y littlefs lo
enlaza en su lista—). Los dos bits sólo existen mientras dura la compactación.

Las condiciones, las mira el builtin en este orden y con estos textos, los MISMOS en las dos VMs:

- **desde `Main`**: el marco actual es el que abrió el `CALL` de `__startup` —su bp de vuelta es el fondo de la
  pila del hilo principal, y su pc de vuelta es código y no un centinela de `[0, 0x100)` (un handler de evento,
  un despacho de la GUI, el puente de una native)—; en la VM-C, además, sin una native a medias en el hilo (no
  abren marco BP). Si no: `RuntimeError` «Heap.compact: sólo se puede llamar desde Main».
- **sin otros hilos vivos**, mirado con el mundo ya parado (fuera, con el scheduler SMP, un worker podía resucitar
  entre mirar y parar a un hilo que murió debiendo un evento, #342); y un hilo TERMINATED que aún debe eventos
  cuenta como vivo, como en miVM, donde ni llega a TERMINATED. Si no: `RuntimeError` «Heap.compact: no se puede
  compactar con otros hilos vivos».

Y desde una `native` es un error de COMPILACIÓN (`AotCEmitter`): guarda direcciones del heap en variables de C. Con
el GC apagado (`--nogc`) no hace nada; si el recorrido del GC se descarrila (#494), lanza la prefabricada del heap
descarrilado. La prueba: `bpgenvm-c/samples/HeapCompact.bp` (paridad y ronda de placas).

## 271 — `__machineGetEnv(key: string)` → string — una clave del ENV de la placa  *(V7/H10 #626, 2-oct)*

Lo usa `Machine.getEnv(key)`: el valor de la clave en el ENV —las variables `clave=valor` de la placa, las de
«Variables del entorno» del IDE—, o `""` si no está. Decisión de Eduardo (2-oct): una primitiva GENERAL, con la que
`Rtc.Clock.isSet()` se escribe en BP (lee `imagen.fecha`, la fecha de la imagen que el arranque anota una vez). La
regla es la del ENV: claves sin distinguir mayúsculas, y el valor ENTERO o nada (`bpvm_env_get_exacto`, la de la
ranura `env_get` de la BIOS). Es la vista de la FLASH, lo último que se escribió, y se lee en cada llamada: un
`ENV_SET` del IDE se ve sin reiniciar (#638, 3-oct); lo que el arranque configura con el ENV (`stack`, `vm.runs`…)
cambia en el arranque siguiente.

En la VM-C, el ENV lo registra el arranque común (`bpvm_machine_set_env`, en Boot 2), que es quien sabe dónde está
el de cada familia; el micro simulado registra el suyo. Sin registrar —la VM-C de host— no hay ENV: `""`, como en
miVM, donde el PC tampoco tiene. Una clave de más de 63 bytes no puede estar (`""`); un valor de más de 255 lanza
`RuntimeError` («Machine.getEnv: el valor de la clave no cabe»), y no puede pasar: el wire los corta a 191.

---

## 272–277 — las primitivas de LIBERAR ES DESTRUIR  *(V7 #668, 8-oct)*

Las usan `Core.__destruye(x)` y `Core.__destruyePendientes()`, que son BP: el recorrido y las llamadas a los
destructores son el MISMO bytecode en las dos VMs, y aquí sólo está lo que BP no puede hacer (Eduardo: «La destrucción
del objeto en realidad la hace en gran medida su propio destructor… la típica división del trabajo de la POO»). Todas
son privadas de `Core`. El diseño, en `docs/V7_IDEAS.md` §«#666–#668», 7 y 8; la VM-C, en `src/destruir.c`.

| id | firma | qué hace |
|----|-------|----------|
| 272 | `__liberaSiSinDestructores(x: any): boolean` | Si en el árbol de owner de `x` (él, sus campos owner y los elementos de un array poseído, hasta abajo) no hay ningún destructor, lo suelta entero por la cascada de siempre y devuelve `true`. Si lo hay, no toca nada: `false`. Un `x` null o muerto: `true`. Es la vía rápida: un owner sin destructores no paga más que antes. |
| 273 | `__ordenDestruccion(x: any): any[]` | El árbol de owner de `x` en el orden de destruir: los hijos antes que su padre y, entre hermanos, del último al primero (Eduardo: «de los hijos hasta los padres, en orden inverso»); `x`, el último. Un hijo que aparece dos veces (dos dueños, o un ciclo) sale una. Sin memoria para ordenarlo: `RuntimeError` «No space in heap». |
| 274 | `__preparaDestructor(x: any): integer` | Qué destructor tiene `x`: 0, ninguno (el vacío de `Object` o de `Thread`, o un array); 1, el de una clase de `Object`; 2, el de un `Thread` —otra raíz: su destructor no está en la ranura del de `Object` (allí está `join`), así que `Core` tiene una llamada para cada una—. Y si tiene, lo deja listo para él: sus campos owner que apuntan a un hijo ya destruido pasan a null («en su destructor un objeto ya no tiene a sus hijos owner»; un handle muerto en el campo diría «no es null» y usarlo gritaría). |
| 275 | `__liberaSolo(x: any)` | La memoria de ESE bloque y nada más (sus hijos ya se destruyeron, antes que él): su handle muere y el bloque vuelve a la lista de libres (con dos runs, al siguiente GC). |
| 276 | `__avisaDestructor(x: any, msg: string)` | Un destructor lanzó: se dice por el diagnóstico, con la clase de `x` (`[bpvm] el destructor de Mod.Clase lanzó: msg — se sigue con la limpieza`), y la limpieza sigue (Eduardo: «se atrapa y se continúa con la liberación»). |
| 277 | `__siguientePendiente(): any` | Lo siguiente de la cola de lo pendiente de destruir —lo que el GC encontró inalcanzable con destructor, o, al acabar el programa, todo lo vivo con destructor, el más nuevo primero—, o null. Al vaciarse, el hilo que la vaciaba deja de hacerlo. |

Quién llama a `Core.__destruye`: el compilador, al morir un owner —un `var owner` al salir de su ámbito, al reasignarlo
o al acabar un `try var`; un campo owner al reasignarlo (el viejo, tras escribir el nuevo, si no es null ni el mismo)—;
y `Core.__destruyePendientes`, para lo que el GC encontró inalcanzable y para el final del programa. A ésa la mete la
VM en un hilo, entre cuantos, como un evento (vuelve por el sentinela `EVENT_RETURN`): los destructores corren como
código BP corriente, no llamados desde C. Un owner cuyo tipo no puede tener destructor (un string, un número, un array
de ellos) se sigue soltando con `FREE_REF` / `SET_FIELD_OWNER`, sin llamar a nadie; y con un `Core` de antes de #668
(sin `__destruye`), todo como antes. Las pruebas: `bpgenvm-c/samples/Destruye*.bp` (la red de paridad).

---

## 278–281 — SOLTAR el recurso de un objeto  *(V7 #667, 8-oct)*

Lo que usa el `release()` de las clases que toman un canal del hardware, y su destructor, que lo llama (#668: liberar
es destruir). En `Core` se llaman `__pwmRelease`, `__pulseRelease`, `__npRelease` y `__uartRelease`; la tabla da el
nombre con que los declara cada módulo de la stdlib. Eduardo: *«Si un objeto BP consume un recurso, el objeto debe tener un destructor y liberar si es posible
el recurso»*. Cada uno PARA lo suyo, deja la unidad libre para otro y devuelve el pin (o los pines) a como estaban en
el reset. Cómo, lo sabe el backend de cada familia (su `release`), el único que conoce el multiplexado de su micro;
después, el común apunta el pin como libre. En el PC no hay hardware: los stubs lo dicen por la salida, igual en las
dos VMs. En la stdlib, cada clase se los pide una vez y sólo si su constructor acabó (`tomado_`): un objeto cuyo
constructor lanzó también se destruye, y no tiene nada que soltar.

| id | firma | qué hace |
|----|-------|----------|
| 278 | `Pwm.releaseSlice(sliceId: integer, pin: integer)` | Para el PWM y suelta su canal: en la Pico el slice (si su otro canal no es de nadie); en las ESP la ranura del LEDC, y su temporizador si no lo comparte otra; en la STM32 el canal del timer, y el timer si no lo comparte otro (el TIM3 de la Nucleo). El pin, al reset. PC: `[pwm] release slice=S pin=P (sim)`. |
| 279 | `Pulse.releaseSlice(counterId: integer, pin: integer)` | Para el contador y suelta su unidad: el slice de la Pico, la unidad PCNT de las ESP, el timer de la STM32. El pin, al reset. PC: `[pulse] release id=I pin=P (sim)`. |
| 280 | `Neopixel.__npRelease(pin: integer)` | Suelta la máquina de la PIO (Pico) o el canal RMT (ESP) de la tira, tras dejar salir lo último que se le mandó. El pin, al reset; la tira de la PLACA (la NeoPixel de la Metro) no se suelta: vuelve a ser del firmware. `Strip.release()` la apaga antes (`clear()` y `show()`): los LED no se quedan encendidos. PC: nada. |
| 281 | `Uart.releasePort(bus: integer, tx: integer, rx: integer)` | Apaga la UART (su interrupción y su driver o su periférico), suelta su anillo de recepción y sus pines vuelven al reset. PC: `[uart] release bus=B (sim)`. |

Y el pin de un `Gpio.Pin` (254, `__gpioRelease`, el de siempre): ahora también vuelve al reset EN EL HARDWARE —una
salida deja de serlo; Eduardo: *«los pines por defecto deben ser input o nada. Si están como output se debería
revertir»*—, si era del programa y estaba en uso (lo de un pack o de un bus no se toca). Un LED de la PLACA no va al
reset: vuelve a ser del firmware, como lo dejó su arranque (el GP25 de la Pico 2; el verde, el azul y el rojo de la
Nucleo; el rojo y el verde de la DK2). En el PC, `[gpio] release pin=N`.

**Tras `release()`, usar el objeto es un error con nombre** (`RuntimeError` «Pwm.Slice del pin 12: ya se soltó con
release()»): el canal que soltó puede ser ya de otro objeto. Lo hacen `Pwm.Slice`, `Pulse.Counter`,
`Neopixel.Strip`, `Uart.Port`, `Gpio.Pin` (y `AnPin`) y `Adc.Channel`; `Spi.Device` ya lo hacía en C (su CS).

**La red del final del RUN** (`bpvm_destroy`, antes de soltar los pines): lo que ningún destructor soltó —un
`Gpio.init` sin objeto, una destrucción cortada por el tope de 3 s— lo suelta el backend de cada familia
(`fin_de_run`), y se dice por el diagnóstico: `[bpvm] fin de RUN: 1 PWM seguian en marcha: se paran y sus pines
vuelven al reset (#667)`, y lo mismo con los contadores, las tiras, las UART y los pines. En el PC no hay hardware: la
red calla. Las pruebas: `bpgenvm-c/samples/SueltaRecursos.bp` (la red de paridad) y `make test-suelta` (backends falsos
que cuentan: con objetos lo sueltan sus destructores; sin objetos, la red).

## 282 — `__strCompare(a: string, b: string)` → integer — comparar dos cadenas  *(V8/H5, `#713`, 21-oct)*

Compara dos cadenas **por bytes**, sin signo (0..255), y devuelve -1, 0 o 1. Si una es prefijo de la otra, la más
corta va antes. En UTF-8 ese es el orden de los caracteres (sus puntos de código): `V8_IDEAS` §E5, «Comparar por
bytes». Un `null`, «referencia nula: strCompare de un string null» (`#774`, V8; antes contaba como la vacía), y lo
mismo `Str.compare` y `<` entre cadenas. Acepta las dos formas de una cadena (un
handle del heap o un literal crudo). En la VM-C es un `memcmp`; en la VM-Java, el mismo bucle sobre los bytes, sin
pasar por un `String` de Java (que cambiaría el UTF-8 mal formado, `#711`).

No lo escribe nadie a mano. En `Core` va sin `public`, para no cambiar su interfaz. Lo usan:
- `Str.compare`, con su propio `inline CALL_BUILTIN 282`;
- `<`, `<=`, `>` y `>=` entre dos cadenas: el compilador compara su resultado con 0 (`#713`). En una native, el AOT llama
  al mismo builtin por `call_builtin`, sin tocar la ABI;
- el contenido de `==` y `!=`: el `__strequals` que escribe el compilador en cada módulo. Sus reglas no cambian: `null`
  sólo es igual a `null` (lo mira antes de llamar a este builtin); con las dos con valor y la misma longitud, compara con
  este builtin.

Antes, `==` comparaba byte a byte en bytecode (17 instrucciones por byte), y `<` entre cadenas descuadraba la pila. Las
pruebas: `bpgenvm-c/samples/StrOrden.bp` (la red de paridad) y `make test-aotcompara` (el AOT contra el intérprete).

## 283 — `__strNace(b: byte[])` → string — la cadena nace en su sitio  *(V8/H5, `#714`, 23-oct)*

Convierte en cadena el `byte[]` que recibe, **en su sitio y sin copiarlo**: le pone el bit de solo lectura
(`HEAP_LAYOUT.md` §2.1) y devuelve la misma referencia. El bit ASCII no lo toca: ya lo dejó `__strCopy` al llenar el
array. RTErr (sólo por un mal uso interno): `strNace: es null`, `strNace: no es ref a heap` y `strNace: no es un array de
bytes`, con los mismos textos en las dos VMs.

No lo escribe nadie a mano. En `Core` va sin `public`: un programa que guardara el `byte[]` podría seguir escribiéndole
después (las escrituras `b[i] := x` no miran el bit). Lo usan los `__strconcat` y `__strconcatN` que escribe el
compilador en cada módulo, al final, con el array que acaban de llenar. Un `.mod` anterior no lo lleva: sus sumas siguen
sin marcar. (El `StringBuilder` no lo puede usar: una clase sólo ve lo público de su módulo; su `toString` sigue con
`fromBytes`, que copia y marca.) La prueba: `bpgenvm-c/samples/MoveSoloLectura.bp` (la red de paridad), con una cadena
de cada origen.

## 284–286 — descodificar, ignorando el error si se pide  *(V8/H5, `#711`, 23-oct)*

| id | nombre | firma | uso |
|---|---|---|---|
| 284 | `__fromBytesI` (alias público: `fromBytes`) | `(b: byte[], ignorarError: boolean): string` | `fromBytes(b, true)` |
| 285 | `__readFileI` (alias público: `readFile`) | `(path: string, ignorarError: boolean): string` | `readFile(path, true)` |
| 286 | `__freadLineI` (y en `IO`) | `(st: byte[], ignorarError: boolean): string` | `TextFile.readLine(true)` |

La regla de Eduardo (22-oct): un string es un invariante; lo de fuera se DESCODIFICA al nacer la cadena. Con
`ignorarError` = `false`, como los de un argumento (120, 38 y 249): `EncodingError` (hija de `RuntimeError`, en `Core`)
con dónde, `<que>: UTF-8 mal formado en el byte N (0xNN)`. Con `true`, cada trozo malo pasa a U+FFFD («�», `EF BF BD`),
uno por «subparte máxima», como dice el estándar de Unicode (§3.9): `61 F1 6F` → `a�o`; `ED A0 80` (un sustituto en tres
bytes) → tres. La rutina, la misma en las dos VMs (`bpvm_internal.h` `utf8_paso`…, miVM `Utf8`), probada contra la tabla
3-7 del estándar con todas las secuencias de 1 a 3 bytes (`make test-utf8`, `Utf8Test`).

Son SOBRECARGAS de los de un argumento, no un valor por defecto: el valor por defecto lo pone quien llama, así que lo ya
compilado llamaría a otra firma; con la sobrecarga, los de siempre siguen como estaban (y en una clase, su clave de
enlace: `TextFile.readLine()` y `readStr(n)`, y `Net.Tcp.recvStr`, también son sobrecargas). Se escribe igual:
`fromBytes(b)` y `fromBytes(b, true)`. La prueba: `bpgenvm-c/samples/Utf8Descodifica.bp` (la red de paridad), con lo
que tiene que salir escrito a mano.

## 287–294 — el sonido  *(V8/H4, `#708`, 27-oct)*

| id | nombre | firma | uso |
|---|---|---|---|
| 287 | `__soundPlay` (y en `Sound`) | `(path: string)` | `Sound.play(path)` |
| 288 | `__soundStop` | `()` | `Sound.stop()` |
| 289 | `__soundIsPlaying` | `(): boolean` | `Sound.isPlaying()` |
| 290 | `__soundSetVolume` | `(v: integer)` | `Sound.setVolume(v)` |
| 291 | `__soundGetVolume` | `(): integer` | `Sound.getVolume()` |
| 292 | `__soundIsAvailable` | `(): boolean` | `Sound.isAvailable()` |
| 293 | `__soundGetPosition` | `(): integer` | `Sound.getPosition()` |
| 294 | `__soundGetLength` | `(): integer` | `Sound.getLength()` |

Un `.wav` SIEMPRE EN PARALELO al programa (Eduardo, 27-oct: «la reproducción de sonidos debería ser siempre en paralelo
al resto»): `play` lee la cabecera y vuelve; lo toca una tarea propia, a trozos de unos 50 ms. Uno a la vez: un `play`
con otro sonando lo corta. Lo común está en la HAL BP (`bpvm_sonido.c`), que da el error de una placa sin sonido con su
motivo; debajo, la parte de cada familia (el PC: `sonido_host.c`, winmm o en silencio con `BPVM_SONIDO=mudo`). En la
VM-Java, `Sonido.java` (`javax.sound`). Los textos de los errores, iguales en las dos: `Sound.play('<ruta>'): no se pudo
abrir`, `… no es un WAV`, `… formato no admitido (<lo que trae>); se admite PCM de 8 o 16 bits, mono o estereo, de 8000
a 48000 Hz`, `Sound.play: <el motivo de la placa>` y `Sound.setVolume: el volumen va de 0 a 100 (es <v>)`. El progreso
(293 y 294), en bytes del sonido sin la cabecera. Los sin valor (287, 288 y 290) dejan el 0 que el compilador descarta
tras una llamada, como todos. Las no públicas viven en `Core` sin `public` (su interfaz no cambia); las públicas, en
`Sound`. La prueba: `bpgenvm-c/samples/SonidoParalelo.bp` (la red de paridad, en silencio).

## 295 — el fin de línea  *(V8, `#755`, 31-oct)*

| id | nombre | firma | uso |
|---|---|---|---|
| 295 | `eol` (en `Core`, pública) | `(): string` | `eol()` |

El fin de línea de la plataforma: `"\r\n"` en Windows, `"\n"` en Linux y en los micros, como el `System.lineSeparator()`
de Java (Eduardo, 31-oct: «la 2 está bien»). Es también el salto que pone el `print`. Un `"\n"` es siempre LF y un
`"\r"`, CR: nadie los cambia. En la VM-Java, `System.lineSeparator()`; en la VM-C, `BPVM_EOL` (`bpvm_out.h`), y la
consola del PC escribe en binario y cierra cada línea con él. Pública: la interfaz de `Core` cambia, como con `#711`
(en MOD9 el enlace es por nombre: lo compilado antes sigue enlazando). La prueba: `bpgenvm-c/samples/Eol.bp` (la red
de paridad: el stdout, byte a byte).

---

## Los que no tienen fila aquí  *(el mapa por rangos)*

Los de los periféricos, la GUI y algunos sueltos no tienen todavía su fila en este catálogo. Su firma exacta está en
`bpgenvm-c/src/builtin_ids.h` (generado de `Core.bp`, con la firma BP al lado), y lo que hacen, en la clase de la stdlib
que los usa (`referencia.html` §13–§15 y `gui.html`):

| ids | qué son | los usa |
|-----|---------|---------|
| 78–81 | `__gpioInit`/`Write`/`Read`/`Pull` | `Gpio` |
| 82–84, 86–89 | `__i2cInit`/`Write`/`Read`; `__spiInit`/`Write`/`Read`/`Transfer` | `I2c`, `Spi` |
| 85 | `newIntArray` | la raíz (`Core`) |
| 90–93 | `__uartInit`/`Write`/`Read`/`Available` | `Uart` |
| 94–98, 99–103 | el contador de pulsos y el PWM | `Pulse`, `Pwm` |
| 104–108, 111, 123, 201–205, 208–209, 233 | la identidad y el estado del micro: id, placa, temperatura, reloj de la CPU, `uptime`, causa del reset, las migas (`setMark`…), arranques, contadores, el micro | `Machine` |
| 109–110 | `sleepSec`, `sleepUs` | la raíz |
| 112–113 | el reloj (`__rtcNowSec`/`SetNowSec`) | `Rtc` |
| 114–115 | `__adcInitChannel`/`ReadChannel` | `Adc` |
| 116–118 | `__wdtEnable`/`Feed`/`Disable` | `Wdt` |
| 121–122 | `heapFrag`, `heapMap` (sólo miVM) | la raíz |
| 124–125 | `__npInit`/`Show` | `Neopixel` |
| 131–134 | `__tcpConnect`/`Send`/`Recv`/`Close` (sólo con backend en el PC) | `Net` |
| 135–199, 206–207, 210, 214–220, 222–223, 234–239 | la GUI: widgets, eventos, fuentes, rotación, `Chart`, los formularios, `Gui.shot` | `Gui` |
| 200 | `eval` | la raíz |
| 211–213 | el programa en ejecución (`mainModule`, `mainModulePath`, `projectPath`) | `App` |
| 221, 232 | `__eventRaise`, `__runArg` | sólo el compilador |
| 224–227 | recorrer la zona de packs y lo que lleva cada uno | `Packs` |
| 228–231 | `clamp`, `wrap`, `hypot`, `remap` | `Math` |

---

## Próximos builtins propuestos (no asignados todavía)

- **`__threadCurrent()` → Thread**: devuelve el Thread BP del thread
  actual. Útil para identificación de logs.
- **`__threadId(t: Thread)` → int**: tid de un Thread (también para
  logs).

(Los sockets TCP ya existen: `tcpConnect`…`tcpClose`, ids 131..134, con backend
sólo en el PC; el de las placas llega con el WiFi, V8.)

Estos van con `id` consecutivo cuando se implementen, **siempre al
final** del enum.
