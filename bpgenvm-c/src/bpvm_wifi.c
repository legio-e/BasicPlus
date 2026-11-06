/*
 * bpvm_wifi.c — V8/H3 (6-nov): LA WIFI, el enlace, lo común de la HAL BP. La interfaz y el porqué, en bpvm_wifi.h.
 *
 * LA TAREA. Una sola, creada al primer encender: en una placa con la WiFi apagada no nace nunca. Espera a que pase algo o a
 * la hora que diga la máquina, le da a la máquina lo que ha pasado y le hace a la radio lo que la máquina diga. Sólo ella
 * llama a la radio: `encender` bloquea segundos, y nadie más tiene que esperar por eso.
 *
 * LA COLA. Lo que pasa —las órdenes y los avisos de la radio, que llegan desde la tarea de eventos de la familia— entra en
 * una cola corta y se atiende en orden: «conectada» y «caída» no son conmutativas.
 */
#include "bpvm_wifi.h"
#include "bpvm.h"
#include "bpvm_bios.h"
#include "bpvm_fs.h"
#include "bpvm_pack_api.h"
#include "bpvm_platform.h"
#include "bpvm_red.h"
#include "json_min.h"

#include <stdio.h>
#include <string.h>

#define WIFI_PILA      8192u   /* `encender` llega hasta la pila de la radio; la marca sale en el log */
#define WIFI_COLA      16u
#define WIFI_JSON_MAX  384

/* ── La máquina ────────────────────────────────────────────────────────────────────────────────────────────────────── */

const char* bpvm_wifi_estado_nombre(int e) {
    switch (e) {
    case BPVM_WIFI_OFF:        return "off";
    case BPVM_WIFI_STARTING:   return "starting";
    case BPVM_WIFI_CONNECTING: return "connecting";
    case BPVM_WIFI_CONNECTED:  return "connected";
    case BPVM_WIFI_NO_NETWORK: return "noNetwork";
    case BPVM_WIFI_BAD_KEY:    return "badKey";
    default:                   return "?";
    }
}

void bpvm_wifi_maq_iniciar(bpvm_wifi_maq_t* m) {
    memset(m, 0, sizeof *m);
    m->estado    = BPVM_WIFI_OFF;
    m->espera_ms = BPVM_WIFI_ESPERA_MIN_MS;
    m->auto_off  = BPVM_WIFI_AUTO_OFF_DEFECTO;
}

/* Con la radio en marcha, y lista para que cuente el apagado por inactividad. */
static int en_marcha(int e) {
    return e == BPVM_WIFI_CONNECTING || e == BPVM_WIFI_CONNECTED || e == BPVM_WIFI_NO_NETWORK;
}

static int64_t apagado_en(const bpvm_wifi_maq_t* m) {
    if (!en_marcha(m->estado) || m->auto_off <= 0 || m->conexiones > 0) return 0;
    return m->actividad_en + (int64_t) m->auto_off * 1000;
}

/* Sin red: se reintenta tras la espera, y la siguiente vez, el doble. */
static void a_reintentar(bpvm_wifi_maq_t* m, int64_t ahora) {
    m->estado       = BPVM_WIFI_NO_NETWORK;
    m->reintento_en = ahora + m->espera_ms;
    m->espera_ms   *= 2u;
    if (m->espera_ms > BPVM_WIFI_ESPERA_MAX_MS) m->espera_ms = BPVM_WIFI_ESPERA_MAX_MS;
}

static int a_conectar(bpvm_wifi_maq_t* m, int64_t ahora) {
    m->estado        = BPVM_WIFI_CONNECTING;
    m->reintento_en  = 0;
    m->conectando_en = ahora;
    return BPVM_WIFI_A_CONECTAR;
}

