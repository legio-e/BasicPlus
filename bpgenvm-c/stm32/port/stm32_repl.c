/*
 * stm32_repl.c — REPL wire v1 bare-metal (H9.2 / H9.4).
 *
 * H9.2.a: HELLO/INFO/TIME/PING/RESET + LIST/DF/LOG → el IDE conecta.
 * H9.2.b: + FS en RAM (PUT/GET/DEL/STAT/LIST real/DF real/MKDIR/FORMAT) y
 *         RUN (carga el .mod del FS, lo ejecuta, hace streaming de OUTPUT y
 *         emite EXITED) → "Run on STM32" completo.
 * H9.2.c: RUN resuelve imports (carga <owner>.mod del FS, ≤4 pasadas) + guard.
 * H9.4:   al boot registra los backends de HW (GPIO + info de MCU) y
 *         pre-instala la stdlib core en /lib (stm32_mods_install) → los
 *         programas que importan stdlib resuelven y controlan pines reales.
 *
 * Single-thread, sin FreeRTOS. La salida del programa BP se reenvía como
 * eventos OUTPUT (bytes verbatim, escapados a JSON → paridad de contenido).
 */
#include "bpvm_pico.h"   /* HAL BP: los dos nombres de la identidad */
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "stm32_fs.h"
#include "board_mgr_stm32.h"  /* H9 — arranque escalonado + STATE/ENV/PART */
#include "flash_layout_stm32.h" /* BP_ENV_SECTOR: s_put_buf hace de scratch del env */
#include "bpvm_pack.h"        /* H3 — BPVM_PACK_BURN_CHUNK (bulk del PACK_BURN_DATA) */
#include "log.h"              /* log persistente de diagnóstico (espejo del Pico) */
#include "bpvm_fs.h"          /* H19 — base-dir / main module por proyecto */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "stm32_mods.h"     /* stdlib core embebida (pre-install /lib) */
#include "gpio_stm32.h"     /* stm32_hw_register (backends GPIO + Pico) */
#include "json_min.h"
#include "bpvm.h"
#include "bpvm_io.h"   /* V6/A1.5: el segundo hilo, comun a todas las familias */
#include "bpvm_internal.h"   /* recorrido de vm->modules[] para los .mdn del AOT */
#include "bpvm_entry.h"      /* #344 — el RUN, escrito una vez */
#include "bpvm_rtc.h"        /* H10 — TIME aplica la hora al RTC (bpvm_rtc_set_now_ms) */
#include "mdn_loader.h"      /* H9.5: overlay AOT .mdn desde el FS (loader compartido) */
#include "aot_registry.h"
#include "bpvm_repl.h"       /* V6/U3: los verbos COMUNES del REPL (grupo 1: meta) */
#include "bpvm_mem.h"        /* V6/U6.11: el planificador de memoria COMÚN (una regla, cinco placas) */
#include "bpvm_mods.h"       /* V6/U4: el bucle y la regla de la stdlib embebida, comunes */    /* H9.5: bpvm_aot_clear entre RUNs (registry global) */

#include "main.h"
#include "board.h"              /* placa: BOARD_WIRE_UART, BOARD_NAME, BOARD_SRAM_BYTES, BOARD_LED_* */

#include <string.h>
#include <stdio.h>

#define SERVER_NAME "bpvm-stm32"
/* BOARD_NAME, BOARD_SRAM_BYTES y los BOARD_LED_* los provee board.h (por placa). */

/* Buffers estáticos (NO en stack: el stack C del micro es pequeño). */
static char    s_line[WIRE_LINE_MAX];
/* #294/#334 — el buffer del bulk YA NO tiene que dar para el fichero entero:
 * desde que el IDE sube por trozos (PUT_BEGIN/DATA/END, verificado en placa en
 * las 3 familias el 28-jul) sólo necesita el MAYOR de sus dos papeles:
 *   (a) un trozo de streaming            = PUT_STREAM_CHUNK (16 KB)
 *   (b) el scratch del gestor de placa
 *
 * #338 (2-ago) — los dos papeles ADELGAZARON, y con ellos el buffer, de 28 a 12 KB:
 *   (a) el IDE manda trozos de 8 KB (BpvmClient.PUT_STREAM_CHUNK). Un cliente viejo
 *       que mandase más recibe NO_SPACE y se drena: se queja, no se corrompe.
 *   (b) el gestor ya no pide TRES sectores aquí: las dos copias del env se las presta
 *       la zona de rascar compartida (board_mgr_stm32.c) y de este buffer sale sólo el
 *       sector de trabajo + la respuesta = BP_ENV_SECTOR + reply.
 * El sector del env es la página de BORRADO y aquí son 8 KB (el doble que en
 * RP2350/ESP32), así que esta familia se queda en 12 KB donde las otras bajan a 8:
 * es el silicio, no una excepción. La comprobación EN COMPILACIÓN de abajo sigue
 * marcando el suelo: si alguien lo baja de más, no compila. */
#define V1_PUT_BUF_SIZE  (BP_ENV_SECTOR + 4096u)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= BP_ENV_SECTOR + 512u) ? 1 : -1];
/* H13 hallazgo 31 (5-ago) — 128 KB era un DESCUIDO, no una decisión: se fijó al
 * nacer el port y nadie volvió a mirarlo. Medido sobre el ELF de la Nucleo: de
 * los 768 KB de RAM el estático total eran ~247 KB (128 de éstos) ⇒ ~520 KB
 * PARADOS, en la placa que tenía el heap MÁS PEQUEÑO del parque (64 KB, contra
 * 96 del S3 y 257 de la Pico). Con 512 KB la regla común (25 % pilas, suelo 64,
 * techo 512) reparte 384 de heap + 128 de pila.
 *
 * El presupuesto que lo justifica, para quien lo revise: 768 total − 512 aquí
 * − ~119 del resto del estático = ~137 KB libres, y el linker sólo exige 20
 * (_Min_Heap_Size 16K + _Min_Stack_Size 4K). Margen de 6×.
 *
 * Lo comparten LAS DOS placas de la familia y la que manda es la Nucleo: la
 * Discovery tiene 3008 KB de RAM, así que aquí le sobra. Si algún día se quiere
 * afinar por placa, el sitio es board.h — no este #define. */
