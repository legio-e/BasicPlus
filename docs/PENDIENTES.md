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

### N-frontend-neq-hang — `a <> b` cuelga el compilador
`<>` (desigualdad estilo Modula — no existe en BP, es `!=`) mete al frontend en
un bucle infinito tras "Procesando" (hay que matar el java). → convertirlo en
error de sintaxis con sugerencia ("¿querías `!=`?"), o al menos no colgar. Pequeño.

### N17 — `const` de CLASE con init no-literal: crash del emisor
Una const de clase con valor **literal** funciona (se inlina). Con init
**no-literal**, `literalValue=null` y su lectura emite `GET_GLOBAL "Cls.K"` —un
símbolo que nadie declara→ `RuntimeException` del ModWriter, no un diagnóstico
limpio. El error semántico de L8 solo cubre nivel módulo (`ownerClass==null`).
Decidir: extender el error a clases, o materializar backing + asignación en
`Cls.__init`. (De paso: `const C := Color.RED` —valor de enum, conocido en
compilación— hoy cae en "requiere literal"; mejora natural: inlinarlo desde
`EnumSymbol.values`.)

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
