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

### La firma del handler (2ª sesión, 26-jul) — CERRADA

Decisión de Eduardo: **una firma única para todos los eventos**. Con eso desaparece
la pregunta del tipado que quedaba abierta arriba: si todos los eventos tienen la
misma forma, no hay nada que declarar ni que comprobar, y el mangling de H5.a no
entra en esto.

```
handler(sender: Object, kind: <constante del compilador>, args: Object)
```

#### 1. `sender` — quién dispara

El objeto que emite: el botón, el timer, la conexión. Cubre la necesidad más común
("¿cuál de los treinta botones ha sido?") y, sobre todo, **hace innecesarios los
args en la mayoría de casos**: si el estado vive en el widget, el handler pregunta
`sender.text()` / `sender.value()` / `sender.selectedIndex()`. Es lo que ya hace el
callback de cambio del GUI (*"refleja el estado del widget en el modelo"*,
`src/gui.c`) y lo que hace Swing con `getSource()`.

Va tipado como `Object`: el precio es un `instanceof` o un cast en el punto de uso
cuando un handler sirve a varias fuentes.

#### 2. `args` — el payload, `Object` y casi siempre nulo

Recorrido de la decisión, porque el camino importa:

- **Descartado `Map`** (existe en `Collections.bp:203`). Su constructor crea
  `keyList` + `valList` + `StringComparator` + **`Mutex`**: cinco objetos vacío, y
  con dos entradas más dos strings de clave y dos cajas `Integer` ⇒ **nueve objetos
  de heap y un mutex del SO por cada clic**. En un Pico con ~125 KB libres y en el
  camino caliente, insostenible. Peor aún el lado C: hoy `bpvm_gui_inject_click`
  encola un `objptr` y ya está; con Map, la VM en C tendría que alocar el grafo,
  boxear valores y enlazarlo todo **en las dos VMs y byte-idéntico** para la
  paridad — alocando en pleno despacho de eventos, que es donde vivieron los UAF
  de V4.
- **Convención de Eduardo: un string `"key1:valor1, key2:valor2"`.** El lado C pasa
  de construir un grafo de objetos a hacer un `snprintf` + **una sola llamada a
  `bpvm_heap_alloc_string`**, que ya existe. Además el payload viaja gratis por el
  wire, al log y al depurador, y es legible al depurar.
- **Tipo declarado: `Object`, no `string`.** Cuesta lo mismo (una referencia) y
  quita el techo del string, que tiene tres límites reales: los valores pierden el
  tipo (un float formateado y re-parseado no es el mismo valor), los separadores se
  rompen con texto libre (`"texto:Hola, ¿qué tal?"` parte mal por la coma), y **no
  puede llevar un objeto** — el `sender` cubre uno, los args no podrían cubrir un
  segundo.

⇒ **Lo normal: `null`, sin alocar nada. Lo sencillo: el string `"k:v,k:v"`. Lo raro:
cualquier objeto.** La convención se recomienda; el lenguaje no la impone.

Si algún día se reutiliza el buffer de args entre disparos (optimización posible,
el despachador es uno), hay que fijar la regla: **`args` sólo vale durante el
handler; si te lo quieres guardar, cópialo** — la misma regla que el puntero de
`fs_get`.

#### 3. `kind` — el tipo de evento (idea de Eduardo, del modelo de Windows)

El `uMsg` del `WndProc` no está sólo para saber cómo leer `wParam`: está para que
**un solo handler atienda muchos eventos**. Ése es el punto, y es justo el caso de
"una ventana con decenas de widgets": un método que los atiende a todos
discriminando por `(sender, kind)`, en vez de treinta métodos.

Nótese que en el caso simple es redundante — si suscribes `boton.onClick :=
this::pulsar`, dentro de `pulsar` ya sabes que fue un clic. Aporta cuando el handler
sirve a varias fuentes.

**Tipo: constante que emite el COMPILADOR**, cualificada por módulo. Descartadas:

- *enteros pelados* (Windows): números mágicos y dos librerías pueden chocar
  (Windows lo tapó con `WM_USER` como frontera, que es convención, no garantía);
- *enum*: tipado y barato pero **cerrado** — si vive en la stdlib, una clase de
  usuario no puede añadir sus eventos, y eso reintroduce el acoplamiento.

El compilador sabe qué evento se dispara, así que emite él la constante: nadie
escribe números, no hay colisiones posibles, en ejecución es un entero, y el handler
compara contra un nombre (`if kind == Boton.onClick`). Encaja con lo que ya hace el
compilador cualificando símbolos por módulo.

#### 4. A + B: eventos con nombre **y** cajón de sastre (Eduardo)

Se planteó como disyuntiva y Eduardo la resolvió con las dos:

- **(A) cada evento un campo con nombre** — se lee solo, suscribirse a un evento
  inexistente es error de compilación; cuesta 12 bytes por evento *declarado*, se
  use o no (un Button con 6 eventos = 72 B/instancia; 50 widgets = 3,6 KB);
