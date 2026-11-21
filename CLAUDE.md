# BasicPlus — guía para Claude Code

> **Qué es este fichero.** El mapa mental del proyecto que Claude Code carga
> **en cada sesión**. Su razón de ser es que no haya que volver a explicar la
> visión de conjunto ni redescubrir los comandos cada vez. Si algo estructural
> cambia, se actualiza aquí. El **estado vivo** (qué está en curso, qué está
> cerrado, qué falta) NO va aquí: va en **`docs/FICHAS.md`**, que es la **fuente
> única de verdad** del proyecto.

## Qué es BasicPlus

Un **lenguaje de propósito general para microcontroladores de 32 bits** con
sintaxis BASIC moderna, orientación a objetos, GUI (LVGL) y un debugger de
primera clase. Compila a bytecode (`.mod`) que corre **idéntico** en el PC y en
el micro: el mismo programa se depura en el sobremesa y luego parpadea un LED en
una Pico 2, un ESP32-S3/P4 o un STM32 — sin recompilar, sin `#ifdef`.

Filosofía completa en `docs/PHILOSOPHY.md`. Esto es solo el mapa.

## EL INVARIANTE SAGRADO (léelo siempre)

El bytecode lo ejecutan **dos VMs independientes**, y su salida debe ser
**byte-idéntica**:

| VM | Lenguaje | Rol |
|---|---|---|
| **miVM** | Java | Implementación de **referencia**. PC: desarrollo, debug, CI. |
| **bpgenvm-c** | C99 | El *mismo* algoritmo bajo reglas de C. PC (host) + firmwares. |

**Regla de oro:** para el mismo `.mod`, el `stdout` de miVM y el de bpgenvm-c
tiene que ser **idéntico byte a byte**. Toda feature se verifica contra esto
antes de entrar. Es lo que hace real el "depura en el PC, despliega en el micro".

## LA CASCADA DE VERIFICACIÓN — orden obligatorio: Java → C → Pico

Las tres implementaciones se desarrollan **siempre en el mismo orden**, de bugs
más baratos de cazar a más caros:

1. **VM-Java (miVM)** — entorno rico, referencia. Caza bugs de **lógica/semántica**.
2. **VM-C host (bpgenvm-c)** — mismo algoritmo en C. Caza bugs de **portabilidad/
   representación**. Su contrato: `stdout` byte-idéntico al de Java.
3. **VM-Pico (firmware)** — C + restricciones del micro. Solo queda lo
   **genuinamente hardware** (RTOS, flash, transporte serie).

> **Lo que no funciona en Java tampoco funcionará en C; lo que no funciona en C
> tampoco en la Pico.** Cada capa es un superconjunto de restricciones de la
> siguiente.

**Regla práctica: NINGUNA feature de la VM se prueba primero en la Pico.** Se
valida en Java, luego en C con diff de stdout contra Java, y solo entonces al
firmware. Detalle en `docs/PHILOSOPHY.md` §"La cascada de verificación".

### Paridad del debugger (wire) — matiz importante
El protocolo de debug sigue la misma disciplina, con una excepción documentada:
la VM-Java está *fusionada* (runtime + host, tiene el `.dbg`); la VM-C/Pico es
**device puro** (pc/sp/bp/memoria cruda, sin `.dbg`). Por eso los campos que
cargan símbolos (`BP_HIT` con `file`/`line`, `serverName`…) **difieren a
propósito**: el device los omite y el host los rellena con su `.dbg`. Paridad
wire = **byte-idéntica** para lo agnóstico de rol (sobre todo `OUTPUT`/stdout) y
**equivalente en comportamiento** para el control. Ver `docs/BPVM_WIRE_PROTOCOL.md`.

## El sobre de plataformas (qué entra y qué no)

Objetivo: **MCU de 32 bits sin SO** (bare-metal o RTOS), del Cortex-M0+ humilde
al crossover ~1 GHz con PSRAM. **Se diseña para el dispositivo pequeño** (manda
el piso, no el techo); lo grande es headroom opcional, nunca requisito. **Fuera:**
64-bit y Linux embebido (MMU+DRAM+SO); 8/16-bit. Criterio fino en `PHILOSOPHY.md`.

- **Tres familias no gráficas** (a la par): **RP2350** (Pico 2 / Metro), **ESP32-S3**, **STM32**.
- **Con pantalla** (GUI LVGL): **ESP32-P4** y **STM32U5G9J-DK2**.
- Una **imagen única** por familia; variante/pines/panel se deciden en *runtime*
  (`/sys/board.json`), no con macros.

