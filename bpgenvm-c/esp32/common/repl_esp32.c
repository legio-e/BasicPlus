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
#include "bpvm_repl.h"
#include "bpvm_env.h"          /* V7/H10: gc y latido, del ENV, para el RUN común */     /* V6/U3: el REPL comun + la cintura de familia */

#include "bpvm.h"
#include "bpvm_internal.h"   /* recorrido de vm->modules[] para los .mdn del AOT */
#include "bpvm_entry.h"      /* #344 — el RUN, escrito una vez */
#include "bpvm_rtc.h"        /* H14 — TIME del wire → RTC (bpvm_rtc_set_now_ms) */
#include "bpvm_machine.h"       /* paso 4 cierre — bpvm_machine_reset_cause (INFO) */
#include "crc32.h"           /* paso 4 cierre — CRC por fichero en el LS */
#include "board_mgr_esp32.h" /* H9: gestión de placa (STATE/ENV/PART) + board_boot_status */
#include "log.h"             /* log persistente (post-mortem) → LOG_DUMP / LOG_CLEAR */
#include "bpvm_dbg_wire.h"   /* #326: ramo de depuración, núcleo portable compartido */
#include "bpvm_io.h"         /* V6/A1.2: el segundo hilo, comun a todas las familias */
#include "aot_registry.h"    /* H4 AOT: bpvm_aot_clear/count (hook esp_aot_register) */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "afinidad_esp32.h"  /* V7/H12: el núcleo de `run` y `run2` (`vm.cores=1`) */
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
/* #503 (12-sep, F1) — LA GUARDA DE ARRIBA ESTUVO MUERTA DEL 31-AGO AL 12-SEP.
 * Decia `#if BPVM_ESP_AOT_MDN && defined(__has_include)`: como nadie define
 * BPVM_ESP_AOT_MDN antes de llegar aqui, el preprocesador lo evalua a 0, el
 * `__has_include` no se pregunta nunca y el `#ifndef` de abajo lo deja en 0 —
 * en el P4 tambien. Consecuencia: `bpvm_mdn_escanear` no se enlazaba en NINGUNA
 * imagen ESP32 (el .map del P4 no lo tenia), el puente `.mdn` del pack de SQLite
 * no se cargaba y `SqlDemo` moria con «falta el codigo nativo del pack 'SQLI'»
 * — una funcion de V5 rota trece dias sin que nada lo dijera, porque Bench
 * interpretado imprime los mismos numeros y la BD no se volvio a probar en el
 * P4 hasta las pruebas finales de V6. Lo cazo F1.
 *
 * La condicion que se queria escribir es por CAPACIDAD (#465), y son DOS:
 *   (a) la API de cache (esp_cache.h): sin ella no hay RAM ejecutable coherente;
 *   (b) que la ABI de coma flotante del firmware sea la que el IDE emite para
 *       RISC-V (`ilp32f`, destino riscv32-esp-p4). El C3 y el C6 son `ilp32`:
 *       cargarian un blob del P4 y fallarian al tocar la FPU (#502). Hasta que
 *       tengan su destino, aqui NO se carga nada y sus `native` van interpretadas
 *       — que es lo que la documentacion de V6 dice de ellos. */
#if !defined(BPVM_ESP_AOT_MDN) && defined(__has_include)
#  if __has_include("esp_cache.h") && defined(__riscv_float_abi_single)
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
#include "pack_esp32.h"      /* V7/H8 #602: el bloque de RAM de los motores, para el INFO */
#include "esp_ota_ops.h"     /* V7/H9: la partición de la app que corre (el sello de la imagen) */
#include "esp_image_format.h"/* V7/H9: su longitud real */
#include "crc32.h"           /* V7/H9: el sello de la imagen */

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
/* V7/H8 #495 — el búfer de FICHEROS del PUT troceado, aparte del de comunicaciones: un sector de la flash SPI
 * (4 KB), el bloque de littlefs en esta familia (ESP_FS_BLOCK_SIZE, fs_lfs_esp32.c). */
