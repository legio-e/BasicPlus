# Objetivo: un modelo de memoria global y robusto para SMP real

**Estado:** planteamiento del problema (para retomar). **No** es un diseño todavía.
**Foco:** arquitectura. **Rol:** Claude-Analista enmarca el problema; el diseño se
decide con el usuario.

> Dirección del usuario (2026-07-05): no arreglar los huecos puntuales del GC/owner
> (H-001, H-006, H-010, H-011, H-012) porque el modelo se va a **rediseñar entero**;
> arreglar algo que se sustituye en breve no compensa. Se **registran** para que no
> reaparezcan. El objetivo real es diseñar un **modelo global de memoria para todo
> el sistema** (no solo el GC) que sea **robusto bajo multitarea real**.

## El problema en una frase

En cuanto hay **paralelismo real (2 cores)**, dos mutadores pueden **crear y
destruir objetos a la vez**, y dos mecanismos de destrucción — **`owner`
(determinista)** y **GC (recolección)** — pueden **eliminar objetos
simultáneamente**. El modelo actual **no lo soporta**: se comprobó empíricamente.

## Evidencia empírica (VM-Java — bug "B1", `HECHO_V2.md`)

Las pruebas se hicieron en la **VM-Java**, que corre los workers en **hilos de SO
reales** (paralelismo verdadero; la VM-C en el micro hoy usa un modelo cooperativo
/ core0). Datos:

- **Escala con cores:** `w1 = 0 %` de fallo, `w2 ≈ 25 %`, `w4 = 100 %`. Con 1
  worker (cooperativo, = modelo del device) **no aparece**. → es paralelismo real.
- **No es el GC:** con heap de 64 MB y **0 GCs**, `w2` sigue fallando ~25 %. El
  heap pequeño (GC constante) solo *amplifica* y añade un use-after-free distinto.
- **Firma:** corrupción del estado de ejecución de un thread (`pc`/`bp` basura →
  "HALT en PC basura", "INVOKE_VIRTUAL null receiver"); race **worker↔worker**.
- **Causa caracterizada:** *"las lecturas fuera de `vmLock` (`GET_FIELD`, `ALOAD`,
  …) pueden ver bytes desactualizados en multi-core"* (`HECHO_V2 §B1`).

## Por qué el modelo actual falla por construcción

El diseño SMP (`SMP_ARCH.md`) busca velocidad con estas reglas:
- **Interp loop LOCK-FREE** (regla 3): el worker ejecuta el quantum sobre `mem[]`
  compartida **sin lock**; solo re-toma `vm_lock` para `heap_alloc`, `gc`, `spawn`,
  mutex BP o tocar `tc.status`.
- **UN solo `vm_lock` global** (regla 2) + **STW GC con safepoint** (regla 4).

Consecuencia: los accesos ordinarios a objetos (`GET_FIELD`/`SET_FIELD`/`ALOAD`/
`ASTORE`) leen y escriben memoria compartida **sin sincronización ni garantías de
visibilidad** entre cores. El `vm_lock` protege la *estructura* del heap (alloc/GC),
pero **no** el *contenido* que los mutadores tocan en paralelo. Es una **data race**
estructural, independiente del GC. Añadir `owner` (destrucción determinista que
puede solapar con lecturas de otro core y con el GC) agrava el mismo cruce.

## Hardware objetivo (la envolvente que el modelo debe cubrir)

Datos del usuario (2026-07-05):

| Recurso | Rango | Nota |
|---|---|---|
| Arquitectura | 32 bits | — |
| Núcleos | **1 o 2** | el SMP a soportar es de **2 cores como máximo**, no N |
| Frecuencia | ~100–150 MHz … **800 MHz** | factor ~5-8× entre el más lento y el más rápido |
| RAM interna | <512 KB … >1 MB | **rápida pero escasa** |
| PSRAM (opcional) | 8 MB … 32 MB | **grande pero más lenta** que la RAM |
| Flash | abundante (≫ RAM) | sistema embebido **y** sistema de ficheros |

**Implicaciones de diseño (no menores):**
- **El SMP está acotado a 2 cores.** Simplifica: no hace falta escalar a N; basta
  un modelo correcto y rápido para 2 mutadores. Muchos esquemas (p. ej. una arena
  por core) se vuelven baratos.