- **(B) un solo campo + `kind`** — 12 B por widget (600 B en la misma ventana), pero
  pierdes el nombre autoexplicativo, todo handler empieza con un `switch`, y con un
  solo campo y un solo suscriptor **quien se apunta se lo lleva todo**.

**A+B**: uno o dos eventos con nombre para lo normal, y un **cajón de sastre** para
todo lo demás. Es a lo que converge todo el mundo: LVGL tiene códigos concretos *y*
`LV_EVENT_ALL`; el DOM tiene `onclick` *y* `addEventListener`; Windows tiene mensajes
concretos *y* `DefWindowProc`.

Si el cajón de sastre vive en la **clase base** de los widgets, lo heredan todos por
12 bytes y cada widget concreto añade su uno o dos ⇒ **1,2–1,8 KB** en la ventana de
cincuenta widgets, entre los dos extremos. Y una clase de usuario que invente un
evento nuevo **no necesita campo nuevo**: dispara el cajón de sastre con su propia
constante. Ahí es donde paga la constante cualificada por el compilador.

#### 5. Un clic dispara LOS DOS, primero el específico

La decisión semántica que acompaña a A+B. **Ojo: el modelo de Windows NO está
disponible** — *"llama al específico; si no lo trata, pásalo al general"* exige que
el handler pueda decir "ya me he ocupado", y eso es un valor de retorno, descartado
en la decisión 6. Hay que decidirlo estáticamente.

Se elige **disparan los dos, el específico primero**:

- un cajón de sastre que a veces no ve las cosas no es un cajón de sastre; si el
  general dejara de enterarse *porque otro se suscribió al específico*, el
  comportamiento dependería de quién más se suscribió — el acoplamiento invisible
  que veníamos a quitar;
- así el general **siempre** ve todo, lo que habilita usos reales que si no se
  pierden: un trazador de interacción, un log de depuración, un panel de actividad;
- coste: el despachador mira dos campos en vez de uno.

Consecuencia bonita: como el específico también recibe `kind` (redundante ahí, pero
mantiene **una sola firma**), el mismo método puede servir de handler específico de
un botón y de cajón de sastre de la ventana sin cambiar nada.

#### Criterio de aceptación (Eduardo, al cerrar)

> *"Creo que hemos llegado a una buena solución. Y lo creo porque es sencilla, es
> barata y es abierta."*

Las tres, comprobables: **sencilla** (una firma, sin declaraciones de tipo, sin
registro central), **barata** (12 bytes por evento, cero alocación en el camino
común, un `snprintf` en el lado C), **abierta** (`args` admite cualquier objeto, el
cajón de sastre admite eventos que aún no existen, y nada de esto obliga a tocar la
stdlib para añadir uno).

#### Estado

Cerrado: el par (receptor, destino), un suscriptor, handler privado, `::`, cola,
silencio sin suscriptor, sin valor de retorno, la firma única, `args` abierto,
`kind` del compilador, A+B, y disparo doble.

**Sigue abierto (implementación):** el desmontaje que cede el control a mitad, y las
raíces de GC del receptor. Son las dos que hay que resolver antes de escribir código.

### Declaración y acceso — CERRADO

**`event` es palabra reservada** (Eduardo). No es azúcar sobre `property`: es una
clase de miembro con su propia regla de acceso.

**Quién dispara: el objeto de la clase, y nadie más.** En palabras de Eduardo, *"un
evento es más bien una propiedad: la asignación es pública mientras que la referencia
(la llamada) es privada"*. O sea, **acceso asimétrico**:

| operación | quién |
|---|---|
| asignar (suscribirse / desuscribir con `null`) | cualquiera, desde fuera |
| leer y **disparar** | sólo la clase (y sus descendientes) |

Se llegó por un camino que conviene registrar, porque desmonta una objeción:

1. Se verificó que **BP no tiene `protected`** (el lexer sólo reconoce `public`) y que
   `PropertySymbol` lleva **un único `isPublic`** — sin visibilidad por accesor. ⇒ la
   asimetría no se puede expresar hoy con nada, y eso **justifica** la palabra propia
   en vez de resolverlo con modificadores sobre `property`.
2. Se planteó una objeción: si disparar es privado *de la clase que lo declara*, el
   cajón de sastre vive en la clase base y **`Button` no podría dispararlo** ⇒ A+B roto.
3. **Eduardo lo desmontó**: en BP lo implícito (sin `public`) *no* es el `private` de
   Java, es su `protected` — privado de cara al exterior, accesible en clase y
   subclases. **Verificado en `SemanticAnalyzer.checkVisibility` (~línea 2988):**

```java
if (currentClass != null && owner != null
        && (currentClass == owner || currentClass.isSubclassOf(owner)))
    return;                                    // <- la subclase pasa
if (!isPublic)
    err(line, col, "miembro privado '" + sub.name + "' inaccesible aquí");
```

⇒ La objeción no existía: `Button` dispara el cajón de sastre de `Widget` **por la
visibilidad que ya hay**, sin regla nueva. Y `event` queda como **lo implícito de
siempre + un setter público**: un bit de asimetría, no una tercera categoría de
visibilidad.

