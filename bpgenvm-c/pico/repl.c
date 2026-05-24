/*
 * repl.c — implementación del REPL sobre USB CDC.
 *
 * Comandos:
 *   HELLO              — banner "OK bpvm-pico vX.Y"
 *   LS                 — listado: "OK <count>\n<line>*"
 *   PUT name size      — escribe; tras la linea, esperar `size` bytes
 *   GET name           — devuelve "OK <size>\n<bytes>"
 *   DEL name           — borra
 *   RUN name           — ejecuta y stream output
 *   MEM                — stats de memoria
 *   SAVE               — persiste FS a flash
 *   FORMAT             — borra todo el FS (RAM); requiere SAVE para persistir
 *   RESET              — reboot
 *   BOOTSEL            — reboot a BOOTSEL mode (USB MSC para reflashear)
 *   HELP               — lista de comandos
 */

#include "repl.h"
#include "fs.h"
#include "log.h"

#include "bpvm.h"
#include "bpvm_internal.h"   /* para inspect deps en cmd_run */

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/bootrom.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* --- Buffer de la VM (compartido con main.c). --------------------- */
extern uint8_t s_vm_buffer[];
extern unsigned int VM_BUFFER_SIZE_VAR;     /* hack: usamos macro indirecta */
extern const uint32_t s_vm_buffer_size;

