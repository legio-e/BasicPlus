# V3 — cajón de ideas (informal)

> **Aún no es formal.** Es el cajón de V3: cuando se cierre V2 y se abra V3 se
> organiza en condiciones (como se hizo con V2). Iniciado 2026-06-07; la sección
> GUI se desarrolló el 2026-06-13 (charla Eduardo + Claude).

## Principios de V3

- **V2 es la base.** El lenguaje se mantiene (se añaden *eventos* y poco más),
  se corrigen bugs y se amplía de forma selectiva. Eduardo contento con el
  lenguaje tal cual.
- **Objetivo cabecera: una librería GUI.** No el único, pero el que manda.

## 1. GUI gráfica (objetivo cabecera)

### Decisión de arquitectura: un API `Gui.*`, varios backends de render

El contrato BP (`Gui.Screen`, `Gui.Button`, `Gui.Label`, eventos…) se define
**una sola vez**. Detrás, **render backends intercambiables**:

```
            un solo API  Gui.*   (contrato, en BP)
           /             |               \
      Java (PC)       LVGL+SDL          LVGL+TFT
   (miVM, ventana)   (VM-C, PC)       (VM-C, placa)
```

- **Placa:** LVGL sobre el display (objetivo real). LVGL = C, vive en
  `bpgenvm-c`. Se *envuelve* LVGL (no se reinventa): subconjunto razonable,
  incremental.
- **PC, VM-C:** el **simulador SDL de LVGL** enlazado en el host → "desarrolla
  en el PC, despliega en el micro" se mantiene también en la VM-C.
- **PC, miVM:** render en **Java puro** detrás del mismo `Gui.*`. Backend
  inicial **Swing/Java2D** (en el JDK, cero deps nativas). **Si conviene,
  migrar a JavaFX — que esa elección NO limite el diseño** (Eduardo, 13-jun):
  el `Gui.*` es el contrato estable y el toolkit Java de debajo es un detalle
  reemplazable. (JavaFX: scene graph acelerado, CSS, mejor para GUIs ricas y
  táctil; coste: ya no viene en el JDK → arrastra OpenJFX + sus nativos de
  render. Sigue siendo portable y sin LVGL. Swing: cero deps, más simple. Se
  decide al hacerlo, sin reescribir el API.)

### La verificación cruzada (paridad) revive — a nivel de comportamiento

El invariante dual-VM se **reformula** para GUI: no hay paridad de píxeles
(Java2D/JavaFX y LVGL rasterizan distinto), pero sí **paridad de lógica /
comportamiento** — mismo programa BP, mismos widgets, mismo estado, misma
secuencia de eventos en ambas VMs. Verificable con una suite que corra la app en
miVM y en VM-C y compare *eventos/estado*, no la pantalla. Las apps **sin** GUI
mantienen la paridad completa (byte-idéntica) intacta.

### La viga maestra: el upcall C→BP

