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

## El sobre de plataformas: microcontroladores de 32 bits, sin SO

"Portable a múltiples dispositivos" necesita una frontera explícita, o acabamos
diseñando para todo y sirviendo bien a nada. El **sobre** (envelope) de hardware
es: **microcontroladores de 32 bits que se ejecutan sin sistema operativo
tradicional** — bare-metal o sobre un RTOS (FreeRTOS). Del más humilde al más
potente.

**El techo es arquitectónico, no de tamaño.** Lo único que deja un dispositivo
fuera es:

- **64 bits.** El modelo de memoria de la VM es plano y direccionado a 32 bits
  (las direcciones se manejan como `i32` / `writeInt32`): los 32 bits están
  *cocidos* en el runtime. El espacio de direcciones de 32 bits (4 GB) da
  headroom de sobra; ver abajo.
- **SO tradicional / Linux embebido.** El criterio limpio es *¿quién es el
  runtime?* En lo nuestro **el firmware ES el programa + la VM**; en Linux **el
  SO es el runtime y la app es un proceso** — construyes una *imagen de SO*
  (Yocto/Buildroot), no un firmware, sobre otra clase de placa (SoC + DRAM +
  eMMC). El marcador técnico de la raya es **MMU + DRAM externa + un bootloader
  que carga un SO**. MPU sin MMU = dentro; MMU + SO = fuera (uClinux también
  fuera: sigue siendo Linux).

"No 64 bits" y "no Linux embebido" caen casi en la misma raya dibujada desde dos
lados: en este nicho, lo 64-bit suele ser un Cortex-A con MMU corriendo Linux.

**Sin tope por arriba dentro del sobre.** Un MCU 32-bit potente **no es un caso
borde: es núcleo.** Los micros más capaces ya traen 1 MB+ de SRAM interna; la
PSRAM por ~5 $ ya es de 16 MB y vendrá de 32; las frecuencias van de ~100 MHz a
~1 GHz (crossover tipo i.MX RT / Cortex-M7). Todo eso entra holgado en el espacio
de 32 bits y **no tiene límite superior por nuestra parte**. La VM toma el tamaño
de memoria por **configuración por-dispositivo** (`memorySize` / `stackBase`); el
default de 512 KB es comodidad de host, y el build de cada device fija su RAM real.

**La restricción que manda es el piso, no el techo: diseñar para el pequeño.** Si
optimizamos asumiendo recursos grandes, **hacemos sufrir al dispositivo pequeño**
— que es justo nuestro caso de uso. Por eso:

- El **dispositivo pequeño es el objetivo de diseño**; lo grande es **headroom
  opcional, nunca requisito**.
- Las features deben **correr en el baseline y escalar** con los recursos, no
  **exigirlos**. PSRAM, pantallas/LVGL, heaps grandes = opt-in según la capacidad
  del device (de ahí, p.ej., las **dos imágenes de V3**: con y sin LVGL).
- Frugalidad por defecto (GC, colecciones, strings). Mantener honesto el baseline
  implica probar también con configuraciones de memoria pequeñas, no sólo el
  default cómodo.

**Target ≠ host.** La paridad dual-VM se *ejecuta* en un PC de 64 bits (JVM +
VM-C compilada para PC): eso es **banco de pruebas (host)**, no un objetivo. El
target siempre es el MCU de 32 bits — que algo "corra en un PC de 64 bits" para
desarrollar no convierte 64-bit en plataforma soportada.

| Zona | Qué entra / criterio |
|---|---|
| **Dentro (núcleo)** | Toda la clase MCU 32-bit sin SO, del Cortex-M0+ humilde al crossover ~1 GHz con 16–32 MB de PSRAM. Ejemplos (no normativos): Cortex-M0+/M4/M33/M55 (RP2040, RP2350B, STM32, nRF52/53), Xtensa (ESP32/S2/S3), RV32 (ESP32-C3/C6, RISC-V futuro), i.MX RT (alto de gama) |
| **Zona difusa** | Silicio dual-capaz (puede correr Linux pero lo usamos bare-metal/RTOS) → lo resuelve "quién es el runtime": bare-metal/RTOS = dentro |
| **Fuera (arriba)** | MMU + DRAM + imagen de SO (Linux embebido): Cortex-A (Raspberry Pi, Allwinner…), 64-bit, SOs de escritorio como *target* |
| **Fuera (abajo)** | 8-bit (AVR, PIC16/18) y 16-bit (MSP430, PIC24/dsPIC): no pueden hospedar el modelo de VM de 32 bits |

