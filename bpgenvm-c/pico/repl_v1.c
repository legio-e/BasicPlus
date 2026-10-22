/*
 * repl_v1.c — dispatcher de mensajes JSON wire v1 en el firmware Pico.
 *
 * Fase A: HELLO/HELLO_REPLY.
 * Fase B: FILES (LIST/STAT/GET/PUT/DEL/MKDIR/RMDIR/RENAME/FORMAT/
 *         SAVE/DF/LOG_DUMP).
 * Fase C-E: pendiente (TERMINAL, META, DEBUG).
 *
 * Patrón de cada handler:
 *   1. Validar campos requeridos. Si faltan → wire_v1_send_error(id,
 *      "INVALID_PARAM", ...).
 *   2. Hacer el trabajo (FS, etc.). Si falla → error con código
 *      apropiado.
 *   3. Enviar reply (reply_empty si no hay datos, o construir con
 *      builders).
 *
 * El thread caller es la task del REPL — single-threaded. No hace
 * falta sincronizar el acceso a stdout entre handlers.
 */

#include "repl_v1.h"
#include "nucleos.h"        /* V7/H12, etapa 3.1: el arranque completo y `vm.cores=1` */
#include "wire_v1.h"
#include "bpvm_repl.h"      /* V6/U3: los verbos comunes del REPL */
#include "json_min.h"
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm_entry.h"   /* #344 — el RUN, escrito una vez */          /* H19-F1: base-dir por proyecto (bpvm_fs_basedir / bpvm_fs_set_basedir_from_module) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "log.h"
#include "bpvm_dbg_wire.h"   /* #326: el ramo de depuración salió de aquí a src/ */
#include "bpvm_io.h"         /* V6/A1.4: el segundo hilo, comun a todas las familias */
#include "mdn_loader.h"      /* H3 #158 fase D: cargar .mdn desde FS */
#include "bpvm_mdn_scan.h"   /* V5/H4: el escaneo del .mdn, compartido */

#include "bpvm.h"
#include "bpvm_internal.h"   /* la cintura del RUN: los módulos cargados, al log antes de correr */
#include "bpvm_machine.h"       /* INFO: uniqueId/boardName/temp/freq/uptime */
#include "board_desc.h"      /* INFO: variante/gpio/flash/psram del board_desc */
#include "board_mgr_pico.h"  /* H9: gestión de placa (entorno + particiones) */
#include "bpvm_rtc.h"        /* TIME: set epoch */
#include "aot_registry.h"    /* H3 #160: bpvm_aot_clear */
#include "pack_pico.h"       /* V5/H4: cargar los packs nativos en el primer Run */
#include "crc32.h"           /* V7/H9: el sello de la imagen */
#include "flash_layout.h"    /* V7/H9: __bp_zone3_start; V8/H5: __bp_imagen_ini */

#include "pico/bootrom.h"    /* reset_usb_boot (BOOTSEL) */
#include "pico/stdlib.h"     /* getchar_timeout_us (poll de KILL, #257) */

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Buffer VM compartido (declarado en main.c). */
#include "bpvm_sqlmem.h"   /* V5/H: motivo + minimo, para el aviso del INFO */
#include "bpvm_sd_placa.h" /* V7/H3 SD.3: la SD de la placa (verbos y vigía), en el común */

extern uint8_t* s_vm_buffer;          /* H7.2.b: SRAM interna o ventana PSRAM */
extern uint8_t* s_sqlite_base;        /* V5/H: bloque de la BD (NULL = no hay) */
extern uint32_t s_sqlite_size;
extern int      s_sqlite_res;         /* motivo (bpvm_sqlite_res_t)            */
extern long     s_sqlite_asked_mb;    /* lo que pedia el ENV                   */
extern uint32_t s_vm_buffer_size;
extern TaskHandle_t g_vm_task;        /* #354: para su marca de agua en el INFO */
/* V7/H12, 2b: la tarea del 2º run (NULL: va con uno). Aquí arriba porque su marca de agua sale en el INFO. */
static TaskHandle_t s_run2_task = NULL;

#ifndef BPVM_PICO_BUILD_DATE
#define BPVM_PICO_BUILD_DATE  __DATE__ " " __TIME__
#endif

/* Buffer reusable para la línea entrante (WIRE_V1_LINE_MAX = 2 KB). */
static char s_line_buf[WIRE_V1_LINE_MAX];

/* Buffer estático para construir replies pequeñas (no LIST/LOG_DUMP). */
static char s_reply_buf[1024];

/* Buffer estático para data PUT entrante. 16 KB es de sobra para los
 * .mod típicos (2-10 KB) y los driver bundles más generosos. PUTs
 * más grandes se rechazan con NO_SPACE en el dispatcher.
 *
 * Por qué no FS_DATA_SIZE (128 KB) entero: ya tenemos s_data (128 KB),
 * tmp del compact (128 KB) y VM_BUFFER (128 KB). Otro buffer del
 * mismo tamaño desbordaría los 512 KB SRAM del RP2350. Si alguna vez
 * hace falta subir ficheros >16 KB, opciones a explorar:
 *  - Heap dinámico (FreeRTOS heap actual es solo 32 KB; insuficiente).
 *  - Streaming chunk a chunk a fs_put (requiere extender la API FS
 *    para soportar PUT incremental).
 *  - Subir mediante múltiples PUTs con prefijo y un comando FINISH
 *    que concatene (workaround a nivel cliente, sin cambios FS). */
/* INTERINO (16-jul): subido 16K→48K porque Json.mod ya pesa 17.8K (Gui 31.8K)
 * y el bulk se rechazaba con NO_SPACE. Cabe en los 520K de SRAM del RP2350
 * (VM 128K + FS s_data 128K + este 48K). El fix DE VERDAD (Eduardo) es el
 * "Streaming chunk a chunk a fs_put" de arriba — trocear el envío para NO
 * buferizar el fichero entero, así el tamaño deja de estar acotado. Los .mod
 * seguirán creciendo → subir el buffer es un parche con techo. Ver tarea/mejora. */
/* #294/#334 — el buffer del bulk YA NO tiene que dar para el fichero entero:
 * desde que el IDE sube por trozos (PUT_BEGIN/DATA/END, verificado en placa en
 * las 3 familias el 28-jul) sólo necesita el MAYOR de sus dos papeles:
 *   (a) un trozo de streaming            = PUT_STREAM_CHUNK
 *   (b) el scratch del gestor de placa
 *
 * #338 (2-ago) — los dos papeles ADELGAZARON, y con ellos el buffer, de 20 a 8 KB:
 *   (a) el IDE manda trozos de 8 KB (BpvmClient.PUT_STREAM_CHUNK). Si un cliente
 *       viejo mandase más, el despachador responde NO_SPACE y drena: se queja, no
 *       se corrompe.
 *   (b) el gestor ya no pide TRES sectores aquí. Las dos copias del env se las
 *       presta la zona de rascar compartida (board_mgr_pico.c) y de este buffer
 *       sale sólo el sector de trabajo + la respuesta = BP_ENV_SECTOR + reply.
 * La respuesta se queda en 4 KB, que es con lo que el STM32 lleva funcionando
 * desde el primer día — no es una apuesta nueva.
 *
 * El sector del env es la página de borrado y NO es igual en todas (4 KB en
 * RP2350/ESP32, 8 KB en el U5), así que el número sigue siendo por familia. Por
 * eso la comprobación EN COMPILACIÓN de abajo: si alguien lo baja de más, no
 * compila, en vez de romperse en placa. */
