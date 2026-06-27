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

### B-174b — slot de vtable divergente al añadir métodos a clase base con subclases
Añadir métodos a una clase que tiene subclases desplaza su vtable y `ClassSymbol.ensureMethodSlots`
calcula slots distintos en el frontend y en el `ModWriter`. Síntomas: en `Component` (Gui) da
**error del emisor** al compilar ("slot divergente para X.setChecked frontend=37 ModWriter=29"); en
`Window` **no** da error (no hay subclase suya en el mismo módulo) pero en **runtime el VM-C despacha
al slot equivocado y CUELGA** (miVM lo resuelve bien). **Desbloquea:** `Gui.Window.find(name)` (localizar
un widget de un form por nombre) y, en general, extender clases base con subclases. El propio compilador
señala el sitio: `ClassSymbol.ensureMethodSlots`. Encontrado 28-jun (intento de `find()` revertido; queda
`Component.name`, commit `6f711c1`).

### B-gui-load-missing — `Window.load(".win")` con fichero ausente CUELGA el VM-C
Si el `.win` no está donde el FS/sandbox de la VM lo busca, `Window.load` → `Json.readJsonFile` →
`readFile` devuelve vacío y `parseJson("")` (o el wrapper) entra en bucle en vez de dar error → el VM-C
cuelga (miVM no). Un recurso ausente debería fallar limpio, no colgar (afecta también al device si falta
un resource). Repro: `Window.load("noexiste.win")` o un `.win` fuera del workdir. Encontrado 28-jun.

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