/* V6/U6.12 — el tamaño lo pone CADA PLACA en board.h (BOARD_VM_BYTES): la Nucleo
 * sigue en 512 KB y la Discovery, con 3008 KB de RAM, pasa a 1536. «Quedarnos con
 * lo bueno y no unificar a lo peor» (Eduardo). */
static uint8_t s_vm_mem[BOARD_VM_BYTES];      /* RAM que gestiona la VM */
/* V6/U6.11 — lo que el planificador COMÚN (bpvm_mem) decidió sobre ese array.
 * Aquí no hay nada que medir en marcha: la región es el array entero, EXCLUSIVA
 * de la VM y SIN margen, porque el margen de esta familia lo pone el ENLAZADOR
 * (si `_Min_Heap_Size + _Min_Stack_Size` no caben detrás del estático, no enlaza,
 * y eso es más fuerte que cualquier comprobación en marcha). Pero la decisión y
 * la línea del log son las mismas que en las otras cuatro placas. 0 = sin plan. */
static size_t  s_vm_bytes = 0;
#define VM_MEM_MIN  (64u * 1024u)             /* el suelo de las cinco placas */
static char    s_out_esc[2048];               /* salida escapada (sink) */
static char    s_out_msg[2300];               /* evento OUTPUT completo */
static long    s_session = 0;                 /* contador de sesiones RUN */
static long    s_run_session = 0;             /* sesión activa (para el sink) */

/* ---- helpers de envío ---- */

static void reply_empty(const char* type, long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"id\":%ld}", type, id);
    if (n > 0) wire_v1_send_line(buf, (size_t) n);
}

/* ---- META ---- */

/* V6/U3 g5 — HELLO vive en el común; lo propio va en la cintura, abajo. */

/* ── V6/U3 g4: LA CINTURA de esta familia ────────────────────────────────────
 * El REPL común pide los datos; aquí se dicen. Ver bpvm_repl.h para la forma
 * y la medida que la decidió (18 campos comunes, 11 sólo del RP2350). */

static void stm32_repl_info(bpvm_repl_info_t* o) {
    /* N-stm32-info — DE DONDE SALE CADA VALOR de esta placa. El mensaje ya no
     * se arma aquí (lo hace src/bpvm_repl.c con los 18 campos comunes); esto
     * sólo los RELLENA, que es lo que de verdad es propio del STM32.
     * Valores del NUCLEO-U575ZI-Q (datasheet DS13737): flash 2 MB (sufijo
     * ZI), SRAM 768 KB, sin PSRAM. gpioCount=114 (I/Os del LQFP144).
     * pioCount=0 (PIO es RP2350-only). pwmSlices=28 — el campo lleva
     * SALIDAS PWM, como en los otros ports (Pico 24, ESP32 8): TIM1/TIM8
     * avanzados (4+4) + TIM2/3/4/5 GP (4×4) + TIM15 (2) + TIM16/17 (1+1),
     * sin contar complementarias ni los 4 LPTIM.
     * adcChannels=20 (ADC1 14-bit, "up to 20 multiplexed channels"; hay
     * además un ADC4 12-bit con 19 canales externos). Son datos del CHIP:
     * los backends BP de Pwm/Adc en STM32 aún no están cableados (H9 doc).
     * tempMilliC=0 (el diálogo oculta la línea; el sensor interno queda
     * para más adelante). FLASH_SIZE real del registro por si montan otra
     * variante. */
    static char uid[28];
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    uint32_t u2 = *(volatile uint32_t*) (UID_BASE + 8U);
    snprintf(uid, sizeof uid, "%08lX%08lX%08lX",
             (unsigned long) u2, (unsigned long) u1, (unsigned long) u0);
    o->unique_id     = uid;
    /* V6 (9-sep) — POR LA FACHADA. `board_name` del wire es el MICRO (el IDE lo
     * rotula «Micro»), no la placa; `BOARD_NAME` es la PLACA y sale por
     * `Machine.getBoard()`. Antes esto era una segunda fuente escrita a mano.
     * Ver el comentario gemelo en esp32/common/repl_esp32.c. */
    static char micro[24];
    bpvm_pico_micro_name(micro, sizeof micro);
    o->board_name    = micro;
    o->reset_reason  = stm32_reset_cause();
    o->arch          = (unsigned) bpvm_mdn_host_arch();
    o->cpu_hz        = (unsigned long) SystemCoreClock;
    o->uptime_ms     = (unsigned long) HAL_GetTick();
    o->temp_milli_c  = 0;            /* sensor interno, para más adelante */
    o->gpio_count    = 114;          /* I/Os del LQFP144 */
    o->pio_count     = 0;            /* PIO es del RP2350 */
    o->pwm_slices    = 28;           /* salidas PWM, como cuentan los otros ports */
    o->adc_channels  = 20;
    o->flash_bytes   = (unsigned long) (*(volatile uint16_t*) FLASHSIZE_BASE) * 1024UL;
    o->sram_bytes    = BOARD_SRAM_BYTES;
    o->psram_bytes   = 0;
    /* V6/U6.11 — sobre lo PLANIFICADO, no sobre el sizeof: hoy coinciden (objetivo
     * 0 = el array entero) y, si un día dejan de coincidir, el INFO dirá la verdad. */
    {
        size_t vmb = s_vm_bytes;
        size_t pil = vmb ? bpvm_stack_region_bytes(vmb) : 0;
        o->vm_heap_bytes  = (unsigned long) (vmb - pil);
        o->vm_stack_bytes = (unsigned long) pil;
    }
    o->fs_total_bytes = (unsigned long) fs_total_bytes();
    o->fs_used_bytes  = (unsigned long) fs_used_bytes();
}

static unsigned long stm32_repl_fs_total(void) { return (unsigned long) fs_total_bytes(); }
static unsigned long stm32_repl_fs_used(void)  { return (unsigned long) fs_used_bytes(); }
static int           stm32_repl_fs_count(void) { return fs_count(); }
static int           stm32_repl_fs_format(void){ fs_format(); return 0; }
static int           stm32_repl_fs_save(void)  { fs_save(); return 0; }

/* El sello de compilación de ESTA familia. Ver la nota de `server_build` en
 * bpvm_repl.h: es la fecha de este fichero, no la del enlace — no identifica la
 * imagen por sí solo, pero es lo que había y no empeora al centralizar. */
static const char s_build[] = __DATE__ " " __TIME__;