**Consecuencia aceptada:** con asignación pública y un solo suscriptor, cualquiera
puede **pisar la suscripción de otro en silencio** (el último gana). Es inherente a
"un suscriptor" — no hay alternativa salvo que asignar sobre un evento ya ocupado se
queje. Queda documentado como comportamiento, no como bug.

**Desuscribirse** = asignar `null`, consistente con que suscribirse sea asignar.

---

### Estado final del diseño (26-jul)

**Lenguaje: CERRADO.** Par (receptor, destino) · 1 suscriptor · handler privado · `::`
· cola · silencio sin suscriptor · sin valor de retorno · firma única
`(sender, kind, args)` · `args` `Object` y casi siempre nulo · `kind` del compilador ·
A+B con cajón de sastre en la clase base · disparo doble (específico primero) ·
`event` como palabra reservada con asignación pública y disparo de clase+subclases.

**Abierto: sólo implementación de VM** (se resuelve mirando el código, no decidiendo):
1. el desmontaje que **cede el control** a mitad ⇒ la cola se drena con media ventana
   destruida (mitigación: bandera «cerrándose», como LVGL);
2. **raíces de GC** del receptor del evento.

### Los dos puntos de VM — investigados contra el código (26-jul)

#### 1. Raíces de GC — RESUELTO, sin tocar el GC

El recolector marca los campos de un objeto leyendo el **bitmap de campos** del
descriptor de clase (`src/heap.c`, rama `BPVM_TYPE_OBJECT`):

```c
for (uint32_t i = 0; i < num_fields; i++) {
    uint32_t word = read_u32(fbm_base + (i / 32) * 4);
    if (word & (1u << (i & 31))) {
        /* campo ref = 8 bytes (2 slots, bit en el slot BASE) */
        uint32_t slot = (uint32_t) read_i64(user_ref + 4 + i * 4);
        mark_recursive(vm, slot);
    }
}
```

El bitmap es **por slot de 4 bytes**, y una referencia ocupa 2 slots con el bit
puesto en el **slot base**. Un campo de evento son 12 bytes = **3 slots**:

| slot | contenido | bit |
|---|---|---|
| i, i+1 | receptor (handle de 64b) | **1** en el slot i |
| i+2 | destino (dirección o slot de vtable) | **0** |

Es exactamente expresable con lo que hay: el GC marcará el receptor e ignorará el
destino — que es justo lo que hace falta, porque el destino **no es una referencia**
y marcarlo sería corromper el marcado.

**El caso heredado también está cubierto**, que era mi duda con el cajón de sastre en
la clase base: `computeClassLayout` construye el layout del hijo **a partir del de su
base "viva donde viva"** (`ModuleInterface.java:588`), usando `publishedLayoutOf` para
bases cross-module. Así que los bits de los campos heredados están en el bitmap del
hijo, también cuando la base vive en otro módulo (p.ej. `Widget` en la stdlib y
`Button` en el módulo del usuario).

⇒ **Trabajo: sólo de compilador.** Colocar el campo del evento en el layout y poner el
bit en el slot del receptor. Y como #299 dejó el layout de clase en **una sola
función**, es un sitio, no diecisiete.

#### 2. El desmontaje que cede el control — NO se materializa hoy, pero destapa una DECISIÓN

`Gui.run()` es un **builtin** (`BUILTIN_GUI_RUN`, `src/builtins.c:671`) y el lazo de
eventos entero vive dentro de él:

```c
for (;;) {
    while ((objptr = bpvm_gui_next_event(&kind)) != 0)
        bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u);   /* ejecuta el handler BP */
    ...
    bpvm_gui_lvgl_pump();
}
```

La prueba de que el scheduler no puede intercalarse está escrita ahí mismo, en el
comentario de #257: *"el scheduler no corre quanta mientras este builtin bombea, así
que poleamos el wire aquí mismo"* — tuvieron que polear el KILL a mano precisamente
porque el planificador no entra.

⇒ Si un handler llama a `ventana.close()`, el desmontaje corre **anidado dentro del
propio lazo de drenaje**, que está suspendido en la llamada al handler. **El drenaje
no puede reentrar**, la cola sólo acumula, y se vacía cuando el handler retorna. El
miedo al orden no se materializa.

**Pero eso es hoy, y por un motivo que queríamos quitar.** Los handlers corren dentro
de un builtin vía `bpvm_call_bp_from_builtin`, que lleva escrito *"Restricción v1: la
función BP no debe ceder al scheduler"*. Es decir: no hay intercalado **porque el
handler tiene prohibido ceder**. Y el modelo de cola se eligió justamente para quitar
esa prohibición y que un handler pueda ceder, bloquear o lanzar excepciones.

**DECISIÓN PENDIENTE (de Eduardo): ¿dónde se drena la cola de eventos?**

- **(a) Dentro de un builtin, como hoy.** Cero intercalado por construcción, el
  desmontaje es atómico gratis y no hace falta bandera de «cerrándose». A cambio, la
  restricción de no ceder **se queda** — y con ella el problema que el modelo de cola
  venía a resolver. Sirve para el GUI; no sirve para eventos de un timer, de la red o
  de un thread cualquiera.
