/*
 * test/debug_listen.c — H6.b.2.b: servidor wire del debugger para la VM-C en
 * HOST (desktop). Es el stand-in del firmware Pico para desarrollar/probar el
 * "Debug on Pico" SIN hardware (cascada Java→C→Pico: ver docs/PHILOSOPHY.md).
 *
 * Rol DEVICE (regla de oro H6): trabaja en pc/sp/bp/memoria CRUDOS, sin `.dbg`.
 * El host (IDE) tiene el `.dbg` y resuelve símbolos (línea↔pc, slot→nombre);
 * por eso los breakpoints son POR PC (el IDE convierte línea→pc) y BP_HIT lleva
 * pc/sp/bp/cs, no file/line.
 *
 * Arquitectura (= la del Pico, mapeada a host):
 *   - thread READER (main): acepta 1 cliente, parsea comandos wire-v1 y los
 *     traduce a las primitivas del núcleo (#215: add/clear breakpoint,
 *     request_pause) + al canal de threads (#216).
 *   - thread WORKER: corre bpvm_run(). En una pausa, el pause_cb envía BP_HIT
 *     por el socket y BLOQUEA en una cond hasta que el reader inyecta
 *     continue/step/stop. La salida del programa (output_cb) va como OUTPUT.
 *
 * Uso: bpvm_dbgserver <port> <fichero.mod>
 * Protocolo: líneas JSON terminadas en '\n' (wire-v1, subconjunto de debug).
 */

#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define BAD_SOCK   INVALID_SOCKET
  #define CLOSESOCK  closesocket
  static void sock_startup(void){ WSADATA w; WSAStartup(MAKEWORD(2,2), &w); }
  static void sock_cleanup(void){ WSACleanup(); }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define BAD_SOCK   (-1)
  #define CLOSESOCK  close
  static void sock_startup(void){}
  static void sock_cleanup(void){}
#endif

#define MEMSZ (512 * 1024)

/* ---- Estado del servidor (compartido reader/worker) ---- */
typedef struct {
    bpvm_t*         vm;
    sock_t          cli;
    pthread_mutex_t wlock;          /* serializa sends al socket */
    /* canal de control (igual patrón que #216 / test_debug_mt) */
    pthread_mutex_t cmtx;
    pthread_cond_t  ccmd;
    int             cmd;            /* acción pendiente o -1 */
    int             session;        /* id de sesión (1 tras RUN) */
} server_t;

/* ---- envío de líneas JSON (thread-safe) ---- */
static void send_line(server_t* s, const char* buf, int len) {
    pthread_mutex_lock(&s->wlock);
    int off = 0;
    while (off < len) {
        int n = (int) send(s->cli, buf + off, len - off, 0);
        if (n <= 0) break;
        off += n;
    }
    char nl = '\n';
    send(s->cli, &nl, 1, 0);
    pthread_mutex_unlock(&s->wlock);
}
/* ---- mini-parser: extrae campos de una línea JSON plana ---- */
/* devuelve 1 + escribe out si encuentra "type":"<...>"; 0 si no. */
static int json_type(const char* line, char* out, int cap) {
    const char* p = strstr(line, "\"type\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0;
    p = strchr(p, '"');  if (!p) return 0;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < cap - 1) out[i++] = *p++;
    out[i] = '\0';
    return 1;
}
/* devuelve el valor entero de "<key>": N, o `def` si no aparece. */
static long json_int(const char* line, const char* key, long def) {
    char pat[48];
    snprintf(pat, sizeof pat, "\"%s\"", key);
    const char* p = strstr(line, pat);
    if (!p) return def;
    p = strchr(p, ':'); if (!p) return def;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    return strtol(p, NULL, 10);
}

