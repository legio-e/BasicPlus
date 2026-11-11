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
#include "pack_esp32.h"        /* V7/H6: el pack nativo, común a la familia */
#include "bpvm_bios.h"         /* bpvm_bios_slot_count */
#include "board_mgr_esp32.h"   /* H9: arranque escalonado + estado del boot */
#include "bpvm_pack.h"          /* V7/H7 (Boot 2, fase 1): el montaje de la zona y su estado, comunes */
#include "bpvm_boot2.h"        /* V7/H7 (Boot 2, fase 4): el arranque común, con los ganchos de aquí */
#include "bpvm_env.h"
#include "freertos/FreeRTOS.h" /* V7/H12: configNUMBER_OF_CORES, los runs por defecto */
#include "log.h"               /* log persistente (post-mortem) — lo antes posible */
#include "esp_heap_caps.h"     /* H11: el heap de la VM se pide en runtime, no es .bss */
#include "esp_memory_utils.h"  /* #565: la vista de instrucciones de la SRAM */
#include "mdn_loader.h"        /* #565: bpvm_mdn_set_vista_ejecutable */

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
/* Cuánto pide la VM y cómo se llama el chip: lo pone el proyecto (`chip_cfg.h`).
 * Era por fuerza —`vm_buffer_init()` corría antes que el ENV—, pero desde V7/H6
 * el ENV ya está leído a esa hora: `pack_esp32_ram_init()`, que va justo antes,
 * lo carga (`board_mgr_esp32_env_temprano`). O sea que una clave del ENV puede
 * decidir el tamaño: `vmheap=N` (KB, el bloque entero —Eduardo, 30-sep—), en
 * vm_buffer_init. El porqué de cada número está en el `chip_cfg.h` que lo fija — y
 * en el del C3 está escrito que AÚN NO ESTÁ MEDIDO. */
#include "chip_cfg.h"
#ifndef CHIP_TIENE_WIFI
#define CHIP_TIENE_WIFI 0      /* V8/H3: lo dice cada `chip_cfg.h` que la lleve */
#endif
#if CHIP_TIENE_WIFI
#include "bpvm_wifi.h"
#include "wifi_esp32.h"
#include "wire_red.h"
#endif
#ifndef CHIP_TIENE_BT
#define CHIP_TIENE_BT 0        /* V8/H2, paso 5: lo dice cada `chip_cfg.h` que lo lleve */
#endif
#if CHIP_TIENE_BT
#include "bpvm_bt.h"
#include "bt_esp32.h"
#endif
#include "bpvm_mem.h"          /* V6/U6: el planificador comun de memoria */
#include "bpvm_sd_placa.h"    /* V7/H3 SD.3: la SD de la placa, en el comun */
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;
/* #691 (12-oct) — POR QUÉ no hay VM, si no la hay (vacío si la hay): el motivo de la capa de la app (layer_app, en
 * board_mgr_esp32.c), que es lo que ven la línea «boot: estado», el panel del IDE y el NOT_READY de un RUN. */
char           s_vm_motivo[BPVM_BOOT_REASON_MAX] = "";

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
    size_t objetivo_imagen = 0;                  /* exclusiva: todo lo que el plan deje */
#else
    size_t objetivo_imagen = CHIP_VM_OBJETIVO;   /* politica: cuanto quiere la VM (no se sube aunque quepa) */
