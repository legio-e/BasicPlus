/*
 * repl_esp32.c — dispatcher del wire BPVM v1 en el firmware ESP32-S3.
 *
 * Adaptado de pico/repl_v1.c. Diferencias clave:
 *  - I/O por UART0 (wire_v1.c ESP32), no stdio/USB-CDC.
 *  - OUTPUT / EXITED / LIST se emiten por el wire (no fputs a stdout —
 *    en ESP32 stdout es la consola USB-JTAG, canal distinto).
 *  - Sin AOT (.mdn es ARM Thumb-2, no cruza a Xtensa) ni mdn_loader.
 *  - Sin BOOTSEL (pico-specific). RESET → esp_restart().
 *  - FS en RAM (fs_ram.c).
 *
 * Subset de comandos: HELLO, INFO, PING, RESET, LIST, STAT, GET, PUT,
 * DEL, RUN. El resto responde UNSUPPORTED.
 */
#include "repl_esp32.h"
#include "wire_v1.h"
#include "json_min.h"
#include "fs.h"
#include "bpvm_fs.h"   /* H19-F1: base-dir por proyecto (bpvm_fs_set_basedir_from_module) */
#include "bpvm_repl.h"     /* V6/U3: el REPL comun + la cintura de familia */

#include "bpvm.h"
#include "bpvm_internal.h"   /* recorrido de vm->modules[] para los .mdn del AOT */
#include "bpvm_entry.h"      /* #344 — el RUN, escrito una vez */
#include "bpvm_rtc.h"        /* H14 — TIME del wire → RTC (bpvm_rtc_set_now_ms) */
#include "bpvm_pico.h"       /* paso 4 cierre — bpvm_pico_reset_cause (INFO) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "board_mgr_esp32.h" /* H9: gestión de placa (STATE/ENV/PART) + board_boot_status */
#include "log.h"             /* log persistente (post-mortem) → LOG_DUMP / LOG_CLEAR */
#include "bpvm_dbg_wire.h"   /* #326: ramo de depuración, núcleo portable compartido */
#include "bpvm_io.h"         /* V6/A1.2: el segundo hilo, comun a todas las familias */
#include "aot_registry.h"    /* H4 AOT: bpvm_aot_clear/count (hook esp_aot_register) */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"      /* esp_restart */
#include "esp_timer.h"       /* uptime */
#include "esp_mac.h"         /* INFO: uniqueId desde la MAC de efuse */
#include "esp_flash.h"       /* INFO: tamaño real de la flash montada */
#include "esp_heap_caps.h"   /* INFO: PSRAM mapeada (0 si el módulo no trae) */
/* mdn_loader NO va dentro del #if: el INFO publica `arch` (bpvm_mdn_host_arch)
 * en TODA la familia — el S3 también tiene que decir a qué ISA compilar. Sólo la
 * CARGA de .mdn dinámico es de momento del P4. */
#include "mdn_loader.h"      /* H4/H11: bpvm_load_mdn + bpvm_mdn_host_arch (INFO) */
#include "bpvm_mdn_scan.h"   /* V5: el bucle que busca el .mdn en FS *y* pack.
                              * Su .c ya estaba en el CMakeLists de las dos
                              * placas ESP32 — compilándose sin que nadie lo
                              * llamara, que es la forma silenciosa de tener
                              * media función. */

/* #465 — «ES RISC-V» NO IMPLICA «TIENE API DE CACHÉ».
 *
 * Estas guardas se escribieron para el P4 —RISC-V con caché gestionada— usando la
 * arquitectura como atajo para «soporta cargar un .mdn en RAM ejecutable». El
 * ENSAYO DEL C3 lo rompió en la primera compilación: el C3 también es RISC-V y su
 * build no encontraba `esp_cache.h`.
 *
 * La condición real es la CAPACIDAD, no la familia. Se pregunta por la cabecera.
 * Para el S3 (Xtensa) y el P4 el resultado es EXACTAMENTE el mismo que antes.
 *
 * ⚠️ CORREGIDO EL MISMO DÍA, y a pregunta de Eduardo (*«la C3 es RISC-V, debería
 * soportar AOT»*). Aquí ponía que `esp_cache.h` *«viene de `esp_mm`, que ese
 * silicio no tiene»*, y **era falso**: el `CMakeLists` de `esp_mm` sólo excluye
 * `linux` y compila `esp_cache_msync.c` para cualquier target. La cabecera no
 * faltaba por el silicio — faltaba porque el proyecto del C3 **no pedía `esp_mm`
 * en `REQUIRES`**, cosa que heredó de copiar la lista del S3, que es Xtensa y no
 * lo necesita. Añadida esa línea, el C3 compila con el camino del `.mdn` DENTRO.
 *
 * O sea que la puerta por capacidad sigue siendo lo correcto —y sigue haciendo
 * falta para el S3—, pero el diagnóstico que la motivó atribuía al chip lo que
 * era un descuido del build. Un «no se puede» que era un «no está pedido». */
#if BPVM_ESP_AOT_MDN && defined(__has_include)
#  if __has_include("esp_cache.h")
#    define BPVM_ESP_AOT_MDN 1
#  endif
#endif
#ifndef BPVM_ESP_AOT_MDN
#  define BPVM_ESP_AOT_MDN 0
#endif

#if BPVM_ESP_AOT_MDN
#include "esp_cache.h"       /* H4 AOT: sync de cachés tras copiar .mdn a RAM exec */
#include "esp_log.h"         /* H4 AOT: trazas del cargador .mdn (consola) */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* H4 AOT — hook de registro de funciones AOT baked-in. Se define FUERTE por
 * target en un fichero aparte que enlaza cada build (P4: aot_funcs_p4.c con el
 * thunk nativo RISC-V; S3: aot_funcs_stub.c no-op). Aqui SOLO se DECLARA: asi
 * repl.o deja la referencia sin resolver y el linker TIRA del objeto fuerte del
 * archive. (Con un weak no-op local el linker se conformaria con el y jamas
 * enlazaria el fuerte del .a → AOT muerto: lo vimos, esp_aot_register salia 'W'
 * y el thunk no aparecia.) Se llama tras link, antes de bpvm_run (igual Pico).
 * El .mdn dinamico cross-arch = Hito 2. */
void esp_aot_register(struct bpvm* vm);

/* Buffer VM compartido (definido en main.c). Ahora PUNTERO (no array): el S3 lo
 * apunta a un array estático en SRAM interna y el P4 a PSRAM reservada en boot.
 * Misma convención que la Pico/Metro (repl_v1.c, H7.2.b). */
extern uint8_t* s_vm_buffer;
extern uint32_t s_vm_buffer_size;

/* H9 — reparto heap/stacks de la VM: la región de stacks BP se TOPA en 512 KB y
 * el RESTO es heap (decisión Eduardo 19-jul, igual que el Pico repl_v1.c). El
 * default de bpvm_init (tamaño/2) desperdiciaba ~1 MB en stacks con los 2 MB de
 * PSRAM del P4. Cálculo ÚNICO, compartido por RUN (bpvm_init) e INFO. */
static size_t vm_stack_region_bytes(void) {
    /* Esta función tenía una COPIA de la regla, y se había quedado en /2
     * mientras el Pico ya iba por /4 — dos placas repartiendo distinto sin que
     * nadie lo hubiera decidido. Ahora pregunta al núcleo (bpvm.h). */
    return bpvm_stack_region_bytes((size_t) s_vm_buffer_size);
}

#define ESP32_BUILD_DATE  (__DATE__ " " __TIME__)

