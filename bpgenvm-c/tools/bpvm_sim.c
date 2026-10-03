/*
 * bpvm_sim.c — H10: el MICRO SIMULADO del IDE (la VM-C haciendo de placa).
 *
 * Es un "device" wire v1 COMPLETO servido por TCP: el IDE se conecta con
 * BpvmBackend ("host:port") y lo trata igual que a una placa — explorador de
 * ficheros, Run, Stop, INFO, gestión de placa y packs — sin hardware delante.
 * Nació como boardsim (H9: sólo STATE / ENV_* / PART_*) y en H10 se le enchufan el
 * FS real y la VM: es el mismo binario, crecido.
 *
 * Por qué importa: es el micro de pruebas estándar en PC (para nosotros y para
 * quien no tenga placa) y la referencia contra la que comparar cuando algo
 * "sólo falla en placa".
 *
 * UNA SOLA FUENTE DE VERDAD, por partes — nada aquí es una reimplementación:
 *   - gestión de placa (STATE / ENV_* / PART_* / PACK_*) → bpvm_bmgr_wire, el MISMO
 *     núcleo que los 3 firmwares ⇒ replies byte-idénticas por construcción.
 *   - FS → littlefs sobre un fichero imagen (fs_lfs_host), el MISMO motor que
 *     el micro; no es un FS de mentira.
 *   - ejecución → la VM-C de verdad (bpvm_run), con su resolución de imports,
 *     su overlay AOT (.mdn) y su KILL.
 * Lo único simulado es el silicio: la flash A/B del env es un fichero, la zona
 * de packs es RAM, y los tamaños (RAM/PSRAM/flash) los dices por línea de
 * comandos — que es justo el punto de H10.
 *
 * Uso:  bpvm-sim [puerto] [fichero-flash] [flashSizeBytes]        (posicional)
 *       bpvm-sim [--port=N] [--flash-file=F] [--flash=N] [--mem=N] [--psram=N]
 *                [--fs=IMG] [--fs-size=N] [--board=NOMBRE] [--screen=WxH]
 *                [--no-screen] [--pack=F]... [--zona-sucia]
 *   defaults: 127.0.0.1:5099, "boardsim.flash", 4 MB de flash, 512 KiB de RAM,
 *             imagen de FS "<fichero-flash>.fs".
 *   --pack=<f.pack> (repetible): "graba" la imagen en la zona de packs simulada
 *   (región RAM, 0xFF = NOR virgen) para que PACK_LS la liste y los imports
 *   resuelvan XIP, igual que el --pack= de la VM-C host.
 *   --zona-sucia: la zona empieza con RESTOS (como una partición recién creada sobre
 *   flash usada): está sin formatear, y el micro llega a la app en «casi» (V7/H7).
 *
 * Ver docs/BPVM_WIRE_PROTOCOL.md (catálogo de mensajes) y
 *     docs/H9_KERNEL_CAPAS.md §Comandos de gestión de placa.
 */
#include "bpvm.h"
#include "bpvm_adc.h"

#include "bpvm_io.h"   /* V6/A1.1: el hilo io */
#include "bpvm_internal.h"   /* vm->modules[].{name,imports,import_count} para deps */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_boot.h"
#include "bpvm_boot2.h"   /* V7/H7 (Boot 2, fase 4): las claves del reparto, las del arranque comun */
#include "bpvm_fs.h"
#include "bpvm_net.h"
#include "bpvm_pack.h"
#include "bpvm_dbg_wire.h"   /* #326: banco de pruebas del depurador en el micro simulado */
#include "bpvm_rtc.h"
#include "crc32.h"
#include "mdn_loader.h"
#include "bpvm_mdn_scan.h"   /* V7/H2 (#493): el escaner comun de .mdn (FS + zona) */
#include "aot_registry.h"
#include "json_min.h"
#include "bpvm_repl.h"      /* U3.24: el REPL comun */
#include "bpvm_sd_placa.h"  /* V7/H3: la SD de la placa (aqui, un fichero: --sd=IMAGEN) */
#include "bpvm_env.h"       /* U6.1: `stack=N` del ENV, como las placas */
#include "bpvm_machine.h"   /* V7/H10 #626: el ENV de Machine.getEnv */
#include "bpvm_bus.h"       /* V7/H6.c G4.a1 */
#include "bpvm_wire_v1.h"
#include "bpvm_platform.h"   /* V7/H8 #495 (paso 4): el cerrojo del cable */
#ifdef BPVM_GUI
#include "bpvm_entry.h"   /* #344 — el RUN, escrito una vez */
#include "bpvm_gui.h"   /* H10 — --screen=WxH / --no-screen */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef BPVM_SQLITE
/* V6 — EL SIMULADOR CON BASE DE DATOS (`make sim SQLITE=1`).
 *
 * Criterio de Eduardo: «¿es util para el programador? Si la respuesta es si, se
 * hace». Poder probar en el PC un programa que consulta una BD, sin grabar una
 * placa, lo es.
 *
 * El puente es EL MISMO que en una placa: `SQLite.mod` y `Orm.mod` viajan igual
 * —son bytecode, 2 bytes de seccion nativa— y sus 16 `native` se casan POR
 * NOMBRE con bpvm_aot_register_by_name. Lo unico que cambia es de donde sale el
 * thunk: en la placa del `.npk` reubicado, y aqui de codigo x86-64 enlazado en
 * este mismo binario, compilado del MISMO aot_SQLite.c que genera el compilador.
 *
 * ⚠️ Y EL ORDEN IMPORTA: `sqlite3_initialize` EXIGE que haya un vfs registrado y
 * FALLA EN SILENCIO si no lo hay (leccion del 8-ago). El vfs 'bp' se registra
 * desde dentro de `bpsql_publicar`, que por eso va ANTES que nada. */
#include "sqlite3.h"
#include "packglue.h"
extern void aot_SQLite_register(struct bpvm* vm);   /* generado por AotMain */
extern int  bpsql_publicar(const bpvm_bios_t* bios);/* sqlite_shim.c        */

#define SIM_SQL_ARENA (2u * 1024u * 1024u)
static unsigned char s_sql_arena_cruda[SIM_SQL_ARENA + 16];

static void sim_sqlite_arranca(void) {
    unsigned char* arena = (unsigned char*)
        (((uintptr_t) s_sql_arena_cruda + 7u) & ~(uintptr_t) 7u);
    if (sqlite3_config(SQLITE_CONFIG_HEAP, arena, (int) SIM_SQL_ARENA, 64) != SQLITE_OK) {
        fprintf(stderr, "sim: no se pudo dar la arena a SQLite\n"); return;
    }
    if (sqlite3_config(SQLITE_CONFIG_PMASZ, (unsigned) 64) != SQLITE_OK) {
        fprintf(stderr, "sim: no se pudo fijar PMASZ\n"); return;
    }
    int rc = bpsql_publicar(packglue_bios());   /* registra el vfs 'bp' */
    if (rc != 0) { fprintf(stderr, "sim: el pack de SQLite no publico: %d\n", rc); return; }
    packglue_callar(1);          /* el log del BIOS taparia la salida del programa */
}
#endif

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define BAD_SOCK INVALID_SOCKET
  #define close_sock closesocket
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define BAD_SOCK (-1)
  #define close_sock close
#endif

#define WIRE_LINE_MAX  2048   /* línea JSON máxima (= WIRE_V1_LINE_MAX del firmware) */
#define SERVER_NAME    "bpvm-sim"
/* #456 - los paths que el sim CONSTRUYE (el recorrido del LIST, la imagen de
 * FS de la linea de comandos). Los que llegan por el wire ya no se copian. */
#define PATH_MAX_SIM   BPVM_FS_PATH_MAX

/* --- "flash" simulada: dos sectores A/B en RAM, respaldados por un fichero --- */
#define SECTOR      4096u
#define PART_BASE   0x100000u   /* 1 MB reservado (imagen + env) */
#define DEF_FLASH   0x400000u   /* 4 MB por defecto (tipo Pico) */

