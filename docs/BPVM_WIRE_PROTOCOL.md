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

Categorías actuales: `FILES`, `TERMINAL`, `DEBUG`, `META`.

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
 "cpuFreqHz":150000000,
 "uptimeMs":12345,
 "tempC":43.5,
 "fsTotalBytes":131072,
 "fsUsedBytes":18432}
```

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

#### `STAT` (request)

Metadata de un fichero individual.

Request: `{"type":"STAT","id":N,"path":"/app/Hello.mod"}`

Reply: `{"type":"STAT_REPLY","id":N,"size":3519,"isDir":false,"mtime":0}`

Error si no existe: `{"type":"ERROR","id":N,"code":"NOT_FOUND",...}`

#### `GET` (request, reply con bulk)

Descarga un fichero. La reply lleva bulk con los bytes.

Request: `{"type":"GET","id":N,"path":"/app/Hello.mod"}`

Reply:
```
{"type":"GET_REPLY","id":N,"bulk":3519}\n
<3519 bytes raw>
```

#### `PUT` (request con bulk)

Sube un fichero. El request lleva bulk con los bytes. Crea o
sobreescribe.

Request:
```
{"type":"PUT","id":N,"path":"/app/Hello.mod","bulk":3519}\n
<3519 bytes raw>
```

Reply: `{"type":"PUT_REPLY","id":N}`

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

Request: `{"type":"RUN","id":N,"path":"/app/Hello.mod","args":[]}`

Reply: `{"type":"RUN_REPLY","id":N,"session":3}`

#### `KILL` (request)

Mata un programa en ejecución.

Request: `{"type":"KILL","id":N,"session":3}`

Reply: `{"type":"KILL_REPLY","id":N}`

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

`status` ∈ `OK | RUNTIME_ERROR | KILLED | INTERNAL_ERROR`.
Si `status=RUNTIME_ERROR`, también `errorMessage` y opcionalmente
`stackTrace`.

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
C → S: RUN id=11 path="/app/Hello.mod" args=[]
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
