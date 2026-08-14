# Análisis del modelo de memoria / heap de BasicPlus

**Fecha:** 2026-07-05 · **Foco primario:** arquitectura (con aristas de
especificación y verificación) · **Estado del código:** solo lectura, commit
`c2f82929`.

Este documento consolida en un solo sitio lo que hoy está repartido por
`HEAP_LAYOUT.md`, `h3bench/NOTAS.md`, `V2_BACKLOG.md`, `V4_BACKLOG.md` y
`HECHO_V2.md`. Reconstruye **qué es el modelo actual**, **cómo se llegó a él**
(la solución de compromiso de la V2), **qué evidencia lo respalda** y **qué
problemas quedan abiertos**.

---

## 1. El modelo actual, en una página

- **`memory[]` único** dividido en: sentinela → módulos (ext-table + data +
  code) → **heap** (crece hacia arriba con `heapNext`) → **stacks** por thread
  (a partir de `STACK_BASE`, default `memorySize/2`). Todo **big-endian**.
  (`HEAP_LAYOUT §1`)
- **Objetos**: header de 8 B (`tag` + `length/class_ptr`), payload alineado a 4.
  El `tag` lleva bits de GC (`MARK`, `FREE`) y 6 bits de tipo. 5 tipos
  documentados: `ARRAY_I8/I16/I32/REF` y `OBJECT`. (`HEAP_LAYOUT §2`)
- **GC: mark-sweep NO-MOVING, stop-the-world.** Free-list first-fit + split +
  **coalescing** de huecos adyacentes en cada sweep. Disparo **por umbral** de
  crecimiento del bump (~1/8 del heap) además de "bump lleno". **heapNext-retreat**
  devuelve el run libre final al bump. En **ambas VMs**. (`HEAP_LAYOUT §7`,
  `h3bench/NOTAS.md`)
- **Raíces GC**: stack de cada thread (escaneo **conservador**: cualquier i32 en
  rango de heap se trata como ref potencial), `allocAnchor` por thread, y data
  block de cada módulo. (`HEAP_LAYOUT §7`)

### 1.1 Tres regímenes de vida de la memoria (no solo heap + stacks)

El modelo tiene **tres** políticas de vida conviviendo, no una:

| Régimen | Dónde vive | Cómo se libera | Para qué |
|---|---|---|---|
| **Heap "normal"** | heap | GC mark-sweep (perezoso, no-determinista) | objetos y arrays generales |
| **`owner`** (`var owner x`, `owner` fields) | **heap** | **determinista**: `FREE_REF` al reasignar / salir de scope; **cascada** por `owner_bitmap` y arrays REF. **No lo traza el GC como "suyo"** | recursos con vida acotada (`OwnerList`, handles) |
| **Arrays locales fijos** (`var buf: byte[64]`) | **stack** del thread | automático al salir de scope (unwind de pila) | buffers pequeños y rápidos de I/O |

Reglas `owner` clave (`manual §12`):
- `x := y`: libera el valor anterior de `x` **antes** (FREE_REF).
- Asignar `owner → variable no-owner`: **copia la ref sin transferir** propiedad.
- Asignar `owner → owner`: **transfiere** (la fuente queda `null` sin liberar).
- `SET_FIELD_OWNER` libera el valor anterior del campo antes de escribir.

> **Nota de analista.** Los tres regímenes son un buen diseño (dan al programador
> palancas deterministas que descargan al GC), pero **conviven sobre la misma
> memoria y el mismo GC**, y las reglas de esa convivencia **no están en el
> documento canónico del heap** (`HEAP_LAYOUT §7` habla solo del GC). Ahí es donde
> aparecen los riesgos de la §3-bis.

## 2. Cómo se llegó aquí — la solución de compromiso (H3, V2)

| Fase | Estado del GC | Problema |
|---|---|---|
| V1 | **mark-sweep bump-SIN-reuse** en la VM-C: marcaba `FREE` pero no reusaba → el heap solo crecía | "El acantilado": estrés de heap = **OOM** en el MCU, justo donde más duele |
| V2 / H3 (2026-05-31) | Se reimplementa el modelo de memoria manteniendo la interfaz GC↔VM. Se construyen herramientas (`heapFrag`, `heapMap`) y se caracteriza con workloads de estrés | Decisión: **NO-MOVING** + free-list + coalescing + mejoras model-agnostic. **Compactación (moving) DIFERIDA** |

