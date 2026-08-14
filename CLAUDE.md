# BasicPlus — guía para Claude Code

> **Qué es este fichero.** El mapa mental del proyecto que Claude Code carga
> **en cada sesión**. Su razón de ser es que no haya que volver a explicar la
> visión de conjunto ni redescubrir los comandos cada vez. Si algo estructural
> cambia, se actualiza aquí. El **estado vivo** (qué está en curso, bugs, próximos
> pasos) NO va aquí: va en `docs/ESTADO.md` y `docs/PENDIENTES.md`.

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
lexer-java/   compilador (frontend): .bp → .mod + .bpi + .dbg (+ AOT .mdn)   [Maven]
miVM/         VM Java + debugger + daemon TCP                                 [Maven]
bpgenvm-c/    VM C99: host + firmwares (pico/esp32/esp32p4/stm32)             [make]
BpIde/        IDE Swing (fat-jar)                                             [Maven]
bpstdlib/     biblioteca estándar (.bp + .mod compilados)
samples/      programas de ejemplo (.bp / .mod)
docs/         manual, specs (.mod, opcodes, heap, wire), backlog, ESTADO
```

Los **tres proyectos Maven** son `miVM`, `lexer-java` y `BpIde` (donde se hace la
"cirugía"). `bpgenvm-c` se compila con `make`. (Hay carpetas legacy como `lexer/`,
`lexer2/`, `vm/`: no son las activas.)

## Comandos (PC)

Requisitos: JDK 8+, Maven, GCC (MinGW en Windows), make.

```sh
# Toolchain Java (compilador + VM Java)
mvn -f miVM/pom.xml install
mvn -f lexer-java/pom.xml install

# VM-C de host (LVGL=1 para la ventana de la GUI)
cd bpgenvm-c && make && cd ..

# Compilar un ejemplo y correrlo en AMBAS VMs
java -jar lexer-java/target/basicplus-frontend.jar samples/blink.bp --compile samples --backend=mivm
java -jar miVM/target/bpgenvm-1.0.jar samples/Blink.mod      # VM-Java
bpgenvm-c/build/bpgenvm-c samples/Blink.mod                  # VM-C

# El IDE
mvn -f BpIde/pom.xml package
java -jar BpIde/target/BpIde-4.0.jar
```

### Cómo verificar la paridad dual-VM (el gesto más repetido)
```sh
java -jar miVM/target/bpgenvm-1.0.jar samples/X.mod > /tmp/java.out 2>&1
bpgenvm-c/build/bpgenvm-c            samples/X.mod > /tmp/c.out    2>&1
diff /tmp/java.out /tmp/c.out && echo "PARIDAD OK"
```
Cualquier diferencia de `stdout` es un fallo del invariante, aunque el programa
"funcione".

## Dónde mirar antes de tocar el código (no reinventes)

Antes de bucear en el código, el doc correcto suele ahorrar el viaje:

- **Por qué / decisiones de diseño** → `docs/PHILOSOPHY.md`
- **Estado vivo, WIP, próximos pasos** → `docs/ESTADO.md`
- **Bugs y limitaciones abiertas** → `docs/PENDIENTES.md`
- **Formato del bytecode** → `docs/MOD_FORMAT.md` · **Opcodes** → `docs/OPCODES.md`
- **Heap / memoria** → `docs/HEAP_LAYOUT.md` · **Builtins** → `docs/BUILTINS.md`
- **Protocolo de debug** → `docs/BPVM_WIRE_PROTOCOL.md`
- **Referencia del lenguaje** → `docs/manual.html` · **Gramática** → `basicplus_grammar.ebnf.txt`
- **AOT / native functions** → `docs/AOT_*.md`
- **Roadmaps/backlogs por versión** → `docs/V{2,3,4,5}_*.md`

## Convenciones de trabajo

- **Idioma:** español (código, docs, commits, conversación).
- **Versión actual:** V4 (consolidación) cerrando; primeros trabajos de V5. Ver `ESTADO.md`.
- `docs/PENDIENTES.md` es el "diario honesto": bugs/limitaciones vivos (B/L/N/M).
- `docs/HECHO_V*.md` son snapshots **inmutables** de versiones cerradas: no se tocan.
- **Al terminar una sesión de trabajo, deja el traspaso en `docs/ESTADO.md`**
  (qué quedó cerrado, qué está a medias, qué riesgo acecha), para que la siguiente
  sesión arranque sabiendo dónde estábamos sin depender de mantener el contexto vivo.
