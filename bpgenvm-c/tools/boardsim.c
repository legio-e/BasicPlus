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
 * Uso:  bpvm-boardsim [--pack=<f.pack>]... [puerto] [fichero-flash] [flashSizeBytes]
 *   por defecto: 127.0.0.1:5099, "boardsim.flash", 4 MB de flash simulada.
 *   --pack=<f.pack> (repetible): H3 — "graba" la imagen en la zona de packs
 *   simulada (región RAM, 0xFF = NOR virgen) para que PACK_LS la liste, igual
 *   que el --pack= de la VM-C host.
 *
 * El IDE se conecta con BpvmBackend ("host:port") como a cualquier device wire v1.
 * Ver docs/H9_KERNEL_CAPAS.md §Comandos de gestión de placa.
 */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_boot.h"
#include "bpvm_pack.h"
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

/* H3 — zona de packs simulada (región RAM que hace de flash; 0xFF = borrada).
 * Se precarga con los --pack= y PACK_LS la sirve por el dispatch compartido. */
#define PACKS_REGION_SIZE (1024u * 1024u)
static uint8_t g_packs[PACKS_REGION_SIZE];

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
    if (!f) { fprintf(stderr, "boardsim: --pack: no puedo abrir %s\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* img = (sz > 0) ? (uint8_t*) malloc((size_t) sz) : NULL;
    if (!img || fread(img, 1, (size_t) sz, f) != (size_t) sz) {
        fprintf(stderr, "boardsim: --pack: error leyendo %s\n", path);
        free(img); fclose(f); return -1;
    }
    fclose(f);
    int32_t off = bpvm_pack_add(g_packs, PACKS_REGION_SIZE, img, (uint32_t) sz);
    free(img);
    if (off < 0) {
        fprintf(stderr, "boardsim: --pack: %s %s\n", path,
                (off == BPVM_PACK_ERR_BADIMG) ? "no es un pack valido" : "no cabe");
        return -1;
    }
    printf("boardsim: pack %s grabado en 0x%06X\n", path, (unsigned) off);
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

/* --- dispatch de un request --- */
static void handle(sock_t c, const json_obj_t* obj) {
    char type[32];
    if (json_get_str(obj, "type", type, sizeof type) < 0) return;
    long id = json_get_long(obj, "id", -1);
    char buf[4096]; sb_t s; sb_init(&s, buf, sizeof buf);

    if (!strcmp(type, "HELLO")) {
        sb_raw(&s, "{\"type\":\"HELLO_REPLY\",\"id\":"); sb_long(&s, id);
        sb_raw(&s, ",\"protoVersion\":1,\"serverName\":\"bpvm-boardsim\","
                   "\"serverBuild\":\"h9\",\"capabilities\":[\"META\",\"BOARDMGR\",\"PACKS\"]}");
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
        /* H3 — bulk crudo tras la línea (PACK_BURN_DATA): leerlo SIEMPRE que se
         * anuncie, aunque sobre, para no desincronizar el wire. */
        static uint8_t s_bulk[BPVM_PACK_BURN_CHUNK];
        long bulk = json_get_long(obj, "bulk", -1);
        if (bulk > 0) {
            if (bulk > (long) sizeof s_bulk) {
                uint8_t sink[512];
                long rem = bulk;
                while (rem > 0) {
                    size_t chunk = rem < (long) sizeof sink ? (size_t) rem : sizeof sink;
                    if (recv_exact(c, sink, chunk) != 0) return;
                    rem -= (long) chunk;
                }
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

int main(int argc, char** argv) {
    /* H3: los --pack= se filtran primero; el resto sigue siendo posicional. */
    memset(g_packs, 0xFF, sizeof g_packs);   /* zona de packs virgen */
    const char* pos[3]; int npos = 0;
    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "--pack=", 7)) {
            if (packs_preload(argv[i] + 7) != 0) return 1;
        } else if (npos < 3) {
            pos[npos++] = argv[i];
        }
    }
    int port = (npos > 0) ? atoi(pos[0]) : 5099;
    if (npos > 1) g_flash_path = pos[1];
    uint32_t flash = (npos > 2) ? (uint32_t) strtoul(pos[2], NULL, 0) : DEF_FLASH;

    flash_load();
    g_bm.a = g_a; g_bm.b = g_b; g_bm.scratch = g_scratch; g_bm.sector = SECTOR;
    g_bm.part_base = PART_BASE;
    g_bm.usable_flash = bpvm_part_usable_flash(flash, 0);   /* la flash simulada es la verdad */
    g_bm.packs_base = g_packs;                              /* H3: PACK_LS sirve esta región */
    g_bm.packs_size = PACKS_REGION_SIZE;
    g_bm.packs_flash = &g_packs_fl;                         /* H3: BURN escribible en el sim */

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
    printf("estado inicial: %s\n", bpvm_boot_state_name((bpvm_boot_state_t) bpvm_bmgr_wire_state(&g_bm)));
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
