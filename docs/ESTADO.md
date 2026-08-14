# BasicPlus — Estado vivo

> **Documento vivo de traspaso entre sesiones.** Se lee al empezar y se actualiza
> al terminar. Aquí va lo que está EN CURSO y lo que ACECHA; los bugs formales
> viven en `PENDIENTES.md` y las decisiones de fondo en `PHILOSOPHY.md`.
>
> Convención de traspaso: al cerrar una tanda, anota en "Última sesión" qué quedó
> cerrado, qué a medias y qué riesgo hay abierto. Sé concreto (issue, fichero, prueba).

---

## Versión y foco actual

- **V4 — CERRADA y publicada** (release v4.0, 6-ago). Lo caro fue reestructurar la
  **gestión de memoria** (referencias por handle con contador de generación) y el
  **sistema de ficheros** (littlefs en las tres familias).
- **V5 — EN CURSO**, y corta por decisión: SQLite + ORM, el traslado al P4, los
  pendientes y cositas del IDE. Lo grande (partir común/hardware) es V6.
- ⚠️ **El registro vivo de V5 está FUERA del repo**: `notas/V5_BACKLOG.md` y
  `notas/V5_IDEAS.md` (commit `62d4b3b`: de la versión EN CURSO no se publica nada).
  Y ese backlog **se quedó el 12-ago**: H8 entero, la tanda H9 y todo lo del 13 y
  14-ago sólo constan en el `git log`. Si hace falta el detalle de esos dos días,
  está en los mensajes de commit, que son largos a propósito.

### Hitos de V5 cerrados

| hito | qué | cerrado |
|---|---|---|
| H1 | la Metro **lee** la tarjeta SD | 7-ago, en placa |
| H2 | la SD como **sistema de ficheros** (FatFs) | 8-ago, en placa |
| H3 | **SQLite corre en la Metro** (la tabla BIOS presta memoria) | 8-ago, en placa |
| H4 | un programa BP **consulta una BD de verdad** | 10-ago, salida idéntica al host |
| H5 | **el ORM**: DAO a mano → `@BD{...}` → generador → verificador | 11-ago |
| H6 | la SD del P4 por **SDMMC** + `LIST_DIR` a código común | 11-ago, en placa |
| H7 | **SQLite en el P4**: nativo RISC-V ejecutándose, motor arrancado y API publicada | 12-ago (falta grabar el pack: ver abajo) |
| H8 | *la herramienta antes que el artefacto*: relocalizador que coincide con `ld`, `sources`, un `.mod` y N `.mdn`, botón de grabar que relocaliza | 13-ago, en host |
| H9 | la tanda de **arreglos del compilador** (#384, #385, #386, #387, #388, #392, #393, #406) | **en curso** |

*(El nombre de H8/H9 no está en ningún doc: sale de los prefijos de commit. Ojo con
confundir el H9 de V5 con el H9 de V4, que era el kernel por capas.)*

## En curso / a medias

- **🔴 Trabajo del P4 sin commitear desde el 12-ago** (dos días parado). Son los
  pasos 2-3 de la lista "PARA MAÑANA" del backlog: inicializador común de la tabla
  BIOS + alta de `bpvm_bios_fs.c` en los dos builds ESP32 + el cargador de packs
  del P4. Ficheros:
  - **nuevos y ni siquiera dados de alta en git**: `bpgenvm-c/esp32p4/main/bios_p4.c`,
    `bpgenvm-c/esp32p4/main/pack_p4.c`.
  - modificados: `esp32p4/main/main.c` (+116), los dos `CMakeLists.txt` de ESP32,
    `esp32/main/board_mgr_esp32.h`, `include/bpvm_bios.h`, `pico/bios_pico.c`,
    `src/bpvm_npack.c`, `test/test_npack.c`, `pico/scripts/bpvm-pico.py`,
    `BpIde/.../PacksPanel.java`.
  - Nadie sabe hoy si eso compila en las cinco familias: se dejó a medias.
- **Lo que queda de H7**, por orden (backlog 12-ago): **1)** que el P4 **diga su
  dirección de packs** por el INFO — bloquea el sello y sin ella no se graba nada;
  **4)** sellar con la dirección de verdad, grabar y probar en placa; **5)**
  reconstruir y **re-verificar el pack ARM** (cambió al ganar `fabs`); **6)** los
  dos juegos de arquitectura en un solo pack.
- **#362 (recursos desde la zona de packs) no se ha probado en placa.** Verde en
  host: `make test-packres` 12/12, end-to-end con un PNG real y control en rojo,
  paridad dual-VM 28/28. Su propio commit lo dice: las cinco cinturas montan zona
  por el mismo `bpvm_pack_mount`, *"pero eso es un argumento, no una medida"*.
