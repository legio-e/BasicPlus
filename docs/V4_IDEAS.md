# V4 — cajón de ideas / bitácora de diseño (informal)

> Bitácora de diseño de V4 (charlas Eduardo + Claude). Iniciada 2026-07-11.
> El análisis de **H1** (modelo de memoria por handles) vive aparte en
> `bp_propuesta_modelo_memoria/` (spec + verificación + arquitectura). Aquí
> arranca **H2**.

---

## H2 — Sistema de archivos · FASE A: flash-FS con littlefs

**Estado: PLAN EN REVISIÓN (Eduardo). No se ha tocado código.**

### Encuadre
- **H2 = sistema de archivos.** Dividido en dos frentes:
  - **Fase A — flash interna (ESTE).** Un único FS de flash, igual en todos los micros.
  - **Fase B — SD/FAT.** **Aplazada indefinidamente a V5** (se decide cuándo más adelante).
    El andamiaje de la fase A se deja **general** (multi-montaje, multi-motor) para que B
    entre sin rediseño — pero *no* se construye ahora.
- **Propuesta del analista** (leída y aterrizada contra el código):
  `C:\lenguajes\bp-analisis\temas\sistema-archivos\arquitectura\propuesta-fase-a-flash-fs.md`
  (+ `mapa-terreno.md`, `prior-art.md`, `frente-2-fabricantes.md`, `contexto.md` 12 chunks).
- **Decisiones de Eduardo (11-jul):**
  1. **Plan en docs primero** (esta bitácora), revisar, y *luego* codear (analizar antes de programar).
  2. **El hito cierra con los 3 micros** (Pico/Metro + STM32 + ESP32) verificados sobre littlefs,
     no antes. *(La secuencia interna sí puede ir un-micro-a-fondo → batch de los otros dos, pero
     los tres están DENTRO del hito, no diferidos a otro.)*

### Estado actual del terreno (verificado en código, 11-jul)
- **Fachada [`bpvm_fs.h`](../bpgenvm-c/include/bpvm_fs.h) = ya es el VFS a nivel fichero** que se
  conserva: tabla de backend `stat/read/write/remove/rename/mkdir/rmdir/copy/isdir/mtime` +
  `bpvm_fs_resolve` + base-dir/proyecto (H19). **Es de FICHERO ENTERO** (`read(path,dst,cap)` /
  `write(path,data,len)`), sin `open/seek/stream`.
- **Tres backends stopgap divergentes (~1.100 líneas):** [`pico/fs.c`](../bpgenvm-c/pico/fs.c)
  420 (persiste a flash), [`stm32_fs.c`](../bpgenvm-c/stm32/port/stm32_fs.c) 339 (arena RAM 96 KB +
  región flash 128 KB, `MAX_FILES=40`, `NAME_MAX=64`),
  [`esp32/fs_ram.c`](../bpgenvm-c/esp32/main/fs_ram.c) 333.
