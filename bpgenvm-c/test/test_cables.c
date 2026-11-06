/*
 * test_cables.c — [V8/H3] DOS CABLES A LA VEZ (wire_cables.c), en el PC y con dos cables de mentira que lo apuntan todo.
 *
 * Lo que tiene que pasar, escrito a mano: habla el último que ha mandado un mensaje entero, las respuestas van a ése, el ruido
 * no cuenta ni hace esperar, una línea o un bulk a medias tienen su plazo, un cable que se cae no cuelga a nadie, y lo que
 * se le mandaba al de antes se corta. Y con dos tareas escribiendo mientras el activo cambia, ninguna línea sale partida ni
 * mezclada.
 */
#include "bpvm_log.h"
#include "bpvm_platform.h"
#include "bpvm_repl.h"
#include "bpvm_wire_cables.h"
#include "bpvm_wire_v1.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0, buenas = 0;
static void ok(const char* que, int cond) {
    printf("%s %s\n", cond ? "ok   " : "FALLO", que);
    if (cond) buenas++; else fallos++;
}

/* Lo que el REPL de verdad haría; aquí se cuenta. */
static int s_idos = 0;
void bpvm_repl_cliente_ido(void) { s_idos++; }

/* ── Dos cables de mentira ─────────────────────────────────────────────────────────────────────────────────────────── */

#define SALIDA_MAX (1024 * 1024)

typedef struct {
    const char* entra;      /* lo que llegará */
    size_t      pos, largo;
    int         caido;      /* `leer` da -1 */
    int         abierto;
    int         fines;
    int         sondeo_da;  /* lo que dirá el próximo sondeo, una vez */
    int         esperas;    /* cuánto ha llegado a esperar */
    char*       salida;
    size_t      nsal;
} falso_t;

static falso_t L, R;
static volatile int s_sin_esperas = 0;   /* para que el activo cambie muchas veces mientras otros escriben */

static void pon(falso_t* f, const char* texto) {
    f->entra = texto;
    f->pos = 0;
    f->largo = strlen(texto);
}

static void limpia(falso_t* f) {
    char* s = f->salida;
    memset(f, 0, sizeof *f);
    f->salida = s;
    f->abierto = 1;
    f->sondeo_da = BPVM_WIRE_SIGUE;
}

static int leer(falso_t* f, uint8_t* buf, size_t n, int ms) {
    size_t k = 0;
    if (f->caido) return -1;
    while (k < n && f->pos < f->largo) buf[k++] = (uint8_t) f->entra[f->pos++];
    if (k) return (int) k;
    f->esperas += ms;
    if (ms && !s_sin_esperas) bpvm_platform_thread_sleep_ms(ms);
    return 0;
}

static void escribir(falso_t* f, const uint8_t* d, size_t n) {
    if (f->nsal + n <= SALIDA_MAX) { memcpy(f->salida + f->nsal, d, n); f->nsal += n; }
}

static int  l_leer(uint8_t* b, size_t n, int ms) { return leer(&L, b, n, ms); }
static int  r_leer(uint8_t* b, size_t n, int ms) { return leer(&R, b, n, ms); }
static void l_escribir(const uint8_t* d, size_t n) { escribir(&L, d, n); }
static void r_escribir(const uint8_t* d, size_t n) { escribir(&R, d, n); }
static void r_fin(void) { R.fines++; }
static int  r_sondeo(void) { int d = R.sondeo_da; R.sondeo_da = BPVM_WIRE_SIGUE; return d; }
static int  r_abierto(void) { return R.abierto; }

static const bpvm_wire_cable_t LOCAL = { "local", l_leer, l_escribir, NULL, NULL, NULL };
static const bpvm_wire_cable_t RED   = { "red",   r_leer, r_escribir, r_fin, r_sondeo, r_abierto };

static void reinicia(void) {
    limpia(&L);
    limpia(&R);
    s_idos = 0;
}