#define V1_PUT_BUF_SIZE  (8 * 1024)
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
/* V7/H8 #495 — el búfer de FICHEROS del PUT troceado, aparte del de comunicaciones: un sector de la flash
 * (FLASH_SECTOR_SIZE, 4 KB), el bloque de littlefs en esta familia (fs_lfs_pico.c). */
static uint8_t s_fich_buf[4096];
/* C99 no tiene _Static_assert: el truco del array de tamaño negativo. */
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= 4096u + 512u) ? 1 : -1];

/* Reparto heap/stacks de la VM. DOS reglas superpuestas:
 *
 *  (a) Eduardo 19-jul — la región de stacks se TOPA en 512 KB y el resto es
 *      heap. Nació por la Metro: con 8 MB de PSRAM, un reparto proporcional
 *      dormía ~4 MB en stacks que usan decenas de KB.
 *  (b) Eduardo 28-jul — del bloque, 25% stacks y 75% heap (antes era mitad y
 *      mitad). Va junto con que la Pico ya no coge 128 KB fijos sino TODA la
 *      SRAM libre (main.c): con el bloque grande, el 25% siguen siendo MÁS
 *      stacks que los 64 KB de antes, así que no se pierde por ningún lado.
 *
 * Cómo queda:
 *   Pico  (343 KB): min(85K, 512K) = 85K  → heap 257 KB + stacks 85 KB.
 *   Metro (8 MB):   manda el tope, 512 KB → heap ~7,5 MB (IGUAL que antes:
 *                   el 25% serían 2 MB, así que el tope sigue mandando).
 *
 * ÚNICO cálculo, y esto es lo importante: lo usan el RUN (bpvm_init), el INFO
 * (mostrar el reparto) y el arranque de main.c. Si se copiara en cada sitio,
 * el día que cambie la regla unos dirían una cosa y otros otra — que es
 * exactamente cómo el INFO acabó enseñando 171+171. */
size_t vm_stack_region_bytes(void) {
    /* La REGLA ya no vive aquí: es bpvm_stack_region_bytes() del núcleo, que
     * comparten las 3 familias (ver bpvm.h). Esto es sólo el envoltorio que le
     * pasa el tamaño de ESTA placa. */
    return bpvm_stack_region_bytes((size_t) s_vm_buffer_size);
}

/* #440 — el testigo del MPU se arma en el gancho POST-ENLACE (ver bpvm.h y el
 * comentario del `mpu=1` más abajo): es el primer instante en que el código de
 * un módulo ya no debe cambiar. */
static void mpu_armar_tras_enlace(bpvm_t* vm) {
    extern void bpvm_pico_mpu_armar(const uint8_t*, const bpvm_module_t*, int);
    bpvm_pico_mpu_armar(vm->memory, vm->modules, vm->module_count);
}

/* #304 — accesor para que el REPL de texto (repl.c) comparta este buffer en vez
 * de duplicar 32 K (modo-texto y modo-wire son mutuamente excluyentes). */

/* ============================================================ */
/* DEPURADOR — el lazo de pausa, sus dos costuras y el JSON -> comando son del común (V7/H10, el RUN común: el
 * depurador de `src/bpvm_repl.c` sobre el núcleo de `src/bpvm_dbg_wire.c`). Aquí queda el búfer que le presta esta
 * placa (`.dbg_linea` de la cintura del RUN). */

/* V6/A1.7 - EL DEPURADOR TIENE SU PROPIO BUFFER, y no es un lujo.
 *
 * `s_line_buf` es del hilo `io`: ahi mete la linea que acaba de leer del cable.
 * Cuando A1.7 hizo que el `next_cmd` del depurador sacara su comando de la cola
 * de control, lo dejo escribiendo en ESE MISMO buffer — y ese lo lee la tarea
 * `vm`. O sea: dos tareas escribiendo el mismo array.
 *
 * En un nucleo la ventana es un cambio de tarea; en DOS es solapamiento de
 * verdad, y el sintoma seria un JSON del depurador pisado a media linea. Salio
 * al mirar, a peticion de Eduardo, que consecuencias tiene usar los dos nucleos
 * ANTES de activarlos: la respuesta es que esta era una, y estaba puesta hoy
 * mismo. Un buffer por ROL, que es lo que deberia haber hecho desde el principio. */
static char s_dbg_buf[WIRE_V1_LINE_MAX];

/* ============================================================ */
/* HELLO — META. */


/* ============================================================ */
/* V6/U3 g8 — LIST vive en el común. El recorrido de allí no necesita la zona de
 * scratch (ésta la pedía para TODO el listado) ni tiene tope de entradas por
 * directorio. A cambio emite dentro del callback de la fachada, o sea con el
 * lock del FS cogido; ver la nota del commit. */


/* ============================================================ */
/* STAT */


/* ============================================================ */
/* GET — reply con bulk. */


/* ============================================================ */
/* PUT — request lleva bulk. */


/* ============================================================ */





/* ============================================================ */
/* V7/H3 SD.3 (24-sep) — SD_INFO, SD_MOUNT y la tarjeta en caliente viven en el COMÚN
 * (`src/bpvm_sd_placa.c`): se trajeron de aquí con los mismos campos, y ahora hablan con el
 * dispositivo que registró el arranque (el SPI de la Metro) en vez de con el protocolo SPI. */

/* ============================================================ */
/* V6/U3.22 — LIST_DIR vive en el COMÚN (`src/bpvm_repl.c`), envoltorio incluido.
 *
 * El núcleo (`bpvm_listdir_emitir`) ya era común desde V5/H6 — salió justamente de
 * aquí. Lo que quedaba en esta placa eran 21 líneas, comparadas línea a línea con
 * las 17 del ESP32 antes de borrar: idénticas salvo el transporte. Y en el Pico
 * `wire_v1_send_bulk` ES el mismo `fwrite(stdout)+fflush` que hacía el sink local,
 * con el `tx_lock` que el atajo se saltaba: mismos bytes en el cable.
 *
 * La documentación de por qué LIST y LIST_DIR son verbos DISTINTOS se fue con el
 * código, que es donde sirve. */

/* ============================================================ */
/* DEL */


/* ============================================================ */
/* MKDIR / RMDIR — no-op en FS plano. */



/* ============================================================ */
/* RENAME — copia + delete. */


/* ============================================================ */
/* FORMAT — borra todo el FS RAM. Requiere confirm:"YES". */


/* ============================================================ */
/* SAVE — persiste el FS RAM a flash. */


