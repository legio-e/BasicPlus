/*
 * bt_esp32.c — V8/H2, paso 5 (11-nov): EL BLUETOOTH DE LAS ESP, la radio de debajo de bpvm_bt.c (V8_IDEAS §E1, «El
 * paso 5»). NimBLE como anfitrión, con el controlador del chip (la C5; después la S3, y la P4 por su C6). Publica la
 * tabla 'BT  ' que ya tiene el PC (bt_host.c): encender, apagar, buscar, conectar, desconectar y olvidar. Los estados,
 * lo recordado y el fichero son de lo común; aquí sólo se traduce a NimBLE. Sale del firmware de prueba del 14-oct
 * (C:\tmp\v8bt\p4_hid), que conectó, emparejó y reconectó el teclado y el ratón de Eduardo.
 *
 * APAGADA DEL TODO (Eduardo, 19-oct: manda el ahorro de energía): encender es nimble_port_init, que arranca el
 * controlador y la pila, y su tarea; apagar, parar la tarea y nimble_port_deinit, que los deshace. La RAM vuelve.
 *
 * LOS HILOS: NimBLE avisa desde su tarea. Lo que se le cuenta a lo común desde ahí (la radio lista, un aparato visto)
 * sólo coge su cerrojo un momento. La tabla se llama desde los hilos de la VM, nunca desde la tarea de NimBLE.
 *
 * EL PRIMER TRAMO (11-nov): encender, apagar y buscar. Conectar, desconectar y olvidar, en el segundo.
 */
#include "bt_esp32.h"
#include "bpvm_bios.h"
#include "bpvm_bt.h"
#include "bpvm_pack_api.h"
#include "log.h"

#include "esp_err.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include <stdio.h>
#include <string.h>

void ble_store_config_init(void);   /* store/config: las claves, en la RAM (sin la NVS) */

static bpvm_bt_aviso_fn s_aviso = NULL;
static bpvm_bt_visto_fn s_visto = NULL;
static volatile int s_encendida = 0;   /* NimBLE arrancado (de encender a apagar) */
static uint8_t s_propia = 0;           /* el tipo de nuestra dirección, al sincronizar */

/* ── Encender y apagar ─────────────────────────────────────────────────────────────────────────────────────────────── */

static void al_sincronizar(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc == 0) rc = ble_hs_id_infer_auto(0, &s_propia);
    if (rc != 0) {
        log_printf("bt: NimBLE sin direccion propia (%d)", rc);
        return;
    }
    log_printf("bt: NimBLE en marcha");
    if (s_aviso) s_aviso(BPVM_BT_AV_LISTA, NULL);
}

static void al_reiniciar(int motivo) {
    log_printf("bt: NimBLE se reinicia (motivo %d)", motivo);
}

static void tarea_nimble(void* arg) {
    (void) arg;
    nimble_port_run();            /* hasta que apagar la pare */
    nimble_port_freertos_deinit();
}

static int r_encender(bpvm_bt_aviso_fn aviso, char* err, size_t n) {
    esp_err_t e;
    if (s_encendida) return 0;
    s_aviso = aviso;
    e = nimble_port_init();
    if (e != ESP_OK) {
        snprintf(err, n, "Bluetooth: la radio no arranca (%s)", esp_err_to_name(e));
        return -1;
    }
    ble_hs_cfg.sync_cb = al_sincronizar;
    ble_hs_cfg.reset_cb = al_reiniciar;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_store_config_init();
    s_encendida = 1;
    nimble_port_freertos_init(tarea_nimble);
    return 0;
}

static void r_apagar(void) {
    if (!s_encendida) return;
    if (ble_gap_disc_active()) ble_gap_disc_cancel();
    nimble_port_stop();           /* corta las conexiones y para la tarea */
    nimble_port_deinit();         /* la pila y el controlador: la RAM vuelve */
    s_encendida = 0;
    log_printf("bt: apagado");
}

/* ── Buscar ────────────────────────────────────────────────────────────────────────────────────────────────────────── */

/* Lo que se va sabiendo de cada aparato durante una búsqueda: el anuncio trae si está en modo de emparejar (los flags) y
 * a veces el nombre; el nombre suele llegar aparte, en la respuesta al escaneo. Se cuenta a lo común cuando se sabe que
 * está en modo de emparejar y cómo se llama, una vez. */
typedef struct {
    ble_addr_t addr;
    char       nombre[BPVM_BT_NOMBRE];
    uint16_t   apariencia;
    int8_t     rssi;
    uint8_t    emparejable;
    uint8_t    contado;
} visto_t;

static visto_t s_vistos[BPVM_BT_MAX_ENCONTRADOS];
static int s_nvistos = 0;

static void dir_texto(const ble_addr_t* a, char* s, size_t n) {
    snprintf(s, n, "%02X:%02X:%02X:%02X:%02X:%02X", a->val[5], a->val[4], a->val[3], a->val[2], a->val[1], a->val[0]);
}

/* La apariencia del estándar (la categoría HID es 0x03C0): 0x03C1, teclado; 0x03C2, ratón. Sin ella, aún no se sabe:
 * el tipo seguro sale del aparato al conectarlo (14-oct: el teclado de Eduardo no la anuncia). */