static uint8_t s_fich_buf[4096];
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
    /* V7/H7 (Boot 2, fase 2): la guarda y el log, en el cargador común. */
    if (s_packs_loader) (void) s_packs_loader();
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

/* ====================== TERMINAL (RUN) ======================
 * El RUN es el común (V7/H10: `bpvm_repl_run`, en src/bpvm_repl.c), con la cintura de abajo. El de esta familia —su
 * sink, su EXITED y su poll— se borró en el paso 6 (3-oct), probado ya en placa. */

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

    char     real[BPVM_FS_PATH_MAX];   /* #456 - lo CONSTRUYE el resolvedor */
    uint32_t size = 0;
    if (bpvm_entry_resolve(nombre, real, sizeof real, &size) != 0) return NULL;   /* #344: la regla común */

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
#endif

/* V6/A1.2 — la cola de salida de `io`, 2 KB. No es un búfer de salida, es un amortiguador: cuando se llena, `vm`
 * espera a que `io` drene (contrapresión), que es lo correcto —un programa no puede producir más deprisa de lo que el
 * transporte traga, y fingir lo contrario sería comerse la RAM del micro—. */
#define ESP32_IO_OQ_BYTES 2048

/* ── V7/H10, el RUN común, paso 5 (3-oct): LA CINTURA DE LAS ESP32 ────────────────────────────────────────────────
 * Lo que es suyo de verdad: el bloque de la VM, el cable leído sin esperar (UART o USB-Serial-JTAG), sus búferes (uno
 * por rol, A1.7), el reinicio, los motores nativos, el `.mdn` del FS a RAM EJECUTABLE (con su coherencia de cachés),
 * `esp_aot_register` tras cargar —en su fichero, por el enlazador: en el P4 es el fuerte, en las demás el weak— y,
 * después, devolver esa RAM y la línea `mem:` (la marca de agua de la DRAM interna, #336). */