/* ============================================================ */
/* DF — stats del FS. */


/* ============================================================ */
/* LOG_DUMP — text del log persistente, embebido como string JSON. */

typedef struct {
    int first;        /* 1 = todavía no se ha escrito el header */
} log_ctx_t;



/* ============================================================ */
/* META — INFO, RESET, BOOTSEL. (TIME y PING viven en el común.) */

/* ── V6/U3 g10 — INFO: los 18 comunes por aqui, los 11 propios por info_extra ──
 * El reparto no es arbitrario: en `info` va lo que TODA placa tiene (serie,
 * reloj, memoria, FS) y en `info_extra` lo que solo significa algo aqui — la
 * zona de packs con su direccion XIP, el bloque de SQLite y las marcas de agua
 * de FreeRTOS. Meter esto ultimo en el struct comun obligaria a las otras
 * familias a declarar campos que no saben rellenar. */
static void packs_cargar_una_vez(void);   /* abajo; V7/H7: también desde el INFO */

static void pico_repl_info(bpvm_repl_info_t* out) {
    /* V7/H7 (Boot 2, fase 2) — el INFO también dispara la carga de los motores, como en la
     * ESP32 y la STM32: la tanda y el IDE preguntan por el INFO ANTES del primer Run (lo que
     * publica un motor —la pantalla de un pack— tiene que estar). La guarda, en el común. */
    packs_cargar_una_vez();
    /* Estaticos porque el struct guarda punteros y el comun los lee DESPUES. */
    static char unique[20], board[16];
    bpvm_machine_unique_id(unique, sizeof unique);
    /* V6 (9-sep) — el `board_name` del wire es el MICRO (el IDE lo rotula
     * «Micro»), asi que aqui va `micro_name` y no `board_name`. La PLACA —que en
     * el RP2350 SI se conoce, porque la Pico 2 y la Metro llevan la misma imagen
     * y la variante la da el ENV— sale por `Machine.getBoard()`. Antes esta linea
     * ponia la placa donde las otras dos familias ponen el micro. */
    bpvm_machine_micro_name(board, sizeof board);
    const board_desc_t* bd = board_desc();
    size_t vstack = vm_stack_region_bytes();

    out->unique_id    = unique;
    out->board_name   = board;
    /* H11 — la ARQUITECTURA del nativo que esta imagen ejecuta. Sin esto el IDE
     * no sabe a que ISA compilar el .mdn de un `.bp` suelto. Lo dice la placa. */
    out->arch         = (unsigned) bpvm_mdn_host_arch();
    out->cpu_hz       = (unsigned long) bpvm_machine_cpu_freq_hz();
    out->uptime_ms    = (unsigned long) bpvm_machine_uptime_ms();
    out->reset_reason = bpvm_machine_reset_cause();
    /* El wire v1 no lleva floats (el parser del cliente rechaza decimales), asi
     * que la temperatura viaja en milesimas: 20400 -> 20,4 grados en la UI. */
    out->temp_milli_c = (long) (bpvm_machine_temp_c() * 1000.0f);
    out->gpio_count   = bd->gpio_count;
    out->pio_count    = bd->pio_count;
    /* PWM en SALIDAS, no slices: cada slice tiene 2 canales (A/B) -> 24 en
     * RP2350, que es la cifra que anuncian las placas. El campo del wire
     * conserva el nombre historico "pwmSlices". */
    out->pwm_slices   = bd->pwm_slices * 2;
    out->adc_channels = bd->adc_channels;
    /* V7/H8 #602 — el bloque de RAM de los motores, al INFO común (era `sqlite*`, en el info_extra). */
    out->pack_ram_base   = (unsigned long) (uintptr_t) s_pack_ram_base;
    out->pack_ram_bytes  = (unsigned long) s_pack_ram_size;
    out->pack_ram_res    = s_pack_ram_res;
    out->pack_ram_clave  = s_pack_ram_clave;
    out->pack_ram_pedido = s_pack_ram_pedido;
    out->flash_bytes  = (unsigned long) bd->flash_bytes;
    out->sram_bytes   = 520UL * 1024UL;
    out->psram_bytes  = (unsigned long) bd->psram_bytes;
    /* El MISMO reparto que usa el RUN (vm_stack_region_bytes), visible sin
     * ejecutar nada. */
    out->vm_heap_bytes  = (unsigned long) (s_vm_buffer_size - vstack);
    out->vm_stack_bytes = (unsigned long) vstack;
    out->fs_total_bytes = (unsigned long) fs_total_bytes();
    out->fs_used_bytes  = (unsigned long) fs_used_bytes();
}

static int pico_repl_info_extra(char* buf, unsigned long buf_max, int off) {
    size_t max = (size_t) buf_max;
#define X_S(k, v) do { off = wire_v1_field_string(buf, max, (size_t) off, (k), (v)); \
                       if (off < 0) return off; } while (0)
#define X_L(k, v) do { off = wire_v1_field_long(buf, max, (size_t) off, (k), (long) (v)); \
                       if (off < 0) return off; } while (0)
    const board_desc_t* bd = board_desc();
    char variant[2]; variant[0] = bd->variant; variant[1] = '\0';
    X_S("variant", variant);
    /* V8/H5 (22-oct) — qué firmware de la imagen corre: P, el código en la flash, o M, la VM caliente en la SRAM (con
     * PSRAM). Van los dos en el mismo .uf2, y el imageCrc es el del que corre. */
#if defined(BPVM_IMAGEN_M)
    X_S("imagen", "M");
#else
    X_S("imagen", "P");
