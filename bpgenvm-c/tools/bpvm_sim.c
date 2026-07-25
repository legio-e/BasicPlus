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
 *                [--no-screen] [--pack=F]...
 *   defaults: 127.0.0.1:5099, "boardsim.flash", 4 MB de flash, 512 KiB de RAM,
 *             imagen de FS "<fichero-flash>.fs".
 *   --pack=<f.pack> (repetible): "graba" la imagen en la zona de packs simulada
 *   (región RAM, 0xFF = NOR virgen) para que PACK_LS la liste y los imports
 *   resuelvan XIP, igual que el --pack= de la VM-C host.
 *
 * Ver docs/BPVM_WIRE_PROTOCOL.md (catálogo de mensajes) y
 *     docs/H9_KERNEL_CAPAS.md §Comandos de gestión de placa.
 */
#include "bpvm.h"
#include "bpvm_internal.h"   /* vm->modules[].{name,imports,import_count} para deps */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_boot.h"
#include "bpvm_fs.h"
#include "bpvm_net.h"
#include "bpvm_pack.h"
#include "bpvm_rtc.h"
#include "crc32.h"
#include "mdn_loader.h"
#include "aot_registry.h"
#include "json_min.h"
#ifdef BPVM_GUI
#include "bpvm_gui.h"   /* H10 — --screen=WxH / --no-screen */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#define PATH_MAX_SIM   192    /* paths del wire (el micro usa 64; aquí sobra) */

/* --- "flash" simulada: dos sectores A/B en RAM, respaldados por un fichero --- */
#define SECTOR      4096u
#define PART_BASE   0x100000u   /* 1 MB reservado (imagen + env) */
#define DEF_FLASH   0x400000u   /* 4 MB por defecto (tipo Pico) */

static uint8_t g_a[SECTOR], g_b[SECTOR], g_scratch[SECTOR];
static bpvm_bmgr_t g_bm;
static const char* g_flash_path = "boardsim.flash";

/* H3 — zona de packs simulada (región RAM que hace de flash; 0xFF = borrada).
 * Se precarga con los --pack= y PACK_LS la sirve por el dispatch compartido. */
#define PACKS_REGION_SIZE (1024u * 1024u)
static uint8_t* g_packs = NULL;   /* tallada dentro del buffer de la VM (ver main) */

/* ── H10: el "silicio" configurable ─────────────────────────────────────────
 * Son los mimbres que el IDE deja tocar: la RAM que gestiona la VM, la PSRAM
 * declarada (hoy informativa: la reporta INFO igual que una placa con PSRAM) y
 * el tamaño de la flash, del que salen las particiones. */
static size_t   g_mem_size   = 512u * 1024u;   /* RAM de la VM (--mem) */
static uint32_t g_psram_size = 0;              /* PSRAM declarada (--psram) */
static uint32_t g_flash_size = DEF_FLASH;      /* flash total (--flash) */
static uint8_t* g_vm_mem     = NULL;           /* buffer de la VM (RAM del micro) */
static const char* g_board   = NULL;           /* --board: identidad por defecto */
static int      g_screen_w   = 0;              /* --screen=WxH (0 = el default de gui.c) */
static int      g_screen_h   = 0;
static int      g_no_screen  = 0;              /* --no-screen: placa sin panel */

/* Estado del servidor mientras hay un cliente: el sink de OUTPUT y el poll del
 * KILL son callbacks de la VM y necesitan el socket → global, igual que el
 * s_run_session de los firmwares. */
static sock_t g_cli         = BAD_SOCK;
static long   g_session     = 0;    /* contador de sesiones RUN */
static long   g_run_session = 0;    /* sesión activa (para el sink) */
static long   g_kill_ack_id = -1;   /* KILL recibido durante el run (ack diferido) */

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
static void send_line(sock_t c, const char* line) {
    if (send_all(c, line, strlen(line)) == 0) send_all(c, "\n", 1);
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
/* Lee una línea (hasta '\n', descarta '\r'). Devuelve len o -1 si el peer cierra. */
static int recv_line(sock_t c, char* buf, size_t cap) {
    size_t n = 0;
    for (;;) {
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
static void ensure_parent_dirs(const char* path) {
    char tmp[PATH_MAX_SIM];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char* p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        bpvm_fs_mkdir(tmp);   /* ok si ya existe */
        *p = '/';
    }
}

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

typedef struct { uint32_t used; int count; int truncated; } fs_tally_t;
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
        else g_tally.truncated = 1;
        return;
    }
    if (g_n_ents < WALK_MAX_ENTRIES) {
        snprintf(g_ents[g_n_ents].path, PATH_MAX_SIM, "%s", full);
        g_ents[g_n_ents].size = size;
        g_n_ents++;
    } else {
        g_tally.truncated = 1;
    }
}

