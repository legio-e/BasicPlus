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
#include "bpvm_machine.h"   /* HAL BP: los dos nombres de la identidad */
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "stm32_fs.h"
#include "board_mgr_stm32.h"  /* H9 — arranque escalonado + STATE/ENV/PART */
#include "bpvm_boot2.h"       /* V7/H7 (Boot 2, fase 4): el arranque común, con los ganchos de aquí */
#include "bpvm_sd_placa.h"    /* V7/H7 (Boot 2, fase 4): el porqué de no tener SD, al registro común */
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
#include "bpvm_platform.h"   /* #473 (punto 7): ceder un tick en el lazo ocioso */
#include "bpvm_internal.h"   /* recorrido de vm->modules[] para los .mdn del AOT */
#include "bpvm_entry.h"      /* #344 — el RUN, escrito una vez */
#include "bpvm_rtc.h"        /* H10 — TIME aplica la hora al RTC (bpvm_rtc_set_now_ms) */
#include "mdn_loader.h"      /* H9.5: overlay AOT .mdn desde el FS (loader compartido) */
#include "bpvm_mdn_scan.h"   /* V7/H2 (#493): el escaner comun de .mdn (FS + zona) */
#include "aot_registry.h"
#include "bpvm_repl.h"       /* V6/U3: los verbos COMUNES del REPL (grupo 1: meta) */
#include "bpvm_mem.h"        /* V6/U6.11: el planificador de memoria COMÚN (una regla, cinco placas) */
#include "bpvm_mods.h"       /* V6/U4: el bucle y la regla de la stdlib embebida, comunes */    /* H9.5: bpvm_aot_clear entre RUNs (registry global) */
#include "bpvm_bios.h"       /* V7/H6 G5.b0: la BIOS de los packs nativos, comprobada al arrancar */
#include "bpvm_sqlmem.h"     /* V7/H6 G5.b0: la regla de `SQLite=` para el bloque de los motores */
#include "pack_stm32.h"      /* V7/H6 G5.b0: el cargador de motores y su bloque de RAM */
#include "gui_display_ltdc.h" /* V7/H6 G5.b: la pantalla de la DK2, publicada como 'PANT' */

#include "main.h"
#include "board.h"              /* placa: BOARD_WIRE_UART, BOARD_NAME, BOARD_SRAM_BYTES, BOARD_LED_* */

#include <string.h>
#include <stdio.h>
#include <malloc.h>     /* V7/H7: mallinfo, para la línea `mem:` */
#include "FreeRTOS.h"   /* V7/H7: el heap del RTOS, ídem */

#define SERVER_NAME "bpvm-stm32"

static void stm32_mem_log(void);   /* V7/H7: la línea `mem:` (junto a stm32_mem_regiones) */
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
/* V7/H8 #495 — el búfer de FICHEROS del PUT troceado, aparte del de comunicaciones: una página de la flash
 * (FLASH_PAGE_SIZE, 8 KB en el U5), el bloque de littlefs en esta familia (FS_BLOCK_SIZE, fs_lfs_stm32.c). */
static uint8_t s_fich_buf[FLASH_PAGE_SIZE];
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= BP_ENV_SECTOR + 512u) ? 1 : -1];
/* H13 hallazgo 31 (5-ago) — 128 KB era un DESCUIDO, no una decisión: se fijó al
 * nacer el port y nadie volvió a mirarlo. Medido sobre el ELF de la Nucleo: de
 * los 768 KB de RAM el estático total eran ~247 KB (128 de éstos) ⇒ ~520 KB
 * PARADOS, en la placa que tenía el heap MÁS PEQUEÑO del parque (64 KB, contra
 * 96 del S3 y 257 de la Pico). Con 512 KB la regla común (25 % pilas, suelo 64,
 * techo 512) reparte 384 de heap + 128 de pila. (V8 #740: la regla es ahora la
 * pila del main, 16 KB; las de los hilos salen del heap: 496 de heap.)
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
/* V7/H6 G5.b0 (26-sep) — EN SU SITIO FIJO: la sección `.bss.bp_vm` la pone el `.ld`
 * de cada placa LO PRIMERO de la RAM (0x20000000). Del principio sale el bloque de
 * los packs nativos (packRam= del ENV), y los motores grabados llevan esa dirección
 * en su SELLO: en la `.bss` se movía con cada enlace. El arranque lo comprueba
 * contra los símbolos del `.ld` (ver stm32_mem_planificar). */
