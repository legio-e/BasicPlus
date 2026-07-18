/*
 * boardsim.c — H9: "device" simulado para el IDE (FrmBoard) SIN placa.
 *
 * Sirve el protocolo de gestión de placa (STATE / ENV_* / PART_*) sobre TCP con el
 * framing wire v1 (líneas JSON + `\n`), despachando cada comando al bpvm_bmgr REAL
 * → una sola fuente de verdad: el IDE se prueba contra la MISMA lógica (offsets
 * derivados, validación, clamp #292, A/B con seq+1) y el MISMO wire que irán a placa.
 * El único trozo que aquí es "de mentira" es la flash: dos sectores A/B respaldados
 * por un fichero (flash_load/flash_store) en vez de flash_range_*.
 *
 * Uso:  bpvm-boardsim [puerto] [fichero-flash] [flashSizeBytes]
 *   por defecto: 127.0.0.1:5099, "boardsim.flash", 4 MB de flash simulada.
 *
 * El IDE se conecta con BpvmBackend ("host:port") como a cualquier device wire v1.
 * Ver docs/H9_KERNEL_CAPAS.md §Comandos de gestión de placa.
 */
#include "bpvm_bmgr.h"
#include "bpvm_boot.h"
#include "json_min.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define BAD_SOCK INVALID_SOCKET
  #define close_sock closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define BAD_SOCK (-1)
  #define close_sock close
#endif

#define WIRE_LINE_MAX  2048   /* línea JSON máxima (= WIRE_V1_LINE_MAX del firmware) */

/* --- "flash" simulada: dos sectores A/B en RAM, respaldados por un fichero --- */
#define SECTOR      4096u
#define PART_BASE   0x100000u   /* 1 MB reservado (imagen + env) */
#define DEF_FLASH   0x400000u   /* 4 MB por defecto (tipo Pico) */

static uint8_t g_a[SECTOR], g_b[SECTOR], g_scratch[SECTOR];
static bpvm_bmgr_t g_bm;
static const char* g_flash_path = "boardsim.flash";

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
    if (!f) { perror("boardsim: no puedo escribir la flash"); return; }
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

/* --- estado del arranque derivado (para STATE): virgen→0, sin particiones→1, listo→3 --- */
static int derive_state(void) {
    if (!bpvm_bmgr_env(&g_bm, NULL)) return BPVM_BOOT_KERNEL;      /* sin env → suelo */
    bpvm_part_layout_t lay; int bad;
    if (bpvm_bmgr_part_layout(&g_bm, SECTOR, &lay, &bad) != BPVM_PART_OK)
        return BPVM_BOOT_PARTITIONS;                               /* env pero sin tamaños */
    return BPVM_BOOT_APP;                                          /* completo */
}