static uint8_t g_a[SECTOR], g_b[SECTOR], g_scratch[SECTOR];
static bpvm_bmgr_t g_bm;
/* V7/H6.c G4.a1 — el ENV simulado, con la regla de siempre (entero o nada). */
static int sim_env_get(const char* clave, char* buf, size_t cap) { return bpvm_bmgr_env_get(&g_bm, clave, buf, cap); }
/* V7/H10 #626 — el ENV que ve el PROGRAMA (Machine.getEnv): el del ARRANQUE, como en la placa —una copia de las dos
 * del gestor, hecha al arrancar—. Un ENV_SET se ve al volver a lanzar el simulador sobre la misma flash, como en la
 * placa al reiniciar (el RESET del sim no rearranca: no hay silicio). */
static uint8_t    g_env_a0[SECTOR], g_env_b0[SECTOR];
static bpvm_env_t g_env0;
static int sim_env_arranque(const char* clave, char* buf, size_t cap) { return bpvm_env_get_exacto(&g_env0, clave, buf, cap); }
static const char* g_flash_path = "boardsim.flash";

/* H3 — zona de packs simulada (región RAM que hace de flash; 0xFF = borrada).
 * Se precarga con los --pack= y PACK_LS la sirve por el dispatch compartido. */
#define PACKS_REGION_SIZE (1024u * 1024u)
static uint8_t* g_packs = NULL;   /* tallada dentro del buffer de la VM (ver main) */
/* V7/H7 (Boot 2, fase 1) — el gancho del montaje (como el XIP fijo de la Pico y la STM32)
 * y la línea del log: la regla y el estado de la zona los pone el común. */
static int  sim_montar_packs(void);
static void sim_log_linea(const char* l) { fprintf(stderr, "sim: %s\n", l); }

/* ── H10: el "silicio" configurable ─────────────────────────────────────────
 * Son los mimbres que el IDE deja tocar: la RAM que gestiona la VM, la PSRAM
 * declarada (hoy informativa: la reporta INFO igual que una placa con PSRAM) y
 * el tamaño de la flash, del que salen las particiones. */
static size_t   g_mem_size   = 512u * 1024u;   /* RAM de la VM (--mem) */
static uint32_t g_psram_size = 0;              /* PSRAM declarada (--psram) */
static uint32_t g_flash_size = DEF_FLASH;      /* flash total (--flash) */
static uint8_t* g_vm_mem     = NULL;           /* buffer de la VM (RAM del micro) */
static const char* g_board   = NULL;           /* --board: identidad por defecto */

/* V7/H3 (24-sep) — LA RANURA DEL MICRO SIMULADO: un fichero hace de tarjeta (`--sd=IMAGEN`). Cumple el contrato
 * de bloque como el SPI de la Metro o el SDMMC del P4, así que todo lo de encima —el registro común, FAT,
 * `SD_INFO`/`SD_MOUNT`— es el MISMO código que en una placa con ranura. Sin `--sd`, el micro simulado no tiene
 * SD, y lo dice como una placa sin ranura. */
static const char* g_sd_path    = NULL;
static FILE*       g_sd_f       = NULL;
static uint32_t    g_sd_bloques = 0;
static const char* g_sd_motivo  = "OK";

static int simsd_init(void) {
    if (!g_sd_path) { g_sd_motivo = "sin imagen de tarjeta"; return -1; }
    if (!g_sd_f && !(g_sd_f = fopen(g_sd_path, "r+b"))) {
        g_sd_motivo = "no se pudo abrir la imagen de la tarjeta";
        return -1;
    }
    if (fseek(g_sd_f, 0, SEEK_END) != 0) { g_sd_motivo = "la imagen de la tarjeta no se deja medir"; return -1; }
    long n = ftell(g_sd_f);
    g_sd_bloques = n > 0 ? (uint32_t) ((unsigned long) n / BPVM_BLK_TAM) : 0u;
    if (g_sd_bloques == 0) { g_sd_motivo = "la imagen de la tarjeta esta vacia"; return -1; }
    g_sd_motivo = "OK";
    return 0;
}
static int simsd_mueve(uint32_t lba, uint32_t n) {
    if (!g_sd_f || (uint64_t) lba + n > g_sd_bloques) { g_sd_motivo = "fuera de la tarjeta"; return -1; }
    if (fseek(g_sd_f, (long) ((uint64_t) lba * BPVM_BLK_TAM), SEEK_SET) != 0) { g_sd_motivo = "fseek"; return -1; }
    return 0;
}
static int simsd_leer(uint32_t lba, uint32_t n, uint8_t* dst) {
    if (simsd_mueve(lba, n) != 0) return -1;
    if (fread(dst, BPVM_BLK_TAM, n, g_sd_f) != n) { g_sd_motivo = "lectura corta"; return -1; }
    return 0;
}
static int simsd_escribir(uint32_t lba, uint32_t n, const uint8_t* src) {
    if (simsd_mueve(lba, n) != 0) return -1;
    if (fwrite(src, BPVM_BLK_TAM, n, g_sd_f) != n) { g_sd_motivo = "escritura corta"; return -1; }
    return 0;
}
static int         simsd_hay_medio(void)   { return g_sd_path != NULL; }   /* la tarjeta del sim no se saca */
static uint32_t    simsd_bloques(void)     { return g_sd_bloques; }
static int         simsd_sincronizar(void) { return g_sd_f ? fflush(g_sd_f) : 0; }
static const char* simsd_motivo(void)      { return g_sd_motivo; }
/* Una tarjeta de mentira con identidad fija: lo que SD_INFO enseña se puede comprobar byte a byte. */
static int simsd_identidad(bpvm_blk_identidad_t* out) {
    memset(out, 0, sizeof *out);
    out->peldano    = g_sd_bloques ? 0 : -1;
    out->version    = 2;
    out->alta_cap   = 1;
    out->bloques    = g_sd_bloques;
    out->fabricante = 0x42;               /* 'B' */
    memcpy(out->oem, "BP", 3);
    memcpy(out->producto, "BPSIM", 6);
    out->serie      = 0xFDB1A2C3u;   /* #550: >= 2^31, como el PSN de una tarjeta de verdad (32 bits SIN signo) */
    out->anno       = 2026;
    out->mes        = 9;
    return 0;
}
static const bpvm_blk_backend_t SIM_SD = {
    .init        = simsd_init,
    .leer        = simsd_leer,
    .escribir    = simsd_escribir,
    .hay_medio   = simsd_hay_medio,
    .bloques     = simsd_bloques,
    .sincronizar = simsd_sincronizar,
    .motivo      = simsd_motivo,
    .identidad   = simsd_identidad,
};
static int      g_screen_w   = 0;              /* --screen=WxH (0 = el default de gui.c) */
static int      g_screen_h   = 0;
static int      g_no_screen  = 0;              /* --no-screen: placa sin panel */

/* El cliente: el cable del simulado. La sesión, el ack del KILL y el del RESET eran de aquí; desde el RUN común (V7/H10,
 * paso 2) son de src/bpvm_repl.c, como en las placas que lo usen. */
static sock_t g_cli         = BAD_SOCK;

/* Cintura de "flash" del sim para el BURN (erase/program sobre la región RAM).
 * Bloque de borrado 4K (como Pico/ESP; el STM32 real usa 8K). */
static int sim_pack_erase(void* u, uint32_t off, uint32_t len) {
    (void) u;
    if (off + len > PACKS_REGION_SIZE) return -1;
    memset(g_packs + off, 0xFF, len);
    return 0;
}
static int sim_pack_program(void* u, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) u;
    if (off + len > PACKS_REGION_SIZE) return -1;
    memcpy(g_packs + off, d, len);
    return 0;
}
static const bpvm_pack_flash_t g_packs_fl = { sim_pack_erase, sim_pack_program, NULL, 4096u };

static int packs_preload(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "sim: --pack: no puedo abrir %s\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* img = (sz > 0) ? (uint8_t*) malloc((size_t) sz) : NULL;
    if (!img || fread(img, 1, (size_t) sz, f) != (size_t) sz) {
        fprintf(stderr, "sim: --pack: error leyendo %s\n", path);
        free(img); fclose(f); return -1;
    }
    fclose(f);
    int32_t off = bpvm_pack_add(g_packs, PACKS_REGION_SIZE, img, (uint32_t) sz);
    free(img);
    if (off < 0) {
        fprintf(stderr, "sim: --pack: %s %s\n", path,
                (off == BPVM_PACK_ERR_BADIMG) ? "no es un pack valido" : "no cabe");
        return -1;
    }
    printf("sim: pack %s grabado en 0x%06X\n", path, (unsigned) off);
    return 0;
}