static uint8_t s_vm_mem[BOARD_VM_BYTES] __attribute__((section(".bss.bp_vm"), aligned(8)));
/* Donde empieza la VM de verdad: detrás del bloque de los motores, si lo hay. */
static uint8_t* s_vm_base = s_vm_mem;
extern char __bp_vm_start[];   /* el .ld: la sección de la VM */
extern char __bp_ram_start[];  /* el .ld: ORIGIN(RAM) */
/* V6/U6.11 — lo que el planificador COMÚN (bpvm_mem) decidió sobre ese array.
 * Aquí no hay nada que medir en marcha: la región es el array entero, EXCLUSIVA
 * de la VM y SIN margen, porque el margen de esta familia lo pone el ENLAZADOR
 * (si `_Min_Heap_Size + _Min_Stack_Size` no caben detrás del estático, no enlaza,
 * y eso es más fuerte que cualquier comprobación en marcha). Pero la decisión y
 * la línea del log son las mismas que en las otras cuatro placas. 0 = sin plan. */
static size_t  s_vm_bytes = 0;
#define VM_MEM_MIN  (64u * 1024u)             /* el suelo de las cinco placas */
/* #691 (12-oct) — por qué no hay VM, si el plan no la dio (vacío si la hay): el motivo de la capa de la app del
 * arranque (board_mgr_stm32.c), por stm32_vm_sin_bloque(). */
static char    s_vm_motivo[BPVM_BOOT_REASON_MAX] = "";

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

static void packs_cargar_una_vez(void);   /* V7/H6 G5.b0: más abajo, junto al RUN */

/* V7/H8 #602 — para el INFO común: el motivo de la regla, la clave del ENV que pidió el bloque y lo que pedía. */
static int         s_pack_ram_res    = 0;
static const char* s_pack_ram_clave  = NULL;
static long        s_pack_ram_pedido = 0;

