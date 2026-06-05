# H6 — Debugger (arranque 2026-06-04)

Fase H6 de V2 (tras H1 tipos · H2 strings · H3 GC · H4 ESP32 · H5 Object).
Renombrada desde "Debugger" (ver V2_BACKLOG §16). Mismo método que el IDE:
**inventario → decidir alcance → tareas, una a una.**

## Regla de oro (decisión de arquitectura, usuario 2026-06-04)

> **El `.mod` no cambia con/sin debugger. La info de debug va en un `.dbg`
> SEPARADO. El host/IDE tiene el `.dbg`; el device (VM-C) trabaja SOLO en
> pc/direcciones.**

Razón: asimetría de recursos. La VM-Java (host) tiene memoria ~ilimitada; la
VM-C (micro) no. Por eso:
- El device NUNCA carga el `.dbg` en su SRAM. Trabaja en pc/direcciones.
- El host (IDE) tiene el `.dbg` y hace TODA la traducción simbólica:
  línea→pc (poner breakpoint), pc→línea + slot/dir→nombre-de-variable (mostrar
  estado). El device solo: rompe por pc, hace step, reporta pc/sp/bp/mem crudos.
- Es más ligero incluso que "consultar el `.dbg` de la flash bajo demanda": el
  device ni lo toca.

## H6.1 — Inventario (HECHO 2026-06-04)

**Ya existe** (no partimos de cero):
- **`.dbg` separado** lo emite el frontend (MivmEmitter/ModWriter). Contenido
  hoy: tabla **pc→línea** (relPc→línea origen), **source path**, y **properties
  públicas de módulo** (`.dbg` v2). El `.mod` es debug-agnóstico. ✓ regla de oro.
- **VM-Java**: `ModuleManager` **carga el `.dbg`** (sourcePath, pc→línea,
  properties). `lineForPc`, etc.