Este sobre **ratifica** lo que ya tenemos (VM-C sin-SO, memoria plana, asignación
por-config), **acota** el surface de HW (buses, GPIO, PSRAM…) sin asumir
POSIX / FS-de-SO / sockets-de-SO, **enmarca V3** (GUIs pequeñas en pantallas
pequeñas) y **da público a la documentación**: "graba un firmware en un MCU de
32 bits y programa en BP". Evoluciona *por arriba sin tocar la raya* (más RAM /
MHz / PSRAM entran solos); sólo se revisaría si cambiara la arquitectura base del
nicho.

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

## IDE y VM separados — y eventualmente remotos

Hoy la VM se ejecuta dentro del proceso del IDE: el IDE invoca a la
VM por llamadas in-process Java↔Java. Era el camino más corto para
ponerlo todo en pie y arrancar.

**A medio plazo, IDE y VM tienen que ser dos programas separados,
comunicados por un canal bidireccional**. La motivación es la misma
que el resto del proyecto: el destino final es un dispositivo
pequeño que ejecuta la VM. El IDE corre en el PC del programador.
Cuando llegue ese momento, lo que separa "PC" de "robot" no es más
que el cable (o radio) entre ellos.

Por eso conviene hacer el split AHORA, mientras los dos lados están
en la misma máquina. El protocolo nace local-only y se extiende
naturalmente a remoto cuando lo necesitemos.

**Qué tiene que hacer el canal**:

- **IDE → VM**: arranca un programa, párelo, paso a paso, breakpoints,
  inspección de memoria/locals/properties, copia de ficheros (.mod /
  .bpi / config) al dispositivo.
- **VM → IDE**: salida de `print` redirigida a una ventana del IDE,
  eventos del debugger (paused, hit-breakpoint, exception, exit),
  notificación de errores no atrapados.

**Implicaciones para el diseño del lenguaje y la VM**:

- `print` no puede asumir un `System.out` "local". La VM lo escribe
  a un buffer / stream que el transporte serializa al IDE.
- El debugger ya tiene una API (`DebugSession` en el IDE,
  `DebugContext` en la VM); esa API es la que se serializa al canal.
- La VM debe seguir funcionando **sin IDE conectado** (un dispositivo
  desplegado no tiene IDE), simplemente sin redirigir `print` ni
  reportar eventos de debug. Conectar/desconectar el IDE en caliente
  es deseable.

Esta separación tampoco es gratis: añade latencia, protocolo,
serialización, manejo de desconexiones. La aceptamos porque sin
ella el escenario "robot a 5 metros con IDE en el portátil" no
es posible.

---

## Una aplicación por VM, módulos stdlib pre-instalados

Una decisión deliberada: **la VM ejecuta UNA aplicación a la vez. No
multi-app**. La razón es que el target son dispositivos pequeños, y
multiplexar varias apps complica enormemente el modelo de memoria (heap
compartido vs aislado, GC entre apps, schedulers anidados…) sin
beneficio real para el caso de uso.

Dentro de una app sí hay multitarea: el programa puede arrancar
**N hilos concurrentes sin límite predefinido**. La VM los multiplexa
con su scheduler cooperativo + GC stop-the-world. Una app que controla
un robot tendrá fácilmente 10 hilos (lectura de sensores, control de
motores, comunicaciones, control loop…); eso es lo normal.

### Qué se sube al dispositivo y cuándo

Hay dos clases de módulos con frecuencias de actualización opuestas:

- **Módulos stdlib** (`Math.mod`, `IO.mod`, `Json.mod`, …):
  pre-instalados en el dispositivo. Sólo se suben cuando sale una
  nueva versión del lenguaje. Viven fuera del workdir, accesibles
  vía `--stdlibDir` (o `BpVM.cfg` `stdlibDir`). Es responsabilidad
  del administrador del dispositivo mantenerlos al día — el IDE
  del programador NO los gestiona en cada Run.

- **Módulos de la aplicación** (`App.mod`, `Util.mod`, …): generados
  por el compilador cada vez que el usuario hace cambios. Se suben
  al **workdir** de la VM (efímero / por sesión) por cada Run.
  Junto con sus `.bpi` (interfaces) y `.dbg` (info de debug).

El **workdir** es por tanto un directorio "de aplicación": vacío en
estado limpio, se llena con los `.mod` de la app que se va a ejecutar,
y opcionalmente con ficheros que la app cree en runtime. Cuando termina
la sesión, el workdir queda como esté — el IDE puede limpiarlo o
preservarlo según interese (logs de la app, por ejemplo).

