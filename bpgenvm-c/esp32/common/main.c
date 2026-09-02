/*
 * main.c — H4.3: firmware de la familia ESP32 con REPL wire v1.
 *
 * V6/P1.C3.2 — COMPARTIDO por los proyectos de la familia (S3 y C3 hoy). Lo que
 * cambia de un silicio a otro son TRES cosas —el nombre y los dos tamaños del
 * heap de la VM— y viven en el `chip_cfg.h` del componente `main/` de cada uno.
 * Antes esto era un fichero por chip, copiado: el C3 se diferenciaba en 176
 * lineas de las cuales 17 eran reales, y los arreglos no viajaban (`#464` y
 * `#465` se hicieron en uno y no en el otro).
 *
 * Arranca el FS (RAM), el wire v1 sobre UART0 y entra al bucle del REPL.
 * El IDE (SerialBackend) conecta al UART0 (puerto del bridge USB-UART),
 * sube .mod (PUT), los ejecuta (RUN) y recibe la salida (OUTPUT/EXITED).
 *
 * Canales (ver esp32/README.md):
 *   - UART0 (bridge CP210x/CH340)  → wire v1, lo usa el IDE.
 *   - USB-Serial-JTAG (puerto nativo) → consola/logs ESP-IDF (printf),
 *     para depurar con `idf.py monitor` SIN contaminar el wire.
 */
#include <stdio.h>
#include <stdint.h>

#include "fs.h"
#include "bpvm.h"             /* #353: bpvm_diag_set_sink */
#include "repl_esp32.h"
#include "hw_esp32.h"
#include "esp32_mods.h"
#include "board_mgr_esp32.h"   /* H9: arranque escalonado + estado del boot */
#include "log.h"               /* log persistente (post-mortem) — lo antes posible */
#include "esp_heap_caps.h"     /* H11: el heap de la VM se pide en runtime, no es .bss */

/* Buffer caller-provided de la VM. repl_esp32.c lo referencia como extern
 * (PUNTERO, igual convención que repl_v1.c en la Pico/Metro y que el P4).
 *
 * H11 — deja de ser 128 KB de .bss. Era un array estático, o sea 128 KB de
 * dram0_0_seg RESERVADOS a piñón en el enlace, se usara la VM o no. En el S3 eso
 * no es teoría: con 128 KB aquí + 64 KB del espejo del FS + 48 KB del PUT, el
 * enlace iba tan justo que al añadir el log se pasó del segmento por 304 bytes.
 * Ahora se pide al heap en el arranque, que es lo que ya hacía el P4 con la
 * PSRAM: mismos bytes en marcha, pero el enlazador deja de tener que encajar un
 * bloque contiguo enorme y, si no hay sitio, el boot lo DICE (se queda por
 * debajo del estado 3 con su motivo) en vez de fallar al enlazar. */
/* Cuánto pide la VM y cómo se llama el chip: lo pone el proyecto, porque son
 * las dos cosas que NO se pueden preguntar en marcha (`vm_buffer_init()` corre
 * antes que el ENV). El porqué de cada número está en el `chip_cfg.h` que lo
 * fija — y en el del C3 está escrito que AÚN NO ESTÁ MEDIDO. */
#include "chip_cfg.h"
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

