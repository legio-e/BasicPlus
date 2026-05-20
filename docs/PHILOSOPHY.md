# Filosofía del proyecto BasicPlus

Documento vivo. Captura las decisiones de fondo y la posición que ocupa
BasicPlus frente a otros lenguajes de su nicho. Actualizar cuando se
añadan ejes nuevos o cambien las prioridades.

---

## Qué queremos ser

**Un lenguaje de propósito general, portable a múltiples dispositivos,
con sintaxis sencilla y un debugger usable.**

- "Propósito general": no es un DSL ni un lenguaje de scripting. Tiene
  clases, módulos, herencia, properties, concurrencia, manejo de
  errores — lo que esperarías de un lenguaje "completo".
- "Portable a múltiples dispositivos": la VM está escrita en Java, no
  en C. Eso significa que cualquier dispositivo con JVM lo ejecuta sin
  tocar la VM. Para dispositivos sin JVM, lo que haya que portar es
  Java, no nuestra VM.
- "Sintaxis sencilla": estilo BASIC moderno —
  `function`, `if/then/endif`, `while/do/endwh`, `:=` para asignación.
  Pensado para que el código sea fácil de leer sin conocer todos los
  rincones del lenguaje.
- "Debugger usable": breakpoints, paso a paso, inspección de variables
  locales y properties de módulos. El debugger es un ciudadano de
  primera clase, no un añadido.

---

## Posición frente a las alternativas

### vs. Java

Java cumple la promesa de "compila una vez, ejecuta en todas partes"
muy bien — esa es exactamente la idea que queremos heredar.

**El problema con Java en dispositivos pequeños**: consume mucha
memoria. La JVM en sí, las librerías estándar, el overhead por
objeto… en la práctica, Java no sirve para hardware con recursos
ajustados. Aquí queremos consumir **menos memoria que Java**. La VM
tiene un heap fijo, sin GC compactador moderno (mark-sweep
conservativo basta), sin reflection, sin classloaders dinámicos.
La superficie es pequeña a propósito.

### vs. MicroPython

MicroPython es el otro buen ejemplo del nicho. Lo que hace bien:

- **Portabilidad de programas**: el mismo `.py` corre en muchísimos
  dispositivos casi sin cambios.
- **VM compacta y bien diseñada**.

Lo que no hace bien (y lo que queremos resolver):

1. **No tiene debugger**. Esto es un dolor real cuando trabajas en un
   dispositivo embebido.
2. **El intérprete es algo lento**. Aceptable para scripts pequeños,
   limitante para lógica más densa.
3. **Portar la VM a un dispositivo nuevo es muchísimo trabajo en C**.
   Y no es sólo "portar la VM": es integrarla con el SO embebido
   correspondiente. Cada plataforma es un proyecto por separado.

Nuestro planteamiento se aparta de MicroPython en dos puntos:

- **La VM en Java, no en C**. Permite portabilidad a coste cero
  donde haya JVM. Para dispositivos sin JVM, sigue siendo Java
  estándar (más fácil de portar que código C que se mete con el SO
  embebido).
- **Aceptamos un consumo algo mayor que MicroPython** a cambio de:
  debugger integrado, sintaxis más estructurada y portabilidad de
  la VM por construcción.

---

## Trade-offs aceptados

Lo decimos explícitamente para no perderlo de vista:

| Atributo | BasicPlus | Java | MicroPython |
|---|---|---|---|
| Consumo de memoria | medio | alto | bajo |
| Portabilidad de programas | sí | sí | sí |
| Portabilidad de la VM | requiere JVM | requiere JVM | requiere portar C |
| Debugger | sí | sí | no |
| Velocidad | aceptable | rápida | lenta |
| Tamaño del lenguaje | medio | grande | pequeño |

El cuadrante que ocupamos: **portabilidad alta, memoria moderada,
herramientas decentes, complejidad media**. No competimos en el
extremo "menor footprint posible" (eso es MicroPython) ni en el
extremo "máxima velocidad / ecosistema enorme" (eso es Java).

---

## Paralelismo real, no `async`

El caso de uso que tenemos en mente — pongamos un robot pequeño —
suele necesitar **muchas tareas concurrentes a la vez**: leer
sensores, controlar motores, comunicarse por red, parsear comandos,
mantener un loop de control. Diez hilos no son raros, son lo normal.

Aquí MicroPython falla por la dirección opuesta a la nuestra:

- Los hilos de MicroPython "no funcionan bien" en la práctica. La
  GIL global y la integración floja con cada port hacen que el modelo
  multithread sea inestable o muy limitado según el dispositivo.
- La salida recomendada es `async`/`await`. Funciona, pero **obliga
  al programador a colorear todas las funciones** (sync vs async),
  propagar `await` por todos los call-sites, gestionar event loops…
  Para un programa "haz N cosas a la vez" es ruido constante.

BasicPlus apuesta por **hilos preemptivos de verdad**, gestionados
por la VM:

- `class MyTask extends Thread` con un `run()` virtual. `start()` /
  `join()` como en Java.
- Sincronización con `Mutex` y `sync property` integrados en el
  lenguaje, no en una librería.
- El intérprete ya tiene un scheduler propio que reparte CPU entre
  los `ThreadContext`s con safepoints, GC stop-the-world, y bloqueo
  en lock/sleep.
- Ninguna función necesita marcarse como `async` ni propagar `await`.
  Las funciones se llaman entre sí como siempre; lo paralelo es el
  hilo, no la función.

El coste de esa decisión es:

- La VM se complica internamente (scheduler, GC con safepoint, JMM,
  hand-off de mutex). Lo asumimos — es UNA vez al implementar la VM,
  y libera al usuario para siempre.
- Hay un pequeño residual de race conditions bajo contención
  extrema (ver B1 en PENDIENTES.md). Es deuda nuestra, no del modelo
  conceptual.

A cambio, el código de usuario se lee como código secuencial normal,
y "haz 10 cosas a la vez" se escribe arrancando 10 threads.

---

## Lo que NO somos

- **No somos un lenguaje de sistemas**. No vamos a competir con C
  o Rust en escribir un kernel o drivers.
- **No buscamos máxima velocidad**. Un intérprete bytecode "bien
  hecho" basta — no es un objetivo añadir JIT.
- **No queremos un ecosistema gigante**. Stdlib pequeña, ortogonal,
  bien documentada. Si la stdlib crece, debe crecer en módulos
  separados (`Math`, `IO`, `Json`...), no en una masa amorfa.

---

## Notas de uso de este documento

Este es un documento de "por qué" — no de "cómo". Para los detalles
técnicos:

- `docs/manual.html` — referencia del lenguaje y de la VM.
- `docs/PENDIENTES.md` — backlog de bugs y mejoras.
- `basicplus_grammar.ebnf.txt` — gramática formal.

Cuando aparezca una decisión nueva de diseño que tenga raíz en la
filosofía (no en un caso técnico concreto), anotarla aquí.