/* Recorre TODO el árbol desde "/". Con sb != NULL emite las entries del
 * LIST_REPLY; siempre deja el tally (bytes usados + nº de ficheros) en g_tally. */
static void fs_walk(sb_t* sb) {
    g_n_dirs = 1; g_n_ents = 0;
    g_tally.used = 0; g_tally.count = 0; g_tally.truncated = 0;
    snprintf(g_dirs[0], PATH_MAX_SIM, "%s", "/");

    /* Fase 1: recorrer (el índice avanza sobre una cola que crece). */
    for (int i = 0; i < g_n_dirs && i < WALK_MAX_DIRS; i++)
        bpvm_fs_list(g_dirs[i], collect_cb, g_dirs[i]);

    /* Fase 2: ya fuera del lock — leer para el CRC y emitir. */
    for (int i = 0; i < g_n_ents; i++) {
        g_tally.used += g_ents[i].size;
        g_tally.count++;
        if (!sb) continue;
        /* CRC del contenido (== java.util.zip.CRC32): con él el IDE se salta el
         * PUT cuando el fichero ya está idéntico en el device. */
        uint32_t crc = 0, fsz = 0;
        uint8_t* d = fs_read_all(g_ents[i].path, &fsz);
        if (d) { crc = bpvm_crc32(d, fsz); free(d); }

        if (i > 0) sb_raw(sb, ",");
        sb_raw(sb, "{\"name\":\""); sb_esc(sb, g_ents[i].path);
        sb_raw(sb, "\",\"size\":");   sb_ulong(sb, (unsigned long) g_ents[i].size);
        sb_raw(sb, ",\"crc\":");      sb_ulong(sb, (unsigned long) crc);
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

/* ── META ─────────────────────────────────────────────────────────────────── */

static void handle_hello(sock_t c, long id) {
    char buf[320]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"HELLO_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"protoVersion\":1,\"serverName\":\"" SERVER_NAME "\","
               "\"serverBuild\":\"" __DATE__ " " __TIME__ "\","
               "\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\",\"BOARDMGR\",\"PACKS\"]}");
    if (s.ok) send_line(c, s.buf);
}

static void handle_info(sock_t c, long id) {
    char idv[64];
    int has = bpvm_bmgr_env_get(&g_bm, "board", idv, sizeof idv) >= 0;
    uint32_t fs_total = fs_partition_size();
    fs_walk(NULL);

    char buf[768]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"INFO_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"uniqueId\":\"SIMULATED0000000\",\"boardName\":\"");
    sb_esc(&s, has ? idv : (g_board ? g_board : "sim"));
    sb_raw(&s, "\",\"serverName\":\"" SERVER_NAME "\"");
    sb_raw(&s, ",\"cpuFreqHz\":0,\"uptimeMs\":");
    sb_ulong(&s, (unsigned long) ((unsigned long) clock() * 1000UL / CLOCKS_PER_SEC));
    sb_raw(&s, ",\"tempMilliC\":0,\"resetReason\":\"sim\"");
    sb_raw(&s, ",\"gpioCount\":0,\"pioCount\":0,\"pwmSlices\":0,\"adcChannels\":0");
    sb_raw(&s, ",\"flashBytes\":");   sb_ulong(&s, (unsigned long) g_flash_size);
    sb_raw(&s, ",\"sramBytes\":");    sb_ulong(&s, (unsigned long) g_mem_size);
    sb_raw(&s, ",\"psramBytes\":");   sb_ulong(&s, (unsigned long) g_psram_size);
    sb_raw(&s, ",\"fsTotalBytes\":"); sb_ulong(&s, (unsigned long) fs_total);
    sb_raw(&s, ",\"fsUsedBytes\":");  sb_ulong(&s, (unsigned long) g_tally.used);
    /* H10 — el panel simulado, para que el IDE pueda mostrarlo (0x0 = sin pantalla). */
    sb_raw(&s, ",\"screenW\":"); sb_long(&s, g_no_screen ? 0 : (g_screen_w > 0 ? g_screen_w : 480));
    sb_raw(&s, ",\"screenH\":"); sb_long(&s, g_no_screen ? 0 : (g_screen_h > 0 ? g_screen_h : 320));
    sb_raw(&s, "}");
    if (s.ok) send_line(c, s.buf); else send_err(c, id, "INTERNAL_ERROR", "INFO_REPLY no cabe");
}