static void vm_buffer_init(void) {
    /* SRAM interna a propósito: el S3 de referencia no lleva PSRAM y el heap de
     * la VM tiene que ser rápido. (El P4, que sí la tiene, va por su main.c.)
     *
     * V6/U6.7 — EL BLOQUE YA NO SE PIDE A CIEGAS. Antes: malloc(VM_BUFFER_SIZE) y
     * si fallaba, malloc(VM_BUFFER_FALLBACK); dos números por chip puestos una
     * vez y revisados nunca (el margen del S3 estaba por 3,3×, U6.4). Ahora se
     * MIDE lo que hay y se aplican las dos restricciones que salieron del C3:
     *
     *   (a) el bloque tiene que caber en el mayor hueco CONTIGUO — en el C3 hay
     *       280 KB libres en siete trozos y el mayor son 136 (P1.C3.3);
     *   (b) y tiene que dejarle al sistema lo que consume en marcha:
     *       CHIP_MARGEN_SISTEMA, medido con tools/medir_margen.ps1 (U6.4).
     *
     * El objetivo (CHIP_VM_OBJETIVO) sigue siendo el de siempre a propósito:
     * Eduardo no quiere reducir el heap del RTOS, que piensa explotar más. Lo
     * que cambia es que si la realidad es peor que el objetivo, se baja Y SE
     * DICE, con los números — en vez de fallar el malloc en silencio o, peor,
     * caber hoy y ahogar al IDF dentro de un rato. */
    multi_heap_info_t hi;
    heap_caps_get_info(&hi, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    /* La foto en BLOQUES antes de pedir nada: un solo hueco = lo ocupado está en
     * un extremo; varios = hay reservas en medio (en el C3, 37 del IDF). */
    log_printf("heap: libre %u | mayor %u | bloques: %u libres, %u usados | usado %u",
               (unsigned) hi.total_free_bytes, (unsigned) hi.largest_free_block,
               (unsigned) hi.free_blocks, (unsigned) hi.allocated_blocks,
               (unsigned) hi.total_allocated_bytes);

    unsigned libre    = (unsigned) hi.total_free_bytes;
    unsigned contiguo = (unsigned) hi.largest_free_block;
    unsigned techo_b  = (libre > CHIP_MARGEN_SISTEMA) ? libre - CHIP_MARGEN_SISTEMA : 0u;
    int      limita_a = (contiguo < techo_b);                 /* ¿qué restricción manda? */
    unsigned techo    = limita_a ? contiguo : techo_b;
    unsigned bloque   = (CHIP_VM_OBJETIVO < techo) ? CHIP_VM_OBJETIVO : techo;
    bloque &= ~1023u;                                          /* KB enteros */

    /* Escalera: si el asignador no sirve lo que sus contadores prometían
     * (cabeceras, un bloque que se ocupó entre la consulta y la petición), se
     * baja de 4 en 4 KB hasta el suelo. Misma idea que el P4 en PSRAM (baja de
     * 1 en 1 MiB): UNA manera de no caber, no una por chip. */
    s_vm_buffer = NULL; s_vm_buffer_size = 0;
    while (bloque >= CHIP_VM_MIN) {
        s_vm_buffer = (uint8_t*) heap_caps_malloc(bloque, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (s_vm_buffer) { s_vm_buffer_size = bloque; break; }
        bloque -= 4096u;
    }

    unsigned after     = (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    unsigned after_blk = (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (s_vm_buffer) {
        /* Al log y a la consola (P1.C3.3: el Pico lo dice en su banner desde
         * siempre; aquí faltaba y se notó al medir un silicio nuevo). */
        log_printf("vm: heap %u KB reservado (objetivo %u, techo %u por %s, margen %u) | DRAM libre %u->%u B (bloque mayor %u->%u B)",
                   (unsigned)(s_vm_buffer_size / 1024u), (unsigned) CHIP_VM_OBJETIVO / 1024u, techo / 1024u,
                   limita_a ? "el bloque contiguo" : "el margen del sistema",
                   (unsigned) CHIP_MARGEN_SISTEMA, libre, after, contiguo, after_blk);
        printf("[boot] vm: heap %u KB reservado (objetivo %u, techo %u) | DRAM libre %u->%u B (bloque mayor %u->%u B)\n",
               (unsigned)(s_vm_buffer_size / 1024u), (unsigned) CHIP_VM_OBJETIVO / 1024u, techo / 1024u,
               libre, after, contiguo, after_blk);
        if (s_vm_buffer_size < CHIP_VM_OBJETIVO) {
            /* No es error: es la placa diciendo que hoy no da para el objetivo,
             * y POR QUÉ. Antes esto era el «respaldo» mudo o un malloc fallido. */
            log_printf("vm: AVISO por debajo del objetivo: manda %s",
                       limita_a ? "el bloque contiguo" : "el margen del sistema");
            printf("[boot] vm: AVISO por debajo del objetivo (%u KB): manda %s\n",
                   (unsigned) CHIP_VM_OBJETIVO / 1024u,
                   limita_a ? "el bloque contiguo" : "el margen del sistema");
        }
    } else {
        /* No es fatal: el kernel sigue vivo y el host puede hablar con la placa
         * (H9). Lo que no habrá es VM — y el climb lo reportará. */
        log_printf("vm: heap NO CABE — techo %u B (%s), suelo %u KB | DRAM libre %u B (bloque mayor %u B)",
                   techo, limita_a ? "bloque contiguo" : "margen del sistema",
                   (unsigned) CHIP_VM_MIN / 1024u, libre, contiguo);
        printf("[boot] AVISO: sin RAM para el heap de la VM (techo %u B, suelo %u KB)\n",
               techo, (unsigned) CHIP_VM_MIN / 1024u);
    }
}

/* #353 — sink de diagnóstico de la VM: al log persistente. Aquí NO había fallo
 * (el wire es UART0 y la consola es USB-Serial-JTAG: canales distintos), pero
 * se pone igual para que las tres familias digan lo mismo en el mismo sitio, y
 * porque el log SOBREVIVE al reset y la consola no. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

void app_main(void)
{
    /* Estos printf van a la CONSOLA = USB-Serial-JTAG (puerto nativo),
     * NO al wire (UART0). Sirven para depurar el arranque. */
    printf("\n=== BasicPlus VM en " CHIP_NOMBRE " (H4.3 — wire v1) ===\n");
    printf("[boot] wire v1 = %s\n", wire_v1_transport_name());

    /* LO PRIMERO: el log persistente. Recupera el snapshot de la sesión anterior
     * (post-mortem: si el arranque previo se fue al garete, aquí está escrito) y
     * queda grabando desde antes del climb del boot. */
    log_init();
    bpvm_diag_set_sink(diag_al_log);          /* #353 */
    log_printf("=== boot " CHIP_NOMBRE " ===");
    /* #439 — DE DONDE sale lo que trae el log. Sin esto, un volcado con dos
     * arranques no distingue «la RAM sobrevivio» de «se cargo de flash lo que
     * el arranque anterior volco» — que es exactamente lo que habia antes.
     * Estaba solo en la Pico: el mecanismo viajo a las 4 imagenes y el AVISO
     * no. Un arreglo a medias es el que no se puede comprobar. */
    log_printf(bpvm_log_origen_ram()
               ? "log: RAM SUPERVIVIENTE (lineas de ANTES del reset)"
               : "log: arranque en frio (cargado de flash)");


    /* El heap de la VM, ANTES del climb: layer_app comprueba s_vm_buffer. */
    vm_buffer_init();

    /* H9 — arranque escalonado: identidad → particiones del env → FS → VM. Sin
     * particiones/FS el climb se queda abajo y el host conduce (nada se
     * auto-inicializa); stdlib solo con el FS montado. */
    board_mgr_esp32_boot();
    if (board_boot_status()->state >= BPVM_BOOT_FS) {
        fs_register_bpvm();    /* #247 — file I/O desde BP sobre este FS */
        esp32_mods_install();  /* stdlib core embebida -> /lib (if-absent) */
    }
    esp32_hw_register();   /* backends de HW (GPIO, pico/info) — siempre */
    /* V6/P1.C3.3 — la identidad de placa, si este silicio trae la suya.
     *
     * Por defecto el REPL reporta la del S3 (`s_default_board` en
     * `repl_esp32.c`), y eso es lo que hay que pisar: sin esto el C3 saludaba
     * como `bpvm-esp32` y anunciaba los GPIOs y la SRAM de otro chip. Mismo bug
     * que `U3.19` cazó en el P4. Va por el `chip_cfg.h` —el mecanismo que ya
     * tenemos para lo que cambia por silicio— y no por un símbolo weak, porque
     * en ESP-IDF el `.o` de un componente no entra si nadie lo referencia. */
    CHIP_INSTALAR_BOARD_ID();
    wire_v1_transport_init();

    printf("[boot] REPL wire v1 escuchando en %s.\n", wire_v1_transport_name());

    /* P-autorun (#256) — si /sys/auto.txt existe, arranca la app antes
     * del REPL. El wire ya está vivo y el poll del run atiende
     * HELLO/KILL: el IDE puede conectar y parar la app en cualquier
     * momento. */
    /* #423 — A PARTIR DE AQUI, EL LOG LO MANDA EL ENTORNO (`log=0|1`).
     *
     * El arranque entero queda registrado SIEMPRE: son unas quince lineas y no
     * llenan nada, y son justo las que hacen falta cuando una placa no arranca.
     * Lo que se apaga es el rastro de EJECUCION — el que llena la region de
     * 8 KB en ~26 colectas del GC y hacia que un cuelgue no dejara su ultimo
     * momento escrito (#423).
     *
     * Por defecto APAGADO: se enciende con `log=1` en el entorno cuando se va a
     * depurar, que es lo que pidio Eduardo. Para moverlo mas arriba, basta con
     * subir esta llamada: todo lo que quede por encima se registra siempre. */
    /* `stack=N` (KB) — el usuario reparte el bloque entre pilas y heap;
     * ausente o 0 = el reparto de siempre. Mismo sitio y misma clave que
     * en las otras familias: la regla vive en bpvm_stack_region_bytes(). */
    bpvm_set_stack_kb((unsigned long) bpvm_env_get_long(board_mgr_env(), "stack", 0));
    bpvm_set_quantum_ops(bpvm_env_get_long(board_mgr_env(), "quantum", 0));   /* #462 */
    bpvm_log_set_enabled(bpvm_env_get_bool(board_mgr_env(), "log", 0));

    /* #464 — EL `if` ESTABA SIN LLAVES y se comía la línea equivocada.
     *
     * Estaba escrito como en el P4 pero repartido en varias líneas, con catorce
     * de comentario en medio: el `if` gobernaba `bpvm_set_stack_kb` —que debe
     * aplicarse SIEMPRE— y dejaba `repl_esp32_autorun()` incondicional, que es
     * justo lo contrario de lo que dice su propio comentario. Los dos efectos,
     * cambiados. El P4 lo tiene bien y en una sola línea; se copia esa forma. */
    if (board_boot_status()->state == BPVM_BOOT_APP && !board_boot_status()->degraded)
        repl_esp32_autorun();   /* H9: autorun solo con la placa sana en estado 3 */
    repl_esp32_run();   /* no retorna */
}