static void flash_load(void) {
    memset(g_a, 0xFF, SECTOR);   /* borrada = placa virgen */
    memset(g_b, 0xFF, SECTOR);
    FILE* f = fopen(g_flash_path, "rb");
    if (!f) return;
    if (fread(g_a, 1, SECTOR, f) != SECTOR) memset(g_a, 0xFF, SECTOR);
    if (fread(g_b, 1, SECTOR, f) != SECTOR) memset(g_b, 0xFF, SECTOR);
    fclose(f);
}

static void flash_store(void) {   /* la "cintura de flash" del sim */
    FILE* f = fopen(g_flash_path, "wb");
    if (!f) { perror("sim: no puedo escribir la flash"); return; }
    fwrite(g_a, 1, SECTOR, f);
    fwrite(g_b, 1, SECTOR, f);
    fclose(f);
}

/* --- string builder minúsculo para construir replies JSON --- */
typedef struct { char* buf; size_t cap, off; int ok; } sb_t;
static void sb_init(sb_t* s, char* buf, size_t cap) { s->buf = buf; s->cap = cap; s->off = 0; s->ok = 1; }
static void sb_raw(sb_t* s, const char* str) {
    if (!s->ok) return;
    size_t n = strlen(str);
    if (s->off + n >= s->cap) { s->ok = 0; return; }
    memcpy(s->buf + s->off, str, n);
    s->off += n;
    s->buf[s->off] = '\0';   /* siempre NUL-terminado (send_line usa strlen) */
}
static void sb_long(sb_t* s, long v) { char t[24]; snprintf(t, sizeof t, "%ld", v); sb_raw(s, t); }
static void sb_ulong(sb_t* s, unsigned long v) { char t[24]; snprintf(t, sizeof t, "%lu", v); sb_raw(s, t); }
static void sb_esc(sb_t* s, const char* str) {   /* string JSON escapado (sin comillas) */
    if (!s->ok) return;
    for (const char* p = str; *p; p++) {
        char c = *p;
        const char* rep = NULL; char u[8];
        switch (c) {
        case '"':  rep = "\\\""; break;
        case '\\': rep = "\\\\"; break;
        case '\n': rep = "\\n";  break;
        case '\r': rep = "\\r";  break;
        case '\t': rep = "\\t";  break;
        default:
            if ((unsigned char) c < 0x20) { snprintf(u, sizeof u, "\\u%04x", c); rep = u; }
            break;
        }
        if (rep) sb_raw(s, rep);
        else {
            if (s->off + 1 >= s->cap) { s->ok = 0; return; }
            s->buf[s->off++] = c;
            s->buf[s->off] = '\0';
        }
    }
}

/* --- I/O de socket (framing wire v1: línea + '\n') --- */
static int send_all(sock_t c, const char* data, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        int r = send(c, data + sent, (int)(n - sent), 0);
        if (r <= 0) return -1;
        sent += (size_t) r;
    }
    return 0;
}

/* V7/H8 #495 (paso 4) — EL CERROJO DEL CABLE, que el simulado no tenía. Escriben en el socket el hilo del REPL (y,
 * durante un RUN, la VM contestando al depurador) y el hilo `io` (la salida del programa y lo que contesta su poll):
 * las placas lo tienen desde #473, y con las respuestas troceadas que manda `io` la línea de un hilo podría caer
 * dentro del mensaje del otro. No es recursivo: dentro, `send_all`, nunca `send_line`. Se crea en `main`, antes del
 * primer cliente. */
static bpvm_platform_mutex_handle_t s_tx_mtx;
static void tx_lock(void)   { bpvm_platform_mutex_lock(&s_tx_mtx); }
static void tx_unlock(void) { bpvm_platform_mutex_unlock(&s_tx_mtx); }

static void send_line(sock_t c, const char* line) {
    tx_lock();
    if (send_all(c, line, strlen(line)) == 0) send_all(c, "\n", 1);
    tx_unlock();
}
static void send_err(sock_t c, long id, const char* code, const char* msg) {
    char buf[512]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"ERROR\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"code\":\""); sb_esc(&s, code);
    sb_raw(&s, "\",\"message\":\""); sb_esc(&s, msg); sb_raw(&s, "\"}");
    if (s.ok) send_line(c, s.buf);
}
static void send_ok(sock_t c, const char* type, long id) {
    char buf[96]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\""); sb_raw(&s, type);
    sb_raw(&s, "\",\"id\":"); sb_long(&s, id); sb_raw(&s, "}");
    if (s.ok) send_line(c, s.buf);
}
static int recv_exact(sock_t c, uint8_t* buf, size_t n);   /* U3.24: se usa abajo */

/* ── V6/U3.24 — EL CONTRATO DEL WIRE, para poder usar el REPL COMUN ──────────
 *
 * POR QUE. Este simulador tenia VEINTIUN verbos propios: una implementacion
 * paralela entera del REPL, en el host, que `sim_smoke.py` ejercita por el wire.
 * O sea que existia la herramienta capaz de cazar un fallo del REPL comun en
 * segundos... y miraba otro codigo.
 *
 * Lo destapo el bug de `U3.23`: el `LIST` comun no contestaba, y hubo que verlo
 * en una P4. El Makefile ya lo tenia escrito — «los CUATRO firmwares (+ el
 * simulador cuando migre)».
 *
 * El contrato pide solo DOS funciones de la familia; todo lo demas (los
 * constructores JSON, los errores) ya vive en `src/wire_v1_proto.c`. Aqui el
 * "transporte" es el socket del cliente en curso, que ya era global (`g_cli`). */
void wire_v1_send_line(const char* data, size_t len) {
    if (g_cli == BAD_SOCK) return;
    tx_lock();
    if (len == 0 || send_all(g_cli, data, len) == 0) (void) send_all(g_cli, "\n", 1);
    tx_unlock();
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    if (g_cli == BAD_SOCK || n == 0) return;
    tx_lock();
    (void) send_all(g_cli, (const char*) data, n);
    tx_unlock();
}

/* #495 (paso 4) — la línea y sus bytes con el cerrojo cogido UNA vez: lo que tiene que ir junto (bpvm_wire_v1.h). */
void wire_v1_send_msg(const char* line, size_t len, const uint8_t* bulk, size_t n) {
    if (g_cli == BAD_SOCK) return;
    tx_lock();
    if ((len == 0 || send_all(g_cli, line, len) == 0) && send_all(g_cli, "\n", 1) == 0 && n > 0)
        (void) send_all(g_cli, (const char*) bulk, n);
    tx_unlock();
}

/* V7/H8 #495 — el contrato del bulk (bpvm_wire_v1.h): -2 si se estanca WIRE_V1_BULK_ESTANCADO_MS sin
 * recibir un byte. Por TCP no se pierden bytes, pero un cliente puede quedarse a medias sin cerrar; y así el
 * segundo intento del IDE se prueba aquí igual que en una placa. */
int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    size_t got = 0;
    if (n > buf_max) return -1;
    while (got < n) {
        fd_set rd;
        struct timeval tv;
        FD_ZERO(&rd);
        FD_SET(g_cli, &rd);
        tv.tv_sec  = (long) (WIRE_V1_BULK_ESTANCADO_MS / 1000u);
        tv.tv_usec = (long) ((WIRE_V1_BULK_ESTANCADO_MS % 1000u) * 1000u);
        int s = select((int) g_cli + 1, &rd, NULL, NULL, &tv);
        if (s == 0) return -2;                          /* estancado */
        if (s < 0) return -1;
        int r = recv(g_cli, (char*) buf + got, (int) (n - got), 0);
        if (r <= 0) return -1;
        got += (size_t) r;
    }
    return (int) n;
}

/* Lee una línea (hasta '\n', descarta '\r'). Devuelve len o -1 si el peer cierra.
 * V7/H8 #495 — y una línea a medias que se ESTANCA WIRE_V1_ESTANCADA_MS se tira en silencio y se sigue
 * esperando, como en las placas (bpvm_wire_v1.h): sin eso, la basura de una subida rota se queda a medias y
 * el mensaje siguiente —el HELLO del segundo intento del IDE— se le pega detrás y se pierde. */