- **VM-C**: NO carga `.dbg`. Hook `debug_pc_to_line` (callback, #139); la core
  trabaja en pc. Cero `.dbg` en SRAM.
- **Debugger A1 del host** (`DebugServer`/`VmClient`/`DebugController`) — bastante
  completo sobre el wire v1: `SET_BP`/`CLR_BP`/`LIST_BP`, `STEP into/over/out`,
  `CONTINUE`, `PAUSE`, `STACK`, `LOCALS`, `EVAL`, `INSPECT`, `READ_INT`,
  `READ_STRING`, `MODULE_PROPERTIES`, + FS (LIST/GET/PUT/...).
- **Device debugger** (#140 P-debug-pico-impl) = `[v2]`, diferido.

**HUECO principal (confirmado leyendo `sendLocalsReply`)**: `LOCALS_REPLY`
devuelve los locales como **array crudo de i32 por índice** (`readLocal(i*4)`,
`nLocals = (sp-bp)/4`) — **SIN nombres ni tipos**. El `.dbg` NO lleva los
nombres de variables locales/params por función → slot. Para mostrar locales
**por nombre** (la UX natural del debugger) falta esa info.

## H6.2 — Alcance (decidido: empezar por (a))

- **(a) Pulir el debugger del HOST** ◀ EMPEZAR AQUÍ. Lo más rentable y bajo
  riesgo, sobre lo que ya funciona (A1). El frente claro:
  - **H6.a.1 — locales por nombre** ✅ HECHO (v1, 2026-06-04). El `.dbg` sube a
    **v3** con sección `vars`: por función, `func <qn> <startRelPc>` + líneas
    `<nombre> <offsetConSigno> <sizeBytes> <isArray>` para params (offset<0) y
    locales (offset>=0). Additivo: el `.mod` NO cambia; la VM-C ignora el `.dbg`.
    Cadena: ModWriter captura var→offset por función (filtra temporales `__*`,
    conserva `this`/iteradores/constructores) → MivmEmitter lo vuelca al `.dbg`
    → ModuleManager lo parsea + `functionForPc(pc)` (floor lookup) → DebugServer
    `LOCALS_REPLY.named[]` resuelve `bp+offset` por nombre → BpvmClient/DebugSession
    `getNamedLocals()` → IDE `LocalsTableModel` muestra `Nombre | Offset | Valor`.
    Verificado end-to-end con `samples/LocalsDbg.bp` (suma: n,base,total,big(8B),i).
  - **H6.a.2 — tipos en `vars`** ✅ HECHO (2026-06-04). El `.dbg` v3 gana una 5ª
    columna por var = **tag de tipo BP** (`integer·long·float·double·string·
    boolean·ref·"?"`). Lo aporta MivmEmitter (`varDbgTypeTag(BpType)`) al declarar
    params/locales (ModWriter lo almacena en el slot). El parser de ModuleManager
    lo lee (5º campo opcional; tolera `.dbg` de 4 cols). DebugServer produce un
    `display` ya renderizado por tipo en `LOCALS_REPLY.named[]`: string→texto
    (vía `readStringIfPossible`), double→`longBitsToDouble`, float→`intBitsToFloat`,
    boolean→true/false, ref→`@handle`, array local→`[len=N]`. El IDE muestra
    `Nombre | Tipo | Valor | Offset`. Verificado: `.dbg` inspeccionado (big=long,
    ratio=double, msg=string, ok=boolean) + NamedLocalsDbgTest comprueba los tipos.
    - **Pendiente (H6.a.3, fase 3)**: inspección PROFUNDA de objeto/array (campos
      de un `ref`, elementos de un array) — hoy se muestran como `@handle`. Reusar
      INSPECT. El render del `display` lo hace hoy la VM-Java (tiene heap+`.dbg`);
      para el device (#140) el host hará esa traducción sobre bytes crudos.
  - (posibles, según interés) watch de expresiones, breakpoints condicionales
    (EVAL ya existe como base), mejor render de objetos/arrays.
- **(b) Debugger del DEVICE** (#140) — la pieza grande. Regla de oro: el device
  trabaja SOLO en pc/sp/bp/memoria cruda; el host (IDE) tiene el `.dbg` y traduce
  (línea↔pc, slot/dir→nombre). Por eso los breakpoints son **por pc** (el IDE
  convierte línea→pc) y LOCALS = frame crudo del device + resolución en el IDE.

  **Inventario (2026-06-04)**: ✅ hook #139 (fronteras de línea, fire-and-forget),
  ✅ plumbing wire/JSON (`wire_v1.c`) + cola de salida. ❌ ningún comando de debug
  en `repl_v1.c`, ❌ sin registro de breakpoints, ❌ sin pausa/bloqueo, ❌ sin
  BP_HIT, ❌ build host = batch puro (sin `--listen`).

  Decomposición (decidida: empezar por el núcleo portable):
  - **H6.b.1 — núcleo portable** ✅ HECHO (2026-06-04, #215). En el core C
    (`bpvm.c`/`interp.c`/headers): registro de breakpoints POR PC (add/clear/list/
    idempotente, tabla fija de 32, MCU-friendly), máquina pausa/continue/step en el
    inner loop de `bpvm_interp_run_quantum`, y **bloqueo inyectado** vía
    `bpvm_pause_cb_t` (el embedder lo provee: condvar en host / cola FreeRTOS en
    Pico). `BPVM_DBG_STOP` aborta (status `BPVM_DBG_STOPPED`). Accessors
    `bpvm_thread_pc/sp/bp/cs` para que el embedder reporte el frame crudo. Coste
    hot-path cuando `pause_cb==NULL`: un null-check (cero impacto en paridad).
    Test C `test/test_debug.c` (`make test-debug`): step + breakpoint-por-pc +
    stop + list/clear, sin wire ni hardware. Verificado + 0 regresión (Arith
    normal + native bridge intactos).
  - **H6.b.2 — transporte** (DECIDIDO: servidor wire en host C, depurable en
    desktop como A1; el firmware Pico reusa la misma lógica de pause_cb después).
    - **H6.b.2.a — pausa/resume cruzando threads** ✅ HECHO (2026-06-04, #216).
      El corazón socket-free: el intérprete corre en un thread; el pause_cb
      (embedder, pthreads) BLOQUEA en una cond var mientras un thread controlador
      inyecta CONTINUE/STEP/STOP por mutex+cond — sin deadlock. Patrón que
      necesita el server host y que mapea al Pico (task intérprete bloquea, task
      comm alimenta comandos). Test `test/test_debug_mt.c` (`make test-debug-mt`):
      worker pausa 5× y el controlador reanuda → status OK. Sin sockets ni HW.
    - **H6.b.2.b — sockets + wire JSON** ✅ HECHO (2026-06-04). Servidor wire host
      `bpvm_dbgserver <port> <mod>` (`test/debug_listen.c`, portable Winsock/POSIX):
      TCP accept de 1 cliente, thread READER parsea wire-v1 line-JSON, thread
      WORKER corre `bpvm_run`. El `pause_cb` envía `BP_HIT` (pc/sp/bp/cs CRUDOS,
      rol device — sin file/line) y bloquea vía el canal #216; `output_cb` →
      `OUTPUT`; al terminar → `EXITED`. Comandos: HELLO/PING/PAUSE/RUN/SET_BP-por-pc/
      CLR_BP/CONTINUE/STEP/STOP. Verificado end-to-end SIN hardware con un cliente
      Python (`test/dbg_client.py`, `make test-listen`): HELLO_REPLY(bpvm-c) →
      PAUSE → RUN → BP_HIT(pc,sp,bp) → SET_BP → STEP×2 → CONTINUE → 14×OUTPUT +
      EXITED(OK). Build limpio.
    - **H6.b.2.c — inspección cruda por wire** ✅ HECHO (2026-06-04). El server
      añade `READ_INT{addr}`, `READ_STRING{ref}`, `LOCALS`, `STACK` (sólo válidos
      mientras pausado: el worker está bloqueado en pause_cb → el frame no muta).
      LOCALS = i32 crudos entre bp y sp POR ÍNDICE; STACK = walk de frames
      (saved pc en bp-12, saved bp en bp-8, = Java); el host resolverá nombres con
      el `.dbg` (H6.b.3). Usa `bpvm_mem_read_i32/u32` + `tc->stack_base` vía
      `bpvm_internal.h` (host trusted; no toca el core → 0 riesgo de paridad).
      Verificado (`make test-listen`): en pc=1923 (sp>bp) LOCALS=[0],
      READ_INT(bp)==LOCALS[0] (cross-check), STACK con frame, READ_STRING(0)="".
      - **Pendiente (H6.b.3)**: el IDE resuelve símbolos sobre estos reads crudos.
  - **H6.b.3 — IDE resuelve símbolos sobre device crudo**: cuando el runtime es
    una VM-C (host o Pico), el host aplica el `.dbg` que tiene sobre los reads
    crudos del device → locales por nombre, igual que hoy hace la VM-Java pero
    del lado del host.
    - **H6.b.3.a — capa de resolución (host)** ✅ HECHO (2026-06-04, #217).
      `DbgFile` (lector standalone del `.dbg`: `lineForRelPc` + `functionForRelPc`,
      reusa `ModuleManager.FunctionVars/LocalVarDescriptor`) + `DeviceFrameResolver`
      (dado relPc=pc-cs + bp + un `MemReader`=READ_INT/READ_STRING del wire,
      produce locales {nombre,tipo,valor,display} + línea; render por tipo del
      lado host). Pura, sin GUI ni sockets. Test `DeviceResolveTest` con
      `samples/LocalsDbg.dbg`: en relPc(línea 10) → función suma con
      n/base=integer, big=long(100), ratio=double(1.5), msg=string("hola"),
      ok=boolean(true), i=integer. (Parser `.dbg` duplicado a propósito vs
      ModuleManager — read-only, menos riesgo que refactorizar lo load-bearing.)
    - **H6.b.3.b — cableado al IDE** ✅ NÚCLEO HECHO (2026-06-05). `BpvmClient`:
      (a) reconcilia el `pc` del BP_HIT device→`absPc` (la VM-C manda `pc`, la
      VM-Java `absPc`); (b) `resolveDeviceFrame(dbg, ev)` envuelve
      `READ_INT`/`READ_STRING` del wire como `MemReader` y llama
      `DeviceFrameResolver` con `relPc = absPc - cs`; (c) `setBreakpointPc(pc)`
      y `requestPause()` (API device-oriented: el host convierte línea→pc con el
      `.dbg`). **Oráculo Java↔C** verificado por `DeviceWireOracleSmoke`: cliente
      REAL del IDE ↔ VM-C (`bpvm_dbgserver`) ↔ `.dbg` host → en BP_HIT de la
      línea 10 de `suma` resuelve los 8 locales POR NOMBRE Y TIPO
      (n/base/total=integer, big=long, ratio=double, msg=string, ok=boolean,
      i=integer), idéntico a la VM-Java. Es la cascada Java→C→Pico ejercida
      end-to-end sobre el debugger, sin HW.
      - **GUI cableada** (2026-06-05): el listener de pausa de FrmMain, si
        `getNamedLocals()` viene vacío (= server device-role sin símbolos), cae
        a `resolveDeviceNamedLocals(pe)` → `BpvmClient.resolveDeviceFrame` con el
        `.dbg` que el host precarga al hacer `debug.attach` (campo `deviceDbg`,
        cargado del outDir local; limpiado en detach). Convierte
        `DeviceFrameResolver.Local` → `NamedLocal` y alimenta el MISMO modelo de
        la tabla de Variables. Aditivo y gated: la ruta Java-VM (que sí manda
        `named[]`) queda intacta. El panel de Variables muestra ahora locales por
        nombre/tipo también al depurar una VM-C remota (TCP endpoint A2.6).
      - **Pendiente**: el port del `pause_cb` al firmware Pico (#140, requiere
        HW); "Debug on Pico" sobre serie sigue mostrando el diálogo de "no
        implementado" hasta ese port.

## Próximo paso concreto
**H6.b.2 — transporte del debugger del device**: elegir host-wire-server vs Pico
y conectar el `pause_cb` (BP_HIT + bloqueo + comandos) al wire v1. El núcleo
(#215) ya expone todo lo necesario: breakpoints por pc, request_pause, accessors
de frame, y `BPVM_DBG_STOPPED`.
