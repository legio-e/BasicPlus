/*
 * test_net.c — H11 (#241): cliente TCP de BP contra un echo server local.
 *
 * Modo test (por defecto):
 *   1. Levanta un echo server en 127.0.0.1:47391 (thread propio): acepta
 *      UNA conexión y devuelve cada chunk recibido tal cual.
 *   2. Carga TcpEchoTest.mod + Net.mod + Core.mod y corre la VM con un
 *      sink que captura stdout del programa.
 *   3. PASS si status==OK y la salida contiene los ecos esperados.
 *
 * Modo serve (paridad dual-VM):
 *   test_net serve [segundos]  → SOLO el echo server (acepta conexiones
 *   en bucle hasta agotar el tiempo). Permite correr el MISMO .mod con
 *   la VM-Java y la VM-C y comparar stdout byte a byte.
 *
 * Salida esperada del .mod (ver samples/TcpEchoTest.bp):
 *   conectado / eco: hola eco / eco: BasicPlus en red /
 *   timeout-len: 0 / tras-close: false / fin
 */
#include "bpvm.h"
#include "bpvm_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET srv_sock_t;
  #define SRV_INVALID INVALID_SOCKET
  #define srv_close   closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int srv_sock_t;
  #define SRV_INVALID (-1)
  #define srv_close   close
#endif

#define ECHO_PORT 47391

/* ---------- echo server ---------- */

typedef struct { int accept_loops; } srv_args_t;

static void* echo_server(void* arg) {
    srv_args_t* sa = (srv_args_t*) arg;
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    srv_sock_t ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == SRV_INVALID) return NULL;
    int yes = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*) &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7F000001u);   /* 127.0.0.1 */
    addr.sin_port        = htons(ECHO_PORT);
    if (bind(ls, (struct sockaddr*) &addr, sizeof(addr)) != 0 ||
        listen(ls, 4) != 0) {
        srv_close(ls);
        return NULL;
    }
    for (int loops = 0; loops < sa->accept_loops; loops++) {
        srv_sock_t c = accept(ls, NULL, NULL);
        if (c == SRV_INVALID) break;
        char buf[1024];
        for (;;) {
            int n = (int) recv(c, buf, sizeof(buf), 0);
            if (n <= 0) break;
            int off = 0;
            while (off < n) {
                int w = (int) send(c, buf + off, n - off, 0);
                if (w <= 0) { off = n; break; }
                off += w;
            }
        }
        srv_close(c);
    }
    srv_close(ls);
    return NULL;
}

/* ---------- captura de stdout del programa BP ---------- */

static char  s_out[8192];
static size_t s_out_len = 0;

static void capture_sink(const char* data, size_t len, void* user) {
    (void) user;
    if (s_out_len + len < sizeof(s_out) - 1) {
        memcpy(s_out + s_out_len, data, len);
        s_out_len += len;
        s_out[s_out_len] = 0;
    }
    fwrite(data, 1, len, stdout);   /* eco al exterior, útil en paridad */
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc > 1 && strcmp(argv[1], "serve") == 0) {
        /* Modo paridad: server en primer plano, N segundos ≈ N conexiones
         * (cada VM abre una). accept es bloqueante: usamos el nº de loops
         * como límite simple — 1 loop por conexión esperada. */
        int loops = (argc > 2) ? atoi(argv[2]) : 4;
        srv_args_t sa = { loops };
        fprintf(stderr, "[net] echo server en 127.0.0.1:%d (%d conexiones)\n",
                ECHO_PORT, loops);
        echo_server(&sa);
        return 0;
    }

    const char* main_mod = (argc > 1) ? argv[1] : "samples/TcpEchoTest.mod";
    const char* net_mod  = (argc > 2) ? argv[2] : "../bpstdlib/Net.mod";
    const char* core_mod = (argc > 3) ? argv[3] : "../bpstdlib/Core.mod";

    srv_args_t sa = { 1 };
    pthread_t th;
    pthread_create(&th, NULL, echo_server, &sa);

    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); return 1; }

    bpvm_net_register_host();
    bpvm_set_output(vm, capture_sink, NULL);

    if (bpvm_load_mod(vm, main_mod) != BPVM_OK ||
        bpvm_load_mod(vm, net_mod)  != BPVM_OK ||
        bpvm_load_mod(vm, core_mod) != BPVM_OK) {
        fprintf(stderr, "load_mod falló (¿faltan .mod? regenera con el frontend)\n");
        return 1;
    }

    bpvm_status_t s = bpvm_run(vm);
    pthread_join(th, NULL);

    int pass = (s == BPVM_OK)
            && strstr(s_out, "conectado")          != NULL
            && strstr(s_out, "eco: hola eco")      != NULL
            && strstr(s_out, "eco: BasicPlus en red") != NULL
            && strstr(s_out, "timeout-len: 0")     != NULL
            && strstr(s_out, "fin")                != NULL;
    fprintf(stderr, "[net] status=%s %s\n", bpvm_status_str(s),
            pass ? "PASS" : "FAIL");

    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}
