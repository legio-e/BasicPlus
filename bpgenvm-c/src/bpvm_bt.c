/*
 * bpvm_bt.c — V8/H2 (8-nov): EL BLUETOOTH, lo común de la HAL BP. La interfaz y el porqué, en bpvm_bt.h.
 *
 * Sin tarea propia: la radio vuelve en seguida y cuenta lo que pase por sus avisos. Todo el estado, bajo un cerrojo; a la
 * radio se la llama SIN él (en el PC avisa desde dentro de la propia llamada, y en una placa desde su tarea).
 *
 * LO RECORDADO lo escribe la clase Air.Bluetooth (en BP, con el módulo Json); aquí sólo se lee, al encender: los
 * recordados se conectan por su dirección, sin emparejar otra vez (lo probado el 14-oct). El JSON, con los aparatos por su
 * dirección, que es única, en un objeto: json_min no lee listas.
 *
 * LA MEMORIA: una placa sin radio casi no gasta nada (la C3 y la C6 van justas). Las listas (~1,4 KB) se reservan al
 * encender la primera vez, y el búfer del fichero (1,5 KB), sólo mientras se lee; las dos con bpvm_malloc_os, como el
 * sonido: son de la plataforma, no del programa, y las listas duran de un RUN a otro.
 */
#include "bpvm_bt.h"
#include "bpvm.h"
#include "bpvm_alloc.h"
#include "bpvm_bios.h"
#include "bpvm_fs.h"
#include "bpvm_pack_api.h"
#include "bpvm_platform.h"
#include "json_min.h"

#include <stdio.h>
#include <string.h>

#define BT_JSON_MAX  1536   /* 8 recordados de ~120 caracteres, con margen */

/* ── La radio ──────────────────────────────────────────────────────────────────────────────────────────────────────── */

typedef int  (*radio_encender_fn)(bpvm_bt_aviso_fn aviso, char* err, size_t n);
typedef void (*radio_apagar_fn)(void);
typedef int  (*radio_buscar_fn)(int ms, bpvm_bt_visto_fn visto, char* err, size_t n);
typedef int  (*radio_conectar_fn)(const char* dir, const char* tipo, char* clase, size_t nclase, char* err, size_t n);
typedef void (*radio_dir_fn)(const char* dir);

static radio_encender_fn s_encender;
static radio_apagar_fn   s_apagar;
static radio_buscar_fn   s_buscar;
static radio_conectar_fn s_conectar;
static radio_dir_fn      s_desconectar;
static radio_dir_fn      s_olvidar;
static char s_motivo[96] = "Bluetooth: esta placa no lo tiene";

/* La tabla, la primera vez que hace falta (como la WiFi: un pack podría publicarla después del arranque). */
static int radio(void) {
    const bpvm_pack_api_t* a;
    const char* mal;
    if (s_encender) return 1;
    a = (const bpvm_pack_api_t*) bpvm_bios_busca(BPVM_BT_MARCA);
    if (!a) return 0;
    mal = bpvm_pack_api_verify(a, BPVM_BT_MARCA, BPVM_BT_API_VER);
    if (mal) { snprintf(s_motivo, sizeof s_motivo, "Bluetooth: la tabla de la radio: %s", mal); return 0; }
    {
        radio_encender_fn e = (radio_encender_fn) bpvm_pack_api_find(a, "encender");
        radio_apagar_fn   p = (radio_apagar_fn)   bpvm_pack_api_find(a, "apagar");
        radio_buscar_fn   b = (radio_buscar_fn)   bpvm_pack_api_find(a, "buscar");
        radio_conectar_fn c = (radio_conectar_fn) bpvm_pack_api_find(a, "conectar");
        radio_dir_fn      d = (radio_dir_fn)      bpvm_pack_api_find(a, "desconectar");
        radio_dir_fn      o = (radio_dir_fn)      bpvm_pack_api_find(a, "olvidar");
        if (!e || !p || !b || !c || !d || !o) {
            snprintf(s_motivo, sizeof s_motivo, "Bluetooth: a la tabla de la radio le falta %s",
                     !e ? "encender" : !p ? "apagar" : !b ? "buscar" : !c ? "conectar" : !d ? "desconectar" : "olvidar");
            return 0;
        }
        s_apagar = p; s_buscar = b; s_conectar = c; s_desconectar = d; s_olvidar = o;
        s_encender = e;
    }
    return 1;
}