#endif

    /* V5/H — LO QUE EL IDE NECESITA PARA PRE-ENLAZAR UN PACK NATIVO.
     * El pack se enlaza en base 0 y el IDE lo realoja en el PC antes de grabar,
     * asi que tiene que saber DONDE va a caer. Y ninguno de estos se deduce
     * desde fuera:
     *  · packsXipBase — ⚠️ la direccion que ve la CPU (XIP_BASE + offset de la
     *    particion), NO el offset crudo en flash. Solo la placa conoce ese
     *    mapeo, y equivocarse ahi desplaza TODO el codigo por una constante, en
     *    silencio: es el error mas caro posible en este camino.
     *  · packsBytes  — para negarse ANTES de compilar 400 KB que no caben.
     *  · (el bloque de RAM de los motores —era sqliteBase/Bytes— sale desde
     *    V7/H8 #602 del INFO COMÚN, como packRam*, en las cuatro familias.)
     *  · floatAbi    — junto a `arch`, el SELLO. `arch` sola no distingue hard
     *    de softfp, y esa discrepancia da numeros mal sin avisar. */
    /* V7/H8 (28-sep) — `packsXipBase`/`packsBytes` y `floatAbi` salen ya del INFO COMÚN, para las cuatro familias
     * (bpvm_repl.c: la zona montada, en su vista ejecutable). Aquí queda lo que sólo sabe la Pico. */
    /* V7/H8 #602 — el bloque de RAM de los motores y su AVISO (el motivo, lo pedido y el mínimo: decisión de
     * Eduardo, el aviso vive en el INFO) salen ya del INFO COMÚN, como packRam*, en las cuatro familias. Eran
     * `sqliteBase`/`Bytes`/`Status`/`AskedMb`/`MinMb`, y sólo contaban la reserva en PSRAM. */

    /* #354 — LO QUE NUNCA LE HEMOS PREGUNTADO A FreeRTOS: cuanto de lo que se le
     * reservo llego a usar. SOLO DIAGNOSTICO. Dos cifras que contestan a dos
     * preguntas distintas y conviene no confundir:
     *  · rtosHeapMinFreeBytes = lo que quedo libre de `ucHeap` en el PEOR
     *    momento. OJO: ese heap no tiene estructuras del kernel, tiene PILAS DE
     *    TAREAS — 16 KB solo vm_task y 4 KB por thread BP. Lo que sobra ahi es
     *    el TECHO DE THREADS, no memoria muerta: recortarlo baja una capacidad.
     *  · vmTaskStackFreeBytes = lo que le sobro a la pila de vm_task. Aqui si,
     *    si sobra mucho, sobra de verdad.
     * uxTaskGetStackHighWaterMark devuelve PALABRAS en FreeRTOS de serie (en
     * ESP-IDF son BYTES — la misma trampa que xTaskCreate), de ahi el producto.
     * Y una marca tomada sin haber ejecutado nada no dice nada: se mira DESPUES
     * de correr algo con carga. */
    X_L("rtosHeapMinFreeBytes", xPortGetMinimumEverFreeHeapSize());
    if (g_vm_task != NULL) {
        UBaseType_t words = uxTaskGetStackHighWaterMark(g_vm_task);
        X_L("vmTaskStackFreeBytes", (size_t) words * sizeof(StackType_t));
    }
    /* V7/H12, 2b — la del 2º run (`runStackFree`, la común, es la del run que cerró el último programa). */
    if (s_run2_task != NULL) {
        UBaseType_t words = uxTaskGetStackHighWaterMark(s_run2_task);
        X_L("run2StackFreeBytes", (size_t) words * sizeof(StackType_t));
    }
#undef X_S
#undef X_L
    return off;
}