static char    s_line_buf[WIRE_V1_LINE_MAX];
static char    s_reply_buf[2048];
#ifndef V1_PUT_BUF_SIZE
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
 *   (b) el gestor ya no pide TRES sectores aquí: las dos copias del env se las
 *       presta la zona de rascar compartida (board_mgr_esp32.c) y de este buffer
 *       sale sólo el sector de trabajo + la respuesta = BP_ENV_SECTOR + reply.
 * En el S3 son 12 KB de DRAM que vuelven, que es justo la moneda escasa (#336).
 * La respuesta se queda en 4 KB, lo mismo que el STM32 lleva usando desde el
 * primer día — no es una apuesta nueva. */
#define V1_PUT_BUF_SIZE  (8 * 1024)
#endif
static uint8_t s_put_buf[V1_PUT_BUF_SIZE];
typedef char bp_chk_put_buf[(V1_PUT_BUF_SIZE >= 8u*1024u &&
                             V1_PUT_BUF_SIZE >= 4096u + 512u) ? 1 : -1];

/* Identidad de placa para INFO/HELLO. Por defecto = ESP32-S3; una placa la
 * cambia con repl_set_board_id() (p.ej. el P4 desde p4_board_id.c).
 *
 * NB: NO usamos weak/strong. En ESP-IDF cada componente es un .a y el linker
 * solo tira del .o del override si algo referencia su símbolo; con la def débil
 * ya satisfecha, el .o "fuerte" no se enlazaba y el override se ignoraba (salía
 * esp32s3 en el P4). El setter explícito lo evita: main referencia la función
 * de install -> el .o se enlaza, y la llamada fija el puntero. */
static const repl_board_id_t s_default_board = {
    "esp32s3",        /* board_name   */
    "bpvm-esp32",     /* server_name  */
    240000000L,       /* cpu_freq_hz  */
    45,               /* gpio_count   */
    0,                /* pio_count    */
    8,                /* pwm_slices   */
    20,               /* adc_channels */
    512L * 1024L,     /* sram_bytes   */
};
static const repl_board_id_t *s_board_id = &s_default_board;

void repl_set_board_id(const repl_board_id_t *id) { if (id) s_board_id = id; }

/* == V5/H7 - EL PACK NATIVO SE CARGA EN EL PRIMER `Run`, NO AL ARRANCAR ====
 *
 * La razon es de Eduardo y estaba escrita en `pico/pack_pico.c` desde el 7-ago:
 * *«un cuelgue durante un Run se arregla desenchufando una vez; un cuelgue en el
 * ARRANQUE se repite en cada arranque y obliga a regrabar. El salto es el unico
 * paso que puede colgar, asi que se dispara cuando tu quieres, no cuando la
 * placa enciende.»*
 *
 * La decision se aplico en la Pico y NO llego a esta familia: el P4 barria la
 * zona y saltaba dentro de `wire_task`, antes del REPL. Costaba 338 ms de cada
 * arranque -medidos en placa el 16-ago, casi la mitad de los 717 hasta que el
 * wire esta listo- y, lo que importa mas, ponia el unico paso que puede colgar
 * en el sitio del que no se sale sin regrabar.
 *
 * OJO A LO QUE NO SE MUEVE: **mapear** la zona sigue en el arranque. Es gratis
 * (0 ms en el log) y el IDE la necesita desde el principio para poder listar y
 * grabar packs. Lo que se retrasa es BUSCAR el ancla y SALTAR.
 *
 * Y el registro es un SETTER explicito, no weak/strong, por lo mismo que la
 * identidad de placa de aqui arriba: en ESP-IDF cada componente es un `.a` y el
 * override debil no se enlaza. El S3 sencillamente no registra ninguno -no
 * tiene pack nativo- y aqui eso es un puntero nulo, no un caso especial. */
static int32_t (*s_packs_loader)(void) = 0;

void repl_set_packs_loader(int32_t (*fn)(void)) { s_packs_loader = fn; }

/* Se llama en CADA Run y solo trabaja la primera vez que sale bien. Si no hay
 * pack grabado no se marca como hecho: el siguiente Run lo reintenta, que es
 * justo lo que se quiere despues de grabar uno, sin reiniciar la placa. */
static void packs_cargar_una_vez(void) {
    static int s_hecho = 0;
    if (s_hecho || s_packs_loader == 0) return;
    int32_t r = s_packs_loader();
    if (r >= 0) {
        s_hecho = 1;
        log_printf("packs: cargado, la entrada devolvio %ld", (long) r);
    } else {
        /* El detalle -cuantos candidatos, que peldano fallo- ya lo dejo el
         * cargador en el log. Aqui solo se dice que se sigue sin pack, que no
         * es un error: un programa que no use packs no tiene por que llevar
         * ninguno. */
        log_printf("packs: sin pack utilizable (peldano %ld) - se sigue sin el",
                   (long) -r);
    }
}

/* ---- fs_status_t → (code, message) v1 ---- */
static void map_fs_status(fs_status_t s, const char** code, const char** msg) {
    switch (s) {
        case FS_OK:                 *code = "OK";              *msg = "ok"; break;
        case FS_ERR_NOT_FOUND:      *code = "NOT_FOUND";       *msg = "fichero no existe"; break;
        case FS_ERR_EXISTS:         *code = "EXISTS";          *msg = "ya existe"; break;
        case FS_ERR_NO_SPACE:       *code = "NO_SPACE";        *msg = "FS lleno"; break;
        case FS_ERR_NAME_TOO_LONG:  *code = "INVALID_PATH";    *msg = "nombre demasiado largo"; break;
        case FS_ERR_TOO_BIG:        *code = "NO_SPACE";        *msg = "fichero demasiado grande"; break;
        case FS_ERR_TABLE_FULL:     *code = "NO_SPACE";        *msg = "tabla FS llena"; break;
        case FS_ERR_BAD_FLASH:      *code = "INTERNAL_ERROR";  *msg = "flash op falló"; break;
        case FS_ERR_INVALID:        *code = "INVALID_PARAM";   *msg = "argumento inválido"; break;
        default:                    *code = "INTERNAL_ERROR";  *msg = "fs unknown"; break;
    }
}

/* ====================== META ====================== */

/* V6/U3 g5 - HELLO vive en el comun: mismo protoVersion 1, mismo serverName,
 * serverBuild y el array de capabilities tal cual lo da la cintura. */

/* V6/U3 g4 - INFO vive en el comun; lo propio de esta placa es RELLENAR los
 * 18 campos, y eso esta abajo en la cintura (esp32_repl_info). */

/* V6/U3 g1 - PING vive en el comun (identico: responde PONG vacio). */

static void handle_reset(long id, const json_obj_t* obj) {
    (void) obj;
    wire_v1_send_reply_empty("RESET_REPLY", id);
    vTaskDelay(pdMS_TO_TICKS(100));   /* deja vaciar el TX del wire */
    esp_restart();                     /* no retorna */
}

/* ====================== FILES ====================== */

/* #398/#408 — LA MEDIDA DEL REFRESCO DEL ÁRBOL.
 *
 * «El refresco del árbol tarda 1-2 s sin SD y 5 s o más con SD» (Eduardo,
 * 15-ago) — y cualquier operación que refresque (añadir, borrar) lo paga. El
 * arranque ya se midió y NO era: 965 ms con tarjeta, wire listo. O sea que el
 * tiempo se va aquí, en el LIST que alimenta el árbol.
 *
 * Esto NO arregla nada: MIDE, que es lo que toca antes de tocar. Y mide
 * repartiendo por CARPETA RAÍZ en vez de preguntar «¿es la SD?», para no
 * asumir la respuesta: si la cara resulta ser `/lib`, también lo dirá.
 *
 * El sospechoso que hay que confirmar o descartar es el CRC de abajo: se
 * calcula para CADA fichero, leyéndolo entero, en CADA listado. Por eso se
 * cronometra aparte del resto.
 *
 * Coste del instrumento: dos lecturas de reloj por entrada y una línea de log
 * por refresco (ojo con #423, que el log se llena). */
/* Apunta la entrada en el casillero de su carpeta raíz. Un fichero de la raíz
 * (`auto.txt`) cuenta como "/" — que también es un dato: dice si el coste está
 * en los sueltos o en un volumen. */
/* V6/U3.22 — LIST_DIR vive en el común (`src/bpvm_repl.c`). */

/* V6/U3.23 - LIST vive en el comun, DESGLOSE POR RAIZ INCLUIDO: el de aqui
 * subio a bpvm_repl.c en vez de perderse, y ahora la Pico y el STM32 lo ganan. */

/* ── SAVE / DF / MKDIR: el IDE los manda y la familia ESP32 no los tenía ──
 * SAVE lo dispara PicoExplorer tras cada subida y desde el botón de guardar, así
 * que el desfase saltaba en uso normal. Con littlefs la escritura ya es firme
 * (fs_save_to_flash es el punto de sincronización de la fachada), pero el
 * comando debe existir y contestar como en el Pico: el protocolo es UNO. */
/* V6/U3 g4 - SAVE vive en el comun, con su durationMs. El mapeo especifico
 * del fallo (NO_SPACE, INVALID_PATH...) que solo tenia esta familia se SUBIO
 * al comun en el mismo paso: no se pierde, lo ganan las tres. */

/* V6/U3 g4 - DF vive en el comun: mismos cuatro campos y en el mismo orden,
 * y ademas protege la resta total-usado. Los tres getters, en la cintura. */

/* Igual que en el Pico: el `/` es namespace, no hay nodos de directorio → MKDIR
 * es idempotente y silenciosa. Existe para que el IDE no se coma un error. */
/* V6/U3 g2 - MKDIR vive en el comun: en un FS plano con "/" de namespace no
 * hay nodos de directorio, asi que es idempotente y silenciosa. */

/* ── LOG (post-mortem) ── Escapa el texto a JSON y lo va soltando por bulk, sin
 * buffer intermedio del tamaño del log. Mismo formato de reply que el Pico. */
/* V6/U3 g1 - el sink de escapado del LOG vive en el comun. El de aqui ya
 * emitia \uXXXX para los caracteres de control, igual que el comun: no habia
 * divergencia que arreglar en esta familia. */

/* V6/U3 g1 - LOG_DUMP vive en el comun (misma cabecera, mismo bulk, mismo
 * cierre; el comun ademas comprueba el snprintf). */

/* V6/U3 g1 - LOG_CLEAR vive en el comun (identico, incluida la linea que
 * fecha el corte en el log nuevo). */

/* ── #326 DEPURACIÓN: cintura del núcleo portable bpvm_dbg_wire ──
 * La máquina de depurar (breakpoints por pc, pausa, step) ya estaba en la placa
 * —vive en el intérprete— pero el ESP32 no tenía el cableado del wire, así que
 * "Debug on device" contestaba UNSUPPORTED. Lo único de aquí es traducir JSON ↔
 * comando tipado y prestar los buffers; la lógica es la MISMA que la del Pico
 * (de ahí se extrajo el núcleo).
 *
 * Sobre reusar s_line_buf en el bucle de pausa: es correcto porque el pause_cb
 * corre DENTRO de bpvm_run, en la misma task del REPL, y el `obj` de la request
 * que lanzó el RUN ya no se usa cuando la VM está corriendo. Mismo esquema que
 * el Pico lleva funcionando desde #140. */
static void dbgw_send(const char* line, size_t len, void* user) {
    (void) user;
    wire_v1_send_line(line, len);
}

static int dbgw_next_cmd(bpvm_dbg_cmd_t* out, void* user) {
    (void) user;
    int n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
    if (n <= 0) return -1;                       /* overflow / vacía: reintentar */
    json_obj_t o;
    if (json_parse(s_line_buf, (size_t) n, &o) != 0) return -1;
    char type[40];
    if (json_get_str(&o, "type", type, sizeof type) < 0) return -1;
    out->kind = bpvm_dbg_wire_kind(type);
    out->id   = json_get_long(&o, "id",    0);
    out->pc   = json_get_long(&o, "pc",   -1);
    out->bpId = json_get_long(&o, "bpId", -1);
    out->addr = json_get_long(&o, "addr", -1);
    out->ref  = json_get_long(&o, "ref",   0);
    return 0;
}

/* Vive todo el RUN: el núcleo lo recibe como `user` del pause_cb. */
static bpvm_dbg_wire_t s_dbgw = {
    dbgw_next_cmd, dbgw_send, NULL, s_reply_buf, sizeof s_reply_buf, 0
};

/* Rellena un comando tipado desde el JSON ya parseado (camino fuera de pausa). */
static void dbgw_cmd_from_json(bpvm_dbg_cmd_t* c, long id,
                               const json_obj_t* obj, const char* type) {
    c->kind = bpvm_dbg_wire_kind(type);
    c->id   = id;
    c->pc   = json_get_long(obj, "pc",   -1);
    c->bpId = json_get_long(obj, "bpId", -1);
    c->addr = json_get_long(obj, "addr", -1);
    c->ref  = json_get_long(obj, "ref",   0);
}

/* V6/U3 g2 - STAT vive en el comun (mismo bpvm_fs_stat, mismo CRC bajo
 * demanda de #398, mismos campos). Se comparo linea a linea antes de borrar. */

/* V6/U3 g2 - GET vive en el comun. Comparado linea a linea: mismo
 * bpvm_fs_stat para la cabecera, mismos trozos de 256 B (el interno de
 * littlefs), mismo bpvm_fs_read_at y los mismos errores. */

/* V6/U3 g12 - EL GRUPO PUT VIVE EN EL COMUN (PUT + PUT_BEGIN/DATA/END).
 *
 * Lo que hacia falta para poder migrarlo no era el handler: era QUIEN LEE EL
 * BULK. El bulk viaja detras de la linea JSON y solo puede leerlo uno; el comun
 * lo lee DENTRO del handler, asi que la pre-lectura del despachador tenia que
 * dejar de ocurrir para estos cuatro verbos. Eso lo prepara `U3.20` (subir el
 * `type`) y lo activa la bandera `lo_lee_el_comun` de abajo.
 *
 * Comparado linea a linea antes de borrar, y salio UNA diferencia: el `fs_put`
 * de esta familia crea los directorios que falten y el comun no lo hacia. En vez
 * de migrar perdiendola, se subio al comun (#455) -- donde ademas la RECUPERAN
 * la Pico y el STM32, que la perdieron sin ruido en `U3.12`. */
/* V6/U3 g2 - DEL vive en el comun, y ADEMAS mejora: el de aqui juntaba "no
 * existe" con "existe y no se puede borrar", que en el segundo caso es
 * simplemente falso. El comun los distingue (la Pico ya lo hacia). */

/* ====================== TERMINAL (RUN) ====================== */

/* Resolución de módulo: base-dir del proyecto, luego /app/ y /lib/ (igual que Pico).
 *
 * H11 — devuelve la RUTA que existe y su tamaño, NO los bytes. Antes resolvía a un
 * puntero, y sostener ese puntero costaba un espejo estático de 64 KB: el fichero
 * entero en RAM sólo para que el loader lo copiase a memory[]. Ahora el llamante
 * abre por esa ruta y lee por trozos. Mismo cambio que cerró #305 en el Pico. */
static fs_status_t v1_resolve_path(const char* name, char* out, size_t out_cap,
                                   uint32_t* size_out) {
    /* #344 — la REGLA vive en el núcleo (bpvm_entry_resolve): basedir del
     * proyecto → tal cual → /app → /lib. Estas 15 líneas estaban COPIADAS
     * palabra por palabra en Pico, ESP32 y STM32. Aquí sólo queda la traducción
     * al fs_status_t que usa el mapeo de errores del wire. */
    return (bpvm_entry_resolve(name, out, out_cap, size_out) == 0)
           ? FS_OK : FS_ERR_NOT_FOUND;
}

/* (El lector por trozos vivía aquí. Se lo llevó #344: ahora el que abre el .mod
 * y lo lee a cachos es bpvm_load_entry_file, en el núcleo, para las cinco.) */

static long s_active_session = 0;
static long s_next_session = 1;

typedef struct { long session; } v1_sink_ctx_t;

/* Sink de la VM: cada chunk del programa → evento OUTPUT por el wire.
 * Trocea en frames de ≤~900 bytes raw para no desbordar el buffer. */
static void v1_output_sink(const char* data, size_t len, void* user) {
    v1_sink_ctx_t* ctx = (v1_sink_ctx_t*) user;
    size_t i = 0;
    do {
        char buf[1024];
        int hn = snprintf(buf, sizeof(buf),
                          "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"",
                          ctx->session);
        if (hn < 0) return;
        size_t o = (size_t) hn;
        while (i < len && o < sizeof(buf) - 8) {
            char c = data[i++];
            switch (c) {
                case '"':  buf[o++] = '\\'; buf[o++] = '"'; break;
                case '\\': buf[o++] = '\\'; buf[o++] = '\\'; break;
                case '\n': buf[o++] = '\\'; buf[o++] = 'n'; break;
                case '\r': buf[o++] = '\\'; buf[o++] = 'r'; break;
                case '\t': buf[o++] = '\\'; buf[o++] = 't'; break;
                default:
                    if ((unsigned char) c < 0x20) {
                        static const char* HEX = "0123456789abcdef";
                        buf[o++] = '\\'; buf[o++] = 'u'; buf[o++] = '0'; buf[o++] = '0';
                        buf[o++] = HEX[(c >> 4) & 0xF];
                        buf[o++] = HEX[c & 0xF];
                    } else {
                        buf[o++] = c;
                    }
                    break;
            }
        }
        buf[o++] = '"';
        buf[o++] = '}';
        wire_v1_send_line(buf, o);
    } while (i < len);
}

static void send_exited(long session, const char* status, int exit_code,
                        long elapsed_ms, const char* errmsg) {
    int off = wire_v1_msg_begin_event(s_reply_buf, sizeof(s_reply_buf), 0, "EXITED");
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "session", session);
    if (off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "status", status);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "exitCode", exit_code);
    if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "elapsedMs", elapsed_ms);
    if (errmsg && off >= 0) off = wire_v1_field_string(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "errorMessage", errmsg);
    if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
    if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);
}