static int recv_line(sock_t c, char* buf, size_t cap) {
    size_t n = 0;
    for (;;) {
        if (n > 0) {
            fd_set rd;
            struct timeval tv;
            FD_ZERO(&rd);
            FD_SET(c, &rd);
            tv.tv_sec  = (long) (WIRE_V1_ESTANCADA_MS / 1000u);
            tv.tv_usec = (long) ((WIRE_V1_ESTANCADA_MS % 1000u) * 1000u);
            int s = select((int) c + 1, &rd, NULL, NULL, &tv);
            if (s == 0) { n = 0; continue; }            /* estancada: se tira */
            if (s < 0) return -1;
        }
        char ch; int r = recv(c, &ch, 1, 0);
        if (r <= 0) return -1;
        if (ch == '\n') { buf[n < cap ? n : cap - 1] = '\0'; return (int) n; }
        if (ch == '\r') continue;
        if (n < cap - 1) buf[n++] = ch;
    }
}
/* Lee EXACTAMENTE n bytes crudos (el bulk que sigue a una línea con "bulk":N). */
static int recv_exact(sock_t c, uint8_t* buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        int r = recv(c, (char*) buf + got, (int) (n - got), 0);
        if (r <= 0) return -1;
        got += (size_t) r;
    }
    return 0;
}
/* ¿hay algo pendiente de leer? (poll del KILL durante un RUN, sin bloquear). */
static int sock_has_data(sock_t c) {
    fd_set rd;
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 0;
    FD_ZERO(&rd);
    FD_SET(c, &rd);
    return select((int) c + 1, &rd, NULL, NULL, &tv) > 0;
}
/* Traga y descarta `n` bytes de bulk. CRÍTICO cuando el request se rechaza: si
 * no se consumen, el wire queda desincronizado y todo lo siguiente es basura. */
static int drain_bulk(sock_t c, long n) {
    uint8_t sink[4096];
    while (n > 0) {
        size_t chunk = (n < (long) sizeof sink) ? (size_t) n : sizeof sink;
        if (recv_exact(c, sink, chunk) != 0) return -1;
        n -= (long) chunk;
    }
    return 0;
}

/* ── FS: helpers sobre la fachada (littlefs sobre imagen) ─────────────────── */

/* Lee un fichero entero a un buffer nuevo (free() del llamante). NULL si no. */
static uint8_t* fs_read_all(const char* path, uint32_t* size_out) {
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) return NULL;
    uint8_t* buf = (uint8_t*) malloc(size ? size : 1);
    if (!buf) return NULL;
    if (size > 0 && bpvm_fs_read(path, buf, size) != (long) size) { free(buf); return NULL; }
    *size_out = size;
    return buf;
}

/* Crea los directorios intermedios de `path`. littlefs es JERÁRQUICO: un write
 * a "/app/proj/x.mod" falla si /app/proj no existe, y el IDE sube rutas
 * completas dando por hecho que el device se apaña (así lo hacen los firmwares,
 * de FS plano). */
/* ── Recorrido del árbol del FS ────────────────────────────────────────────
 * littlefs es jerárquico pero el wire lista PLANO con rutas completas
 * ("/lib/Math.mod") — es lo que mandan los firmwares y lo que el árbol del IDE
 * espera para que un GET/DEL posterior acierte.
 *
 * DOS FASES, y no es un capricho: el callback de bpvm_fs_list corre BAJO el
 * lock grueso del FS (lo dice fs_lfs.c: "el cb NO debe re-entrar en la
 * fachada"). Leer el fichero para calcular su CRC desde dentro del cb es un
 * AUTOBLOQUEO — el mutex no es recursivo y el sim se queda colgado sin decir ni
 * mu. Así que fase 1 sólo ACUMULA nombres, y fase 2 lee/emite ya fuera. Por lo
 * mismo se recorre en anchura con cola explícita, sin recursión. */
#define WALK_MAX_DIRS     64
#define WALK_MAX_ENTRIES 512

typedef struct { uint32_t used; int count; int truncated; } fs_tally_t;   /* #425: truncated CUENTA, no es bandera */
static fs_tally_t g_tally;

typedef struct { char path[PATH_MAX_SIM]; uint32_t size; } fs_ent_t;
static fs_ent_t g_ents[WALK_MAX_ENTRIES];
static int      g_n_ents;
static char     g_dirs[WALK_MAX_DIRS][PATH_MAX_SIM];
static int      g_n_dirs;

/* Fase 1 — SÓLO acumula. Prohibido llamar a la fachada desde aquí (ver arriba). */
static void collect_cb(const char* name, int is_dir, uint32_t size, void* user) {
    const char* parent = (const char*) user;
    char full[PATH_MAX_SIM];
    size_t plen = strlen(parent);
    int need_slash = (plen > 0 && parent[plen - 1] != '/');
    snprintf(full, sizeof full, "%s%s%s", parent, need_slash ? "/" : "", name);

    if (is_dir) {
        if (g_n_dirs < WALK_MAX_DIRS) snprintf(g_dirs[g_n_dirs++], PATH_MAX_SIM, "%s", full);
        else g_tally.truncated++;   /* #425 */
        return;
    }
    if (g_n_ents < WALK_MAX_ENTRIES) {
        snprintf(g_ents[g_n_ents].path, PATH_MAX_SIM, "%s", full);
        g_ents[g_n_ents].size = size;
        g_n_ents++;
    } else {
        g_tally.truncated++;   /* #425 */
    }
}

/* Recorre TODO el árbol desde "/". Con sb != NULL emite las entries del
 * LIST_REPLY; siempre deja el tally (bytes usados + nº de ficheros) en g_tally. */
static void fs_walk(sb_t* sb) {
    g_n_dirs = 1; g_n_ents = 0;
    g_tally.used = 0; g_tally.count = 0; g_tally.truncated = 0;
    snprintf(g_dirs[0], PATH_MAX_SIM, "%s", "/");

    /* Fase 1: recorrer (el índice avanza sobre una cola que crece). */
    for (int i = 0; i < g_n_dirs && i < WALK_MAX_DIRS; i++) {
        /* #473 (lote D, 248; 3-oct): sólo el volumen propio, como `bpvm_fs_count_files`: con `--sd`, lo usado y la
         * cuenta del INFO y del DF sumaban los ficheros de la tarjeta. */
        if (i > 0 && !bpvm_fs_en_raiz(g_dirs[i])) continue;
        bpvm_fs_list(g_dirs[i], collect_cb, g_dirs[i]);
    }

    /* Fase 2: emitir.
     *
     * #398 — AQUÍ YA NO SE CALCULA EL CRC, y el `-1` es deliberado.
     *
     * El CRC servía para que el IDE se saltara un PUT cuyo contenido ya está en
     * el device: una optimización de la SUBIDA que se cobraba en TODOS los
     * listados, leyendo el FS entero cada vez que se refresca el árbol. Medido
     * en la P4 (15-ago): 6903 ms de 6953 — el 99 % del refresco.
     *
     * Ahora el CRC se pide fichero a fichero con `STAT {crc:true}`, que es
     * cuando de verdad hace falta: justo antes de subir ESE fichero.
     *
     * `-1` no es "cero": es el valor que el IDE ya interpretaba como «este
     * firmware no da CRC» (`PicoExplorer`: `if (rf.crc >= 0)`), así que un IDE
     * viejo contra este sim degrada al heurístico de siempre en vez de creerse
     * un CRC falso y saltarse una subida que hacía falta. */
    for (int i = 0; i < g_n_ents; i++) {
        g_tally.used += g_ents[i].size;
        g_tally.count++;
        if (!sb) continue;
        if (i > 0) sb_raw(sb, ",");
        sb_raw(sb, "{\"name\":\""); sb_esc(sb, g_ents[i].path);
        sb_raw(sb, "\",\"size\":");   sb_ulong(sb, (unsigned long) g_ents[i].size);
        sb_raw(sb, ",\"crc\":-1");
        sb_raw(sb, ",\"isDir\":false,\"mtime\":0}");
    }
    /* Si se truncó, que se SEPA: un listado corto silencioso se lee como "no hay
     * más ficheros" y el IDE tomaría decisiones sobre una foto incompleta. */
    if (g_tally.truncated)
        printf("%s: aviso: el arbol del FS excede %d ficheros / %d directorios;"
               " el listado va INCOMPLETO\n", SERVER_NAME, WALK_MAX_ENTRIES, WALK_MAX_DIRS);
}