int bpvm_wifi_maq_paso(bpvm_wifi_maq_t* m, int entrada, int64_t ahora) {
    int conectando = (m->estado == BPVM_WIFI_CONNECTING), conectada = (m->estado == BPVM_WIFI_CONNECTED);
    switch (entrada) {
    case BPVM_WIFI_E_ENCENDER:
        if (m->estado != BPVM_WIFI_OFF && m->estado != BPVM_WIFI_BAD_KEY) return BPVM_WIFI_A_NADA;   /* ya lo está */
        m->estado       = BPVM_WIFI_STARTING;
        m->fallos_clave = 0;
        m->espera_ms    = BPVM_WIFI_ESPERA_MIN_MS;
        m->reintento_en = 0;
        m->actividad_en = ahora;
        return BPVM_WIFI_A_ENCENDER;
    case BPVM_WIFI_E_APAGAR:
    case BPVM_WIFI_E_ROTA:
        /* De badKey también: la radio ya está apagada, pero «apágala» tiene que dejar de decir que la clave no vale. */
        if (m->estado == BPVM_WIFI_OFF) return BPVM_WIFI_A_NADA;
        { int ya = (m->estado == BPVM_WIFI_BAD_KEY);
          m->estado = BPVM_WIFI_OFF;
          m->reintento_en = 0;
          return ya ? BPVM_WIFI_A_NADA : BPVM_WIFI_A_APAGAR; }
    case BPVM_WIFI_E_ARRIBA:
        if (m->estado != BPVM_WIFI_STARTING) return BPVM_WIFI_A_NADA;
        return a_conectar(m, ahora);
    case BPVM_WIFI_E_NO_ARRANCA:
        if (m->estado != BPVM_WIFI_STARTING) return BPVM_WIFI_A_NADA;
        m->estado = BPVM_WIFI_OFF;
        return BPVM_WIFI_A_APAGAR;      /* lo que `encender` haya dejado a medias */
    case BPVM_WIFI_E_CONECTADA:
        if (!conectando && m->estado != BPVM_WIFI_NO_NETWORK) return BPVM_WIFI_A_NADA;
        m->estado       = BPVM_WIFI_CONNECTED;
        m->fallos_clave = 0;
        m->espera_ms    = BPVM_WIFI_ESPERA_MIN_MS;
        m->reintento_en = 0;
        m->actividad_en = ahora;        /* una conexión cuenta como actividad */
        return BPVM_WIFI_A_NADA;
    case BPVM_WIFI_E_CLAVE:
        if (!conectando && !conectada) return BPVM_WIFI_A_NADA;
        if (++m->fallos_clave >= BPVM_WIFI_INTENTOS_CLAVE) {
            m->estado = BPVM_WIFI_BAD_KEY;
            m->reintento_en = 0;
            return BPVM_WIFI_A_APAGAR;  /* reposo = apagada del todo */
        }
        a_reintentar(m, ahora);
        return BPVM_WIFI_A_NADA;
    case BPVM_WIFI_E_SIN_RED:
    case BPVM_WIFI_E_CAIDA:
        if (!conectando && !conectada) return BPVM_WIFI_A_NADA;
        a_reintentar(m, ahora);
        return BPVM_WIFI_A_NADA;
    case BPVM_WIFI_E_TIEMPO: {
        int64_t apagar = apagado_en(m);
        if (apagar && ahora >= apagar) {
            m->estado = BPVM_WIFI_OFF;
            m->reintento_en = 0;
            return BPVM_WIFI_A_APAGAR;
        }
        if (m->estado == BPVM_WIFI_NO_NETWORK && m->reintento_en && ahora >= m->reintento_en) return a_conectar(m, ahora);
        if (conectando && ahora - m->conectando_en >= BPVM_WIFI_CONECTANDO_MS) a_reintentar(m, ahora);
        return BPVM_WIFI_A_NADA;
    }
    default:
        return BPVM_WIFI_A_NADA;
    }
}

int64_t bpvm_wifi_maq_cuando(const bpvm_wifi_maq_t* m) {
    int64_t c = 0, a = apagado_en(m);
    if (m->estado == BPVM_WIFI_NO_NETWORK) c = m->reintento_en;
    if (m->estado == BPVM_WIFI_CONNECTING) c = m->conectando_en + BPVM_WIFI_CONECTANDO_MS;
    if (a && (!c || a < c)) c = a;
    return c;
}

void bpvm_wifi_maq_actividad(bpvm_wifi_maq_t* m, int64_t ahora) { m->actividad_en = ahora; }