- **(b) A nivel de scheduler**, drenando en el thread dueño entre quanta. El handler
  es código BP normal: puede ceder, bloquear y lanzar. Es lo que hace falta para que
  los eventos sean generales y no una pieza del GUI. A cambio **vuelve el intercalado**
  y hay que implementar la bandera de «cerrándose» (descartar lo de la ventana y sus
  descendientes mientras se desmonta), como hace LVGL.

Mi lectura: (b) es lo coherente con "esto sirve para ahora y para el futuro" — un
evento no debería ser una cosa del GUI. Y el coste, la bandera, ya lo teníamos
identificado y tiene prior art. Pero (a) es hoy gratis y funciona, así que es una
decisión real y no un trámite.

#### Dónde se drena la cola — DECIDIDO: un mecanismo, no dos

Eduardo preguntó si se podían tener las dos opciones o una combinación. Sí, pero
**no como dos mecanismos: como uno, dejando que (a) deje de ser un caso especial.**

**Aviso que motivó el encuadre:** dos caminos de despacho es justo lo que ha mordido
dos veces en este mismo proyecto — `repl.c` con dos protocolos (el de texto acabó con
su propia resolución de módulos divergiendo en silencio) y el FS con dos caminos al
mismo motor. Si el GUI drenara de una forma y el resto de otra, en unos meses uno de
los dos tendría un arreglo que el otro no.

**Lo decidido:**

- **El motor es (b):** el drenaje vive en el punto **entre quanta** del scheduler, que
  YA EXISTE — `vm->poll_cb` en `src/scheduler.c:87` y `src/scheduler_smp.c:107`. Los
  handlers son código BP normal en el thread dueño: **pueden ceder, bloquear y lanzar
  excepciones**.
- **`Gui.run()` pasa a ser un CLIENTE, no un lazo de eventos.** Vuelve a significar lo
  que dice: *bloquea este thread hasta que se cierre la ventana*, bombeando LVGL y
  cediendo. **Deja de ejecutar handlers.**

**Ganancia que no es menor:** con eso `bpvm_call_bp_from_builtin` **desaparece del
camino de eventos** — y esa función es dos cosas a la vez: la fuente de la restricción
*"la función BP no debe ceder al scheduler"*, y donde vivió el bug de raíces de GC de
#20 (upcalls del GUI).

**El detalle que decide si es UN mecanismo o dos disfrazados de uno:** la restricción
de ceder **no viene de quién drena, viene de dónde está el frame**. Si el builtin
llamara a la misma función de drenaje, heredaría la restricción igual, porque el frame
sigue anidado dentro de un builtin. Por tanto la regla es dura: **el builtin no
ejecuta handlers en absoluto**, sólo bombea y cede.

**Latencia — descartada como preocupación (Eduardo).** Los 50 ms del `poll_cb` son el
**tope de la espera con el scheduler OCIOSO** (`if (poll_cb != NULL && dt > 50) dt = 50`),
no la latencia normal: con trabajo pendiente el drenaje cae en el cambio de quantum.
Razonamiento de Eduardo: *"Para el Gui no afecta. En lo único que puede afectar es en
hardware y aquí sabiendo que estamos en BP también sabemos que tenemos cierta
limitación de rendimiento. Lo normal si necesitamos mucha velocidad es que pasemos a
hacerlo por pull."* Un evento es para *avísame cuando pase algo*, no para un lazo de
control; lo que necesita velocidad va por sondeo o baja a `native`.

**Consecuencia:** al drenar a nivel de scheduler **vuelve el intercalado** durante el
desmontaje ⇒ hace falta la **bandera de «cerrándose»** (descartar los eventos de la
ventana y sus descendientes mientras se desmonta), como hace LVGL internamente. Estaba
ya identificada como mitigación; ahora es trabajo confirmado, no hipótesis.

---

## H5.c — DISEÑO COMPLETO (26-jul)

Lenguaje y VM, cerrados. No queda ninguna decisión pendiente.

Resumen ejecutable: evento = par **(receptor, destino)** en 12 bytes dentro del objeto
· 1 suscriptor · handler **privado** · `obj::metodo` · palabra `event` con **asignación
pública y disparo de clase+subclases** · firma única **`(sender, kind, args)`** con
`args` `Object` casi siempre nulo y convención `"k:v,k:v"` · `kind` **constante emitida
por el compilador** · **A+B** (eventos con nombre + cajón de sastre en la clase base) ·
**disparan los dos, específico primero** · sin valor de retorno · **sin suscriptor =
silencio** · **cola drenada entre quanta por el scheduler**, con `Gui.run()` como
cliente.