#endif
    /* V7/H7 — `vmheap=N` (KB, el bloque entero) manda sobre el de la imagen. Por
     * encima del techo manda el techo (el margen del sistema sigue protegido), y por
     * debajo del suelo no hay VM: la línea del plan lo dice. */
    long vmheap_kb     = bpvm_env_get_long(board_mgr_env(), "vmheap", 0);
    cfg.objetivo       = bpvm_mem_objetivo_env(vmheap_kb, objetivo_imagen);
    if (vmheap_kb > 0) log_printf("vm: vmheap=%ld KB (ENV): el objetivo del bloque", vmheap_kb);
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
        if (s_vm_buffer_size < ((unsigned) plan.bytes & ~1023u)) {
            /* La escalera bajo: el asignador no dio lo que sus contadores decian.
             * Contra lo PEDIDO (el plan en KB enteros, como `bloque`): contra
             * plan.bytes avisaba siempre que el plan no fuera múltiplo de 1 KB
             * —la C5, 30-sep: «solo dio 6653 KB de los 6653 planificados»—. */
            log_printf("vm: AVISO el asignador solo dio %u KB de los %u planificados",
                       (unsigned)(s_vm_buffer_size / 1024u), (unsigned)(plan.bytes / 1024u));
        }
    } else {
        /* No es fatal: el kernel sigue vivo y el host puede hablar con la placa
         * (H9). Lo que no habra es VM — y el climb lo reportara. La linea del plan
         * ya dijo por que (NO CABE + techo + suelo).
         * #691 — y el MOTIVO corto, el que sube al estado del arranque y al IDE. Si el
         * plan cabia, es la escalera la que no encontro ni el suelo, y eso la linea
         * del plan no lo decia: decia «vm: N KB en …», como si hubiera VM. */
        bpvm_mem_sin_vm_motivo(&plan, reg, &cfg, vmheap_kb, s_vm_motivo, sizeof s_vm_motivo);
        log_printf("vm: AVISO la placa arranca SIN VM: %s", s_vm_motivo);
        printf("[boot] AVISO: la placa arranca SIN VM: %s\n", s_vm_motivo);
        log_flush();
    }
}

/* #353 — sink de diagnóstico de la VM: al log persistente. Aquí NO había fallo
 * (el wire es UART0 y la consola es USB-Serial-JTAG: canales distintos), pero
 * se pone igual para que las tres familias digan lo mismo en el mismo sitio, y
 * porque el log SOBREVIVE al reset y la consola no. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

/* #565 — la vista EJECUTABLE de la RAM. La C3 ve su SRAM por dos direcciones
 * (`SOC_DIRAM_DRAM_LOW` 0x3FC8_0000 para datos, `SOC_DIRAM_IRAM_LOW` 0x4038_0000
 * para instrucciones); saltar por la de datos es una excepción. Es la misma
 * traducción que hace el heap del IDF con MALLOC_CAP_EXEC. Donde las dos vistas
 * coinciden (C5, C6) o el puntero no es SRAM de datos (la PSRAM, la zona), la
 * identidad. */
static const uint8_t* vista_ejecutable_ram(const uint8_t* p) {
    if (!esp_dram_match_iram() && esp_ptr_in_diram_dram(p))
        return (const uint8_t*) esp_ptr_diram_dram_to_iram(p);
    return p;
}

/* ===== V7/H7 (Boot 2, fase 4) — LOS GANCHOS DE LA FAMILIA ESP32 ==================
 * El orden, la capa que pide cada paso y las reglas (con el FS, la zona y /lib; con la app
 * sana, el autorun) son del arranque común (src/bpvm_boot2.c). Aquí, sólo lo que es de
 * estos silicios (S3, C3, C5 y C6; la P4, en su main). */

/* 2a — la memoria. El bloque de RAM del código nativo, ANTES que el heap de la VM: la
 * dirección tiene que ser la misma en cada arranque (es el sello del pack). Luego el heap
 * de la VM (layer_app lo comprueba al subir). Y #565: el código nativo fundido en el
 * `.mod` se ejecuta desde ese heap, y en la C3 la SRAM tiene dos direcciones: se salta
 * por la del bus de instrucciones (en la C5 y la C6 son la misma: la identidad; el `.mdn`
 * suelto del FS no pasa por aquí: repl_esp32.c lo copia con MALLOC_CAP_EXEC). */
static void esp32_memoria(void) {
    pack_esp32_ram_init();
    vm_buffer_init();
    bpvm_mdn_set_vista_ejecutable(vista_ejecutable_ram);
}

/* 2b — subir: la flash que dice el chip, las particiones del env, el FS y la VM. */
static const bpvm_boot_status_t* esp32_subir(void) {
    board_mgr_esp32_boot();
    return board_boot_status();
}