/* P-run-stop (#257) + P-autorun (#256) — wire durante el run (la VM
 * invoca el poll entre quanta, en la MISMA task que corre bpvm_run —
 * aquí no hay comm task, así que el poll puede responder directamente
 * sin riesgo de entrelazado):
 *   KILL  → ack diferido (KILL_REPLY tras parar, antes del EXITED) +
 *           devuelve 1 (BPVM_KILLED).
 *   HELLO → HELLO_REPLY inmediato — el IDE puede conectarse con un
 *           (auto)run en marcha y ofrecer Stop.
 *   otra  → error BUSY inmediato. */
static long s_kill_ack_id = -1;

#if BPVM_ESP_AOT_MDN
/* H4 AOT — RAM ejecutable de los .mdn cargados en este RUN. El loader es
 * zero-copy (los thunks apuntan a estos buffers), así que persisten durante el
 * run y se liberan justo después. */
#define MDN_MAX_EXEC 16
static void* s_mdn_exec[MDN_MAX_EXEC];
static int   s_mdn_exec_n;

/*
 * Trae un `.mdn` DEL FS a RAM ejecutable. Es el callback de
 * `bpvm_mdn_escanear`: la parte que sólo esta familia sabe hacer.
 *
 * <p>La otra fuente —el pack— NO pasa por aquí y es a propósito: su zona está
 * mapeada como INST, o sea que su código ya es ejecutable donde está y copiarlo
 * sería gastar RAM para nada. El bucle común le pasa al loader el puntero XIP
 * directamente. Lo confirma el arranque: *«INST y DATA dan LA MISMA
 * dirección»*.
 *
 * <p>La DRAM del ESP32 sí necesita la copia: en Xtensa hay IRAM y DRAM
 * separadas y la segunda no ejecuta. En el P4 (RISC-V) la RAM interna es
 * unificada y por eso `MALLOC_CAP_EXEC` ni existe ahí — de ahí el #ifdef.
 */