void bpvm_wifi_maq_conexion(bpvm_wifi_maq_t* m, int abierta, int64_t ahora) {
    if (abierta) m->conexiones++;
    else if (m->conexiones > 0) m->conexiones--;
    m->actividad_en = ahora;
}

/* ── La configuración ──────────────────────────────────────────────────────────────────────────────────────────────── */

static void di(char* motivo, unsigned cap, const char* texto) {
    if (motivo && cap) snprintf(motivo, cap, "%s", texto);
}

int bpvm_wifi_cfg_parse(const char* json, size_t len, bpvm_wifi_cfg_t* c, char* motivo, unsigned cap) {
    json_obj_t o;
    long off;
    int r;
    memset(c, 0, sizeof *c);
    c->auto_off = BPVM_WIFI_AUTO_OFF_DEFECTO;
    if (motivo && cap) motivo[0] = 0;
    if (json_parse(json, len, &o) != 0) { di(motivo, cap, "no es JSON valido"); return -1; }
    r = json_get_str(&o, "ssid", c->ssid, sizeof c->ssid);
    if (r == -2) { di(motivo, cap, "ssid no cabe (32 bytes)"); return -1; }
    if (r <= 0)  { di(motivo, cap, "falta ssid"); return -1; }
    r = json_get_str(&o, "password", c->clave, sizeof c->clave);
    if (r == -2) { di(motivo, cap, "password no cabe (64 bytes)"); return -1; }
    r = json_get_str(&o, "country", c->pais, sizeof c->pais);
    if (r == -2 || r == 0 || r == 1) { di(motivo, cap, "country son dos letras (ES)"); return -1; }
    c->auto_start = json_get_bool(&o, "autoStart", 0);
    off = json_get_long(&o, "autoOff", BPVM_WIFI_AUTO_OFF_DEFECTO);
    if (off < 0 || off > 0x7FFFFFFFL) { di(motivo, cap, "autoOff son segundos (0: no se apaga sola)"); return -1; }
    c->auto_off = (int32_t) off;
    return 0;
}

/* Sin el fichero, -2: es lo normal en una placa sin configurar. Mal escrito, -1 con el motivo. */
static int leer_fichero(bpvm_wifi_cfg_t* c, char* motivo, unsigned cap) {
    char json[WIFI_JSON_MAX];
    long n;
    if (!bpvm_fs_exists(BPVM_WIFI_FICHERO)) { di(motivo, cap, "no hay " BPVM_WIFI_FICHERO); return -2; }
    n = bpvm_fs_read(BPVM_WIFI_FICHERO, (uint8_t*) json, sizeof json);
    if (n < 0) { di(motivo, cap, "no se puede leer " BPVM_WIFI_FICHERO); return -1; }
    while (n > 0 && (json[n - 1] == '\n' || json[n - 1] == '\r' || json[n - 1] == ' ')) n--;
    return bpvm_wifi_cfg_parse(json, (size_t) n, c, motivo, cap);
}

/* ── La radio ──────────────────────────────────────────────────────────────────────────────────────────────────────── */

typedef int  (*radio_encender_fn)(const char* pais, bpvm_wifi_aviso_fn aviso, char* err, size_t n);
typedef void (*radio_apagar_fn)(void);
typedef int  (*radio_conectar_fn)(const char* ssid, const char* clave, char* err, size_t n);
typedef int  (*radio_rssi_fn)(void);

static radio_encender_fn s_encender;
static radio_apagar_fn   s_apagar;
static radio_conectar_fn s_conectar;
static radio_rssi_fn     s_rssi;
static char s_motivo[96] = "esta placa no tiene WiFi: nadie publica la radio";