/* Tamaño de la partición FS según la tabla de particiones (0 = placa virgen). */
static uint32_t fs_partition_size(void) {
    bpvm_part_layout_t lay;
    if (bpvm_bmgr_part_layout(&g_bm, SECTOR, &lay, NULL) != BPVM_PART_OK) return 0;
    const bpvm_part_t* p = bpvm_part_get(&lay, BPVM_PART_FS);
    return p ? p->size : 0;
}

/* ── V6/U3.24 — LA CINTURA DEL SIMULADOR ────────────────────────────────────
 *
 * A partir de aquí el sim atiende los verbos de META y FILES con el MISMO
 * código que las cuatro placas (`src/bpvm_repl.c`). Sus handlers propios se
 * han borrado, no sombreado: un handler propio que gane al común deja el común
 * sin ejercitar, que es exactamente el agujero por el que se coló el bug de
 * `U3.23` (el `LIST` que no contestaba, y hubo que verlo en una P4).
 *
 * Lo que el sim sigue poniendo de su parte es lo que de verdad es suyo: el
 * silicio de mentira que se pide por línea de comandos (`--mem`, `--psram`,
 * `--flash`, `--screen`) y el formateo, que aquí es cerrar la imagen, borrar
 * el fichero y volver a montar. */

static uint32_t fs_partition_size(void);      /* definido más abajo */
static void     fs_walk(sb_t* sb);

static void sim_repl_info(bpvm_repl_info_t* out) {
    char idv[64];
    int has = bpvm_bmgr_env_get(&g_bm, "board", idv, sizeof idv) >= 0;
    static char s_board[64];
    snprintf(s_board, sizeof s_board, "%s", has ? idv : (g_board ? g_board : "sim"));
    fs_walk(NULL);

    out->unique_id    = "SIMULATED0000000";
    out->board_name   = s_board;
    out->reset_reason = "sim";
    out->arch         = 0;
    out->cpu_hz       = 0;
    out->uptime_ms    = (unsigned long) ((unsigned long) clock() * 1000UL / CLOCKS_PER_SEC);
    out->flash_bytes  = (unsigned long) g_flash_size;
    out->sram_bytes   = (unsigned long) g_mem_size;
    out->psram_bytes  = (unsigned long) g_psram_size;
    out->fs_total_bytes = (unsigned long) fs_partition_size();
    out->fs_used_bytes  = (unsigned long) g_tally.used;
    /* V6/U6.1 — el reparto de la VM, que iba a CERO mientras las placas daban el
     * numero real. Es el dato con el que se compara "cuanta memoria tiene el
     * programa aqui y alli", asi que un cero en el arnes es peor que un numero
     * feo: parece un dato y no lo es. Misma regla que las tres familias. */
    {
        size_t pilas = bpvm_stack_region_bytes(g_mem_size);
        out->vm_stack_bytes = (unsigned long) pilas;
        out->vm_heap_bytes  = (unsigned long) (g_mem_size - pilas);
    }
}

/* H10 — el panel simulado, para que el IDE pueda mostrarlo (0x0 = sin pantalla).
 * Es el único campo propio del sim, y por eso entra por el gancho de extras. */
static int sim_repl_info_extra(char* buf, unsigned long buf_max, int off) {
    off = wire_v1_field_long(buf, (size_t) buf_max, (size_t) off, "screenW",
                             g_no_screen ? 0 : (g_screen_w > 0 ? g_screen_w : 480));
    if (off < 0) return -1;
    return wire_v1_field_long(buf, (size_t) buf_max, (size_t) off, "screenH",
                              g_no_screen ? 0 : (g_screen_h > 0 ? g_screen_h : 320));
}

static unsigned long sim_fs_total(void) { return (unsigned long) fs_partition_size(); }
static unsigned long sim_fs_used(void)  { fs_walk(NULL); return g_tally.used; }
static int           sim_fs_count(void) { fs_walk(NULL); return g_tally.count; }

static char     g_fs_img[PATH_MAX_SIM] = "";
static unsigned g_fs_blocks = 0;

static int sim_fs_format(void) {
    bpvm_fs_lfs_filebd_close();
    remove(g_fs_img);                       /* imagen virgen → el mount la formatea */
    return bpvm_fs_register_lfs_filebd(g_fs_img, SECTOR, g_fs_blocks, 1);
}

/* El scratch del PUT. 64 KB porque aquí no cuesta nada y es lo que había: el
 * sim usaba `malloc(bulk)` sin tope, y el IDE manda de una pieza hasta 40 KB
 * (por encima trocea con PUT_BEGIN/DATA/END). Un buffer más apretado que el
 * umbral del IDE convertiría el sim en más estricto que cualquier placa. */
static unsigned char g_sim_put_buf[64 * 1024];
/* V7/H8 #495 — el búfer de FICHEROS del PUT troceado, aparte del de comunicaciones: un bloque del FS (SECTOR). */
static unsigned char g_sim_fich_buf[SECTOR];

static const bpvm_repl_run_ops_t SIM_RUN_OPS;   /* V7/H10: la cintura del RUN, más abajo */

/* V7/H10, el RUN común, paso 0 (3-oct): con NOMBRES, como las cintas de las tres familias. En orden, cada campo
 * nuevo de bpvm_repl_ops_t obligaba a contar posiciones (y faltaba el último: el aviso de «missing initializer»). */
static const bpvm_repl_ops_t SIM_REPL_OPS = {
    .info           = sim_repl_info,
    .server_name    = SERVER_NAME,
    .server_build   = __DATE__ " " __TIME__,
    .capabilities   = "[\"META\",\"FILES\",\"TERMINAL\",\"BOARDMGR\",\"PACKS\"]",
    .put_buf        = g_sim_put_buf,
    .put_buf_size   = sizeof g_sim_put_buf,
    .after_put      = NULL,                 /* littlefs ya persiste */
    .info_extra     = sim_repl_info_extra,
    .fs_total_bytes = sim_fs_total,
    .fs_used_bytes  = sim_fs_used,
    .fs_file_count  = sim_fs_count,
    .fs_format      = sim_fs_format,
    .fs_save        = NULL,                 /* idem — el común contesta OK, que es la verdad */
    .fich_buf       = g_sim_fich_buf,       /* V7/H8 #495: el búfer de ficheros, un bloque (SECTOR) */
    .fich_bytes     = sizeof g_sim_fich_buf,
    .put_trozo      = 512,                  /* V7/H8 #495: el trozo y la ventana del HELLO, los de las placas */
    .put_ventana    = 4,
    .image_crc      = NULL,                 /* V7/H9: el sim no tiene imagen que sellar */
    .run            = &SIM_RUN_OPS,         /* V7/H10: el RUN común (el sim lo estrena) */
};

/* ── META ─────────────────────────────────────────────────────────────────── */

/* ── FILES ────────────────────────────────────────────────────────────────── */


/* Datos de la imagen del FS: hacen falta para re-formatear (cerrar → borrar el
 * fichero → volver a montar formateando), que es como se formatea una flash. */
/* ── TERMINAL: RUN / KILL ─────────────────────────────────────────────────── *
 * V7/H10, el RUN común, paso 2 (3-oct): aquí vivían el sink de OUTPUT (cortaba en el primer NUL), el EXITED y el poll
 * del RUN; los tres son del común (`bpvm_repl_run`, `wire_v1_salida`). De esta familia queda su cintura, más abajo. */

/* Resuelve un nombre de módulo en el FS: base-dir del proyecto, tal cual,
 * /app/<name>, /lib/<name> (el IDE sube las deps a /lib). Mismo orden que los
 * firmwares — si aquí y allí no buscasen igual, el sim dejaría de ser espejo. */
static uint8_t* sim_fs_resolve(const char* name, uint32_t* size) {
    char p[PATH_MAX_SIM];
    const char* bd = bpvm_fs_basedir();
    uint8_t* d;
    if (bd && bd[0] && name[0] != '/') {
        snprintf(p, sizeof p, "%s/%s", bd, name);
        if ((d = fs_read_all(p, size)) != NULL) return d;
    }
    if ((d = fs_read_all(name, size)) != NULL) return d;
    snprintf(p, sizeof p, "/app/%s", name);
    if ((d = fs_read_all(p, size)) != NULL) return d;
    snprintf(p, sizeof p, "/lib/%s", name);
    if ((d = fs_read_all(p, size)) != NULL) return d;
    return NULL;
}