/* Designados a propósito: la cintura va a crecer verbo a verbo, y con campos
 * posicionales meter uno en medio corre todos los de abajo en silencio — un
 * puntero a función acabaría llamándose por el hueco de otro. Ya nos pasó al
 * quitar un método de una base de la stdlib. */
/* Persistir sólo lo que sobrevive al reset de forma útil: /lib lo re-instala el
 * embebido en cada boot, así que un PUT a /lib (el IDE lo hace en cada Run) no
 * necesita flash → se ahorra un erase+program por ejecución. */
static void stm32_after_put(const char* path) {
    if (strncmp(path, "/lib/", 5) != 0) fs_save();
}

static const bpvm_repl_ops_t s_repl_ops = {
    .info          = stm32_repl_info,
    .info_extra    = NULL,     /* sin campos propios: los 18 comunes bastan */
    .server_name   = SERVER_NAME,
    .server_build  = s_build,
    .capabilities  = "[\"META\",\"FILES\",\"TERMINAL\",\"PACKS\"]",
    .fs_total_bytes= stm32_repl_fs_total,
    .fs_used_bytes = stm32_repl_fs_used,
    .fs_file_count = stm32_repl_fs_count,
    .fs_format     = stm32_repl_fs_format,
    .fs_save       = stm32_repl_fs_save,
    .put_buf       = s_put_buf,
    .put_buf_size  = sizeof s_put_buf,
    .after_put     = stm32_after_put,
};

/* ---- FILES ---- */

/* V6/U3 g3 — LIST vive en el común (streaming, crc:-1, montajes de la fachada). */





/* V6/U3 g5 — PUT vive en el común. Lo propio de esta familia son el scratch y
 * qué hacer después de una subida; ambos están en la cintura, abajo. */

/* V6/U3 g6 — #294 streaming PUT (BEGIN/DATA/END) vive en el común, con la
 * sesión y el scratch. Lo de esta familia sigue siendo el buffer y el
 * after_put, ya en la cintura. */


/* Sink que escapa cada chunk del log y lo escribe RAW → streaming como el Pico
 * (header + chunks + cierre), sin buffer para el log entero. */

/* V6/U3 — LOG_DUMP vive en el común (src/bpvm_repl.c). */
/* V6/U3 g2 — DEL/STAT/GET/MKDIR también (y RENAME/RMDIR, que esta familia no tenía). */

/* ---- TERMINAL: RUN + streaming ---- */

/* Cada PRINT_* de la VM llega aquí → evento OUTPUT con los bytes escapados. */
static void v1_output_sink(const char* s, size_t len, void* user) {
    (void) user;
    if (stm32_wire_json_escape(s, len, s_out_esc, sizeof(s_out_esc)) < 0) return;
    int n = snprintf(s_out_msg, sizeof(s_out_msg),
        "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"%s\"}",
        s_run_session, s_out_esc);
    if (n > 0) wire_v1_send_line(s_out_msg, (size_t) n);
}

static void emit_exited(long session, const char* status, int code, uint32_t ms) {
    char buf[200];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"%s\",\"exitCode\":%d,"
        "\"elapsedMs\":%lu}", session, status, code, (unsigned long) ms);
    if (n > 0) wire_v1_send_line(buf, (size_t) n);
}

/* P-run-stop (#257) + P-autorun (#256) — wire durante el run (la VM
 * invoca el poll entre quanta; bare-metal single-thread → el poll puede
 * responder directamente):
 *   KILL  → ack diferido (KILL_REPLY tras parar, antes del EXITED) + 1.
 *   HELLO → HELLO_REPLY inmediato — el IDE puede conectarse con un
 *           (auto)run en marcha y ofrecer Stop.
 *   otra  → error BUSY inmediato. */
static long s_kill_ack_id = -1;
static long s_reset_ack_id = -1;   /* RESET recibido en-run (#452) */

/* El reinicio, en UN sitio: lo piden el despachador de reposo y —desde #452— el
 * final de un RUN que recibió RESET mientras corría. No retorna. */
static void stm32_hacer_reset(long id) {
    log_printf("RESET (wire): reinicio");
    log_flush();                 /* persiste la sesión antes de reiniciar */
    reply_empty("RESET_REPLY", id);
    HAL_Delay(50);
    NVIC_SystemReset();
}

/* V6/A1.5 - la costura de esta familia para el hilo `io`: su `poll` (el de
 * siempre) y su `line` (el sink de siempre, que ahora recibe lineas enteras en
 * vez de trozos). Nada mas; el lazo y el troceado son comunes. */
#define STM32_IO_OQ_BYTES 2048
static int stm32_run_poll_cb(bpvm_t* vm, void* user);
static int stm32_io_poll(void* user) {
    (void) user;
    return stm32_run_poll_cb(NULL, NULL);
}