/* La tabla, la primera vez que hace falta: un pack la publica al cargar, y puede ser después del arranque. */
static int radio(void) {
    const bpvm_pack_api_t* a;
    const char* mal;
    if (s_encender) return 1;
    a = (const bpvm_pack_api_t*) bpvm_bios_busca(BPVM_WIFI_MARCA);
    if (!a) return 0;
    mal = bpvm_pack_api_verify(a, BPVM_WIFI_MARCA, BPVM_WIFI_API_VER);
    if (mal) { snprintf(s_motivo, sizeof s_motivo, "esta placa no tiene WiFi: la tabla de la radio: %s", mal); return 0; }
    {
        radio_encender_fn e = (radio_encender_fn) bpvm_pack_api_find(a, "encender");
        radio_apagar_fn   p = (radio_apagar_fn)   bpvm_pack_api_find(a, "apagar");
        radio_conectar_fn c = (radio_conectar_fn) bpvm_pack_api_find(a, "conectar");
        if (!e || !p || !c) {
            snprintf(s_motivo, sizeof s_motivo, "esta placa no tiene WiFi: a la tabla de la radio le falta %s",
                     !e ? "encender" : !p ? "apagar" : "conectar");
            return 0;
        }
        s_rssi     = (radio_rssi_fn) bpvm_pack_api_find(a, "rssi");
        s_apagar   = p;
        s_conectar = c;
        s_encender = e;
    }
    return 1;
}

/* ── La tarea ──────────────────────────────────────────────────────────────────────────────────────────────────────── */

static bpvm_platform_mutex_handle_t  s_m;
static bpvm_platform_cond_handle_t   s_c;
static bpvm_platform_thread_handle_t s_hilo;
static int s_listo = 0;       /* el cerrojo y la condición, creados */
static int s_hilo_vivo = 0;

static bpvm_wifi_maq_t s_maq;
static bpvm_wifi_cfg_t s_cfg;
static struct { int entrada, dato; } s_cola[WIFI_COLA];
static unsigned s_cola_cabeza = 0, s_cola_n = 0;

/* Con el cerrojo cogido. La cola no se llena de verdad: lo que entra son unos pocos avisos por intento. */
static void meter(int entrada, int dato) {
    if (s_cola_n == WIFI_COLA) return;
    s_cola[(s_cola_cabeza + s_cola_n) % WIFI_COLA].entrada = entrada;
    s_cola[(s_cola_cabeza + s_cola_n) % WIFI_COLA].dato    = dato;
    s_cola_n++;
    bpvm_platform_cond_broadcast(&s_c);
}

static void pasa(int entrada, int dato) {
    bpvm_platform_mutex_lock(&s_m);
    meter(entrada, dato);
    bpvm_platform_mutex_unlock(&s_m);
}

static void al_aviso(int aviso, int dato) {
    static const int ENTRADA[] = { 0, BPVM_WIFI_E_CONECTADA, BPVM_WIFI_E_SIN_RED, BPVM_WIFI_E_CLAVE, BPVM_WIFI_E_CAIDA,
                                   BPVM_WIFI_E_ROTA };
    if (aviso >= BPVM_WIFI_AV_CONECTADA && aviso <= BPVM_WIFI_AV_ROTA) pasa(ENTRADA[aviso], dato);
}

/* Lo que le toca hacer a la radio, fuera del cerrojo: `encender` bloquea segundos. */
static void hacer(int accion, const bpvm_wifi_cfg_t* cfg) {
    char err[96];
    err[0] = 0;
    switch (accion) {
    case BPVM_WIFI_A_ENCENDER: {
        int64_t t0 = bpvm_platform_now_ms();
        if (s_encender(cfg->pais, al_aviso, err, sizeof err) != 0) {
            bpvm_diag("wifi: no enciende: %s", err[0] ? err : "sin motivo");
            pasa(BPVM_WIFI_E_NO_ARRANCA, 0);
            return;
        }
        bpvm_diag("wifi: encendida en %ld ms (de la pila de su tarea quedan %ld B)",
                  (long) (bpvm_platform_now_ms() - t0), (long) bpvm_platform_pila_libre_actual());
        bpvm_red_preparar();
        pasa(BPVM_WIFI_E_ARRIBA, 0);
        return;
    }
    case BPVM_WIFI_A_CONECTAR:
        if (s_conectar(cfg->ssid, cfg->clave, err, sizeof err) != 0) {
            bpvm_diag("wifi: no se puede conectar: %s", err[0] ? err : "sin motivo");
            pasa(BPVM_WIFI_E_CAIDA, 0);
        }
        return;
    case BPVM_WIFI_A_APAGAR:
        s_apagar();
        return;
    default:
        return;
    }
}

