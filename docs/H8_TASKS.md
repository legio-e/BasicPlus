# H8 — Ampliación de lenguaje (incremental, en paralelo con la consolidación)

> **Arranque 2026-06-06.** Fase de V2 dedicada a **añadir features de lenguaje
> útiles** a la vez que seguimos **tapando agujeros** (la consolidación). La idea
> (usuario): avanzar y endurecer al mismo tiempo — el sistema gana potencia *y*
> solidez en paralelo. H8 es un **cajón vivo**: empieza con dos temas y se le irán
> añadiendo más sobre la marcha.

Método de siempre: **inventario → decidir alcance → tareas, una a una**, con
paridad dual-VM (miVM Java ↔ bpgenvm-c C) byte-idéntica en todo lo que toque la VM.

## Orden acordado (usuario, 2026-06-06)
1. **H8.1 — §10 Parámetros por defecto** ✅ **HECHO (2026-06-07)**.
2. **H8.2 — §2 Tuplas first-class** ✅ **HECHO (2026-06-07)**.
3. (se irá ampliando…)

---

## H8.1 — Parámetros con valor por defecto
> **✅ HECHO (2026-06-07).** Sintaxis **`:=`** (no `=`: BP no tiene `=` simple, solo
> `==` y `:=`; además `:=` es consistente con `var y: integer := 10`). Implementado
> como **reescritura de AST en el llamante** (`SemanticAnalyzer.checkArgs` anexa el
> literal default a los argumentos omitidos) → **CERO cambios en emisor y VM**;
> paridad dual-VM byte-idéntica por construcción. Cubre **funciones, métodos y
> constructores**, same-module y **cross-module** (`.bpi` **v7** expone los defaults).
> Validación: literal constante + asignable al tipo + regla "los defaults van al
> final" + guard de tipos estrechos. Samples: `samples/DefaultParams.bp`,
> `DefApp.bp`/`DefLib.bp` (cross-module), `DefaultParamsClass.bp` (método+ctor),
> `DefaultParamsBad.bp` (negativos). Regresión OK (narrowtypes 7, cascada 1, suite
> miVM 34/34).

**Ref**: `docs/V2_BACKLOG.md §10`. Diseño ya cerrado.
- **Qué**: `function f(x: integer, y: integer := 10)` y poder omitir `y` en la llamada.
- **Alcance**: **solo defaults CONSTANTES** (literales/const) — "con constantes es
  suficiente"; no expresiones arbitrarias (evita decidir contexto de evaluación).
- **Mecanismo**: sustitución **en el LLAMANTE** (modelo C++) → **CERO coste de VM**
  (el compilador rellena los args omitidos con el default). El `.bpi` expone los
  defaults para que los importadores también puedan omitirlos.
- **Toca**: lexer/parser (sintaxis `= valor` en params), semántico (validación +
  relleno), `.bpi` (serializar defaults), emisor (push de defaults en el call-site).
  La VM **no** cambia → paridad intacta por construcción.
- **Verificar**: sample + paridad dual-VM + cross-module (omitir args de una función
  importada).

## H8.2 — Tuplas first-class
> **✅ HECHO (2026-06-07).** La tupla es ahora **valor de primera clase**: guardar en
> una var (`var t: (integer,string) := f()`), pasarla como parámetro, **devolver** una
> tupla almacenada (`return t`), y leerla de **colecciones** (`any`→tupla). Además,
> **destructuring a lvalues no-simples**: `{ arr[i], obj.x } := f()`. Implementación
> mínima (el grueso ya existía de T1/T3): relajar `analyzeReturn` (return no-literal),
> `TupleType.isAssignableFrom(any)`, y extender `storeToTupleTarget` (índice/campo/
> property, reusando el cableado de `emitAssign`). **Cero opcodes nuevos / cero coste
> de VM**; paridad dual-VM byte-idéntica. Las tuplas LITERALES `(a,b)` siguen solo en
> `return` (decisión de parser: evita la ambigüedad con `( )`). Sample:
> `samples/TupleFirstClass.bp`. Regresión OK (T1/T3 byte-idénticos, narrowtypes 7,
> cascada 1, suite miVM 34/34).

**Ref**: `docs/V2_BACKLOG.md §2`. Hoy hecho: T1 (destructuring de retorno same-module),
T3 (cross-module), T4 (familia `Str.parse*`).
- **Qué falta**: que la tupla sea **valor de primera clase** — guardarla en una var
  (`var t := f()`), pasarla como parámetro, meterla en colecciones; y destructuring
  a **lvalues no-simples** (`{ obj.x, arr[i] } := f()`).
- **Base**: la tupla ya es un objeto sintético `__Tuple_<sig>` (NEW_OBJECT +
  SET_FIELD/GET_FIELD), así que ampliar el alcance es sobre todo **frontend**
  (tipos tupla como tipo de var/param, inferencia, .bpi) — cero/poco coste de VM.
- **Nota AOT**: `function native` no soporta tuplas (decisión previa); si molesta,
  un guard semántico claro. Fuera de alcance inicial.
- **Verificar**: sample + paridad dual-VM.

---

## Temas candidatos a entrar en H8 más adelante (de `V2_BACKLOG`)
- §8 Funciones de primera clase / callbacks + eventos (+ §9 interrupciones HW →
  eventos BP, *prereq de la GUI de V3*).
- §13 buses que faltan · §14b String (catálogo nuevo) · §14e Compress/Archive.
- (lo que vaya surgiendo)

## Descartado
- §5 **Interfaces de clase** — DESCARTADO (usuario, 2026-06-06). El polimorfismo por
  herencia simple + objeto-callback / función-valor cubre el caso. Ver `V2_BACKLOG §5`.
