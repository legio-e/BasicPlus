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
- **1.2 Selección de backend en host** ✅ **HECHO 12-jul** (`1e7f1dc`): flag `--fs=lfs:<img>`
  (libc default). `samples/IoLfs.bp` byte-idéntico a 3 bandas (VM-C-libc == VM-C-lfs == miVM).
  BONUS: la suite destapó la regresión de pops de ref de H1.2a → tandas 1+2 (`c2fe54d`,
  `7270e40`) — 49 sitios arreglados en VM-C + Thread/Mutex en miVM; **resuelto el bug diferido
  de Thread de H1** (ThreadsMin.bp lo guarda).
- **1.4 Lock grueso de FS** ✅ **HECHO 13-jul** (`bd9c6a1`, adelantado por el requisito). Lock
  por OPERACIÓN COMPLETA en fs_lfs.c (wrappers; mutex de cintura pthread/FreeRTOS). ROJO→VERDE:
  sin lock littlefs ASSERTA (pcache) con 4 pthreads; con lock 0 corrupciones a 1/4/8
  (`test-fslfsmt`). BP-level `IoMt.bp` + `test-iomt`: 3 Threads BP × 4 ficheros, salida IDÉNTICA
  en 5 corridas (oráculo --smp=1/2/4 + libc + miVM), 0 líneas rotas. El bug de Thread NO afloró
  aquí porque ya lo había cazado la tanda 2. Gotcha BP: charAt devuelve string y `==` de strings
  compara refs → charCodeAt para contenido.
- **1.3 Directorios reales + `list` + montajes** ✅ **HECHO 13-jul** (`6b4b563`) — **CIERRA B1.**
  Op `list` (callback, sin allocs) en fachada + fs_lfs (bajo el lock) + fs_host (dirent).
  Montajes: tabla por prefijo (raíz + hasta 3; `bpvm_fs_mount("/sd", be)` para fase B), ruteo
  por prefijo más largo, cross-mount rename/copy → -1. `test-fsvfs` 33 asserts (jerarquía real,
  list exacto, resolve/base-dir H19 sobre el oráculo, stub en /mnt). Regresión completa verde.

> **🏁 BLOQUE B1 COMPLETO (13-jul).** El host es el oráculo funcional completo del FS que irá a
> placa: motor + fachada + selección por flag + lock multitarea + dirs/list/montajes, todo con
> tests permanentes. Siguiente: **B2** (RP2350 en Metro+Pico, flasheo intensivo — con Eduardo).

> **AUDITORÍA DEL CAMINO DE COMUNICACIONES (pregunta de Eduardo, 13-jul).** Los handlers del
> wire en los 3 firmwares (PUT/GET/DEL/COPY/LS+CRC en `repl_v1.c`/`repl_esp32.c`/STM32) llaman
> el API LEGADO `fs_get/fs_put/fs_delete` DIRECTAMENTE — se saltan la fachada. Los builtins BP
> van por la fachada → backend device → los mismos `fs_*`: dos caminos al mismo motor → la
> carrera comm↔worker ya existe en potencia HOY (mitigada por patrón de uso). **Decisión de
> diseño para B2:** (1) añadir `list` a la fachada (1.3); (2) migrar los handlers del repl a la
> fachada (PUT→write, GET→read, DEL→remove, COPY→copy, LS→list) → **el lock de B1.4 en el
> backend cubre a los DOS clientes por construcción**; (3) el zero-copy de `fs_get` (puntero al
> mirror) muere con el mirror — RUN carga el .mod copiando a RAM vía `read` (ya previsto §3 de
> la propuesta). El host no está afectado (comm_host solo drena output; el daemon no escribe
> ficheros). El bug de pops (tandas 1-2) NO toca el comm: no ejecuta bytecode.

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

---

## H5.c — EVENTOS (charla de diseño, 26-jul)

**Estado: DISEÑO EN CONVERSACIÓN. No se ha tocado código.** Método de siempre:
acotar antes de programar.

### Por qué hacen falta (planteamiento de Eduardo)

Hoy, para que una clase llame a una función, hay que importar y cablear la llamada.
Eso da dos problemas:

1. **Referencias circulares** en los imports.
2. **Solución fija**: si escribes una clase genérica, no sabes quién la va a usar,
   así que no puedes nombrar la función a la que llamar.

Hace falta algo **flexible y dinámico**: sin importar módulos desconocidos y sin
cablear una función concreta de forma permanente.

### El terreno: esto ya existe a medias en el GUI

