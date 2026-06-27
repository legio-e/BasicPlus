# BasicPlus — Pendientes vivos

> Documento **vivo**: bugs abiertos y limitaciones/decisiones del lenguaje que
> aplican a cualquier versión (se pueden tocar en cualquier momento, también
> durante V3 — son bugs/pulido, no features).
>
> **Mapa de docs:**
> - **`V3_BACKLOG.md`** — lo aparcado para V3 (fuente única del backlog de V3).
> - **`HECHO_V2.md`** — snapshot **inmutable** del backlog tal como quedó al
>   cerrar V2 (el "diario" de cómo se resolvió cada cosa; no se actualiza).
> - **`HECHO_V1.md`** — puntero a `PROJECT_ROADMAP.md` (crónica de v1).
>
> Convención: B=bug · L=limitación · N=hallazgo · M=mejora.

---

## 🔴 Bugs abiertos

> Bugs **delicados** (vtable/módulos + GC) movidos a **`V4_BACKLOG.md`** (27-jun): `B-174b`,
> `B-gc-allocanchor`, `B-freeref-no-recursivo` — exigen tocar slots/GC con red de pruebas, no
> son fixes contenidos de V3.

### N-readfile-msg-skew — el mensaje de `RuntimeError` de `readFile(ausente)` difiere entre VMs
Al abrir un fichero inexistente, **ambas** VMs lanzan `RuntimeError` (bien), pero el **texto** difiere:
miVM `readFile('x'): x` vs VM-C `readFile('x'): no se pudo abrir`. Si un programa lo atrapa e imprime
`e.msg`, la salida NO es byte-idéntica → roza el invariante de paridad. Contenido: alinear el wording en
`miVM` (builtin readFile) y/o `bpgenvm-c/src/builtins.c`. Menor (sólo si se captura y se imprime el msg).
Hallado 27-jun al cerrar B-gui-load-missing.
*(B-gui-load-missing RESUELTO 27-jun, commit `ad06993`: el supuesto "cuelgue" NO se reproducía —`readFile`
ausente lanza limpio en ambas VMs—; el problema real era que `parseJson("")`/basura divergía: VM-C devolvía
`0` mudo. Fix en `Json.parseNumberOn`: sin dígito → `RuntimeError` claro en AMBAS VMs. Paridad byte-idéntica
verificada.)*

### GAP-4 — formateo de `double` extremo en la VM-C: paridad rota (notación científica TODO)
`bpvm_format_double` (`bpgenvm-c/src/interp.c:321`) es **byte-idéntico** a `VirtualMachine.formatBpDouble`
(Java) en punto fijo, PERO para magnitudes `|x| >= 1e12` o `0 < |x| < 1e-6` la notación científica es un
**TODO** (aritmética IEEE determinista + `%lld/%06lld`). Para esos doubles extremos las dos VMs podrían NO
ser byte-idénticas → rompe el **invariante de paridad dual-VM** (raro de disparar, pero es el invariante
sagrado del proyecto). *(Minor relacionado: `%lld` no va en newlib-nano del STM32 — hoy mitigado con helpers
`u64_dec`; ojo si se añade `%lld` directo.)* Hallado 28-jun (auditoría).

## 🟡 Limitaciones / decisiones documentadas del lenguaje

- **L7 — `owner`/`final` no aplican a property de módulo.** Por diseño: `owner`
  pide FREE_REF en cascada (solo campos de instancia); `final` aplica a herencia
  (los módulos no la tienen). Reabrible si surge caso de uso.
- **L9 — `Mutex` no reentrante.** Por diseño (documentado en el manual). Para
  re-entrada, usar otro patrón (flag + condvar).
- **N2 — convención de acceso a `mem[]` / `JavaMutex`.** Cualquier acceso a
  `JavaMutex.{ownerTid, waiters}` debe ir bajo `vmLock` (o acquire/release
  explícito). Hoy se cumple; conviene documentar la regla.
- **N9 — clase sintetizada declarada parcialmente por el usuario.** Si el usuario
  declara `class SyncList` con solo `add`, la suya gana e incompleta; diagnosticar
  la incompatibilidad de firma sería útil.

## 🟢 Pulido (no urgente)

- **M2 — auto-unbox `any → primitive` con check en runtime** (variante "segura"
  de L1; coste: tag de tipo en cada `any`; discutible si compensa).
- **M4 — namespace separado para identificadores sintéticos** (`__prop_get_X`,
  `__strconcat`…; el prefijo `__` ya está reservado — sería un check explícito).
- **M5 — debugger: inspección de properties heredadas** (verificar que se recorre
  la cadena de herencia al inspeccionar; relacionado con N11).
- **M6 — `const` con valor de enum (`const C := Color.RED`)** hoy da "requiere
  literal" (el valor de enum es conocido en compilación). Mejora natural: tratarlo
  como literal e inlinarlo desde `EnumSymbol.values`. (Sale de N17, resuelto: una
  const de clase no-literal ya da diagnóstico limpio en vez de tumbar el emisor.)