Cuando la app importa `Math`, la VM busca primero `Math.mod` en el
workdir (no estará — el IDE no lo subió) y luego en `stdlibDir` (sí
estará — el admin lo instaló una vez). Si el dispositivo no tiene
el stdlib instalado todavía, el import falla con un error claro: el
usuario sabe que tiene que aprovisionar el dispositivo primero.

Esta separación cumple dos objetivos:

- **Eficiencia**: no hay que retransmitir megabytes de stdlib en
  cada Run. La app del usuario es pequeña; el stdlib puede ser
  comparativamente grande.
- **Responsabilidad clara**: el IDE habla con UN dispositivo a la
  vez como cliente "usuario" — sube su app y la ejecuta. Cómo se
  aprovisiona el dispositivo (stdlib, configuración de memoria,
  permisos) es un problema separado, que un día tendrá su propia
  herramienta de administración.

---

## La cascada de verificación: VM-Java → VM-C → VM-Pico

Tenemos tres implementaciones de la VM y las desarrollamos **siempre en el
mismo orden**: primero Java, luego C (host), luego Pico (firmware). No es
casualidad que funcione tan bien — es una **cascada de tamices**, donde cada
VM corre en un entorno estrictamente **más restringido** que la anterior:

- **VM-Java** — el entorno más rico: GC, memoria ~ilimitada, el mejor tooling
  y debugger. Es la **implementación de referencia**. Caza barato los bugs de
  **lógica y semántica** (despacho virtual, herencia, scheduler, try/catch…).
- **VM-C (host)** — el *mismo* algoritmo bajo las reglas de C: memoria manual,
  buffers fijos, big-endian explícito, sin GC del lenguaje host. Hereda la
  lógica ya probada en Java y caza los bugs de **portabilidad y representación**.
  Su contrato es la invariante central del proyecto: **stdout byte-idéntico**
  al de la VM-Java para el mismo `.mod`.
- **VM-Pico (firmware)** — C + las restricciones del micro: RAM ajustada,
  FreeRTOS, flash, transporte serie. Para cuando llegamos aquí, la lógica y la
  portabilidad ya están probadas dos veces; sólo queda lo **genuinamente
  hardware** (integración con el SO embebido y el transporte).

La consecuencia operativa, que hemos comprobado una y otra vez:

> **Lo que no funciona en Java tampoco funcionará en C; lo que no funciona en C
> tampoco funcionará en la Pico.** Cada capa es un superconjunto de las
> restricciones de la siguiente.

Por eso el orden Java→C→Pico ordena los bugs de **más baratos de cazar** a
**más caros**. Probar en la Pico es lo más costoso (flashear, cable, iterar a
ciegas); al llegar allí queremos tener garantías altas y reservar la placa para
validar sólo lo que *de verdad* es específico del hardware.

**Regla práctica**: ninguna feature de la VM se prueba primero en la Pico.
Se valida en Java, luego en C (con diff de stdout contra Java), y sólo entonces
se lleva al firmware.

### Extensión al debugger: paridad de *wire*

La misma disciplina aplica al protocolo de debug. La VM-Java (`DebugServer`,
A1) es la referencia del wire; la VM-C `--listen` debe producir un wire
**equivalente** para la misma secuencia de comandos, igual que iguala el stdout.

Matiz importante (la **regla de oro de H6**): la VM-Java está *fusionada*
(runtime + host, tiene el `.dbg`), mientras que la VM-C/Pico es **device puro**
(trabaja en pc/sp/bp/memoria cruda, sin `.dbg`). Por eso los mensajes que
cargan símbolos (p.ej. `BP_HIT` con `file`/`line`, o `serverName`) **difieren a
propósito**: el device los omite y el host (IDE) los rellena con el `.dbg` que
él tiene. La paridad de wire es entonces:

- **byte-idéntica** para lo agnóstico de rol (sobre todo `OUTPUT` = el stdout
  del programa, que es la paridad de siempre);
- **equivalente en comportamiento** para el control (mismos breakpoints
  alcanzados en los mismos puntos lógicos, mismos valores de locales), con los
  campos simbólicos como diferencia documentada device↔host.

Así el debugger del device se prueba en desktop (VM-C) antes de tocar la Pico,
y en la placa sólo validamos que el **transporte serie** mueve esos mismos bytes.

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