`bpvm_resolve_handler` (`bpgenvm-c/src/builtins.c:366`) hace en cada clic: coge el
objeto, mira su `class_ptr`, busca en qué módulo cae esa dirección, compone la cadena
`"libreria.modulo.onClick"` y hace **búsqueda lineal por strcmp** sobre la tabla de
símbolos. Y `bpvm_call_bp_from_builtin` (`src/interp.c:235`) monta el frame, con la
restricción escrita *"la función BP no debe ceder al scheduler"*.

Limitaciones que mapean 1:1 con las preguntas de Eduardo: sólo llama a **funciones
libres** (nunca a un método, no hay `this`), exige que sean **públicas** (busca en
exports), y resuelve el módulo por el truco de "el objeto me dice dónde vive" — que
sólo funciona porque el handler está en el mismo módulo que la clase.

Es la prueba de que el mecanismo hace falta, y el inventario de lo que no repetir.

### Decisión de fondo: un evento no es una referencia a función

Es **una referencia a función MÁS su receptor**. Si el evento apunta a `obj.onTick()`,
quien dispara no sabe sobre qué objeto invocarlo — y no puede saberlo, precisamente
porque es genérico. El objeto viaja dentro del evento.

Con eso, los tres tipos de función colapsan en **un par `(receptor, destino)`**:

| receptor | destino | qué es |
|---|---|---|
| `0` | dirección absoluta de código | función de módulo / método estático |
| handle del objeto | slot de vtable | método de instancia |

Discriminante: `receptor == 0`. El segundo caso sale **polimórfico de regalo** —
guardar el slot y no la dirección hace que una subclase que sobreescriba reciba el
evento correcto.

**El CS NO se guarda: se deriva.** Las dos derivaciones ya están escritas y en
producción:

- desde el objeto: `class_ptr -> vtable[slot] -> bpvm_get_cs_for_data_addr(desc)`
  (es lo que hace `OP_INVOKE_VIRTUAL`, `src/interp.c:297`);
- desde la dirección absoluta: `bpvm_module_for_code_addr(vm, target) -> code_start`
  (es lo que hace el upcall del GUI, `src/interp.c:243`).

Por eso el destino es una **dirección absoluta y no un índice**: un índice de
ext-table sólo vale dentro del módulo que la posee, y un evento viaja — se crea en A
y se dispara desde B.

### Decisiones tomadas

1. **Un solo suscriptor** (Eduardo). Cubre la mayoría de casos y evita gestionar una
   lista. Puerta abierta a multicast en el futuro.
2. **El handler puede ser PRIVADO.** El compilador emite la referencia donde se
   *asigna* el evento, y ahí está dentro del módulo dueño. Obligar a hacerlo público
   sería reintroducir el acoplamiento que el evento elimina, y choca con la norma
   "público => property". Precedente en el código: `bpvm_aot_call_method_i32`
   documenta *"NO requiere que el método esté exportado (la vtable lo lleva)"*. La
   visibilidad protege el **nombre**, no la **dirección**.
3. **Sintaxis `obj::onTick` / `Modulo::procesar`** (Eduardo recordó el `::` de
   Java 8). Motivo propio de BP, no imitación: BP tiene **properties**, así que
   `obj.algo` sin paréntesis ya significa "lee la property" — y una property
   *ejecuta código*. El choque sería peor que en Java, que eligió `::` justo por no
   colisionar con los campos.
4. **Disparar ENCOLA; el thread dueño ejecuta** (modelo de interrupción). Ya es lo
   que hace el GUI: `lvgl_click_cb` no ejecuta BP, encola el objptr y el worker lo
   corre en `GUI_RUN` (`src/gui.c:178`). Elimina la restricción "no debe ceder al
   scheduler", que es una bomba de relojería, y sobrevive a SMP.
5. **Sin suscriptor => SILENCIO** (Eduardo). Razón de fondo: si disparar exigiera
   suscriptor, el emisor dependería del suscriptor — el acoplamiento que veníamos a
   romper. "Nadie escuchando" es un estado legítimo. Precedente: en Swing los eventos
   son notificaciones, no llamadas; si nadie escucha, no hay error. Se pierde poco:
   el nombre mal escrito, el método inexistente y la firma que no encaja los caza el
   **compilador**. El residuo mudo es sólo "olvidé escribir la suscripción", y eso no
   lo detecta ningún lenguaje.