- **Jerarquía de memoria, no un plano único.** El modelo actual es *un solo
  `memory[]`* (data blocks + heap + stacks) en RAM. Con RAM escasa + PSRAM lenta +
  Flash abundante, el nuevo modelo debería **colocar cada cosa donde toca**:
  - *Flash (XIP, solo lectura, abundante):* bytecode, literales de string,
    descriptores de clase, `.mod` — todo lo **inmutable**. Hoy el data block se
    **copia a RAM**; servirlo desde Flash (XIP) libera RAM. (V4 ya apuntaba a
    `.mod` autodescriptivo + Packs/XIP para ahorrar RAM.)
  - *RAM (rápida, escasa):* pilas, objetos "calientes", estado mutable.
  - *PSRAM (grande, lenta):* heap grande, arrays voluminosos, lo "frío".
- **"Mover de RAM a Flash es bienvenido"** (usuario): criterio explícito — todo lo
  que sea read-only debe tender a Flash, no ocupar RAM.
- **Tensión con la paridad:** el "único array reproducible byte a byte" es lo que
  hoy sustenta la paridad Java↔C. Un modelo **con niveles** (RAM/PSRAM/Flash y
  colocación variable) complica esa reproducibilidad → hay que diseñar la paridad
  para que sea **independiente de dónde vive físicamente** cada objeto.

## Las fuerzas que el nuevo modelo debe conciliar

1. **Corrección bajo concurrencia real** — sin data races ni UAF, con visibilidad
   definida entre cores.
2. **Velocidad** — no serializar todo con un lock global por opcode (eso mató el
   speedup y por eso existe el interp lock-free).
3. **Paridad dual-VM byte-idéntica** — la regla sagrada del proyecto. Cualquier
   modelo de concurrencia debe dar el **mismo resultado observable** en Java y en C.
   Esto acota muchísimo: nada de "depende del scheduling del SO".
4. **Cabe en un micro** — RAM escasa (<512 KB…1 MB), sin lujos (ni generacional ni,
   hoy, compactador). El modelo tiene que ser barato.
5. **Coexistencia de los tres actores sobre la misma memoria**: alloc, `owner`-free
   determinista y GC — potencialmente **simultáneos** en 2 cores.
6. **Aprovechar la jerarquía RAM/PSRAM/Flash** — colocar lo inmutable en Flash
   (XIP), lo caliente en RAM, lo grande/frío en PSRAM; y hacerlo **sin romper la
   paridad** (el resultado observable no puede depender de dónde vive el objeto).

## Preguntas de diseño a resolver "a la vuelta" (agenda, sin respuesta aún)

1. **Modelo de memoria/visibilidad:** ¿qué garantías de ordenación sobre `mem[]`
   entre cores? (es la causa directa de B1). ¿Barreras, secciones publicadas,
   estructuras inmutables?
2. **Granularidad de sincronización del heap:** lock global (serializa) vs.
   striped/particionado vs. estructuras lock-free vs. **memoria por core /
   ownership de regiones** (cada core aloca en su arena; menos contención).
3. **Coexistencia `owner` ↔ GC concurrentes:** ¿cómo se evita que uno libere lo que
   el otro está trazando/usando? (reúne H-006, H-007, H-009 bajo concurrencia real).
4. **¿STW sigue siendo viable?** Un STW que para 2 cores es una pausa doble; ¿GC
   concurrente/incremental, o STW aceptable con cota (enlaza H-004)?
5. **Determinismo observable:** cómo garantizar salida byte-idéntica pese a que el
   orden de ejecución entre cores sea no determinista (¿confinar los efectos
   observables? ¿serializar solo los puntos observables?).
6. **Colocación en la jerarquía de memoria:** ¿qué vive en Flash-XIP (inmutable),
   qué en RAM (caliente), qué en PSRAM (grande/frío)? ¿Es la colocación estática
   (por tipo de dato) o dinámica (por temperatura)? ¿Cómo se referencia un objeto
   sin saber en qué nivel vive (¿handles/indirección vs. direcciones planas?) — y
   qué coste tiene eso para la ref conservadora del GC y para la ABI del AOT?

## Línea de diseño en discusión (Eduardo, 2026-07-05)

Planteamiento inicial del usuario, con una decisión ya tomada. **En discusión, no
cerrado.**

**Descomposición del problema en tres ejes** (idea del usuario, afinada):
- **(A) Concurrencia pura** — visibilidad y carreras sobre `mem[]` (campos/arrays)
  entre cores. Es el bug B1; existe sin `owner`.
- **(B) Semántica de `owner`** — cascada, "1 owner + N refs", contrato B; existe
  con 1 core.
- **(C) La costura: liberar vs. leer/reusar el mismo objeto** — donde A y B se
  tocan y se corrompe la memoria.