/* ── FILES ────────────────────────────────────────────────────────────────── */

static void handle_list(sock_t c, long id) {
    static char big[256 * 1024];   /* host: el árbol entero cabe de sobra */
    sb_t s; sb_init(&s, big, sizeof big);
    sb_raw(&s, "{\"type\":\"LIST_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"entries\":[");
    fs_walk(&s);
    sb_raw(&s, "]}");
    if (s.ok) send_line(c, s.buf);
    else      send_err(c, id, "INTERNAL_ERROR", "LIST_REPLY no cabe");
}

static void handle_stat(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) { send_err(c, id, "NOT_FOUND", "no existe"); return; }
    char buf[160]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"STAT_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"size\":"); sb_ulong(&s, (unsigned long) size);
    sb_raw(&s, ",\"isDir\":false,\"mtime\":0}");
    if (s.ok) send_line(c, s.buf);
}

static void handle_df(sock_t c, long id) {
    uint32_t total = fs_partition_size();
    fs_walk(NULL);
    unsigned long used = g_tally.used;
    char buf[224]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"DF_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"totalBytes\":"); sb_ulong(&s, (unsigned long) total);
    sb_raw(&s, ",\"usedBytes\":");  sb_ulong(&s, used);
    sb_raw(&s, ",\"freeBytes\":");  sb_ulong(&s, (total > used) ? total - used : 0UL);
    sb_raw(&s, ",\"fileCount\":");  sb_long(&s, g_tally.count);
    sb_raw(&s, "}");
    if (s.ok) send_line(c, s.buf);
}

static void handle_get(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    uint32_t size = 0;
    uint8_t* data = fs_read_all(path, &size);
    if (!data) { send_err(c, id, "NOT_FOUND", "no existe"); return; }
    char buf[96]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"GET_REPLY\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\"bulk\":"); sb_ulong(&s, (unsigned long) size); sb_raw(&s, "}");
    if (s.ok) {
        send_line(c, s.buf);
        if (size > 0) send_all(c, (const char*) data, size);
    }
    free(data);
}

static void handle_put(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    long bulk = json_get_long(obj, "bulk", -1);
    int has_path = json_get_str(obj, "path", path, sizeof path) >= 0;
    if (bulk < 0) { send_err(c, id, "INVALID_PARAM", "falta bulk"); return; }
    /* Consumir SIEMPRE el bulk antes de decidir nada (o el wire se desincroniza). */
    uint8_t* data = (uint8_t*) malloc(bulk ? (size_t) bulk : 1);
    if (!data) { drain_bulk(c, bulk); send_err(c, id, "NO_SPACE", "sin memoria"); return; }
    if (bulk > 0 && recv_exact(c, data, (size_t) bulk) != 0) { free(data); return; }
    if (!has_path) { free(data); send_err(c, id, "INVALID_PARAM", "falta path"); return; }

    ensure_parent_dirs(path);
    int rc = bpvm_fs_write(path, data, (uint32_t) bulk, 0);
    free(data);
    if (rc != 0) { send_err(c, id, "NO_SPACE", "no se pudo escribir"); return; }
    send_ok(c, "PUT_REPLY", id);
}

/* #294 streaming PUT — BEGIN crea/trunca, cada DATA apende, END verifica. */
static struct { int active; char path[PATH_MAX_SIM]; uint32_t received, expected; } g_put;

