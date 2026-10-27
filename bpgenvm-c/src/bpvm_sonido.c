/*
 * bpvm_sonido.c — V8/H4 (27-oct, #708): EL SONIDO, lo común de la HAL BP. La interfaz y el porqué, en bpvm_sonido.h.
 *
 * LA TAREA. Una sola, creada al primer tocar: en una placa sin sonido no nace nunca, y no gasta más que este fichero.
 * Espera órdenes con el cerrojo y su condición. Para tocar, lee el WAV a trozos de unos 50 ms con bpvm_fs_read_at, por
 * ruta y desplazamiento: no tiene el fichero abierto, así que un PUT o el fin de RUN no chocan con ella (el sistema de
 * ficheros ya tiene su cerrojo para varias tareas). Pasa cada trozo a PCM de 16 bits y se lo da a la cintura, que
 * bloquea al ritmo del sonido. Entre trozo y trozo mira si ha llegado otra orden —la GENERACIÓN: cada tocar y cada
 * parar la suben— y si el volumen ha cambiado.
 *
 * UNO A LA VEZ: tocar con otro sonando sube la generación; la tarea corta el de antes (cerrar sin drenar) y toma el
 * nuevo.
 *
 * `sonando` lo pone a 1 el propio tocar antes de volver —`play` y en seguida `isPlaying()` da true, siempre—, y a 0
 * parar, o la tarea al acabar el fichero (sólo si nadie ha pedido otro entretanto).
 *
 * El espejo en la VM-Java es Sonido.java: la misma cabecera, con las mismas reglas y en el mismo orden (los textos de
 * los errores tienen que salir iguales).
 */
#include "bpvm_sonido.h"
#include "bpvm.h"
#include "bpvm_alloc.h"
#include "bpvm_fs.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <string.h>

#define SON_TROZO_MAX   4096u   /* bytes del WAV por lectura (50 ms a 48 kHz estéreo de 16 bits son 9600: se acota) */
#define SON_PILA        6144u   /* la pila de la tarea; sus búferes van aparte, con bpvm_malloc_os */
#define SON_ESPERA_FIN  1000    /* ms que el fin de RUN espera, como mucho, a que la tarea suelte el camino */
#define SON_VUELTAS_MAX 64      /* trozos de la cabecera que se miran buscando `fmt ` y `data` */

enum { ORDEN_NADA = 0, ORDEN_TOCAR = 1, ORDEN_PARAR = 2 };

typedef struct {
    char     ruta[BPVM_FS_PATH_MAX];
    uint32_t hz;
    int      canales, bits;
    uint32_t datos_off, datos_len;
} son_wav_t;

static const bpvm_sonido_cintura_t* s_cintura = NULL;
static char s_motivo[128] = "esta placa no tiene sonido: no registra ninguno (su ENV no lleva linea `audio`)";

static bpvm_platform_mutex_handle_t  s_m;
static bpvm_platform_cond_handle_t   s_c;
static bpvm_platform_thread_handle_t s_hilo;
static int      s_listo = 0;       /* el cerrojo y la condición, creados (al registrar una cintura) */
static int      s_hilo_vivo = 0;
static int      s_orden = ORDEN_NADA;
static uint32_t s_gen = 0;
static int      s_sonando = 0;
static int      s_tocando = 0;     /* la tarea tiene la cintura abierta */
static volatile int s_volumen = BPVM_SONIDO_VOLUMEN_DEFECTO;
static son_wav_t s_pedido;
static uint32_t s_pos = 0;        /* el progreso del último tocar: bytes del sonido entregados a la cintura… */
static uint32_t s_largo = 0;      /* …y los que tiene */

static uint32_t le32(const uint8_t* b) {
    return (uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24);
}
static uint16_t le16(const uint8_t* b) { return (uint16_t) (b[0] | (b[1] << 8)); }

/* ── El registro ──────────────────────────────────────────────────────────────────────────────────────────────────── */

