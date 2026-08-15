/*
 * main.c — H4.3: firmware ESP32-S3 con REPL wire v1.
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
/* Cuánto pide la VM. NO es un número a ojo: sale de medir (#336).
 *   DRAM libre al arrancar, antes de reservar ......... 319632 B
 *   lo que el sistema consume EN MARCHA por encima .....  86256 B  (medido con
 *     heap_caps_get_minimum_free_size tras varios RUN: 188556 justo tras
 *     reservar − 102300 en el peor momento observado)
 *   ⇒ tope seguro = 319632 − 86256 − margen
 * Con 64 KB de margen salen ~164 KB; se redondea a la baja a 160.
 * NO se sube a 192 (que daría heap 128) porque dejaría el peor caso en ~36 KB
 * libres, y con las tareas del IDF creándose EN MARCHA eso ya no es margen.
 * El reparto lo decide bpvm_stack_region_bytes: con 160 KB manda el suelo ⇒
 * stacks 64 KB (igual que antes) y heap 96 KB (+50%).
 * Si algún día no cabe, hay escalón de respaldo abajo: mejor una VM más
 * pequeña que ninguna. */
#define VM_BUFFER_SIZE   (160 * 1024)
#define VM_BUFFER_FALLBACK (128 * 1024)   /* el de siempre, known-good */
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

static void vm_buffer_init(void) {
    /* SRAM interna a propósito: el S3 de referencia no lleva PSRAM y el heap de
     * la VM tiene que ser rápido. Si el módulo trae PSRAM, ampliarlo aquí es un
     * cambio local (el P4 ya lo hace en su main.c). */
    /* #329 — la DRAM interna ANTES y DESPUÉS de la reserva. El Pico dice "libre
     * para malloc: 255 KB" desde siempre y por eso allí se ve enseguida cuándo
     * la RAM aprieta. Aquí falta esa foto: si tras coger 128 KB queda poco, todo
     * lo que malloquee después (littlefs, el wire) empieza a fallar con errores
     * que NO se parecen a "sin memoria". El bloque contiguo mayor importa tanto
     * como el total: se puede tener RAM de sobra y aun así no caber. */
    unsigned before      = (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    unsigned before_blk  = (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_vm_buffer = heap_caps_malloc(VM_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_vm_buffer_size = s_vm_buffer ? VM_BUFFER_SIZE : 0;
    if (!s_vm_buffer) {
        /* Escalón de respaldo: si el bloque grande no cabe (otra variante de S3,
         * un IDF que reserve más), se pide el de siempre en vez de quedarse sin
         * VM. Y se DICE cuál tocó, que si no el día que baje nadie se entera. */
        s_vm_buffer = heap_caps_malloc(VM_BUFFER_FALLBACK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        s_vm_buffer_size = s_vm_buffer ? VM_BUFFER_FALLBACK : 0;
        if (s_vm_buffer) log_printf("vm: AVISO %d KB no caben — se usa el respaldo de %d KB",
                                    VM_BUFFER_SIZE / 1024, VM_BUFFER_FALLBACK / 1024);
    }
    log_printf("vm: heap %u KB %s | DRAM interna libre %u->%u B (bloque mayor %u->%u B)",
               (unsigned)(s_vm_buffer_size / 1024u), s_vm_buffer ? "reservado" : "NO CABE",
               before, (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               before_blk,
               (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!s_vm_buffer) {
        /* No es fatal: el kernel sigue vivo y el host puede hablar con la placa
         * (H9). Lo que no habrá es VM — y el climb lo reportará. */
        printf("[boot] AVISO: sin RAM para el heap de la VM (%d KB)\n", VM_BUFFER_SIZE / 1024);
        log_printf("boot: heap de la VM NO reservado (%d KB) — sin VM", VM_BUFFER_SIZE / 1024);
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
    printf("\n=== BasicPlus VM en ESP32-S3 (H4.3 — wire v1) ===\n");
    printf("[boot] consola/logs = USB-Serial-JTAG | wire v1 = UART0 @115200\n");

    /* LO PRIMERO: el log persistente. Recupera el snapshot de la sesión anterior
     * (post-mortem: si el arranque previo se fue al garete, aquí está escrito) y
     * queda grabando desde antes del climb del boot. */
    log_init();
    bpvm_diag_set_sink(diag_al_log);          /* #353 */
    log_printf("=== boot ESP32-S3 ===");

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
    wire_v1_uart_init();

    printf("[boot] REPL wire v1 escuchando en UART0. Conecta el IDE al puerto del bridge.\n");

    /* P-autorun (#256) — si /sys/auto.txt existe, arranca la app antes
     * del REPL. El wire ya está vivo y el poll del run atiende
     * HELLO/KILL: el IDE puede conectar y parar la app en cualquier
     * momento. */
    if (board_boot_status()->state == BPVM_BOOT_APP && !board_boot_status()->degraded)
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
    bpvm_log_set_enabled(bpvm_env_get_bool(board_mgr_env(), "log", 0));

        repl_esp32_autorun();   /* H9: autorun solo con la placa sana en estado 3 */
    repl_esp32_run();   /* no retorna */
}