/* V7/H2 (#493) — la cintura del escáner común de `.mdn`: del FS a un buffer que
 * vive todo el RUN (keep_buf). En el PC cualquier memoria vale para «ejecutar»:
 * el gate de arch rechaza el blob de todas formas (x64 frente a ARM/RISC-V). */
static void keep_buf(uint8_t* p);
static const uint8_t* sim_mdn_del_fs(void* user, const char* nombre, uint32_t* len) {
    (void) user;
    uint32_t msz = 0;
    uint8_t* mdn = sim_fs_resolve(nombre, &msz);
    if (!mdn) return NULL;
    keep_buf(mdn);
    *len = msz;
    return mdn;
}

/* Los buffers de los .mod cargados tienen que seguir VIVOS mientras corre la VM
 * (el loader NO copia el código: apunta al buffer). Se liberan al terminar. */
#define MAX_LOADED_BUFS 64
static uint8_t* g_bufs[MAX_LOADED_BUFS];
static int      g_n_bufs = 0;
static void keep_buf(uint8_t* p) {
    if (g_n_bufs < MAX_LOADED_BUFS) g_bufs[g_n_bufs++] = p; else free(p);
}
static void free_bufs(void) {
    for (int i = 0; i < g_n_bufs; i++) free(g_bufs[i]);
    g_n_bufs = 0;
}

/* Nombre del módulo dueño de un import ("Gui.Button" → "Gui"). */
static void import_owner(const char* imp, char* out, size_t cap) {
    size_t i = 0;
    while (imp[i] && imp[i] != '.' && i < cap - 1) { out[i] = imp[i]; i++; }
    out[i] = '\0';
}

static int module_loaded(const bpvm_t* vm, const char* name) {
    for (int j = 0; j < vm->module_count; j++)
        if (strcmp(vm->modules[j].name, name) == 0) return 1;
    return 0;
}

/* ── V7/H10, el RUN común, paso 2 (3-oct): LA CINTURA DEL SIMULADO ──────────────────────────────────────────────────
 *
 * El primero que estrena `bpvm_repl_run` (src/bpvm_repl.c). Aquí vivían su RUN, su depurador (#326: el banco de
 * pruebas del de la Pico) y su poll —y el depurador y el poll leían el socket A LA VEZ—. Ahora el cable lo lee `io`
 * (el poll común) y los comandos del depurador le llegan a la VM por la cola de control, como en las placas. De esta
 * familia queda lo que es suyo de verdad: */

/* el bloque de la VM: el del micro simulado (--mem) */
static int sim_run_memoria(uint8_t** base, size_t* bytes) {
    *base = g_vm_mem;
    *bytes = g_mem_size;
    return 0;
}

/* una línea del socket SIN ESPERAR (si hay algo, la línea entera); <0, el cliente se ha ido */
static int sim_run_leer_linea(char* buf, size_t cap) {
    if (g_cli == BAD_SOCK) return -1;
    if (!sock_has_data(g_cli)) return 0;
    if (recv_line(g_cli, buf, cap) < 0) return -1;
    return (int) strlen(buf);
}

static char s_run_linea[WIRE_LINE_MAX];       /* la de `io` */
static char s_run_dbg_linea[WIRE_LINE_MAX];   /* la del depurador: un búfer por rol (A1.7) */
static char s_run_mensaje[2048];              /* RUN_REPLY, EXITED y las respuestas del depurador */

/* el RESET pedido durante el RUN: no hay silicio que reiniciar; se contesta y se corta, que es lo que ve el IDE */
static void sim_run_reiniciar(long id) {
    send_ok(g_cli, "RESET_REPLY", id);
    close_sock(g_cli);
    g_cli = BAD_SOCK;
}

/* los nativos que no vienen en un .mdn: el puente de SQLite (con SQLITE=1), por NOMBRE, con el módulo ya cargado */
static void sim_run_tras_cargar(bpvm_t* vm) {
#ifdef BPVM_SQLITE
    aot_SQLite_register(vm);
#else
    (void) vm;
#endif
}

/* los .mdn que trajo el escáner viven todo el RUN (keep_buf); al acabar, fuera */
static void sim_run_despues(bpvm_t* vm) {
    (void) vm;
    free_bufs();
}

/* el ENV del simulado, para `gc` y `latido` */
static int sim_run_env_bool(const char* clave, int defecto) {
    char v[8];
    if (sim_env_get(clave, v, sizeof v) < 0) return defecto;
    return v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 's' || v[0] == 'S';
}

/* el consejo cuando falta un módulo: una placa trae la stdlib embebida; el simulado arranca con el FS vacío y
 * necesita el pack de la stdlib */
static const char* sim_run_consejo(void) {
    uint32_t pk_rs = 0;
    return (bpvm_pack_mounted(&pk_rs) != NULL)
        ? "no esta en el FS ni en los packs grabados. Comprueba que el pack que lo trae este en la libreria de packs."
        : "no esta en el FS y no hay ningun pack grabado. Si es de la libreria estandar, configura la libreria de "
          "packs (engranaje del micro simulado) y reinicia el simulador.";
}

static const bpvm_repl_run_ops_t SIM_RUN_OPS = {
    .memoria           = sim_run_memoria,
    .leer_linea        = sim_run_leer_linea,
    .linea             = s_run_linea,     .linea_cap     = sizeof s_run_linea,
    .dbg_linea         = s_run_dbg_linea, .dbg_linea_cap = sizeof s_run_dbg_linea,
    .mensaje           = s_run_mensaje,   .mensaje_cap   = sizeof s_run_mensaje,
    .reiniciar         = sim_run_reiniciar,
    .mdn_del_fs        = sim_mdn_del_fs,
    .tras_cargar       = sim_run_tras_cargar,
    .despues_de_correr = sim_run_despues,
    .env_bool          = sim_run_env_bool,
    .consejo_falta     = sim_run_consejo,
    .io_cola           = 0,               /* la de por defecto */
};

/* ── dispatch de un request ───────────────────────────────────────────────── */


static void handle(sock_t c, json_obj_t* obj) {
    char type[32];
    if (json_get_str(obj, "type", type, sizeof type) < 0) return;
    long id = json_get_long(obj, "id", -1);
    bpvm_repl_antes_de_despachar(type);   /* #495: un verbo que no convive con una subida a medias, la acaba */

    /* #326 — ramo de depuración (pre-RUN): SET_BP / CLR_BP / PAUSE. El núcleo
     * acumula y contesta; si no es de su ramo devuelve 0 y seguimos. */
    if (bpvm_repl_depurador_previo(type, id, obj)) return;   /* V7/H10: el depurador común */

    /* U3.24 — EL COMUN PRIMERO. META (HELLO/PING/INFO/TIME/LOG_*) y FILES
     * (LIST, LIST_DIR, STAT, DF, GET, los tres PUT, DEL, MKDIR, RMDIR, RENAME,
     * FORMAT y SAVE) los
     * atiende `src/bpvm_repl.c`, el mismo fichero que las cuatro placas. Abajo
     * solo queda lo que es de verdad del sim. */
    if (bpvm_repl_dispatch(type, id, obj)) return;

    /* META */
    if (!strcmp(type, "RESET")) {
        /* No hay silicio que rebootear: se contesta y se corta la conexión, que
         * es lo que ve el IDE en una placa. El estado vive en la "flash". */
        send_ok(c, "RESET_REPLY", id);
        close_sock(c);
        g_cli = BAD_SOCK;
        return;
    }

    /* TERMINAL */
    /* V7/H10, el RUN común, paso 2 (3-oct). Y el KILL sin programa, NO_SESSION como en las placas (regla del 2-oct:
     * aquí contestaba KILL_REPLY). */
    if (!strcmp(type, "RUN"))  { bpvm_repl_run(id, json_str_inplace(obj, "path"), json_str_inplace(obj, "arg")); return; }
    if (!strcmp(type, "KILL")) { wire_v1_send_error(id, "NO_SESSION", "no hay programa en ejecución"); return; }

    /* --- gestión de placa (STATE, ENV_x, PART_x, PACK_x): NÚCLEO COMPARTIDO con el
     *     firmware. El sim solo parsea el JSON al `req`, despacha, y —si hubo escritura—
     *     vuelca la "flash" (el fichero A/B). Las replies las construye bpvm_bmgr_wire →
     *     byte-idénticas a las del device. --- */
    if (!strcmp(type, "STATE") || !strncmp(type, "ENV_", 4) || !strncmp(type, "PART_", 5)
        || !strncmp(type, "PACK_", 5)) {
        bpvm_bmgr_req_t req;
        memset(&req, 0, sizeof req);
        snprintf(req.type, sizeof req.type, "%s", type);
        req.id = id;
        req.has_key   = json_get_str(obj, "key",   req.key,   sizeof req.key)   >= 0;
        req.has_value = json_get_str(obj, "value", req.value, sizeof req.value) >= 0;
        for (int i = 0; i < BPVM_PART_COUNT; i++)
            req.part_sizes[i] = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);
        req.off = json_get_long(obj, "offset", -1);     /* H3: PACK_ENTRIES/DEL/READ */
        req.has_off = req.off >= 0;
        req.size = json_get_long(obj, "size", -1);      /* H3: PACK_BURN_BEGIN */
        req.has_size = req.size >= 0;
        {                                               /* H3: PACK_FORMAT (confirm=YES) */
            char confirm[8];
            req.confirm_yes = json_get_str(obj, "confirm", confirm, sizeof confirm) >= 0
                              && strcmp(confirm, "YES") == 0;
        }
        /* H3 — bulk crudo tras la línea (PACK_BURN_DATA): leerlo SIEMPRE que se
         * anuncie, aunque sobre, para no desincronizar el wire. */
        static uint8_t s_bulk[BPVM_PACK_BURN_CHUNK];
        long bulk = json_get_long(obj, "bulk", -1);
        if (bulk > 0) {
            if (bulk > (long) sizeof s_bulk) {
                if (drain_bulk(c, bulk) != 0) return;
                send_err(c, id, "INVALID_PARAM", "chunk demasiado grande");
                return;
            }
            if (recv_exact(c, s_bulk, (size_t) bulk) != 0) return;
            req.bulk = s_bulk;
            req.bulk_len = bulk;
        }
        char rbuf[4096];
        int wrote = -1;
        int n = bpvm_bmgr_wire_dispatch(&g_bm, &req, rbuf, sizeof rbuf, &wrote);
        if (n < 0) { send_err(c, id, "INTERNAL_ERROR", "reply no cabe"); return; }
        if (wrote >= 0) flash_store();     /* la "flash" del sim = el fichero A/B */
        send_line(c, rbuf);
        return;
    }
    send_err(c, id, "UNSUPPORTED", "comando no soportado por el sim");
}