void bpvm_sonido_registrar(const bpvm_sonido_cintura_t* c) {
    if (!c) { bpvm_sonido_sin_cintura(NULL); return; }
    if (!s_listo) {
        if (bpvm_platform_mutex_init(&s_m) != 0) {
            bpvm_sonido_sin_cintura("esta placa no tiene sonido: no se pudo crear el cerrojo del sonido");
            return;
        }
        if (bpvm_platform_cond_init(&s_c) != 0) {
            bpvm_platform_mutex_destroy(&s_m);
            bpvm_sonido_sin_cintura("esta placa no tiene sonido: no se pudo crear la condicion del sonido");
            return;
        }
        s_listo = 1;
    }
    s_cintura = c;
}

void bpvm_sonido_sin_cintura(const char* motivo) {
    s_cintura = NULL;
    snprintf(s_motivo, sizeof s_motivo, "%s", (motivo && motivo[0]) ? motivo : "esta placa no tiene sonido");
}

int bpvm_sonido_hay(void) { return s_cintura != NULL; }
const char* bpvm_sonido_motivo(void) { return s_motivo; }

/* ── La cabecera del WAV ──────────────────────────────────────────────────────────────────────────────────────────── */

/* RIFF / WAVE, y después los trozos: `fmt ` (el formato) y `data` (las muestras), saltando los demás (LIST…), cada uno
 * con su tamaño y alineado a 2. Se admite PCM (formato 1) de 8 o 16 bits, 1 o 2 canales, de 8000 a 48000 Hz; lo que no,
 * BPVM_SONIDO_FORMATO con lo que trae, comprobado en este orden (el de Sonido.java). */
static int leer_cabecera(const char* ruta, son_wav_t* w, char* det, size_t n) {
    uint8_t b[16];
    uint32_t off = 12;
    int fmt_visto = 0, data_visto = 0, vuelta;
    uint16_t formato = 0;
    long r = bpvm_fs_read_at(ruta, 0, b, 12);
    if (r < 0) return BPVM_SONIDO_NO_ABRE;
    if (r < 12 || memcmp(b, "RIFF", 4) != 0 || memcmp(b + 8, "WAVE", 4) != 0) return BPVM_SONIDO_NO_ES_WAV;
    for (vuelta = 0; vuelta < SON_VUELTAS_MAX && !data_visto; vuelta++) {
        uint32_t tam, sig;
        if (bpvm_fs_read_at(ruta, off, b, 8) < 8) return BPVM_SONIDO_NO_ES_WAV;
        tam = le32(b + 4);
        if (memcmp(b, "fmt ", 4) == 0) {
            if (tam < 16 || bpvm_fs_read_at(ruta, off + 8, b, 16) < 16) return BPVM_SONIDO_NO_ES_WAV;
            formato    = le16(b);
            w->canales = le16(b + 2);
            w->hz      = le32(b + 4);
            w->bits    = le16(b + 14);
            fmt_visto  = 1;
        } else if (memcmp(b, "data", 4) == 0) {
            if (!fmt_visto) return BPVM_SONIDO_NO_ES_WAV;
            w->datos_off = off + 8;
            w->datos_len = tam;
            data_visto   = 1;
            break;
        }
        sig = off + 8u + tam + (tam & 1u);
        if (sig <= off) return BPVM_SONIDO_NO_ES_WAV;   /* un tamaño que da la vuelta */
        off = sig;
    }
    if (!data_visto) return BPVM_SONIDO_NO_ES_WAV;
    if (formato != 1)                     { snprintf(det, n, "formato %u, no es PCM", (unsigned) formato); return BPVM_SONIDO_FORMATO; }
    if (w->bits != 8 && w->bits != 16)    { snprintf(det, n, "%d bits", w->bits); return BPVM_SONIDO_FORMATO; }
    if (w->canales < 1 || w->canales > 2) { snprintf(det, n, "%d canales", w->canales); return BPVM_SONIDO_FORMATO; }
    if (w->hz < 8000u || w->hz > 48000u)  { snprintf(det, n, "%lu Hz", (unsigned long) w->hz); return BPVM_SONIDO_FORMATO; }
    return BPVM_SONIDO_OK;
}

/* ── La tarea ─────────────────────────────────────────────────────────────────────────────────────────────────────── */

