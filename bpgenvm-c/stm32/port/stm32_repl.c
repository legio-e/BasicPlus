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
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "stm32_fs.h"
#include "stm32_mods.h"     /* stdlib core embebida (pre-install /lib) */
#include "gpio_stm32.h"     /* stm32_hw_register (backends GPIO + Pico) */
#include "json_min.h"
#include "bpvm.h"
#include "bpvm_internal.h"   /* vm->modules[].{name,imports,import_count} para deps */
#include "mdn_loader.h"      /* H9.5: overlay AOT .mdn desde el FS (loader compartido) */
#include "aot_registry.h"    /* H9.5: bpvm_aot_clear entre RUNs (registry global) */

#include "main.h"
#include "stm32u5xx_nucleo.h"   /* BSP_LED_Toggle, LED_GREEN */

#include <string.h>
#include <stdio.h>

#define SERVER_NAME "bpvm-stm32"
#define BOARD_NAME  "nucleo-u575zi"

/* Buffers estáticos (NO en stack: el stack C del micro es pequeño). */
static char    s_line[WIRE_LINE_MAX];
static uint8_t s_put_buf[32u * 1024u];        /* payload de PUT */
static uint8_t s_vm_mem[128u * 1024u];        /* RAM que gestiona la VM */
static char    s_out_esc[2048];               /* salida escapada (sink) */
static char    s_out_msg[2300];               /* evento OUTPUT completo */
static long    s_session = 0;                 /* contador de sesiones RUN */
static long    s_run_session = 0;             /* sesión activa (para el sink) */

/* ---- helpers de envío ---- */

static void reply_empty(const char* type, long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"id\":%ld}", type, id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- META ---- */

static void handle_hello(long id) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"HELLO_REPLY\",\"id\":%ld,\"protoVersion\":1,"
        "\"serverName\":\"%s\",\"serverBuild\":\"%s %s\","
        "\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\"]}",
        id, SERVER_NAME, __DATE__, __TIME__);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_info(long id) {
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    uint32_t u2 = *(volatile uint32_t*) (UID_BASE + 8U);
    char buf[320];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"INFO_REPLY\",\"id\":%ld,"
        "\"uniqueId\":\"%08lX%08lX%08lX\","
        "\"boardName\":\"%s\",\"cpuFreqHz\":%lu,\"uptimeMs\":%lu,\"tempC\":0,"
        "\"fsTotalBytes\":%lu,\"fsUsedBytes\":%lu}",
        id, (unsigned long) u2, (unsigned long) u1, (unsigned long) u0,
        BOARD_NAME, (unsigned long) SystemCoreClock, (unsigned long) HAL_GetTick(),
        (unsigned long) fs_total_bytes(), (unsigned long) fs_used_bytes());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- FILES ---- */

static void handle_list(long id, json_obj_t* obj) {
    char prefix[64];
    if (json_get_str(obj, "path", prefix, sizeof(prefix)) < 0) prefix[0] = '\0';
    size_t plen = strlen(prefix);

    char buf[1024];
    size_t o = 0;
    o += (size_t) snprintf(buf + o, sizeof(buf) - o,
        "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[", id);
    int first = 1;
    int cnt = fs_count();
    for (int i = 0; i < cnt; i++) {
        const char* name; uint32_t size;
        if (fs_entry(i, &name, &size) != 0) continue;
        if (plen > 0 && strncmp(name, prefix, plen) != 0) continue;
        const char* rel = name + plen;             /* basename tras el prefijo */
        /* Solo recortar la '/' del resto cuando HAY prefijo. Con LIST("")
         * (el del árbol del IDE) hay que devolver el nombre COMPLETO tal
         * cual está guardado ("/app/X.mod") — como hace el Pico — o el
         * DEL/GET del árbol mandan el path sin barra y find() exacto da
         * NOT_FOUND. */
        if (plen > 0 && *rel == '/') rel++;
        int w = snprintf(buf + o, sizeof(buf) - o,
            "%s{\"name\":\"%s\",\"size\":%lu,\"isDir\":false,\"mtime\":0}",
            first ? "" : ",", rel, (unsigned long) size);
        if (w < 0 || (size_t) w >= sizeof(buf) - o) break;   /* no cabe más */
        o += (size_t) w;
        first = 0;
    }
    o += (size_t) snprintf(buf + o, sizeof(buf) - o, "]}");
    stm32_wire_send_line(buf, o);
}