static int stm32_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    int c = stm32_wire_getchar();
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, s_line, sizeof(s_line));
    if (n < 0) return 0;                      /* rota/estancada: descartar */
    json_obj_t obj;
    if (json_parse(s_line, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { s_kill_ack_id = rid; return 1; }
    /* V6/E1 (#452) — RESET, con la MISMA forma que KILL. El enunciado de la ficha
     * era falso: RESET SÍ llegaba, y se rechazaba aquí a propósito con BUSY por no
     * tener rama. En dos tiempos: marcar el id y devolver 1 para que el RUN muera
     * por el camino de siempre, y reiniciar DESPUÉS, ya fuera de `bpvm_run`. */
    if (strcmp(type, "RESET") == 0) { s_reset_ack_id = rid; return 1; }
    if (strcmp(type, "HELLO") == 0) { bpvm_repl_dispatch(type, rid, &obj); return 0; }
    wire_v1_send_error(rid, "BUSY", "ejecución en curso: solo HELLO/KILL/RESET");
    return 0;
}

/* Resuelve un nombre de módulo en el FS: prueba base-dir/name, name, /app/name,
 * /lib/name (el IDE sube las deps a /lib). 0 OK, -1 no está.
 *
 * H11 — devuelve la RUTA que existe y su tamaño, NO los bytes. Antes resolvía a
 * un puntero, y sostenerlo costaba un espejo estático del tamaño de la arena del
 * FS viejo: 496 KB en la Discovery, 96 KB en la Nucleo. Ahora el llamante abre
 * por esa ruta y lee por trozos. Mismo cambio que #305 en el Pico. */
static int stm32_fs_resolve(const char* name, char* out, size_t out_cap, uint32_t* size) {
    /* #344 — la REGLA vive en el núcleo (bpvm_entry_resolve): basedir del
     * proyecto → tal cual → /app → /lib. Estas 15 líneas estaban COPIADAS
     * palabra por palabra en Pico, ESP32 y STM32; aquí ya sólo se traduce el
     * 0/-1 que espera el resto de este fichero. */
    return bpvm_entry_resolve(name, out, out_cap, size);
}

/* Núcleo del RUN — compartido entre el comando RUN del wire (id >= 0)
 * y el autorun de boot (#256, id < 0). Con id < 0 no hay cliente: sin
 * RUN_REPLY, y una ruta inexistente enciende el LED rojo (el idioma de
 * diagnóstico de este port) en vez de mandar un error al vacío. */
/* V6/#412 — `arg` es el ARGUMENTO DE EJECUCION del programa, y viaja como
 * campo ESCALAR del RUN (no `args:[]`: el mini-parser de las placas no sabe
 * leer arrays anidados, ver json_min.h). NULL = no se dio, y manda el valor
 * por defecto que declare `Main(arg: string := ...)`.
 *
 * El AUTORUN no pasa ninguno, a proposito (Eduardo, 6-sep): el parametro es
 * para PROBAR el programa con distintas opciones; una vez probado se fija el
 * valor por defecto en el fuente, y la mision del autorun es solo que arranque
 * al encender el micro. O sea que el defecto ES la configuracion de despliegue. */
static void run_module_path(const char* path, long id, const char* arg) {
    /* H19-F1 — fija el base-dir/main-module del proyecto si el módulo vive en
     * /app/<proj>/ (el IDE manda la ruta cualificada). Plano → sin base-dir. */
    bpvm_fs_set_basedir_from_module(path);
    char main_path[80]; uint32_t size;
    if (stm32_fs_resolve(path, main_path, sizeof(main_path), &size) != 0) {
        if (id >= 0) wire_v1_send_error(id, "NOT_FOUND", "no existe");
        else         BOARD_LED_ERR_ON();            /* autorun: ruta mala */
        return;
    }

    long session = ++s_session;
    s_run_session = session;
    if (id >= 0) { /* RUN_REPLY con la sesión, ANTES de ejecutar */
        char buf[80];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"RUN_REPLY\",\"id\":%ld,\"session\":%ld}", id, session);
        if (n > 0) wire_v1_send_line(buf, (size_t) n);
    }

    /* Antes pasaba 0 = 'default de bpvm_init' (mitad y mitad): la única de las
     * tres familias que ni siquiera tenía la regla. Ahora usa LA MISMA que Pico
     * y ESP32. Con 128 KB manda el suelo ⇒ 64/64, idéntico a hoy. */
    /* V6/U6.11 — el bloque es el que decidió el planificador en el arranque (hoy,
     * el array entero). Sin plan (NO_CABE) no hay VM y se dice: no se arranca a medias. */
    if (s_vm_bytes == 0) {
        log_printf("run: sin memoria planificada para la VM — no se ejecuta");
        BOARD_LED_ERR_ON(); emit_exited(session, "INTERNAL_ERROR", -1, 0); return;
    }
    size_t stack_region = bpvm_stack_region_bytes(s_vm_bytes);
    bpvm_t* vm = bpvm_init(s_vm_mem, s_vm_bytes, s_vm_bytes - stack_region);
    /* V6/#412 — el argumento de ejecucion, antes de arrancar: lo recoge el
     * builtin __runArg que `__startup` llama justo antes de entrar en Main. */
    if (vm) bpvm_set_run_arg(vm, arg);
    if (!vm) { BOARD_LED_ERR_ON(); emit_exited(session, "INTERNAL_ERROR", -1, 0); return; }
    bpvm_set_output(vm, v1_output_sink, NULL);

    uint32_t t0 = HAL_GetTick();
    BOARD_LED_RUN_ON();                           /* LED RUN = ejecutando un programa */
    /* #344 — UNA carga: bpvm_load_entry despacha .mod/.pack, lee por trozos
     * (H11: el .mod se queda en el FS), resuelve las dependencias con la regla
     * común —FS primero, y si no está, los packs grabados en XIP, código EN
     * FLASH y cero copia— y NOMBRA la que falte.
     *
     * De las tres familias ésta era la que MÁS tenía (era la única con packs y
     * con el guard anti-hard-fault), y es justo lo que se subió al núcleo: aquí
     * desaparecen ~70 líneas sin perder ni una capacidad. El guard sigue siendo
     * imprescindible en bare-metal — un CALL_EXT sin resolver cuelga el micro —
     * sólo que ahora lo hace `first_missing` para las cinco. */
    /* [V6/N1.4] EL VACIADO VA ANTES DE CARGAR. El registro AOT es global y hay
     * que tirar el del RUN anterior (los módulos se recargan en direcciones
     * frescas), pero desde que un `.mod` trae su bloque nativo dentro,
     * `bpvm_load_entry` YA REGISTRA thunks — y vaciar después los borraba.
     * No da error: se ejecuta interpretado y el resultado sale bien. Se vio en
     * la Pico el 23-ago, y sólo con el log encendido. */
    bpvm_aot_clear();

    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t st = bpvm_load_entry(vm, path, &entry);
    char missing[40] = {0};
    if (entry.missing[0]) strncpy(missing, entry.missing, sizeof(missing) - 1);
    if (entry.from_pack)
        log_printf("run: pack '%s' (main=%s)", entry.resolved, entry.main_module);
    /* [25-ago] EL CAMINO DEL FALLO HABLA. Un RUN que moria en la carga dejaba
     * UNA linea en el log (el guardian de memoria, al destruir) y nada mas: ni
     * el status, ni el motivo, ni que fichero se estaba leyendo. Depurar eso
     * era adivinar — pregunta de Eduardo: "¿por que el log solo añade una linea
     * cuando en la Pico añade 12?". Esta linea es la respuesta al proximo. */
    if (st != BPVM_OK) {
        uint8_t m4[4] = {0};
        (void) bpvm_fs_read(main_path, m4, 4);   /* el MAGIC que vio el loader */
        log_printf("run: load '%s' FALLO st=%d fallo='%s' missing='%s' magic=%02X%02X%02X%02X",
                   path, (int) st, entry.fallo, missing, m4[0], m4[1], m4[2], m4[3]);
    }

    /* H9.5 — overlay AOT: para cada módulo cargado, si el FS tiene su
     * <Modulo>.mdn (PIC Thumb-2 de build_mdn.sh — mismo -mcpu=cortex-m33
     * que el RP2350, la U575 es el mismo core), registra sus thunks
     * zero-copy apuntando al buffer del FS (RAM, dirección estable durante
     * el RUN). El registry AOT es GLOBAL → clear antes de cada RUN para no
     * arrastrar thunks de una sesión anterior (buffers FS ya movidos).
     * Tolerante: sin .mdn o rc != OK → se ejecuta interpretado, sin más.
     * Nota U575: el código ejecuta desde SRAM (S-bus); el ICACHE del U5
     * cachea la ruta de flash (C-bus), así que no hace falta invalidación
     * — confirmar en placa con el primer smoke (fib_native).
     *
     * ⚠️ EL `bpvm_aot_clear()` YA NO ESTÁ AQUÍ — ver arriba, antes de la carga.
     * Desde [V6/N1.4] un `.mod` puede traer su bloque nativo DENTRO y el
     * cargador registra sus thunks durante `bpvm_load_entry`: vaciar el
     * registro después borraba justo eso. */
    if (st == BPVM_OK && !missing[0]) {
        for (int mi = 0; mi < vm->module_count; mi++) {
            const char* mname = vm->modules[mi].name;
            if (!mname || !mname[0]) continue;
            char mdn_path[72];   /* name[64] + ".mdn" + NUL: sin -Wformat-truncation */
            snprintf(mdn_path, sizeof(mdn_path), "%s.mdn", mname);
            char mdn_real[80]; uint32_t mdn_size;
            if (stm32_fs_resolve(mdn_path, mdn_real, sizeof(mdn_real), &mdn_size) != 0) continue;
            /* H11 — el .mdn es ZERO-COPY: los thunks se registran como punteros
             * DENTRO de este buffer, así que tiene que seguir vivo y en RAM toda
             * la ejecución. Antes vivía en el espejo del fs_get (la razón de que
             * el espejo fuera permanente y del tamaño de la arena); ahora se le
             * reserva de la arena de la VM exactamente lo que ocupa, 4-alineado
             * que es lo que pide Thumb-2. Mismo remedio que el Pico. */
            uint8_t* mdn_data = bpvm_arena_reserve(vm, mdn_size, 4);
            if (!mdn_data) {
                log_printf("AOT: %s (%lu B) no cabe en la arena — sin overlay",
                           mdn_real, (unsigned long) mdn_size);
                continue;
            }
            if (bpvm_fs_read(mdn_real, mdn_data, mdn_size) != (long) mdn_size) {
                log_printf("AOT: %s no se pudo leer — sin overlay", mdn_real);
                continue;
            }
            int mrc = bpvm_load_mdn(vm, mdn_data, (size_t) mdn_size);
            /* Visible en la consola del IDE: sin esto, un fallo de carga
             * (p.ej. buffer desalineado) caía a interpretado EN SILENCIO. */
            char mmsg[96];
            int mn = snprintf(mmsg, sizeof(mmsg), "[AOT] %s %s (rc=%d)\n",
                              mdn_path, (mrc == 0) ? "OK" : "FALLO -> interpretado", mrc);
            if (mn > 0) v1_output_sink(mmsg, (size_t) mn, NULL);
        }
    }

    /* P-run-stop (#257) — poll del wire entre quanta (KILL/HELLO/BUSY). */
    s_kill_ack_id = -1;
    s_reset_ack_id = -1;
    if (st == BPVM_OK && !missing[0]) bpvm_set_poll(vm, stm32_run_poll_cb, NULL);

    /* V6/A1.5 - EL HILO `io`, igual que en el PC, el ESP32 y la Pico.
     *
     * Mientras esta tarea (`vm`) ejecuta opcodes, `io` lee el wire y arma la
     * salida POR LINEAS. Aqui esto pesa mas que en ninguna otra placa: la UART
     * va a 115 200 baud y las 2 000 lineas de PrintBench costaban 71 357 ms
     * porque viajaban 820 018 bytes; con una linea por mensaje son ~239 KB.
     *
     * La cola, 2 KB: como el ESP32, que tambien sale por serie. Cuando se llena,
     * `vm` espera a que `io` drene, que es lo correcto - el programa no puede
     * producir mas deprisa de lo que traga el cable. */
    bpvm_io_ops_t io_ops = { stm32_io_poll, v1_output_sink, NULL };
    int con_io = (st == BPVM_OK && !missing[0])
                 && bpvm_io_start(vm, &io_ops, STM32_IO_OQ_BYTES) == 0;

    if (st == BPVM_OK && !missing[0]) st = bpvm_run(vm);
    BOARD_LED_RUN_OFF();

    /* Parar `io` ANTES de nada mas: drena la cola entera, asi que al volver de
     * aqui la ultima linea del programa YA salio por la UART, y el wire vuelve a
     * ser de esta tarea (que es quien manda KILL_REPLY y EXITED). */
    if (con_io) bpvm_io_stop(vm);
    uint32_t dt = HAL_GetTick() - t0;

    /* P-run-stop — ack diferido del KILL, antes del EXITED. */
    bpvm_set_poll(vm, NULL, NULL);
    if (s_kill_ack_id >= 0) { reply_empty("KILL_REPLY", s_kill_ack_id); s_kill_ack_id = -1; }

    if (missing[0]) {
        BOARD_LED_ERR_ON();
        char buf[160];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
            "\"exitCode\":-2,\"elapsedMs\":0,"
            "\"errorMessage\":\"falta el modulo %s en el FS (stdlib no embebida?)\"}",
            session, missing);
        if (n > 0) wire_v1_send_line(buf, (size_t) n);
    } else if (st != BPVM_OK && entry.fallo[0]) {
        /* #421 (17-ago) — el PORQUÉ del fallo de CARGA, con su ruta. El resto
         * de familias lo mandaba desde el 16-ago y ésta seguía con el «IO
         * error» pelado — lo destapó el CENSO (docs/CENSO_FAMILIAS.md §5): el
         * patrón de siempre, el común creció y esta copia no. Va antes del
         * error de enlace porque un fichero que no se puede leer no llega a
         * enlazarse. */
        BOARD_LED_ERR_ON();
        char buf[288];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
            "\"exitCode\":%d,\"elapsedMs\":%lu,\"errorMessage\":\"load: %s\"}",
            session, (int) st, (unsigned long) dt, entry.fallo);
        if (n > 0) wire_v1_send_line(buf, (size_t) n);
    } else {
        if (st != BPVM_OK && st != BPVM_KILLED) BOARD_LED_ERR_ON();
        const char* link_err = bpvm_link_error(vm);   /* paso 4 — "" salvo fallo de link */
        if (link_err[0]) {
            char buf[320];
            int n = snprintf(buf, sizeof(buf),
                "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"LINK_ERROR\","
                "\"exitCode\":%d,\"elapsedMs\":%lu,\"errorMessage\":\"%s\"}",
                session, (int) st, (unsigned long) dt, link_err);
            if (n > 0) wire_v1_send_line(buf, (size_t) n);
        } else {
            /* #406 — el DETALLE del error de ejecucion, no solo la categoria.
             *
             * Esta familia se habia quedado corta: el Pico y los ESP32 ya
             * miraban bpvm_runtime_error() y el STM32 solo trataba link_err, asi
             * que en el IDE salia «RUNTIME_ERROR» pelado y el texto —el motivo
             * real— se perdia. Es el patron de siempre: el comun crecio y una
             * copia privada no.
             *
             * El mensaje lo rellena la VM tanto si el error lo lanza ella como
             * si es un `throw` de clase de usuario sin atrapar (eso ultimo es lo
             * que anadio #406 en src/exceptions.c). */
            const char* rt_err = bpvm_runtime_error(vm);
            if (st != BPVM_OK && st != BPVM_KILLED && rt_err[0]) {
                char buf[320];
                int n = snprintf(buf, sizeof(buf),
                    "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
                    "\"exitCode\":%d,\"elapsedMs\":%lu,\"errorMessage\":\"%s\"}",
                    session, (int) st, (unsigned long) dt, rt_err);
                if (n > 0) wire_v1_send_line(buf, (size_t) n);
            } else {
                emit_exited(session,
                            (st == BPVM_OK)     ? "OK"
                          : (st == BPVM_KILLED) ? "KILLED" : "RUNTIME_ERROR",
                            (st == BPVM_KILLED) ? 130 : (int) st, dt);
            }
        }
    }
    bpvm_destroy(vm);

    /* V6/E1 (#452) — el RESET pedido durante el RUN, servido AHORA: el EXITED ya
     * salió, así que el IDE ve la secuencia entera en vez de un BUSY. No retorna. */
    if (s_reset_ack_id >= 0) {
        long rid = s_reset_ack_id;
        s_reset_ack_id = -1;
        stm32_hacer_reset(rid);
    }
}

