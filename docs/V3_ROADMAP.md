# BasicPlus — Brújula de V3

> **V3 EN ARRANQUE (H0 — organización).** V2 cerrada y publicada el 2026-06-15
> (tag `v2.0`, repo público + Pages + Release). V3 arranca el **2026-06-16**.
> Esta es la brújula del ciclo **V3**: su tesis, los principios que la gobiernan
> y el instrumental que la verifica.
> Ideas y detalle de diseño: `docs/V3_IDEAS.md`. Brújula de V2: `docs/V2_ROADMAP.md`.
> El "sobre" de plataformas: `docs/PHILOSOPHY.md`.
>
> *Redactada 2026-06-16 (charla Eduardo + Claude). **Visión (§1–§4) cerrada;
> alcance (§5) y mapa de hitos (§6) PENDIENTES de la siguiente fase de H0.***

---

## 1. La tesis de V3

V1 probó la idea (lenguaje + 2 VMs en paridad byte-idéntica + 2 micros + IDE +
debugger). V2 la endureció, sumó una 3ª familia (STM32) y la sacó al mundo.

**V3 lleva el mismo bytecode soberano del territorio "texto + buses" al territorio
"gráfico + interactivo" — sin romper jamás la verificación cruzada de las 3 VMs.**

El **GUI gráfico** es el buque insignia, pero la prueba de fondo es más profunda:
demostrar que el modelo —*una plataforma abstracta idéntica en las 3 VMs; depura
en una, confía en las tres*— **escala** a algo mucho más rico que un `print`. Si
funciona con interfaces gráficas interactivas, el modelo queda validado para casi
cualquier cosa.

## 2. Principios rectores (la forma de decidir)

Cada principio trae su **test**: la pregunta que zanja "¿esto cuadra con el
conjunto?". Son los límites del ciclo.

1. **La verificación cruzada es la línea roja.**
   *Test:* ¿puedo comparar esto entre las 3 VMs (byte-idéntico, o por volcado del
   modelo)? Si no → se rediseña o no entra.

2. **Todo se añade con el patrón de 3 capas** (HW → HAL → puente → contrato `X.*`
   idéntico). Buses ayer, GUI hoy, lo que venga mañana.
   *Test:* ¿es un contrato `X.*` portable con la plataforma encerrada en el
   backend? Nada de hardware filtrándose al programa BP.

3. **El lenguaje está terminado; V3 es de *plataforma*, no de sintaxis.**
   "Eventos y poco más."
   *Test:* ¿esto pide tocar el lenguaje? La barra es altísima — casi seguro NO.