/* ── main ─────────────────────────────────────────────────────────────────── */

/* Tamaños con sufijo cómodo: 512K, 8M. 0 = inválido. */
static size_t parse_size(const char* s) {
    char* end = NULL;
    long long v = strtoll(s, &end, 0);
    if (v <= 0) return 0;
    if (end && (*end == 'k' || *end == 'K')) v *= 1024;
    else if (end && (*end == 'm' || *end == 'M')) v *= 1024 * 1024;
    return (size_t) v;
}

int main(int argc, char** argv) {
    if (bpvm_platform_mutex_init(&s_tx_mtx) != 0) { fprintf(stderr, "sim: no se pudo crear el cerrojo del cable\n"); return 1; }
#ifdef BPVM_SQLITE
    /* UNA VEZ, y lo PRIMERO: sqlite3_config solo vale antes de initialize, y
     * el vfs tiene que estar registrado antes que nada (leccion del 8-ago). */
    sim_sqlite_arranca();
#endif
    const char* pos[3]; int npos = 0;
    const char* fs_img_arg = NULL;
    size_t fs_size_arg = 0;
    const char* packs[8]; int n_packs = 0;
    int zona_sucia = 0;
    int port = 5099;

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (!strncmp(a, "--pack=", 7))            { if (n_packs < 8) packs[n_packs++] = a + 7; }
        else if (!strcmp(a, "--zona-sucia"))      zona_sucia = 1;   /* V7/H7: el «casi» */
        else if (!strncmp(a, "--port=", 7))       port = atoi(a + 7);
        else if (!strncmp(a, "--flash-file=", 13)) g_flash_path = a + 13;
        else if (!strncmp(a, "--flash=", 8))      g_flash_size = (uint32_t) parse_size(a + 8);
        else if (!strncmp(a, "--mem=", 6))        { size_t n = parse_size(a + 6); if (n) g_mem_size = n; }
        else if (!strncmp(a, "--psram=", 8))      g_psram_size = (uint32_t) parse_size(a + 8);
        else if (!strncmp(a, "--fs=", 5))         fs_img_arg = a + 5;
        else if (!strncmp(a, "--fs-size=", 10))   fs_size_arg = parse_size(a + 10);
        else if (!strncmp(a, "--board=", 8))      g_board = a + 8;
        else if (!strncmp(a, "--sd=", 5))         g_sd_path = a + 5;   /* V7/H3: la tarjeta */
        /* H10 — pantalla. Con GUI=0 en el build se aceptan y se ignoran: el IDE
         * las pasa siempre sin tener que saber cómo se compiló el sim. */
        else if (!strncmp(a, "--screen=", 9)) {
            if (sscanf(a + 9, "%dx%d", &g_screen_w, &g_screen_h) != 2
                || g_screen_w <= 0 || g_screen_h <= 0) {
                fprintf(stderr, "sim: --screen espera ANCHOxALTO (p.ej. --screen=480x320)\n");
                return 2;
            }
        }
        else if (!strcmp(a, "--no-screen"))       g_no_screen = 1;
        else if (a[0] == '-') { fprintf(stderr, "sim: argumento desconocido: %s\n", a); return 2; }
        else if (npos < 3) pos[npos++] = a;
    }
    /* Compat con la forma posicional del boardsim: [puerto] [flash] [tamaño]. */
    if (npos > 0) port = atoi(pos[0]);
    if (npos > 1) g_flash_path = pos[1];
    if (npos > 2) g_flash_size = (uint32_t) strtoul(pos[2], NULL, 0);
    if (g_flash_size == 0) g_flash_size = DEF_FLASH;
    /* V6/#469 — el micro SIMULADO no tiene ADC y lo dice a propósito: el backend del host (la rampa determinista de
     * siempre). Así «no hay backend» queda para una placa DE VERDAD que se lo dejó. Era en cada RUN (V7/H10: el RUN
     * común no sabe de ADC); es un registro global que nada borra. */
    bpvm_adc_set_backend(bpvm_adc_backend_host());

    /* --- RAM del micro: un solo buffer con la zona de packs TALLADA al final.
     * Igual que en test/main.c: así los offsets del código XIP (cb = puntero −
     * vm->memory) caben en uint32 también en un host de 64 bits. --- */
    g_vm_mem = (uint8_t*) calloc(1, g_mem_size + PACKS_REGION_SIZE);
    if (!g_vm_mem) {
        fprintf(stderr, "sim: no hay memoria para %lu bytes de RAM\n",
                (unsigned long) g_mem_size);
        return 1;
    }
    g_packs = g_vm_mem + g_mem_size;
    memset(g_packs, 0xFF, PACKS_REGION_SIZE);   /* zona de packs virgen */
    /* Un pack que no entra NO tumba el simulador (22-ago). Antes era `return 1`
     * y bastaba con que el IDE le pasara `SQLite.pack` —1,08 MB contra el 1 MB
     * de aqui abajo— para que el micro simulado no arrancase: exit 1 nada mas
     * empezar, con los packs que trae la propia distribucion. O sea que fallaba
     * recien instalado y sin que el usuario hubiera hecho nada.
     *
     * Mismo criterio que Eduardo fijo ese dia para grabar packs en placa: lo que
     * no se pueda usar se deja fuera y se DICE, pero no se aborta. Aqui es aun
     * mas claro, porque el que no cabe no impide que los demas sirvan: la stdlib
     * entra de sobra y es la que hace falta para casi todo.
     *
     * (Que SQLite no corra en el simulador es OTRA cosa, sabida y documentada:
     * no hay motor nativo para PC. Eso no se arregla aqui.) */
    int cargados = 0, fuera = 0;
    for (int i = 0; i < n_packs; i++) {
        if (packs_preload(packs[i]) == 0) cargados++;
        else                              fuera++;
    }
    if (fuera > 0)
        fprintf(stderr, "sim: %d pack(s) fuera y %d cargado(s) — el simulador"
                        " arranca igual con los que si caben\n", fuera, cargados);
    if (zona_sucia) memset(g_packs, 0x5A, 64);   /* restos: el byte 0 no es BPAK ni 0xFF */
    /* (V7/H7: la zona se monta DESPUÉS del FS, con la regla común; ver abajo.) */

    /* --- "flash" del env (A/B) + gestor de placa --- */
    flash_load();
    g_bm.a = g_a; g_bm.b = g_b; g_bm.scratch = g_scratch; g_bm.sector = SECTOR;
    g_bm.part_base = PART_BASE;
    g_bm.usable_flash = bpvm_part_usable_flash(g_flash_size, 0);
    g_bm.packs_flash = &g_packs_fl;   /* la zona que ve el gestor: la montada (tras el FS) */

    /* --- FS: littlefs sobre una imagen en fichero (MISMO motor que el micro).
     * El tamaño sale de la partición FS si la placa ya está provisionada; si no
     * (placa virgen), 1 MB para poder trabajar desde el minuto cero. --- */
    if (fs_img_arg) snprintf(g_fs_img, sizeof g_fs_img, "%s", fs_img_arg);
    else            snprintf(g_fs_img, sizeof g_fs_img, "%s.fs", g_flash_path);
    uint32_t fs_bytes = fs_partition_size();
    if (fs_size_arg > 0) fs_bytes = (uint32_t) fs_size_arg;
    if (fs_bytes == 0)   fs_bytes = 1024u * 1024u;
    g_fs_blocks = fs_bytes / SECTOR;
    if (bpvm_fs_register_lfs_filebd(g_fs_img, SECTOR, g_fs_blocks, 1) != 0) {
        fprintf(stderr, "sim: no se pudo montar el FS sobre %s\n", g_fs_img);
        return 1;
    }
    /* V7/H7 (Boot 2, fase 1) — la zona, con el FS y por la regla común (como las placas),
     * y el gestor la ve MONTADA. Antes se montaba antes del FS y sin regla. */
    (void) bpvm_pack_montar_arranque(1, sim_montar_packs, sim_log_linea);
    g_bm.packs_base = bpvm_pack_mounted(&g_bm.packs_size);
    bpvm_repl_set_ops(&SIM_REPL_OPS);   /* U3.24: la cintura, antes del primer mensaje */
    /* V7/H3 — la SD, como una placa con ranura: registrar y montar en el arranque, ENCIMA del FS. */
    if (g_sd_path) {
        bpvm_sd_placa_registrar(&SIM_SD, &bpvm_fs_fat_ops, "fichero");
        bpvm_sd_placa_montar_arranque();
    } else {
        bpvm_sd_placa_sin_sd("el micro simulado no tiene SD: arrancalo con --sd=IMAGEN (un fichero hace de tarjeta)");
    }
    /* V6/U6.1 — `stack=N` del ENV, como las tres placas (`bpvm_set_stack_kb`).
     *
     * El simulador tenia gestor de placa y ENV —de hecho responde a `ENV_SET`—
     * y era el unico que no aplicaba esta clave. Se lee AQUI, en el arranque, y
     * no en cada `Run`: en la placa el valor se recoge en el boot y hace falta un
     * reset para que entre. Aplicarlo por `Run` seria mas comodo y por eso mismo
     * infiel — el simulador tiene que doler donde duele la placa. */
    {
        bpvm_env_t env;
        if (bpvm_bmgr_env(&g_bm, &env))   /* devuelve 1 si HAY env, no 0 */
            bpvm_boot2_claves(&env);   /* V7/H7 (Boot 2, fase 4): stack= y quantum=, como las placas */
    }
    bpvm_bus_desde_env(sim_env_get);   /* V7/H6.c G4.a1: los buses del ENV, en el boot como en la placa */
    /* V7/H10 #626 — Machine.getEnv: la foto del ENV del arranque. El sim no anota la fecha de su imagen (no sube por
     * el arranque común): la pone quien la quiera probar (tools/env_smoke.py). */
    memcpy(g_env_a0, g_a, SECTOR);
    memcpy(g_env_b0, g_b, SECTOR);
    bpvm_env_pick(g_env_a0, SECTOR, g_env_b0, SECTOR, &g_env0);
    bpvm_machine_set_env(sim_env_arranque);

    bpvm_net_register_host();   /* sockets TCP del SO, como en la VM-C host */

    /* H10 — el panel simulado. Se fija ANTES de que ningún programa cree el
     * screen; a partir de ahí Gui.Screen() reporta este tamaño y el layout se
     * comporta como en la placa que estamos imitando. */