static void handle_run(long id, json_obj_t* obj) {
    const char* path = json_str_inplace(obj, "path");   /* #456 - sin copia, sin tope */
    if (path == NULL) {
        wire_v1_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    /* V6/#412 — el argumento de ejecucion: campo ESCALAR opcional. Si no viene,
     * NULL, y manda el valor por defecto que declare el fuente. */
    const char* arg = json_str_inplace(obj, "arg");
    run_module_path(path, id, arg);
}

/* P-autorun (#256) — si existe /sys/auto.txt, ejecuta el módulo de su
 * primera línea por el mismo camino que un RUN del wire. El poll del
 * run atiende HELLO/KILL → el IDE puede conectar y parar la app. */
/* #345 paso 2 — cintura del gate. Cuatro funciones y ni un verbo nuevo: el
 * HELLO que el IDE ya manda al conectar dice "hay alguien", y el KILL que ya
 * manda el Stop dice "no arranques". La política está en el núcleo. */
static void stm32_autorun_anuncia(const char* path, int ventana_ms, void* user) {
    (void) user;
    log_printf("autorun: %s arranca en %d ms — Stop en el IDE para cancelar",
               path, ventana_ms);
}

static int stm32_autorun_escucha(void* user) {
    (void) user;
    int c = stm32_wire_getchar();
    if (c < 0) return 0;                       /* nada pendiente */
    int n = wire_v1_recv_line(c, s_line, sizeof(s_line));
    if (n < 0) return 1;                       /* línea rota, pero HAY alguien */
    json_obj_t obj;
    if (json_parse(s_line, (size_t) n, &obj) != 0) return 1;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { reply_empty("KILL_REPLY", rid); return 2; }
    if (strcmp(type, "HELLO") == 0) { bpvm_repl_dispatch(type, rid, &obj); return 1; }
    return 1;                                  /* hay alguien: basta con eso */
}

static void stm32_autorun_espera(int ms, void* user) {
    (void) user; HAL_Delay((uint32_t) ms);
}

static uint32_t stm32_autorun_ahora(void* user) {
    (void) user; return HAL_GetTick();
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    stm32_autorun_anuncia, stm32_autorun_escucha,
    stm32_autorun_espera,  stm32_autorun_ahora, NULL
};

static void autorun_boot(void) {
    /* #345 — leer y limpiar la primera línea lo hace el núcleo. (H11 sigue
     * valiendo: sólo la CABEZA del fichero, nunca un espejo.) */
    char path[64];
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */

    /* #345 paso 2 — la ventana de rescate. Decide el usuario. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        return;
    }
    log_printf("autorun: %s", path);
    run_module_path(path, -1, NULL);   /* autorun: sin argumento, manda el defecto */
}

/* ---- dispatch ---- */

static void dispatch(int first_char) {
    int len = wire_v1_recv_line(first_char, s_line, sizeof(s_line));
    if (len == -2) return;   /* línea estancada: silencio; el IDE reintenta */
    if (len < 0)  { stm32_wire_send_fatal("PROTOCOL_ERROR", "line too long"); return; }

    json_obj_t obj;
    if (json_parse(s_line, (size_t) len, &obj) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bad JSON"); return;
    }
    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "missing type"); return;
    }

    /* H9 — gestión de placa (STATE/ENV_x/PART_x + H3 PACK_x): el host configura el
     * env/particiones cuando el arranque no llegó al FS, y consulta/graba la zona
     * de packs. Las replies las pone bpvm_bmgr_wire (idénticas al boardsim y a las
     * otras placas). s_put_buf presta el scratch; el bulk de PACK_BURN_DATA va a
     * su buffer propio (chunk ≤ 4K) para no pisar el scratch. */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        || strncmp(type, "PACK_", 5) == 0) {
        static uint8_t s_burn_chunk[BPVM_PACK_BURN_CHUNK];
        const uint8_t* bulk_ptr = NULL;
        unsigned long  bulk_n   = 0;
        long bulk = json_get_long(&obj, "bulk", -1);
        if (bulk > 0) {
            /* CRÍTICO: consumir SIEMPRE los bytes anunciados (como el PUT) para
             * no desincronizar el wire, quepan o no. */
            if ((size_t) bulk > sizeof s_burn_chunk) {
                size_t rem = (size_t) bulk;
                while (rem > 0) {
                    size_t chunk = rem < sizeof s_burn_chunk ? rem : sizeof s_burn_chunk;
                    if (wire_v1_recv_bulk(s_burn_chunk, chunk, sizeof s_burn_chunk) < 0) {
                        stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
                    }
                    rem -= chunk;
                }
                wire_v1_send_error(id, "INVALID_PARAM", "chunk demasiado grande");
                return;
            }
            if (wire_v1_recv_bulk(s_burn_chunk, (size_t) bulk, sizeof s_burn_chunk) < 0) {
                stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
            }
            bulk_ptr = s_burn_chunk;
            bulk_n = (unsigned long) bulk;
        }
        board_mgr_stm32_handle(id, &obj, type, s_put_buf, sizeof s_put_buf, bulk_ptr, bulk_n);
        return;
    }
    /* H9 — gating: sin FS montado, las ops de fichero no valen; sin VM, el RUN no.
     * HELLO/INFO/STATE/PING/TIME/RESET van SIEMPRE (para conectar y configurar). */
    {
        int st = (int) board_boot_status()->state;
        int is_fs = (strcmp(type, "LIST")  == 0 || strcmp(type, "STAT") == 0 || strcmp(type, "DF")  == 0
                  || strcmp(type, "GET")   == 0 || strcmp(type, "PUT")  == 0 || strcmp(type, "DEL") == 0
                  || strncmp(type, "PUT_", 4) == 0   /* #294 streaming: PUT_BEGIN/DATA/END */
                  || strcmp(type, "MKDIR") == 0 || strcmp(type, "FORMAT") == 0
                  || strcmp(type, "RENAME") == 0 || strcmp(type, "RMDIR") == 0);
                  /* V6/U3 g2: RENAME/RMDIR entran por el común, pero el gate
                   * de FS-no-montado es de esta familia y debe cubrirlos. */
        if (is_fs && st < BPVM_BOOT_FS) {
            /* El bulk que esta puerta rechaza hay que TRAGARSELO igual, o se
             * queda en el cable y el mensaje siguiente se lee desde la mitad.
             * Aquí no lo ha leído nadie todavía: el bloque de PACK_* retorna
             * antes de llegar a esta puerta. */
            long b_pend = json_get_long(&obj, "bulk", 0);
            if (b_pend > 0) (void) bpvm_repl_drain_bulk((unsigned long) b_pend);
            wire_v1_send_error(id, "NOT_READY", "FS no montado (configura particiones)");
            return;
        }
        if (strcmp(type, "RUN") == 0 && st < BPVM_BOOT_APP) {
            wire_v1_send_error(id, "NOT_READY", "VM no lista (configura particiones)");
            return;
        }
    }

    /* V6/U3 — PRIMERO el común: PING/TIME/LOG_DUMP/LOG_CLEAR viven en
     * src/bpvm_repl.c para las cuatro familias. Si lo atiende, hemos acabado.
     * Los verbos de esta familia siguen debajo y van migrando por grupos. */
    if (bpvm_repl_dispatch(type, id, &obj)) return;

    else if (strcmp(type, "RUN")       == 0) handle_run(id, &obj);
    /* P-run-stop (#257) — KILL en idle: nada que matar (el útil llega
     * DURANTE un RUN y lo atiende stm32_run_poll_cb). */
    else if (strcmp(type, "KILL")      == 0)
        wire_v1_send_error(id, "NO_SESSION", "no hay programa en ejecución");
    else if (strcmp(type, "RESET")     == 0) { stm32_hacer_reset(id); }
    else {
        char msg[96];
        snprintf(msg, sizeof(msg), "type '%s' no implementado (H9.2)", type);
        wire_v1_send_error(id, "UNSUPPORTED", msg);
    }
}

