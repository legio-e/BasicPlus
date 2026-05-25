# Catálogo de builtins (`CALL_BUILTIN id`)

Documento canónico de los builtins de la VM BP. Cada builtin tiene:
- `id` = `ordinal()` del enum `Builtin` (estable; **se añade siempre al
  final** para no romper compat con `.mod` ya emitidos).
- `bpName`: nombre con el que el frontend lo resuelve al ver una
  llamada. Builtins implícitos (sin prefijo `__`) son visibles desde BP
  sin import; los intrínsecos `__*` son uso interno de la stdlib y
  módulos como Math/IO los exponen con nombres limpios.
- Stack effect y semántica.

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

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 0 | `strlen` | `(s: string)` | `int` | Length del array de chars. |
| 1 | `parseInt` | `(s: string)` | `int` | Java `Integer.parseInt(s.trim())`. RTErr si no parsea. |
| 2 | `parseFloat` | `(s: string)` | `float` | Java `Double.parseDouble`. |
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
| 27 | `random` | `()` | `float` | `[0, 1)`. |
| 28 | `randomInt` | `(min, max: int)` | `int` | Uniforme en `[min, max)`. |

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
| 37 | `input` | `()` | `string` | Lee línea de stdin (sin newline final). |
| 38 | `readFile` | `(path: string)` | `string` | Lee fichero entero como UTF-8. RTErr si no existe / sandbox. |
| 39 | `writeFile` | `(path, content: string)` | `void` | Sobreescribe. Crea el fichero si no existe. RTErr sandbox. |
| 40 | `appendFile` | `(path, content: string)` | `void` | Append. RTErr sandbox. |
| 41 | `fileExists` | `(path: string)` | `bool` | RTErr sandbox. |
| 42 | `listDir` | `(path: string)` | `string[]` | Nombres de los entries (no recursivo). RTErr si no es dir. |

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
| 47 | `__charsToString` | `(chars: int[], len: int)` | `string` | Aloca string copiando `chars[0..len)`. |
| 48 | `charCodeAt` | `(s: string, i: int)` | `int` | Codepoint del char `i` (sin alocar). |

## 49..51 — Threading

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 49 | `__threadStart` | `(t: Thread)` | `void` | Crea un nuevo `ThreadContext` con stackBase nuevo, inicializa frame para llamar a `t.run()`, lo encola en `runQueue`. |
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
| 55 | `move` | `(src, dst, srcStart, dstStart, count)` | `void` | Copia `count` elementos. Soporta overlapping en el mismo array (`System.arraycopy` sobre el byte[] subyacente). RTErr si: src/dst null o no son arrays; tipos distintos (i8/i16/i32/ref); count negativo; rangos fuera de length. |

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
| 68 | `__pathAbsolute` | `(p: string)` | `string` | Resuelve `..` y `.`. |
| 69 | `__mkdir` | `(p: string)` | `void` | Recursive. No falla si existe. |
| 70 | `__rmdir` | `(p: string)` | `void` | Sólo si vacío. |
| 71 | `__removeFile` | `(p: string)` | `void` | |
| 72 | `__rename` | `(src, dst: string)` | `void` | Sobreescribe destino. |
| 73 | `__copyFile` | `(src, dst: string)` | `void` | Sobreescribe destino. |
| 74 | `__fileSize` | `(p: string)` | `int` | RTErr si >2GB (no representable en i32). |
| 75 | `__isDirectory` | `(p: string)` | `bool` | |
| 76 | `__lastModified` | `(p: string)` | `int` | Epoch ms truncado a i32. |

## 77 — UI (N20)

| id | bpName | Args | Return | Semántica |
|---|---|---|---|---|
| 77 | `__prompt` | `(spec: string)` | `string` | Si no hay `PromptSender` registrado (= no IDE): RTErr BP atrapable. Si lo hay: bloquea el thread BP (`tc.status=BLOCKED_PROMPT`, registrado en `pendingPrompts[promptId]=tc`), envía `{"type":"PROMPT_REQUEST","promptId":N,"spec":...}` al IDE. El IDE responde con `{"type":"PROMPT_RESPONSE","id":M,"promptId":N,"values":...}` (wire v1) y la VM despierta el thread con el JSON empujado al stack. |

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

---

## Próximos builtins propuestos (no asignados todavía)

- **`__threadCurrent()` → Thread**: devuelve el Thread BP del thread
  actual. Útil para identificación de logs.
- **`__threadId(t: Thread)` → int**: tid de un Thread (también para
  logs).
- Builtins de socket TCP/UDP (cuando se integre lwIP en F4/F5).

Estos van con `id` consecutivo cuando se implementen, **siempre al
final** del enum.