/* --- Sink para el output de la VM ------------------------------ */
static void usb_sink(const char* data, size_t len, void* user) {
    (void) user;
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

/* --- I/O helpers ---------------------------------------------- */

/* Lee un char no-bloqueante. Devuelve -1 si no hay datos. Usa
 * getchar_timeout_us(0) que retorna inmediatamente — esto es CRUCIAL
 * porque permite que vTaskDelay cede el control y tud_task() vacíe
 * los buffers de salida USB. Un getchar() bloqueante en este punto
 * deja la salida atascada para siempre. */
static int try_get_char(void) {
    return getchar_timeout_us(0);    /* PICO_ERROR_TIMEOUT (-1) si nada */
}

/* Lee una línea desde stdin. Acepta CR, LF o CRLF como terminador.
 * Devuelve len. Eco cada carácter al usuario para que vea lo que
 * escribe (los terminales serie no hacen eco local por default). */
static int read_line(char* buf, int max) {
    static int last_was_cr = 0;
    int n = 0;
    while (n < max - 1) {
        int c = try_get_char();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        /* LF inmediatamente tras CR → parte del CRLF, ignorar. */
        if (c == '\n' && last_was_cr) {
            last_was_cr = 0;
            continue;
        }
        last_was_cr = (c == '\r');
        if (c == '\r' || c == '\n') {
            putchar('\r'); putchar('\n');
            fflush(stdout);
            break;
        }
        /* Backspace: corrige edición. */
        if (c == 0x7F || c == 0x08) {
            if (n > 0) {
                n--;
                putchar('\b'); putchar(' '); putchar('\b');
                fflush(stdout);
            }
            continue;
        }
        buf[n++] = (char) c;
        putchar(c);             /* eco local */
        fflush(stdout);
    }
    buf[n] = '\0';
    return n;
}

/* Lee N bytes binarios desde stdin a un buffer. */
static int read_bytes(uint8_t* buf, uint32_t n) {
    uint32_t got = 0;
    while (got < n) {
        int c = try_get_char();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        buf[got++] = (uint8_t) c;
    }
    return (int) got;
}

/* --- Callback para LS ----------------------------------------- */
static int ls_cb(const char* name, uint32_t size, void* user) {
    (void) user;
    printf("%s %u\n", name, (unsigned) size);
    return 0;
}

/* --- Comandos individuales ------------------------------------ */

static void cmd_hello(void) {
    /* Incluye el build timestamp del firmware. Cuando algo se
     * comporta como una versión vieja, este string permite verificar
     * en segundos si el UF2 flasheado es el esperado. */
    printf("OK bpvm-pico v0.2 (RP2350+FreeRTOS) build " __DATE__ " " __TIME__ "\n");
    fflush(stdout);
}

static void cmd_ls(void) {
    int count = fs_file_count();
    printf("OK %d\n", count);
    fs_list(ls_cb, NULL);
    fflush(stdout);
}

static void cmd_mem(void) {
    printf("OK total=%u used=%u free=%u count=%d\n",
           (unsigned) fs_total_bytes(),
           (unsigned) fs_used_bytes(),
           (unsigned) fs_free_bytes(),
           fs_file_count());
    fflush(stdout);
}

/* Buffer temporal para PUT. 32 KB cubre cualquier .mod razonable
 * (los más grandes vistos son ~3-5 KB; Json.mod sube de 20 KB). */
static uint8_t s_put_buf[32 * 1024];

static void cmd_put(const char* args) {
    /* parse "name size" */
    char name[FS_NAME_LEN];
    unsigned int size;
    if (sscanf(args, "%39s %u", name, &size) != 2) {
        printf("ERR usage: PUT <name> <size>\n");
        fflush(stdout);
        return;
    }
    if (size > sizeof(s_put_buf)) {
        printf("ERR file too big (max %u)\n", (unsigned) sizeof(s_put_buf));
        fflush(stdout);
        return;
    }
    /* Acknowledge para que el cliente sepa que ya puede enviar. */
    printf("READY %u\n", size);
    fflush(stdout);

    read_bytes(s_put_buf, size);
    fs_status_t s = fs_put(name, s_put_buf, size);
    if (s == FS_OK) printf("OK %u free\n", (unsigned) fs_free_bytes());
    else            printf("ERR %s\n", fs_status_str(s));
    fflush(stdout);
}

static void cmd_get(const char* args) {
    char name[FS_NAME_LEN];
    if (sscanf(args, "%39s", name) != 1) {
        printf("ERR usage: GET <name>\n");
        fflush(stdout);
        return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(name, &data, &size);
    if (s != FS_OK) {
        printf("ERR %s\n", fs_status_str(s));
        fflush(stdout);
        return;
    }
    printf("OK %u\n", (unsigned) size);
    fflush(stdout);
    fwrite(data, 1, size, stdout);
    fflush(stdout);
}

static void cmd_del(const char* args) {
    char name[FS_NAME_LEN];
    if (sscanf(args, "%39s", name) != 1) {
        printf("ERR usage: DEL <name>\n");
        fflush(stdout);
        return;
    }
    fs_status_t s = fs_delete(name);
    if (s == FS_OK) printf("OK\n");
    else            printf("ERR %s\n", fs_status_str(s));
    fflush(stdout);
}

static void cmd_save(void) {
    fs_status_t s = fs_save_to_flash();
    if (s == FS_OK) printf("OK saved\n");
    else            printf("ERR %s\n", fs_status_str(s));
    fflush(stdout);
}

static void cmd_format(void) {
    fs_format_ram();
    printf("OK formatted (RAM only — use SAVE to persist)\n");
    fflush(stdout);
}

static void cmd_run(const char* args) {
    char name[FS_NAME_LEN];
    if (sscanf(args, "%39s", name) != 1) {
        printf("ERR usage: RUN <name>\n");
        fflush(stdout);
        return;
    }
    const uint8_t* data; uint32_t size;
    fs_status_t s = fs_get(name, &data, &size);
    if (s != FS_OK) {
        printf("ERR %s\n", fs_status_str(s));
        fflush(stdout);
        return;
    }
    extern uint8_t s_vm_buffer[];
    extern const uint32_t s_vm_buffer_size;
    bpvm_t* vm = bpvm_init(s_vm_buffer, s_vm_buffer_size, 0);
    if (!vm) {
        printf("ERR bpvm_init failed\n");
        fflush(stdout);
        return;
    }
    bpvm_set_output(vm, usb_sink, NULL);

    bpvm_status_t ls = bpvm_load_mod_buffer(vm, data, size, name);
    if (ls != BPVM_OK) {
        printf("ERR load: %s\n", bpvm_status_str(ls));
        bpvm_destroy(vm);
        fflush(stdout);
        return;
    }

    /* Resolución de deps embebida: el frontend, al compilar Blink que
     * importa Gpio, mete CALL_EXT a "Gpio.__init", "Gpio.write", etc.
     * Necesitamos cargar Gpio.mod desde el FS antes de bpvm_link_all.
     * Iteramos los imports del módulo recién cargado y, por cada uno
     * cuyo módulo dueño no esté ya cargado, lo buscamos en el FS. */
    for (int pass = 0; pass < 4; pass++) {
        int loaded_any = 0;
        int n_before = vm->module_count;
        for (int mi = 0; mi < n_before; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                /* Derive owner module name: hasta el primer '.'. */
                char owner[40]; size_t ol = 0;
                while (imp[ol] && imp[ol] != '.' && ol < sizeof(owner) - 1) {
                    owner[ol] = imp[ol]; ol++;
                }
                owner[ol] = '\0';
                if (!owner[0]) continue;
                /* ¿Ya cargado? */
                int already = 0;
                for (int j = 0; j < vm->module_count; j++) {
                    if (strcmp(vm->modules[j].name, owner) == 0) {
                        already = 1; break;
                    }
                }
                if (already) continue;
                /* Buscar <owner>.mod en el FS. */
                char fname[48];
                snprintf(fname, sizeof(fname), "%s.mod", owner);
                const uint8_t* dep; uint32_t dep_size;
                if (fs_get(fname, &dep, &dep_size) != FS_OK) {
                    printf("[run] dep '%s' no encontrado en FS\n", fname);
                    continue;
                }
                bpvm_status_t ds = bpvm_load_mod_buffer(vm, dep, dep_size, owner);
                if (ds != BPVM_OK) {
                    printf("[run] load dep '%s' falló: %s\n", fname,
                           bpvm_status_str(ds));
                    bpvm_destroy(vm);
                    fflush(stdout);
                    return;
                }
                printf("[run] cargado dep %s (%u bytes)\n", fname,
                       (unsigned) dep_size);
                loaded_any = 1;
            }
        }
        if (!loaded_any) break;   /* punto fijo */
    }

    printf("--- VM output ---\n");
    fflush(stdout);
    log_printf("RUN %s (%u bytes)", name, (unsigned) size);
    bpvm_status_t rs = bpvm_run(vm);
    printf("\n--- VM finished: %s ---\n", bpvm_status_str(rs));
    log_printf("RUN %s finished: %s", name, bpvm_status_str(rs));
    fflush(stdout);
    bpvm_destroy(vm);
}

static void log_to_stdout(const char* data, size_t len, void* user) {
    (void) user;
    fwrite(data, 1, len, stdout);
}

static void cmd_log(void) {
    printf("OK %u/%u bytes\n", (unsigned) log_used_bytes(),
                                (unsigned) log_total_bytes());
    log_dump(log_to_stdout, NULL);
    /* Asegura newline final. */
    if (log_used_bytes() > 0) printf("\n");
    fflush(stdout);
}

static void cmd_logsave(void) {
    log_printf("LOGSAVE comando manual");
    log_flush();
    printf("OK flushed (%u bytes)\n", (unsigned) log_used_bytes());
    fflush(stdout);
}

static void cmd_logclear(void) {
    log_clear_ram();
    log_clear_flash();
    printf("OK log cleared (ram+flash)\n");
    fflush(stdout);
}

static void cmd_help(void) {
    printf("OK\n");
    printf("HELLO              banner\n");
    printf("LS                 list files\n");
    printf("PUT name size      upload a file (binary)\n");
    printf("GET name           download a file\n");
    printf("DEL name           delete a file\n");
    printf("RUN name           execute a .mod\n");
    printf("MEM                fs stats\n");
    printf("SAVE               persist fs to flash\n");
    printf("FORMAT             wipe fs (ram only)\n");
    printf("LOG                dump persistent log\n");
    printf("LOGSAVE            snapshot log to flash\n");
    printf("LOGCLEAR           wipe log (ram+flash)\n");
    printf("RESET              soft reboot\n");
    printf("BOOTSEL            reboot into bootloader (drag-drop uf2)\n");
    printf("HELP               this list\n");
    fflush(stdout);
}

/* --- Loop principal ------------------------------------------- */

void repl_run(void) {
    char line[160];

    log_printf("REPL entry");
    log_flush();   /* snapshot del estado de arranque a flash */

    /* Banner repetido 3 veces para evitar perderlo si el host tarda
     * en estar listo tras la apertura del COM. Cada uno con su flush
     * y delay — fuerza varias rondas de tud_task(). */
    for (int i = 0; i < 3; i++) {
        printf("\n");
        printf("=========================================\n");
        printf(" bpvm-pico REPL listo. HELP para comandos.\n");
        printf("=========================================\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for (;;) {
        printf("> ");
        fflush(stdout);
        int n = read_line(line, sizeof(line));
        if (n <= 0) continue;

        /* Parse: COMANDO [args...] */
        char* sp = strchr(line, ' ');
        const char* args = "";
        if (sp) { *sp = '\0'; args = sp + 1; }

        /* Uppercase del comando. */
        for (char* p = line; *p; p++) {
            if (*p >= 'a' && *p <= 'z') *p -= 32;
        }

        if      (strcmp(line, "HELLO")   == 0) cmd_hello();
        else if (strcmp(line, "LS")      == 0) cmd_ls();
        else if (strcmp(line, "PUT")     == 0) cmd_put(args);
        else if (strcmp(line, "GET")     == 0) cmd_get(args);
        else if (strcmp(line, "DEL")     == 0) cmd_del(args);
        else if (strcmp(line, "RUN")     == 0) cmd_run(args);
        else if (strcmp(line, "MEM")     == 0) cmd_mem();
        else if (strcmp(line, "SAVE")    == 0) cmd_save();
        else if (strcmp(line, "FORMAT")  == 0) cmd_format();
        else if (strcmp(line, "HELP")    == 0) cmd_help();
        else if (strcmp(line, "LOG")     == 0) cmd_log();
        else if (strcmp(line, "LOGSAVE") == 0) cmd_logsave();
        else if (strcmp(line, "LOGCLEAR") == 0) cmd_logclear();
        else if (strcmp(line, "RESET")   == 0) {
            log_printf("RESET: rebooting");
            log_flush();   /* snapshot antes de morir */
            printf("OK rebooting (log flushed)\n"); fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(100));
            extern void watchdog_reboot(uint32_t, uint32_t, uint32_t);
            watchdog_reboot(0, 0, 0);
        }
        else if (strcmp(line, "BOOTSEL") == 0) {
            log_printf("BOOTSEL: entering bootloader");
            log_flush();
            printf("OK entering bootloader (log flushed)\n"); fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(100));
            reset_usb_boot(0, 0);
        }
        else {
            printf("ERR unknown command: %s\n", line);
            fflush(stdout);
        }
    }
}