**Idea de diseño (ejes B+C):**
1. **Centralizar la autoridad de alloc/free en el heap** — una sola puerta mantiene
   los invariantes (mata H-010, que nace de tener la liberación partida entre
   intérprete y heap). Matiz: centralizar el *quién manda*, no forzosamente un
   *lock único* — con 2 cores cabe una **arena por core** para el alloc rápido.
2. **No reclamar en el sitio mientras corren los mutadores.** El objetivo no es
   *impedir* que free y read se solapen (carísimo por acceso), sino que el
   solapamiento sea **inofensivo**: la liberación **difiere** la reclamación física
   a un **safepoint** (maquinaria que el STW ya usa).

**✅ DECISIÓN (Eduardo, 2026-07-05): `owner` puede liberar en DIFERIDO.** No
necesita recuperar la memoria al instante → el modelo diferido es viable.

**Mecanismo resultante (owner y GC unificados):**
> `owner`-free = (1) poner **lápida** atómicamente ya [muerte lógica → contrato B
> activo, un ref colgante falla "objeto eliminado"], (2) encolar para reclamar,
> (3) reclamar/reusar físicamente **solo en el safepoint**. El GC hace lo mismo:
> ambos son "marcar para reclamar", reclamados en el mismo paso.

Esto **cierra la ventana de H-007** (la lápida salta antes de que el bloque pueda
reusarse) y disuelve H-006/H-009/H-010 (un solo paso de reclamación, con el
`owner_bitmap`, con invariantes correctos).

**Lo que este modelo NO resuelve (¡importante, honrando la separación de ejes!):**
el **eje (A)** — la data race cruda de lecturas/escrituras de campos sobre `mem[]`
(B1 falla con 0 GCs) — es **independiente** y sigue abierto. La reclamación diferida
arregla B y C; A necesita su propia respuesta (barreras de memoria / confinar el
acceso mutable / arenas). No dejar que el buen resultado en B+C tape que A sigue ahí.

**Eje A — dirección (Eduardo, 2026-07-05): objetos thread-safe, no todo al programador.**
Un programador de BASIC puede no ser experto → no cargarle toda la sincronización,
pero sin cortarle las alas. Modelo **Java**: objetos seguros por defecto
(`java.util.concurrent`) + primitivas (`Mutex`/`synchronized`/`sync`) para el
experto. Ejemplo ya existente: **`SyncList`** (cola sincronizada con `popBlocking`,
estilo `BlockingQueue`). El eje A queda **partido**:
- **A2 — qué objetos son seguros de compartir → DECIDIDO:** objetos thread-safe de
  librería (estilo collections de Java) + primitivas para el experto. Capa de
  *lenguaje/ergonomía*.
- **A1 — que la VM misma sea memory-safe → PENDIENTE (el fundamento):** publicación
  segura de objetos entre cores, orden de lecturas/escrituras, y **la VM nunca debe
  corromper su propio `pc`/`bp` por una carrera del usuario** (una carrera puede dar
  datos raros, jamás tumbar la máquina). Con la restricción de paridad: reglas
  explícitas, iguales en JVM y C/ARM, sin apoyarse en la coherencia del hardware.

> **⚠ Observación clave:** B1 se midió con **`SyncListTest`** — o sea, la corrupción
> ocurrió **ejecutando la lista thread-safe**. Los objetos seguros (A2) **no** la
> evitaron porque el fallo vive **debajo**, en A1. Las collections concurrentes de
> Java funcionan **solo porque debajo está el Java Memory Model** (publicación
> segura, *happens-before*). BasicPlus tiene la planta de arriba (`SyncList`) y le
> falta la de abajo (un modelo de memoria de la VM). **A1 es la de abajo.**

**Sub-preguntas abiertas del modelo diferido:**
- **Disparo del safepoint con RAM escasa:** el garbage diferido ocupa RAM hasta el
  safepoint; hace falta forzar uno por umbral (como el `gc_bump_threshold` actual).
- **Atomicidad con 2 cores:** poner la lápida y encolar deben ser seguros con 2
  productores (¿colas de reclamación por core, fundidas en el safepoint?).
- **¿Destructores con efectos?** Hoy `owner`-free es solo memoria (sin código de
  usuario) → el orden de reclamación no afecta a la salida → paridad a salvo. Si
  algún día `owner` ejecuta cleanup (cerrar bus/fichero), el timing/orden pasaría a
  ser observable → reevaluar.

## Mecanismo candidato: tabla de handles con contador de generación (2026-07-05)

Sugerido por el usuario (de una charla previa con Claude). Técnica conocida
(*slotmap* / *generational index* / generational handles). **Encaja como el
primitivo unificador** de los ejes B, C, tiering y parte de A1.