6. **Un evento NUNCA devuelve valor** (Eduardo): *"Si queremos un valor lo suyo sería
   utilizar una llamada a una función normal, no un evento."* Con eso la semántica es
   **una sola** (notificación, void): no hacen falta dos verbos, ni un camino síncrono
   en la VM, ni decidir qué devolver cuando no hay nadie. La firma de un evento es
   siempre void, lo que además simplifica el tipado.

### Vida de los objetos: `owner` cambia el problema

Planteamiento de Eduardo: cerrar un suscriptor sin dar de baja las suscripciones es un
error de programación, pero también un rollo — el caso común es **una ventana con
decenas de widgets**, y al destruirla deberían destruirse todos (declarados `owner`).
Su preocupación: que el **orden** de destrucción no sea el adecuado y salte un evento
espurio.

Análisis. En Java/C# sólo hay dos malas opciones: referencia fuerte -> fuga y objeto
zombi que sigue recibiendo eventos; débil -> puede evaporarse en silencio. **BP no está
en ese mundo**: tiene un tercer modelo, el de los *arenas generacionales* — propiedad
explícita para decidir la vida, índice+generación para detectar lo rancio.

- `owner` es palabra clave del lenguaje (RAII). `OP_SET_FIELD_OWNER` libera el objeto
  anterior y `bpvm_handle_kill` **sube la generación en ese instante**.
- `bpvm_ref_dead` (`include/bpvm_internal.h:488`) compara la generación embebida en el
  handle con la del slot. **O(1)**, ya usado en cada acceso a campo y array (el
  «contrato B» que grita *"referencia a objeto eliminado"*).

Conclusión: el evento **no necesita ser una referencia débil**, porque el objeto muere
cuando su dueño lo suelta, apunte quien apunte. Se descarta la referencia débil, que
traía el riesgo de que el handler se evaporase sin avisar.

### Por qué la cola resuelve el miedo al orden

**La cola convierte un problema de ORDEN en un problema de VIVEZA — y el de viveza ya
está resuelto desde H1.** Si disparar encola, durante el desmontaje no se ejecuta nada;
los eventos se acumulan y, al drenar, la destrucción ya terminó y quién vive es un
hecho asentado.

- *Clic encolado justo antes de cerrar* -> al drenar, generación rancia -> se descarta.
- *Eventos que dispara el propio desmontaje* (LVGL manda `LV_EVENT_DELETE`, foco...) ->
  en ese instante el objeto aún vive, pero como sólo se encolan, al drenar ya están
  liberados y caen igual.
- *El handler de A toca el widget B ya liberado* -> con la cola, el handler de A no
  llega a ejecutarse durante el desmontaje.

Los tres con el mismo mecanismo, sin depender de acertar el orden.

Y la decisión 5 (silencio) **colapsa dos casos en uno**: "evento nulo" y "receptor con
generación rancia" hacen lo mismo. El despachador no distingue ni tiene política:
*¿hay destino vivo? Si no, vuelve.* Una comparación y un `return`.

### Riesgos abiertos (a cerrar antes de codear)

- **Si el cierre CEDE el control, la cola puede drenarse a medias.** Todo lo anterior
  se apoya en que el desmontaje corre entero sin ceder al scheduler. Si es largo,
  bloquea, o llama a algo que cede, la cola puede vaciarse con media ventana destruida
  y vuelve el problema del orden. Mitigación propuesta: marcar la ventana como
  «cerrándose» y que el despachador descarte lo suyo y lo de sus descendientes — que
  es lo que hace LVGL internamente con su bandera de borrado.
- **Raíces de GC.** El evento guarda un handle. Si vive en variable de módulo o dentro
  de otro objeto, el recolector tiene que marcarlo. Con el historial de UAF por refs
  mal contadas (y #20, que fue justo *"raíces GC de los upcalls"*), esto se decide en
  el diseño, no se parchea después.
- **Tipado.** Sin firma, un handler con la firma equivocada corrompe la pila en
  silencio. H5.a ya dejó el mangling de firmas: hay con qué comprobarlo al compilar.
  Falta decidir cómo se declara la firma del evento.

### Números del caso real

12 bytes por evento (handle de 64b + destino de 32b) **dentro del propio widget, sin
tocar el heap**. Cincuenta widgets = 600 bytes. Suscribirse no puede fallar por
memoria, y cerrar la ventana no dispara una cascada de liberaciones de nodos de lista
— porque no hay listas. Confirma que "un suscriptor y sin alocación" era lo correcto
para el caso de uso, no sólo lo simple.