/* Toca `w` hasta el final, hasta que llegue otra orden o hasta que algo falle. */
static void tocar(const son_wav_t* w, uint32_t gen, uint8_t* crudo, int16_t* pcm) {
    char err[96];
    uint32_t marco = (uint32_t) (w->canales * (w->bits / 8));
    uint32_t trozo = (w->hz / 20u) * marco;   /* unos 50 ms */
    uint32_t hechos = 0;
    int vol, drenar = 1;
    if (trozo > SON_TROZO_MAX) trozo = SON_TROZO_MAX - (SON_TROZO_MAX % marco);
    err[0] = 0;
    if (s_cintura->abrir(w->hz, w->canales, err, sizeof err) != 0) {
        bpvm_diag("[sonido] '%s': %s no abre (%s)", w->ruta, s_cintura->nombre, err[0] ? err : "sin motivo");
        return;
    }
    vol = s_volumen;
    s_cintura->volumen(vol);
    while (hechos < w->datos_len) {
        uint32_t pide = w->datos_len - hechos, muestras, i;
        int otra, v;
        long r;
        bpvm_platform_mutex_lock(&s_m);
        otra = (s_gen != gen);
        v    = s_volumen;
        bpvm_platform_mutex_unlock(&s_m);
        if (otra) { drenar = 0; break; }
        if (v != vol) { vol = v; s_cintura->volumen(vol); }
        if (pide > trozo) pide = trozo;
        r = bpvm_fs_read_at(w->ruta, w->datos_off + hechos, crudo, pide);
        if (r < 0) {
            bpvm_diag("[sonido] '%s': fallo al leer en el byte %lu: se para", w->ruta, (unsigned long) (w->datos_off + hechos));
            drenar = 0;
            break;
        }
        r -= r % (long) marco;
        if (r <= 0) break;   /* el fichero acaba antes de lo que dice su cabecera */
        if (w->bits == 8) {
            for (i = 0; i < (uint32_t) r; i++) pcm[i] = (int16_t) (((int) crudo[i] - 128) * 256);
            muestras = (uint32_t) r;
        } else {
            for (i = 0; i < (uint32_t) r / 2u; i++) pcm[i] = (int16_t) le16(crudo + 2u * i);
            muestras = (uint32_t) r / 2u;
        }
        if (s_cintura->escribir(pcm, muestras) != 0) {
            bpvm_diag("[sonido] '%s': %s no acepta el sonido: se para", w->ruta, s_cintura->nombre);
            drenar = 0;
            break;
        }
        hechos += (uint32_t) r;
        bpvm_platform_mutex_lock(&s_m);
        if (s_gen == gen) s_pos = hechos;   /* el progreso, sólo si este sigue siendo el sonido pedido */
        bpvm_platform_mutex_unlock(&s_m);
    }
    s_cintura->cerrar(drenar);
}

static void tarea(void* arg) {
    uint8_t* crudo = (uint8_t*) bpvm_malloc_os(SON_TROZO_MAX);
    int16_t* pcm   = (int16_t*) bpvm_malloc_os(SON_TROZO_MAX * sizeof(int16_t));   /* de 8 bits salen tantas como bytes */
    (void) arg;
    if (!crudo || !pcm) bpvm_diag("[sonido] la tarea no tiene memoria para sus bufferes: no sonara nada");
    for (;;) {
        son_wav_t w;
        uint32_t gen;
        int orden;
        bpvm_platform_mutex_lock(&s_m);
        while (s_orden == ORDEN_NADA) bpvm_platform_cond_wait(&s_c, &s_m);
        orden = s_orden;
        s_orden = ORDEN_NADA;
        gen = s_gen;
        w = s_pedido;
        s_tocando = (orden == ORDEN_TOCAR && crudo && pcm);
        bpvm_platform_mutex_unlock(&s_m);
        if (s_tocando) tocar(&w, gen, crudo, pcm);
        bpvm_platform_mutex_lock(&s_m);
        s_tocando = 0;
        if (orden == ORDEN_TOCAR && s_gen == gen) s_sonando = 0;   /* acabó, y nadie ha pedido otro entretanto */
        bpvm_platform_cond_broadcast(&s_c);   /* el fin de RUN espera esto */
        bpvm_platform_mutex_unlock(&s_m);
    }
}

/* ── Las órdenes ──────────────────────────────────────────────────────────────────────────────────────────────────── */