/* Lo que se cuenta al log. Los reintentos dejan de contarse al llegar a la espera más larga: una placa sin su red, un día
 * entero, llenaría el log de lo mismo. */
static void contar(int antes, const bpvm_wifi_maq_t* m, int entrada, int dato, const char* ssid) {
    if (m->estado == antes) return;
    switch (m->estado) {
    case BPVM_WIFI_CONNECTED:
        bpvm_diag("wifi: conectada a '%s'", ssid);
        break;
    case BPVM_WIFI_NO_NETWORK:
        if (m->espera_ms < BPVM_WIFI_ESPERA_MAX_MS || antes == BPVM_WIFI_CONNECTED)
            bpvm_diag("wifi: sin red '%s' (%s, motivo %d): reintenta", ssid,
                      entrada == BPVM_WIFI_E_CLAVE ? "la clave" : entrada == BPVM_WIFI_E_SIN_RED ? "no aparece"
                      : entrada == BPVM_WIFI_E_TIEMPO ? "la radio no contesta" : "caida", dato);
        break;
    case BPVM_WIFI_BAD_KEY:
        bpvm_diag("wifi: la clave de '%s' no vale: %d intentos, y se apaga", ssid, BPVM_WIFI_INTENTOS_CLAVE);
        break;
    case BPVM_WIFI_OFF:
        bpvm_diag("wifi: apagada (%s)", entrada == BPVM_WIFI_E_TIEMPO ? "sin actividad" : entrada == BPVM_WIFI_E_ROTA
                  ? "la radio no contesta" : entrada == BPVM_WIFI_E_NO_ARRANCA ? "no ha encendido" : "pedido");
        break;
    default:
        break;
    }
}

static void tarea(void* arg) {
    (void) arg;
    for (;;) {
        bpvm_wifi_cfg_t cfg;
        int entrada = 0, dato = 0, accion, antes, conectada;
        bpvm_wifi_maq_t foto;
        bpvm_platform_mutex_lock(&s_m);
        for (;;) {
            int64_t cuando, ahora;
            if (s_cola_n) {
                entrada = s_cola[s_cola_cabeza].entrada;
                dato    = s_cola[s_cola_cabeza].dato;
                s_cola_cabeza = (s_cola_cabeza + 1u) % WIFI_COLA;
                s_cola_n--;
                break;
            }
            cuando = bpvm_wifi_maq_cuando(&s_maq);
            ahora  = bpvm_platform_now_ms();
            if (cuando && cuando <= ahora) { entrada = BPVM_WIFI_E_TIEMPO; break; }
            if (cuando) {
                int64_t ms = cuando - ahora;
                (void) bpvm_platform_cond_timed_wait(&s_c, &s_m, ms > 3600000 ? 3600000 : (int) ms);
            } else {
                bpvm_platform_cond_wait(&s_c, &s_m);
            }
        }
        antes  = s_maq.estado;
        accion = bpvm_wifi_maq_paso(&s_maq, entrada, bpvm_platform_now_ms());
        foto   = s_maq;
        cfg    = s_cfg;
        bpvm_platform_mutex_unlock(&s_m);

        contar(antes, &foto, entrada, dato, cfg.ssid);
        /* El enlace, abajo: ya no hay IP, aunque la capa de arriba tarde en darse cuenta. */
        conectada = (antes == BPVM_WIFI_CONNECTED);
        if (conectada && foto.estado != BPVM_WIFI_CONNECTED) bpvm_red_hay_ip(0);
        hacer(accion, &cfg);
    }
}

/* ── Las órdenes ───────────────────────────────────────────────────────────────────────────────────────────────────── */

static int cerrojo(void) {
    if (s_listo) return 0;
    if (bpvm_platform_mutex_init(&s_m) != 0) return -1;
    if (bpvm_platform_cond_init(&s_c) != 0) { bpvm_platform_mutex_destroy(&s_m); return -1; }
    bpvm_wifi_maq_iniciar(&s_maq);
    s_listo = 1;
    return 0;
}