Trabajo identificado, por sitio:
- **Compilador:** palabra `event` + layout del campo (3 slots, bit de GC en el
  receptor) en la ÚNICA función de layout (#299) · `::` en el parser · constante de
  `kind` cualificada por módulo · la regla de acceso asimétrico.
- **VM (las dos):** drenaje en el punto entre quanta (`poll_cb` ya existe) · retirar
  los handlers de dentro del builtin de `Gui.run()` · bandera de «cerrándose».
- **Ninguno:** el GC. El bitmap ya expresa la forma del campo, herencia cross-module
  incluida.

### Qué pasa con Gui.mod — DECIDIDO: se reforma entera (Eduardo, 26-jul)

Pregunta de Eduardo al terminar el paso 2: *"cuando terminemos habrá que modificar
la Gui.mod?"*

**Hallazgo: el GUI ya tiene el par (receptor, destino), hecho a mano.** El Camino A
de Forms (H13.1) guarda en cada `Component`:

```
var __win: Object            // la ventana dueña      <- RECEPTOR
var __slotClic: integer      // slot del handler      <- DESTINO
var __slotChange: integer

public function onClick()
  if this.__win != null then __guiInvokeBySlot(this.__win, this.__slotClic, this)
end onClick
```

Es exactamente lo que H5.c convierte en mecanismo del lenguaje. Así que `Gui.mod` no
cambia tanto por *añadir* como por **poder borrar**: los tres campos, `__bindEvents`,
los intrínsecos `__guiInvokeBySlot` y `__guiInvokeByName`, y del lado C
`bpvm_resolve_handler` entero — esa búsqueda lineal con `strcmp` sobre la tabla de
símbolos **en cada clic**.

**La decisión (Eduardo):** *"Yo me inclino por reformar toda la librería gráfica y las
demos. Tener 2 sistemas mezclados no es bueno. Ahora no lo hacemos hasta que el
sistema nuevo esté verificado. Y como paso transitorio se puede mezclar los 2
sistemas. Pero fíjate lo que nos ha pasado con el FS y el nuevo, que todavía tenemos
una mezcla de los dos sistemas."*

Se descarta por tanto la opción (c) que yo proponía —dejar `onClick()` como método
virtual cuya implementación por defecto dispara el evento— pese a no romper nada:
dejaría dos mecanismos conviviendo indefinidamente, que es justo el problema.

**Coste asumido:** hoy el camino documentado es *subclasear y reescribir `onClick`*.
Al pasar `onClick` a campo de evento, todo `class MiBoton extends Gui.Button` con su
`onClick()` deja de compilar ⇒ hay que migrar la librería, **las demos**, el formato
`.win` y el horneado de slots del IDE.

#### La lección del FS, con su mecanismo

La mezcla del FS no duró por falta de ganas: duró porque **el puente tenía un uso que
no se podía migrar**. El shim de `fs.h` sobrevivía por `fs_get`, y `fs_get` sobrevivía
por UN solo consumidor legítimo —cargar el módulo a ejecutar— que exigía resolver un
problema más difícil (de dónde sale ese buffer). Con el shim vivo, todo lo demás podía
seguir colgando de él sin que nada fallara nunca.

⇒ **Un puente transitorio dura lo que dure su uso más difícil de migrar.**

Para el GUI ese uso NO es la librería (eso es mecánico) sino **el `.win` + el IDE**:
hoy el IDE hornea los slots de vtable dentro del `.win` y el loader de Forms los lee.
Si se reforma `Gui.bp` y las demos pero el IDE sigue horneando a la vieja usanza,
queda exactamente la foto del FS.

**Disciplina para cuando toque:** que el borrado de lo viejo vaya **en la misma tanda
que lo prueba**, no en una posterior. Es lo que se hizo con el REPL de texto del Pico
en #305 — se fue en el mismo paso que demostró que el wire lo cubría.

**Cuándo:** no ahora. Cuando el sistema de eventos esté verificado. Como paso
transitorio se admite la convivencia, con el borrado planificado, no aplazado.

### Vida de los objetos y eventos — VERIFICADO contra el código (26-jul)

Pregunta de Eduardo: *"dijimos que si utilizamos 'owner' no hacía falta liberar los
eventos, ¿eso sigue en pie?"*

**Sí, y ahora está comprobado, no supuesto.** La pieza de la que depende es
`bpvm_free_owned` (`src/interp.c:484`), que es **recursiva** y recorre el
**owner-bitmap** del descriptor de clase:

```c
for (uint32_t i = 0; i < num_fields; i++) {
    uint32_t word = read_u32(owner_base + (i >> 5) * 4);
    if (((word >> (i & 31)) & 1u) != 0u) {
        bpref_t child = bpref_load(...);
        bpvm_free_owned(vm, child);      // cascada
    }
}
```

Encajan tres cosas:

1. **Destruir la ventana destruye los widgets, y con ellos sus eventos.** La cascada
   baja por los `owner` hasta el fondo; el campo del evento muere dentro del widget.
   No hay nada que dar de baja.
2. **Si muere el SUSCRIPTOR, el evento que le apuntaba se queda rancio y calla.**
   `bpvm_handle_kill` sube la generación en el acto y el despachador pregunta con
   `bpvm_ref_dead` (O(1)). Tampoco hay nada que dar de baja.
3. **La cascada NO sigue al receptor del evento**, porque su bit de owner es **0**
   (ref sí, owner no — ver computeClassLayout). Es lo correcto y conviene tenerlo
   escrito: un evento apunta a un suscriptor que **no le pertenece**; si la cascada
   lo siguiera, destruir un botón se llevaría por delante el panel de otro.

**El ciclo no fuga.** ventana →posee→ widget →evento→ apunta a→ ventana es un ciclo.
En un sistema de **contador de referencias** ésa es la fuga clásica de manual. Aquí
no, porque la destrucción la manda la **PROPIEDAD**, no el conteo: cuando el dueño de
la ventana la suelta, la cascada se lleva ventana y widgets y el ciclo no pinta nada.

#### La única condición (⚠️ para el manual)

El receptor del evento es una raíz de GC **FUERTE** — se decidió así al descartar la
referencia débil, porque `owner` hacía innecesaria la debilidad. Consecuencia: **un
suscriptor cuyo ÚNICO apuntador sea el evento no muere nunca.**

```
boton.onClick := Panel()::pulsado    // ese Panel no lo guarda nadie más -> vive para siempre
```

No deja de funcionar: el handler se sigue llamando. Simplemente el objeto no se
libera jamás. No es el caso normal —el suscriptor suele ser la ventana, que ya está
viva por otro lado— pero es el ÚNICO en el que "con `owner` no hace falta dar de baja"
deja de ser cierto.

**Regla para la documentación de usuario:** *el suscriptor tiene que estar sostenido
por alguien que no sea el evento.* Si lo está (lo normal: es un objeto con dueño), no
hay que desuscribirse nunca — ni al cerrar la ventana, ni al destruir el widget, ni al
morir el propio suscriptor.

### REVISIÓN: el evento declara su FIRMA (Eduardo, 26-jul) — se come el `kind` y el `Object`

Pregunta de Eduardo tras el paso 3: *"ahora que tenemos sobrecarga de funciones,
¿podemos comernos el kind y el object?"*

**Sí para los eventos con nombre.** Si cada evento declara su propia firma, el
discriminante deja de ser un dato en ejecución y pasa a ser **la suscripción**:

```
class Boton
  event onClick(sender: Boton)
end Boton

class Panel
  function event pulsado(b: Boton)     // tipado; sin kind, sin cast
end Panel

boton.onClick := panel::pulsado
```

Si `boton.onClick := this::pulsado`, entonces `pulsado` **es** el handler de clic y
no hay nada que preguntar. Y `sender` deja de ser `Object`, así que desaparece el
casteo del punto de uso.

**Encaja con H5.a de una forma que no se había visto:** si `Panel` tiene
`pulsado(b: Boton)` y `pulsado(s: Slider)` SOBRECARGADOS, `panel::pulsado` sería
ambiguo — y lo resuelve **la firma declarada del evento**. `::` se convierte en un
sitio de resolución de sobrecarga y la maquinaria de mangling de H5.a se aplica tal
cual. Es reutilizar algo ya pagado.

**Lo que NO se come: el cajón de sastre.** Su razón de ser es que UN handler atienda
MUCHOS eventos distintos, y con firmas tipadas eso es imposible por construcción.
Así que conserva la forma genérica `(sender: Object, kind: integer, args: Object)`.

⇒ **`kind` y `Object` no desaparecen: se repliegan al cajón de sastre**, que es donde
tienen sentido. Los eventos con nombre quedan tipados.

#### Aridad: VARIABLE pero ACOTADA (corrección de mi propia propuesta)

Yo había propuesto **aridad fija en dos** (`sender` + `args`) para que la entrada de
la cola fuese de tamaño fijo. El criterio de Eduardo —*"todo lo que sea facilidad
para el usuario y tenga un coste razonable merece la pena"*— la invalida: con aridad
fija hay que inventarse una clase para cualquier evento con dos datos
(`onValueChanged(sender, args: ValueArgs)`), que es PEOR que lo que había.

Decisión: **aridad variable hasta un máximo pequeño (4 parámetros)**, con la entrada
de cola de tamaño fijo dimensionada a ese máximo. El usuario escribe la firma natural
y la cola sigue siendo un anillo de entradas iguales.

Para que el GC sepa cuáles de esos argumentos son referencias que debe seguir, la
entrada lleva una **máscara de refs** — el mismo patrón que ya usa
`bpvm_call_bp_from_builtin(..., ref_mask)`.

#### Impacto sobre lo ya commiteado

- **Paso 1 (35a0f9a) — INTACTO.** El par (receptor, destino), los dos campos y los 12
  bytes no cambian. La firma no vive en el objeto, vive en la declaración.
- **Paso 2 (4c17e34) — casi intacto.** `::` sigue igual; se le AÑADE la comprobación
  de que la firma del handler casa con la declarada por el evento (y con ella, la
  resolución de sobrecarga). `function event` sigue haciendo la misma falta: el
  destino sigue siendo un slot de vtable.
- **Paso 3 (0748878) — se retoca.** El `raise` pasa a empujar los argumentos
  declarados en vez de `(sender, kind, args)`; el `kind` sólo se emite para el cajón
  de sastre. El builtin `__eventRaise` cambia de aridad ⇒ hay que fijar su contrato
  antes del paso 4.
- **Paso 4 — sin empezar**, así que absorbe el cambio sin coste: la cola se diseña ya
  con la entrada acotada + máscara de refs.

El grueso del trabajo nuevo es de COMPILADOR (declarar la firma, comprobarla en el
`::`, emitir los args), que es donde el coste es razonable. La VM sólo ve una entrada
de cola con N palabras y una máscara.

#### Contrato de `__eventRaise` (221) — fijado al implementar el rediseño (e2d1af2)

Lo que el emisor deja en la pila, de abajo arriba. La **cabecera va encima** para que
el despachador la saque primero y sepa cuántos argumentos tiene que desapilar:

```
arg0 .. argN-1   cada uno 8 bytes          <- los declarados, en orden
refMask          4B, bit i = argi es ref   <- para el GC de la cola
nargs            4B                        <- 0..4
dest             4B, slot de vtable        <- 0 = sin suscriptor
recv             8B, handle del receptor   <- null = sin suscriptor
```

**Los argumentos se normalizan a 8 bytes.** Con anchos mixtos el despachador
necesitaría saber el de cada uno para desapilarlos, y eso serían DOS máscaras (refs
para el GC + anchos para la pila). Con todo a 8 basta la de refs, y desaparece una
clase entera de fallos de ancho — que en este proyecto no es teórica: el 4→8B costó
H1 entera. El coste es 4 bytes de pila por argumento primitivo, en una operación que
ya va a la cola.

Los omitidos los rellena el COMPILADOR con su valor por defecto (H8.1, sustitución en
el llamante), así que el builtin siempre ve la aridad declarada: no hay "argumento
ausente" en tiempo de ejecución.

#### El `::` pide el slot por la sobrecarga ELEGIDA, no por el nombre

Un fallo que se cazó al probar el rediseño, y que merece quedar escrito porque es
silencioso. Por la regla de H5.a *la primera firma se queda el nombre pelado y las
siguientes se manglean*, así que `slotOf("pulsado")` devuelve **siempre** el slot de
la primera declarada. Si la que casa con la firma del evento era la segunda, el
semántico elegía bien y el emisor colgaba la otra — muda, y con la firma equivocada
al despachar. El slot se pide con `emitName(fs)` de la elegida.

`samples/EvSubs.bp` declara a propósito la sobrecarga que NO casa **primero**, para
que el sample falle si alguien vuelve al atajo.

### Paso 4 — la cola y el drenaje, ya en las dos VMs (26-jul)

Funciona: `raise` encola y vuelve; el scheduler, entre quanta, le monta al
thread destino el frame del handler; desde ahí es código BP corriente.

**El frame inyectado es el que montaría una llamada normal, byte a byte.** No
hay convención de eventos aparte — se reusa la que ya existe. Por eso rectifico
lo que había decidido en e2d1af2 (normalizar los argumentos a 8 bytes):
normalizar era inventarse una ABI paralela, y este proyecto ya sabe lo que
cuestan los desajustes de ancho. Los argumentos van en su ancho NATURAL y la VM
no lo adivina: se lo dice el compilador en la misma palabra que la máscara de
refs del GC (bits 0-3 ref, bits 8-11 ocho-bytes).

**La vuelta va por un opcode SENTINELA**, `OP_EVENT_RETURN` (0x6F) en
`memory[2]`, tercero de la región reservada tras THREAD_EXIT y NATIVE_RETURN.
Hace falta porque el RET del handler deja su valor de retorno en la pila y el
código interrumpido no lo espera. El PC de reanudación se guarda EN LA PILA,
debajo de los argumentos, no en el `tc`: así un handler interrumpido no pisa la
vuelta del de abajo, y la reentrada sale gratis y sin estado extra.

#### Lo que costó una medida: un handler a la vez

Yo había puesto "un evento por punto de planificación" pensando que bastaba para
el FIFO. **No basta**, y lo cazó un sample con tres cadenas de eventos: el
siguiente punto de planificación puede caer DENTRO de un handler que aún no ha
terminado, y el frame nuevo se le monta encima → el segundo evento se completa
ANTES que el primero.

Lo interesante es que **sólo se veía en la VM-Java**: su quantum es por TIEMPO y
puede expirar dentro del handler; el de la VM-C es por OPCODES y el handler
cabía entero. Un mismo programa daba `10 20 30 11 21 31` en C y
`10 20 11 30 31 21` en Java. Sin la pareja de VMs esto se habría quedado dentro
como un fallo intermitente de los que aparecen en placa seis meses después.

Arreglo: `ev_depth` por thread — el drenaje no inyecta si ya hay un handler
corriendo. **Un handler corre hasta el final antes de despachar el siguiente**,
que es la semántica de un pump de mensajes (el EDT de Swing hace exactamente
esto). Un `raise` DESDE un handler sigue valiendo: eso encola, no inyecta.

#### Comportamiento anotado (no decidido, sólo medido)

- **Excepción no atrapada en un handler**: se reporta y se para, igual que
  cualquier otra excepción que nadie atrapa. No es silencioso, que es lo que
  importa. Un despachador estilo EDT reportaría y seguiría con el siguiente
  evento; si algún día se quiere eso, el sitio es el borde de la inyección.
  Sample `EvThrow.bp` (rojo a propósito).
- **Eventos pendientes al terminar el programa**: se pierden. El drenaje
  necesita puntos de planificación, y si `main` acaba no hay más.
- **Cola llena** (16) y **slot no resoluble**: gritan por stderr y descartan.
  **Receptor muerto** entre el raise y el drenaje: se ignora sin ruido — es el
  caso normal al destruir un suscriptor con eventos pendientes.

---

## Ideas FUTURAS (sin plazo) — charla del 29-jul

Ninguna de estas es de V4. Se anotan para que no se pierdan; **la segunda no es
siquiera de V5**, es una posibilidad a estudiar sin compromiso.

### Módulo entero native — "una isla sin puentes dentro"

Pregunta de Eduardo: el AOT va función a función, ¿se pueden poner todas juntas?

**Ya están juntas.** El `.mdn` es POR MÓDULO y el emisor recoge todas las `native`
en un único fichero C; dos native del mismo módulo se llaman **directamente en C**,
sin puente ni frame de VM (`AotCEmitter`: `if (!nativeFuncNames.contains(name))`
→ sólo entonces `emitBridgeCall`). Lo que es por-función no es el empaquetado: es
**la decisión**.

Así que el premio de "módulo entero native" no es juntarlas, es **que desaparezcan
los cruces de puente**: hoy cada llamada native→BP paga un `call_bp_i32` que monta
un frame de intérprete. Con el módulo entero, todas las llamadas internas son C
directo y el compilador puede además inlinear entre ellas.

**El muro no es de formato, es de COBERTURA.** El emisor lanza
`UnsupportedAotException` en bastante: intrínsecos cross-module, métodos
privados/super/estáticos, construcción de objetos, `try/catch` dentro de native.
Hoy se esquiva marcando sólo la función caliente; un módulo entero se los come
todos de golpe.

Y tres cosas sistémicas que hoy no duelen porque las estancias en native son
CORTAS, y que con un módulo entero pasan a primer plano:

1. **Raíces del GC** — es el paso 3 de #302 (shadow stack), diferido. Cuanto más
   tiempo vive la ejecución dentro de C, más referencias hay en locales/registros
   que el GC no ve. Deja de ser teórico.
2. **El planificador** — el native es una caja negra entre quanta. Un módulo
   entero puede correr mucho sin ceder: threads que no avanzan, eventos
   encolados, `KILL` que no responde. Lo contrario del evento `Run` asíncrono.
3. **Depuración** — dentro de native no hay breakpoints.

**Pregunta que cambia el diseño (abierta):** ¿es para módulos de LIBRERÍA (Json,
Str, Math — cálculo puro, sin GUI ni threads, donde las tres pegas casi no
aplican) o para módulos de APLICACIÓN cualesquiera? Lo primero se puede abordar
sin resolver nada de lo anterior y da casi todo el beneficio.

### Backend propio de código máquina — "un JIT, pero previo"

Idea de Eduardo, **sin plazo y sin compromiso**: generar código máquina
DIRECTAMENTE, con emisor propio, en vez de BP → C → GCC → enlazar.

**El listón está alto.** El Fibo del P4 dio **113×** sobre el intérprete, y eso lo
pone `gcc -O3`, no el emisor. Un generador propio de primera generación (plantilla
por opcode, sin optimizador) se mueve típicamente en 3-10×. Muchísimo frente a
interpretar, pero un orden de magnitud POR DEBAJO del camino actual. No sería un
sustituto del AOT: sería otro punto de la curva.

**Pero la mitad difícil ya está hecha.** Un JIT tiene dos mitades: generar el
código y poder ejecutarlo. La segunda —cargar en RAM ejecutable, hijack por
registro, tag de arquitectura, W^X en el P4— es H4 y ya funciona en placa. Falta
sólo el emisor.

Dónde gana de verdad, y **no es la velocidad**:

- **Quita la dependencia del toolchain**: hoy usar `native` exige los
  cross-compilers instalados. Con emisor propio, el AOT no depende de nada
  externo — literalmente la visión de soberanía.
- **Es el único camino hacia compilar EN LA PLACA**. Con GCC de por medio, jamás.
  Encaja con los packs de código nativo de V5.
- **Ciclo de build instantáneo**, sin compilar ni enlazar.

Hacerlo **previo** (no JIT) es lo correcto para empezar: el emisor se depura en el
PC, con desensamblador, comparando contra lo que hace GCC. Moverlo a la placa
después es cambiar dónde corre, no qué hace.

Si algún día se aborda: **conviviendo, no sustituyendo**. Emisor propio como
camino por defecto (siempre disponible, rápido) y el camino C como "modo release".
El `.mdn` ya lleva tag de arquitectura, así que el formato admite las dos
procedencias sin tocarlo.