/* --- dispatch de un request --- */
static void handle(sock_t c, const json_obj_t* obj) {
    char type[32];
    if (json_get_str(obj, "type", type, sizeof type) < 0) return;
    long id = json_get_long(obj, "id", -1);
    char buf[4096]; sb_t s; sb_init(&s, buf, sizeof buf);

    if (!strcmp(type, "HELLO")) {
        sb_raw(&s, "{\"type\":\"HELLO_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"protoVersion\":1,\"serverName\":\"bpvm-boardsim\","
                   "\"serverBuild\":\"h9\",\"capabilities\":[\"META\",\"BOARDMGR\"]}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "PING")) { send_err(c, id, "OK", "pong"); return; }  /* inocuo */
    if (!strcmp(type, "LIST")) {   /* FS vacío: el sim no tiene FS */
        sb_raw(&s, "{\"type\":\"LIST_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"entries\":[]}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "INFO")) {
        char v[64]; int has = bpvm_bmgr_env_get(&g_bm, "board", v, sizeof v) >= 0;
        sb_raw(&s, "{\"type\":\"INFO_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"serverName\":\"bpvm-boardsim\",\"boardName\":\"");
        sb_esc(&s, has ? v : "(sin identidad)"); sb_raw(&s, "\"}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "STATE")) {
        int st = derive_state();
        sb_raw(&s, "{\"type\":\"STATE_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"state\":"); sb_long(&s, st);
        sb_raw(&s, ",\"name\":\""); sb_esc(&s, bpvm_boot_state_name((bpvm_boot_state_t) st));
        sb_raw(&s, "\",\"degraded\":false,\"reason\":\"\"}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "ENV_LS")) {
        int n = bpvm_bmgr_env_count(&g_bm);
        sb_raw(&s, "{\"type\":\"ENV_LS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"entries\":[");
        for (int i = 0; i < n; i++) {
            char k[64], val[128];
            if (!bpvm_bmgr_env_pair_at(&g_bm, i, k, sizeof k, val, sizeof val)) continue;
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"key\":\""); sb_esc(&s, k);
            sb_raw(&s, "\",\"value\":\""); sb_esc(&s, val); sb_raw(&s, "\"}");
        }
        sb_raw(&s, "]}");
        if (s.ok) send_line(c, s.buf); else send_err(c, id, "INTERNAL_ERROR", "reply no cabe");
        return;
    }
    if (!strcmp(type, "ENV_GET")) {
        char key[64], val[256];
        if (json_get_str(obj, "key", key, sizeof key) < 0) { send_err(c, id, "INVALID_PARAM", "falta key"); return; }
        if (bpvm_bmgr_env_get(&g_bm, key, val, sizeof val) < 0) { send_err(c, id, "NOT_FOUND", "clave no existe"); return; }
        sb_raw(&s, "{\"type\":\"ENV_GET_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"value\":\""); sb_esc(&s, val); sb_raw(&s, "\"}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "ENV_SET") || !strcmp(type, "ENV_DEL")) {
        char key[64], val[256];
        if (json_get_str(obj, "key", key, sizeof key) < 0) { send_err(c, id, "INVALID_PARAM", "falta key"); return; }
        const char* value = NULL;
        if (!strcmp(type, "ENV_SET")) {
            if (json_get_str(obj, "value", val, sizeof val) < 0) { send_err(c, id, "INVALID_PARAM", "falta value"); return; }
            value = val;
        }
        int slot = -1;
        if (bpvm_bmgr_env_set(&g_bm, key, value, &slot) != 0) { send_err(c, id, "NO_SPACE", "no cabe en el env"); return; }
        flash_store();
        sb_raw(&s, "{\"type\":\""); sb_raw(&s, type); sb_raw(&s, "_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"slot\":"); sb_long(&s, slot); sb_raw(&s, "}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "PART_LS")) {
        bpvm_part_layout_t lay; int bad;
        bpvm_part_err_t e = bpvm_bmgr_part_layout(&g_bm, SECTOR, &lay, &bad);
        sb_raw(&s, "{\"type\":\"PART_LS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"base\":"); sb_long(&s, (long) g_bm.part_base);
        sb_raw(&s, ",\"usableFlash\":"); sb_long(&s, (long) g_bm.usable_flash);
        if (e == BPVM_PART_ERR_MISSING) {
            sb_raw(&s, ",\"missing\":true,\"parts\":[]}");   /* virgen: el IDE ofrece defaults */
        } else {
            sb_raw(&s, ",\"missing\":false,\"parts\":[");
            for (int i = 0; i < BPVM_PART_COUNT; i++) {
                if (i) sb_raw(&s, ",");
                sb_raw(&s, "{\"name\":\""); sb_esc(&s, bpvm_part_name((bpvm_part_kind_t) i));
                sb_raw(&s, "\",\"offset\":"); sb_long(&s, (long) lay.parts[i].offset);
                sb_raw(&s, ",\"size\":"); sb_long(&s, (long) lay.parts[i].size); sb_raw(&s, "}");
            }
            sb_raw(&s, "]}");
        }
        if (s.ok) send_line(c, s.buf); else send_err(c, id, "INTERNAL_ERROR", "reply no cabe");
        return;
    }
    if (!strcmp(type, "PART_DEFAULTS")) {
        uint32_t sizes[BPVM_PART_COUNT];
        bpvm_bmgr_part_defaults(&g_bm, SECTOR, sizes);
        sb_raw(&s, "{\"type\":\"PART_DEFAULTS_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"parts\":[");
        for (int i = 0; i < BPVM_PART_COUNT; i++) {
            if (i) sb_raw(&s, ",");
            sb_raw(&s, "{\"name\":\""); sb_esc(&s, bpvm_part_name((bpvm_part_kind_t) i));
            sb_raw(&s, "\",\"size\":"); sb_long(&s, (long) sizes[i]); sb_raw(&s, "}");
        }
        sb_raw(&s, "]}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    if (!strcmp(type, "PART_APPLY")) {
        /* tamaños: un campo long por partición, keyed por su nombre (evita parsear arrays) */
        uint32_t sizes[BPVM_PART_COUNT];
        for (int i = 0; i < BPVM_PART_COUNT; i++) {
            long v = json_get_long(obj, bpvm_part_name((bpvm_part_kind_t) i), -1);
            if (v < 0) { send_err(c, id, "INVALID_PARAM", "falta tamano de una particion"); return; }
            sizes[i] = (uint32_t) v;
        }
        int bad = -1, slot = -1;
        bpvm_part_err_t e = bpvm_bmgr_part_apply(&g_bm, SECTOR, sizes, &bad, &slot);
        if (e != BPVM_PART_OK) {
            char m[128];
            snprintf(m, sizeof m, "%s (particion %d: %s)", bpvm_part_err_str(e), bad,
                     bad >= 0 && bad < BPVM_PART_COUNT ? bpvm_part_name((bpvm_part_kind_t) bad) : "?");
            send_err(c, id, "INVALID_PARAM", m);
            return;
        }
        flash_store();
        sb_raw(&s, "{\"type\":\"PART_APPLY_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"slot\":"); sb_long(&s, slot); sb_raw(&s, "}");
        if (s.ok) send_line(c, s.buf);
        return;
    }
    send_err(c, id, "UNSUPPORTED", "comando no soportado por el sim");
}

int main(int argc, char** argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 5099;
    if (argc > 2) g_flash_path = argv[2];
    uint32_t flash = (argc > 3) ? (uint32_t) strtoul(argv[3], NULL, 0) : DEF_FLASH;

    flash_load();
    g_bm.a = g_a; g_bm.b = g_b; g_bm.scratch = g_scratch; g_bm.sector = SECTOR;
    g_bm.part_base = PART_BASE;
    g_bm.usable_flash = bpvm_part_usable_flash(flash, 0);   /* la flash simulada es la verdad */

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

    printf("bpvm-boardsim: 127.0.0.1:%d  flash=%s (%u bytes usables)\n",
           port, g_flash_path, (unsigned) g_bm.usable_flash);
    printf("estado inicial: %s\n", bpvm_boot_state_name((bpvm_boot_state_t) derive_state()));
    fflush(stdout);

    for (;;) {   /* un cliente a la vez (el IDE) */
        sock_t cli = accept(srv, NULL, NULL);
        if (cli == BAD_SOCK) continue;
        printf("boardsim: cliente conectado\n"); fflush(stdout);
        char line[WIRE_LINE_MAX];
        while (recv_line(cli, line, sizeof line) >= 0) {
            json_obj_t obj;
            if (json_parse(line, strlen(line), &obj) == 0) handle(cli, &obj);
            /* líneas no-JSON se ignoran (tolerante) */
        }
        close_sock(cli);
        printf("boardsim: cliente desconectado\n"); fflush(stdout);
    }
}