static void handle_reset(long id, const json_obj_t* obj) {
    (void) obj;
    /* El cliente espera la reply ANTES del reset. Mandamos primero,
     * después delay corto para que el USB CDC vacíe sus buffers, y
     * por fin watchdog_reboot. El protocolo §6 lo documenta así. */
    log_printf("RESET (wire v1): rebooting");
    log_flush();
    wire_v1_send_reply_empty("RESET_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));
    extern void watchdog_reboot(uint32_t, uint32_t, uint32_t);
    watchdog_reboot(0, 0, 0);
    /* no retorna */
}

static void handle_bootsel(long id, const json_obj_t* obj) {
    (void) obj;
    log_printf("BOOTSEL (wire v1): entering bootloader");
    log_flush();
    wire_v1_send_reply_empty("BOOTSEL_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));
    reset_usb_boot(0, 0);
    /* no retorna */
}

/* ============================================================ */
/* TERMINAL — el RUN es el común (V7/H10: `bpvm_repl_run`, en src/bpvm_repl.c), con la cintura de abajo. El de esta
 * placa —su sink, su EXITED, su poll y su latido— se borró en el paso 6 (3-oct), probado ya en placa. */

/* V5/H4 — LOS PACKS SE CARGAN AQUÍ, EN EL PRIMER Run.
 *
 * Un pack nativo publica su API al arrancar (`bp_pack_init` → `publica`), y un
 * programa BP que use esa API la busca por su marca. O sea que alguien tiene
 * que haber cargado el pack ANTES de que corra el programa. Ese alguien es
 * esto.
 *
 * ─── POR QUÉ EN EL Run Y NO EN EL ARRANQUE ───
 *
 * Decisión de Eduardo, de la tanda del 7-ago (commit 262bb01): el salto al
 * pack es el único paso que puede colgar. Un cuelgue durante un Run se arregla
 * desenchufando UNA vez; uno en el arranque se repite en CADA arranque y
 * obliga a regrabar. Con la Metro, que no tiene botón de reset, eso no es una
 * molestia: es perder la placa hasta reflashearla.
 *
 * ─── UNA VEZ, Y SÓLO SI SALIÓ BIEN ───
 *
 * El pack se carga y se queda; volver a saltar a su entrada intentaría publicar
 * la misma marca dos veces y el punto de encuentro lo rechazaría (-3). Por eso
 * la bandera se pone SÓLO cuando salió bien: si no hay pack grabado, el
 * siguiente Run lo vuelve a intentar — que es justo lo que quieres después de
 * grabarlo, sin reiniciar.
 *
 * Y no se trata como error: un programa que no use packs no tiene por qué
 * llevar ninguno. Quien los necesite se enterará por su nombre cuando el puente
 * no encuentre la marca. */
static void packs_cargar_una_vez(void) {
    /* V7/H7 (Boot 2, fase 2): la guarda y el log, en el cargador común. */
    (void) pack_pico_cargar();
}

/* ── V5/H4: la CINTURA del escaneo de .mdn en esta familia ─────────────────
 *
 * El bucle, el orden de búsqueda y los mensajes están en `bpvm_mdn_escanear`
 * (src/bpvm_mdn_scan.c), compartidos. Aquí queda lo único que es del RP2350:
 * de dónde sale la RAM ejecutable cuando el `.mdn` viene del FS.
 *
 * Del pack no pasa por aquí — se ejecuta EN SITIO desde la flash y no gasta
 * arena. Ésa es, de hecho, una de las ganancias de meterlo en el pack. */
static const uint8_t* pico_mdn_del_fs(void* user, const char* nombre,
                                      uint32_t* len) {
    bpvm_t* vm = (bpvm_t*) user;
    char     real[BPVM_FS_PATH_MAX];   /* #456 - lo CONSTRUYE el resolvedor */
    uint32_t size = 0;
    if (bpvm_entry_resolve(nombre, real, sizeof real, &size) != 0) return NULL;   /* #344: la regla común */

    /* H11 — ZERO-COPY: los thunks se registran como punteros DENTRO de este
     * buffer, así que tiene que seguir vivo toda la ejecución. Se reserva de la
     * arena exactamente lo que ocupa, 4-alineado (lo que pide Thumb-2). */
    uint8_t* dst = bpvm_arena_reserve(vm, size, 4);
    if (!dst) {
        /* Se dice AQUÍ y no arriba: para el escaneo esto es "no está", y un
         * descarte por tamaño en silencio no se distinguiría de no tenerlo. */
        log_printf("AOT: %s (%u B) no cabe en la arena — sin overlay",
                   real, (unsigned) size);
        return NULL;
    }
    if (bpvm_fs_read(real, dst, size) != (long) size) {
        log_printf("AOT: %s no se pudo leer — sin overlay", real);
        return NULL;
    }
    *len = size;
    return dst;
}

/* V6/A1.4 — la cola de salida de `io`, 1 KB: en la Pico la salida es USB CDC y ahí el cuello es el transporte, no el
 * amortiguador (medido: 2 000 líneas costaban 7,7 s con un hilo, casi todo esperando al USB). */
#define PICO_IO_OQ_BYTES 1024

/* ── V7/H10, el RUN común, paso 4 (3-oct): LA CINTURA DE LA PICO ──────────────────────────────────────────────────
 * Lo que es suyo de verdad: el bloque de la VM, el USB leído sin esperar, sus búferes (uno por rol, A1.7), el
 * reinicio, los motores nativos, el `.mdn` del FS a la arena, la lista de módulos y el testigo del MPU antes de correr,
 * su SMP (si el build lo pide) y, después, desarmar el MPU y la línea `mem:`. El latido y `gc=0`, que eran suyos, son
 * ahora del común (en las cuatro, por el ENV). */
static int pico_run_memoria(uint8_t** base, size_t* bytes) {
    if (!s_vm_buffer || s_vm_buffer_size == 0) return -1;
    *base = (uint8_t*) s_vm_buffer;
    *bytes = (size_t) s_vm_buffer_size;
    return 0;
}
static int pico_run_leer_linea(char* buf, size_t cap) {
    int c = getchar_timeout_us(0);
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, buf, cap);
    return (n < 0) ? 0 : n;                     /* rota: se descarta */
}
static void pico_run_reiniciar(long id) { handle_reset(id, NULL); }
static void pico_run_antes(bpvm_t* vm) {
    /* V5/H4 — qué módulos hay y de dónde salieron, antes de arrancar */
    log_printf("modulos cargados: %d", vm->module_count);
    for (int mi = 0; mi < vm->module_count; mi++)
        log_printf("  [%d] %-12s %s cb=0x%08lX main=%ld", mi, vm->modules[mi].name,
                   vm->modules[mi].en_pack ? "PACK" : "FS  ", (unsigned long) vm->modules[mi].cb,
                   (long) vm->modules[mi].main_offset);
    /* #440 — EL TESTIGO DEL MPU, detrás del ENV `mpu=1` (por defecto NO): el código de cada módulo, de sólo lectura,
     * para que la escritura que lo pisa levante un DACCVIOL con su PC (lo cuenta el manejador de #447). Se arma por
     * el GANCHO POST-ENLACE y no aquí: el enlace de `bpvm_run` ESCRIBE aunque escriba el mismo valor, y al MPU el
     * valor le da igual —el 29-ago tumbó `StackTrace`, un programa sano, sobre su propio fixup de `eh_class`—. Andamio
     * de diagnóstico, no producción: una región mal calculada tumbaría una placa buena. */
    if (board_env_bool("mpu", 0)) bpvm_set_after_link_hook(mpu_armar_tras_enlace);
    log_printf("AOT: scan done, about to bpvm_run");
    log_flush();   /* CHECKPOINT: lo que cuelgue después es la ejecución */
    /* V7/H12, etapa 3.1, paso 4 — SÓLO PRUEBAS: `prueba.fallo=1` en el ENV provoca aquí un HardFault (una instrucción
     * indefinida: UNDEFINSTR), para probar en la placa el camino fatal —con dos núcleos: el perro guardián, el informe en
     * la RAM que sobrevive, la causa del reinicio y el arranque siguiente sin autorun—. Con la clave puesta, CADA RUN
     * falla: se quita después. (Con un núcleo, la placa se queda parada como hoy.) */
    if (board_env_bool("prueba.fallo", 0)) {
        log_printf("prueba.fallo=1: un HardFault a proposito (SOLO PRUEBAS)");
        __asm volatile ("udf #0");
    }
}
/* (V7/H12, etapa 2a.2 — fuera `pico_run_correr`, el camino SMP de la Pico: BPVM_PICO_SMP_WORKERS, que su build no
 * definía. El programa lo corre el run permanente de la cola; en la 2b, el segundo run.) */
static void pico_run_despues(bpvm_t* vm) {
    (void) vm;
    { extern void bpvm_pico_mpu_desarmar(void); bpvm_pico_mpu_desarmar(); }   /* #440: SIEMPRE */
    { extern void bpvm_pico_mem_log(void); bpvm_pico_mem_log(); }             /* V7/H7: la línea `mem:` */
}

static const bpvm_repl_run_ops_t s_run_ops = {
    .memoria           = pico_run_memoria,
    .leer_linea        = pico_run_leer_linea,
    .linea             = s_line_buf,  .linea_cap     = sizeof s_line_buf,
    .dbg_linea         = s_dbg_buf,   .dbg_linea_cap = sizeof s_dbg_buf,
    .mensaje           = s_reply_buf, .mensaje_cap   = sizeof s_reply_buf,
    .reiniciar         = pico_run_reiniciar,
    .cargar_motores    = packs_cargar_una_vez,
    .mdn_del_fs        = pico_mdn_del_fs,
    .antes_de_correr   = pico_run_antes,
    .despues_de_correr = pico_run_despues,
    .env_bool          = board_env_bool,
    .io_cola           = PICO_IO_OQ_BYTES,
};
/* ============================================================ */
/* #345 paso 2 — cintura del gate. Cuatro funciones y ni un verbo nuevo: el
 * HELLO que el IDE ya manda al conectar dice "hay alguien", y el KILL que ya
 * manda el Stop dice "no arranques". La política está en el núcleo. */
static void pico_autorun_anuncia(const char* path, int ventana_ms, void* user) {
    (void) user;
    log_printf("autorun: %s arranca en %d ms — Stop en el IDE para cancelar",
               path, ventana_ms);
}

static void pico_autorun_espera(int ms, void* user) {
    (void) user; vTaskDelay(pdMS_TO_TICKS(ms));
}

static uint32_t pico_autorun_ahora(void* user) {
    (void) user; return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    pico_autorun_anuncia, bpvm_repl_autorun_escucha,   /* V7/H10, el RUN común, paso 6: la escucha, la común */
    pico_autorun_espera,  pico_autorun_ahora, NULL
};

/* P-autorun (#256) — arranque autónomo desde /sys/auto.txt.     */
/*                                                               */
/* main.c lo invoca tras FS + stdlib + board, justo ANTES del    */
/* repl_run() — el wire ya está operativo y el poll del run      */
/* atiende HELLO/KILL, así que la placa nunca queda sorda: el    */
/* IDE puede conectarse con el autorun corriendo y pararlo.      */
/* Formato del fichero: primera línea = ruta del módulo (p.ej.   */
/* "/app/MiApp.mod"); espacios y CR/LF tolerados; vacío o ruta   */
/* inexistente → log + REPL normal (nunca un boot-loop).         */
/* Vías de escape: Stop en el IDE → borrar /sys/auto.txt         */
/* (comando `autorun off` de la consola) → reset.                */
/* ============================================================ */
void repl_v1_autorun(void) {
    pico_arranque_completo();   /* V7/H12, etapa 3.1: el arranque, completo (con su primer volcado del log): el perro
                                 * del arranque, apagado ANTES del programa —que puede durar lo que quiera— */
    /* #345 — leer y limpiar la primera línea lo hace el núcleo: eran las mismas
     * doce líneas en Pico, ESP32 y STM32. (#305 sigue valiendo: sólo la CABEZA
     * del fichero, que aquí se busca un renglón, no un fichero.) */
    char path[BPVM_FS_PATH_MAX];   /* #456 - este se CONSTRUYE (sale del fichero) */
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */
    if (pico_arranque_tras_fatal()) {   /* V7/H12 3.1, paso 4: un programa que falla al arrancar, en bucle, no */
        log_printf("autorun: %s NO arranca: el arranque anterior acabo en un fallo fatal (ver el log)", path);
        return;
    }
    log_printf("autorun: %s", path);

    /* Gracia de arranque: el camino del RUN hace log_flush (erase de
     * flash con IRQs off) y la app puede tocar flash también — si eso
     * coincide con la ENUMERACIÓN USB del host, Windows da el puerto
     * por muerto ("dispositivo desconocido"). 2 s dejan a TinyUSB
     * terminar la enumeración antes de arrancar. Boots sin auto.txt
     * no pagan nada (return arriba). */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* #345 paso 2 — la ventana de rescate. Decide el usuario: si el IDE está
     * conectado (su HELLO llega) se le dan 10 s para darle a Stop. Sin nadie
     * escuchando no se espera nada. Los dos verbos son los de siempre.
     *
     * DESPUÉS de la gracia de USB, y no antes: el gate pregunta "¿hay alguien?"
     * mirando el CDC, y antes de que TinyUSB enumere ahí no puede haber llegado
     * nada aunque el IDE esté abierto. Preguntando antes, la respuesta sería
     * siempre "no hay nadie" y la ventana no se abriría nunca — un mecanismo de
     * rescate que sólo funciona cuando no hace falta. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        return;
    }
    bpvm_repl_run(-1, path, NULL);   /* V7/H10: el RUN común; sin argumento, manda el defecto */
    log_printf("autorun: terminado — REPL normal");
}

