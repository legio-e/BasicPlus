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
- **(b) Debugger del DEVICE** (#140) — la pieza grande siguiente, ya con el
  `.dbg` completo: host-tiene-el-`.dbg`, device-en-pc sobre el wire. Aquí la
  regla de oro se gana el sueldo. Diferida a después de (a).

## Próximo paso concreto
**H6.a.1 — locales por nombre**: (1) extender el writer del `.dbg` con la tabla
var→slot por función; (2) cargarla en `ModuleManager`; (3) resolver nombres en
`sendLocalsReply`/IDE. Additivo, dual-VM-safe (el `.mod` no cambia; la VM-C lo
ignora). Empezar leyendo el formato exacto del `.dbg` y el writer en ModWriter.