static void reply_put_field(sock_t c, const char* type, long id,
                            unsigned long val, const char* field) {
    char buf[128]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\""); sb_raw(&s, type);
    sb_raw(&s, "\",\"id\":"); sb_long(&s, id);
    sb_raw(&s, ",\""); sb_raw(&s, field); sb_raw(&s, "\":"); sb_ulong(&s, val);
    sb_raw(&s, "}");
    if (s.ok) send_line(c, s.buf);
}

static void handle_put_begin(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    ensure_parent_dirs(path);
    if (bpvm_fs_write(path, NULL, 0, 0) != 0) {
        send_err(c, id, "NO_SPACE", "no se pudo crear"); return;
    }
    g_put.active   = 1;
    g_put.received = 0;
    g_put.expected = (uint32_t) json_get_long(obj, "size", 0);
    snprintf(g_put.path, sizeof g_put.path, "%s", path);
    reply_put_field(c, "PUT_BEGIN_REPLY", id, 0, "received");
}

static void handle_put_data(sock_t c, long id, const json_obj_t* obj) {
    long bulk = json_get_long(obj, "bulk", -1);
    if (bulk < 0) { send_err(c, id, "INVALID_PARAM", "falta bulk"); return; }
    uint8_t* data = (uint8_t*) malloc(bulk ? (size_t) bulk : 1);
    if (!data) { drain_bulk(c, bulk); send_err(c, id, "NO_SPACE", "sin memoria"); return; }
    if (bulk > 0 && recv_exact(c, data, (size_t) bulk) != 0) { free(data); return; }
    if (!g_put.active) { free(data); send_err(c, id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return; }
    int rc = (bulk > 0) ? bpvm_fs_write(g_put.path, data, (uint32_t) bulk, 1) : 0;
    free(data);
    if (rc != 0) { g_put.active = 0; send_err(c, id, "NO_SPACE", "no se pudo apendar"); return; }
    g_put.received += (uint32_t) bulk;
    reply_put_field(c, "PUT_DATA_REPLY", id, g_put.received, "received");
}

static void handle_put_end(sock_t c, long id) {
    if (!g_put.active) { send_err(c, id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return; }
    uint32_t recvd = g_put.received, exp = g_put.expected;
    g_put.active = 0;
    if (exp != 0 && recvd != exp) { send_err(c, id, "SIZE_MISMATCH", "bytes != size"); return; }
    reply_put_field(c, "PUT_END_REPLY", id, recvd, "size");
}

static void handle_del(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    if (bpvm_fs_remove(path) != 0) { send_err(c, id, "NOT_FOUND", "no existe"); return; }
    send_ok(c, "DEL_REPLY", id);
}

static void handle_mkdir(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    ensure_parent_dirs(path);
    if (bpvm_fs_mkdir(path) != 0) { send_err(c, id, "INTERNAL_ERROR", "no se pudo crear"); return; }
    send_ok(c, "MKDIR_REPLY", id);
}

static void handle_rename(sock_t c, long id, const json_obj_t* obj) {
    char from[PATH_MAX_SIM], to[PATH_MAX_SIM];
    if (json_get_str(obj, "from", from, sizeof from) < 0 ||
        json_get_str(obj, "to",   to,   sizeof to)   < 0) {
        send_err(c, id, "INVALID_PARAM", "faltan from/to"); return;
    }
    ensure_parent_dirs(to);
    if (bpvm_fs_rename(from, to) != 0) { send_err(c, id, "NOT_FOUND", "no se pudo renombrar"); return; }
    send_ok(c, "RENAME_REPLY", id);
}

/* Datos de la imagen del FS: hacen falta para re-formatear (cerrar → borrar el
 * fichero → volver a montar formateando), que es como se formatea una flash. */
static char     g_fs_img[PATH_MAX_SIM] = "";
static unsigned g_fs_blocks = 0;

static void handle_format(sock_t c, long id, const json_obj_t* obj) {
    char confirm[8];
    if (json_get_str(obj, "confirm", confirm, sizeof confirm) < 0 || strcmp(confirm, "YES") != 0) {
        send_err(c, id, "MISSING_CONFIRM", "confirm:\"YES\""); return;
    }
    bpvm_fs_lfs_filebd_close();
    remove(g_fs_img);                       /* imagen virgen → el mount la formatea */
    if (bpvm_fs_register_lfs_filebd(g_fs_img, SECTOR, g_fs_blocks, 1) != 0) {
        send_err(c, id, "FLASH_ERROR", "no se pudo re-montar el FS"); return;
    }
    send_ok(c, "FORMAT_REPLY", id);
}

/* ── TERMINAL: RUN / KILL ─────────────────────────────────────────────────── */

/* Cada print de la VM llega aquí → evento OUTPUT con los bytes escapados. */
static void sim_output_sink(const char* s, size_t len, void* user) {
    (void) user;
    if (g_cli == BAD_SOCK) return;
    size_t cap = len * 6 + 128;   /* el escape puede crecer x6 (\u00XX) */
    char* buf = (char*) malloc(cap);
    char* txt = (char*) malloc(len + 1);
    if (!buf || !txt) { free(buf); free(txt); return; }
    memcpy(txt, s, len); txt[len] = '\0';

    sb_t sb; sb_init(&sb, buf, cap);
    sb_raw(&sb, "{\"type\":\"OUTPUT\",\"session\":"); sb_long(&sb, g_run_session);
    sb_raw(&sb, ",\"stream\":\"stdout\",\"data\":\""); sb_esc(&sb, txt);
    sb_raw(&sb, "\"}");
    if (sb.ok) send_line(g_cli, sb.buf);
    free(txt); free(buf);
}

static void emit_exited(sock_t c, long session, const char* status, int code,
                        unsigned long ms, const char* err_msg) {
    char buf[640]; sb_t s; sb_init(&s, buf, sizeof buf);
    sb_raw(&s, "{\"type\":\"EXITED\",\"session\":"); sb_long(&s, session);
    sb_raw(&s, ",\"status\":\""); sb_raw(&s, status);
    sb_raw(&s, "\",\"exitCode\":"); sb_long(&s, code);
    sb_raw(&s, ",\"elapsedMs\":"); sb_ulong(&s, ms);
    if (err_msg && err_msg[0]) { sb_raw(&s, ",\"errorMessage\":\""); sb_esc(&s, err_msg); sb_raw(&s, "\""); }
    sb_raw(&s, "}");
    if (s.ok) send_line(c, s.buf);
}

/* Poll del wire entre quanta (#257): KILL para el programa (ack diferido, tras
 * parar), HELLO se contesta al vuelo (el IDE puede conectar con algo corriendo
 * y ofrecer Stop) y cualquier otra cosa devuelve BUSY. */
static int sim_run_poll_cb(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    if (g_cli == BAD_SOCK || !sock_has_data(g_cli)) return 0;
    char line[WIRE_LINE_MAX];
    if (recv_line(g_cli, line, sizeof line) < 0) return 1;   /* peer cerró → parar */
    json_obj_t obj;
    if (json_parse(line, strlen(line), &obj) != 0) return 0;
    char type[32] = {0};
    json_get_str(&obj, "type", type, sizeof type);
    long rid = json_get_long(&obj, "id", 0);
    if (!strcmp(type, "KILL"))  { g_kill_ack_id = rid; return 1; }
    if (!strcmp(type, "HELLO")) { handle_hello(g_cli, rid); return 0; }
    send_err(g_cli, rid, "BUSY", "ejecución en curso: solo HELLO/KILL");
    return 0;
}

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

static void handle_run(sock_t c, long id, const json_obj_t* obj) {
    char path[PATH_MAX_SIM];
    if (json_get_str(obj, "path", path, sizeof path) < 0) {
        send_err(c, id, "INVALID_PARAM", "falta path"); return;
    }
    /* H19-F1 — base-dir/main-module del proyecto si vive en /app/<proj>/. */
    bpvm_fs_set_basedir_from_module(path);
    bpvm_fs_set_main_module_path(path);

    uint32_t size = 0;
    uint8_t* data = sim_fs_resolve(path, &size);
    if (!data) { send_err(c, id, "NOT_FOUND", "no existe"); return; }
    keep_buf(data);

    long session = ++g_session;
    g_run_session = session;
    { char buf[96]; sb_t s; sb_init(&s, buf, sizeof buf);
      sb_raw(&s, "{\"type\":\"RUN_REPLY\",\"id\":"); sb_long(&s, id);
      sb_raw(&s, ",\"session\":"); sb_long(&s, session); sb_raw(&s, "}");
      if (s.ok) send_line(c, s.buf); }

    bpvm_t* vm = bpvm_init(g_vm_mem, g_mem_size, 0);
    if (!vm) {
        free_bufs();
        emit_exited(c, session, "INTERNAL_ERROR", -1, 0, "no se pudo inicializar la VM");
        g_run_session = 0;
        return;
    }
    bpvm_set_output(vm, sim_output_sink, NULL);

    clock_t t0 = clock();
    bpvm_status_t st = bpvm_load_mod_buffer(vm, data, size, path);

    /* Resolución iterativa de imports (≤4 pasadas: deps de deps). El FS ECLIPSA
     * al pack; si no está en FS se carga XIP desde la zona de packs. Idéntico a
     * los firmwares. */
    for (int pass = 0; st == BPVM_OK && pass < 4; pass++) {
        int loaded_any = 0;
        int n_before = vm->module_count;
        for (int mi = 0; mi < n_before && st == BPVM_OK; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[64];
                import_owner(imp, owner, sizeof owner);
                if (!owner[0] || module_loaded(vm, owner)) continue;

                char fname[80]; snprintf(fname, sizeof fname, "%s.mod", owner);
                uint32_t pk_rs = 0;
                const uint8_t* pk_rb = bpvm_pack_mounted(&pk_rs);
                uint32_t pk_len = 0;
                const uint8_t* pk_mod = pk_rb
                    ? bpvm_pack_find(pk_rb, pk_rs, "mod", owner, &pk_len) : NULL;
                uint32_t dep_size = 0;
                uint8_t* dep = sim_fs_resolve(fname, &dep_size);
                if (!dep) {
                    if (!pk_mod) continue;              /* falta → lo caza el guard */
                    bpvm_status_t ds = bpvm_loader_load_xip(vm, pk_mod, pk_len, owner);
                    if (ds != BPVM_OK) { st = ds; break; }
                    /* Que se vea en la consola del IDE de dónde sale cada módulo:
                     * "no aparece por ningún lado" y "vino del pack, no del que
                     * acabas de subir" son dos ratos de búsqueda distintos. */
                    { char m2[128];
                      int n2 = snprintf(m2, sizeof m2,
                                        "[sim] '%s' desde el pack (XIP, %lu B)\n",
                                        owner, (unsigned long) pk_len);
                      if (n2 > 0) sim_output_sink(m2, (size_t) n2, NULL); }
                    loaded_any = 1;
                    continue;
                }
                keep_buf(dep);
                bpvm_status_t ds = bpvm_load_mod_buffer(vm, dep, dep_size, owner);
                if (ds != BPVM_OK) { st = ds; break; }
                loaded_any = 1;
            }
        }
        if (!loaded_any) break;
    }

    /* Guard: ¿quedó algún import sin resolver? Mejor un error limpio que un
     * CALL_EXT al vacío. */
    char missing[64] = {0};
    if (st == BPVM_OK) {
        for (int mi = 0; mi < vm->module_count && !missing[0]; mi++) {
            bpvm_module_t* m = &vm->modules[mi];
            for (int k = 0; k < m->import_count && !missing[0]; k++) {
                const char* imp = m->imports[k];
                if (!imp || !imp[0]) continue;
                char owner[64];
                import_owner(imp, owner, sizeof owner);
                if (owner[0] && !module_loaded(vm, owner))
                    snprintf(missing, sizeof missing, "%s", owner);
            }
        }
    }

    /* Overlay AOT (.mdn) del FS, si lo hay. El registry es GLOBAL → clear antes
     * de cada RUN para no arrastrar thunks de una sesión anterior. */
    bpvm_aot_clear();
    if (st == BPVM_OK && !missing[0]) {
        for (int mi = 0; mi < vm->module_count; mi++) {
            const char* mname = vm->modules[mi].name;
            if (!mname || !mname[0]) continue;
            char mdn_path[96]; snprintf(mdn_path, sizeof mdn_path, "%s.mdn", mname);
            uint32_t msz = 0;
            uint8_t* mdn = sim_fs_resolve(mdn_path, &msz);
            if (!mdn) continue;
            keep_buf(mdn);
            int mrc = bpvm_load_mdn(vm, mdn, (size_t) msz);
            char mmsg[128];
            int mn = snprintf(mmsg, sizeof mmsg, "[AOT] %s %s (rc=%d)\n",
                              mdn_path, (mrc == 0) ? "OK" : "FALLO -> interpretado", mrc);
            if (mn > 0) sim_output_sink(mmsg, (size_t) mn, NULL);
        }
    }

    g_kill_ack_id = -1;
    if (st == BPVM_OK && !missing[0]) {
        bpvm_set_poll(vm, sim_run_poll_cb, NULL);
        st = bpvm_run(vm);
        bpvm_set_poll(vm, NULL, NULL);
    }
    unsigned long dt = (unsigned long) ((clock() - t0) * 1000L / CLOCKS_PER_SEC);

    /* Ack diferido del KILL, ANTES del EXITED (como los firmwares). */
    if (g_kill_ack_id >= 0) { send_ok(c, "KILL_REPLY", g_kill_ack_id); g_kill_ack_id = -1; }

    if (missing[0]) {
        /* Que el mensaje diga QUÉ hacer. El caso típico es un módulo de la
         * librería (Core el primero, que se importa implícito): una placa lo
         * trae embebido en el firmware y por eso el IDE no lo sube, pero el
         * simulado arranca con el FS vacío y necesita el pack de la stdlib. */
        char msg[256];
        uint32_t pk_rs = 0;
        int hay_packs = bpvm_pack_mounted(&pk_rs) != NULL;
        snprintf(msg, sizeof msg,
                 "falta el modulo '%s': no esta en el FS%s. %s",
                 missing,
                 hay_packs ? " ni en los packs grabados" : " y no hay ningun pack grabado",
                 hay_packs ? "Comprueba que el pack que lo trae este en la libreria de packs."
                           : "Si es de la libreria estandar, configura la libreria de packs "
                             "(engranaje del micro simulado) y reinicia el simulador.");
        emit_exited(c, session, "RUNTIME_ERROR", -2, 0, msg);
    } else {
        const char* link_err = bpvm_link_error(vm);
        if (link_err[0]) {
            emit_exited(c, session, "LINK_ERROR", (int) st, dt, link_err);
        } else {
            emit_exited(c, session,
                        (st == BPVM_OK)     ? "OK"
                      : (st == BPVM_KILLED) ? "KILLED" : "RUNTIME_ERROR",
                        (st == BPVM_KILLED) ? 130 : (int) st, dt,
                        (st == BPVM_OK || st == BPVM_KILLED) ? "" : bpvm_runtime_error(vm));
        }
    }
    bpvm_destroy(vm);
    free_bufs();
    g_run_session = 0;
}

/* ── dispatch de un request ───────────────────────────────────────────────── */

static void handle(sock_t c, const json_obj_t* obj) {
    char type[32];
    if (json_get_str(obj, "type", type, sizeof type) < 0) return;
    long id = json_get_long(obj, "id", -1);

    /* META */
    if (!strcmp(type, "HELLO")) { handle_hello(c, id); return; }
    if (!strcmp(type, "PING"))  { send_ok(c, "PONG", id); return; }
    if (!strcmp(type, "INFO"))  { handle_info(c, id); return; }
    if (!strcmp(type, "TIME"))  {
        long epoch = json_get_long(obj, "epochSec", 0);
        if (epoch > 0) bpvm_rtc_set_now_ms((int64_t) epoch * 1000);
        send_ok(c, "TIME_REPLY", id); return;
    }
    if (!strcmp(type, "RESET")) {
        /* No hay silicio que rebootear: se contesta y se corta la conexión, que
         * es lo que ve el IDE en una placa. El estado vive en la "flash". */
        send_ok(c, "RESET_REPLY", id);
        close_sock(c);
        g_cli = BAD_SOCK;
        return;
    }

    /* FILES */
    if (!strcmp(type, "LIST"))       { handle_list(c, id); return; }
    if (!strcmp(type, "STAT"))       { handle_stat(c, id, obj); return; }
    if (!strcmp(type, "DF"))         { handle_df(c, id); return; }
    if (!strcmp(type, "GET"))        { handle_get(c, id, obj); return; }
    if (!strcmp(type, "PUT"))        { handle_put(c, id, obj); return; }
    if (!strcmp(type, "PUT_BEGIN"))  { handle_put_begin(c, id, obj); return; }
    if (!strcmp(type, "PUT_DATA"))   { handle_put_data(c, id, obj); return; }
    if (!strcmp(type, "PUT_END"))    { handle_put_end(c, id); return; }
    if (!strcmp(type, "DEL"))        { handle_del(c, id, obj); return; }
    if (!strcmp(type, "MKDIR"))      { handle_mkdir(c, id, obj); return; }
    if (!strcmp(type, "RENAME"))     { handle_rename(c, id, obj); return; }
    if (!strcmp(type, "FORMAT"))     { handle_format(c, id, obj); return; }
    if (!strcmp(type, "SAVE"))       { send_ok(c, "SAVE_REPLY", id); return; }  /* littlefs ya persiste */

    /* TERMINAL */
    if (!strcmp(type, "RUN"))  { handle_run(c, id, obj); return; }
    if (!strcmp(type, "KILL")) { send_ok(c, "KILL_REPLY", id); return; }  /* nada corriendo */

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
    const char* pos[3]; int npos = 0;
    const char* fs_img_arg = NULL;
    size_t fs_size_arg = 0;
    const char* packs[8]; int n_packs = 0;
    int port = 5099;

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (!strncmp(a, "--pack=", 7))            { if (n_packs < 8) packs[n_packs++] = a + 7; }
        else if (!strncmp(a, "--port=", 7))       port = atoi(a + 7);
        else if (!strncmp(a, "--flash-file=", 13)) g_flash_path = a + 13;
        else if (!strncmp(a, "--flash=", 8))      g_flash_size = (uint32_t) parse_size(a + 8);
        else if (!strncmp(a, "--mem=", 6))        { size_t n = parse_size(a + 6); if (n) g_mem_size = n; }
        else if (!strncmp(a, "--psram=", 8))      g_psram_size = (uint32_t) parse_size(a + 8);
        else if (!strncmp(a, "--fs=", 5))         fs_img_arg = a + 5;
        else if (!strncmp(a, "--fs-size=", 10))   fs_size_arg = parse_size(a + 10);
        else if (!strncmp(a, "--board=", 8))      g_board = a + 8;
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
    for (int i = 0; i < n_packs; i++) if (packs_preload(packs[i]) != 0) return 1;
    bpvm_pack_mount(g_packs, PACKS_REGION_SIZE);

    /* --- "flash" del env (A/B) + gestor de placa --- */
    flash_load();
    g_bm.a = g_a; g_bm.b = g_b; g_bm.scratch = g_scratch; g_bm.sector = SECTOR;
    g_bm.part_base = PART_BASE;
    g_bm.usable_flash = bpvm_part_usable_flash(g_flash_size, 0);
    g_bm.packs_base = g_packs;
    g_bm.packs_size = PACKS_REGION_SIZE;
    g_bm.packs_flash = &g_packs_fl;

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
    bpvm_net_register_host();   /* sockets TCP del SO, como en la VM-C host */

    /* H10 — el panel simulado. Se fija ANTES de que ningún programa cree el
     * screen; a partir de ahí Gui.Screen() reporta este tamaño y el layout se
     * comporta como en la placa que estamos imitando. */
#ifdef BPVM_GUI
    if (g_screen_w > 0) bpvm_gui_set_screen_size(g_screen_w, g_screen_h);
#endif
#ifdef BPVM_LVGL
    if (g_no_screen) bpvm_gui_disp_set_headless(1);
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
        while (g_cli != BAD_SOCK && recv_line(g_cli, line, sizeof line) >= 0) {
            json_obj_t obj;
            if (json_parse(line, strlen(line), &obj) == 0) handle(g_cli, &obj);
            /* líneas no-JSON se ignoran (tolerante) */
        }
        if (g_cli != BAD_SOCK) close_sock(g_cli);
        g_cli = BAD_SOCK;
        printf("%s: cliente desconectado\n", SERVER_NAME); fflush(stdout);
    }
}