/* ── El estado, bajo el cerrojo ────────────────────────────────────────────────────────────────────────────────────── */

typedef struct { char dir[BPVM_BT_DIR]; char clase[BPVM_BT_CLASE]; } bt_conectado_t;
typedef struct {
    bpvm_bt_aparato_t vistos[BPVM_BT_MAX_ENCONTRADOS];   /* lo que encontró la última búsqueda */
    bt_conectado_t    con[BPVM_BT_MAX_CONECTADOS];       /* lo conectado, con lo que es */
} bt_listas_t;

static bpvm_platform_mutex_handle_t s_m;
static int s_listo = 0;             /* el cerrojo y las listas: sólo en una placa con radio, al encenderla */
static int s_estado = BPVM_BT_OFF;
static int64_t s_busca_hasta = 0;   /* buscando: hasta cuándo (ms) */
static bt_listas_t* s_l = NULL;
static int s_nvistos = 0;
static int s_ncon = 0;
static int s_tocado = 0;            /* un programa lo ha encendido o apagado: al acabar, vuelve a lo del fichero */

static int cerrojo(void) {
    if (s_listo) return 0;
    if (!s_l) {
        s_l = (bt_listas_t*) bpvm_malloc_os(sizeof *s_l);
        if (!s_l) return -1;
        memset(s_l, 0, sizeof *s_l);
    }
    if (bpvm_platform_mutex_init(&s_m) != 0) return -1;
    s_listo = 1;
    return 0;
}
static void coge(void)  { if (s_listo) bpvm_platform_mutex_lock(&s_m); }
static void suelta(void) { if (s_listo) bpvm_platform_mutex_unlock(&s_m); }

static void copia(char* dst, size_t cap, const char* src) {
    size_t n = src ? strlen(src) : 0;
    if (n >= cap) n = cap - 1;
    if (n) memcpy(dst, src, n);
    dst[n] = '\0';
}

/* Con el cerrojo cogido. -1 si no está. */
static int conectado_en(const char* dir) {
    int i;
    for (i = 0; i < s_ncon; i++) if (strcmp(s_l->con[i].dir, dir) == 0) return i;
    return -1;
}
static void quita_conectado(const char* dir) {
    int i = conectado_en(dir);
    if (i < 0) return;
    s_l->con[i] = s_l->con[--s_ncon];
}

/* Lo que cuenta la radio, desde la tarea que sea. */
static void reconectar_recordados(void);
static void al_aviso(int aviso, const char* dir) {
    int lista = 0;
    coge();
    if (aviso == BPVM_BT_AV_LISTA && s_estado == BPVM_BT_STARTING) { s_estado = BPVM_BT_ON; lista = 1; }
    else if (aviso == BPVM_BT_AV_SE_FUE && dir) quita_conectado(dir);
    suelta();
    if (lista) reconectar_recordados();   /* sin el cerrojo: llama a la radio */
}
static void al_visto(const bpvm_bt_aparato_t* a) {
    int i;
    coge();
    if (s_estado == BPVM_BT_SCANNING && conectado_en(a->dir) < 0 && s_nvistos < BPVM_BT_MAX_ENCONTRADOS) {
        for (i = 0; i < s_nvistos; i++) if (strcmp(s_l->vistos[i].dir, a->dir) == 0) break;
        if (i == s_nvistos) s_l->vistos[s_nvistos++] = *a;   /* el mismo, otra vez: no se repite */
    }
    suelta();
}

/* ── Lo recordado: /sys/bluetooth.json ─────────────────────────────────────────────────────────────────────────────── */

/* El fichero, en `buf` (terminado en NUL). 0, o -1 sin fichero o mal leído. */
static int lee_fichero(char* buf, size_t cap, json_obj_t* raiz) {
    long n;
    if (!bpvm_fs_exists(BPVM_BT_FICHERO)) return -1;
    n = bpvm_fs_read(BPVM_BT_FICHERO, (uint8_t*) buf, cap - 1);
    if (n <= 0) return -1;
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) n--;
    buf[n] = '\0';
    return json_parse(buf, (size_t) n, raiz) == 0 ? 0 : -1;
}