/* Quién contesta ahora: se manda una línea y se mira por dónde sale. */
static const falso_t* contesta(void) {
    size_t l = L.nsal, r = R.nsal;
    wire_v1_send_line("x", 1);
    return (L.nsal != l) ? &L : (R.nsal != r) ? &R : NULL;
}

static void vuelve_al_local(void) {
    reinicia();
    pon(&L, "{\"a\":0}\n");
    { char b[64]; (void) wire_v1_recv_line(-1, b, sizeof b); }
    reinicia();
}

static int es(const char* b, int n, const char* texto) { return n == (int) strlen(texto) && memcmp(b, texto, (size_t) n) == 0; }

static void prueba_uno_a_la_vez(void) {
    char b[64];
    int n;
    reinicia();
    pon(&L, "{\"a\":1}\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("una linea por el local", es(b, n, "{\"a\":1}"));
    ok("  contesta el local", contesta() == &L);
    ok("  y el local, sin cambio, no corta nada", s_idos == 0);

    pon(&R, "{\"b\":2}\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("una linea por la red", es(b, n, "{\"b\":2}"));
    ok("  ahora contesta la red", contesta() == &R);
    ok("  y lo del local, cortado", s_idos == 1);
    ok("  la red sabe que ha acabado el mensaje", R.fines == 1);

    pon(&L, "{\"c\":3}\r\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("otra por el local (y el \\r no cuenta)", es(b, n, "{\"c\":3}"));
    ok("  vuelve a contestar el local", contesta() == &L);
    ok("  y lo de la red, cortado", s_idos == 2);
}

static void prueba_ruido(void) {
    char b[64];
    int n;
    vuelve_al_local();
    pon(&L, "xx\n\n{\"a\":1}\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("el ruido antes del mensaje se tira", es(b, n, "{\"a\":1}"));

    /* Ruido en el otro cable: ni cambia nada ni hace esperar. */
    pon(&R, "ruido sin fin de linea");
    pon(&L, "{\"b\":2}\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("con ruido en la red, llega lo del local", es(b, n, "{\"b\":2}"));
    ok("  y sigue contestando el local", contesta() == &L);

    vuelve_al_local();
    pon(&L, "ruido");
    pon(&R, "{\"c\":3}\n");
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("con ruido en el activo, el otro no espera", es(b, n, "{\"c\":3}") && R.esperas == 0);
    ok("  y habla el otro", contesta() == &R);
}

static void prueba_poll(void) {
    char b[64];
    int c, n;
    vuelve_al_local();
    ok("sin nada: -1", wire_v1_try_getchar() == -1);
    ok("  sin esperar", L.esperas == 0 && R.esperas == 0);
    pon(&R, "{\"kill\":1}\n");
    c = wire_v1_try_getchar();
    ok("durante un RUN se mira el otro cable", c == '{');
    n = wire_v1_recv_line(c, b, sizeof b);
    ok("  y la linea sale de el", es(b, n, "{\"kill\":1}"));
    ok("  y habla el", contesta() == &R);

    /* El activo, primero. */
    pon(&R, "{\"r\":1}\n");
    pon(&L, "{\"l\":1}\n");
    c = wire_v1_try_getchar();
    n = wire_v1_recv_line(c, b, sizeof b);
    ok("con los dos a la vez, primero el activo", es(b, n, "{\"r\":1}"));
    c = wire_v1_try_getchar();
    n = wire_v1_recv_line(c, b, sizeof b);
    ok("  y luego el otro", es(b, n, "{\"l\":1}"));

    vuelve_al_local();
    R.abierto = 0;
    pon(&R, "{\"r\":1}\n");
    ok("un cable cerrado no se lee", wire_v1_try_getchar() == -1);
}