/* Enciende con esta configuración: la deja a la vista de la tarea y le da la orden. */
static int encender_con(const bpvm_wifi_cfg_t* c, char* det, size_t n) {
    bpvm_platform_mutex_lock(&s_m);
    if (!s_hilo_vivo) {
        if (bpvm_platform_thread_create_nativa(&s_hilo, tarea, NULL, WIFI_PILA) != 0) {
            bpvm_platform_mutex_unlock(&s_m);
            snprintf(det, n, "no se pudo crear la tarea de la WiFi");
            return -1;
        }
        s_hilo_vivo = 1;
    }
    s_cfg = *c;
    s_maq.auto_off = c->auto_off;
    meter(BPVM_WIFI_E_ENCENDER, 0);
    bpvm_platform_mutex_unlock(&s_m);
    return 0;
}

int bpvm_wifi_encender(char* det, size_t n) {
    bpvm_wifi_cfg_t c;
    char motivo[96];
    if (det && n) det[0] = 0;
    if (!radio()) { snprintf(det, n, "%s", s_motivo); return -1; }
    if (cerrojo() != 0) { snprintf(det, n, "no se pudo crear el cerrojo de la WiFi"); return -1; }
    if (leer_fichero(&c, motivo, sizeof motivo) != 0) {
        snprintf(det, n, "%s: %s", BPVM_WIFI_FICHERO, motivo);
        return -1;
    }
    return encender_con(&c, det, n);
}

void bpvm_wifi_apagar(void) {
    if (s_listo) pasa(BPVM_WIFI_E_APAGAR, 0);
}

void bpvm_wifi_arranque(int env_wifi) {
    bpvm_wifi_cfg_t c;
    char motivo[96], det[128];
    int r;
    if (!radio()) {
        if (env_wifi == 1) bpvm_diag("wifi: wifi=1, pero %s", s_motivo);
        return;
    }
    if (env_wifi == 0) { bpvm_diag("wifi: apagada (wifi=0 del Entorno)"); return; }
    r = leer_fichero(&c, motivo, sizeof motivo);
    if (r != 0) {
        /* Sin fichero y sin que nadie la pida, es lo normal: apagada y sin decir nada. */
        if (r != -2 || env_wifi == 1) bpvm_diag("wifi: no se enciende: %s", motivo);
        return;
    }
    if (env_wifi != 1 && !c.auto_start) return;
    if (cerrojo() != 0 || encender_con(&c, det, sizeof det) != 0) { bpvm_diag("wifi: no se enciende: sin cerrojo o sin tarea"); return; }
    bpvm_diag("wifi: se enciende (%s), para conectarse a '%s'", env_wifi == 1 ? "wifi=1" : "autoStart", c.ssid);
}

int bpvm_wifi_hay(void) { return radio(); }
const char* bpvm_wifi_motivo(void) { return s_motivo; }

int bpvm_wifi_estado(void) {
    int e;
    if (!s_listo) return BPVM_WIFI_OFF;
    bpvm_platform_mutex_lock(&s_m);
    e = s_maq.estado;
    bpvm_platform_mutex_unlock(&s_m);
    return e;
}

void bpvm_wifi_ssid(char* buf, size_t n) {
    if (n) buf[0] = 0;
    if (!s_listo) return;
    bpvm_platform_mutex_lock(&s_m);
    snprintf(buf, n, "%s", s_cfg.ssid);
    bpvm_platform_mutex_unlock(&s_m);
}

int bpvm_wifi_rssi(void) {
    return (s_rssi && bpvm_wifi_estado() == BPVM_WIFI_CONNECTED) ? s_rssi() : 0;
}

void bpvm_wifi_actividad(void) {
    if (!s_listo) return;
    bpvm_platform_mutex_lock(&s_m);
    bpvm_wifi_maq_actividad(&s_maq, bpvm_platform_now_ms());
    bpvm_platform_cond_broadcast(&s_c);
    bpvm_platform_mutex_unlock(&s_m);
}

void bpvm_wifi_conexion(int abierta) {
    if (!s_listo) return;
    bpvm_platform_mutex_lock(&s_m);
    bpvm_wifi_maq_conexion(&s_maq, abierta, bpvm_platform_now_ms());
    bpvm_platform_cond_broadcast(&s_c);
    bpvm_platform_mutex_unlock(&s_m);
}