/* ============================================================ */
/* Dispatcher principal. */


/* ── V6/U3 g9 — la cintura del REPL de esta familia ────────────────────────
 * Se registra A MEDIAS a propósito: `info` y `put_buf` llegan en sus pasos. El
 * común no se fía del orden de la cadena para eso — cada verbo comprueba la
 * pieza que necesita y contesta UNSUPPORTED si falta. */
static int  pico_repl_fs_format(void) { return fs_format_ram(); }   /* #473 (466): lo que pasó */
static int  pico_repl_fs_save(void)   { return fs_save_to_flash() == FS_OK ? 0 : -1; }
static unsigned long pico_repl_fs_total(void) { return (unsigned long) fs_total_bytes(); }
static unsigned long pico_repl_fs_used(void)  { return (unsigned long) fs_used_bytes(); }
static int  pico_repl_fs_count(void)  { return (int) fs_file_count(); }

/* V7/H9 (2-oct) — el sello de la imagen: el CRC32 de la ZONA 3 de la flash (FLASH_MAIN: el código, las constantes y los
 * valores iniciales de los datos), de __bp_zone3_start a __flash_binary_end. La zona 1 (el arranque) cambia con ella, y
 * la 2 es el ENV, que cambia sola y no es la imagen. Una vez, la primera que se pide.
 * V8/H5 (22-oct) — desde __bp_imagen_ini, el FLASH_MAIN del firmware que corre: en P es la zona 3 de siempre (el mismo
 * CRC que antes); en M, su código, en 0x10110000. */
extern char __flash_binary_end[];
static uint32_t pico_image_crc(void) {
    static uint32_t s_crc = 0;
    static int s_hecho = 0;
    if (!s_hecho) {
        const uint8_t* a = (const uint8_t*) __bp_imagen_ini;
        s_crc = bpvm_crc32(a, (size_t) ((const uint8_t*) __flash_binary_end - a));
        s_hecho = 1;
    }
    return s_crc;
}

static const bpvm_repl_ops_t s_repl_ops = {
    .run            = &s_run_ops,     /* V7/H10: la cintura del RUN común */
    .info           = pico_repl_info,
    .image_crc      = pico_image_crc,
    .info_extra     = pico_repl_info_extra,
    .server_name    = "bpvm-pico",
    .server_build   = BPVM_PICO_BUILD_DATE,
    /* BOOTSEL va como capacidad aparte: es de esta familia y el IDE decide con
     * ella si enseñar el botón. Y PACKS (30-sep): los PACK_* del común los tenía
     * y no los decía. */
    .capabilities   = "[\"META\",\"FILES\",\"TERMINAL\",\"DEBUG\",\"PACKS\",\"BOOTSEL\"]",
    .put_buf        = s_put_buf,
    .put_buf_size   = sizeof s_put_buf,
    /* Sin after_put a propósito: littlefs committea en cada close, así que aquí
     * NO hay nada que persistir tras una subida (a diferencia del STM32). */
    .after_put      = NULL,
    .fich_buf       = s_fich_buf,     /* V7/H8 #495: un sector de la flash */
    .fich_bytes     = sizeof s_fich_buf,
    .put_trozo      = 512,            /* V7/H8 #495: 4 trozos de 512 B = 2 KB, la FIFO del CDC (Eduardo, 1-oct) */
    .put_ventana    = 4,
    .fs_total_bytes = pico_repl_fs_total,
    .fs_used_bytes  = pico_repl_fs_used,
    .fs_file_count  = pico_repl_fs_count,
    .fs_format      = pico_repl_fs_format,
    .fs_save        = pico_repl_fs_save,
};