int bpvm_sonido_tocar(const char* ruta, char* det, size_t n) {
    son_wav_t w;
    int r;
    if (det && n) det[0] = 0;
    if (!s_cintura) { snprintf(det, n, "%s", s_motivo); return BPVM_SONIDO_SIN_PLACA; }
    memset(&w, 0, sizeof w);
    snprintf(w.ruta, sizeof w.ruta, "%s", ruta);
    r = leer_cabecera(w.ruta, &w, det, n);
    if (r != BPVM_SONIDO_OK) return r;
    {
        /* Lo que de verdad trae: un WAV grabado a trozos puede decir más (hasta 0xFFFFFFFF) de lo que hay. Y en marcos
         * enteros, que es lo que la tarea entrega. */
        uint32_t tam = 0, marco = (uint32_t) (w.canales * (w.bits / 8));
        if (bpvm_fs_stat(w.ruta, &tam) == 0)
            w.datos_len = (tam > w.datos_off) ? ((tam - w.datos_off < w.datos_len) ? tam - w.datos_off : w.datos_len) : 0u;
        if (w.datos_len > 0x7FFFFFFFu) w.datos_len = 0x7FFFFFFFu;   /* lo que cabe en un integer de BP */
        w.datos_len -= w.datos_len % marco;
    }
    bpvm_platform_mutex_lock(&s_m);
    if (!s_hilo_vivo) {
        /* La tarea, como el ejecutor de una native: con su pila y a la prioridad de quien la crea, la tarea de la VM.
         * Pasa casi todo el tiempo esperando a la cintura. */
        if (bpvm_platform_thread_create_nativa(&s_hilo, tarea, NULL, SON_PILA) != 0) {
            bpvm_platform_mutex_unlock(&s_m);
            return BPVM_SONIDO_SIN_TAREA;
        }
        s_hilo_vivo = 1;
    }
    s_gen++;
    s_pedido  = w;
    s_orden   = ORDEN_TOCAR;
    s_sonando = 1;
    s_pos     = 0;
    s_largo   = w.datos_len;
    bpvm_platform_cond_broadcast(&s_c);
    bpvm_platform_mutex_unlock(&s_m);
    return BPVM_SONIDO_OK;
}

int32_t bpvm_sonido_posicion(void) {
    uint32_t p;
    if (!s_listo) return 0;
    bpvm_platform_mutex_lock(&s_m);
    p = s_pos;
    bpvm_platform_mutex_unlock(&s_m);
    return (int32_t) p;
}

int32_t bpvm_sonido_largo(void) {
    uint32_t l;
    if (!s_listo) return 0;
    bpvm_platform_mutex_lock(&s_m);
    l = s_largo;
    bpvm_platform_mutex_unlock(&s_m);
    return (int32_t) l;
}

void bpvm_sonido_parar(void) {
    if (!s_cintura || !s_hilo_vivo) return;
    bpvm_platform_mutex_lock(&s_m);
    s_gen++;
    s_orden   = ORDEN_PARAR;
    s_sonando = 0;
    bpvm_platform_cond_broadcast(&s_c);
    bpvm_platform_mutex_unlock(&s_m);
}

int bpvm_sonido_sonando(void) {
    int r;
    if (!s_cintura || !s_hilo_vivo) return 0;
    bpvm_platform_mutex_lock(&s_m);
    r = s_sonando;
    bpvm_platform_mutex_unlock(&s_m);
    return r;
}

void bpvm_sonido_poner_volumen(int v) { s_volumen = v; }   /* la tarea lo mira entre trozo y trozo */
int  bpvm_sonido_volumen(void) { return s_volumen; }

void bpvm_sonido_fin_de_run(void) {
    s_volumen = BPVM_SONIDO_VOLUMEN_DEFECTO;
    if (!s_cintura || !s_hilo_vivo) return;
    bpvm_platform_mutex_lock(&s_m);
    s_gen++;
    s_orden   = ORDEN_PARAR;
    s_sonando = 0;
    s_pos     = 0;   /* el progreso, también: el programa siguiente empieza sin ninguno */
    s_largo   = 0;
    bpvm_platform_cond_broadcast(&s_c);
    {
        int64_t t0 = bpvm_platform_now_ms();
        while ((s_tocando || s_orden != ORDEN_NADA) && bpvm_platform_now_ms() - t0 < SON_ESPERA_FIN)
            (void) bpvm_platform_cond_timed_wait(&s_c, &s_m, 50);
    }
    bpvm_platform_mutex_unlock(&s_m);
}