- **Host:** [`fs_host.c`](../bpgenvm-c/src/fs_host.c) (libc → FS real), registrado en `test/main.c:118`.
- **littlefs NO vendorizado** · **no existe capa block-device** todavía → se crean.
- **Límites `#define`** (`FS_DATA_SIZE=128K`, `FS_FLASH_OFFSET` calculado, `MAX_FILES=40`,
  `NAME_MAX=64`) → la §6 los pasa a **config leída al arranque**. Enlaza con `/sys/device.json`
  (#134) y la jerarquía `/sys /lib /app` (#133); auditoría previa en `docs/FS_AUDIT.md` (#132).

### Arquitectura (de la propuesta)
```
 Programas BP ─► API de fichero (readFile/writeFile/list/mkdir/…)   ← NO cambia
   ┌───────────────────────────────────────────────┐
   │ VFS — fachada bpvm_fs.h (+ montajes, dirs reales) │  homogeneidad
   ├───────────────────────────────────────────────┤
   │ Motor: littlefs (flash interna) + lock grueso   │  robusto, RAM acotada
   ├───────────────────────────────────────────────┤
   │ Cintura block-device por micro: read/prog/erase/sync │  minimiza vendor
   └───────────────────────────────────────────────┘
   Pico flash_range_* · STM32 HAL_FLASH_* · ESP esp_partition_*
```

### Contrato de durabilidad de la fachada (pregunta de Eduardo, 12-jul: ¿flush para logs?)
- **Con la fachada de fichero-entero, el flush va implícito:** cada `writeFile`/`appendFile` es
  `open→write→close` DENTRO del builtin, y el `close` de littlefs **committea a flash**. Cada
  append cuya llamada retornó ya está grabado → si el micro se cuelga, el log queda íntegro
  hasta la última llamada completada (la línea a medias se pierde limpia, sin corromper —
  verificado por el barrido de cortes de B0.3). **No hace falta builtin `flush()` en fase A.**
- **Precio:** durabilidad-por-llamada = un commit littlefs por append (~ms + desgaste). Log
  parlanchín → acumular en RAM y appendear cada N líneas (patrón del programa BP, no de la VM).
- **Futuro (API de streaming,** open/write-chunk persistente entre llamadas, fuera de fase A**):**
  ahí SÍ se expone `flush()` → `lfs_file_sync` (committea sin cerrar). Queda apuntado.

### Tres aclaraciones que fijamos ANTES de codear
1. **Fachada de fichero entero, no stream.** La victoria de RAM de littlefs (F2) es matar el
   **mirror del FS entero** (p.ej. la arena RAM de 96 KB del STM32 vuelve), **no** el buffer
   por-fichero del llamante (sigue siendo 1 copia del tamaño del artefacto). **Streaming a nivel
   BP (`open/seek/read-chunk`) queda FUERA de fase A**; littlefs entra limpio *debajo* de las ops
   de fichero entero que ya existen.
2. **El eje de paridad se INVIERTE para el FS.** Todo lo demás es VM-Java ↔ VM-C byte-idéntico;
   el FS **no**: es **VM-C-PC (littlefs sobre fichero) ≈ VM-C-micro** (mismo motor, distinto
   block-device). El **oráculo** es la imagen littlefs en fichero, con **inyección de fallos**
   (corte de corriente, bloques dañados). *No* se fuerza dual-VM en el FS. → cambia el arnés.
3. **flash↔XIP (invariante 2) sigue siendo real con 1 solo worker de intérprete.** Las placas de
   2 cores corren 1 worker (core0=comm, core1=worker), pero **el core de comm ejecuta firmware por
   XIP desde la misma flash** → borrar un sector en el worker le corta el fetch → hay que pausarlo.
   Es `flash_safe_execute` (Pico) y **reutiliza el safepoint/STW que H1 acaba de construir**. Parte
   delicada → va la última.

### Descomposición en pasos committables
Cada paso: **committable + verificable**. Regla en cada uno: **verificar contra la emulación
(VM-C-PC) y (cuando toque) contra el micro**; no estrechar el andamiaje (dejar sitio a la fase B).

**Bloque 0 — Fundaciones host (oráculo), CERO placa**
- **0.1 Vendorizar littlefs** (`lfs.c` + `lfs_util.c` + headers) + integrarlo en el build host.
  *Verif:* test C mínimo `format+mount+mkdir+write+read+unmount` sobre buffer RAM.
- **0.2 Block-device en fichero** (host): `read/prog/erase/sync` sobre una imagen en fichero,
  geometría configurable (`block_size`, count). *Verif:* persistir + remontar + releer.
- **0.3 Inyección de fallos** en el block-device de fichero: corte de corriente (abortar a media
  escritura/erase) + bloques dañados. *Verif:* littlefs recupera sin corromper.

**Bloque 1 — VFS host sobre littlefs (el oráculo funcionando)**

> **REQUISITO EXPLÍCITO (Eduardo, 12-jul): el FS debe ser ROBUSTO EN MULTITAREA.** Hoy = 1 core
> con VARIOS Threads BP leyendo/escribiendo varios archivos (lo testeable ya); futuro = multitarea
> multi-core. Análisis: con 1 worker los builtins de IO se serializan solos (corren enteros en el
> quantum), **PERO la COMM TASK es un 2º cliente del FS** (PUT/LS/GET del wire, en placa corre en
> el OTRO core) → concurrencia real sobre littlefs YA en fase A, incluso con 1 worker → **el lock
> grueso NO es futuro-proofing, es necesario ahora**. En host se estresa de verdad: `--smp=N`
> (workers pthread) + thread de comm. Por eso **1.4 sube por delante de 1.3**. Garantía a nivel BP:
> **append atómico por llamada** (una línea de log jamás se entrelaza a medias con otra).

- **1.1 Backend `bpvm_fs` → littlefs** ✅ **HECHO 12-jul** (`a0802c3`; smoke B0: `b85e564`+
  `957c864`+`02b46ec`). Dos capas: `fs_lfs.c` portable (ops sobre lfs_t, semántica espejo de
  fs_host.c) + `fs_lfs_host.c` (filebd/.img, modo oráculo). 33 asserts + persistencia + mount-no-
  reformatea. Trazas littlefs silenciadas en la lib (romperían paridad). B0.3 = 84 escenarios de
  corte de corriente, 0 corrupciones.
- **1.2 Selección de backend en host:** libc = **default** (dev-loop intacto, corren los `.mod`
  como hoy); littlefs-en-fichero = **opt-in** por flag `--fs=lfs:<img>` (oráculo). *Verif:*
  dev-loop intacto + la suite BP de IO da los mismos resultados sobre el oráculo.
- **1.4 Lock grueso de FS** (ADELANTADO — requisito de multitarea). Mutex de plataforma por
  OPERACIÓN COMPLETA de la fachada en `fs_lfs.c` (no por llamada lfs_* suelta → ops atómicas,
  sin TOCTOU). *Verif en dos niveles:* (a) C-level: N pthreads martillean la fachada sobre el
  oráculo (control 1 thread, metodología test-smphandles); (b) BP-level: sample con varios
  Threads BP appendeando a ficheros propios + uno compartido, verificación por PROPIEDADES
  (conteos exactos + ninguna línea rota), con `--smp=1` (como placa) y `--smp=2/4` (estrés).
  OJO: puede aflorar el bug diferido de H1 (owner-alloc en Thread.run()) → si sale, se caza aquí.
- **1.3 Directorios reales** (`/sys /lib /app` como dirs littlefs) + **montajes** en el VFS
  (andamiaje multi-montaje/multi-motor). *Verif:* paths absolutos y relativos (base-dir H19)
  resuelven sobre dirs reales.

**Bloque 2 — Un micro de referencia a fondo**
- **2.1 Cintura block-device del micro de referencia** (sobre sus primitivas), absorbiendo su quirk.
  *Verif:* littlefs monta y opera en placa; **paridad VM-C-PC ≈ micro** en la suite de FS.
- **2.2 Descriptor de particiones leído al arranque** (datos, no `#define`) + "automático"
  calculado en PC. **Vive en un sitio FIJO conocido, FUERA/ANTES del FS** (no en `/sys/device.json`,
  que vive DENTRO del FS → huevo y gallina): un sector-descriptor a offset fijo en Pico/STM32; en
  ESP la tabla `esp_partition` ya lo es. La herramienta de PC lo escribe (conoce firmware + zona de
  packs → calcula el "automático"). *Verif:* el micro aprende su layout de ese sitio fijo antes de
  montar; redimensionar = reformatear.
- **2.3 flash↔XIP en el safepoint/STW de H1** (pausar el otro core durante erase/program).
  *Verif:* escrituras de FS concurrentes con ejecución no corrompen ni cuelgan; **en placa**.

**Bloque 3 — Cross-family batch + retirada (cierre del hito)**
- **3.1 Cinturas de los otros dos micros** (mismo patrón; quirks: STM32 quadword 16 B, ESP
  `esp_partition`). *Verif:* paridad VM-C-PC ≈ cada micro.
- **3.2 Retirar los 3 backends stopgap.** *Verif:* nada los referencia; los 3 firmwares compilan
  y corren sobre littlefs.
- **3.3 Regen de blobs stdlib embebidos + reformateo limpio** (bump de formato) en los 3.
  *Verif:* `/lib` reinstalado, `/app` re-subido, arranque limpio en las placas.

**Barra de cierre del hito:** los **3 micros verdes** + **huella de RAM medida** (debe caer
mucho vs el mirror) + **stopgaps retirados**.

### Arnés de verificación (paridad por emulación — reemplaza el diff dual-VM para el FS)
- **Oráculo:** VM-C-PC con littlefs sobre fichero. Aquí se cazan los **fallos lógicos** del FS
  (fácil depurar) → en el micro solo quedan fallos de **hardware**.
- **Robustez:** inyectar corte de corriente / bloques dañados en el block-device emulado → recupera
  sin corromper.
- **Concurrencia:** interleavings deterministas de threads BP en PC → sin corrupción bajo el lock.
- **RAM:** medir huella real de littlefs por micro vs el mirror actual (debe caer).
- **Corrección funcional:** mismas ops, mismo resultado en VM-C-PC y micro.

### Riesgos / partes delicadas (orden seguro → delicado)
1. **(Bajo, host)** Vendoring + block-device en fichero + inyección de fallos. Todo en PC.
2. **(Medio)** Integración de build de littlefs en **4 toolchains** (host make, Pico SDK/CMake,
   STM32 CubeIDE/Makefile, ESP-IDF/CMake). Bien trillado (littlefs está diseñado para esto).
3. **(Medio)** Descriptor de particiones + bootstrap en **Pico/STM32** (ESP ya tiene `esp_partition`).
4. **(Alto, placa)** **flash↔XIP** en placa (invariante 2). La parte genuinamente delicada; va la
   última, con la placa delante — patrón de siempre (lo de PC primero).

### Decisiones CERRADAS (Eduardo, 11-jul)
- **Micro de referencia = familia RP2350**, y se valida en **DOS placas: Metro (16M flash) + Pico 2**
  — mismo `.uf2` (imagen única), **dos geometrías de flash distintas con el mismo código**. Además
  del oráculo VM-C-PC≈micro, ganamos **Metro≈Pico** → caza bugs dependientes de geometría/tamaño sin
  abrir otra familia. (`fs.c` ya persiste a flash + `flash_safe_execute` nativo → flash↔XIP barato de
  validar primero.)
- **Descriptor de particiones: sitio FIJO fuera del FS, NO `device.json`** (device.json vive DENTRO
  del FS → no puede decir dónde está el FS). Ver 2.2.

### Decisiones aún abiertas (para afinar contigo)
- **Ubicación del vendoring:** `bpgenvm-c/third_party/littlefs/` (propongo) vs `vendor/`.
- **Mecanismo de selección de backend host:** flag `--fs=lfs:<img>` vs entrada en `BpVM.cfg`.
- **Formato del descriptor de particiones** (binario/JSON) y su offset fijo en Pico/STM32.
- **Tuning de littlefs por micro:** `cache_size`/`lookahead_size` acotados, más holgados en el P4
  (PSRAM); expuestos en config.
- **VM-Java / IDE:** la VM-Java **no** usa littlefs (sigue con el FS host, cómodo). Pero el explorer
  del IDE quizá quiera conocer dirs reales/montajes → anotar si hay que tocar el wire de listado
  (probablemente no en fase A; el wire de subir/bajar **no cambia**).

### Qué cambia y qué NO
- **Cambia:** 3 backends → 1 motor + cintura por micro · mirror RAM → streaming interno ·
  "dirs por prefijo" → dirs reales · tamaños `#define` → config.
- **NO cambia:** el **API de fichero** que ven los programas BP · la comodidad de la VM-Java con el
  FS host · el **wire de subir/bajar** · la paridad como regla sagrada (ahora vía emulación).
- **Migración:** reformateo limpio al bump de formato (`/lib` lo reinstala el firmware, `/app` se
  re-sube) — igual que el bump de `FS_VERSION` de hoy.

### Sinergias con lo ya hecho
- **H1 safepoint/STW** → lo reutiliza flash↔XIP (invariante 2). H1 acaba de construir esa maquinaria.
- **`/sys /lib /app` (#133)** → pasan de prefijos a dirs littlefs reales.
- **`docs/FS_AUDIT.md` (#132)** → auditoría previa del FS actual.
- **Nota:** `/sys/device.json` (#134) NO es el descriptor de particiones (vive DENTRO del FS; es
  config de runtime que consumen los programas BP). El descriptor de particiones es una capa por
  debajo, fuera del FS (2.2). Dos "configs" en dos capas distintas.