4. **El upcall C→BP es la viga única.** Eventos GUI, HW, timers, red: todos
   cuelgan de una sola pieza, bien hecha una vez (generalizar `call_bp`, #210).
   *Test:* ¿reusa el upcall, o inventa su propio mecanismo? Reusar.

5. **Cada port nuevo, más barato que el anterior.** V3 mete más silicio (kits
   gráficos, uno RISC-V). El coste de portar = el tamaño de `bpvm_port.h`.
   *Test:* ¿esto encoge el contrato de port, o lo engorda?

6. **Diseñar para el piso + soberanía del stack.** El device pequeño manda (GUI =
   clase PSRAM; dos imágenes, con y sin pantalla); el stack es nuestro (envolvemos
   LVGL, no nos casamos con él).
   *Test:* ¿lo grande se ha colado como requisito en vez de headroom? ¿añade una
   dependencia que no controlamos?

7. **V3 hereda V2: ampliar sin romper.** Un programa V2 corre **sin cambios** en
   V3. Dos ejes de compatibilidad:
   - *Horizontal (paridad):* las 3 VMs de una versión dan lo mismo. *(= principio 1)*
   - *Vertical (herencia):* la versión nueva ejecuta lo viejo. **Asimétrico** —
     VM3 hace todo lo de VM2, no al revés (como el `.class` de Java 8 en el JRE 17).
   *Test:* ¿esto rompería un programa V2? Entonces se hace **aditivo** (opcode /
   builtin / clase / versión-de-formato *nuevos*), nunca modificando lo existente.
   *Matiz:* compatibilidad del comportamiento **especificado** — los bugs se siguen
   arreglando (un bug no es contrato; contamos con que algún bug de V2 se nos coló).

## 3. El objetivo cabecera: GUI (resumen)

El GUI se modela como los buses: un contrato **`Gui.*`** (subconjunto reducido de
la API de LVGL) idéntico en las 3 VMs, con backend por plataforma — **VM-C
(host+micro) → LVGL**; **miVM → Java puro** reimplementando el subconjunto (NO un
puente JNI). **Paridad de comportamiento**, hecha operativa con un **volcado
textual del árbol de widgets** byte-idéntico (la verificación cruzada sobre el
*modelo de la UI* en vez del `print`; los píxeles quedan fuera). Eventos por
**upcall C→BP**. Orden de trabajo: miVM → VM-C host (LVGL+SDL) → micro (LVGL+TFT).

**Decisión consolidada y detalle completo: `docs/V3_IDEAS.md` §1.**

## 4. El arnés de no-regresión V2→V3 (instrumental del principio 7)

Lo que hace **verificable** la herencia, no un buen deseo. Se monta **antes de
tocar el núcleo**:

- **Oráculo dorado:** la **VM-Java V2 congelada** (`edu:bpgenvm:1.0` del tag
  `v2.0`, guardada como jar inmutable, p. ej. `oracle/bpgenvm-v2.jar`). Se ejecuta
  un `.mod` en el oráculo-V2 y en la VM-V3 y se hace *diff*. No se guarda *fuente*
  aparte (se pudre): el tag congela el fuente, el jar congela el comportamiento.
- **OpCodes congelados:** golden `nombre→id` de V2 + test que exige que V3 sea
  **superconjunto con los ids V2 intactos** (añadir sí; mover/reusar/recodificar
  no). Se apoya en `OPCODES.md`.
- **Guardián del `.mod`** — dos caras:
  - *Lectura (sagrado):* set de `.mod` V2 binarios + salida esperada → la VM-V3 los
    carga y ejecuta byte-idéntico. El contrato es el **loader** (la VM nueva lee lo
    viejo).
  - *Escritura (anti-accidente):* snapshot byte-a-byte del `.mod` que emite el
    frontend para fuentes fijas; un cambio **salta el test y pide permiso**
    (¿intencional o accidente?). + el disassembler (#163) como validador
    estructural contra `MOD_FORMAT.md`.
- **Regresión V2 verde:** todos los samples/tests de V2 corren verdes y
  byte-idénticos en las VMs de V3. **V3 no se da por bueno si rompe un solo
  programa V2.**

## 5. Alcance y cierre — PENDIENTE de decidir

> Un proyecto tiene **principio (hoy) y fin** — a poder ser, un **buen fin**
> (Eduardo, 2026-06-16). *"Llegamos hasta aquí en esta versión; si seguimos, se
> pasa a V4."*

Antes de cerrar H0 hay que **acotar el alcance de V3**: hasta dónde llega y qué
queda para V4, para que V3 tenga un final digno y no se arrastre. **A decidir en
la siguiente fase de H0.**

Candidato natural de cierre (a confirmar): el **GUI corriendo cross-family** en
los kits con pantalla — el mismo patrón con que el ESP32 cerró v1 y el STM32
cerró v2.

## 6. Mapa de hitos — PRELIMINAR (a formalizar tras §5)

Esqueleto candidato, ordenado por dependencias. **No cerrado** — se formaliza al
fijar el alcance.

- **El arnés de no-regresión (§4) primero** — la red antes del trapecio.
- **GUI** (el camino crítico): fundación de eventos (upcall + `Gui.*` en miVM) →
  GUI en VM-C host (LVGL+SDL) → portabilidad + observabilidad de bring-up → GUI en
  micro (1er kit STM32F769I-DISCO) → rollout cross-family (ESP32-P4 RISC-V,
  Metro+DVI).
- **Infra que el GUI arrastra:** dual-core RP2350 (#153) + fix B1; mapa de memoria
  por PSRAM (#225); FS grande (#229).
- **Ampliación selectiva:** WiFi en placa + servidor TCP (#145); IDE Win+Linux
  (`jSerialComm`).
- **Cierre:** documentación + publicación v3.0.

*(**H1** se detallará a continuación — candidato: el arnés, o la fundación de
eventos.)*

## 7. Próximo paso

Cerrar H0: (1) decidir el **alcance** de V3 (§5); (2) formalizar el **mapa de
hitos** (§6); (3) detallar **H1**. La visión (§1–§4) queda fijada como base
estable para decidir todo lo demás.