static const uint8_t* esp32_mdn_del_fs(void* user, const char* nombre,
                                       uint32_t* len) {
    (void) user;
    if (s_mdn_exec_n >= MDN_MAX_EXEC) return NULL;   /* ya no caben más overlays */

    char     real[FS_NAME_LEN];
    uint32_t size = 0;
    if (v1_resolve_path(nombre, real, sizeof(real), &size) != FS_OK) return NULL;

#ifdef MALLOC_CAP_EXEC
    void* exec = heap_caps_malloc(size, MALLOC_CAP_EXEC);
#else
    void* exec = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
    if (!exec) {
        /* Se dice AQUÍ: para el bucle esto es «no está», y un descarte por
         * memoria en silencio no se distinguiría de no tenerlo. */
        log_printf("AOT: %s (%u B) sin RAM ejecutable — sin overlay",
                   real, (unsigned) size);
        return NULL;
    }
    if (bpvm_fs_read(real, (uint8_t*) exec, size) != (long) size) {
        log_printf("AOT: %s no se pudo leer entero — sin overlay", real);
        heap_caps_free(exec);
        return NULL;
    }
#if BPVM_ESP_AOT_MDN
    /* Coherencia de cachés: bajar la D-cache a memoria (C2M), invalidar la
     * I-cache y `fence.i`. Sin esto se ejecutaría código rancio o basura.
     * Va antes de que el loader registre nada: los thunks apuntan aquí. */
    esp_cache_msync(exec, size,
        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    esp_cache_msync(exec, size,
        ESP_CACHE_MSYNC_FLAG_TYPE_INST | ESP_CACHE_MSYNC_FLAG_INVALIDATE
        | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    __asm__ volatile ("fence.i" ::: "memory");
#endif
    /* Se apunta para liberarlo al acabar el RUN, cargue o no: si el loader lo
     * rechaza (arch/ABI), el buffer ya está pedido y hay que devolverlo igual. */
    s_mdn_exec[s_mdn_exec_n++] = exec;
    *len = size;
    return (const uint8_t*) exec;
}

static void esp32_mdn_decir(void* user, const char* msg) {
    (void) user;
    log_printf("%s", msg);
}
#endif

static int esp32_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    int c = wire_v1_try_getchar();
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
    if (n < 0) return 0;
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) return 0;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) { s_kill_ack_id = rid; return 1; }
    if (strcmp(type, "HELLO") == 0) { bpvm_repl_dispatch(type, rid, &obj); return 0; }   /* attach en caliente */
    wire_v1_send_error(rid, "BUSY", "ejecución en curso: solo HELLO/KILL");
    return 0;
}

/* -- V6/A1.2 - LAS DOS TAREAS, TAMBIEN AQUI --------------------------------
 *
 * Durante un RUN esta tarea se dedica a lo suyo -ejecutar opcodes- y el hilo
 * `io` (comun, src/bpvm_io.c) se lleva el resto: lee el wire en todo momento y
 * enmarca la salida del programa, una vez POR LINEA. Es EL MISMO arranque que
 * en el PC (test/main.c y el simulador), palabra por palabra: lo unico de esta
 * familia son estas dos costuras, su `poll` y su `line`.
 *
 * El `line` ya existia y no se toca (`v1_output_sink`): lo que cambia es que
 * ahora lo llama `io` con una linea entera en vez de la VM con cada trozo.
 *
 * La cola: 2 KB. No es un buffer de salida, es un amortiguador - cuando se
 * llena, `vm` espera a que `io` drene (contrapresion), que es lo correcto: un
 * programa no puede producir mas deprisa de lo que el transporte traga, y
 * fingir lo contrario seria comerse la RAM del micro. */
#define ESP32_IO_OQ_BYTES 2048

static int esp32_io_poll(void* user) {
    (void) user;
    return esp32_run_poll_cb(NULL, NULL);   /* el de siempre: KILL / HELLO / BUSY */
}

/* Núcleo del RUN — compartido entre el comando RUN del wire (id >= 0)
 * y el autorun de boot (#256, id < 0). Con id < 0 no hay cliente: sin
 * RUN_REPLY y errores de resolución a la consola (USB-Serial-JTAG).
 * Lo demás (sesión, OUTPUT, poll, EXITED) es idéntico. */