**El compromiso**, en una frase: se eligió el allocator **no-moving** y se
**aplazó la compactación**, aceptando conscientemente que un patrón concreto
(muchos objetos longevos **dispersos**) puede fragmentar el heap sin que el
sistema lo resuelva — a cambio de no pagar el coste del *pinning* que exigiría
mover objetos que las funciones `native` recorren como buffers crudos.

La tesis que sostiene la decisión (y es correcta): **es un problema de la VM, no
del lenguaje.** El `.mod` es estable, así que el allocator se puede sustituir sin
tocar lenguaje, compilador ni código de usuario.

## 3. La evidencia (lo que midieron las pruebas de V2)

De `h3bench/NOTAS.md`, allocator actual (no-moving + coalescing):

| Workload | frag | huecos | mayor hueco | committed | Lectura |
|---|---|---|---|---|---|
| churn (basura efímera) | 0.000 | 1 | 6284 | 8140 | coalesce perfecto |
| **supervivientes dispersos** | **0.965** | 51 | 312 | 13732 | **fragmenta fuerte** |
| realista (lazo de control) | 0.637 | 11 | 51344 | 146104 | frag benigna; **over-commit** |
| tamaños mixtos dispersos | 0.966 | 51 | 720 | 32644 | fragmenta fuerte |

Tres conclusiones de la evidencia:
1. La **fragmentación patológica** (frag ≥ 0.96) aparece **solo** con muchos
   longevos dispersos. Ningún no-moving (ni TLSF) la arregla — solo compactación.
2. El patrón **realista** (working-set + churn + retención acotada) **no** la
   produce.
3. El dolor real del patrón realista era **otro**: **over-commit por GC
   perezoso**. Atacado con disparo-por-umbral + heapNext-retreat → **−59 % de
   pico** de heap sin compactar.

> Nota de analista: la decisión está **bien fundamentada en datos**, pero su
> validez depende de una hipótesis — *"los workloads reales no acumulan longevos
> dispersos"*. Esa hipótesis **no tiene hoy ni criterio de verificación ni monitor
> en producción** que avise si se incumple (ver H-005).

## 3-bis. Interacciones entre los tres regímenes — donde viven los hazards

Cada mecanismo por separado es razonable; el riesgo está en cómo se cruzan.

1. **Aliasing de un `owner` → use-after-free** ([H-007](../../hallazgos/H-007-owner-aliasing-uaf.md)).
   El manual **permite explícitamente** copiar la ref de un `owner` a una
   variable **no-owner** ("copia la ref sin transferir propiedad"). Cuando el
   dueño sale de scope, el objeto se `FREE_REF`-libera (va a la free-list), pero
   la copia no-owner **sigue apuntando** al bloque → puntero colgante. Si el
   bloque se reusa, la copia aliasa **otro** objeto vivo → corrupción silenciosa.
   BP no tiene borrow-checker que impida crear ese alias.

2. **Cascada `owner` documentada pero no implementada del todo**
   ([H-006](../../hallazgos/H-006-freeref-cascada.md)). El manual §12.2/§12.3
   promete liberación **recursiva** por `owner_bitmap` y por arrays REF. La
   implementación (`FREE_REF`) hoy solo libera la raíz (`B-freeref-no-recursivo`)
   → los sub-objetos poseídos **fugan hasta que el GC los recoja**. La promesa
   determinista se cumple a un nivel, no en cascada.

3. **Escaneo conservador del stack + datos en línea → raíces falsas.**
   El GC escanea el stack tratando cualquier i32 en rango de heap como ref. Un
   **array local** (`byte[64]`) vive en el stack y contiene datos crudos (bytes
   de SPI/UART, etc.); interpretados de 4 en 4, pueden **coincidir** con
   direcciones de heap → el GC **retiene objetos muertos** (falsos positivos).
   No es un bug de corrección, pero sí de **predecibilidad de memoria** en MCU —
   justo el valor que la V2 puso como pilar. (Pendiente de elevar a hallazgo.)

4. **Coexistencia `FREE_REF` ↔ mark-sweep sin especificar.** ¿Puede un bloque
   estar a la vez `TAG_FREE` (en la free-list por FREE_REF) y `TAG_MARK`
   (marcado conservadoramente por una copia no-owner en el stack)? ¿Qué gana en
   el sweep? `HEAP_LAYOUT §7` no lo dice. Hueco de especificación en el punto de
   cruce de los dos gestores. (Pendiente de elevar a hallazgo.)