Todo el modelo de eventos cuelga de **que C pueda llamar a una función/método
BP** (LVGL recibe el callback en C y "sube" al intérprete a ejecutar el handler).
Ya prototipado: el puente `call_bp` del AOT (#210, native→BP) hace justo eso; se
generaliza.

- Con el upcall, los **eventos por listener OO** (implementar una interfaz
  `Gui.ClickListener` / subclasear y override `onClick`) funcionan **sin tocar
  el lenguaje** — interfaces + dispatch virtual ya existen en V2. *Este* es el
  "eventos y poco más": casi nada nuevo en el lenguaje.
- **Funciones-como-valor** (§8, `CALL_INDIRECT`): azúcar ergonómico *opcional*
  encima (`btn.onClick(miFuncion)`); fase 2, no prerrequisito.
- El mismo upcall sirve a HW (interrupción→handler), timers y red: **§8/§9 es la
  pieza foundational de V3 y el GUI es su primer cliente.**

### Camino crítico

1. **Upcall C→BP robusto** (generalizar `call_bp`; serializar contra el lazo de
   LVGL, que no es thread-safe).
2. **Driver de display + táctil** (de los "un par de drivers más":
   ST7789/ILI9341, o el DSI del F769I-DISCO, + el táctil). Prerrequisito físico.
3. **Wrappers OO `Gui.*`** sobre el subconjunto LVGL, incrementales.
4. **Lazo de eventos** `Gui.run()` (sobre la cola que ya insinúan `SyncList` + el
   comm-task SMP). Con dual-core (#153), un núcleo dedicado a lo gráfico.
5. **Dos imágenes** (con/sin LVGL): los dispositivos sin pantalla no pagan RAM.
   GUI = feature de clase PSRAM/SDRAM (F769I-DISCO 4" táctil + 16 MB SDRAM ya
   disponible; Metro + PSRAM).

### Kits gráficos disponibles (validación cross-family)

Eduardo tiene **varios kits con pantalla, cada uno de una familia de fabricante
distinta** → banco de pruebas ideal para que el GUI sea de verdad cross-family
(lo que cambia por placa es el driver de display/táctil; el API `Gui.*` y LVGL
por encima, no). **Empezar por uno** — el **STM32F769I-DISCO** (4" táctil DSI +
16 MB SDRAM) es el mejor candidato — y, si va bien, **ampliar al resto**: misma
mecánica que el despliegue de micros en V2 (Pico → ESP32 → STM32). Alimenta el
#258 (árbol familia/micro/placa). *(Listar aquí los kits concretos cuando se
confirmen.)*

### Orden de trabajo (la senda que ya funcionó: VM-Java → VM-C → VM-micro)

Metodología de V1/V2: probar primero en Java (iteración rápida + debugger),
luego portar a la VM-C de host, luego al micro. Aplicada al GUI:

0. **(Decisión previa) Bake-off Swing vs JavaFX** para el backend Java del GUI —
   barato, decide con evidencia. JavaFX parte con ventaja (scene graph *retained*
   ≈ árbol de LVGL → más fácil mantener la paridad de comportamiento). **Matiz:
   la VM-Java NO se "migra"** (es headless, hoy no tiene GUI) — se le *añade* un
   backend `Gui.*` nuevo, en JavaFX o Swing según el bake-off; el intérprete no
   se toca. El único *migrar* de verdad es el **IDE** (Swing→JavaFX).
   **[BIFURCACIÓN ABIERTA, decide Eduardo]** IDE: ¿migración prerequisito duro, o
   diferida/oportunista? Recomendación: **diferida** — el IDE ya funciona;
   migrar "por si acaso" es coste/riesgo sin beneficio inmediato; se justifica
   solo si se quiere el mismo toolkit para un preview/diseñador de GUI embebido.
1. **miVM (Java)**: definir el API `Gui.*` + modelo de eventos. El upcall aquí es
   trivial (Java llama a su propio intérprete, sin JNI) → el sitio más barato
   para fijar la semántica del API y los eventos, con debugger.
2. **VM-C host**: LVGL + simulador SDL; aquí el upcall C→BP de verdad
   (generalizar `call_bp` #210). Arranca la paridad de comportamiento miVM↔VM-C.
3. **VM-micro**: LVGL sobre el TFT — Discovery primero (driver display+táctil,
   SDRAM, dual-core #153); luego ampliar a los demás kits (cross-family).

### Disciplina / coste

- `Gui.*` **pequeño** (mínimo común que LVGL y el backend Java puedan honrar) —
  restricción sana, muy "diseñar para el piso".
- Varios backends = mantenerlos en sync; se mitiga con la suite de comportamiento.
- Dial de diseño: cuanta más lógica de **layout** viva en BP portable, más
  paridad de comportamiento (a cambio de aprovechar menos el layout de LVGL).

## 2. Lenguaje (se mantiene; "eventos y poco más")

- §8 callbacks / función-valor (`CALL_INDIRECT`) — opcional, ver arriba.
- §9 eventos: HW interrupción→evento BP (Modelo B) + eventos GUI, sobre el upcall.
- Nada más previsto. Eduardo contento con la base del lenguaje.

## 3. Librerías estándar (mejorar / ampliar)

- **TCP/IP**: V2 deja un cliente simple. V3: WiFi en placa (lwIP, #145) +
  **servidor** (listen/accept), más API, ¿TLS/UDP/DNS? (Con un servidor HTTP
  mínimo, una "GUI web" sería un camino alternativo al LVGL para placas con red.)
- **Compresión**: subir de LZSS a deflate-lite (LZ77 + Huffman) y/o un formato
  Archive multi-fichero. Aislado, fácil de encajar.
- Lo que vaya surgiendo.

## 4. Hardware / dispositivos

- Un par de drivers más, según necesidad. El display + táctil del GUI ya cubre
  buena parte de la cuota.

## 5. Entorno IDE

- Base sólida; retoques pequeños y continuos.
- **Esfuerzo Windows + Linux**: Swing ya es cross-platform; los riesgos reales
  son el **puerto serie** (migrar purejavacomm → jSerialComm, más portable) y la
  fontanería (lanzador `.sh` además del `.bat`, rutas, asunciones de toolchain).
  Workstream propio, independiente del lenguaje/VM.
- Horizonte (stretch): un **diseñador visual de GUI** en el IDE que emita BP.

## Infraestructura que el GUI arrastra (movida desde V2)

- **Dual-core** (#153): un núcleo a lo gráfico para un rendimiento equilibrado.
  Con él viaja el **fix de B1** (la race solo aparece con paralelismo real; en V2
  mitigada a 1 worker, el device single-core es inmune).
- **Mapa de memoria configurable por PSRAM** (#225) y **FS grande** (#229):
  relevantes para framebuffers y recursos del GUI.