- **Metro (RP2350) y S3 compiladas y sin correr** desde el 12-ago.
- **H9 — lo que falta**: **#389**, el estrechamiento de `Object` a una clase no se
  comprueba (un DAO que dice devolver `Cosa` y devuelve `Otra` imprime **504**:
  basura plausible). Con él van dos bugs hermanos medidos el 13-ago: un `Object`
  con una cadena imprime **el handle**, y un `double` metido en un `Object` sale
  **0**. El diseño ya está decidido y escrito — `docs/OBJECT_COMODIN.md`, *"decidido
  13-ago, **sin implementar**"* — con el reparto: **envolver = librería** (una clase
  contenedora con `set` sobrecargado, posible desde que #387 arregló la sobrecarga
  cross-module), **desenvolver con seguridad = lenguaje** (sobre `instanceof`, #52).

## Riesgos / decisiones pendientes (atacar ANTES de seguir)

- **El check de `.pack` YA está duplicado** (esto corrige lo que decía la versión
  anterior de este documento: no vive sólo en el CLI de la VM-C). Comprobado el
  14-ago, la misma regla escrita a mano en cuatro sitios:
  `bpgenvm-c/src/bpvm.c:828`, `miVM/src/main/java/edu/bpgenvm/Main.java:378`,
  `BpIde/.../FrmMain.java:2556` y `:3165`, `BpIde/.../SimRunner.java:101`.
  Si el criterio cambia (mayúsculas, un pack sin extensión), hay que tocar los
  cuatro y el que se olvide no fallará al compilar. *(Cosa distinta, no confundir:
  la construcción del nombre en el frontend — `lexer-java/.../Main.java:1586,1682`
  y `PackStep.java:235`.)*
- **`dist/BasicPlus-4.0-win/packs/Stdlib.pack` queda rancio** respecto al layout
  nuevo de `Collections.Map` (#390: `layout 8 10` → `8 12`). Se dejó a propósito
  —es una distribución construida de V4, no una fuente— pero es exactamente el
  desfase de `.mod`/pack que ya ha costado tiempo otras veces. `packs/Stdlib.pack`
  sí se regeneró (`9f95e92`).
- **Sobrante que confunde:** `docs/390-private-wip.patch` (14-ago) es un borrador
  de #390 superado por el commit `0b258d3`; comprobado que **no aplica ni hacia
  delante ni al revés**. Borrarlo.
- **Ruido en las búsquedas:** `miVM/.claude/worktrees/jolly-blackburn-9ec483/`
  (29-jul) es una copia completa del repo — todo `grep` global sale por duplicado.
- **`docs/V4_SAMPLES_ROJOS.md`** (sin commitear) es un censo del **15-jul** (343
  samples: 225 verdes, 80 skip-HW, 36 rojos). O se rehace o se marca como
  histórico: tal cual, no dice el estado de hoy de nada.

## Bugs abiertos que rozan el invariante (resumen — detalle en PENDIENTES.md)

- **GAP-4** — `bpvm_format_double` (VM-C, `interp.c:321`) es byte-idéntico a
  `formatBpDouble` (Java) en punto fijo, pero la **notación científica** para
  `|x| >= 1e12` o `0 < |x| < 1e-6` es un TODO → las dos VMs podrían NO ser
  byte-idénticas en doubles extremos. Raro de disparar pero toca el invariante sagrado.
  *(Ojo: `%lld` no va en newlib-nano del STM32; hoy mitigado con `u64_dec`.)*
- **N-readfile-msg-skew** — el **texto** del `RuntimeError` de `readFile(ausente)`
  difiere entre VMs (`x` vs `no se pudo abrir`). Si un programa atrapa e imprime
  `e.msg`, la salida no es byte-idéntica. Alinear el wording en miVM y/o
  `bpgenvm-c/src/builtins.c`.

## Próximos pasos

1. **Ordenar el árbol de trabajo**: decidir qué entra del WIP del P4 (12-ago) y
   commitearlo o tirarlo. Es lo más viejo y lo que más estorba a cualquier tanda.
2. **H9: #389 y los dos bugs de `Object`**, con el diseño ya decidido en
   `OBJECT_COMODIN.md`. Empezar por la clase contenedora (librería), que no toca
   el compilador.
3. **Cerrar H7**: pasos 1, 4, 5 y 6 de la lista del backlog.
4. **Probar #362 en placa** (la zona de packs sirviendo recursos).
5. (Backlog) árbol perezoso del IDE con `LIST_DIR`; **#379** (timeout de INFO en el
   P4, arrastrado y se recupera solo); la media flash del P4 (aparcada a propósito:
   32 MB físicos, bootloader configurado para 16).

## Ideas aparcadas (no urgente)

- **Prueba de resistencia larga** (post-V4): dejar una placa corriendo días con
  carga VARIADA (no un bucle), con marca periódica en el log-anillo de flash para
  que "la muerte deje rastro". Antes, la versión barata en host. (Ver PENDIENTES.md.)

---

## Última sesión

<!-- Fecha — quién — resumen del traspaso. La entrada más reciente arriba. -->

- **2026-08-14 (tarde) — Eduardo + Claude.** Reescrito este documento con datos del
  repo en vez del andamiaje inicial. Dos cosas que decía y **eran falsas**:
  (a) **#310 no está abierto** — se cerró en V4 y está verificado en las tres
  familias (`16c7970`, `237c963`, `677bcf2` *"batería estándar del P4 al completo —
  8/8, y #310 en las tres familias"*); (b) el **check de `.pack` ya está duplicado**
  en cuatro sitios, no vive sólo en el CLI de la VM-C. Añadido lo que faltaba de
  los días 12-14: H8, la tanda H9, #390, #362 y #403.
- **2026-08-14 — Eduardo.** V4 dada por cerrada. En V5 se añadió soporte de SQLite
  y ha quedado bastante bien.
- **2026-08-14 — (andamiaje inicial)** — este ESTADO.md se creó a partir de
  `README.es.md`, `PHILOSOPHY.md` y `PENDIENTES.md`. Sus secciones "En curso" y
  "Próximos pasos" eran semilla sin verificar; corregidas arriba.