static void stm32_repl_info(bpvm_repl_info_t* o) {
    /* V7/H6 G5.b (26-sep) — como la ESP32 (pregunta 6 de V7_IDEAS §H6): el INFO publica screenW/H sólo si hay
     * render, y con la GUI por el pack eso exige el pack CARGADO; la tanda y el IDE preguntan el tamaño de
     * pantalla ANTES del primer Run. Así que el INFO también dispara la carga «una vez» (reintentable: si no
     * hay pack no marca nada). La política de no cargar en el ARRANQUE no cambia: esto ya es el host hablando,
     * y un cuelgue aquí no se repite en cada reset. Sin esto, la DK2 sin LVGL en la imagen decía «sin
     * pantalla» y la tanda SALTABA las pruebas de GUI. */
    packs_cargar_una_vez();
    /* V7/H8 #602 — el bloque de RAM de los motores (stm32_mem_planificar), al INFO común. */
    o->pack_ram_base   = (unsigned long) (uintptr_t) s_pack_ram_base;
    o->pack_ram_bytes  = (unsigned long) s_pack_ram_size;
    o->pack_ram_res    = s_pack_ram_res;
    o->pack_ram_clave  = s_pack_ram_clave;
    o->pack_ram_pedido = s_pack_ram_pedido;
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
    bpvm_machine_micro_name(micro, sizeof micro);
    o->board_name    = micro;
    o->reset_reason  = stm32_reset_cause();
    o->arch          = (unsigned) bpvm_mdn_host_arch();
    o->cpu_hz        = (unsigned long) SystemCoreClock;
    o->uptime_ms     = (unsigned long) HAL_GetTick();
    /* V8 #826 (15-nov): la del sensor del ADC, por la fachada (Machine.tempC), como la Pico; antes, un 0 escrito a mano. */
    o->temp_milli_c  = (long) (bpvm_machine_temp_c() * 1000.0f);
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
static int           stm32_repl_fs_format(void){ return fs_format(); }   /* #473 (466): lo que pasó */
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

/* V7/H9 (2-oct) — el sello de la imagen: el CRC32 de la flash de FLASH_BASE al final de los valores iniciales de .data
 * (_sidata + su tamaño): la imagen que graba la sonda, la misma que da `objcopy -O binary` del .elf. Una vez. */
extern uint8_t _sidata[], _sdata[], _edata[];
static uint32_t stm32_image_crc(void) {
    static uint32_t s_crc = 0;
    static int s_hecho = 0;
    if (!s_hecho) {
        const uint8_t* a = (const uint8_t*) FLASH_BASE;
        s_crc = bpvm_crc32(a, (size_t) (_sidata - a) + (size_t) (_edata - _sdata));
        s_hecho = 1;
    }
    return s_crc;
}

static const bpvm_repl_run_ops_t s_run_ops;   /* V7/H10: la cintura del RUN común, más abajo */

static const bpvm_repl_ops_t s_repl_ops = {
    .run           = &s_run_ops,   /* V7/H10: la cintura del RUN común */
    .info          = stm32_repl_info,
    .image_crc     = stm32_image_crc,
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
    .fich_buf      = s_fich_buf,      /* V7/H8 #495: una página de la flash */
    .fich_bytes    = sizeof s_fich_buf,
    .put_trozo     = 512,             /* V7/H8 #495: 4 trozos de 512 B = 2 KB, con sus cabeceras dentro del
                                       * anillo de 4 KB que llena el DMA (Eduardo, 1-oct) */
    .put_ventana   = 4,
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

/* ---- TERMINAL ----
 * El RUN es el común (V7/H10: `bpvm_repl_run`, en src/bpvm_repl.c), con la cintura de abajo. El de esta familia —su
 * sink (que tiraba la línea cuyo escape no cabía), su EXITED y su poll— se borró en el paso 6 (3-oct), probado ya en
 * placa; con él, 4,3 KB de búferes estáticos. */

/* El reinicio, en UN sitio: lo piden el despachador de reposo y —desde #452— el
 * final de un RUN que recibió RESET mientras corría. No retorna. */
static void stm32_hacer_reset(long id) {
    log_printf("RESET (wire): reinicio");
    log_flush();                 /* persiste la sesión antes de reiniciar */
    reply_empty("RESET_REPLY", id);
    HAL_Delay(50);
    NVIC_SystemReset();
}

/* V6/A1.5 — la cola de salida de `io`, 2 KB: como la ESP32, que también sale por serie. */
#define STM32_IO_OQ_BYTES 2048

/* V7/H2 (#493) — la cintura del escáner común de `.mdn` (bpvm_mdn_scan.h): del
 * FS al buffer ejecutable. H11 — el .mdn es ZERO-COPY: los thunks se registran
 * como punteros DENTRO de este buffer, así que tiene que seguir vivo y en RAM
 * toda la ejecución; se reserva de la arena de la VM exactamente lo que ocupa,
 * 4-alineado (Thumb-2). El código ejecuta desde SRAM (S-bus); el ICACHE del U5
 * cachea la ruta de flash (C-bus), así que no hace falta invalidación. */
static const uint8_t* stm32_mdn_del_fs(void* user, const char* nombre, uint32_t* len) {
    bpvm_t* vm = (bpvm_t*) user;
    char real[BPVM_FS_PATH_MAX]; uint32_t size = 0;   /* paso 6: 80 B cortaban las rutas largas (el RUN ya las valía) */
    if (bpvm_entry_resolve(nombre, real, sizeof real, &size) != 0) return NULL;   /* #344: la regla común */
    uint8_t* dst = bpvm_arena_reserve(vm, size, 4);
    if (!dst) {
        log_printf("AOT: %s (%lu B) no cabe en la arena — sin overlay", real, (unsigned long) size);
        return NULL;
    }
    if (bpvm_fs_read(real, dst, size) != (long) size) {
        log_printf("AOT: %s no se pudo leer — sin overlay", real);
        return NULL;
    }
    *len = size;
    return dst;
}

/* V7/H6 G5.b0 — LOS MOTORES NATIVOS SE CARGAN AQUÍ, EN EL PRIMER Run (como la Pico:
 * pico/repl_v1.c). Un motor publica su API al arrancar (`bp_pack_init` -> `publica`)
 * y el programa la busca por su marca, así que tiene que estar cargado ANTES. En el
 * Run y no en el arranque (Eduardo, 7-ago): el salto es lo único que puede colgar,
 * y un cuelgue en un Run se arregla desenchufando una vez. Una vez, y sólo si salió
 * bien: volver a saltar publicaría la misma marca dos veces; si no había motor, el
 * siguiente Run lo reintenta — justo lo que se quiere tras grabarlo. */
static void packs_cargar_una_vez(void) {
    /* V7/H7 (Boot 2, fase 2): la guarda y el log, en el cargador común. */
    (void) pack_stm32_cargar();
}

/* ── V7/H10, el RUN común, paso 3 (3-oct): LA CINTURA DE LA STM32 ─────────────────────────────────────────────────
 * Lo que es suyo de verdad: el bloque de la VM (el que planificó el arranque), leer el anillo de la RX sin esperar,
 * sus búferes, el reinicio, los motores nativos, el `.mdn` del FS a la arena, el LED y la línea `mem:`. Con esto, la
 * STM32 deja de tirar la línea cuyo escape no cabía (el sink común), y gana el depurador. */
/* #691 — ver stm32_repl.h. */
const char* stm32_vm_sin_bloque(void) {
    if (s_vm_bytes) return NULL;
    return s_vm_motivo[0] ? s_vm_motivo : "la VM no tiene bloque";
}

static int stm32_run_memoria(uint8_t** base, size_t* bytes) {
    if (s_vm_bytes == 0) return -1;             /* sin plan: no se arranca a medias */
    *base = s_vm_base;
    *bytes = s_vm_bytes;
    return 0;
}
static int stm32_run_leer_linea(char* buf, size_t cap) {
    int c = stm32_wire_getchar();
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, buf, cap);
    if (n == -2) bpvm_repl_linea_estancada();   /* V8 #693: se dice (y el RUN sigue: su 0) */
    return (n < 0) ? 0 : n;                     /* rota o estancada: se descarta */
}
static void stm32_run_led(int estado) {
    if (estado > 0) { BOARD_LED_RUN_ON(); return; }
    BOARD_LED_RUN_OFF();
    if (estado < 0) BOARD_LED_ERR_ON();
}
static void stm32_run_despues(bpvm_t* vm) {
    (void) vm;
    stm32_mem_log();                            /* V7/H7: tras cada RUN, antes de soltar la VM */
}
static char s_run_dbg_linea[512];               /* los comandos del depurador son cortos (un pc, un id) */
static char s_run_mensaje[1024];                /* RUN_REPLY, EXITED y las respuestas del depurador */

static const bpvm_repl_run_ops_t s_run_ops = {
    .memoria           = stm32_run_memoria,
    .leer_linea        = stm32_run_leer_linea,
    .linea             = s_line,          .linea_cap     = sizeof s_line,   /* el lazo está parado durante el RUN */
    .dbg_linea         = s_run_dbg_linea, .dbg_linea_cap = sizeof s_run_dbg_linea,
    .mensaje           = s_run_mensaje,   .mensaje_cap   = sizeof s_run_mensaje,
    .reiniciar         = stm32_hacer_reset,
    .cargar_motores    = packs_cargar_una_vez,
    .mdn_del_fs        = stm32_mdn_del_fs,
    .despues_de_correr = stm32_run_despues,
    .led               = stm32_run_led,
    .env_bool          = board_mgr_stm32_env_bool,
    .io_cola           = STM32_IO_OQ_BYTES,
};

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

static void stm32_autorun_espera(int ms, void* user) {
    (void) user; HAL_Delay((uint32_t) ms);
}

static uint32_t stm32_autorun_ahora(void* user) {
    (void) user; return HAL_GetTick();
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    stm32_autorun_anuncia, bpvm_repl_autorun_escucha,   /* V7/H10, el RUN común, paso 6: la escucha, la común */
    stm32_autorun_espera,  stm32_autorun_ahora, NULL
};

static void autorun_boot(void) {
    /* #345 — leer y limpiar la primera línea lo hace el núcleo. (H11 sigue
     * valiendo: sólo la CABEZA del fichero, nunca un espejo.) */
    char path[BPVM_FS_PATH_MAX];   /* paso 6: 64 B cortaban las rutas largas (el RUN ya las valía) */
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */

    /* #345 paso 2 — la ventana de rescate. Decide el usuario. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        return;
    }
    log_printf("autorun: %s", path);
    bpvm_repl_run(-1, path, NULL);   /* V7/H10: el RUN común; sin argumento, manda el defecto */
}

/* ---- dispatch ---- */

static void dispatch(int first_char) {
    int len = wire_v1_recv_line(first_char, s_line, sizeof(s_line));
    if (len == -2) { bpvm_repl_linea_estancada(); return; }   /* línea estancada: se dice (V8 #693); el IDE reintenta */
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
    bpvm_repl_antes_de_despachar(type);   /* #495: un verbo que no convive con una subida a medias, la acaba */

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
    /* V7/H10 #473 (punto 10): la puerta, en el común. Aquí el bulk sigue en el cable siempre: no lo ha leído nadie
     * todavía (el bloque de PACK_* retorna antes de llegar a esta puerta). */
    if (bpvm_repl_puerta_arranque(type, id, &obj, board_boot_status(), 1)) return;   /* #691: con su motivo */

    /* V6/U3 — PRIMERO el común: PING/TIME/LOG_DUMP/LOG_CLEAR viven en
     * src/bpvm_repl.c para las cuatro familias. Si lo atiende, hemos acabado.
     * Los verbos de esta familia siguen debajo y van migrando por grupos. */
    /* V7/H10, el RUN común, paso 6: también el RUN, el KILL sin programa y el depurador de antes del RUN (que esta
     * familia estrenó con el RUN común) —durante un RUN, todo lo del cable lo lee el poll común—. */
    if (bpvm_repl_dispatch(type, id, &obj)) return;

    if (strcmp(type, "RESET") == 0) { stm32_hacer_reset(id); }
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
/* V7/H6 (#468) — la instalación entera, registrada para volver a correrla cuando
 * cambia la zona de packs (el wire común: grabar, borrar o formatear un pack). */
static void stm32_mods_install(void) {
    bpvm_mods_set_resync(stm32_mods_install);
    (void) bpvm_mods_instalar_tabla(stm32_mods, stm32_mods_n, stm32_mods_put, stm32_mods_log);
}

/* V7/H7 (30-sep) — EL MARGEN, MEDIDO. Lo que esta familia deja fuera de la VM
 * lo reserva el enlazador (`._user_heap_stack`) y hasta hoy nadie decía cuánto de
 * eso se usa: la línea `mem:` sólo existía en la ESP32. Son tres cosas:
 *  · malloc, de `_end` al principio de la pila del MSP: newlib-nano NUNCA devuelve
 *    memoria a sbrk, así que lo que ha tomado (`arena`) es su marca de agua; y lo
 *    que hay en uso ahora (si crece de un RUN a otro, es una fuga). Ahí va también
 *    la pila del ejecutor de cada nativo (8 KB, src/platform_freertos.c);
 *  · el heap de FreeRTOS (`ucHeap`): las pilas de las tareas —la de `vm`, 16 KB,
 *    y la de `io`, 4 KB; los hilos BP no son tareas— y los objetos del kernel;
 *  · la pila del MSP (`_Min_Stack_Size`): desde que el intérprete corre en la tarea
 *    `vm` (A1.5) sólo la usan las interrupciones —FreeRTOS la devuelve a `_estack`
 *    al arrancar—. Se pinta en main() antes de arrancar el planificador y se
 *    cuenta lo que sigue pintado. Lo que main() tenía por encima al pintar cuenta
 *    como usado: el número es un techo, no un suelo. */
#define MSP_PINTURA 0xA5A5A5A5u

void stm32_msp_pintar(void) {
    extern uint32_t _sstack;
    uint32_t* p  = &_sstack;
    uint32_t* sp = (uint32_t*) __get_MSP();
    while (p + 4 < sp) *p++ = MSP_PINTURA;   /* lo de debajo del SP de ahora no es de nadie */
}

static void stm32_mem_log(void) {
    extern uint8_t  _end, _estack;
    extern uint32_t _sstack, _Min_Stack_Size;
    uint32_t msp_total = (uint32_t) &_Min_Stack_Size;
    const uint32_t* p  = &_sstack;
    const uint32_t* fin = (const uint32_t*) &_estack;
    while (p < fin && *p == MSP_PINTURA) p++;
    uint32_t msp_libre = (uint32_t) ((const uint8_t*) p - (const uint8_t*) &_sstack);
    struct mallinfo mi = mallinfo();
    log_printf("mem: malloc en uso %u B | MAXIMO %u B de %u | heap RTOS libre %u B de %u, MINIMO HISTORICO %u B"
               " | pila MSP usada %u B de %u",
               (unsigned) mi.uordblks, (unsigned) mi.arena,
               (unsigned) ((const uint8_t*) &_sstack - &_end),
               (unsigned) xPortGetFreeHeapSize(), (unsigned) configTOTAL_HEAP_SIZE,
               (unsigned) xPortGetMinimumEverFreeHeapSize(),
               (unsigned) (msp_total - msp_libre), (unsigned) msp_total);
}

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

/* V7/H6 G5.b0 — Y EL BLOQUE DE LOS MOTORES NATIVOS, DELANTE. Es la reserva con
 * nombre del planificador común (la misma con la que la Metro aparta la BD), del
 * tamaño que diga el ENV: `packRam=<KB>` (la clave genérica, la de la ESP32) o, si
 * no está, `SQLite=<MB>` con su regla (bpvm_sqlmem: 2 MB de mínimo). Si no deja a
 * la VM su suelo, el plan no la aparta y aquí se dice: la VM arranca igual (mejor
 * sin motores que sin VM). Como el array está LO PRIMERO de la RAM, el bloque
 * empieza siempre en 0x20000000; y si el `.ld` no lo hubiera puesto ahí, no hay
 * bloque: un motor sellado contra una dirección que se mueve con cada imagen
 * dejaría de valer en silencio al grabar la siguiente. */
static void stm32_mem_planificar(void) {
    bpvm_mem_region_t reg[1];
    int nreg = stm32_mem_regiones(reg, 1);

    long pack_kb = board_mgr_stm32_env_long("packRam", 0);
    long sql_mb  = board_mgr_stm32_env_long("SQLite", 0);
    /* V7/H7 (Boot 2, fase 3) — CUÁNTO, la regla común de las cinco (bpvm_packram_region):
     * `packRam=` en páginas de 4 KB y, si no está, `SQLite=` con la suya; las dos dejan a la
     * VM su suelo (128 KB: aquí era el del plan, 64). Aquí sólo se decide DÓNDE: la reserva
     * del plan, delante de la VM, en 0x20000000. */
    size_t pide = 0;
    const char* nombre = NULL;
    bpvm_sqlite_res_t sqlres = bpvm_packram_region(pack_kb, sql_mb, reg[0].bytes, &pide);
    s_pack_ram_res    = (int) sqlres;                                                   /* #602 */
    s_pack_ram_clave  = (pack_kb > 0) ? "packRam" : (sql_mb > 0) ? "SQLite" : NULL;
    s_pack_ram_pedido = (pack_kb > 0) ? pack_kb : sql_mb;
    if (sqlres == BPVM_SQLITE_OK) nombre = (pack_kb > 0) ? "packs" : "SQLite"; else pide = 0;
    const int en_su_sitio = ((uintptr_t) s_vm_mem == (uintptr_t) __bp_vm_start)
                         && ((uintptr_t) __bp_vm_start == (uintptr_t) __bp_ram_start);
    if (pide && !en_su_sitio) {
        log_printf("packs: SIN bloque de RAM — la VM no esta lo primero de la RAM (s_vm_mem 0x%08lX, "
                   ".bp_vm 0x%08lX, RAM 0x%08lX): el .ld no la pone en su sitio y el sello de un motor "
                   "dejaria de valer con la siguiente imagen",
                   (unsigned long) (uintptr_t) s_vm_mem, (unsigned long) (uintptr_t) __bp_vm_start,
                   (unsigned long) (uintptr_t) __bp_ram_start);
        pide = 0; nombre = NULL;
    }

    bpvm_mem_cfg_t cfg;
    /* Todo lo que el plan deje —el array entero—, salvo `vmheap=N` (KB, el bloque
     * entero; V7/H7). Aquí lo que la VM no coge no es de nadie: el array es estático.
     * Sirve para probar cómo iría una placa más pequeña, no para dar RAM a malloc. */
    long vmheap_kb     = board_mgr_stm32_env_long("vmheap", 0);
    cfg.objetivo       = bpvm_mem_objetivo_env(vmheap_kb, 0);
    if (vmheap_kb > 0) log_printf("vm: vmheap=%ld KB (ENV): el objetivo del bloque", vmheap_kb);
    cfg.vm_min         = VM_MEM_MIN;
    cfg.reserva_bytes  = pide;
    cfg.reserva_nombre = nombre;
    bpvm_mem_plan_t plan;
    bpvm_mem_plan(reg, nreg, &cfg, &plan);
    char linea[160];
    bpvm_mem_plan_str(&plan, reg, &cfg, linea, sizeof linea);
    log_printf("%s", linea);
    s_vm_bytes = (plan.res == BPVM_MEM_OK) ? plan.bytes : 0;
    if (!s_vm_bytes) {
        /* #691 — sin VM: el motivo corto, el que sube al estado del arranque y al IDE. */
        bpvm_mem_sin_vm_motivo(&plan, reg, &cfg, vmheap_kb, s_vm_motivo, sizeof s_vm_motivo);
        log_printf("vm: AVISO la placa arranca SIN VM: %s", s_vm_motivo);
    }

    /* TOMAR: el bloque delante, la VM detrás. */
    size_t res = (plan.res == BPVM_MEM_OK) ? plan.reserva_bytes : 0;
    s_vm_base       = s_vm_mem + res;
    s_pack_ram_base = res ? s_vm_mem : 0;
    s_pack_ram_size = (uint32_t) res;
    if (res)
        log_printf("packs: bloque de RAM %u KB @0x%08lX (packRam=%ld KB, SQLite=%ld MB), delante de "
                   "la VM y en su sitio fijo — los estaticos de cada motor y la arena se reparten al cargar",
                   (unsigned) (res / 1024u), (unsigned long) (uintptr_t) s_pack_ram_base, pack_kb, sql_mb);
    else if (pide)
        log_printf("packs: SIN bloque de RAM — %s=%ld %s no cabe dejando a la VM su suelo (%u KB de %u)",
                   pack_kb > 0 ? "packRam" : "SQLite", pack_kb > 0 ? pack_kb : sql_mb,
                   pack_kb > 0 ? "KB" : "MB", (unsigned) (VM_MEM_MIN / 1024u),
                   (unsigned) (reg[0].bytes / 1024u));
    else if (sqlres != BPVM_SQLITE_OFF)
        log_printf("packs: SIN bloque de RAM — %s (%s=%ld)", bpvm_sqlite_res_str(sqlres),
                   pack_kb > 0 ? "packRam" : "SQLite", pack_kb > 0 ? pack_kb : sql_mb);
    else if (en_su_sitio)
        log_printf("packs: sin bloque de RAM (ni packRam= ni SQLite= en el ENV): un pack nativo no podria cargar");
}

/* ===== V7/H7 (Boot 2, fase 4) — LOS GANCHOS DE LA STM32 ==================================
 * El orden, la capa que pide cada paso y las reglas (con el FS, la zona y /lib; con la app
 * sana, el autorun —aquí corría SIEMPRE: no miraba si la placa había llegado sana—) son del
 * arranque común (src/bpvm_boot2.c). Aquí, sólo lo que es de la STM32. */

/* 2a — la BIOS de los packs nativos, VERIFICADA en cada arranque aunque no haya ninguno
 * grabado (un hueco que sólo se descubre el día que alguien graba un motor cuesta el doble),
 * y su ANCLA: la búsqueda tiene que dar EXACTAMENTE la que este firmware conoce; si da otra,
 * la marca está dos veces y un motor leería punteros de basura. Como la Pico (pico/main.c). */
static void stm32_bios(void) {
    {
        const bpvm_bios_t* bios = bios_stm32_get();   /* él ya loguea qué falta */
        if (bios) log_printf("bios: lista (%d ranuras, v%u)",
                             bpvm_bios_slot_count(), (unsigned) bios->version);
        const bpvm_ancla_t* real    = bios_stm32_ancla();
        uint32_t            barrido = (uint32_t) ((uintptr_t) __bp_flash_end - FLASH_BASE);
        const bpvm_ancla_t* hallada = bpvm_ancla_buscar((const void*) FLASH_BASE, barrido);
        if (hallada == real)
            log_printf("ancla: en 0x%08lX (la busqueda la encuentra, y es LA misma)",
                       (unsigned long) (uintptr_t) real);
        else if (hallada == 0)
            log_printf("ancla: NO SE ENCUENTRA barriendo %u KB desde 0x%08lX (esta en 0x%08lX) "
                       "-> ningun pack podra hallar la BIOS", (unsigned) (barrido / 1024u),
                       (unsigned long) FLASH_BASE, (unsigned long) (uintptr_t) real);
        else
            log_printf("ancla: DUPLICADA — la busqueda da 0x%08lX y la buena es 0x%08lX "
                       "-> un pack leeria punteros de basura",
                       (unsigned long) (uintptr_t) hallada, (unsigned long) (uintptr_t) real);
    }
}

/* 2b — subir: particiones del env → FS (sub-rango de la zona de datos) → VM. */
static const bpvm_boot_status_t* stm32_subir(void) {
    board_mgr_stm32_boot();
    return board_boot_status();
}

/* 2b — lo de la STM32 que pide el FS: la fachada de ficheros de BP (#247) y su línea. */
static void stm32_con_fs(void) {
    stm32_fs_register_bpvm();
    log_printf("fs: %u/%u bytes usados", (unsigned) fs_used_bytes(), (unsigned) fs_total_bytes());
}

/* 2b — el hardware: los backends (GPIO, la info del MCU) y, con la GUI en un pack, la
 * pantalla de la DK2 como 'PANT' para su LVGL (V7/H6 G5.b, como el P4). */
static void stm32_hw(void) {
    stm32_hw_register();
#if defined(BPVM_GUI_PACK)
    stm32_pant_publicar();
#endif
}

/* 2b — la SD: el firmware de la STM32 no tiene lector. Se dice, en vez del texto genérico
 * del registro común (que habla de una línea `sd` del ENV que aquí nadie lee). */
static void stm32_sd(void) {
    bpvm_sd_placa_sin_sd("el firmware de la STM32 no tiene lector de SD");
}

/* 2b — la cintura del REPL común (V6/U3 g4), antes del cable y del autorun. */
static void stm32_cintura(void) {
    bpvm_repl_set_ops(&s_repl_ops);
}

/* 2b — el cable: la UART del wire con su FIFO y, en las placas que lo piden, la RX por IRQ. */
static void stm32_cable(void) {
    /* FIFO RX/TX (8 bytes): absorbe el hueco de procesado entre la línea JSON
     * y los bytes bulk que la siguen → PUT fiable aunque la CPU vaya lenta.
     * (El fix definitivo del timing es subir el reloj a 160 MHz.) */
    HAL_UARTEx_SetTxFifoThreshold(BOARD_WIRE_UART, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(BOARD_WIRE_UART, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(BOARD_WIRE_UART);

#if defined(BOARD_WIRE_IRQn)
    /* RX a un anillo (V3/H5.2 DK2; H10 también Nucleo). La FIFO de 8B (≈700µs)
     * no basta cuando el lazo deja el UART sin sondear ms (bombeo de LVGL en
     * Gui.run()) y se perdían los primeros bytes del KILL. Con el anillo, getchar()
     * no pierde nada y el GUI puede dormir entre frames (__WFI). Desde #495 lo llena
     * el DMA (4 KB: la ventana del wire mientras se escribe la flash). Tras
     * EnableFifoMode para que RXFNE refleje la FIFO ya activa.
     * La placa opta definiendo BOARD_WIRE_IRQn en board.h. */
    stm32_wire_rx_arrancar();
#endif

    wire_v1_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");
    {   /* H10 — causa del último reset (diagnóstico; revela WDT/soft/power-on). */
        char rc[64];
        snprintf(rc, sizeof(rc), "reset cause: %s", stm32_reset_cause());
        wire_v1_send_cstr(rc);
    }
}

/* 2b — el REPL: el lazo del wire, con el latido del LED. No vuelve. */
static void stm32_lazo(void) {
    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') dispatch(c);
        else if (c < 0 && !bpvm_repl_paso()) {   /* #495 (paso 4): sin nada que leer, el trozo siguiente de lo que se esté mandando */
#if defined(BOARD_WIRE_IRQn)
            /* #473 (punto 7; 3-oct) — CEDER CUANDO NO HAY NADA QUE HACER. El lazo sondeaba el cable sin bloquearse
             * nunca, y a la prioridad 2 de esta tarea la ociosa de FreeRTOS no corría jamás con la placa parada (ni
             * su limpieza de tareas borradas, ni un WFI). Un tick (1 ms a 1 kHz): lo que llegue entretanto lo guarda
             * el anillo de la RX (IRQ y DMA, 4 KB); sin anillo, la FIFO de 8 B no aguantaría el milisegundo, y por eso
             * sólo aquí. Bloquearse de verdad hasta que llegue un byte no se puede: la interrupción del cable está por
             * encima del umbral de FreeRTOS (no puede avisar a una tarea). */
            bpvm_platform_thread_sleep_ms(1);
#endif
        }

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat */
            last_blink = now;
            BOARD_LED_BEAT_TOGGLE();
        }
    }
}

static const bpvm_boot2_familia_t s_boot2 = {
    .env          = board_mgr_stm32_env,
    .env_anadir   = board_mgr_stm32_env_anadir,
    .memoria      = stm32_mem_planificar,   /* V6/U6.11 — la línea `vm:` de las cinco placas */
    .bios         = stm32_bios,
    .subir        = stm32_subir,
    .mapear_packs = board_mgr_stm32_montar_packs,
    .stdlib       = stm32_mods_install,
    .con_fs       = stm32_con_fs,
    .hw           = stm32_hw,
    .sd           = stm32_sd,
    .identidad    = NULL,                   /* la de la placa la dice su cintura (board.h) */
    .cintura      = stm32_cintura,
    .cable        = stm32_cable,
    .autorun      = autorun_boot,
    .repl         = stm32_lazo,
    .nucleos      = 1,                      /* V7/H12: un núcleo, un run */
};

void stm32_repl_run(void) {
    /* H9 — arranque ESCALONADO: identidad → particiones del env → FS → VM, parando
     * en la 1ª capa que falla. Sin particiones/FS el climb se queda abajo y el host
     * conduce (Gestión de placa: proponer defaults → aplicar → reset). */
    log_init();                     /* recupera el log de la sesión anterior (post-mortem) */
    /* #439 — y DE DONDE sale lo recuperado: sin esto, un volcado con dos
     * arranques no distingue «la RAM sobrevivio al reset» de «se cargo de
     * flash lo que el arranque anterior volco». */
    log_printf(bpvm_log_origen_ram()
               ? "log: RAM SUPERVIVIENTE (lineas de ANTES del reset)"
               : "log: arranque en frio (cargado de flash)");
    /* #353 — lo que dice la VM (deps que faltan, packs, veredicto del guardián
     * de #339) al log. Aquí el problema era el opuesto al de la Pico: sin un
     * `_write` retargeteado, el stderr del núcleo se PERDÍA. */
    bpvm_diag_set_sink(diag_al_log);
    board_mgr_stm32_env_temprano(); /* V7/H6 G5.b0 — el ENV antes que el plan: packRam= / SQLite= */

    /* V7/H7 (Boot 2, fase 4) — AQUÍ ACABA EL BOOT 1 DE LA STM32 (el HAL, los relojes y el RTOS
     * los puso main(); el log y el ENV, aquí). Lo demás es el arranque común, con los ganchos
     * de arriba: 2a reparte (`stack=`, `quantum=`, la memoria y la BIOS) y 2b sube (las capas,
     * la zona, /lib, el hardware, los buses, la cintura del REPL, el cable, `log=`, el autorun
     * SÓLO con la placa sana y el REPL, que no vuelve). */
    bpvm_boot2_repartir(&s_boot2);
    bpvm_boot2_subir(&s_boot2);
}