**Cómo funciona:**
- Objetos en una **tabla**; cada slot = `{dirección_física, generación}`.
- Referencia (**handle**) = `{índice, generación}` (no es un puntero).
- Deref: comparar `handle.gen == slot.gen`. Distinto → el slot se liberó/reusó →
  error **"referencia a objeto eliminado"** (contrato B, ahora **exacto**).
- Liberar: **incrementar** la generación del slot + marcarlo libre — pero
  **generation-checked** (ver refuerzo).

> **⚠ Refuerzo obligatorio (hallado por la maqueta v1, 2026-07-05):** la liberación
> debe **validar la generación**, no solo la lectura. Solo la **primera** liberación
> de un handle vivo tiene efecto; una liberación **stale o doble** es un **no-op
> seguro** (`gen.compareAndSet(slot, g, g+1)`). Sin esto, liberar un handle colgante
> bumpea la generación del objeto **vivo** que ahora ocupa el slot → **doble-free →
> doble-alloc → corrupción**. La maqueta pasó de 22.432 corrupciones a **0** con este
> arreglo. Ver [`02-verificacion/resultado-maqueta-v1.md`](../../02-verificacion/resultado-maqueta-v1.md).

**Qué unifica:**
- **Contrato B exacto** — el contador detecta precisamente el reuso, que era el
  agujero del tombstone best-effort. Cierra H-007 del todo.
- **Independencia de ubicación** — el objeto puede vivir/moverse entre RAM/PSRAM/
  Flash; solo se actualiza la dirección del slot. Resuelve la tensión tiering↔paridad.
- **Reabre la compactación** — mover = actualizar un slot, **sin pinning general**
  (solo fijar durante una llamada `native`, que usa puntero crudo). Ataca H-005.
- **Tabla hot y pequeña → RAM; objetos → PSRAM** (encaje natural con la jerarquía).
- **Compone con la reclamación diferida**: liberar = incrementar gen + encolar slot;
  reclamar en el safepoint. Handle-table y diferido son el mismo modelo (identidad
  por tabla, vida por incremento-de-generación diferido).
- **Parity-friendly**: software explícito, mismas reglas en JVM y C/ARM → mismos
  fallos observables. Mejor que punteros planos para la paridad.

**Costes (a pesar en el diseño):**
1. **Indirección por acceso** — cada `GET_FIELD`/dispatch: salto por tabla +
   comparar gen. **Pero el coste es RELATIVO** (matiz de Eduardo, 2026-07-05):
   - **Interpretado:** el opcode ya cuesta ~una docena de operaciones → la
     indirección (2-3 más) se ahoga en el ruido del intérprete → overhead de un
     dígito %. **Casi gratis donde BP gasta la mayoría de los ciclos.**
   - **`native`** (AOT, 10-100× más rápido = en la práctica C): ahí SÍ pesaría…
     salvo que **ya** resolvemos el handle **una vez en el borde de la llamada** y
     el nativo opera con **puntero crudo** sobre el objeto **fijado (pin)** — el
     mismo pin que la compactación exige. Los kernels nativos rápidos no alocan →
     sin GC/movimiento a mitad → puntero estable → sin indirección por acceso.
   - **Neto:** gratis en interp, puenteado en native → **debilita mucho la razón de
     ser del híbrido** (no hace falta un 2º régimen de refs para ganar velocidad).
   - ⚠️ **Es hipótesis, no dato** (el propio Eduardo: "habría que confirmarlo") →
     medir con un *spike* (ver `02-verificacion/spike-overhead-handles.md`).
2. **Ancho de generación / wraparound** — contador finito; con pocos bits, un
   handle antiquísimo podría colisionar con una generación reciclada → falso válido
   (UAF de vuelta). Único punto delicado de corrección. 32 bits ≈ imposible.
3. **Contención de la tabla con 2 cores** — mitigar con porciones por core.
4. **Overhead de memoria** (~8 B/objeto vivo) — pero vive en RAM, su sitio.

**Sobre A1:** los handles dan un **punto de publicación único** (escribir el slot),
pero **no** eliminan la barrera: en ARM hay que garantizar orden release (escribir
el slot tras inicializar el objeto) / acquire (leer el slot ve el objeto completo).
Hacen A1 **tratable**, no trivial.

**Decisión pendiente (la bifurcación):** ¿**todas** las referencias pasan a ser
handles (uniforme, paridad simple) o solo las de objetos compartidos/`owner` y las
thread-local siguen con puntero plano (**híbrido**)? **Lean de análisis: uniforme**
— el coste de indirección es casi nulo en interpretado y se puentea en native con
pin+puntero-crudo (ver coste 1), así que la ventaja de velocidad del híbrido casi
desaparece, mientras que su coste (dos regímenes de ref, paridad más difícil,
contradice "VM autómata") se mantiene. Confirmar con el spike de overhead antes de
cerrar.