/* 2b — lo de la familia que pide el FS: la fachada de ficheros de BP (#247) y el cargador
 * de los motores nativos (V7/H6), que BUSCA el ancla y SALTA en el primer `Run`, no aquí. */
static void esp32_con_fs(void) {
    fs_register_bpvm();
    repl_set_packs_loader(pack_esp32_cargar);
}

/* 2b — la SD: la registra el chip (`chip_cfg.h`: la C5, su TF por SPI; el S3, el C3 y la
 * C6, el porqué no). La monta el común con el FS, después de los buses. */
static void esp32_sd(void) {
    CHIP_SD_REGISTRAR();
}

/* 2b — la identidad de placa, si este silicio trae la suya (V6/P1.C3.3). Por defecto el
 * REPL reporta la del S3, y eso es lo que hay que pisar: sin esto el C3 saludaba como
 * `bpvm-esp32` y anunciaba los GPIOs y la SRAM de otro chip. Va por el `chip_cfg.h` y no por
 * un símbolo weak: en ESP-IDF el `.o` de un componente no entra si nadie lo referencia. */
static void esp32_identidad(void) {
    CHIP_INSTALAR_BOARD_ID();
}

/* 2b — el cable: UART0 (el puente USB-UART) o el USB-Serial-JTAG, según el chip. */
static void esp32_cable(void) {
    wire_v1_transport_init();
    printf("[boot] REPL wire v1 escuchando en %s.\n", wire_v1_transport_name());
}

#if CHIP_TIENE_WIFI
/* 2b — la red: la radio de este chip, el cable de la red y la WiFi, si el ENV (`wifi=1`) o /sys/wifi.json ("autoStart") lo
 * piden. No bloquea. */
static void esp32_red(void) {
    wifi_esp32_publicar(NULL);
    wire_red_iniciar();
    bpvm_wifi_arranque(bpvm_env_get_bool(board_mgr_env(), "wifi", -1));
#if CHIP_TIENE_BT
    /* V8/H2, paso 5: y el Bluetooth, si /sys/bluetooth.json lo pide ("autoStart"). Tampoco bloquea. */
    bt_esp32_publicar();
    bpvm_bt_arranque();
#endif
}
#endif

static const bpvm_boot2_familia_t s_boot2 = {
    .env          = board_mgr_env,
    .env_anadir   = board_mgr_esp32_env_anadir,
    .memoria      = esp32_memoria,
    .bios         = bios_esp32_verificar,
    .subir        = esp32_subir,
    .mapear_packs = board_mgr_esp32_mapear_packs,
    .stdlib       = esp32_mods_install,
    .con_fs       = esp32_con_fs,
    .hw           = esp32_hw_register,
    .sd           = esp32_sd,
    .identidad    = esp32_identidad,
    .cintura      = repl_esp32_cintura,
    .cable        = esp32_cable,
    .autorun      = repl_esp32_autorun,
    .repl         = repl_esp32_run,
    .nucleos      = configNUMBER_OF_CORES,   /* V7/H12: la S3, dos (y dos runs); la C3, la C5 y la C6, uno */
    .cpu_desde_env = 1,                      /* V8 #784: `cpu.mhz` del ENV, al arrancar */
#if CHIP_TIENE_WIFI
    .red          = esp32_red,
#endif
};

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

    /* V7/H7 (Boot 2, fase 4) — AQUÍ ACABA EL BOOT 1 DE LA FAMILIA: el log, y el ENV a la
     * vista (las claves del reparto y la memoria lo leen; `pack_esp32_ram_init` lo cargaba
     * por su cuenta, y lo sigue haciendo: es idempotente). Lo demás es el arranque común,
     * con los ganchos de arriba: 2a reparte (`stack=`, `quantum=`, la memoria y la BIOS) y
     * 2b sube (las capas, la zona, /lib, el hardware, los buses, la SD, la identidad, la
     * cintura del REPL, el cable, `log=`, el autorun con la placa sana y el REPL). */
    board_mgr_esp32_env_temprano();
    bpvm_boot2_repartir(&s_boot2);
    bpvm_boot2_subir(&s_boot2);   /* no vuelve: acaba en el REPL */
}