static const char* clase_de(uint16_t apariencia) {
    if (apariencia == 0x03C1) return "keyboard";
    if (apariencia == 0x03C2) return "mouse";
    return "unknown";
}

static visto_t* visto_de(const ble_addr_t* a, int crear) {
    int i;
    for (i = 0; i < s_nvistos; i++) if (ble_addr_cmp(&s_vistos[i].addr, a) == 0) return &s_vistos[i];
    if (!crear || s_nvistos >= BPVM_BT_MAX_ENCONTRADOS) return NULL;
    memset(&s_vistos[s_nvistos], 0, sizeof s_vistos[0]);
    s_vistos[s_nvistos].addr = *a;
    return &s_vistos[s_nvistos++];
}

static int al_descubrir(struct ble_gap_event* ev, void* arg) {
    struct ble_hs_adv_fields f;
    visto_t* v;
    int emparejable;
    (void) arg;
    if (ev->type != BLE_GAP_EVENT_DISC) return 0;
    if (ble_hs_adv_parse_fields(&f, ev->disc.data, ev->disc.length_data) != 0) return 0;
    emparejable = (f.flags & (BLE_HS_ADV_F_DISC_LTD | BLE_HS_ADV_F_DISC_GEN)) != 0;
    /* Sólo se apunta a quien se anuncia para emparejar; lo demás (su respuesta al escaneo), si ya está apuntado. */
    v = visto_de(&ev->disc.addr, emparejable);
    if (!v) return 0;
    if (emparejable) v->emparejable = 1;
    if (f.name != NULL && f.name_len > 0) {
        size_t k = f.name_len < sizeof v->nombre - 1 ? f.name_len : sizeof v->nombre - 1;
        memcpy(v->nombre, f.name, k);
        v->nombre[k] = '\0';
    }
    if (f.appearance_is_present) v->apariencia = f.appearance;
    v->rssi = ev->disc.rssi;
    if (!v->contado && v->emparejable && v->nombre[0] && s_visto) {
        bpvm_bt_aparato_t a;
        memset(&a, 0, sizeof a);
        snprintf(a.nombre, sizeof a.nombre, "%s", v->nombre);
        dir_texto(&v->addr, a.dir, sizeof a.dir);
        snprintf(a.tipo, sizeof a.tipo, "%s",
                 (v->addr.type == BLE_ADDR_PUBLIC || v->addr.type == BLE_ADDR_PUBLIC_ID) ? "public" : "random");
        snprintf(a.clase, sizeof a.clase, "%s", clase_de(v->apariencia));
        a.rssi = v->rssi;
        v->contado = 1;
        s_visto(&a);
    }
    return 0;
}

static int r_buscar(int ms, bpvm_bt_visto_fn visto, char* err, size_t n) {
    struct ble_gap_disc_params p;
    int rc;
    if (!s_encendida) { snprintf(err, n, "Bluetooth.scan: está apagado (turnOn primero)"); return -1; }
    if (ble_gap_disc_active()) ble_gap_disc_cancel();
    s_visto = visto;
    s_nvistos = 0;
    memset(&p, 0, sizeof p);
    p.passive = 0;                /* activo: el nombre suele venir en la respuesta al escaneo */
    p.filter_duplicates = 1;
    p.itvl = 0x50;                /* 50 ms, y escucha 30 de cada 50 (lo del 14-oct) */
    p.window = 0x30;
    rc = ble_gap_disc(s_propia, ms > 0 ? ms : 1, &p, al_descubrir, NULL);
    if (rc != 0) { snprintf(err, n, "Bluetooth.scan: la radio no busca (%d)", rc); return -1; }
    return 0;
}

/* ── Conectar, desconectar y olvidar: el segundo tramo ─────────────────────────────────────────────────────────────── */

static int r_conectar(const char* dir, const char* tipo, char* clase, size_t nclase, char* err, size_t n) {
    (void) dir; (void) tipo;
    if (nclase) clase[0] = '\0';
    snprintf(err, n, "Bluetooth.connect: todavía no en esta placa");
    return -1;
}

static void r_desconectar(const char* dir) { (void) dir; }
static void r_olvidar(const char* dir) { (void) dir; }

/* ── La tabla ──────────────────────────────────────────────────────────────────────────────────────────────────────── */

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "encender",    (bpvm_pack_fn_t) r_encender },
    { "apagar",      (bpvm_pack_fn_t) r_apagar },
    { "buscar",      (bpvm_pack_fn_t) r_buscar },
    { "conectar",    (bpvm_pack_fn_t) r_conectar },
    { "desconectar", (bpvm_pack_fn_t) r_desconectar },
    { "olvidar",     (bpvm_pack_fn_t) r_olvidar },
};
static const bpvm_pack_api_t TABLA = {
    BPVM_BT_MARCA, BPVM_BT_API_VER, sizeof SIMBOLOS / sizeof SIMBOLOS[0], SIMBOLOS
};

void bt_esp32_publicar(void) {
    int r = bpvm_bios_publica(BPVM_BT_MARCA, &TABLA);
    if (r != 0) log_printf("bt: la radio no se pudo publicar (%d)", r);
}
