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
#include "bpvm_mem.h"          /* V6/U6: el planificador comun de memoria */
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

/* V6/U6.9 — LO QUE EL ESP32 OFRECE: una region compartida, la DRAM interna.
 * `bytes` es el bloque CONTIGUO mayor y `libre` el total: las dos restricciones de
 * la rama compartida del planificador (P1.C3.3 / U6.4) salen de ahi. Con PSRAM (el
 * P4) la region exclusiva la anade su propio main.c; el S3 y el C3 no la tienen. */
/* V6 (8-sep) — CON PSRAM, LA VM VA A LA PSRAM; sin ella, a la SRAM interna.
 *
 * La guarda es `CONFIG_SPIRAM`, o sea la CAPACIDAD, no la familia. Este fichero
 * lo comparten S3, C3 y C6: el S3 declara PSRAM en su sdkconfig y entra por la
 * rama exclusiva; el C3 y el C6 no la declaran y siguen exactamente igual que
 * antes. Una placa nueva de esta familia cae donde le toca sin que nadie tenga
 * que acordarse — es la misma leccion de #472, el mismo dia.
 *
 * ⚠️ Y esto CONTRADICE a proposito el comentario que habia aqui («la SRAM interna
 * a proposito... el heap de la VM tiene que ser rapido»). Ese argumento se cayo
 * con una medida: el interprete ENTIERRA la latencia de memoria, y la PSRAM
 * cuesta un 4-7 %. Cambiar 8 MB por un 4-7 % no es una duda. Mover memoria de la
 * VM por VELOCIDAD no compensa; por CAPACIDAD, si.
 *
 * La forma es la del P4 (esp32p4/main/main.c, `p4_mem_regiones`): region
 * EXCLUSIVA, porque la PSRAM no se la disputa nadie mas — los allocs pequenos se
 * quedan en DRAM interna por `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL`.
 *
 * Si la placa declara PSRAM y no la tiene (IGNORE_NOTFOUND deja arrancar), esta
 * rama ve 0 bytes libres, el plan no da para el suelo y se cae a la interna:
 * no hay que preguntar por el modulo en ningun sitio. */
#if defined(CONFIG_SPIRAM)
#  define ESP32_MEM_CAPS   (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#  define ESP32_MEM_NOMBRE "PSRAM"
#  define ESP32_MEM_EXCL   1
   /* El margen de la SRAM (CHIP_MARGEN_SISTEMA) esta MEDIDO para la SRAM y aqui
    * no aplica: en PSRAM lo que hay que dejar es lo que otros subsistemas pidan
    * de ella (con ALWAYSINTERNAL=2048, los allocs grandes de terceros). 512 KB
    * es un margen PRUDENTE Y NO MEDIDO — se ajusta con el log de arranque de la
    * placa, que dice libre/mayor antes y despues de tomar. El P4 reserva 4 MB
    * porque tiene que alimentar a LVGL; el S3 no tiene pantalla. */
#  define ESP32_MEM_MARGEN (512u * 1024u)
#else
#  define ESP32_MEM_CAPS   (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#  define ESP32_MEM_NOMBRE "SRAM interna"
#  define ESP32_MEM_EXCL   0
#  define ESP32_MEM_MARGEN CHIP_MARGEN_SISTEMA
#endif

static int esp32_mem_regiones(bpvm_mem_region_t* out, int max, multi_heap_info_t* hi) {
    if (max < 1) return 0;
    heap_caps_get_info(hi, ESP32_MEM_CAPS);
    out[0].base      = NULL;                      /* se toma pidiendo: heap_caps_malloc */
    out[0].bytes     = hi->largest_free_block;
    out[0].libre     = hi->total_free_bytes;
    out[0].exclusiva = ESP32_MEM_EXCL;
    out[0].margen    = ESP32_MEM_MARGEN;         /* SRAM: medido. PSRAM: prudente, ver arriba */
    out[0].nombre    = ESP32_MEM_NOMBRE;
    return 1;
}