static int esp32_run_memoria(uint8_t** base, size_t* bytes) {
    if (!s_vm_buffer || s_vm_buffer_size == 0) return -1;
    *base = s_vm_buffer;
    *bytes = (size_t) s_vm_buffer_size;
    return 0;
}
static int esp32_run_leer_linea(char* buf, size_t cap) {
    int c = wire_v1_try_getchar();
    if (c < 0) return 0;
    int n = wire_v1_recv_line(c, buf, cap);
    if (n == -2) bpvm_repl_linea_estancada();   /* V8 #693: se dice (y el RUN sigue: su 0) */
    return (n < 0) ? 0 : n;                     /* rota: se descarta */
}
static void esp32_run_reiniciar(long id) { handle_reset(id, NULL); }
static void esp32_run_tras_cargar(bpvm_t* vm) {
    esp_aot_register(vm);
#if BPVM_ESP_AOT_MDN
    s_mdn_exec_n = 0;                           /* la RAM ejecutable de los .mdn de ESTE RUN */
#endif
    /* #681 (10-oct) — SÓLO PRUEBAS, el gemelo del de la Pico (V7/H12, etapa 3.1, paso 4): `prueba.fallo=1` en el ENV
     * provoca aquí un PÁNICO (abort), para probar en la placa el camino fatal —el reinicio por pánico y el arranque
     * siguiente sin autorun—. Con la clave puesta, CADA RUN falla: se quita después. */
    if (bpvm_env_get_bool(board_mgr_env(), "prueba.fallo", 0)) {
        log_printf("prueba.fallo=1: un panico a proposito (SOLO PRUEBAS)");
        log_flush();
        abort();
    }
}
static void esp32_run_despues(bpvm_t* vm) {
    (void) vm;
#if BPVM_ESP_AOT_MDN
    for (int k = 0; k < s_mdn_exec_n; k++) heap_caps_free(s_mdn_exec[k]);
    s_mdn_exec_n = 0;
#endif
    log_printf("mem: DRAM interna libre %u B | MINIMO HISTORICO %u B (bloque mayor %u B)",
               (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned) heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    log_flush();
}
static int esp32_run_env_bool(const char* clave, int defecto) {
    return bpvm_env_get_bool(board_mgr_env(), clave, defecto);
}

static const bpvm_repl_run_ops_t s_run_ops = {
    .memoria           = esp32_run_memoria,
    .leer_linea        = esp32_run_leer_linea,
    .linea             = s_line_buf,  .linea_cap     = sizeof s_line_buf,
    .dbg_linea         = s_dbg_buf,   .dbg_linea_cap = sizeof s_dbg_buf,
    .mensaje           = s_reply_buf, .mensaje_cap   = sizeof s_reply_buf,
    .reiniciar         = esp32_run_reiniciar,
    .cargar_motores    = packs_cargar_una_vez,
#if BPVM_ESP_AOT_MDN
    .mdn_del_fs        = esp32_mdn_del_fs,
#endif
    .tras_cargar       = esp32_run_tras_cargar,
    .despues_de_correr = esp32_run_despues,
    .env_bool          = esp32_run_env_bool,
    .io_cola           = ESP32_IO_OQ_BYTES,
};
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

static void esp32_autorun_espera(int ms, void* user) {
    (void) user; vTaskDelay(pdMS_TO_TICKS(ms));
}

static uint32_t esp32_autorun_ahora(void* user) {
    (void) user; return (uint32_t) (esp_timer_get_time() / 1000);
}

static const bpvm_autorun_wire_t s_autorun_wire = {
    esp32_autorun_anuncia, bpvm_repl_autorun_escucha,   /* V7/H10, el RUN común, paso 6: la escucha, la común */
    esp32_autorun_espera,  esp32_autorun_ahora, NULL
};

void repl_esp32_autorun(void) {
    /* #345 — leer y limpiar la primera línea lo hace el núcleo. (H11 sigue
     * valiendo: sólo la CABEZA del fichero, nunca el espejo de 64 KB.) */
    char path[BPVM_FS_PATH_MAX];   /* #456 - este se CONSTRUYE (sale del fichero) */
    if (!bpvm_autorun_entry(path, sizeof path)) return;   /* sin autorun */

    /* #681 (10-oct) — LO DE LA PICO, EN LA ESP32 (V7/H12, etapa 3.1, paso 4; la decisión de Eduardo para los fallos
     * fatales: «el log y reset», y el arranque siguiente SIN autorun). Un programa de autorun que falla al arrancar
     * dejaba la placa reiniciándose en bucle. Aquí la marca la da el propio chip: el motivo del reinicio, el mismo que
     * enseña el INFO. Cuentan el PÁNICO (una excepción, un abort, un assert) y el perro de INTERRUPCIONES (el sistema
     * colgado con las interrupciones cortadas); NO el perro de tareas, que en la ESP32 es el del módulo `Wdt` del
     * programa —cuando ése salta, relanzar el autorun es justo lo que el usuario quiere—. El IDE lo arranca a mano. */
    esp_reset_reason_t motivo = esp_reset_reason();
    if (motivo == ESP_RST_PANIC || motivo == ESP_RST_INT_WDT) {
        log_printf("autorun: %s NO arranca: el arranque anterior acabo en un fallo fatal (%s; ver el log)",
                   path, motivo == ESP_RST_PANIC ? "panic/exception" : "int-watchdog");
        printf("[autorun] %s NO arranca: el arranque anterior acabo en un fallo fatal\n", path);
        return;
    }

    /* #345 paso 2 — la ventana de rescate. Decide el usuario. */
    if (!bpvm_autorun_gate(&s_autorun_wire, path, 500, 10000)) {
        log_printf("autorun: CANCELADO por el usuario (Stop) — REPL normal");
        printf("[autorun] CANCELADO por el usuario — REPL normal\n");
        return;
    }
    bpvm_repl_run(-1, path, NULL);   /* V7/H10: el RUN común; sin argumento, manda el defecto */
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
    /* V7/H6 (pregunta 6 de V7_IDEAS §H6) — el INFO publica screenW/H sólo si hay
     * render, y con la GUI por el pack eso exige el pack CARGADO: la tanda y el
     * IDE preguntan el tamaño de pantalla ANTES del primer Run. Así que el INFO
     * también dispara la carga «una vez» (reintentable: si no hay pack no marca
     * nada). La política de no cargar en el ARRANQUE no cambia: esto ya es el
     * host hablando, y un cuelgue aquí no se repite en cada reset. */
    packs_cargar_una_vez();
    /* V7/H8 #602 — el bloque de RAM de los motores (pack_esp32_ram_init), al INFO común. */
    o->pack_ram_base   = (unsigned long) (uintptr_t) s_pack_ram_base;
    o->pack_ram_bytes  = (unsigned long) s_pack_ram_size;
    o->pack_ram_res    = s_pack_ram_res;
    o->pack_ram_clave  = s_pack_ram_clave;
    o->pack_ram_pedido = s_pack_ram_pedido;
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
    /* V6 (9-sep) — POR LA FACHADA, no por la tabla de aqui. `board_name` del wire
     * es el MICRO (el IDE lo rotula «Micro»), y quien lo sabe es la cintura, que
     * ya lo publica en HAL BP. Antes salia de `s_board_id`, una SEGUNDA fuente
     * rellenada a mano por familia — y por eso divergio: el programa BP leia la
     * fachada («esp32s3-devkitc» para S3, C3 y C6) y el wire leia esto. Una sola
     * funcion y dos lectores, que es lo que la Pico ya hacia (`repl_v1.c`). */
    static char micro[24];
    bpvm_machine_micro_name(micro, sizeof micro);
    o->board_name    = micro;
    o->reset_reason  = bpvm_machine_reset_cause();
    o->arch          = (unsigned) bpvm_mdn_host_arch();
    o->cpu_hz        = (unsigned long) bpvm_machine_cpu_freq_hz();   /* V8 #784: la de verdad, la de Machine.cpuFreqHz() */
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

static int  esp32_repl_fs_format(void) { return fs_format_ram(); }   /* #473 (466): lo que pasó */
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
/* V7/H9 (2-oct) — el sello de la imagen: el CRC32 de la imagen de la app en su partición, de 0 a su longitud real
 * (la cabecera, los segmentos y el hash del final: el mismo .bin que deja `idf.py build`, así que el PC puede calcular el
 * mismo número). Por trozos, con un búfer estático: se llama una vez, la primera que se pide. */
static uint32_t esp32_image_crc(void) {
    static uint32_t s_crc = 0;
    static int s_hecho = 0;
    static uint8_t trozo[512];
    if (s_hecho) return s_crc;
    s_hecho = 1;
    const esp_partition_t* p = esp_ota_get_running_partition();
    if (!p) return 0;
    esp_image_metadata_t md;
    const esp_partition_pos_t pos = { .offset = p->address, .size = p->size };
    if (esp_image_get_metadata(&pos, &md) != ESP_OK) return 0;
    uint32_t st = BPVM_CRC32_INIT;
    for (uint32_t off = 0; off < md.image_len; off += (uint32_t) sizeof trozo) {
        uint32_t n = md.image_len - off;
        if (n > (uint32_t) sizeof trozo) n = (uint32_t) sizeof trozo;
        if (esp_partition_read(p, off, trozo, n) != ESP_OK) return 0;
        st = bpvm_crc32_update(st, trozo, n);
    }
    s_crc = bpvm_crc32_final(st);
    return s_crc;
}

/* V7/H12 (4-oct) — lo que el INFO común no sabe de la ESP32, para dimensionar las pilas de H12 con medidas y no a ojo:
 * la marca de la pila de la tarea del REPL (`main`, o `wire_uart` en el P4) —con `run`, la que carga los programas y
 * hace de `io`; el INFO lo atiende ella misma, así que la marca es la suya; en el IDF, en BYTES—, con el nombre de la de
 * la Pico (vmTaskStackFreeBytes); y lo que quedó libre de RAM INTERNA en el peor momento desde el arranque
 * (rtosHeapMinFreeBytes: de ahí salen las pilas de las tareas y todo lo que no va a la PSRAM; en la C6, lo que manda). */
static long run2_pila_libre(void);   /* V7/H12, 2b: la marca de la pila del 2º run; -1 sin él (abajo, con la tarea) */
static int esp32_repl_info_extra(char* buf, unsigned long buf_max, int off) {
    size_t max = (size_t) buf_max;
    off = wire_v1_field_long(buf, max, (size_t) off, "vmTaskStackFreeBytes", (long) uxTaskGetStackHighWaterMark(NULL));
    if (off < 0) return off;
    off = wire_v1_field_long(buf, max, (size_t) off, "rtosHeapMinFreeBytes",
                             (long) heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    if (off < 0) return off;
    /* V7/H12, 2b — la del 2º run, con el nombre de la Pico (`runStackFree`, la común, es la del run que cerró el último
     * programa) */
    long r2 = run2_pila_libre();
    if (r2 >= 0) off = wire_v1_field_long(buf, max, (size_t) off, "run2StackFreeBytes", r2);
    return off;
}

static bpvm_repl_ops_t s_repl_ops = {
    .run            = &s_run_ops,     /* V7/H10: la cintura del RUN común */
    .info           = esp32_repl_info,
    .info_extra     = esp32_repl_info_extra,
    .image_crc      = esp32_image_crc,
    .server_name    = "bpvm-esp32",   /* se pisa en repl_esp32_run con el real */
    .server_build   = ESP32_BUILD_DATE,
    /* 30-sep (del mapa de H7, «encontrado de paso»): el depurador (`bpvm_dbg_wire`, como la Pico) y los packs (los
     * PACK_* del común, `bpvm_bmgr_wire.c`) los tenía y no los decía. */
    .capabilities   = "[\"META\",\"FILES\",\"TERMINAL\",\"DEBUG\",\"PACKS\"]",
    .fich_buf       = s_fich_buf,     /* V7/H8 #495: un sector de la flash */
    .fich_bytes     = sizeof s_fich_buf,
    .put_trozo      = 512,            /* V7/H8 #495: 4 trozos de 512 B = 2 KB: caben en el anillo de la UART
                                       * (8 KB) y en el del USB-Serial-JTAG (4 KB) (Eduardo, 1-oct) */
    .put_ventana    = 4,
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
    bpvm_repl_antes_de_despachar(type);   /* #495: un verbo que no convive con una subida a medias, la acaba */

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
    /* V7/H10 #473 (punto 10): la puerta, en el común. El bulk que rechaza sólo sigue en el cable en los verbos PUT:
     * el del resto ya lo pre-leyó el paso de arriba. */
    if (bpvm_repl_puerta_arranque(type, id, &obj, board_boot_status(), lo_lee_el_comun)) return;   /* #691: con su motivo */
    /* El log NO se gatea por el estado del boot: si el arranque se ha quedado a
     * medias es justo cuando hace falta leerlo (vive en RAM + bpenv, no en el FS). */
    /* TERMINAL y DEBUG — el RUN, el depurador de antes del RUN (PAUSE, SET_BP, CLR_BP) y el KILL sin programa los
     * atiende el común (bpvm_repl_dispatch; V7/H10, el RUN común, paso 6): durante un RUN, todo lo del cable lo lee
     * el poll común. */
    /* PROMPT_RESPONSE huérfano (IO.prompt() aún no emite PROMPT_REQUEST en la
     * VM-C): ack silente, como en el Pico — que el IDE no se quede esperando. */
    if (strcmp(type, "PROMPT_RESPONSE") == 0) {
        wire_v1_send_reply_empty("PROMPT_RESPONSE_REPLY", id);
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

/* V7/H7 (Boot 2, fase 4) — la cintura, la registra el arranque común ANTES del cable y
 * del autorun. Se registraba al entrar al REPL —o sea DESPUÉS del autorun—, y un IDE que
 * se conectaba con el autorun corriendo recibía «HELLO: este firmware no ha registrado su
 * cintura de REPL» al saludar. La identidad ya está puesta (va antes). */
/* V7/H12, etapas 0 y 1 (4-oct) — LA TAREA `run` (src/bpvm_repl.c): el programa en una tarea propia y permanente, y la
 * del REPL (`main`, o `wire_uart` en el P4) hace de `io` durante el RUN. El interruptor de la imagen,
 * BPVM_ESP32_RUN_TAREA: encendida en TODA la familia, después de pasar su puerta el P4 (Eduardo: «si esto funciona...
 * el cambio se queda para todos los micros»); con 0 en el CMakeLists de una placa, el RUN de antes. */
#ifndef BPVM_ESP32_RUN_TAREA
#define BPVM_ESP32_RUN_TAREA 1
#endif
#if BPVM_ESP32_RUN_TAREA
/* Su pila, ESTÁTICA y en la RAM INTERNA (el .bss va a la DRAM): una pila en la PSRAM no puede escribir la flash —por lo
 * mismo, la del ejecutor de las natives—. En el IDF, el tamaño de una pila va en BYTES (StackType_t es un byte). 16 KB
 * por defecto, como la de la Pico; la C3 y la C6 —sin PSRAM, justas de RAM interna (H13), y sin SQLite— con 12, desde
 * su CMakeLists: en `run`, el peor caso medido en el P4 (con la GUI, y aún con el GC recursivo de #648) fue ~6,5 KB. */
#ifndef ESP32_RUN_PILA_BYTES
#define ESP32_RUN_PILA_BYTES (16u * 1024u)
#endif
static StackType_t  s_run_pila[ESP32_RUN_PILA_BYTES / sizeof(StackType_t)];
static StaticTask_t s_run_tcb;
static TaskHandle_t s_run_task = NULL;
/* V7/H12, 2b — EL 2º RUN, con `vm.runs=2` en el ENV: el run número 1, a la misma prioridad, en la misma cola. Su pila,
 * como la del primero, de la RAM INTERNA, pero del heap y sólo con dos runs: la C3 y la C6 van justas de RAM interna
 * (H13), y con un run no la pagan. */
static StackType_t* s_run2_pila = NULL;
static StaticTask_t s_run2_tcb;
static TaskHandle_t s_run2_task = NULL;
#endif

static long run2_pila_libre(void) {
#if BPVM_ESP32_RUN_TAREA
    if (s_run2_task != NULL) return (long) uxTaskGetStackHighWaterMark(s_run2_task);   /* en el IDF, BYTES */
#endif
    return -1;
}

void repl_esp32_cintura(void) {
    s_repl_ops.server_name = s_board_id->server_name;   /* S3 o P4: lo dice la placa */
    bpvm_repl_set_ops(&s_repl_ops);   /* V6/U3: la cintura, antes del primer mensaje */
#if ( configNUMBER_OF_CORES > 1 )
    /* V7/H12 (6-oct) — la línea de la Pico: la imagen y, con `vm.cores=1`, la vuelta atrás sin grabar. */
    log_printf("boot: imagen de DOS nucleos%s", bpvm_cores() == 1 ? "; vm.cores=1: todas las tareas, en el nucleo 0" : "");
#endif
#if BPVM_ESP32_RUN_TAREA
    /* A la MISMA prioridad que ésta, la que hace de `io` (el contrato de bpvm_platform.h: ni por debajo —el KILL no
     * llegaba— ni por encima —PrintBench, más lento—): en el P4, 5. Antes del autorun: el arranque común llama aquí
     * antes. Sin afinidad: con los dos núcleos (la S3 y la P4, desde el 6-oct) los reparte el planificador, como en la
     * Pico; con `vm.cores=1`, en el 0 (afinidad_esp32.h). En las de un núcleo, da igual. */
    if (s_run_task == NULL && bpvm_repl_run_tarea_prepara() == 0)
        s_run_task = xTaskCreateStaticPinnedToCore(bpvm_repl_run_tarea, "run", ESP32_RUN_PILA_BYTES, NULL,
                                                   uxTaskPriorityGet(NULL), s_run_pila, &s_run_tcb, esp32_afinidad());
    /* el log, desde aquí y no desde `run` (el log no tiene cerrojo) */
    if (s_run_task == NULL) {
        bpvm_repl_run_tarea_no_nacio();   /* V7/H12, etapa 2a.2: que el RUN no cuelgue programas para un run que no hay */
        log_printf("run: no se pudo crear la tarea run: sigue el RUN de antes");
    } else {
        log_printf("run: la tarea del programa, creada (%u KB de pila, prioridad %u; %s)",
                   (unsigned) (ESP32_RUN_PILA_BYTES / 1024u), (unsigned) uxTaskPriorityGet(NULL),
                   bpvm_repl_run_en_cola() ? "espera en la cola" : "espera el aviso «trabajo», como en la etapa 1");
    }
    /* V7/H12, 2b — el 2º run: sólo con la cola (con el relevo de la etapa 1, `prepara` no admite otro) y con su pila.
     * Sin él, la placa va con uno y lo dice. */
    if (s_run_task != NULL && s_run2_task == NULL && bpvm_runs() >= 2) {
        const char* porque = NULL;
        if (!bpvm_repl_run_en_cola()) {
            porque = "sin la cola";
        } else {
            s_run2_pila = (StackType_t*) heap_caps_malloc(ESP32_RUN_PILA_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (s_run2_pila == NULL) {
                porque = "sin RAM interna para su pila";
            } else if (bpvm_repl_run_tarea_prepara() != 0) {
                porque = "la cola no admite otro";
            } else {
                s_run2_task = xTaskCreateStaticPinnedToCore(bpvm_repl_run_tarea, "run2", ESP32_RUN_PILA_BYTES,
                                                            (void*) (intptr_t) 1, uxTaskPriorityGet(NULL), s_run2_pila,
                                                            &s_run2_tcb, esp32_afinidad());
                if (s_run2_task == NULL) {
                    bpvm_repl_run_tarea_no_nacio();
                    porque = "la tarea no nacio";
                }
            }
            if (s_run2_task == NULL && s_run2_pila != NULL) {
                heap_caps_free(s_run2_pila);
                s_run2_pila = NULL;
            }
        }
        if (s_run2_task != NULL)
            log_printf("run: el 2o run, creado (vm.runs=%d; %u KB de pila, RAM interna): dos runs en la cola",
                       bpvm_runs(), (unsigned) (ESP32_RUN_PILA_BYTES / 1024u));
        else
            log_printf("run: vm.runs=%d, pero el 2o run no se pudo crear (%s): va con uno", bpvm_runs(), porque);
    }
#endif
}

void repl_esp32_run(void) {
    /* #329 — igual que el Pico ("REPL entry (wire v1)"): marca la frontera entre
     * el arranque y el diálogo con el IDE. Si el log se corta antes de esta
     * línea, el problema es de boot; si aparece, la placa estaba escuchando y lo
     * que venga después es del comando. También deja a la vista el tamaño del
     * buffer de bulk, que es distinto por placa (S3 48K / P4 64K). */
    log_printf("REPL entry (wire v1) — buffer de bulk %u B", (unsigned) sizeof(s_put_buf));
    log_flush();
    for (;;) {
        int n;
        if (bpvm_repl_hay_transferencia()) {
            /* #495 (paso 4) — una transferencia a medias: el cable se mira SIN esperar y, si no hay nada, el trozo
             * siguiente. Sin ella, la espera de siempre (cede la CPU hasta que llega algo). */
            int c = wire_v1_try_getchar();
            if (c < 0) { (void) bpvm_repl_paso(); continue; }
            n = wire_v1_recv_line(c, s_line_buf, sizeof(s_line_buf));
        } else {
            n = wire_v1_recv_line(-1, s_line_buf, sizeof(s_line_buf));
        }
        /* V6/#473 — -2 no es -1: la linea se estanco (truncado/ruido). Descartar y
         * seguir; el IDE reintenta. V8 #693: y ya no en silencio. */
        if (n == -2) { bpvm_repl_linea_estancada(); continue; }
        if (n < 0) { wire_v1_send_fatal("PROTOCOL_ERROR", "línea excede WIRE_V1_LINE_MAX"); continue; }
        if (n == 0) continue;
        if (s_line_buf[0] != '{') continue;   /* ignora ruido no-v1 */
        handle_request(s_line_buf, n);
    }
}