## A1 — publicación segura: resolución (dirección, 2026-07-05)

**El problema (en cristiano):** en 2 cores, "escribí A y luego B" **no** garantiza
que el otro core vea A antes que B — cada core tiene su caché y las escrituras se
propagan reordenadas. Aplicado a objetos: un core crea y rellena un objeto y luego
publica su referencia; el otro core puede ver **la referencia antes que el
contenido** → sigue la ref y lee un objeto a medio construir. Es la causa directa
de firmas de B1 como *"INVOKE_VIRTUAL null receiver"*.

**La solución — un apretón de manos de dos lados (release/acquire):**
- **Escritor (release):** termina de inicializar el objeto **antes** de publicar la
  referencia.
- **Lector (acquire):** tras leer la referencia, garantiza ver todo lo que el
  escritor hizo antes de publicarla.
Hacen falta **los dos**; es una pareja.

**No es nada nuevo:** ese apretón de manos es lo que **ya vive dentro de cada
mutex** (`unlock` = release, `lock` = acquire). Por eso "proteger con un candado"
funciona: da exclusión (A2) **y** visibilidad (A1) a la vez.

**Por qué los handles lo hacen tratable:** la tabla de handles es el **único** sitio
donde se publica una referencia (escribir el slot) → el apretón de manos se pone
**ahí, una vez**, en vez de disperso por toda la VM.

**Paridad (se salva):** Java lo expresa con `volatile` / release-acquire; C sobre
ARM con atomics C11 (`memory_order_release`/`acquire` → barreras `dmb`). Misma regla
abstracta en las dos VMs → mismo comportamiento observable, **sin** depender de la
coherencia del hardware.

**Contexto de diseño (por qué esto es trabajo nuestro):** MicroPython/Python usan un
**candado global del intérprete (GIL)** → solo un thread interpreta a la vez → estos
problemas casi no existen, a cambio de renunciar a la velocidad del 2º core. BP
eligió lo contrario (interp lock-free, 2 cores en paralelo) para exprimir el core →
**tiene que ganarse a pulso la seguridad que un GIL da gratis.** Todo el modelo
(handles + reclamación diferida + barreras A1) es reconstruir esa seguridad, rápido.

**Distinción A1 vs A2 (no confundir):** A1 = "¿vi el objeto terminado?" (visibilidad,
se arregla con barreras en la publicación). A2 = "¿dos cores escriben el mismo campo
a la vez?" (mutación concurrente, se arregla con objetos thread-safe / candados, ya
decidido). `SyncList` protegía su estado (A2) pero la VM leía los objetos fuera de
ese candado (A1 sin resolver) → por eso se corrompía igual.

## Estado de validación (maqueta, 2026-07-05)

El núcleo del modelo está **validado en software** con una maqueta ejecutable
([`02-verificacion/maqueta/`](../../02-verificacion/maqueta/README.md)):
- **UAF / contrato B / reclamación diferida** bajo estrés concurrente (1→16 hilos):
  modelo viejo 7,4M corrupciones → modelo nuevo **0**.
- **Cascada owner ≥2 niveles** (C4) y **GC preciso** con raíces de módulo y `long[]`
  (C5): 9/9 aserciones OK.
- **Refinamiento producido por la validación:** la liberación debe ser
  **generation-checked** (ver arriba).
- **H-011/H-008/H-012 imposibles por construcción** (GC preciso, type-agnostic).

Pendiente (inherentemente fuera de x86/una-VM): la **barrera de publicación A1 en
ARM** (fase de hardware) y la **paridad dual-VM** (C6).

## Relación con los hallazgos ya registrados

Los hallazgos H-001/H-006/H-007/H-009/H-010/H-011/H-012 son **síntomas** del modelo
actual. El rediseño los **supersede**: no se arreglan de uno en uno; el nuevo modelo
debe hacerlos imposibles por construcción (raíces bien definidas, liberación con
invariantes correctos, coexistencia owner/GC segura, enumeración de tipos completa).
Sirven como **batería de casos** que el diseño nuevo tiene que superar.

## Fuentes
`HECHO_V2.md §B1` (evidencia empírica VM-Java) · `SMP_ARCH.md` (modelo SMP actual y
sus reglas) · `HEAP_LAYOUT.md §7` (GC) · hallazgos H-001..H-012.