static void prueba_caidas(void) {
    char b[64];
    int n;
    vuelve_al_local();
    pon(&R, "{\"a\":1}\n");
    (void) wire_v1_recv_line(-1, b, sizeof b);
    s_idos = 0;
    R.sondeo_da = BPVM_WIRE_CLIENTE_IDO;
    ok("el cliente de la red se va", wire_v1_try_getchar() == -1);
    ok("  lo suyo, cortado", s_idos == 1);
    ok("  y habla el local", contesta() == &L);

    /* Y si no era el que hablaba, nadie se entera. */
    s_idos = 0;
    R.sondeo_da = BPVM_WIRE_CLIENTE_IDO;
    (void) wire_v1_try_getchar();
    ok("si se va un cliente que no hablaba, no se corta nada", s_idos == 0 && contesta() == &L);

    /* Se cae a mitad de una línea: no ha llegado nada, y nadie se queda esperando. */
    vuelve_al_local();
    pon(&R, "{\"a\":");
    { int c = wire_v1_try_getchar();
      R.caido = 1;
      R.pos = R.largo;
      n = wire_v1_recv_line(c, b, sizeof b); }
    ok("se cae a media linea: nada", n == 0);
    ok("  y sigue hablando el local", contesta() == &L);

    /* Se cae a mitad de un bulk. */
    vuelve_al_local();
    pon(&R, "{\"put\":1}\n");
    (void) wire_v1_recv_line(-1, b, sizeof b);
    R.caido = 1;
    ok("se cae a medio bulk: -1", wire_v1_recv_bulk((uint8_t*) b, 8, sizeof b) == -1);
}

static void prueba_plazos(void) {
    char b[64], largo[40];
    int n;
    int64_t t0;
    vuelve_al_local();
    memset(largo, 'a', sizeof largo);
    largo[0] = '{';
    largo[sizeof largo - 1] = 0;
    pon(&L, largo);
    t0 = bpvm_platform_now_ms();
    { char corto[16]; n = wire_v1_recv_line(-1, corto, sizeof corto); }
    ok("una linea que no cabe: -1", n == -1);
    ok("  y sin esperar", bpvm_platform_now_ms() - t0 < 200);

    pon(&L, "{\"a\":");
    t0 = bpvm_platform_now_ms();
    n = wire_v1_recv_line(-1, b, sizeof b);
    ok("una linea a medias: -2", n == -2);
    ok("  a los 3 s", bpvm_platform_now_ms() - t0 >= (int64_t) WIRE_V1_ESTANCADA_MS - 50);

    pon(&L, "1234");
    t0 = bpvm_platform_now_ms();
    n = wire_v1_recv_bulk((uint8_t*) b, 8, sizeof b);
    ok("un bulk a medias: -2", n == -2);
    ok("  a los 3 s", bpvm_platform_now_ms() - t0 >= (int64_t) WIRE_V1_BULK_ESTANCADO_MS - 50);
    ok("un bulk que no cabe: -1", wire_v1_recv_bulk((uint8_t*) b, sizeof b + 1, sizeof b) == -1);
    pon(&L, "12345678");
    ok("un bulk entero", wire_v1_recv_bulk((uint8_t*) b, 8, sizeof b) == 8 && memcmp(b, "12345678", 8) == 0);

    /* La cabecera por la red y el bulk detrás, por la red aunque el local tenga algo. */
    vuelve_al_local();
    pon(&R, "{\"put\":1}\nABCD");
    pon(&L, "WXYZ");
    (void) wire_v1_recv_line(-1, b, sizeof b);
    ok("el bulk sale del cable de su cabecera", wire_v1_recv_bulk((uint8_t*) b, 4, sizeof b) == 4 && memcmp(b, "ABCD", 4) == 0);
}

static void prueba_mensaje(void) {
    char b[64];
    vuelve_al_local();
    pon(&R, "{\"a\":1}\n");
    (void) wire_v1_recv_line(-1, b, sizeof b);
    R.nsal = 0;
    R.fines = 0;
    wire_v1_send_msg("{\"m\":1}", 7, (const uint8_t*) "BYTES", 5);
    ok("la linea, su salto y sus bytes, seguidos", R.nsal == 13 && memcmp(R.salida, "{\"m\":1}\nBYTES", 13) == 0);
    ok("  y un solo fin de mensaje", R.fines == 1);
    wire_v1_send_bulk((const uint8_t*) "Z", 1);
    ok("el bulk suelto, igual", R.nsal == 14 && R.salida[13] == 'Z' && R.fines == 2);
    wire_v1_send_bulk((const uint8_t*) "Z", 0);
    ok("un bulk vacio no es un mensaje", R.fines == 2);
}