/* ¿Arranca encendido? "autoStart", como en /sys/wifi.json (Eduardo: «como en la wifi»). */
static int fichero_auto_start(void) {
    json_obj_t raiz;
    int r = 0;
    char* buf = (char*) bpvm_malloc_os(BT_JSON_MAX);   /* sólo mientras se lee */
    if (!buf) return 0;
    if (lee_fichero(buf, BT_JSON_MAX, &raiz) == 0) r = json_get_bool(&raiz, "autoStart", 0);
    bpvm_free(buf);
    return r;
}

/* Cada recordado, por su dirección: la radio lo conecta (en una placa, lo espera; en el PC, lo encuentra). Si alguno no
 * está o no conecta, no es un error (Eduardo: «al menos que lo intenten pero si no están o no conectan que no se produzca
 * un error»): se pasa al siguiente. */
static void reconectar_recordados(void) {
    json_obj_t raiz, aparatos, a;
    const json_pair_t* d;
    int i;
    char* buf = (char*) bpvm_malloc_os(BT_JSON_MAX);   /* sólo mientras se lee */
    if (!buf) return;
    if (lee_fichero(buf, BT_JSON_MAX, &raiz) != 0) { bpvm_free(buf); return; }
    d = json_find(&raiz, "devices");
    if (!d || d->type != JSON_TYPE_OBJECT || json_parse(d->value, d->value_len, &aparatos) != 0) {
        bpvm_free(buf);
        return;
    }
    for (i = 0; i < aparatos.n_pairs; i++) {
        char dir[BPVM_BT_DIR], tipo[BPVM_BT_TIPO], clase[BPVM_BT_CLASE], err[96];
        const json_pair_t* p = &aparatos.pairs[i];
        size_t n = p->key_len < sizeof dir - 1 ? p->key_len : sizeof dir - 1;
        memcpy(dir, p->key, n); dir[n] = '\0';
        if (p->type != JSON_TYPE_OBJECT || json_parse(p->value, p->value_len, &a) != 0) continue;
        if (json_get_str(&a, "addressType", tipo, sizeof tipo) < 0) copia(tipo, sizeof tipo, "public");
        clase[0] = '\0';
        if (s_conectar(dir, tipo, clase, sizeof clase, err, sizeof err) != 0) continue;
        coge();
        if (conectado_en(dir) < 0 && s_ncon < BPVM_BT_MAX_CONECTADOS) {
            copia(s_l->con[s_ncon].dir, sizeof s_l->con[s_ncon].dir, dir);
            copia(s_l->con[s_ncon].clase, sizeof s_l->con[s_ncon].clase, clase[0] ? clase : "unknown");
            s_ncon++;
        }
        suelta();
    }
    bpvm_free(buf);
}

/* ── Encender y apagar ─────────────────────────────────────────────────────────────────────────────────────────────── */

static int encender(char* det, size_t n) {
    int estaba;
    if (det && n) det[0] = '\0';
    if (!radio()) { snprintf(det, n, "%s", s_motivo); return -1; }
    if (cerrojo() != 0) { snprintf(det, n, "Bluetooth: no se pudo preparar (sin memoria)"); return -1; }
    coge();
    estaba = s_estado;
    if (estaba == BPVM_BT_OFF) s_estado = BPVM_BT_STARTING;
    suelta();
    if (estaba != BPVM_BT_OFF) return 0;   /* ya está, o encendiéndose */
    if (s_encender(al_aviso, det, n) != 0) {
        coge(); s_estado = BPVM_BT_OFF; suelta();
        return -1;
    }
    return 0;
}

static void apagar(void) {
    if (!radio() || !s_listo) return;
    s_apagar();
    coge();
    s_estado = BPVM_BT_OFF;
    s_ncon = 0;
    s_nvistos = 0;
    suelta();
}

int bpvm_bt_hay(void) { return radio(); }

int bpvm_bt_estado(void) {
    int e;
    if (!s_listo) return BPVM_BT_OFF;
    coge();
    if (s_estado == BPVM_BT_SCANNING && bpvm_platform_now_ms() >= s_busca_hasta) s_estado = BPVM_BT_ON;
    e = s_estado;
    suelta();
    return e;
}

int bpvm_bt_programa_enciende(char* det, size_t n) {
    s_tocado = 1;
    return encender(det, n);
}

void bpvm_bt_programa_apaga(void) {
    s_tocado = 1;
    apagar();
}

/* ── Buscar ────────────────────────────────────────────────────────────────────────────────────────────────────────── */