## Decisiones de fondo que se olvidan a menudo

- **Una app por VM** (no multi-app). Dentro de la app, **N hilos preemptivos de
  verdad** (no `async`), con scheduler propio + GC stop-the-world.
- **Módulos stdlib** pre-instalados en el device (fuera del workdir, vía
  `stdlibDir`); **módulos de la app** al workdir en cada Run. El IDE NO retransmite
  la stdlib en cada ejecución.
- **Memoria por handles (V4):** una referencia no es una dirección, es un handle
  con contador de generación → usar un objeto liberado **falla ahí mismo** en vez
  de corromper en silencio.
- **NO somos:** lenguaje de sistemas, ni buscamos máxima velocidad (no JIT), ni un
  ecosistema gigante (stdlib pequeña y ortogonal).

## Estructura del repositorio

```
toolchain/        SUBMÓDULO micro32-suite/toolchain — lo de Java:
  pack/           el formato de los packs (lo usan la VM-Java y el compilador)        [Maven]
  miVM/           VM Java + debugger + daemon TCP                                     [Maven]
  lexer-java/     compilador de BP y JC: .bp/.jc → .mod + .bpi + .dbg (+ AOT .mdn)   [Maven]
  BpIde/          IDE Swing (fat-jar)                                                 [Maven]
  tools/packtool/ podar y sellar un pack para una placa (lo llama wire_serie.py)
bpgenvm-c/        SUBMÓDULO micro32-suite/runtime — VM C99: host + firmwares (pico/esp32*/stm32) [make]
bpstdlib/         biblioteca estándar (.bp + .mod compilados)
samples/          programas de ejemplo (.bp / .mod)
compat/           la red: paridad dual-VM, MOD8, la cápsula de V6, la resistencia…
scripts/          el empaquetador, los generadores y las herramientas de docs y corpus
docs/             manual, specs (.mod, opcodes, heap, wire), backlog, ESTADO
```

Los **cuatro proyectos Maven**, en `toolchain/`, se construyen en este orden: `pack`, `miVM`
(depende de `pack`), `lexer-java` (de los dos) y `BpIde` (del compilador): en `BpIde` se hace la
"cirugía". `bpgenvm-c` se compila con `make`. (Las carpetas viejas —`lexer/`, `vm/`,
`generador/`, `h3bench/`, `diag/`, `bp_propuesta_modelo_memoria/`— salieron del árbol en la V8.1: siguen en la
historia, en `v8.0`.)

**Desde la V8.1 (#802), tres repositorios en la organización `micro32-suite`:** BasicPlus (éste: la
stdlib, los samples, la red, los docs con FICHAS y ESTADO, el empaquetador) y, como submódulos en las
mismas rutas de siempre, `toolchain` (en `toolchain/`) y `runtime` (en `bpgenvm-c/`). Un cambio de
código se commitea **DENTRO de su submódulo** y después, en BasicPlus, el commit que mueve el puntero
(`git add toolchain` o `git add bpgenvm-c`); uno que toca Java y C son tres commits. El push, de dentro
afuera: primero los submódulos, luego BasicPlus. Un clon nuevo, con `git clone --recursive`. Y ojo:
`git ls-files`/`git grep` desde la raíz no entran en los submódulos (`--recurse-submodules`).

## Comandos (PC)

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), make.

```sh
# Toolchain Java (el formato de los packs + VM Java + compilador), en este orden
mvn -f toolchain/pack/pom.xml install
mvn -f toolchain/miVM/pom.xml install
mvn -f toolchain/lexer-java/pom.xml install

# VM-C de host (LVGL=1 para la ventana de la GUI)
cd bpgenvm-c && make && cd ..

# Compilar un ejemplo y correrlo en AMBAS VMs
java -jar toolchain/lexer-java/target/basicplus-frontend.jar samples/blink.bp --compile samples --backend=mivm
java -jar toolchain/miVM/target/bpgenvm-1.0.jar samples/Blink.mod      # VM-Java
bpgenvm-c/build/bpgenvm-c samples/Blink.mod                            # VM-C

# El IDE
mvn -f toolchain/BpIde/pom.xml package
java -jar toolchain/BpIde/target/BpIde-8.1.jar
```