static void run_module_path(const char* path, long id) {
    if (s_active_session != 0) {
        if (id >= 0) wire_v1_send_error(id, "BUSY", "ya hay una sesión RUN en curso");
        else         printf("[autorun] BUSY — ignorado\n");
        return;
    }

    /* H19-F1 — fija el base-dir del proyecto si el módulo vive en /app/<proj>/
     * (el IDE manda la ruta cualificada). Plano (/app/X.mod o nombre suelto) →
     * sin base-dir = modo plano. Se resetea en cada run. */
    bpvm_fs_set_basedir_from_module(path);

    /* V5/H7 - los packs, ANTES de resolver: si el programa usa una API de pack
     * tiene que estar publicada cuando llegue la primera llamada. Ver arriba
     * por que aqui y no en el arranque. */
    packs_cargar_una_vez();
    /* H19-F2 diag — confirma en consola (idf.py monitor) la raíz del proyecto:
     * con proyecto sale basedir='/app/<proj>'; en fichero-suelto basedir=''.
     * Si esta línea NO aparece, el firmware es PRE-H19 (reflashear). */
    printf("[run] entry='%s' basedir='%s'\n", path, bpvm_fs_basedir());

    char main_path[FS_NAME_LEN]; uint32_t size;
    fs_status_t fs_s = v1_resolve_path(path, main_path, sizeof(main_path), &size);
    if (fs_s != FS_OK) {
        const char* c; const char* m; map_fs_status(fs_s, &c, &m);
        if (id >= 0) wire_v1_send_error(id, c, m);
        else         printf("[autorun] %s: %s — REPL normal\n", path, m);
        return;
    }

    long session = s_next_session++;
    s_active_session = session;
    if (id >= 0) {   /* RUN_REPLY con session antes de ejecutar. */
        int off = wire_v1_msg_begin(s_reply_buf, sizeof(s_reply_buf), 0, "RUN_REPLY", id);
        if (off >= 0) off = wire_v1_field_long(s_reply_buf, sizeof(s_reply_buf), (size_t) off, "session", session);
        if (off >= 0) off = wire_v1_msg_end(s_reply_buf, sizeof(s_reply_buf), (size_t) off);
        if (off >= 0) wire_v1_send_line(s_reply_buf, (size_t) off);
    }

    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size,
                           s_vm_buffer_size - (uint32_t) vm_stack_region_bytes());
    if (!vm) { send_exited(session, "INTERNAL_ERROR", -1, 0, "bpvm_init failed"); s_active_session = 0; return; }

    v1_sink_ctx_t sink_ctx = { session };
    bpvm_set_output(vm, v1_output_sink, &sink_ctx);

    /* #344 — UNA carga: bpvm_load_entry despacha .mod/.pack, lee por trozos
     * (H11: el .mod se queda en el FS, nada de espejo en .bss), resuelve las
     * dependencias con la regla común (FS y, si no está, los packs grabados en
     * XIP) y NOMBRA la que falte. Aquí vivía el bucle de 4 pasadas copiado en
     * las tres familias, y además era una versión POBRE: cortaba el import por
     * el primer punto y no sabía nada del pack en ejecución — que es lo que
     * impedía ejecutar packs en la placa. */
    /* [V6/N1.4] EL VACIADO VA ANTES DE CARGAR, y el orden es el fallo entero.
     *
     * Cada RUN recarga los módulos en direcciones frescas, así que el registro
     * AOT del RUN anterior hay que tirarlo. Pero desde que un `.mod` puede
     * traer su bloque nativo DENTRO, `bpvm_load_entry` YA REGISTRA thunks — y
     * vaciar después borraba lo que la carga acababa de poner.
     *
     * No da error: el módulo corre interpretado y el resultado sale bien. En la
     * Pico se vio el 23-ago y sólo con el log encendido (`0/N thunks`). Aquí
     * estaba igual, en el REPL que comparten el S3 y la P4. */
    bpvm_aot_clear();

    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t ls = bpvm_load_entry(vm, path, &entry);
    if (ls != BPVM_OK) {
        /* #421 — el detalle VIAJA. Antes, todo lo que no fuera «falta un
         * modulo» salia como `bpvm_status_str(ls)`, o sea «IO error», y el IDE
         * mostraba `exit 1 (IO error)`: el firmware sabia por que habia fallado
         * —y lo escribia en su log— pero por el wire no iba nada. */
        if (entry.missing[0]) {
            char em[80]; snprintf(em, sizeof(em), "falta el modulo '%s'", entry.missing);
            send_exited(session, "RUNTIME_ERROR", (int) ls, 0, em);
        } else if (entry.fallo[0]) {
            send_exited(session, "RUNTIME_ERROR", (int) ls, 0, entry.fallo);
        } else {
            send_exited(session, "RUNTIME_ERROR", (int) ls, 0, bpvm_status_str(ls));
        }
        bpvm_destroy(vm); s_active_session = 0; return;
    }
    if (entry.from_pack)
        printf("[run] pack '%s' (main=%s)\n", entry.resolved, entry.main_module);

    /* H4 AOT — registrar funciones AOT baked-in tras link, antes de run (mismo
     * punto que la Pico). En el P4 (RISC-V) esp_aot_register (fuerte, aot_funcs_p4.c)
     * arma los thunks nativos compilados para RISC-V; en el S3 (Xtensa) es weak
     * no-op.
     *
     * ⚠️ EL `bpvm_aot_clear()` YA NO ESTÁ AQUÍ — ver arriba, antes de la carga.
     * Desde [V6/N1.4] el `.mod` puede traer su bloque nativo DENTRO, y el
     * cargador registra sus thunks durante `bpvm_load_entry`. Vaciar el
     * registro después de cargar borraba justo eso. */
    esp_aot_register(vm);

#if BPVM_ESP_AOT_MDN
    /* ── H4 AOT — los `.mdn` de este RUN, con EL BUCLE COMÚN ──────────────────
     *
     * `bpvm_mdn_escanear` recorre los módulos cargados y busca el puente de
     * cada uno en LAS DOS fuentes —el FS y la zona de packs—, aplicando el
     * criterio de Eduardo *«lo que está en un pack busca primero dentro del
     * pack»*, y avisando de los dos casos que se investigan como otra cosa: el
     * eclipse (*«se toma del FS; el del pack queda TAPADO»*) y el .mdn que
     * carga sin enganchar ni un thunk.
     *
     * ⚠️ Aquí había una COPIA de ese bucle que sólo miraba el FS. Con
     * `SQLite.mod` viniendo del pack y su `.mdn` dentro del pack —que es el
     * caso normal desde V5/H8— el puente no se encontraba nunca, las funciones
     * `native` se quedaban sin implementación y la primera llamada lanzaba un
     * RuntimeError sin una sola línea de salida previa. Costó el 13 de agosto
     * entero, y fue el TERCER fallo del mismo día con esta forma: un arreglo
     * que vive en el bucle común y no llega a la familia que tiene su propia
     * copia (los otros dos: la zona sin montar y el detalle del RuntimeError).
     *
     * De aquí en adelante lo específico de la familia es sólo el callback:
     * traer el fichero del FS a RAM ejecutable. El QUÉ y el ORDEN son de todos. */
    s_mdn_exec_n = 0;
    bpvm_mdn_escanear(vm, esp32_mdn_del_fs, esp32_mdn_decir, NULL);