/* ── Las líneas, enteras, con el activo cambiando ──────────────────────────────────────────────────────────────────── */

#define LINEAS 4000
static volatile int s_quietos = 0;

static void escritor(void* arg) {
    char l[80];
    int i, quien = (int) (intptr_t) arg;
    for (i = 0; i < LINEAS; i++) {
        int n = snprintf(l, sizeof l, "{\"quien\":%d,\"i\":%d,\"relleno\":\"%040d\"}", quien, i, i);
        wire_v1_send_line(l, (size_t) n);
        if (i % 20 == 0) bpvm_platform_thread_sleep_ms(1);
    }
    __atomic_add_fetch(&s_quietos, 1, __ATOMIC_SEQ_CST);
}

/* Cuenta las líneas de una salida; 0 si alguna está partida o mezclada. */
static int lineas_buenas(const falso_t* f, int* cuantas) {
    size_t i = 0;
    *cuantas = 0;
    while (i < f->nsal) {
        const char* fin = memchr(f->salida + i, '\n', f->nsal - i);
        int q, k;
        char relleno[48];
        if (!fin) return 0;
        if (sscanf(f->salida + i, "{\"quien\":%d,\"i\":%d,\"relleno\":\"%40[0-9]\"}", &q, &k, relleno) != 3) return 0;
        if ((size_t) (fin - (f->salida + i)) != strlen("{\"quien\":0,\"i\":,\"relleno\":\"\"}") + strlen(relleno)
                                                 + (size_t) snprintf(NULL, 0, "%d", k)) return 0;
        if (atoi(relleno) != k) return 0;
        (*cuantas)++;
        i = (size_t) (fin - f->salida) + 1;
    }
    return 1;
}

static void prueba_escritores(void) {
    bpvm_platform_thread_handle_t h1, h2;
    char b[64];
    int nl = 0, nr = 0, cambios = 0;
    vuelve_al_local();
    s_quietos = 0;
    s_sin_esperas = 1;
    ok("dos escritores", bpvm_platform_thread_create(&h1, escritor, (void*) (intptr_t) 1) == 0
                         && bpvm_platform_thread_create(&h2, escritor, (void*) (intptr_t) 2) == 0);
    while (__atomic_load_n(&s_quietos, __ATOMIC_SEQ_CST) < 2) {
        pon(&R, "{\"r\":1}\n");
        (void) wire_v1_recv_line(-1, b, sizeof b);
        pon(&L, "{\"l\":1}\n");
        (void) wire_v1_recv_line(-1, b, sizeof b);
        cambios += 2;
    }
    s_sin_esperas = 0;
    bpvm_platform_thread_join(&h1);
    bpvm_platform_thread_join(&h2);
    ok("el local: todas enteras", lineas_buenas(&L, &nl));
    ok("la red: todas enteras", lineas_buenas(&R, &nr));
    printf("      (%d lineas por el local y %d por la red, con %d cambios de cable)\n", nl, nr, cambios);
    ok("no se pierde ninguna", nl + nr == 2 * LINEAS);
    ok("  y han salido por los dos", nl > 0 && nr > 0);
}

int main(void) {
    L.salida = (char*) malloc(SALIDA_MAX);
    R.salida = (char*) malloc(SALIDA_MAX);
    bpvm_wire_cables_local(&LOCAL);
    bpvm_wire_cables_red(&RED);
    prueba_uno_a_la_vez();
    prueba_ruido();
    prueba_poll();
    prueba_caidas();
    prueba_plazos();
    prueba_mensaje();
    prueba_escritores();
    printf("\n%d buenas, %d fallos  [status=%s]\n", buenas, fallos, fallos ? "FALLO" : "OK");
    return fallos ? 1 : 0;
}