### Cómo verificar la paridad dual-VM (el gesto más repetido)
```sh
java -jar toolchain/miVM/target/bpgenvm-1.0.jar samples/X.mod > /tmp/java.out 2>&1
bpgenvm-c/build/bpgenvm-c                      samples/X.mod > /tmp/c.out    2>&1
diff /tmp/java.out /tmp/c.out && echo "PARIDAD OK"
```
Cualquier diferencia de `stdout` es un fallo del invariante, aunque el programa
"funcione".

## Dónde mirar antes de tocar el código (no reinventes)

Antes de bucear en el código, el doc correcto suele ahorrar el viaje:

- **Por qué / decisiones de diseño** → `docs/PHILOSOPHY.md`
- **🔒 QUÉ ESTÁ ABIERTO, CERRADO O EN CURSO** → **`docs/FICHAS.md`**. La fuente única
  de verdad: el registro numerado de fichas, con su commit al cerrarlas.
  **Léelo antes de listar pendientes** — y si otro doc lo contradice, manda éste.
- **Qué pasó en cada sesión** (diario, por fechas) → `docs/ESTADO.md`
- **Limitaciones del lenguaje de cara al usuario** → `docs/PENDIENTES.md`
- **Formato del bytecode** → `docs/MOD_FORMAT.md` · **Opcodes** → `docs/OPCODES.md`
- **Heap / memoria** → `docs/HEAP_LAYOUT.md` · **Builtins** → `docs/BUILTINS.md`
- **La RAM de las placas** (dónde se va; la C3 y la C6 van justas) → `docs/MEMORIA.md` · se mide con
  `bpgenvm-c/tools/medir_consumo.py`
- **Protocolo de debug** → `docs/BPVM_WIRE_PROTOCOL.md`
- **Referencia del lenguaje** → `docs/manual.html` · **Gramática** → `basicplus_grammar.ebnf.txt`
- **AOT / native functions** → `docs/AOT_*.md`
- **Roadmaps/backlogs por versión** → `docs/V{2,3,4,5}_*.md`

## Convenciones de trabajo

- **Idioma:** español (código, docs, commits, conversación).
- **Versión actual:** **V8.1, publicada el 21-nov-2026: la suite** (`#802`; tag `v8.1` en los tres
  repositorios de `micro32-suite`; su ZIP, `dist/Micro32-Suite-8.1-win.zip`, es el oráculo de «¿lo he
  roto yo hoy?»). No cambió nada del producto de V8 (20-nov, tag `v8.0`): la red y el Bluetooth, JC
  (BasicPlus con la ropa de Java y C#) y la ESP32-P4X; nueve imágenes, once placas. **Lo siguiente,
  V9, sin abrir** (sus hitos los nombra Eduardo): lo que V8 le pasa, en `docs/FICHAS.md` §ABIERTAS
  (lo que queda abierto en el bloque V8) y en `docs/PENDIENTES.md` («Para V9»); el cuaderno de V8,
  `docs/V8_IDEAS.md`. V7 se publicó el 13-oct (tag `v7.0`).
  **El proyecto entra en su madurez** (Eduardo, 20-nov): cada vez menos cambios internos;
  mejorar librerías y soportar más dispositivos, poco a poco, sin prisas.
- **`docs/FICHAS.md` es la fuente única de verdad.** Decisión de Eduardo (17-ago):
  *«Estado y pendientes son ficheros de trabajo tuyos. Pero el que dice realmente
  cuál es la situación es Fichas.»* Al abrir una ficha, una línea allí; al cerrarla,
  se marca con su commit y se queda. `ESTADO.md` (diario) y `PENDIENTES.md`
  (limitaciones) **no llevan estado de fichas**: si lo llevan, se corrigen ellos.
  El porqué está medido: cuando el estado vivía en tres sitios, se desincronizaban
  —dos bugs cerrados seguían apareciendo como abiertos, un censo hecho como
  pendiente— y costaba tiempo real.
- `docs/HECHO_V*.md` son snapshots **inmutables** de versiones cerradas: no se tocan.
- **Al terminar una sesión, actualiza `docs/FICHAS.md` (lo que cambió de estado) y deja
  el traspaso en `docs/ESTADO.md`** (lo que pasó)
  (qué quedó cerrado, qué está a medias, qué riesgo acecha), para que la siguiente
  sesión arranque sabiendo dónde estábamos sin depender de mantener el contexto vivo.