/* ---- output_cb: salida del programa → OUTPUT wire (corre en el worker) ---- */
static void out_cb(const char* str, size_t len, void* user) {
    server_t* s = (server_t*) user;
    /* {"type":"OUTPUT","data":"<escapado>"}.  Escapado mínimo (\,",\n,\t). */
    char buf[1024];
    int off = snprintf(buf, sizeof buf, "{\"type\":\"OUTPUT\",\"data\":\"");
    for (size_t i = 0; i < len && off < (int)sizeof buf - 8; i++) {
        char c = str[i];
        if      (c == '"' || c == '\\') { buf[off++] = '\\'; buf[off++] = c; }
        else if (c == '\n')             { buf[off++] = '\\'; buf[off++] = 'n'; }
        else if (c == '\t')             { buf[off++] = '\\'; buf[off++] = 't'; }
        else if (c == '\r')             { /* drop */ }
        else                              buf[off++] = c;
    }
    off += snprintf(buf + off, sizeof buf - off, "\"}");
    send_line(s, buf, off);
}

/* ---- pause_cb: BP_HIT + bloqueo (corre en el worker) ---- */
static bpvm_dbg_action_t pause_cb(bpvm_t* vm, bpvm_thread_t* tc,
                                  uint32_t pc, void* user) {
    (void) vm;
    server_t* s = (server_t*) user;
    char buf[256];
    int n = snprintf(buf, sizeof buf,
        "{\"type\":\"BP_HIT\",\"session\":%d,\"tid\":%d,"
        "\"pc\":%u,\"sp\":%u,\"bp\":%u,\"cs\":%u}",
        s->session, bpvm_thread_id(tc), pc,
        bpvm_thread_sp(tc), bpvm_thread_bp(tc), bpvm_thread_cs(tc));
    send_line(s, buf, n);

    pthread_mutex_lock(&s->cmtx);
    s->cmd = -1;
    while (s->cmd < 0)
        pthread_cond_wait(&s->ccmd, &s->cmtx);
    int act = s->cmd;
    pthread_mutex_unlock(&s->cmtx);
    return (bpvm_dbg_action_t) act;
}

static void post_cmd(server_t* s, int act) {
    pthread_mutex_lock(&s->cmtx);
    s->cmd = act;
    pthread_cond_signal(&s->ccmd);
    pthread_mutex_unlock(&s->cmtx);
}