static void handle_stat(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* d; uint32_t size;
    if (fs_get(path, &d, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"STAT_REPLY\",\"id\":%ld,\"size\":%lu,\"isDir\":false,\"mtime\":0}",
        id, (unsigned long) size);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_df(long id) {
    uint32_t total = fs_total_bytes(), used = fs_used_bytes();
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"DF_REPLY\",\"id\":%ld,\"totalBytes\":%lu,\"usedBytes\":%lu,"
        "\"freeBytes\":%lu,\"fileCount\":%d}",
        id, (unsigned long) total, (unsigned long) used,
        (unsigned long) (total - used), fs_count());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_get(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* d; uint32_t size;
    if (fs_get(path, &d, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"GET_REPLY\",\"id\":%ld,\"bulk\":%lu}", id, (unsigned long) size);
    if (n > 0) {
        stm32_wire_send_line(buf, (size_t) n);
        stm32_wire_send_bulk(d, size);
    }
}

static void handle_del(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    if (fs_del(path) != 0) { stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return; }
    if (strncmp(path, "/lib/", 5) != 0) fs_save();   /* /lib se re-provee al boot */
    reply_empty("DEL_REPLY", id);
}

static void handle_put(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { stm32_wire_send_error(id, "INVALID_PARAM", "missing bulk"); return; }

    /* CRÍTICO: consumir SIEMPRE los `bulk` bytes para no desincronizar el wire. */
    if ((size_t) bulk > sizeof(s_put_buf)) {
        size_t rem = (size_t) bulk;
        while (rem > 0) {
            size_t chunk = rem < sizeof(s_put_buf) ? rem : sizeof(s_put_buf);
            if (stm32_wire_recv_bulk(s_put_buf, chunk) != 0) {
                stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
            }
            rem -= chunk;
        }
        stm32_wire_send_error(id, "NO_SPACE", "fichero demasiado grande");
        return;
    }
    if (stm32_wire_recv_bulk(s_put_buf, (size_t) bulk) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bulk underrun"); return;
    }
    if (fs_put(path, s_put_buf, (uint32_t) bulk) != 0) {
        stm32_wire_send_error(id, "NO_SPACE", "FS lleno"); return;
    }
    /* Persistir sólo lo que sobrevive al reset de forma útil: /lib lo re-instala
     * el embebido al boot, así que un PUT a /lib (el IDE lo hace cada Run) no
     * necesita flash → evita un erase+program por ejecución. */
    if (strncmp(path, "/lib/", 5) != 0) fs_save();
    reply_empty("PUT_REPLY", id);
}

static void handle_format(long id, json_obj_t* obj) {
    char confirm[8];
    if (json_get_str(obj, "confirm", confirm, sizeof(confirm)) < 0 ||
        strcmp(confirm, "YES") != 0) {
        stm32_wire_send_error(id, "MISSING_CONFIRM", "confirm:\"YES\""); return;
    }
    fs_format();
    fs_save();                       /* H9.3: persistir el formateo (FS vacío) */
    reply_empty("FORMAT_REPLY", id);
}

static void handle_log_dump(long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"LOG_DUMP_REPLY\",\"id\":%ld,\"text\":\"\"}", id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* ---- TERMINAL: RUN + streaming ---- */

/* Cada PRINT_* de la VM llega aquí → evento OUTPUT con los bytes escapados. */
static void v1_output_sink(const char* s, size_t len, void* user) {
    (void) user;
    if (stm32_wire_json_escape(s, len, s_out_esc, sizeof(s_out_esc)) < 0) return;
    int n = snprintf(s_out_msg, sizeof(s_out_msg),
        "{\"type\":\"OUTPUT\",\"session\":%ld,\"stream\":\"stdout\",\"data\":\"%s\"}",
        s_run_session, s_out_esc);
    if (n > 0) stm32_wire_send_line(s_out_msg, (size_t) n);
}

static void emit_exited(long session, const char* status, int code, uint32_t ms) {
    char buf[200];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"%s\",\"exitCode\":%d,"
        "\"elapsedMs\":%lu}", session, status, code, (unsigned long) ms);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

/* Resuelve un nombre de módulo en el FS: prueba name, /app/name, /lib/name
 * (el IDE sube las deps a /lib). 0 OK, -1 no está. */
static int stm32_fs_resolve(const char* name, const uint8_t** data, uint32_t* size) {
    if (fs_get(name, data, size) == 0) return 0;
    char p[80];
    snprintf(p, sizeof(p), "/app/%s", name);
    if (fs_get(p, data, size) == 0) return 0;
    snprintf(p, sizeof(p), "/lib/%s", name);
    if (fs_get(p, data, size) == 0) return 0;
    return -1;
}

static void handle_run(long id, json_obj_t* obj) {
    char path[64];
    if (json_get_str(obj, "path", path, sizeof(path)) < 0) {
        stm32_wire_send_error(id, "INVALID_PATH", "missing path"); return;
    }
    const uint8_t* data; uint32_t size;
    if (stm32_fs_resolve(path, &data, &size) != 0) {
        stm32_wire_send_error(id, "NOT_FOUND", "no existe"); return;
    }

    long session = ++s_session;
    s_run_session = session;
    { /* RUN_REPLY con la sesión, ANTES de ejecutar */
        char buf[80];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"RUN_REPLY\",\"id\":%ld,\"session\":%ld}", id, session);
        if (n > 0) stm32_wire_send_line(buf, (size_t) n);
    }

    bpvm_t* vm = bpvm_init(s_vm_mem, sizeof(s_vm_mem), 0);
    if (!vm) { BSP_LED_On(LED_RED); emit_exited(session, "INTERNAL_ERROR", -1, 0); return; }
    bpvm_set_output(vm, v1_output_sink, NULL);

    uint32_t t0 = HAL_GetTick();
    BSP_LED_On(LED_BLUE);                         /* azul = ejecutando un programa */
    bpvm_status_t st = bpvm_load_mod_buffer(vm, data, size, path);

    /* Resolución iterativa de imports: por cada módulo cargado y cada import,
     * carga <owner>.mod del FS si aún no está. Hasta 4 pasadas (deps de deps).
     * bpvm_run() enlaza todo al arrancar (bpvm_link_all interno). */
    for (int pass = 0; st == BPVM_OK && pass < 4; pass++) {
        int loaded_any = 0;
        int n_before = vm->module_count;
        for (int mi = 0; mi < n_before && st == BPVM_OK; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[40]; size_t ol = 0;
                while (imp[ol] && imp[ol] != '.' && ol < sizeof(owner) - 1) { owner[ol] = imp[ol]; ol++; }
                owner[ol] = '\0';
                if (!owner[0]) continue;
                int already = 0;
                for (int j = 0; j < vm->module_count; j++)
                    if (strcmp(vm->modules[j].name, owner) == 0) { already = 1; break; }
                if (already) continue;
                char fname[48]; snprintf(fname, sizeof(fname), "%s.mod", owner);
                const uint8_t* dep; uint32_t dep_size;
                if (stm32_fs_resolve(fname, &dep, &dep_size) != 0) continue;  /* falta → guard */
                bpvm_status_t ds = bpvm_load_mod_buffer(vm, dep, dep_size, owner);
                if (ds != BPVM_OK) { st = ds; break; }
                loaded_any = 1;
            }
        }
        if (!loaded_any) break;
    }

    /* Guard anti-hard-fault: ¿quedó algún import sin resolver? En bare-metal un
     * CALL_EXT no resuelto puede colgar el micro → mejor error limpio. */
    char missing[40] = {0};
    if (st == BPVM_OK) {
        for (int mi = 0; mi < vm->module_count && !missing[0]; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count && !missing[0]; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[40]; size_t ol = 0;
                while (imp[ol] && imp[ol] != '.' && ol < sizeof(owner) - 1) { owner[ol] = imp[ol]; ol++; }
                owner[ol] = '\0';
                if (!owner[0]) continue;
                int found = 0;
                for (int j = 0; j < vm->module_count; j++)
                    if (strcmp(vm->modules[j].name, owner) == 0) { found = 1; break; }
                if (!found) strncpy(missing, owner, sizeof(missing) - 1);
            }
        }
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
     * — confirmar en placa con el primer smoke (fib_native). */
    bpvm_aot_clear();
    if (st == BPVM_OK && !missing[0]) {
        for (int mi = 0; mi < vm->module_count; mi++) {
            const char* mname = vm->modules[mi].name;
            if (!mname || !mname[0]) continue;
            char mdn_path[48];
            snprintf(mdn_path, sizeof(mdn_path), "%s.mdn", mname);
            const uint8_t* mdn_data; uint32_t mdn_size;
            if (stm32_fs_resolve(mdn_path, &mdn_data, &mdn_size) != 0) continue;
            int mrc = bpvm_load_mdn(vm, mdn_data, (size_t) mdn_size);
            /* Visible en la consola del IDE: sin esto, un fallo de carga
             * (p.ej. buffer desalineado) caía a interpretado EN SILENCIO. */
            char mmsg[96];
            int mn = snprintf(mmsg, sizeof(mmsg), "[AOT] %s %s (rc=%d)\n",
                              mdn_path, (mrc == 0) ? "OK" : "FALLO -> interpretado", mrc);
            if (mn > 0) v1_output_sink(mmsg, (size_t) mn, NULL);
        }
    }

    if (st == BPVM_OK && !missing[0]) st = bpvm_run(vm);
    BSP_LED_Off(LED_BLUE);
    uint32_t dt = HAL_GetTick() - t0;

    if (missing[0]) {
        BSP_LED_On(LED_RED);
        char buf[160];
        int n = snprintf(buf, sizeof(buf),
            "{\"type\":\"EXITED\",\"session\":%ld,\"status\":\"RUNTIME_ERROR\","
            "\"exitCode\":-2,\"elapsedMs\":0,"
            "\"errorMessage\":\"falta el modulo %s en el FS (stdlib no embebida?)\"}",
            session, missing);
        if (n > 0) stm32_wire_send_line(buf, (size_t) n);
    } else {
        if (st != BPVM_OK) BSP_LED_On(LED_RED);   /* rojo = el programa falló */
        emit_exited(session, (st == BPVM_OK) ? "OK" : "RUNTIME_ERROR", (int) st, dt);
    }
    bpvm_destroy(vm);
}

/* ---- dispatch ---- */

static void dispatch(int first_char) {
    int len = stm32_wire_recv_line(first_char, s_line, sizeof(s_line));
    if (len == -2) return;   /* línea estancada: silencio; el IDE reintenta */
    if (len < 0)  { stm32_wire_send_fatal("PROTOCOL_ERROR", "line too long"); return; }

    json_obj_t obj;
    if (json_parse(s_line, (size_t) len, &obj) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bad JSON"); return;
    }
    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        stm32_wire_send_error(id, "PROTOCOL_ERROR", "missing type"); return;
    }

    if      (strcmp(type, "HELLO")     == 0) handle_hello(id);
    else if (strcmp(type, "INFO")      == 0) handle_info(id);
    else if (strcmp(type, "PING")      == 0) reply_empty("PONG", id);
    else if (strcmp(type, "TIME")      == 0) reply_empty("TIME_REPLY", id);
    else if (strcmp(type, "LIST")      == 0) handle_list(id, &obj);
    else if (strcmp(type, "STAT")      == 0) handle_stat(id, &obj);
    else if (strcmp(type, "DF")        == 0) handle_df(id);
    else if (strcmp(type, "GET")       == 0) handle_get(id, &obj);
    else if (strcmp(type, "PUT")       == 0) handle_put(id, &obj);
    else if (strcmp(type, "DEL")       == 0) handle_del(id, &obj);
    else if (strcmp(type, "MKDIR")     == 0) reply_empty("MKDIR_REPLY", id);
    else if (strcmp(type, "FORMAT")    == 0) handle_format(id, &obj);
    else if (strcmp(type, "RUN")       == 0) handle_run(id, &obj);
    else if (strcmp(type, "LOG_DUMP")  == 0) handle_log_dump(id);
    else if (strcmp(type, "LOG_CLEAR") == 0) reply_empty("LOG_CLEAR_REPLY", id);
    else if (strcmp(type, "RESET")     == 0) {
        reply_empty("RESET_REPLY", id);
        HAL_Delay(50);
        NVIC_SystemReset();
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "type '%s' no implementado (H9.2)", type);
        stm32_wire_send_error(id, "UNSUPPORTED", msg);
    }
}

void stm32_repl_run(void) {
    /* H9.3 — restaura el FS persistido (los ficheros de /app sobreviven al
     * reset). /lib NO se restaura aquí: lo re-instala el embebido a continuación
     * (stdlib siempre fresca del firmware actual). */
    fs_load();
    /* H9.4 — stdlib core embebida en /lib + backends de HW (GPIO + info de MCU),
     * antes de atender el wire: el primer RUN ya resuelve imports y controla
     * pines reales. */
    stm32_mods_install();
    stm32_hw_register();
    stm32_fs_register_bpvm();   /* #247 — readFile/writeFile/... sobre el FS */

    /* FIFO RX/TX (8 bytes): absorbe el hueco de procesado entre la línea JSON
     * y los bytes bulk que la siguen → PUT fiable aunque la CPU vaya lenta.
     * (El fix definitivo del timing es subir el reloj a 160 MHz.) */
    HAL_UARTEx_SetTxFifoThreshold(&hcom_uart[COM1], UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(&hcom_uart[COM1], UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_EnableFifoMode(&hcom_uart[COM1]);

    stm32_wire_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");

    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') dispatch(c);

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat */
            last_blink = now;
            BSP_LED_Toggle(LED_GREEN);
        }
    }
}