/* #353 — sink de diagnóstico de la VM: al log persistente. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

/* V6/U6.11 — ENUMERAR, PLANIFICAR, TOMAR: la misma terna que Pico y ESP32, sobre
 * la memoria más simple del parque. La familia ofrece UNA región: el array
 * estático, exclusivo de la VM (nadie más asigna en él) y sin margen — lo que se
 * deja a malloc y a la pila del MSP no se lo deja la VM: lo exige el enlazador
 * (`._user_heap_stack`). Objetivo 0 = el array entero; suelo, el de las demás.
 * El número es el de siempre (512 KB → 384 de heap + 128 de pilas por la regla
 * común), pero decidido y CONTADO con la misma función y la misma línea que las
 * otras cuatro placas — y `test_mem` lo fija con este caso. */
/* V6/U4 — lo propio de esta familia para el instalador común: el put y el log. */
static int  stm32_mods_put(const char* p, const uint8_t* d, uint32_t n) { return fs_put(p, d, n) == 0 ? 0 : -1; }
static void stm32_mods_log(const char* l) { log_printf("%s", l); }

static int stm32_mem_regiones(bpvm_mem_region_t* out, int max) {
    if (max < 1) return 0;
    out[0].base      = s_vm_mem;
    out[0].bytes     = sizeof s_vm_mem;
    out[0].libre     = sizeof s_vm_mem;      /* un solo bloque: lo contiguo ES el total */
    out[0].exclusiva = 1;
    out[0].margen    = 0;                    /* el margen lo puso el enlazador, no la VM */
    out[0].nombre    = "SRAM estática";
    return 1;
}