static void vm_buffer_init(void) {
    /* SRAM interna a proposito: el S3 de referencia no lleva PSRAM y el heap de
     * la VM tiene que ser rapido. (El P4, que si la tiene, va por su main.c.)
     *
     * V6/U6.9 — ENUMERAR, PLANIFICAR, TOMAR. U6.7 dejo aqui la regla de la rama
     * compartida (techo = min(bloque contiguo, libre - margen), nunca mas que el
     * objetivo, suelo, escalera); ahora esa regla vive en src/bpvm_mem.c, la misma
     * para las cinco placas y probada en host contra los numeros medidos de esta
     * (test/test_mem.c). Aqui queda solo lo que es del silicio: preguntarle al
     * heap_caps que hay, y TOMAR lo decidido con malloc. */
    multi_heap_info_t hi;
    bpvm_mem_region_t reg[1];
    int nreg = esp32_mem_regiones(reg, 1, &hi);
    /* La foto en BLOQUES antes de pedir nada: un solo hueco = lo ocupado esta en
     * un extremo; varios = hay reservas en medio (en el C3, 37 del IDF). */
    log_printf("heap: libre %u | mayor %u | bloques: %u libres, %u usados | usado %u",
               (unsigned) hi.total_free_bytes, (unsigned) hi.largest_free_block,
               (unsigned) hi.free_blocks, (unsigned) hi.allocated_blocks,
               (unsigned) hi.total_allocated_bytes);

    bpvm_mem_cfg_t cfg;
    /* ⚠️ EL TECHO DE POLITICA TAMBIEN VA POR CAPACIDAD, y es el detalle que hacia
     * inutil activar la PSRAM: CHIP_VM_OBJETIVO son 160 KB y significa «no se
     * sube aunque quepa». Con 8 MB delante, la VM habria seguido cogiendo 160 KB
     * y los otros 7,8 MB se habrian quedado parados — con el arranque diciendo
     * «PSRAM» y todo. Con region exclusiva se hace como el P4: objetivo 0 = la VM
     * toma lo que el plan deje. Sin PSRAM, el techo medido del chip sigue mandando. */
#if defined(CONFIG_SPIRAM)
    cfg.objetivo       = 0;                      /* exclusiva: todo lo que el plan deje */
#else
    cfg.objetivo       = CHIP_VM_OBJETIVO;       /* politica: cuanto quiere la VM (no se sube aunque quepa) */
#endif
    cfg.vm_min         = CHIP_VM_MIN;
    cfg.reserva_bytes  = 0;                      /* la reserva con nombre es de la memoria exclusiva */
    cfg.reserva_nombre = NULL;

    bpvm_mem_plan_t plan;
    bpvm_mem_plan(reg, nreg, &cfg, &plan);
    char linea[160];
    bpvm_mem_plan_str(&plan, reg, &cfg, linea, sizeof linea);
    log_printf("%s", linea);

    /* TOMAR: pidiendo al asignador, en KB enteros, y con escalera si el asignador
     * no sirve lo que sus contadores prometian (cabeceras, un bloque ocupado entre
     * la consulta y la peticion): -4 KB hasta el suelo. Misma idea que el P4 en
     * PSRAM (-1 MiB): UNA manera de no caber. */
    s_vm_buffer = NULL; s_vm_buffer_size = 0;
    unsigned bloque = (plan.res == BPVM_MEM_OK) ? ((unsigned) plan.bytes & ~1023u) : 0u;
    while (bloque >= CHIP_VM_MIN) {
        s_vm_buffer = (uint8_t*) heap_caps_malloc(bloque, ESP32_MEM_CAPS);
        if (s_vm_buffer) { s_vm_buffer_size = bloque; break; }
        bloque -= 4096u;
    }

    unsigned after     = (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    unsigned after_blk = (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (s_vm_buffer) {
        /* El detalle de la familia, al log y a la consola (P1.C3.3: el Pico lo
         * dice en su banner desde siempre; aqui faltaba). */
        log_printf("vm: DRAM interna libre %u->%u B (bloque mayor %u->%u B) | margen %u",
                   (unsigned) hi.total_free_bytes, after, (unsigned) hi.largest_free_block, after_blk,
                   (unsigned) CHIP_MARGEN_SISTEMA);
        printf("[boot] %s\n", linea);
        printf("[boot] vm: DRAM interna libre %u->%u B (bloque mayor %u->%u B)\n",
               (unsigned) hi.total_free_bytes, after, (unsigned) hi.largest_free_block, after_blk);
        if (s_vm_buffer_size < (unsigned) plan.bytes) {
            /* La escalera bajo: el asignador no dio lo que sus contadores decian. */
            log_printf("vm: AVISO el asignador solo dio %u KB de los %u planificados",
                       (unsigned)(s_vm_buffer_size / 1024u), (unsigned)(plan.bytes / 1024u));
        }
    } else {
        /* No es fatal: el kernel sigue vivo y el host puede hablar con la placa
         * (H9). Lo que no habra es VM — y el climb lo reportara. La linea del plan
         * ya dijo por que (NO CABE + techo + suelo). */
        printf("[boot] AVISO: sin RAM para el heap de la VM — %s\n", linea);
        log_flush();
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