void repl_v1_handle_request(int first_char) {
    /* 1. Leer la línea JSON completa. */
    int n = wire_v1_recv_line(first_char, s_line_buf, sizeof(s_line_buf));
    /* V6/#473 — -2 no es -1: la linea se estanco (truncado/ruido). Descartar en
     * SILENCIO y seguir; el IDE reintenta. Contestar «demasiado larga» aqui
     * mandaria a mirar donde no esta el problema. */
    if (n == -2) return;
    if (n < 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX");
        return;
    }

    /* 2. Parsear el JSON. */
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "JSON inválido");
        return;
    }

    /* 3. id (puede ser 0 si el peer no lo mandó). */
    long id = json_get_long(&obj, "id", 0);

    /* 4. type. Se saca ANTES de tocar el bulk, y a propósito (V6/U3 g11): hay
     *    que saber QUÉ verbo es para decidir quién lee esos bytes. Leer el tipo
     *    del JSON ya parseado no toca el cable, así que adelantarlo es gratis. */
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta 'type'");
        return;
    }
    bpvm_repl_antes_de_despachar(type);   /* #495: un verbo que no convive con una subida a medias, la acaba */

    /* 5. El bulk — pero SÓLO para quien no se lo lee él mismo.
     *
     * Los verbos PUT viven en el común desde V6/U3 y ahí el bulk se lee DENTRO
     * del handler. Si además se pre-leyera aquí, se leería DOS VECES: la segunda
     * lectura se comería el mensaje siguiente y el wire quedaría desincronizado.
     * No es hipotético — es exactamente el bug que tuvo el ESP32 en 0456da8, y
     * el síntoma no es "falla el PUT" sino que a partir de ahí no funciona nada.
     *
     * Lo que SÍ sigue pre-leyéndose es PACK_BURN_DATA y compañía: el gestor de
     * placa recibe el bulk ya en s_put_buf, y su camino no ha migrado. */
    int lo_lee_el_comun = (strcmp(type, "PUT") == 0 || strncmp(type, "PUT_", 4) == 0);
    long bulk = lo_lee_el_comun ? 0 : json_get_long(&obj, "bulk", 0);
    size_t bulk_size = 0;
    if (bulk > 0) {
        if (bulk > (long) sizeof(s_put_buf)) {
            /* Drain en chunks pequeños y responder NO_SPACE. */
            static uint8_t drain[64];
            long remaining = bulk;
            while (remaining > 0) {
                size_t chunk = (size_t)(remaining > (long) sizeof(drain)
                                         ? (long) sizeof(drain) : remaining);
                if (wire_v1_recv_bulk(drain, chunk, sizeof(drain)) < 0) break;
                remaining -= (long) chunk;
            }
            wire_v1_send_error(id, "NO_SPACE",
                                "bulk supera el buffer del servidor");
            return;
        }
        if (wire_v1_recv_bulk(s_put_buf, (size_t) bulk, sizeof(s_put_buf)) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "lectura de bulk truncada");
            return;
        }
        bulk_size = (size_t) bulk;
    }

    /* 6. Despachar. */
    /* META */
    if (strcmp(type, "RESET")    == 0) { handle_reset(id, &obj);    return; }
    if (strcmp(type, "BOOTSEL")  == 0) { handle_bootsel(id, &obj);  return; }
    /* H9 — gating por estado REAL del boot: sin particiones/FS (estado < 2)
     * los comandos de fichero se RECHAZAN con error claro (nunca cuelgan ni
     * inventan un FS); RUN además necesita la VM (estado 3). Norma de Eduardo:
     * "si no hay partición, nada con el sistema de ficheros". */
    /* V7/H10 #473 (punto 10): la puerta, en el común. El bulk que rechaza sólo sigue en el cable en los verbos PUT:
     * el resto ya lo pre-leyó el paso 5. */
    if (bpvm_repl_puerta_arranque(type, id, &obj, board_boot_status(), lo_lee_el_comun)) return;   /* #691: con su motivo */
    /* FILES */
    /* V6/U3 — el común, DESPUÉS de los propios y no antes (al revés que el
     * STM32) porque esta familia se migra por grupos: lo que esta placa siga
     * implementando gana, y lo que ya no, cae aquí. Al terminar la migración
     * este if sube al principio y la cadena de arriba desaparece.
     *
     * El grupo PUT ya migró (U3.12), y con él la pre-lectura del bulk dejó de
     * ocurrir para esos verbos — la lleva `lo_lee_el_comun` en el paso 5. */
    if (bpvm_repl_dispatch(type, id, &obj)) return;

    /* BOARD (H9) — gestión de placa: entorno + particiones (núcleo compartido con
     * el boardsim vía bpvm_bmgr_wire). */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        /* #327 — PACK_* también. La capacidad ya estaba en el núcleo compartido
         * (bpvm_bmgr_wire atiende PACK_LS y compañía) y este firmware YA lo
         * enlazaba: sólo faltaba encaminarle los comandos, así que el panel de
         * packs del IDE se comía un UNSUPPORTED. El STM32 era el único que lo
         * hacía; ahora las 3. */
        || strncmp(type, "PACK_", 5) == 0) {
        /* El bulk de PACK_BURN_DATA YA lo ha leído el despachador, arriba, en
         * s_put_buf — NO se relee (hacerlo se comería el mensaje siguiente y
         * desincronizaría el wire; ese fue el bug del ESP32 en 0456da8).
         * s_put_buf hace de las dos cosas: el bulk ocupa el principio y el
         * gestor recibe el tramo de DETRÁS, para que no puedan pisarse. */
        size_t used = bulk_size > sizeof s_put_buf ? sizeof s_put_buf : bulk_size;
        board_mgr_pico_handle(id, &obj, type,
                              s_put_buf + used, sizeof s_put_buf - used,
                              bulk_size ? s_put_buf : NULL,
                              (unsigned long) bulk_size);
        return;
    }
    /* TERMINAL y DEBUG — el RUN, el depurador de antes del RUN (PAUSE, SET_BP, CLR_BP) y el KILL sin programa los
     * atiende el común (bpvm_repl_dispatch; V7/H10, el RUN común, paso 6): durante un RUN, todo lo del cable lo lee
     * el poll común. */
    /* PROMPT_RESPONSE: el builtin IO.prompt() aún no está implementado
     * en la VM C, así que nunca emitimos PROMPT_REQUEST. Si llega un
     * RESPONSE huérfano, ack silente. */
    if (strcmp(type, "PROMPT_RESPONSE") == 0) {
        wire_v1_send_reply_empty("PROMPT_RESPONSE_REPLY", id);
        return;
    }

    /* Fase D-E: META resto (INFO/TIME/PING/RESET/BOOTSEL), DEBUG. */
    wire_v1_send_error(id, "UNSUPPORTED",
                        "type no implementado en este firmware");
}

/* --- Bucle de transporte (#305) ------------------------------------
 *
 * Antes vivía en repl.c, mezclado con un REPL de TEXTO (HELLO/LS/PUT/GET/RUN/
 * HELP...) que era el protocolo original del Pico, de cuando aún no existía el
 * wire. Al llegar el wire v1 no se retiró: el bucle hacía peek del primer
 * carácter y elegía protocolo — '{' al wire, cualquier otra cosa a modo texto.
 *
 * Esa convivencia costaba de verdad. El modo texto tenía su PROPIA resolución
 * de módulos (cmd_run duplicaba lo que hace el RUN del wire), su propio
 * GET, su propio PUT... es decir, un segundo camino para las mismas cosas que
 * nadie ejercitaba y que iba divergiendo en silencio. Y sobre todo: mantenía
 * vivos cuatro fs_get, que es justo lo que #305 viene a quitar.
 *
 * El ESP32 y el STM32 nunca tuvieron modo texto — nacieron ya hablando wire v1,
 * con un solo fichero de REPL cada uno. El Pico era el raro por ser el primero.
 * Ahora los tres son iguales.
 *
 * Nada se pierde por funcionalidad: el wire v1 cubre TIME, RESET y BOOTSEL, que
 * era lo único del modo texto sin equivalente obvio. */