static void stm32_mem_planificar(void) {
    bpvm_mem_region_t reg[1];
    int nreg = stm32_mem_regiones(reg, 1);
    bpvm_mem_cfg_t cfg;
    cfg.objetivo       = 0;                  /* todo lo que el plan deje: el array entero */
    cfg.vm_min         = VM_MEM_MIN;
    cfg.reserva_bytes  = 0;                  /* sin SQLite en SRAM: no hay reserva con nombre */
    cfg.reserva_nombre = NULL;
    bpvm_mem_plan_t plan;
    bpvm_mem_plan(reg, nreg, &cfg, &plan);
    char linea[160];
    bpvm_mem_plan_str(&plan, reg, &cfg, linea, sizeof linea);
    log_printf("%s", linea);
    s_vm_bytes = (plan.res == BPVM_MEM_OK) ? plan.bytes : 0;
}

void stm32_repl_run(void) {
    /* H9 — arranque ESCALONADO: identidad → particiones del env → FS → VM, parando
     * en la 1ª capa que falla. Sin particiones/FS el climb se queda abajo y el host
     * conduce (Gestión de placa: proponer defaults → aplicar → reset). Sustituye al
     * fs_load() de región fija. */
    log_init();                     /* recupera el log de la sesión anterior (post-mortem) */
    /* #439 — y DE DONDE sale lo recuperado: sin esto, un volcado con dos
     * arranques no distingue «la RAM sobrevivio al reset» de «se cargo de
     * flash lo que el arranque anterior volco». */
    log_printf(bpvm_log_origen_ram()
               ? "log: RAM SUPERVIVIENTE (lineas de ANTES del reset)"
               : "log: arranque en frio (cargado de flash)");
    stm32_mem_planificar();         /* V6/U6.11 — la línea `vm:` de las cinco placas, al abrir el boot */
    /* #353 — lo que dice la VM (deps que faltan, packs, veredicto del guardián
     * de #339) al log. Aquí el problema era el opuesto al de la Pico: sin un
     * `_write` retargeteado, el stderr del núcleo se PERDÍA. Esto devuelve al
     * log los avisos de pack que este port tenía a mano antes de #344. */
    bpvm_repl_set_ops(&s_repl_ops);   /* V6/U3 g4: la cintura, antes del primer mensaje */
    bpvm_diag_set_sink(diag_al_log);
    board_mgr_stm32_boot();
    const bpvm_boot_status_t* bs = board_boot_status();
    log_printf("boot: estado %d (%s)%s%s", (int) bs->state, bpvm_boot_state_name(bs->state),
               bs->degraded ? " DEGRADED: " : "", bs->degraded ? bs->reason : "");
    /* stdlib core embebida en /lib SOLO con el FS montado (si el climb no llegó al
     * FS, no hay dónde instalarla; el host configura particiones primero). */
    if (bs->state >= BPVM_BOOT_FS) {
        /* V6/U4 — la stdlib embebida a /lib: tabla GENERADA (stm32_mods.c, sólo
         * datos); bucle y regla (#466) en src/bpvm_mods.c; aquí sólo el put. */
        (void) bpvm_mods_instalar_tabla(stm32_mods, stm32_mods_n, stm32_mods_put, stm32_mods_log);
        stm32_fs_register_bpvm();   /* #247 — readFile/writeFile/... sobre el FS */
        log_printf("fs: %u/%u bytes usados", (unsigned) fs_used_bytes(),
                   (unsigned) fs_total_bytes());
    }
    stm32_hw_register();            /* backends de HW (GPIO, info de MCU) — siempre */
    log_flush();                    /* persiste la historia del boot (post-mortem tras corte) */

    /* FIFO RX/TX (8 bytes): absorbe el hueco de procesado entre la línea JSON
     * y los bytes bulk que la siguen → PUT fiable aunque la CPU vaya lenta.
     * (El fix definitivo del timing es subir el reloj a 160 MHz.) */
    HAL_UARTEx_SetTxFifoThreshold(BOARD_WIRE_UART, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(BOARD_WIRE_UART, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(BOARD_WIRE_UART);

#if defined(BOARD_WIRE_IRQn)
    /* RX por IRQ → ring (V3/H5.2 DK2; H10 también Nucleo). La FIFO de 8B (≈700µs)
     * no basta cuando el lazo deja el UART sin sondear ms (bombeo de LVGL en
     * Gui.run()) y se perdían los primeros bytes del KILL. Con la IRQ drenando a un
     * ring de 256B, getchar() no pierde nada y el GUI puede dormir entre frames
     * (__WFI). Tras EnableFifoMode para que RXFNE refleje la FIFO ya activa.
     * La placa opta definiendo BOARD_WIRE_IRQn en board.h. */
    stm32_wire_rx_irq_enable();
#endif

    wire_v1_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");
    {   /* H10 — causa del último reset (diagnóstico; revela WDT/soft/power-on). */
        char rc[64];
        snprintf(rc, sizeof(rc), "reset cause: %s", stm32_reset_cause());
        wire_v1_send_cstr(rc);
    }

    /* P-autorun (#256) — el wire ya está vivo: si la app de auto.txt se
     * queda en bucle, el IDE puede conectar (HELLO) y matarla (KILL). */
    /* #423 — A PARTIR DE AQUI, EL LOG LO MANDA EL ENTORNO (`log=0|1`).
     * Mismo punto que en las otras familias: el arranque entero se registra
     * siempre (son ~15 lineas y hacen falta cuando una placa no arranca) y lo
     * que se apaga es el rastro de EJECUCION, que es el que llena la region.
     * Por defecto APAGADO: `log=1` cuando se va a depurar. */
    /* `stack=N` (KB) — el usuario reparte el bloque entre pilas y heap; ausente
     * o 0 = el reparto de siempre. Mismo sitio y misma clave que en las otras
     * familias: la regla vive en bpvm_stack_region_bytes(). Va aquí, ANTES del
     * primer RUN, que es quien llama a bpvm_init. */
    bpvm_set_stack_kb((unsigned long) board_mgr_stm32_env_long("stack", 0));
    bpvm_set_quantum_ops(board_mgr_stm32_env_long("quantum", 0));   /* #462 */

    bpvm_log_set_enabled(board_mgr_stm32_env_bool("log", 0));

    autorun_boot();

    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') dispatch(c);

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat */
            last_blink = now;
            BOARD_LED_BEAT_TOGGLE();
        }
    }
}