#endif

    /* Ejecutar. bpvm_run single-thread (el SMP en ESP32 es H4.2+). */
    s_kill_ack_id = -1;
    bpvm_set_poll(vm, esp32_run_poll_cb, NULL);   /* P-run-stop (#257) */

    /* #326 — si el IDE dejó breakpoints o pidió PAUSE antes del RUN, engancha el
     * depurador: aplica los pendientes y registra el callback de pausa. Si no hay
     * nada armado no hace nada (coste cero en un run normal). */
    s_dbgw.session = session;
    bpvm_dbg_wire_arm(&s_dbgw, vm);

    /* V6/A1.2 - arranca `io` (ver arriba). CON EL DEPURADOR ARMADO, NO: su
     * `pause_cb` lee el wire desde ESTA tarea, y dos lectores del mismo
     * transporte es una carrera. Mientras el depurador no hable por la cola de
     * control (A1.7), un RUN con breakpoints sigue por el camino de un hilo:
     * el mismo interbloqueo que la Pico ya hace en su camino SMP. */
    v1_sink_ctx_t io_ctx = { session };
    bpvm_io_ops_t io_ops = { esp32_io_poll, v1_output_sink, &io_ctx };
    int con_io = !bpvm_dbg_wire_armed()
                 && bpvm_io_start(vm, &io_ops, ESP32_IO_OQ_BYTES) == 0;

    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bpvm_status_t rs = bpvm_run(vm);
    uint32_t dt = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0;

    /* Parar `io` ANTES de nada mas: drena la cola entera, asi que al volver de
     * aqui la ultima linea del programa YA salio por el wire. Y a partir de
     * este punto el wire vuelve a ser de esta tarea, que es quien manda el
     * KILL_REPLY y el EXITED. */
    if (con_io) bpvm_io_stop(vm);

    /* P-run-stop — ack diferido del KILL, ANTES del EXITED. */
    bpvm_set_poll(vm, NULL, NULL);
    if (s_kill_ack_id >= 0) { wire_v1_send_reply_empty("KILL_REPLY", s_kill_ack_id); s_kill_ack_id = -1; }

    const char* link_err = bpvm_link_error(vm);   /* paso 4 — "" salvo fallo de link */
    const char* status_str = (rs == BPVM_OK)     ? "OK"
                           : (rs == BPVM_KILLED) ? "KILLED"
                           : (link_err[0])       ? "LINK_ERROR" : "RUNTIME_ERROR";
    int exit_code = (rs == BPVM_OK) ? 0 : (rs == BPVM_KILLED) ? 130 : (int) rs;
    /* ── QUÉ error, no SÓLO que hubo uno ────────────────────────────────────
     *
     * Tres fuentes, de la más concreta a la más genérica: el fallo de enlace,
     * el DETALLE del RuntimeError (`bpvm_runtime_error`, p.ej. "referencia a
     * objeto eliminado" o el texto del throw), y por último el nombre del
     * status.
     *
     * ⚠️ El del medio FALTABA en esta familia. La Pico lo manda desde #280 y
     * aquí no se llevó, así que la P4 sólo podía decir `exit 11 (RuntimeError
     * BP no atrapado)` — que es la categoría, no el error. Dos días de
     * diagnóstico a ciegas costó, el 12 y el 13 de agosto: se veía QUE fallaba
     * y no HABÍA forma de saber por qué desde el IDE.
     *
     * Es el mismo patrón que el montaje de la zona de packs de esta mañana:
     * un arreglo que entra en una familia y no viaja a las otras. Si se toca
     * este orden, tocarlo también en `pico/repl_v1.c`. */
    const char* rt_err = bpvm_runtime_error(vm);
    const char* err_msg = (rs == BPVM_OK) ? NULL
                        : (link_err[0] ? link_err
                                       : (rt_err[0] ? rt_err : bpvm_status_str(rs)));
    send_exited(session, status_str, exit_code, (long) dt, err_msg);

#if BPVM_ESP_AOT_MDN
    /* H4 AOT — el run terminó: los thunks .mdn ya no se ejecutan → liberar la RAM
     * ejecutable. bpvm_aot_clear() del próximo RUN limpia el registry. */
    for (int k = 0; k < s_mdn_exec_n; k++) heap_caps_free(s_mdn_exec[k]);
    s_mdn_exec_n = 0;
#endif

    /* #326 — la sesión de depuración muere con el RUN: olvida la vm y los
     * breakpoints pendientes para que la siguiente parta limpia. */
    bpvm_dbg_wire_reset();

    /* MARCA DE AGUA (#336): heap_caps_get_minimum_free_size da el MÍNIMO
     * HISTÓRICO de DRAM interna libre, no el de ahora. Es exactamente el dato
     * que falta para saber cuánto se le puede subir al bloque de la VM sin
     * ahogar al IDF — que en el ESP32 crea tareas DESPUÉS del arranque, así
     * que pasarse no rompe al arrancar sino más tarde, que es peor.
     * Se emite tras cada RUN, que es cuando la memoria hace pico. */
    log_printf("mem: DRAM interna libre %u B | MINIMO HISTORICO %u B (bloque mayor %u B)",
               (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned) heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    log_flush();
    bpvm_destroy(vm);
    s_active_session = 0;
}

static void handle_run(long id, const json_obj_t* obj) {
    char path[FS_NAME_LEN];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        wire_v1_send_error(id, "INVALID_PARAM", "falta 'path'"); return;
    }
    run_module_path(path, id);
}

/* P-autorun (#256) — si existe /sys/auto.txt, ejecuta el módulo de su
 * primera línea por el mismo camino que un RUN del wire. app_main lo
 * llama tras fs_init + wire init, antes del bucle REPL. El poll del
 * run atiende HELLO/KILL → la placa nunca queda sorda. */
/* #345 paso 2 — cintura del gate. Cuatro funciones y ni un verbo nuevo: el
 * HELLO que el IDE ya manda al conectar dice "hay alguien", y el KILL que ya
 * manda el Stop dice "no arranques". La política está en el núcleo. */
static void esp32_autorun_anuncia(const char* path, int ventana_ms, void* user) {
    (void) user;
    log_printf("autorun: %s arranca en %d ms — Stop en el IDE para cancelar",
               path, ventana_ms);
    printf("[autorun] %s en %d ms (Stop para cancelar)\n", path, ventana_ms);
}

static int esp32_autorun_escucha(void* user) {
    (void) user;
    int c = wire_v1_try_getchar();
    if (c < 0) return 0;                       /* nada pendiente */
    int n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
    if (n < 0) return 1;                       /* línea rota, pero HAY alguien */
    json_obj_t obj;
    if (json_parse(s_line_buf, (size_t) n, &obj) != 0) return 1;
    char type[24] = {0};
    json_get_str(&obj, "type", type, sizeof(type));
    long rid = json_get_long(&obj, "id", 0);
    if (strcmp(type, "KILL") == 0) {
        wire_v1_send_reply_empty("KILL_REPLY", rid);
        return 2;
    }
    if (strcmp(type, "HELLO") == 0) { bpvm_repl_dispatch(type, rid, &obj); return 1; }   /* attach en caliente */
    return 1;                                  /* hay alguien: basta con eso */
}

static void esp32_autorun_espera(int ms, void* user) {
    (void) user; vTaskDelay(pdMS_TO_TICKS(ms));
}

static uint32_t esp32_autorun_ahora(void* user) {
    (void) user; return (uint32_t) (esp_timer_get_time() / 1000);
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    esp32_autorun_anuncia, esp32_autorun_escucha,
    esp32_autorun_espera,  esp32_autorun_ahora, NULL
};

