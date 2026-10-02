# BPVM Wire Protocol v1

> **Contrato entre cliente y servidor.** Cualquier implementación que
> cumpla este documento es interoperable. Sirve para que el `BpvmClient`
> del IDE hable contra una VM Java local, un firmware Pico por USB
> CDC, o un firmware Pico por TCP/WiFi — con el mismo código en el
> cliente.
>
> Decisión arquitectónica: **un solo socket, JSON-RPC con bulk
> binario inline** (Opción C de la conversación de diseño). Razones
> y alternativas descartadas en `docs/PROJECT_ROADMAP.md` H1.

*Versión 1.0. Conversación entre Eduardo y Claude Opus 4.7,
24-mayo-2026.*

---

## 1. Visión general

El protocolo permite al IDE:

- **Operar el filesystem remoto** (LIST, GET, PUT, DEL, MKDIR, ...).
- **Ejecutar programas BP** y recibir su output en streaming.
- **Recibir `IO.prompt()`** del programa y devolver la respuesta.
- **Debuggear**: breakpoints, step, eval, inspect.
- **Consultar metadata** del dispositivo (versión, ID, freq, uptime).

Servidor puede ser:
- VM Java en modo daemon (`java -jar bpvm.jar --listen=7332`).
- Firmware Pico sobre USB CDC.
- Firmware Pico sobre TCP/WiFi (futuro #145).

Cliente: el IDE BpIde, vía la abstracción `BpvmClient` con `Transport`
intercambiable.

---

## 2. Framing del wire

### 2.1 Mensajes JSON line-delimited

Cada mensaje es **un objeto JSON en una sola línea de texto**, terminado
con `\n` (0x0A). Encoding **UTF-8**.

```
{"type":"HELLO","id":1,"protoVersion":1}\n
```

El parser puede leer hasta encontrar `\n` y feed eso al JSON parser.
Los `\n` dentro de strings JSON van escapados como `\\n` — el wire
nunca contiene `\n` literal dentro de un mensaje.

### 2.2 Bulk binario inline

Algunos mensajes (PUT request, GET reply) llevan un **payload binario
adjunto**. La regla:

1. El mensaje JSON incluye un campo `bulk` con el número de bytes que
   siguen.
2. Inmediatamente después del `\n` del JSON, vienen **exactamente `bulk`
   bytes raw** (sin separador, sin encoding).
3. Tras esos bytes, el wire vuelve a modo "siguiente mensaje JSON".

Ejemplo PUT de un .mod de 3519 bytes:

```
{"type":"PUT","id":42,"path":"/app/Hello.mod","bulk":3519}\n
<3519 bytes raw del .mod, sin separador>
{"type":"HELLO","id":43,"protoVersion":1}\n   ← siguiente mensaje
```

El receptor debe leer el JSON primero, ver `bulk`, leer ese número
exacto de bytes, y solo entonces volver al modo line-delimited.

**Bulk siempre va bytes raw**, nunca base64. Razones: eficiencia (USB
CDC tiene ancho limitado) y simplicidad del parser embedded.

---

## 3. Estructura común de mensajes

Todo mensaje JSON contiene como mínimo:

| Campo | Tipo | Obligatorio | Descripción |
|---|---|---|---|
| `type` | string | sí | Identifica el mensaje (`HELLO`, `RUN`, `OUTPUT`, ...). UPPERCASE. |
| `id` | integer | depende | Correlación request/reply. Ver §4. |

Los demás campos son específicos de cada `type`.

**IDs**: enteros monotónicos asignados por el cliente. El servidor
responde con el mismo `id`. No tienen significado más allá de la
correlación. Pueden reciclarse tras varios miles (mantenibles en
int32).

---

## 4. Modelo request / reply / event

Tres categorías de mensaje:

### 4.1 Request (client → server)

Lleva `id`. Espera una **reply síncrona** con el mismo `id`.

```json
{"type":"LIST","id":7,"path":"/lib/"}
```

### 4.2 Reply (server → client)

Lleva el `id` correlacionado del request. Devuelve datos o error.

```json
{"type":"LIST_REPLY","id":7,"entries":[
  {"name":"Math.mod","size":2103,"isDir":false},
  {"name":"Gpio.mod","size":1844,"isDir":false}
]}
```

Para errores en respuesta a un request:

```json
{"type":"ERROR","id":7,"code":"NOT_FOUND","message":"path /lib/x not found"}
```

### 4.3 Event (server → client, sin id)

Notificaciones asíncronas no solicitadas. **No llevan `id`**.

```json
{"type":"OUTPUT","session":3,"data":"hola mundo\n"}
{"type":"BP_HIT","session":3,"bpId":1,"frame":{...}}
{"type":"EXITED","session":3,"status":"OK"}
```

### 4.4 Reglas de orden

- El cliente puede tener **múltiples requests in-flight** simultáneos
  (pipelining). Las replies pueden venir en cualquier orden.
- Los eventos pueden intercalarse libremente entre replies. Solo
  garantizan **orden por sesión** (todos los `OUTPUT` de la sesión 3
  llegan en el orden en que se generaron).

---

## 5. Versionado y capabilities

### 5.1 HELLO obligatorio al conectar

El cliente DEBE enviar `HELLO` como primer mensaje. El servidor
responde con `HELLO_REPLY` antes de aceptar ningún otro mensaje.

**Cliente**:
```json
{"type":"HELLO","id":1,"protoVersion":1,"clientName":"BpIde","clientBuild":"1.0.0"}
```

**Servidor**:
```json
{"type":"HELLO_REPLY","id":1,
 "protoVersion":1,
 "serverName":"bpvm-pico",
 "serverBuild":"May 24 2026 12:23:07",
 "capabilities":["FILES","TERMINAL","DEBUG","META"]}
```

`putChunk` y `putWindow` (V7/H8 `#495`, 1-oct; opcionales): el trozo, en bytes, y la ventana —cuántos `PUT_DATA` sin
acuse— con que la placa quiere recibir la subida por trozos (§6.2). Los decide su búfer de comunicaciones: 512 B y 4 en
las cuatro familias. Sin ellos (un firmware anterior, la VM-Java), el cliente sube como siempre: trozos de 8 KB y un
acuse por trozo.

### 5.2 Negociación de versiones

- Si `protoVersion` del cliente > del servidor: servidor responde con
  su versión real. Cliente decide si baja o desconecta.
- Si `protoVersion` del cliente < del servidor: servidor habla en la
  versión pedida. Servidores nuevos retrocompatibles con clientes
  viejos.

### 5.3 Capabilities

Lista de strings que el servidor declara soportar. Permite a clientes
adaptar la UI (esconder pestaña Debug si no hay capability `DEBUG`,
por ejemplo).

Categorías actuales: `FILES`, `TERMINAL`, `DEBUG`, `META`; y `PACKS` (la zona de packs: los `PACK_*`,
comunes en `bpvm_bmgr_wire.c`) y `BOOTSEL` (§6.1, sólo la Pico).

Quién declara qué (30-sep; hasta entonces la Pico y la ESP32 callaban `PACKS`, y la ESP32 `DEBUG`, aunque los
tenían):

| servidor | `META` `FILES` `TERMINAL` | `DEBUG` | `PACKS` | `BOOTSEL` |
|---|---|---|---|---|
| miVM (`DebugServer`) | sí | sí | — (el PC no tiene zona de packs) | — |
| Pico (RP2350) | sí | sí | sí | sí |
| ESP32 (todas) | sí | sí | sí | — |
| STM32 | sí | — (su wire no tiene el depurador) | sí | — |

Futuras: `BLE`, `PIO`, `AOT`, ...

---

## 6. Catálogo de mensajes

### 6.1 Familia META

#### `HELLO` (request)

Ver §5.1. **Obligatorio como primer mensaje.**

#### `INFO` (request)

Lee metadata del dispositivo (lo que hoy hace `Pico.boardName()` etc).

Request: `{"type":"INFO","id":N}`

Reply:
```json
{"type":"INFO_REPLY","id":N,
 "uniqueId":"164BCBA4363DA887",
 "boardName":"pico2",
 "arch":40,
 "aotReceta":"ARMV8M_MAIN_SOFTFP",
 "aotFlagsCrc":100061591,
 "mdnAbi":9,
 "cpuFreqHz":150000000,
 "uptimeMs":12345,
 "tempC":43.5,
 "fsTotalBytes":131072,
 "fsUsedBytes":18432}
```

Los campos **comunes** los pone `src/bpvm_repl.c` a partir de la struct que
rellena cada familia (`bpvm_repl_info_t`); lo propio de una familia entra por
`info_extra`. Cuatro de ellos describen **el código nativo que esta imagen puede
ejecutar**, y el IDE los usa para compilar a la ISA correcta y no subir un `.mdn`
que la placa rechazaría:

| campo | quién lo sabe | qué es |
|---|---|---|
| `arch` | la familia (`bpvm_mdn_host_arch()`) | `e_machine` del ELF: 40 ARM · 243 RISC-V · 94 Xtensa · 0 = sin nativo |
| `aotReceta` | el común, compile-time | **V7/H2 (`#441`)**: la receta de `bpvm_aot_recetas.h` que este firmware exige (`ARMV8M_MAIN_SOFTFP`, `RV32IMAFC_ILP32F`, `RV32IMC_ILP32`); `""` donde no hay (PC, Xtensa) |
| `aotFlagsCrc` | el común, compile-time | CRC32 IEEE del literal de esa receta, **sin signo** (0 sin receta); es lo que la cabecera v2 del `.mdn` lleva en `flags_crc` |
| `mdnAbi` | el común, compile-time | **V7/H5 (27-sep)**: la ABI de helpers que habla este firmware (`MDN_ABI_VERSION` de `mdn_format.h`; 0 sin receta). Un `.mdn` de otra la rechaza el cargador y el programa corre interpretado: con ella el IDE lo dice en vez de subirlo (así se tapó `#504` tres semanas), y la tanda explica un «SIN NATIVO» |

Un firmware de antes de V7/H2 no manda `aotReceta`/`aotFlagsCrc`, ni uno de antes del 27-sep `mdnAbi`: el IDE lee
por nombre y distingue «ausente» (firmware viejo: no compara) de «vacío» o 0 (sin receta).

**El bloque de RAM de los motores nativos** (V7/H8 `#602`, 1-oct), el mismo en las cuatro familias —era de la Pico, como
`sqlite*`—:

| campo | qué es |
|---|---|
| `packRamBase`, `packRamBytes` | dónde y cuánto (0 y 0 = no hay) |
| `packRamStatus` | el motivo de la regla común (`bpvm_packram_region`): `off` (no se pidió), `ok`, `low` (por debajo del mínimo de `SQLite=`), `nofit` (no cabe dejando a la VM su suelo) |
| `packRamKey`, `packRamAsked` | la clave del ENV que lo pidió (`packRam`, `SQLite` o `""`) y lo que pedía, en su unidad (KB, MB) |
| `packRamMinMb` | el mínimo de `SQLite=`, para que el remedio de un `low` lo diga la placa y no un número copiado en el IDE |

Una Pico con firmware anterior manda `sqliteBase`/`sqliteBytes`/`sqliteStatus`/`sqliteAskedMb`/`sqliteMinMb` (sólo
contaban la reserva en PSRAM): el IDE lee los dos.

`ioStackFree` (V7/H8 `#495`, paso 4): lo que le sobró de pila al hilo `io` en el último RUN, en bytes (la marca de
agua); -1 si no ha habido RUN o la plataforma no lo sabe (el PC). Desde el paso 4, `io` corre también el `GET` y el
`LIST` troceados durante un RUN: se publica para mirarlo, no suponerlo. Con 4 KB sobraban 332 B en la S3 y 700 en la
P4, y en la familia ESP32 pasó a 6 KB (2316 y 2748 libres); en la Pico y la STM32, 4 KB con 1208 libres (1-oct).

#### `TIME` (request)

Sincroniza el RTC del dispositivo con el wall clock del IDE.

Request: `{"type":"TIME","id":N,"epochSec":1779617006}`

Reply: `{"type":"TIME_REPLY","id":N}`

#### `PING` (request)

Keep-alive. Útil para detectar conexiones muertas.

Request: `{"type":"PING","id":N}`

Reply: `{"type":"PONG","id":N}`

#### `RESET` (request)

Reboot del firmware. El servidor responde **antes** de rebootear; la
conexión se cae poco después.

Request: `{"type":"RESET","id":N}`

Reply: `{"type":"RESET_REPLY","id":N}` + conexión cerrada en <500ms.

#### `BOOTSEL` (request, solo Pico)

Entra en modo bootloader. Disponible solo si capability `BOOTSEL`.

Request: `{"type":"BOOTSEL","id":N}`

Reply: `{"type":"BOOTSEL_REPLY","id":N}` + conexión cerrada.

---

### 6.2 Familia FILES

#### `LIST` (request)

Lista contenido de un directorio. En FS plano con `/`, lista todas
las entradas cuyo path empieza por el prefijo.

Request: `{"type":"LIST","id":N,"path":"/lib/"}`

Reply:
```json
{"type":"LIST_REPLY","id":N,"entries":[
  {"name":"Math.mod","size":2103,"isDir":false,"mtime":1779617006},
  {"name":"Gpio.mod","size":1844,"isDir":false,"mtime":1779617006}
]}
```

`mtime` puede ser 0 si el FS no lo soporta (Pico actual no).

`mal` (V7/H8 `E1`, 1-oct; sólo si hay problema): por qué la VM NO cargaría el fichero —un `.mod`, `.mdn` o `.pack`
dañado o de un formato que no carga—, en prosa: lo que el IDE enseña, en rojo. Lo examina la VM con la regla de su
cargador, leyendo sólo la cabecera (`bpvm_examen.h`); el contenido no. La VM-Java sólo marca `.mod`, que es lo único
que carga. Por ejemplo: `"dañado: la cabecera dice 144 B y el fichero tiene 94"`, `"formato no válido: MOD5, …"`,
`"formato no válido para esta placa: código de otra arquitectura (65535; esta VM ejecuta la 40)"`.

**Troceado** (V7/H8 `#495`, paso 4; si el cliente lo pide con `"chunked":true`): las entradas llegan en líneas
`LIST_PART` acotadas —las de un directorio; uno grande, en varias— y al final `LIST_END` con `omitted`. La placa
recorre un directorio por vuelta de su lazo, así que entre parte y parte pueden ir otros mensajes (la salida de un
programa, los trozos de un `GET`). Un servidor que no lo conoce contesta el `LIST_REPLY` entero, y el cliente acepta
las dos formas. Uno a la vez: otro troceado con uno en curso, `BUSY`.

```
C→S {"type":"LIST","id":9,"path":"","chunked":true}
S→C {"type":"LIST_PART","id":9,"entries":[{"name":"/app/Hello.mod","size":3519,"crc":-1,"isDir":false,"mtime":0}]}
S→C {"type":"LIST_PART","id":9,"entries":[{"name":"/lib/Core.mod","size":20711,"crc":-1,"isDir":false,"mtime":0},…]}
S→C {"type":"LIST_END","id":9,"omitted":0}
```

#### `STAT` (request)

Metadata de un fichero individual — por **ruta**, o (`#466`) por **nombre de módulo**.

Request: `{"type":"STAT","id":N,"path":"/app/Hello.mod"}`

Reply: `{"type":"STAT_REPLY","id":N,"size":3519,"magic":"MOD9","gen":7,"ifaceCrc":3166772865,"isDir":false,"mtime":0}`

Error si no existe: `{"type":"ERROR","id":N,"code":"NOT_FOUND",...}`

- `crc` sólo si se pide (`"crc":true`, #398): calcularlo para todo el FS en cada listado era el
  99 % del refresco del árbol. Es un entero **sin signo** (0…4294967295; `#466`: antes salía
  negativo con el bit 31 puesto, porque `long` es de 32 bits en las placas, y el IDE lo daba por
  distinto). -1 = no se pudo calcular.
- `magic` (`#466`): los 4 primeros bytes cuando parecen un módulo (`"MOD8"`, `"MOD9"`): su
  **formato**. Un fichero que no empieza por `MOD` no lo lleva.
- `gen` e `ifaceCrc` (V7/H9 `#624`, desde MOD8): la **versión** del módulo —la «version» de su
  proyecto, de la cabecera (+36)— y el CRC-32 de su interfaz (+40, sin signo). Hasta `#624` contaba
  como versión el `magic`, y con todo en MOD9 sólo quedaba el CRC para decidir. Un MOD7 o anterior
  no los lleva.

**Por nombre** (`#466`) — *«¿tienes `Math.mod`, donde sea?»*, la pregunta que el IDE hace por cada
dependencia antes de subirla:

Request: `{"type":"STAT","id":N,"name":"Math.mod","base":"/app/proj","crc":true}`

Reply: `{"type":"STAT_REPLY","id":N,"path":"/lib/Math.mod","size":1708,"magic":"MOD9","gen":7,"ifaceCrc":2768498097,"crc":123456,"isDir":false,"mtime":0}`

El device lo resuelve con **su propio resolvedor**, el del RUN (`base` → literal → `/app` → `/lib`
→ la zona de packs; V7/H2 `#493`: sin `/sys`), para que el IDE no lleve una copia del orden de
búsqueda. `base` es opcional: la carpeta del proyecto que se va a ejecutar. `path` dice dónde lo
tiene — si hay que reemplazarlo, se reemplaza ahí. Si sólo lo tiene un **pack grabado**, `path` es
la ruta cualificada `pack:<Pack>/Math.mod` (la placa lo ejecuta XIP desde ahí; `magic` y `crc`
salen igual): la zona ya no contesta al `stat` de `/app/X.mod` por el nombre, así que un `path`
de `/app` o `/lib` es un fichero de verdad. **La regla con esta respuesta, para una LIBRERÍA**
(V7/H9 `#624`, Eduardo, la de Java; `ReglaDeSubida` en `lexer-java`, y la tanda igual): se sube si no
está (`NOT_FOUND`), si su `magic` es otro (formato incompatible), o si su `gen` es anterior a la
mínima —la del `.mod` contra el que se compiló el programa—; con el mismo formato y una versión igual
o superior, la del device **vale** aunque el `crc` sea otro (otra compilación), y no se sube. Sin
`gen` (un firmware anterior a `#624`), la regla de antes: idéntico no se sube y distinto sí. Si hay
que subir, se sube donde está o, si está en un pack, a donde le toca (`/lib`), que va delante del
pack — un pack no se borra por un `DEL`. Un módulo del PROGRAMA se sube si cambia.

#### `GET` (request, reply con bulk)

Descarga un fichero. La reply lleva bulk con los bytes.

Request: `{"type":"GET","id":N,"path":"/app/Hello.mod"}`

Reply:
```
{"type":"GET_REPLY","id":N,"bulk":3519}\n
<3519 bytes raw>
```

**Troceado** (V7/H8 `#495`, paso 4; si el cliente lo pide con `"chunked":true`). Eduardo (28-sep): *«el get tiene que
trocearse y los paquetes de los diferentes comandos se pueden mezclar, tiene que ser el destinatario el que vaya
recogiendo los diferentes paquetes y los vaya agrupando»*.

```
C→S {"type":"GET","id":7,"path":"/app/datos.bin","chunked":true}
S→C {"type":"GET_PART","id":7,"off":0,"bulk":512}\n<512 bytes>
S→C {"type":"OUTPUT",…}                                  (lo que sea: entre trozo y trozo, el cable es de todos)
S→C {"type":"GET_PART","id":7,"off":512,"bulk":512}\n<512 bytes>
…
S→C {"type":"GET_END","id":7,"size":40000,"crc":3141592653}
```

- **Un trozo por vuelta** del lazo de la placa (o de su hilo `io`, durante un RUN), de hasta 512 B. Cada `GET_PART` es
  un mensaje entero —la línea y sus bytes, con el cerrojo del cable cogido una vez—; entre dos, cualquier otro.
- **Sin acuse**: recibe el cliente, que lee siempre; el control de flujo es el del cable (el NAK del USB, la ventana de
  TCP, la UART que espera). El cliente junta por `off`.
- **`GET_END`**: `size` es lo mandado y `crc` el CRC-32 de lo mandado (el de `STAT`); el cliente lo comprueba.
  `changed:true` si el fichero cambió de tamaño mientras se bajaba (lo escribe el programa) o se acabó antes: lo bajado
  puede no cuadrar. Lo que cambie sin cambiar de tamaño no se ve.
- **Un error a mitad**, `ERROR` con el id del `GET` (hoy, con el bulk anunciado, no había forma de decirlo).
- **Una descarga a la vez** (Eduardo: *«1 a 1»*): otra con una en curso, o con una subida a medias, `BUSY`; un `GET`
  entero con una troceada en curso, también. Un `LIST` sí puede ir con ella.
- **La cortan** —`ERROR` con su id, `ABORTED`— lo mismo que acaba una subida: un `HELLO`, un `RESET`, otra subida y los
  que escriben en el FS (`DEL`, `MKDIR`, `RMDIR`, `RENAME`, `FORMAT`, `SAVE`). Un `RUN`, no: sigue durante el programa.
  Un `HELLO` corta también un `LIST` troceado.
- Sin `chunked`, el `GET_REPLY` entero de siempre; un servidor que no lo conoce contesta así, y el cliente acepta las
  dos formas.

#### `PUT` (request con bulk)

Sube un fichero. El request lleva bulk con los bytes. Crea o
sobreescribe.

Request:
```
{"type":"PUT","id":N,"path":"/app/Hello.mod","bulk":3519}\n
<3519 bytes raw>
```

Reply: `{"type":"PUT_REPLY","id":N}`

#### `PUT_BEGIN` / `PUT_DATA` / `PUT_END` (la subida por trozos, `#294`)

Para lo que no cabe de una pieza (el IDE: más de 8 KB).

```
C→S {"type":"PUT_BEGIN","id":1,"path":"/app/Big.mod","size":40000}
S→C {"type":"PUT_BEGIN_REPLY","id":1,"received":0}
C→S {"type":"PUT_DATA","id":2,"bulk":512}\n<512 bytes>        (hasta putWindow sin acuse, §5.1)
S→C {"type":"PUT_DATA_REPLY","id":2,"received":512}
…
C→S {"type":"PUT_END","id":N,"size":40000}
S→C {"type":"PUT_END_REPLY","id":N,"size":40000}
```

- **La ventana** (V7/H8 `#495`): el cliente puede mandar hasta `putWindow` trozos de `putChunk` bytes sin esperar su
  acuse; los acuses llegan en orden.
- **El fichero, abierto toda la subida** (`#495`, paso 1): lo recibido se junta en un bloque de la flash y se escribe por
  bloques.
- **Una subida que no termina se BORRA** (Eduardo, 1-oct: *«si queda la versión anterior el usuario se va a pensar que
  tiene el nuevo»*). La acaban un `HELLO` (un cliente nuevo), un `RUN`, un `RESET`, otra subida (`PUT`, `PUT_BEGIN`), un
  verbo que escribe en el FS (`DEL`, `MKDIR`, `RMDIR`, `RENAME`, `FORMAT`, `SAVE`), un error o un `PUT_END` que no cuadra
  (`SIZE_MISMATCH`); los que sólo leen pueden ir entre medias. Lo mismo un `PUT` de una pieza cuyo bulk no llega
  entero o no se puede escribir.
- **Un bulk a medias** se da por perdido a los `WIRE_V1_BULK_ESTANCADO_MS` (3 s, en `bpvm_wire_v1.h`) —`FATAL`
  `bulk underrun`— y el cable vuelve a estar en sincronía en la línea siguiente.
- **El segundo intento es del cliente** (Eduardo, 1-oct: *«que lo haga el IDE»*): uno, desde `PUT_BEGIN`, y sólo si
  falla el cable (sin respuesta, un `FATAL`, `SIZE_MISMATCH`, `NO_SESSION`); antes, el cable callado 3,5 s en los dos
  sentidos y un `HELLO`. Si la placa dice que no con nombre (`NO_SPACE`, la ruta…), a la primera.

#### `DEL` (request)

Borra un fichero.

Request: `{"type":"DEL","id":N,"path":"/app/old.mod"}`

Reply: `{"type":"DEL_REPLY","id":N}`

#### `MKDIR` / `RMDIR` (request)

Crea / borra una "carpeta". En FS plano con `/`, MKDIR es
conceptualmente no-op (las carpetas existen al haber ficheros con
ese prefijo). RMDIR falla si hay ficheros con ese prefijo.

Request: `{"type":"MKDIR","id":N,"path":"/data/"}`

Reply: `{"type":"MKDIR_REPLY","id":N}`

#### `RENAME` (request)

Mueve/renombra.

Request: `{"type":"RENAME","id":N,"from":"/app/old.mod","to":"/app/new.mod"}`

Reply: `{"type":"RENAME_REPLY","id":N}`

#### `FORMAT` (request)

Borra todo el FS del usuario. La stdlib pre-instalada en `/lib/` se
re-instala al siguiente boot desde la imagen.

Request: `{"type":"FORMAT","id":N,"confirm":"YES"}`

Reply: `{"type":"FORMAT_REPLY","id":N}`

Si `confirm != "YES"`, error `MISSING_CONFIRM`.

#### `SAVE` (request, solo Pico)

Commit del FS RAM a flash. No aplica a VM Java (siempre persiste).

Request: `{"type":"SAVE","id":N}`

Reply: `{"type":"SAVE_REPLY","id":N,"durationMs":47}`

#### `DF` (request)

Espacio en el FS.

Request: `{"type":"DF","id":N}`

Reply: `{"type":"DF_REPLY","id":N,"totalBytes":131072,"usedBytes":18432,"freeBytes":112640,"fileCount":12}`

#### `LOG_DUMP` (request, solo Pico)

Volcado del log persistente del firmware (`log.c`).

Request: `{"type":"LOG_DUMP","id":N}`

Reply: `{"type":"LOG_DUMP_REPLY","id":N,"text":"...multilínea..."}`

---

### 6.3 Familia TERMINAL

#### `RUN` (request)

Arranca un programa. Devuelve `sessionId`.

Request: `{"type":"RUN","id":N,"path":"/app/Hello.mod","arg":"opcional"}`

⚠️ **`args:[]` NUNCA EXISTIÓ, y no podía existir.** Este documento lo prometió desde el principio,
pero **ninguna línea de código lo parseó jamás** (grep de `"args"` en `bpgenvm-c/src`, `miVM/src` y
`BpIde/src`: cero), y no era un olvido — el mini-parser JSON de las placas **no sabe leer arrays
anidados** (`include/json_min.h:14`), así que implementarlo obligaría a tocar el parser de las cinco
familias. Corregido el 6-sep con `#412`.

**`arg`** (V6/`#412`, opcional): el **argumento de ejecución** del programa, cadena **escalar**. Lo
recoge el builtin `__runArg` que el compilador emite en `__startup`, y llega a `Main` **siempre como
una cadena del heap**. Si no viene, manda el valor por defecto que declare el fuente:

```basic
public function Main(arg: string := "mi valor por defecto")
```

📌 **Y el autorun (`/sys/auto.txt`) NO pasa ninguno, a propósito** (Eduardo, 6-sep): el argumento es
para **probar** el programa con distintas opciones; una vez probado se fija el valor por defecto en
el fuente y la misión del autorun es sólo que arranque al encender el micro. O sea que **el valor por
defecto ES la configuración de despliegue**.

Reply: `{"type":"RUN_REPLY","id":N,"session":3}`

#### `KILL` (request)

Mata un programa en ejecución.

Request: `{"type":"KILL","id":N,"session":3}`

Reply: `{"type":"KILL_REPLY","id":N}`

#### Durante un RUN

Mientras corre un programa, la placa atiende `HELLO` (un IDE que se conecta con algo en marcha), `KILL` y `RESET`, el
ramo de depuración y —desde V7/H8 `#495`, paso 4— los de lectura que no monopolizan el cable: **`STAT` y el `GET` y el
`LIST` troceados**, cuyos trozos van entre la salida del programa. Lo demás contesta `BUSY` con el mismo texto en las
cuatro familias (*«ejecución en curso: sólo HELLO, KILL, RESET, STAT y GET/LIST troceados»*): los enteros (un `GET`
entero dejaría el cable sin salida ni `KILL` mientras dura), `LIST_DIR` (su foto del directorio sale de la zona de
rascar, la misma del `listDir` del programa) y los que escriben (escribir donde el programa lee es otra conversación).

#### `STDIN` (request, futuro)

Manda bytes al stdin del programa. **No implementado en v1.**

#### `OUTPUT` (event)

Stdout/stderr del programa. Se emite a medida que el programa
imprime. **Sin id** (es asíncrono).

Event: `{"type":"OUTPUT","session":3,"data":"hola mundo\n","stream":"stdout"}`

`stream` puede ser `"stdout"` o `"stderr"` (futuro). Por defecto
stdout.

#### `PROMPT_REQUEST` (event)

El programa BP llamó a `IO.prompt(spec)`. El IDE debe mostrar el
form y responder. **El programa está bloqueado hasta la respuesta.**

Event:
```json
{"type":"PROMPT_REQUEST","session":3,"promptId":1,
 "spec":"{title:'Nombre',fields:[{name:'x',type:'text'}]}"}
```

#### `PROMPT_RESPONSE` (request)

Respuesta al prompt. El cliente la envía como request normal con id;
el servidor confirma con reply.

Request:
```json
{"type":"PROMPT_RESPONSE","id":N,"session":3,"promptId":1,
 "values":"{x:'Eduardo'}"}
```

Reply: `{"type":"PROMPT_RESPONSE_REPLY","id":N}`

#### `EXITED` (event)

El programa terminó.

Event:
```json
{"type":"EXITED","session":3,"status":"OK","exitCode":0,
 "elapsedMs":123}
```

`status` ∈ `OK | RUNTIME_ERROR | LINK_ERROR | KILLED | INTERNAL_ERROR`.
Si `status=RUNTIME_ERROR`, también `errorMessage` y opcionalmente
`stackTrace`.

**El EXITED sale SIEMPRE** (V7, `#557`, 25-sep): si el `errorMessage` no cabe en el
buffer de la placa (1.024 B en la Pico y el STM32, 2.048 en la ESP32; el error de
enlace llega a 1.536), va **recortado** en un límite de carácter y terminado en
`...`. Antes, un mensaje que no cabía se llevaba el EXITED entero por delante, y el
IDE se quedaba esperando el final de un programa que ya había terminado. El micro
simulado lo manda siempre entero.

**`nativos`** (V7/H5, 25-sep): los thunks de código nativo (AOT, `.mdn`) que se
registraron en ESTE RUN —0 si corrió todo interpretado—. Existe porque un
programa con funciones `native` da los MISMOS números interpretado que nativo:
la tanda de placas (`tools/tanda.py`, pruebas `"aot"`) comprueba con él que se
usó el código compilado. Sin receta AOT (el micro simulado, la S3) sale 0. Una
imagen de antes no lo manda; un cliente que no lo conozca lo ignora.

**`LINK_ERROR`** (el programa no llegó a arrancar: falló el enlace de sus
módulos): `errorMessage` lleva el texto del cargador, **byte-idéntico en las dos
VMs** y el mismo que el CLI imprime entre `=== ERROR DE LINK: … ===`. Los textos:
los de la norma de versiones (`MOD_FORMAT.md` §3.1: «es mas antiguo que…», «no es
la interfaz con la que se compilo…», «es mas reciente que … y extiende una clase
suya»), el «lib 'X' presente pero no exporta 'S' (la usa 'M'; version vieja?)» de
un símbolo ausente, y en MOD9 los rechazos **E0–E10** del enlace tardío
(`MOD_FORMAT.md` §13.7): siempre nombran el módulo, la clase y el miembro, y
terminan en «recompila 'X'». El IDE los muestra tal cual.

---

### 6.4 Familia DEBUG

Todos los mensajes de debug operan sobre una sesión existente.
**El cliente debe lanzar RUN primero**, después puede setar BPs y
debuggear.

#### `SET_BP` (request)

Pone un breakpoint en `file:line`. Si el programa ya está corriendo,
es efectivo desde el momento en que se procesa.

Request: `{"type":"SET_BP","id":N,"session":3,"file":"Hello.bp","line":12}`

Reply: `{"type":"SET_BP_REPLY","id":N,"bpId":1}`

#### `CLR_BP` (request)

Request: `{"type":"CLR_BP","id":N,"session":3,"bpId":1}`

Reply: `{"type":"CLR_BP_REPLY","id":N}`

#### `LIST_BP` (request)

Request: `{"type":"LIST_BP","id":N,"session":3}`

Reply:
```json
{"type":"LIST_BP_REPLY","id":N,"breakpoints":[
  {"bpId":1,"file":"Hello.bp","line":12,"enabled":true,"hits":3}
]}
```

#### `PAUSE` (request)

Pausa el programa donde esté. Trigger un `BP_HIT` con `bpId:0` (BP
sintético).

Request: `{"type":"PAUSE","id":N,"session":3}`

Reply: `{"type":"PAUSE_REPLY","id":N}`

#### `CONTINUE` (request)

Reanuda hasta el siguiente BP o final del programa.

Request: `{"type":"CONTINUE","id":N,"session":3}`

Reply: `{"type":"CONTINUE_REPLY","id":N}`

#### `STEP` (request)

Step into/over/out.

Request: `{"type":"STEP","id":N,"session":3,"mode":"into"}`

`mode` ∈ `into | over | out`.

Reply: `{"type":"STEP_REPLY","id":N}`

Después del STEP, llega un evento `STEP_DONE` cuando se completa.

#### `EVAL` (request)

Evalúa una expresión BP en el contexto del frame top.

Request: `{"type":"EVAL","id":N,"session":3,"expr":"x + 1"}`

Reply: `{"type":"EVAL_REPLY","id":N,"result":"42","type":"integer"}`

Si la expresión falla: `ERROR id=N code="EVAL_ERROR" message="..."`.

#### `STACK` (request)

Lista de frames del thread principal del programa pausado.

Request: `{"type":"STACK","id":N,"session":3}`

Reply:
```json
{"type":"STACK_REPLY","id":N,"frames":[
  {"frame":0,"function":"factorial","file":"Math.bp","line":12,
   "locals":[{"name":"n","value":"5","type":"integer"}]},
  {"frame":1,"function":"main","file":"Hello.bp","line":7,
   "locals":[{"name":"x","value":"5","type":"integer"}]}
]}
```

#### `LOCALS` (request)

Variables de un frame concreto. Equivalente a `STACK_REPLY.frames[i].locals`
pero individual.

Request: `{"type":"LOCALS","id":N,"session":3,"frame":0}`

Reply: `{"type":"LOCALS_REPLY","id":N,"locals":[...]}`

#### `INSPECT` (request)

Expand recursivo de un objeto/array referenciado por handle.

Request: `{"type":"INSPECT","id":N,"session":3,"ref":12345,"depth":2}`

Reply:
```json
{"type":"INSPECT_REPLY","id":N,
 "class":"Pulse.Counter",
 "fields":[
   {"name":"pin","value":"13","type":"integer"},
   {"name":"id","value":"6","type":"integer"}
 ]}
```

#### `BP_HIT` (event)

El programa llegó a un breakpoint o fue pausado.

Event:
```json
{"type":"BP_HIT","session":3,"bpId":1,
 "frame":{"function":"main","file":"Hello.bp","line":12}}
```

`bpId:0` significa "pausado manualmente" (sin BP real).

#### `STEP_DONE` (event)

El step solicitado se completó. Mismo formato que `BP_HIT` pero sin
`bpId`.

Event:
```json
{"type":"STEP_DONE","session":3,
 "frame":{"function":"main","file":"Hello.bp","line":13}}
```

---

## 7. Flujos típicos

### 7.1 Conexión inicial

```
C → S: HELLO id=1 protoVersion=1
S → C: HELLO_REPLY id=1 protoVersion=1 serverName="bpvm-pico"
        capabilities=["FILES","TERMINAL","DEBUG","META","BOOTSEL"]
C → S: TIME id=2 epochSec=1779617006
S → C: TIME_REPLY id=2
C → S: INFO id=3
S → C: INFO_REPLY id=3 boardName="pico2" cpuFreqHz=150000000 ...
```

### 7.2 Upload + Run

```
C → S: LIST id=10 path="/app/"
S → C: LIST_REPLY id=10 entries=[{"name":"Hello.mod","size":3519,...}]
       ← detecta que size coincide; salta el PUT
C → S: RUN id=11 path="/app/Hello.mod" arg="opcional"
S → C: RUN_REPLY id=11 session=3
S → C: OUTPUT session=3 data="hola\n"
S → C: OUTPUT session=3 data="mundo\n"
S → C: EXITED session=3 status=OK exitCode=0 elapsedMs=42
```

### 7.3 Upload con bulk

```
C → S: PUT id=20 path="/app/New.mod" bulk=2048
       <2048 bytes raw>
S → C: PUT_REPLY id=20
```

### 7.4 Debug session

```
C → S: SET_BP id=30 session=3 file="Hello.bp" line=12
S → C: SET_BP_REPLY id=30 bpId=1
C → S: RUN id=31 path="/app/Hello.mod"
S → C: RUN_REPLY id=31 session=3
S → C: OUTPUT session=3 data="starting...\n"
S → C: BP_HIT session=3 bpId=1 frame={...line=12...}
C → S: STACK id=32 session=3
S → C: STACK_REPLY id=32 frames=[...]
C → S: EVAL id=33 session=3 expr="n + 1"
S → C: EVAL_REPLY id=33 result="6" type="integer"
C → S: STEP id=34 session=3 mode="over"
S → C: STEP_REPLY id=34
S → C: STEP_DONE session=3 frame={...line=13...}
C → S: CONTINUE id=35 session=3
S → C: CONTINUE_REPLY id=35
S → C: OUTPUT session=3 data="done\n"
S → C: EXITED session=3 status=OK exitCode=0
```

### 7.5 Prompt durante ejecución

```
C → S: RUN id=40 path="/app/AskName.mod"
S → C: RUN_REPLY id=40 session=4
S → C: OUTPUT session=4 data="¿Cómo te llamas?\n"
S → C: PROMPT_REQUEST session=4 promptId=1 spec="..."
       ← el cliente muestra el dialog
C → S: PROMPT_RESPONSE id=41 session=4 promptId=1 values="{x:'Eduardo'}"
S → C: PROMPT_RESPONSE_REPLY id=41
S → C: OUTPUT session=4 data="Hola Eduardo\n"
S → C: EXITED session=4 status=OK
```

---

## 8. Reglas de error

### 8.1 Códigos de error estándar

Los `code` de `ERROR` replies son strings estables:

| Código | Significado |
|---|---|
| `NOT_FOUND` | Path no existe. |
| `EXISTS` | Recurso ya existe (cuando PUT debe ser exclusivo). |
| `INVALID_PATH` | Path mal formado. |
| `INVALID_PARAM` | Param fuera de rango o de tipo incorrecto. |
| `NO_SPACE` | FS lleno. |
| `BUSY` | Recurso ocupado (RUN sobre sesión activa, multi-cliente). |
| `MISSING_CONFIRM` | Operación destructiva sin `confirm:"YES"`. |
| `EVAL_ERROR` | Expresión BP no compilable o evaluación falló. |
| `NO_SESSION` | sessionId no existe o ya terminó. |
| `UNSUPPORTED` | Operación no soportada por este servidor (ver capabilities). |
| `PROTOCOL_ERROR` | Mensaje mal formado o secuencia inválida. |
| `INTERNAL_ERROR` | Bug del servidor. Incluye `message` para diagnóstico. |
| `ABORTED` | Una transferencia troceada (`GET`, `LIST`) cortada a medias: llegó un verbo que no convive con ella (§6.2). |

### 8.2 Mensajes desconocidos

Si el servidor recibe un `type` que no reconoce:

```json
{"type":"ERROR","id":<id del request>,"code":"UNSUPPORTED",
 "message":"unknown type: FROBNICATE"}
```

Si el mensaje no llevaba `id`, se ignora silenciosamente (era un
"event" no entendido, no hay a quién responder).

### 8.3 Mensajes mal formados

JSON inválido o estructura imposible → el servidor envía:

```json
{"type":"FATAL","code":"PROTOCOL_ERROR","message":"invalid JSON at byte 42"}
```

Y **cierra la conexión**. El cliente puede reconectar.

### 8.4 ID duplicado

Si el cliente envía dos requests con el mismo `id` antes de recibir
la primera reply: el servidor procesa el primero, ignora el segundo
silenciosamente y loguea. (No vale la pena romper la conexión por
esto.)

### 8.5 Bulk size mismatch

Si el bulk declarado son N bytes y al leer solo llegan M < N antes
de timeout: `FATAL` + cierre de conexión.

### 8.6 Conexión muerta

Si el cliente no oye nada en >30 s y no hay nada en vuelo, debe
enviar `PING`. Si no llega `PONG` en 5 s, considerar conexión muerta
y reconectar.

---

## 9. Restricciones de v1

Cosas que la v1 del protocolo NO soporta. Aceptables; cabe en v2.

- **Múltiples sesiones activas a la vez**. v1: una sesión RUN
  simultánea por servidor. Lanzar RUN cuando ya hay sesión activa →
  `ERROR code=BUSY`.
- **Múltiples clientes conectados**. v1: un cliente a la vez. Si llega
  un segundo, el servidor lo rechaza con error (o cierra la primera
  según política).
- **Streaming bidireccional** (`STDIN` interactivo durante RUN). El
  modelo actual es: el programa imprime, espera prompts, no lee
  stdin libre.
- **Compresión del wire**. Bulk va raw; mensajes JSON van uncompressed.
  Suficiente a las velocidades actuales (USB CDC 115200 ≈ 11 KB/s,
  TCP/WiFi ≈ MB/s).
- **TLS/autenticación**. v1 asume LAN doméstica o USB. Sin auth.

---

## 10. Decisiones para v2

Anotadas aquí para no perderlas:

- **Multi-sesión**: cada `RUN` lanza una sesión independiente. Output
  se distingue por `session`. Ya está parcialmente hecho — solo
  hay que quitar la restricción de "una a la vez".
- **Multi-cliente**: varios IDEs hablando al mismo servidor. Require
  mutex de FS y de sesiones.
- **STDIN streaming**: para REPLs interactivos tipo
  `>>> 2 + 2 → 4`.
- **Eventos de telemetría**: stream periódico de métricas del
  dispositivo (mem libre, temp, GC stats).
- **Compresión opcional**: capability `COMPRESS` con LZ4 para bulks
  grandes en conexiones lentas (USB CDC).
- **Subscription/unsubscription**: el cliente declara qué eventos
  quiere recibir (e.g. solo OUTPUT, no telemetría).
- **Versionado richer**: capabilities con versión (`DEBUG:2` para
  debug v2 con time-travel, etc.).

---

## 11. Apéndice: tabla resumen de mensajes

| Type | Dir | Reply | Familia | Bulk |
|---|---|---|---|---|
| `HELLO` | C→S | `HELLO_REPLY` | META | – |
| `INFO` | C→S | `INFO_REPLY` | META | – |
| `TIME` | C→S | `TIME_REPLY` | META | – |
| `PING` | C→S | `PONG` | META | – |
| `RESET` | C→S | `RESET_REPLY` | META | – |
| `BOOTSEL` | C→S | `BOOTSEL_REPLY` | META | – |
| `LIST` | C→S | `LIST_REPLY` | FILES | – |
| `STAT` | C→S | `STAT_REPLY` | FILES | – |
| `GET` | C→S | `GET_REPLY` | FILES | reply lleva bulk |
| `GET_PART` / `GET_END` | S→C | trozos de un `GET` troceado | FILES | `GET_PART` lleva bulk (`#495`) |
| `LIST_PART` / `LIST_END` | S→C | trozos de un `LIST` troceado | FILES | `#495` |
| `PUT` | C→S | `PUT_REPLY` | FILES | request lleva bulk |
| `DEL` | C→S | `DEL_REPLY` | FILES | – |
| `MKDIR` | C→S | `MKDIR_REPLY` | FILES | – |
| `RMDIR` | C→S | `RMDIR_REPLY` | FILES | – |
| `RENAME` | C→S | `RENAME_REPLY` | FILES | – |
| `FORMAT` | C→S | `FORMAT_REPLY` | FILES | – |
| `SAVE` | C→S | `SAVE_REPLY` | FILES | – |
| `DF` | C→S | `DF_REPLY` | FILES | – |
| `LOG_DUMP` | C→S | `LOG_DUMP_REPLY` | FILES | – |
| `RUN` | C→S | `RUN_REPLY` | TERMINAL | – |
| `KILL` | C→S | `KILL_REPLY` | TERMINAL | – |
| `OUTPUT` | S→C | – (event) | TERMINAL | – |
| `PROMPT_REQUEST` | S→C | – (event) | TERMINAL | – |
| `PROMPT_RESPONSE` | C→S | `PROMPT_RESPONSE_REPLY` | TERMINAL | – |
| `EXITED` | S→C | – (event) | TERMINAL | – |
| `SET_BP` | C→S | `SET_BP_REPLY` | DEBUG | – |
| `CLR_BP` | C→S | `CLR_BP_REPLY` | DEBUG | – |
| `LIST_BP` | C→S | `LIST_BP_REPLY` | DEBUG | – |
| `PAUSE` | C→S | `PAUSE_REPLY` | DEBUG | – |
| `CONTINUE` | C→S | `CONTINUE_REPLY` | DEBUG | – |
| `STEP` | C→S | `STEP_REPLY` | DEBUG | – |
| `EVAL` | C→S | `EVAL_REPLY` | DEBUG | – |
| `STACK` | C→S | `STACK_REPLY` | DEBUG | – |
| `LOCALS` | C→S | `LOCALS_REPLY` | DEBUG | – |
| `INSPECT` | C→S | `INSPECT_REPLY` | DEBUG | – |
| `BP_HIT` | S→C | – (event) | DEBUG | – |
| `STEP_DONE` | S→C | – (event) | DEBUG | – |
| `ERROR` | S→C | reply a request | – | – |
| `FATAL` | S→C | – (event, cierra conn) | – | – |

**Total: 38 tipos de mensaje** (incluyendo replies y eventos).

---

*Versión 1.0. Conversación entre Eduardo y Claude Opus 4.7,
24-mayo-2026. Documento vivo: actualizar cuando se modifique un
mensaje o se añada uno nuevo. Cambios incompatibles → bump de
`protoVersion` a 2.*