/* V7/H7 (Boot 2, fase 4) — la cintura, la registra el arranque común ANTES del cable y
 * del autorun. Se registraba aquí, al entrar al REPL —o sea DESPUÉS del autorun—, y un IDE
 * que se conectaba con el autorun corriendo recibía «HELLO: este firmware no ha registrado
 * su cintura de REPL» al saludar (durante el autorun sólo se atienden HELLO, KILL y RESET). */
/* V7/H12, etapas 0 y 1 (4-oct) — LA TAREA `run`: el programa en una tarea propia y permanente, y ésta (`vm_task`, la del
 * arranque y el REPL) hace de `io` durante el RUN (src/bpvm_repl.c). El interruptor de la imagen, BPVM_PICO_RUN_TAREA,
 * vive en FreeRTOSConfig.h (también cambia el heap de FreeRTOS): con 0, el RUN de antes. */
#if BPVM_PICO_RUN_TAREA
/* Su pila, ESTÁTICA: en el .bss, y la VM recibe menos ella sola (su bloque es «lo que queda» tras el .bss); ni el heap
 * de FreeRTOS ni el margen de malloc. 16 KB, LOS MISMOS que `vm_task`: el GC corre aquí y su marcado es RECURSIVO —32 B
 * por nivel de una cadena de referencias, medido el 4-oct: con 16 KB en `vm_task`, una lista de ~360 nodos ya
 * desbordaba—; con 12 KB, el techo bajaba a ~300. Sin los marcos del REPL encima, aquí cabe algo más (~425). El heap
 * de FreeRTOS baja 8 KB para compensar (FreeRTOSConfig.h: el `io` de cada RUN ya no nace): la VM pierde 8 KB netos. */
#define PICO_RUN_PILA_PALABRAS (PICO_RUN_PILA_BYTES / sizeof(StackType_t))
static StackType_t  s_run_pila[PICO_RUN_PILA_PALABRAS];
static StaticTask_t s_run_tcb;
static TaskHandle_t s_run_task = NULL;
/* V7/H12, 2b — EL 2º RUN, con `vm.runs=2` en el ENV: su pila la apartó el arranque (g_run2_pila: del techo del bloque
 * de la VM, o de malloc con la VM en la PSRAM); aquí, su TCB. Sin ella, la placa va con uno y lo dice. */
static StaticTask_t s_run2_tcb;
#endif

void repl_v1_cintura(void) {
    bpvm_repl_set_ops(&s_repl_ops);   /* V6/U3 g9: la cintura, antes del primer mensaje */
#if BPVM_PICO_RUN_TAREA
    /* A la MISMA prioridad que ésta, la que hace de `io` (el contrato de bpvm_platform.h: ni por debajo —el KILL no
     * llegaba— ni por encima —PrintBench, más lento—). Antes del autorun: el arranque común llama aquí antes. */
    if (s_run_task == NULL && bpvm_repl_run_tarea_prepara() == 0)
        s_run_task = xTaskCreateStatic(bpvm_repl_run_tarea, "run", PICO_RUN_PILA_PALABRAS, NULL,
                                       uxTaskPriorityGet(NULL), s_run_pila, &s_run_tcb);
    pico_tarea_nucleos(s_run_task);   /* V7/H12, etapa 3.1: `vm.cores=1` (NULL: nada) */
    /* el log, desde aquí y no desde `run`: el log no tiene cerrojo, y `run` nacería escribiendo a la vez que el
     * arranque (la revisión del 4-oct) */
    if (s_run_task == NULL) {
        bpvm_repl_run_tarea_no_nacio();   /* V7/H12, etapa 2a.2: que el RUN no cuelgue programas para un run que no hay */
        log_printf("run: no se pudo crear la tarea run: sigue el RUN de antes");
    } else {
        log_printf("run: la tarea del programa, creada (%u KB de pila; %s)",
                   (unsigned) (PICO_RUN_PILA_PALABRAS * sizeof(StackType_t) / 1024u),
                   bpvm_repl_run_en_cola() ? "espera en la cola" : "espera el aviso «trabajo», como en la etapa 1");
    }
    /* V7/H12, 2b — el 2º run: el run número 1, a la misma prioridad, en la misma cola. Sólo con la cola (con el relevo
     * de la etapa 1, `prepara` no admite otro) y con la pila apartada en el arranque. */
    if (s_run_task != NULL && bpvm_runs() >= 2) {
        if (g_run2_pila != NULL && bpvm_repl_run_en_cola() && bpvm_repl_run_tarea_prepara() == 0) {
            s_run2_task = xTaskCreateStatic(bpvm_repl_run_tarea, "run2", PICO_RUN_PILA_PALABRAS, (void*) (intptr_t) 1,
                                            uxTaskPriorityGet(NULL), (StackType_t*) g_run2_pila, &s_run2_tcb);
            pico_tarea_nucleos(s_run2_task);   /* V7/H12, etapa 3.1: `vm.cores=1` (NULL: nada) */
            if (s_run2_task == NULL) bpvm_repl_run_tarea_no_nacio();
        }
        if (s_run2_task != NULL)
            log_printf("run: el 2o run, creado (vm.runs=%d; %u KB de pila @ 0x%08x): dos runs en la cola",
                       bpvm_runs(), (unsigned) (PICO_RUN_PILA_BYTES / 1024u), (unsigned) (uintptr_t) g_run2_pila);
        else
            log_printf("run: vm.runs=%d, pero el 2o run no se pudo crear (%s): va con uno", bpvm_runs(),
                       g_run2_pila == NULL ? "sin pila apartada" : !bpvm_repl_run_en_cola() ? "sin la cola"
                                                                  : "la tarea no nacio");
    }
#endif
}

void repl_v1_run(void) {
    pico_arranque_completo();   /* V7/H12, etapa 3.1: también sin autorun (o con la placa degradada, que no lo mira) */
    log_printf("REPL entry (wire v1)");
    /* #326 CONTROL: se marca en TODO arranque, haya depurador o no. Es la
     * prueba de que el portador funciona — el equivalente a ejecutar T antes
     * de JsonDemo. Sin control, el silencio del rastro no dice nada. */
    log_flush();   /* snapshot del estado de arranque a flash */

    /* Banner x3: si el host abre el COM con retraso se pierde el primero, y sin
     * banner no hay forma de distinguir "la placa no arrancó" de "el puerto no
     * es este". Cada uno con su flush y su delay para forzar rondas de
     * tud_task(). El IDE drena a la conexión, así que no le estorba. */
    for (int i = 0; i < 3; i++) {
        printf("\n=========================================\n");
        printf(" bpvm-pico listo (wire BPVM v1).\n");
        printf("=========================================\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for (;;) {
        int first = getchar_timeout_us(0);
        if (first < 0) {                      /* nada que leer: ceder la CPU */
            if (bpvm_repl_paso()) continue;   /* #495 (paso 4): una transferencia a medias: el trozo siguiente, sin dormir */
            bpvm_sd_placa_vigilar();          /* V5/H2 → V7/H3: ¿ha entrado o salido la SD? (el común) */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (first != '{') continue;           /* ruido entre requests: descartar */
        repl_v1_handle_request(first);
    }
}