int bpvm_bt_buscar(int ms, char* det, size_t n) {
    if (det && n) det[0] = '\0';
    if (bpvm_bt_estado() < BPVM_BT_ON) { snprintf(det, n, "Bluetooth.scan: está apagado (turnOn primero)"); return -1; }
    if (ms < 0) ms = 0;
    coge();
    s_nvistos = 0;
    s_estado = BPVM_BT_SCANNING;
    s_busca_hasta = bpvm_platform_now_ms() + ms;
    suelta();
    if (s_buscar(ms, al_visto, det, n) != 0) {
        coge(); s_estado = BPVM_BT_ON; suelta();
        return -1;
    }
    return 0;
}

int bpvm_bt_encontrados(void) {
    int k;
    if (!s_listo) return 0;
    coge(); k = s_nvistos; suelta();
    return k;
}

int bpvm_bt_encontrado(int i, bpvm_bt_aparato_t* out) {
    int r = -1;
    if (!s_listo) return -1;
    coge();
    if (i >= 0 && i < s_nvistos) { *out = s_l->vistos[i]; r = 0; }
    suelta();
    return r;
}

/* ── Conectar, desconectar y olvidar ───────────────────────────────────────────────────────────────────────────────── */

int bpvm_bt_conectar(const char* dir, const char* tipo, char* det, size_t n) {
    char clase[BPVM_BT_CLASE];
    int ya;
    if (det && n) det[0] = '\0';
    if (bpvm_bt_estado() < BPVM_BT_ON) { snprintf(det, n, "Bluetooth.connect: está apagado (turnOn primero)"); return -1; }
    coge(); ya = conectado_en(dir) >= 0; suelta();
    if (ya) return 1;
    clase[0] = '\0';
    if (s_conectar(dir, tipo, clase, sizeof clase, det, n) != 0) return 0;   /* no conecta: el programa recibe false */
    coge();
    if (conectado_en(dir) < 0 && s_ncon < BPVM_BT_MAX_CONECTADOS) {
        copia(s_l->con[s_ncon].dir, sizeof s_l->con[s_ncon].dir, dir);
        copia(s_l->con[s_ncon].clase, sizeof s_l->con[s_ncon].clase, clase[0] ? clase : "unknown");
        s_ncon++;
    }
    suelta();
    return 1;
}

void bpvm_bt_desconectar(const char* dir) {
    int estaba;
    if (!s_listo || !radio()) return;
    coge(); estaba = conectado_en(dir) >= 0; quita_conectado(dir); suelta();
    if (estaba) s_desconectar(dir);
}

void bpvm_bt_olvidar(const char* dir) {
    if (!s_listo || !radio()) return;
    coge(); quita_conectado(dir); suelta();
    s_olvidar(dir);
}

int bpvm_bt_conectado(const char* dir) {
    int r;
    if (!s_listo) return 0;
    coge(); r = conectado_en(dir) >= 0; suelta();
    return r;
}

/* V8/H2, paso 2 — ¿hay un teclado conectado? En el PC, sin él, la ventana de la GUI no teclea (V8_IDEAS §E1). */
int bpvm_bt_hay_teclado(void) {
    int i, r = 0;
    if (!s_listo) return 0;
    coge();
    for (i = 0; i < s_ncon; i++) if (strcmp(s_l->con[i].clase, "keyboard") == 0) { r = 1; break; }
    suelta();
    return r;
}

void bpvm_bt_clase_de(const char* dir, char* out, size_t n) {
    int i;
    if (n) out[0] = '\0';
    if (!s_listo) return;
    coge();
    i = conectado_en(dir);
    if (i >= 0) copia(out, n, s_l->con[i].clase);
    suelta();
}

/* ── El arranque y el fin de un RUN ────────────────────────────────────────────────────────────────────────────────── */

void bpvm_bt_arranque(void) {
    char det[128];
    if (!radio() || !fichero_auto_start()) return;
    if (encender(det, sizeof det) != 0) bpvm_diag("bluetooth: no se enciende: %s", det);
}

void bpvm_bt_fin_de_run(void) {
    char det[128];
    if (!s_tocado) return;
    s_tocado = 0;
    if (!radio()) return;   /* sin radio, el programa no pudo cambiar nada */
    if (fichero_auto_start()) {
        if (bpvm_bt_estado() == BPVM_BT_OFF && encender(det, sizeof det) != 0) bpvm_diag("bluetooth: no se enciende: %s", det);
    } else {
        apagar();
    }
}