void repl_esp32_autorun(void) {
    /* #345 — leer y limpiar la primera línea lo hace el núcleo. (H11 sigue
     * valiendo: sólo la CABEZA del fichero, nunca el espejo de 64 KB.) */
    char path[FS_NAME_LEN];
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */

    /* #345 paso 2 — la ventana de rescate. Decide el usuario. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        printf("[autorun] CANCELADO por el usuario — REPL normal\n");
        return;
    }
    run_module_path(path, -1);
    printf("[autorun] terminado — REPL normal\n");
}

/* ====================== V6/U3 — LA CINTURA DE ESTA FAMILIA ======================
 *
 * Tercera y última familia del hito (el STM32 y la Pico ya están migrados). El
 * contrato es `bpvm_repl_ops_t` (include/bpvm_repl.h): el común pone la lógica y
 * las replies, y aquí sólo va lo que de verdad es de esta placa.
 *
 * 📌 ESTE PASO ES PURAMENTE ADITIVO: no se borra ni un handler. El despacho común
 * se encadena AL FINAL, así que todo lo que el ESP32 ya implementa sigue ganando
 * y sólo cae al común lo que hoy no tiene. Efecto neto: esta familia GANA
 * `FORMAT`, `RENAME` y `RMDIR`, que le faltaban desde siempre (U3.0). Los verbos
 * se irán migrando después, grupo a grupo, cada uno con su verificación.
 *
 * El grupo PUT migró en `U3.21`, y lo que había que mover no era el handler
 * sino QUIÉN LEE EL BULK: el común lo lee dentro del handler, así que la
 * pre-lectura del despachador tuvo que dejar de ocurrir para esos cuatro verbos
 * (bandera `lo_lee_el_comun`). Leerlo dos veces desincroniza el wire, y como es
 * binario eso no da error: CORROMPE.
 *
 * `after_put` se queda a NULL a propósito (littlefs committea en cada close, lo
 * mismo que la Pico); el común comprueba las piezas que faltan y contesta un
 * `UNSUPPORTED` con nombre en vez de reventar. */

/* Los 18 campos del INFO. El mensaje ya no se arma aqui (lo hace
 * src/bpvm_repl.c): esto solo RELLENA, que es lo unico propio del ESP32.
 * Los valores son los MISMOS que calculaba handle_info -- se movieron tal cual,
 * sin "mejorar" ninguno de paso, para que la comparacion siga valiendo.
 *
 * Datos del chip S3 (datasheet): 45 GPIOs (0-21 y 26-48), sin PIO (el RMT no es
 * comparable), PWM = 8 canales LEDC, ADC = 20 canales, SRAM interna 512 KB.
 * Flash y PSRAM se miden en runtime: dependen del modulo montado. */
static void esp32_repl_info(bpvm_repl_info_t* o) {
    static char uid[16];
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(uid, sizeof uid, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* Tamano FISICO del chip (SFDP/JEDEC), NO el del sobre del build:
     * esp_flash_get_size devuelve el FLASHSIZE del sdkconfig (horneado). Mismo
     * criterio que el JEDEC del RP2350 (#292): la verdad es el chip. */
    uint32_t flash_bytes = 0;
    if (esp_flash_get_physical_size(NULL, &flash_bytes) != ESP_OK || flash_bytes == 0) {
        if (esp_flash_get_size(NULL, &flash_bytes) != ESP_OK) flash_bytes = 0;
    }

    const repl_board_id_t* bid = s_board_id;
    size_t vstack = vm_stack_region_bytes();

    o->unique_id     = uid;
    o->board_name    = bid->board_name;
    o->reset_reason  = bpvm_pico_reset_cause();
    o->arch          = (unsigned) bpvm_mdn_host_arch();
    o->cpu_hz        = (unsigned long) bid->cpu_freq_hz;
    o->uptime_ms     = (unsigned long) (esp_timer_get_time() / 1000LL);
    o->temp_milli_c  = 0;            /* sensor interno, para mas adelante */
    o->gpio_count    = bid->gpio_count;
    o->pio_count     = bid->pio_count;
    o->pwm_slices    = bid->pwm_slices;
    o->adc_channels  = bid->adc_channels;
    o->flash_bytes   = (unsigned long) flash_bytes;
    o->sram_bytes    = (unsigned long) bid->sram_bytes;
    o->psram_bytes   = (unsigned long) heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    o->vm_heap_bytes = (unsigned long) (s_vm_buffer_size - vstack);
    o->vm_stack_bytes= (unsigned long) vstack;
    o->fs_total_bytes= (unsigned long) fs_total_bytes();
    o->fs_used_bytes = (unsigned long) fs_used_bytes();
}

static int  esp32_repl_fs_format(void) { fs_format_ram(); return 0; }
static int  esp32_repl_fs_save(void)   { return fs_save_to_flash() == FS_OK ? 0 : -1; }
static unsigned long esp32_repl_fs_total(void) { return (unsigned long) fs_total_bytes(); }
static unsigned long esp32_repl_fs_used(void)  { return (unsigned long) fs_used_bytes(); }
static int  esp32_repl_fs_count(void)  { return (int) fs_file_count(); }

/* ⚠️ NO es `const`: `server_name` se rellena en runtime desde `s_board_id`.
 * Lo puse literal ("bpvm-esp32") al enganchar la cintura y ERA UN BUG LATENTE:
 * este fichero lo comparten el S3 y la P4, y la P4 se llama "bpvm-esp32p4"
 * (`p4_board_id.c`). No habia mordido porque HELLO seguia siendo de la familia
 * —que si lee `s_board_id`—, y habria mordido justo al migrarlo. Lo caza
 * comparar antes de borrar, no leer el codigo por encima. */
static bpvm_repl_ops_t s_repl_ops = {
    .info           = esp32_repl_info,
    .server_name    = "bpvm-esp32",   /* se pisa en repl_esp32_run con el real */
    .server_build   = ESP32_BUILD_DATE,
    .capabilities   = "[\"META\",\"FILES\",\"TERMINAL\"]",
    .fs_total_bytes = esp32_repl_fs_total,
    .fs_used_bytes  = esp32_repl_fs_used,
    .fs_file_count  = esp32_repl_fs_count,
    .fs_format      = esp32_repl_fs_format,
    .fs_save        = esp32_repl_fs_save,
    /* V6/U3 g12: el scratch del bulk lo presta la familia; el común lo llena. */
    .put_buf        = s_put_buf,
    .put_buf_size   = sizeof s_put_buf,
    /* Sin `after_put` a propósito, igual que la Pico: littlefs committea en cada
     * close, así que no hay nada que persistir después (el STM32 sí lo necesita,
     * su FS vive en un sector que hay que volcar). */
    .after_put      = NULL,
};

/* ====================== Dispatcher ====================== */

static void handle_request(const char* line, int len) {
    json_obj_t obj;
    if (json_parse(line, (size_t) len, &obj) != 0) {
        wire_v1_send_fatal("PROTOCOL_ERROR", "JSON inválido"); return;
    }
    /* V6/U3 - EL `type` SE LEE ANTES QUE EL BULK, y el orden NO es cosmetico.
     *
     * El bulk viaja DESPUES de la linea JSON, asi que quien lo lee tiene que ser
     * uno solo: leerlo dos veces desincroniza el wire, y como el bulk es binario
     * eso no da error -- CORROMPE. Mientras esta pre-lectura ocurriera antes de
     * saber el `type`, el grupo PUT no podia migrar al comun (que lo lee el).
     *
     * El reordenado entro solo (`U3.20`, con la bandera en 0 fijo) y la
     * migracion detras (`U3.21`), a proposito: si algo se rompe, se sabe cual de
     * los dos fue. Desde `U3.21` la bandera YA mira el `type`. */
    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        /* ⚠️ HAY QUE TRAGARSE EL BULK ANTES DE CONTESTAR. Al subir el `type`,
         * este error paso a ocurrir ANTES de la pre-lectura -- y salir de aqui
         * sin consumir el bulk deja el wire A MEDIAS: los bytes del cuerpo se
         * leerian luego como si fueran la linea siguiente. Con un bulk BINARIO
         * eso no da error, CORROMPE. Antes del reordenado no podia pasar,
         * porque el bulk ya estaba leido al llegar aqui. */
        long b = json_get_long(&obj, "bulk", 0);
        if (b > 0) (void) bpvm_repl_drain_bulk((unsigned long) b);
        wire_v1_send_error(id, "PROTOCOL_ERROR", "falta 'type'"); return;
    }

    /* V6/U3 g12 — los verbos PUT ya viven en el común y el bulk se lee ALLÍ.
     * Pre-leerlo aquí además sería leerlo DOS VECES: la segunda se comería el
     * mensaje siguiente y el wire quedaría desincronizado (el bug de 0456da8).
     * Lo que SÍ se sigue pre-leyendo es `PACK_BURN_DATA` y compañía: el gestor
     * de placa recibe el bulk ya en `s_put_buf` y ese camino no ha migrado. */
    const int lo_lee_el_comun = (strcmp(type, "PUT") == 0 || strncmp(type, "PUT_", 4) == 0);
    long bulk = lo_lee_el_comun ? 0 : json_get_long(&obj, "bulk", 0);
    size_t bulk_size = 0;
    if (bulk > 0) {
        if (bulk > (long) sizeof(s_put_buf)) {
            static uint8_t drain[64];
            long remaining = bulk;
            while (remaining > 0) {
                size_t chunk = (size_t)(remaining > (long) sizeof(drain) ? (long) sizeof(drain) : remaining);
                if (wire_v1_recv_bulk(drain, chunk, sizeof(drain)) < 0) break;
                remaining -= (long) chunk;
            }
            /* #329 — este NO_SPACE no tiene NADA que ver con el FS (el volumen
             * puede estar vacío): el bulk no cabe en s_put_buf. Como el IDE sólo
             * ve el código, era indistinguible de "FS lleno" y mandaba a mirar
             * el sitio equivocado. Dejamos las cifras en el log y el mensaje
             * dice de qué buffer habla. */
            log_printf("wire: bulk RECHAZADO %ld B > buffer %u B ('%s') — NO es el FS",
                       bulk, (unsigned) sizeof(s_put_buf), type);
            log_flush();
            {
                char m[96];
                snprintf(m, sizeof m, "bulk %ld B supera el buffer del servidor (%u B)",
                         bulk, (unsigned) sizeof(s_put_buf));
                wire_v1_send_error(id, "BULK_TOO_BIG", m);
            }
            return;
        }
        if (wire_v1_recv_bulk(s_put_buf, (size_t) bulk, sizeof(s_put_buf)) < 0) {
            wire_v1_send_fatal("PROTOCOL_ERROR", "lectura de bulk truncada"); return;
        }
        bulk_size = (size_t) bulk;
    }

    if (strcmp(type, "RESET") == 0) { handle_reset(id, &obj); return; }
    /* H9 — gestión de placa (env + particiones): mismo núcleo que boardsim/Pico. */
    if (strcmp(type, "STATE") == 0
        || strncmp(type, "ENV_", 4) == 0
        || strncmp(type, "PART_", 5) == 0
        /* #327 — PACK_* también. La capacidad ya estaba en el núcleo compartido
         * (bpvm_bmgr_wire atiende PACK_LS y compañía) y este firmware YA lo
         * enlazaba: lo único que faltaba era encaminarle los comandos, así que
         * el panel de packs del IDE se comía un UNSUPPORTED. Sólo el STM32 lo
         * hacía. */
        || strncmp(type, "PACK_", 5) == 0) {
        /* BUG MÍO de 0456da8, cazado al portar esto a la Pico: aquí había una
         * SEGUNDA lectura del bulk (a un s_burn_chunk propio). Pero el
         * despachador YA lo ha leído arriba, en s_put_buf — así que la segunda
         * se comía los bytes del MENSAJE SIGUIENTE y desincronizaba el wire.
         * No saltó porque sólo se probó PACK_LS, que no lleva bulk.
         * El bulk ya está: se PASA, no se relee. Y el s_burn_chunk sobra (4 KB
         * de .bss menos). */
        /* El scratch del gestor y el bulk COMPARTEN s_put_buf: el bulk ocupa el
         * principio y el gestor necesita 3 sectores + 512 al final. Con bulk
         * (≤4 KB de PACK_BURN_DATA) y s_put_buf de 20 KB no se solapan, pero el
         * gestor recibe el tramo de DETRÁS del bulk para que no puedan pisarse. */
        {
            size_t used = bulk_size;
            if (used > sizeof s_put_buf) used = sizeof s_put_buf;   /* nunca pasa */
            board_mgr_esp32_handle(id, &obj, type,
                                   s_put_buf + used, sizeof s_put_buf - used,
                                   bulk_size ? s_put_buf : NULL,
                                   (unsigned long) bulk_size);
        }
        return;
    }
    /* H9 — gating por estado REAL del boot: sin FS (estado<2) los comandos de
     * fichero → NOT_READY; RUN necesita la VM (estado 3). "Sin partición, nada
     * con el sistema de ficheros" (Eduardo). */
    {
        const bpvm_boot_status_t* bs = board_boot_status();
        int is_fs = strcmp(type, "LIST") == 0 || strcmp(type, "LIST_DIR") == 0
                 || strcmp(type, "STAT") == 0
                 || strcmp(type, "GET")  == 0 || strcmp(type, "PUT")  == 0
                 || strncmp(type, "PUT_", 4) == 0   /* #294 streaming: PUT_BEGIN/DATA/END */
                 || strcmp(type, "DEL")  == 0
                 || strcmp(type, "SAVE") == 0 || strcmp(type, "DF") == 0;
        if (is_fs && bs->state < BPVM_BOOT_FS) {
            /* El bulk que esta puerta rechaza HAY QUE TRAGÁRSELO igual, o se
             * queda en el cable y el mensaje siguiente se lee a partir de la
             * mitad de los datos. Sólo el de los verbos PUT: el del resto ya lo
             * pre-leyó el paso de arriba. */
            if (lo_lee_el_comun) {
                long b_pend = json_get_long(&obj, "bulk", 0);
                if (b_pend > 0) (void) bpvm_repl_drain_bulk((unsigned long) b_pend);
            }
            wire_v1_send_error(id, "NOT_READY", "FS no disponible: configurar particiones");
            return;
        }
        if (strcmp(type, "RUN") == 0 && bs->state < BPVM_BOOT_APP) {
            wire_v1_send_error(id, "NOT_READY", "VM no disponible en el estado actual del boot");
            return;
        }
    }
    /* El log NO se gatea por el estado del boot: si el arranque se ha quedado a
     * medias es justo cuando hace falta leerlo (vive en RAM + bpenv, no en el FS). */
    if (strcmp(type, "RUN")   == 0) { handle_run(id, &obj);   return; }
    /* #326 DEPURACIÓN pre-RUN: acumular breakpoints / pedir pausa inicial. Los
     * demás (CONTINUE, STEP, LOCALS, STACK, READ_*) los atiende el bucle de
     * pausa del núcleo INLINE, mientras la VM está detenida. */
    {
        bpvm_dbg_cmd_t c;
        dbgw_cmd_from_json(&c, id, &obj, type);
        if (c.kind != BPVM_DBGC_OTHER && bpvm_dbg_wire_handle(&s_dbgw, &c)) return;
    }
    /* PROMPT_RESPONSE huérfano (IO.prompt() aún no emite PROMPT_REQUEST en la
     * VM-C): ack silente, como en el Pico — que el IDE no se quede esperando. */
    if (strcmp(type, "PROMPT_RESPONSE") == 0) {
        wire_v1_send_reply_empty("PROMPT_RESPONSE_REPLY", id);
        return;
    }
    /* P-run-stop (#257) — KILL en idle: nada que matar (el KILL útil llega
     * DURANTE un RUN y lo atiende esp32_run_poll_cb). */
    if (strcmp(type, "KILL")  == 0) {
        wire_v1_send_error(id, "NO_SESSION", "no hay programa en ejecución");
        return;
    }

    /* V6/U3 — el común, DESPUÉS de los propios y no antes (igual que en la Pico)
     * porque esta familia se migra por grupos: lo que esta placa siga
     * implementando gana, y lo que ya no, cae aquí. Al terminar la migración
     * este `if` sube al principio y la cadena de arriba desaparece. */
    if (bpvm_repl_dispatch(type, id, &obj)) return;

    /* El mensaje DICE CUÁL. El de antes ("type no implementado") obligaba a
     * adivinar qué comando había mandado el IDE — costó una vuelta entera de
     * diagnóstico. Si el wire crece y una familia se queda atrás, que el error
     * nombre al culpable. */
    {
        char msg[96];
        snprintf(msg, sizeof msg, "comando '%s' no implementado en el firmware ESP32", type);
        wire_v1_send_error(id, "UNSUPPORTED", msg);
    }
}

void repl_esp32_run(void) {
    /* #329 — igual que el Pico ("REPL entry (wire v1)"): marca la frontera entre
     * el arranque y el diálogo con el IDE. Si el log se corta antes de esta
     * línea, el problema es de boot; si aparece, la placa estaba escuchando y lo
     * que venga después es del comando. También deja a la vista el tamaño del
     * buffer de bulk, que es distinto por placa (S3 48K / P4 64K). */
    s_repl_ops.server_name = s_board_id->server_name;   /* S3 o P4: lo dice la placa */
    bpvm_repl_set_ops(&s_repl_ops);   /* V6/U3: la cintura, antes del primer mensaje */
    log_printf("REPL entry (wire v1) — buffer de bulk %u B", (unsigned) sizeof(s_put_buf));
    log_flush();
    for (;;) {
        int n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
        if (n < 0) { wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX"); continue; }
        if (n == 0) continue;
        if (s_line_buf[0] != '{') continue;   /* ignora ruido no-v1 */
        handle_request(s_line_buf, n);
    }
}