#ifdef BPVM_GUI
    if (g_screen_w > 0) bpvm_gui_set_screen_size(g_screen_w, g_screen_h);
#endif
#ifdef BPVM_LVGL
    if (g_no_screen) bpvm_gui_disp_set_headless(1);
    bpvm_gui_disp_set_exit_on_close(1);   /* cerrar la ventana = apagar el micro simulado */
    /* #322 — la ventana decía "LVGL Simulator": nombraba la librería que dibuja,
     * no lo que corre. Que se identifique como lo que es —un micro simulado, no
     * una placa— y con la resolución, que es el dato que se compara con la
     * pantalla real cuando el layout no cuadra. */
    {
        char title[64];
        if (g_screen_w > 0)
            snprintf(title, sizeof title, "BasicPlus — micro simulado (%dx%d)",
                     g_screen_w, g_screen_h);
        else
            snprintf(title, sizeof title, "BasicPlus — micro simulado");
        bpvm_gui_disp_set_title(title);
    }
#endif

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { fprintf(stderr, "WSAStartup falló\n"); return 1; }
#endif
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == BAD_SOCK) { fprintf(stderr, "socket() falló\n"); return 1; }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*) &yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short) port);
    if (bind(srv, (struct sockaddr*) &addr, sizeof addr) != 0) { fprintf(stderr, "bind(%d) falló\n", port); return 1; }
    if (listen(srv, 1) != 0) { fprintf(stderr, "listen() falló\n"); return 1; }

    printf("%s: 127.0.0.1:%d\n", SERVER_NAME, port);
    printf("  RAM   %lu KiB   PSRAM %lu KiB   flash %lu KiB (%lu usables)\n",
           (unsigned long) (g_mem_size / 1024), (unsigned long) (g_psram_size / 1024),
           (unsigned long) (g_flash_size / 1024), (unsigned long) (g_bm.usable_flash / 1024));
    printf("  env   %s\n  FS    %s (%u bloques de %u B)\n",
           g_flash_path, g_fs_img, g_fs_blocks, (unsigned) SECTOR);
    if (g_no_screen) printf("  panel sin pantalla\n");
    else             printf("  panel %dx%d\n", g_screen_w > 0 ? g_screen_w : 480,
                            g_screen_h > 0 ? g_screen_h : 320);
    printf("  estado inicial: %s\n",
           bpvm_boot_state_name((bpvm_boot_state_t) bpvm_bmgr_wire_state(&g_bm)));
    fflush(stdout);

    for (;;) {   /* un cliente a la vez (el IDE) */
        sock_t cli = accept(srv, NULL, NULL);
        if (cli == BAD_SOCK) continue;
        g_cli = cli;
        printf("%s: cliente conectado\n", SERVER_NAME); fflush(stdout);
        char line[WIRE_LINE_MAX];
        while (g_cli != BAD_SOCK) {
            /* #495 (paso 4) — una transferencia a medias: el cable se mira sin esperar y, si no hay nada, el trozo
             * siguiente. Sin ella, la espera de siempre. */
            if (bpvm_repl_hay_transferencia() && !sock_has_data(g_cli)) { (void) bpvm_repl_paso(); continue; }
            if (recv_line(g_cli, line, sizeof line) < 0) break;
            json_obj_t obj;
            if (json_parse(line, strlen(line), &obj) == 0) handle(g_cli, &obj);
            /* líneas no-JSON se ignoran (tolerante) */
        }
        if (g_cli != BAD_SOCK) close_sock(g_cli);
        g_cli = BAD_SOCK;
        bpvm_repl_cliente_ido();   /* #495 (paso 4): lo que se le mandaba (y una subida a medias), cortado */
        printf("%s: cliente desconectado\n", SERVER_NAME); fflush(stdout);
    }
}

static int sim_montar_packs(void) {
    bpvm_pack_mount(g_packs, g_packs, PACKS_REGION_SIZE);
    return 1;
}