5. **Coste en stack de los arrays locales narrow (cruza con [H-003](../../hallazgos/H-003-narrow-arrays-i32.md)).**
   Como los narrow locales viven como i32, un `byte[64]` "pequeño y rápido"
   ocupa **256 B**, no 64 — un 12,5 % de un stack de worker de 2 KiB. El
   beneficio "rápido y barato" es real en velocidad, no en huella.

## 4. Problemas abiertos — registro de riesgos y trazabilidad

| ID | Problema | Tipo | Sev. | ¿Trazado en el proyecto? | ¿Probado? |
|---|---|---|---|---|---|
| [H-001](../../hallazgos/H-001-allocanchor-raiz-gc.md) | `allocAnchor` documentado como raíz GC implementada, pero la VM-C no lo escanea (el campo ni existe) | discrepancia spec↔impl + corrección | **Alta** | Sí (`B-gc-allocanchor`, V4) | No |
| [H-002](../../hallazgos/H-002-arrays-8-bytes-sin-tipo.md) | `long[]`/`double[]` (8 B/elem) publicados en V2, pero la tabla de tipos canónica (`HEAP_LAYOUT §2.2`) no los define | hueco de especificación | Media | No | n/a |
| [H-003](../../hallazgos/H-003-narrow-arrays-i32.md) | `var arr: byte[5]` source-level se aloca como i32 (4 B/elem), contra lo que sugiere la tabla de tipos | discrepancia doc↔impl | Media | Sí (`HECHO_V2` follow-up, V4) | Parcial |
| [H-006](../../hallazgos/H-006-freeref-cascada.md) | `FREE_REF` no libera en cascada los `owner` (manual §12 promete recursivo; impl solo libera la raíz) → fuga hasta el GC | discrepancia manual↔impl | **Alta** | Sí (`B-freeref-no-recursivo`, V4) | No |
| [H-007](../../hallazgos/H-007-owner-aliasing-uaf.md) | Alias no-owner de un `owner` → use-after-free tras `FREE_REF`; el manual permite el alias, no hay borrow-check | corrección | **Alta** | No | No |
| H-004 | GC stop-the-world **sin cota de pausa** especificada ni medida; "latencia acotada" es requisito abierto para lazos de control | hueco/riesgo | Media | Parcial (`V2_BACKLOG` lo nombra) | No |
| H-005 | Compactación diferida bajo la hipótesis "sin longevos dispersos", **sin monitor** que detecte si se incumple en producción | riesgo | Media | Parcial | Solo en banco (h3bench) |
| H-008 | Escaneo conservador del stack + datos en línea (arrays locales) → raíces GC falsas → retención impredecible | riesgo (predecibilidad) | Baja-media | No | No |
| H-009 | Reglas de coexistencia `FREE_REF` ↔ mark-sweep sin especificar (`MARK`+`FREE` simultáneos, orden en sweep) | hueco de especificación | Media | No | No |

(H-001..H-003, H-006, H-007 desarrollados como hallazgos independientes;
H-004/H-005/H-008/H-009 quedan en esta tabla a la espera de decidir si se elevan.)

## 5. Preguntas abiertas para el equipo

1. **¿Cuál es la cota de pausa aceptable del GC** para un lazo de control BP? Sin
   un número, "latencia acotada" no es verificable (H-004).
2. **¿Qué señal dispararía retomar la compactación?** Hoy el criterio es
   cualitativo ("si un workload real exhibe la patología"). ¿Se puede instrumentar
   `heapFrag()` en runtime y fijar un umbral? (H-005)
3. **¿El modelo de memoria del MCU aprovecha los narrow types?** Si `byte[]`
   source-level va a i32, el argumento "~4× más compacto" de la V2 no se cumple
   para arrays declarados por el usuario (H-003).
4. **¿Dónde se documenta el layout de `long[]`/`double[]`?** Falta cerrar el
   `HEAP_LAYOUT §2.2` (H-002).

## 6. Fuentes

`HEAP_LAYOUT.md` (canónico) · `h3bench/NOTAS.md` (mediciones H3) ·
`V2_BACKLOG.md §"¿Se puede usar en una solución real?"` y `§H3` ·
`V4_BACKLOG.md §Bugs` · `HECHO_V2.md §narrow types` · `SMP_ARCH.md` (STW/safepoints).