/* ---- worker: corre el .mod, luego EXITED ---- */
static void* worker_main(void* arg) {
    server_t* s = (server_t*) arg;
    bpvm_status_t st = bpvm_run(s->vm);
    char buf[128];
    int n = snprintf(buf, sizeof buf,
        "{\"type\":\"EXITED\",\"session\":%d,\"status\":\"%s\"}",
        s->session, (st == BPVM_OK) ? "OK" : bpvm_status_str(st));
    send_line(s, buf, n);
    return NULL;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 3) { fprintf(stderr, "uso: bpvm_dbgserver <port> <fichero.mod>\n"); return 2; }
    int port = atoi(argv[1]);

    uint8_t* mem = (uint8_t*) calloc(1, MEMSZ);
    bpvm_t*  vm  = bpvm_init(mem, MEMSZ, 0);
    if (!vm || bpvm_load_mod(vm, argv[2]) != BPVM_OK) {
        fprintf(stderr, "init/load fallo: %s\n", argv[2]); return 1;
    }

    server_t s;
    memset(&s, 0, sizeof s);
    s.vm = vm; s.cmd = -1; s.session = 0;
    pthread_mutex_init(&s.wlock, NULL);
    pthread_mutex_init(&s.cmtx, NULL);
    pthread_cond_init(&s.ccmd, NULL);

    sock_startup();
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == BAD_SOCK) { fprintf(stderr, "socket() fallo\n"); return 1; }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short) port);
    if (bind(srv, (struct sockaddr*)&addr, sizeof addr) != 0 || listen(srv, 1) != 0) {
        fprintf(stderr, "bind/listen :%d fallo\n", port); return 1;
    }
    fprintf(stderr, "[dbgserver] escuchando en 127.0.0.1:%d\n", port);

    s.cli = accept(srv, NULL, NULL);
    if (s.cli == BAD_SOCK) { fprintf(stderr, "accept fallo\n"); return 1; }
    fprintf(stderr, "[dbgserver] cliente conectado\n");

    bpvm_set_pause_cb(vm, pause_cb, &s);
    bpvm_set_output(vm, out_cb, &s);

    pthread_t worker;
    int worker_started = 0;

    /* ---- bucle reader: parsea líneas JSON ---- */
    char line[2048];
    int  ll = 0;
    char rx[1024];
    for (;;) {
        int got = (int) recv(s.cli, rx, sizeof rx, 0);
        if (got <= 0) break;   /* EOF / error */
        for (int i = 0; i < got; i++) {
            char c = rx[i];
            if (c == '\r') continue;
            if (c != '\n') { if (ll < (int)sizeof line - 1) line[ll++] = c; continue; }
            line[ll] = '\0'; ll = 0;
            if (line[0] == '\0') continue;

            char type[32]; type[0] = '\0';
            json_type(line, type, sizeof type);
            long id = json_int(line, "id", 0);

            if (!strcmp(type, "HELLO")) {
                char rep[256];
                int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"HELLO_REPLY\",\"id\":%ld,\"protoVersion\":1,"
                    "\"serverName\":\"bpvm-c\",\"serverBuild\":\"H6.b\","
                    "\"capabilities\":[\"DEBUG\"]}", id);
                send_line(&s, rep, n);
            } else if (!strcmp(type, "PING")) {
                char rep[64]; int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"PONG\",\"id\":%ld}", id);
                send_line(&s, rep, n);
            } else if (!strcmp(type, "SET_BP")) {
                long pc = json_int(line, "pc", -1);
                int bpid = (pc >= 0) ? bpvm_debug_add_breakpoint(vm, (uint32_t) pc) : -1;
                char rep[96]; int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"SET_BP_REPLY\",\"id\":%ld,\"bpId\":%d}", id, bpid);
                send_line(&s, rep, n);
            } else if (!strcmp(type, "CLR_BP")) {
                long bpid = json_int(line, "bpId", -1);
                bpvm_debug_clear_breakpoint(vm, (int) bpid);
                char rep[96]; int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"CLR_BP_REPLY\",\"id\":%ld}", id);
                send_line(&s, rep, n);
            } else if (!strcmp(type, "PAUSE")) {
                bpvm_debug_request_pause(vm);
                char rep[96]; int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"PAUSE_REPLY\",\"id\":%ld}", id);
                send_line(&s, rep, n);
            } else if (!strcmp(type, "RUN")) {
                if (!worker_started) {
                    s.session = 1;
                    char rep[96]; int n = snprintf(rep, sizeof rep,
                        "{\"type\":\"RUN_REPLY\",\"id\":%ld,\"session\":1}", id);
                    send_line(&s, rep, n);
                    pthread_create(&worker, NULL, worker_main, &s);
                    worker_started = 1;
                }
            } else if (!strcmp(type, "CONTINUE")) {
                post_cmd(&s, BPVM_DBG_CONTINUE);
            } else if (!strcmp(type, "STEP")) {
                post_cmd(&s, BPVM_DBG_STEP);
            } else if (!strcmp(type, "STOP") || !strcmp(type, "KILL")) {
                post_cmd(&s, BPVM_DBG_STOP);
            } else {
                char rep[160]; int n = snprintf(rep, sizeof rep,
                    "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"UNSUPPORTED\","
                    "\"message\":\"%s\"}", id, type);
                send_line(&s, rep, n);
            }
        }
    }

    /* cliente desconectó: si el worker sigue bloqueado, desbloquéalo con STOP. */
    if (worker_started) { post_cmd(&s, BPVM_DBG_STOP); pthread_join(worker, NULL); }
    CLOSESOCK(s.cli); CLOSESOCK(srv); sock_cleanup();
    bpvm_destroy(vm); free(mem);
    return 0;
}
