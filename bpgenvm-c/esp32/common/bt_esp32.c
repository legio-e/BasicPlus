/*
 * bt_esp32.c — V8/H2, paso 5 (11-nov): EL BLUETOOTH DE LAS ESP, la radio de debajo de bpvm_bt.c (V8_IDEAS §E1, «El
 * paso 5»). NimBLE como anfitrión, con el controlador del chip (la C5; después la S3, y la P4 por su C6). Publica la
 * tabla 'BT  ' (versión 2): encender, apagar, buscar, conectar, desconectar y olvidar. Los estados, lo recordado y su
 * fichero son de lo común; aquí sólo se traduce a NimBLE. Sale del firmware de prueba del 14-oct (C:\tmp\v8bt\p4_hid),
 * que conectó, emparejó y reconectó el teclado y el ratón de Eduardo.
 *
 * APAGADA DEL TODO (Eduardo, 19-oct: manda el ahorro de energía): encender es nimble_port_init, que arranca el
 * controlador y la pila, y su tarea; apagar, parar la tarea y nimble_port_deinit, que los deshace. La RAM vuelve.
 *
 * UNA SOLA TAREA TOCA NIMBLE Y LOS APARATOS: la de NimBLE. Lo que piden los demás hilos (conectar, desconectar, olvidar,
 * buscar) le llega como un evento a su cola, y lo atiende allí, entre los suyos: así no hacen falta cerrojos entre
 * tareas. Conectar espera a que acabe (un semáforo); lo demás, no.
 *
 * EL ÁRBITRO: el controlador hace un procedimiento de conexión a la vez, y no a la vez que una búsqueda. Lo que toca, por
 * orden: una búsqueda pedida; un connect del programa (a la dirección del aparato, hasta su plazo); y, si no hay nada de
 * eso ni un aparato preparándose, LA ESPERA DE LOS RECORDADOS: una conexión sin plazo a la lista blanca del controlador,
 * con los recordados que no están (la idea de Eduardo del 14-oct: por su dirección, sin escanear; un aparato vinculado ya
 * no se anuncia con su nombre). Para empezar otra cosa, la espera se cancela, y se vuelve a ella al acabar.
 *
 * CONECTAR UN APARATO (lo del 14-oct): conectar; pedir el cifrado (la primera vez empareja, en Just Works: la placa no
 * tiene con qué enseñar ni teclear un código; las siguientes, cifra con la clave guardada); buscar el servicio HID
 * (0x1812) y sus características; de cada informe (0x2A4D), leer su referencia (0x2908: su número y si es de entrada o
 * de salida) y, si es de entrada, suscribirse (su CCCD, 0x2902); y leer el descriptor de informes (0x2A4B), del que sale
 * lo que es (la primera colección de aplicación del escritorio genérico: teclado o ratón) y qué es cada informe.
 *
 * LO QUE MANDAN (la pieza 2, 11-nov), al teclado y al ratón del SO: cada informe, con el número de su referencia, a
 * bpvm_hid.c, que lo reparte según el descriptor. Y LAS LUCES DE LOS BLOQUEOS (Bloq Num, Bloq Mayús), que lleva la placa:
 * al conectar un teclado y cuando cambian, a su informe de salida. Y para la GUI del pack (la pieza 3, 12-nov), al
 * publicarse, la tabla 'ENTR' (bpvm_entrada.c): con la radio, la placa trae teclado y ratón.
 *
 * LAS CLAVES, en /sys/bluetooth.keys (Eduardo, 11-nov), no en la NVS: una actualización de la imagen la borra (14-oct).
 * NimBLE las guarda en su almacén de la RAM (store/config, sin la NVS); aquí se envuelven sus escrituras y borrados, y al
 * encender se carga el fichero. Y con ellas, los ALIAS: si un aparato se vincula con otra dirección que la que conoce el
 * programa (la de la búsqueda), la radio la traduce. EL FICHERO NO SE ESCRIBE EN LA TAREA DE NIMBLE (medido el 11-nov
 * en la C5): littlefs pide pila, y con la de NimBLE a 8 KB al controlador ya no le quedaba memoria contigua para empezar
 * una búsqueda (el error 0x07 del HCI, «Memory Capacity Exceeded»). Allí sólo se preparan los bytes; los escribe la tarea
 * del Bluetooth de lo común al acabar un connect, o el hilo del programa al olvidar un aparato o al apagar.
 *
 * LOS PARÁMETROS DE CONEXIÓN: los de la placa (7,5–15 ms, sin latencia); los cambios que piden los aparatos, rechazados
 * (14-oct: aceptar el del ratón colgó la conexión a los 40 s).
 */
#include "bt_esp32.h"
#include "bpvm_bios.h"
#include "bpvm_bt.h"
#include "bpvm_entr.h"
#include "bpvm_fs.h"
#include "bpvm_hid.h"
#include "bpvm_pack_api.h"
#include "log.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "store/config/ble_store_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ble_store_config_init(void);   /* store/config: las claves, en la RAM (sin la NVS) */
int  ble_hs_misc_restore_irks(void); /* de la pila (ble_hs_misc.c): las IRK guardadas, a la lista de resolución */

#define MAX_APARATOS   BPVM_BT_MAX_CONECTADOS   /* los que conoce la radio: conectados, conectándose o esperados */
#define MAX_CHR        32                        /* las características del servicio HID de un aparato */
#define MAPA_MAX       1024                      /* el descriptor de informes (el ratón de Eduardo, 189 B) */
#define UUID_HID       0x1812
#define CLAVES_FICHERO "/sys/bluetooth.keys"
#define ORDENES        6                         /* encargos de otros hilos en vuelo a la vez */

/* La conexión que pide la placa: 7,5–15 ms, sin latencia, 4 s de supervisión (lo del 14-oct). */
static const struct ble_gap_conn_params CONEXION = {
    .scan_itvl = 0x30, .scan_window = 0x30,
    .itvl_min = 6, .itvl_max = 12, .latency = 0, .supervision_timeout = 400,
    .min_ce_len = 0, .max_ce_len = 0,
};

/* ── Los aparatos (sólo los toca la tarea de NimBLE) ───────────────────────────────────────────────────────────────── */

enum { E_NADA = 0, E_CONECTANDO = 1, E_PREPARANDO = 2, E_LISTO = 3 };

typedef struct {
    uint16_t uuid, def, val;
    uint8_t  props;
    uint8_t  id_informe, tipo_informe;   /* de su referencia (0x2908); tipo 1, de entrada */
} chr_t;

typedef struct {
    int        en_uso;
    char       dir[BPVM_BT_DIR];     /* la que conoce el programa */
    ble_addr_t addr;                 /* a la que se conecta: la de `dir`, o la del vínculo si es otra (un alias) */
    int        esperado;             /* recordado: se le espera por su dirección */
    int        estado;
    int        preparando;           /* ya se busca su servicio HID: un segundo aviso de cifrado no lo repite */
    int        pedido;               /* un connect del programa le espera */
    uint16_t   conn;
    uint16_t   svc_ini, svc_fin;
    int        nchr, paso, subpaso;
    chr_t      chr[MAX_CHR];
    uint16_t   cccd, ref;            /* los descriptores de la característica que se prepara */
    uint16_t   mapa;
    uint8_t*   desc;                 /* el descriptor de informes, mientras se lee */
    int        ndesc;
    char       clase[BPVM_BT_CLASE];
    bpvm_hid_mapa_t    hid;          /* lo que dice el descriptor: qué es cada informe y dónde está cada cosa */
    bpvm_hid_teclado_t teclado;      /* lo que tenía apretado, en su informe anterior */
    uint16_t   luces_h;              /* su informe de salida con las luces de los bloqueos (0: no tiene) */
    int        luces_sin_resp;       /* y se escribe sin respuesta */
} aparato_t;

static aparato_t* s_ap = NULL;       /* MAX_APARATOS, en la PSRAM si la hay */

/* ── El estado de la radio ─────────────────────────────────────────────────────────────────────────────────────────── */

static const bt_esp32_enlace_t* s_enlace = NULL;   /* el del controlador, si no está en el chip (la P4: su C6) */
static bpvm_bt_aviso_fn s_aviso = NULL;
static bpvm_bt_visto_fn s_visto = NULL;
static volatile int s_encendida = 0;  /* NimBLE arrancado (de encender a apagar) */
static volatile int s_lista = 0;      /* sincronizada: ya se puede pedir */
static uint8_t s_propia = 0;          /* el tipo de nuestra dirección */
static int s_espera_activa = 0;       /* la espera de los recordados (la lista blanca), en marcha */
static int s_espera_n = 0;            /* a cuántos espera la que está en marcha: si cambian, se rehace */
static int s_espera_fallos = 0;       /* veces seguidas que la espera acabó en error al momento */
static int s_buscar_ms = 0;           /* una búsqueda pedida, que espera a que el controlador quede libre */
static int s_buscando = 0;            /* una búsqueda en marcha */

/* El connect del programa: uno a la vez (sólo lo pide la tarea del Bluetooth de lo común). */
typedef struct {
    struct ble_npl_event ev;
    SemaphoreHandle_t    listo;
    unsigned             id;
    char                 dir[BPVM_BT_DIR], tipo[BPVM_BT_TIPO];
    int                  ms;
    int                  r;
    char                 clase[BPVM_BT_CLASE];
    char                 err[96];
} pedido_t;
static pedido_t s_pedido;
static unsigned s_pedido_id = 0;
static aparato_t* s_pedido_ap = NULL;   /* el aparato al que espera el pedido en curso */

/* Los encargos de otros hilos que no esperan: buscar, desconectar, olvidar. */
enum { O_BUSCAR = 1, O_DESCONECTAR = 2, O_OLVIDAR = 3 };
typedef struct {
    struct ble_npl_event ev;
    int  libre;
    int  que, ms;
    char dir[BPVM_BT_DIR];
} orden_t;
static orden_t s_ordenes[ORDENES];
static SemaphoreHandle_t s_ordenes_m = NULL;
static SemaphoreHandle_t s_orden_hecha = NULL;   /* olvidar: hecho en la tarea de NimBLE (para escribir el fichero) */

static void arbitro(void);
static int al_gap(struct ble_gap_event* ev, void* arg);

/* ── Direcciones ───────────────────────────────────────────────────────────────────────────────────────────────────── */

static void dir_texto(const ble_addr_t* a, char* s, size_t n) {
    snprintf(s, n, "%02X:%02X:%02X:%02X:%02X:%02X", a->val[5], a->val[4], a->val[3], a->val[2], a->val[1], a->val[0]);
}

static int hexa(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* "AA:BB:CC:DD:EE:FF" y "public"/"random" a un ble_addr_t. 0, o -1 si no es una dirección. */
static int dir_de_texto(const char* s, const char* tipo, ble_addr_t* a) {
    int i;
    if (!s || strlen(s) != 17) return -1;
    for (i = 0; i < 6; i++) {
        int h = hexa(s[i * 3]), l = hexa(s[i * 3 + 1]);
        if (h < 0 || l < 0 || (i < 5 && s[i * 3 + 2] != ':')) return -1;
        a->val[5 - i] = (uint8_t) (h * 16 + l);
    }
    a->type = (tipo && strcmp(tipo, "random") == 0) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
    return 0;
}

/* Las direcciones se comparan sin el tipo «de identidad» (PUBLIC_ID es la pública que se resolvió). */
static int misma_dir(const ble_addr_t* a, const ble_addr_t* b) {
    return memcmp(a->val, b->val, 6) == 0;
}

/* ── Las claves: /sys/bluetooth.keys ───────────────────────────────────────────────────────────────────────────────── */

/* El fichero: "BPBK", la versión (1), cuántos registros de claves y cuántos alias, y un byte libre; cada registro de
 * claves, 68 B (de quién: 1 nuestras, 2 suyas; la dirección; lo que NimBLE guarda de un vínculo); cada alias, 25 B (la
 * dirección del programa, en texto, y la del vínculo). Nuestro formato, no la estructura de NimBLE tal cual: así no
 * depende de su versión. */
#define CLAVES_MAX_REG   (2 * MAX_APARATOS)
#define REG_CLAVES       68
#define REG_ALIAS        25

typedef struct { char dir[BPVM_BT_DIR]; ble_addr_t addr; } alias_t;
static alias_t s_alias[MAX_APARATOS];
static int s_nalias = 0;
static int s_cargando = 0;   /* cargando el fichero: lo que se escribe en el almacén no se vuelve a guardar */

static uint8_t* pon_sec(uint8_t* p, int de, const struct ble_store_value_sec* v) {
    *p++ = (uint8_t) de;
    *p++ = v->peer_addr.type;
    memcpy(p, v->peer_addr.val, 6); p += 6;
    *p++ = v->key_size;
    *p++ = (uint8_t) ((v->ltk_present ? 1 : 0) | (v->irk_present ? 2 : 0) | (v->csrk_present ? 4 : 0) |
                      (v->authenticated ? 8 : 0) | (v->sc ? 16 : 0));
    *p++ = (uint8_t) (v->ediv & 0xFF); *p++ = (uint8_t) (v->ediv >> 8);
    memcpy(p, &v->rand_num, 8); p += 8;
    memcpy(p, v->ltk, 16); p += 16;
    memcpy(p, v->irk, 16); p += 16;
    memcpy(p, v->csrk, 16); p += 16;
    return p;
}

static const uint8_t* lee_sec(const uint8_t* p, int* de, struct ble_store_value_sec* v) {
    uint8_t f;
    memset(v, 0, sizeof *v);
    *de = *p++;
    v->peer_addr.type = *p++;
    memcpy(v->peer_addr.val, p, 6); p += 6;
    v->key_size = *p++;
    f = *p++;
    v->ltk_present = (f & 1) != 0; v->irk_present = (f & 2) != 0; v->csrk_present = (f & 4) != 0;
    v->authenticated = (f & 8) != 0; v->sc = (f & 16) != 0;
    v->ediv = (uint16_t) (p[0] | (p[1] << 8)); p += 2;
    memcpy(&v->rand_num, p, 8); p += 8;
    memcpy(v->ltk, p, 16); p += 16;
    memcpy(v->irk, p, 16); p += 16;
    memcpy(v->csrk, p, 16); p += 16;
    return p;
}

typedef struct { uint8_t* p; uint8_t* fin; int n; } volcado_t;

static int vuelca_uno(int obj_type, union ble_store_value* val, void* arg) {
    volcado_t* v = (volcado_t*) arg;
    if (v->p + REG_CLAVES > v->fin) return 1;   /* no cabe: se para (no pasa: CLAVES_MAX_REG) */
    v->p = pon_sec(v->p, obj_type == BLE_STORE_OBJ_TYPE_OUR_SEC ? 1 : 2, &val->sec);
    v->n++;
    return 0;
}

#define CLAVES_CAP  (8 + CLAVES_MAX_REG * REG_CLAVES + MAX_APARATOS * REG_ALIAS)

static SemaphoreHandle_t s_claves_m = NULL;   /* el búfer de los bytes del fichero: lo llena una tarea, lo lee otra */
static uint8_t* s_claves_buf = NULL;          /* CLAVES_CAP, en la PSRAM si la hay */
static size_t s_claves_len = 0;
static volatile int s_claves_sucias = 0;      /* hay bytes nuevos sin escribir */

/* EN LA TAREA DE NIMBLE: los bytes del fichero entero (los vínculos del almacén y los alias). Escribirlos no va aquí
 * (vuelca_claves). */
static void prepara_claves(void) {
    volcado_t v;
    int i, nsec;
    if (!s_claves_m || !s_claves_buf) return;
    xSemaphoreTake(s_claves_m, portMAX_DELAY);
    v.p = s_claves_buf + 8; v.fin = s_claves_buf + 8 + CLAVES_MAX_REG * REG_CLAVES; v.n = 0;
    ble_store_iterate(BLE_STORE_OBJ_TYPE_OUR_SEC, vuelca_uno, &v);
    ble_store_iterate(BLE_STORE_OBJ_TYPE_PEER_SEC, vuelca_uno, &v);
    nsec = v.n;
    for (i = 0; i < s_nalias; i++) {
        memset(v.p, 0, REG_ALIAS);
        memcpy(v.p, s_alias[i].dir, strlen(s_alias[i].dir));
        v.p[18] = s_alias[i].addr.type;
        memcpy(v.p + 19, s_alias[i].addr.val, 6);
        v.p += REG_ALIAS;
    }
    memcpy(s_claves_buf, "BPBK", 4);
    s_claves_buf[4] = 1;
    s_claves_buf[5] = (uint8_t) nsec;
    s_claves_buf[6] = (uint8_t) s_nalias;
    s_claves_buf[7] = 0;
    s_claves_len = (size_t) (v.p - s_claves_buf);
    s_claves_sucias = 1;
    xSemaphoreGive(s_claves_m);
}

/* FUERA DE LA TAREA DE NIMBLE (la del Bluetooth de lo común, o el hilo del programa): si hay bytes nuevos, al fichero. */
static void vuelca_claves(void) {
    uint8_t* copia;
    size_t n;
    if (!s_claves_m || !s_claves_buf || !s_claves_sucias) return;
    copia = (uint8_t*) malloc(CLAVES_CAP);
    if (!copia) return;
    xSemaphoreTake(s_claves_m, portMAX_DELAY);
    n = s_claves_len;
    memcpy(copia, s_claves_buf, n);
    s_claves_sucias = 0;
    xSemaphoreGive(s_claves_m);
    if (!bpvm_fs_exists("/sys")) (void) bpvm_fs_mkdir("/sys");
    if (bpvm_fs_write(CLAVES_FICHERO, copia, (uint32_t) n, 0) != 0) {
        log_printf("bt: no se pudieron guardar las claves en " CLAVES_FICHERO);
        s_claves_sucias = 1;   /* se intentará la próxima vez */
    }
    free(copia);
}

/* AL ENCENDER, en el hilo de quien enciende (con pila de sobra): el fichero, al búfer. Un fichero roto se dice y no se
 * usa. Lo carga en el almacén carga_claves, al sincronizar. */
static void lee_fichero_claves(void) {
    long n;
    s_claves_len = 0;
    if (!s_claves_buf || !bpvm_fs_exists(CLAVES_FICHERO)) return;
    n = bpvm_fs_read(CLAVES_FICHERO, s_claves_buf, (uint32_t) CLAVES_CAP);
    if (n < 8 || memcmp(s_claves_buf, "BPBK", 4) != 0 || s_claves_buf[4] != 1 ||
        s_claves_buf[5] > CLAVES_MAX_REG || s_claves_buf[6] > MAX_APARATOS ||
        n < 8 + s_claves_buf[5] * REG_CLAVES + s_claves_buf[6] * REG_ALIAS) {
        log_printf("bt: " CLAVES_FICHERO " no se entiende o está cortado: no se usa");
        return;
    }
    s_claves_len = (size_t) n;
}

/* AL SINCRONIZAR, en la tarea de NimBLE: los bytes del búfer, al almacén de la RAM, y los alias, a su lista. Tiene que
 * ser aquí y no al encender: al arrancar la pila, su parte de privacidad vuelve a iniciar el almacén (ble_hs_pvcy.c,
 * con BLE_STATIC_TO_DYNAMIC: llama a ble_store_config_init), que lo vacía y vuelve a poner sus funciones. Medido el
 * 11-nov: emparejado el teclado, el fichero no se escribía. Sin tocar el sistema de ficheros: sólo memoria. */
static void carga_claves(void) {
    int i, nsec, nal;
    const uint8_t* p;
    s_nalias = 0;
    if (!s_claves_m || !s_claves_buf) return;
    xSemaphoreTake(s_claves_m, portMAX_DELAY);
    if (s_claves_len >= 8) {
        nsec = s_claves_buf[5];
        nal = s_claves_buf[6];
        p = s_claves_buf + 8;
        s_cargando = 1;
        for (i = 0; i < nsec; i++) {
            union ble_store_value v;
            int de;
            p = lee_sec(p, &de, &v.sec);
            (void) ble_store_config_write(de == 1 ? BLE_STORE_OBJ_TYPE_OUR_SEC : BLE_STORE_OBJ_TYPE_PEER_SEC, &v);
        }
        for (i = 0; i < nal; i++) {
            memcpy(s_alias[s_nalias].dir, p, BPVM_BT_DIR - 1);
            s_alias[s_nalias].dir[BPVM_BT_DIR - 1] = '\0';
            s_alias[s_nalias].addr.type = p[18];
            memcpy(s_alias[s_nalias].addr.val, p + 19, 6);
            s_nalias++;
            p += REG_ALIAS;
        }
        s_cargando = 0;
    }
    xSemaphoreGive(s_claves_m);
}

static int al_escribir_clave(int obj_type, const union ble_store_value* val) {
    int r = ble_store_config_write(obj_type, val);
    if (r == 0 && !s_cargando && (obj_type == BLE_STORE_OBJ_TYPE_OUR_SEC || obj_type == BLE_STORE_OBJ_TYPE_PEER_SEC))
        prepara_claves();
    return r;
}

static int al_borrar_clave(int obj_type, const union ble_store_key* key) {
    int r = ble_store_config_delete(obj_type, key);
    if (r == 0 && (obj_type == BLE_STORE_OBJ_TYPE_OUR_SEC || obj_type == BLE_STORE_OBJ_TYPE_PEER_SEC)) prepara_claves();
    return r;
}

static const alias_t* alias_de(const char* dir) {
    int i;
    for (i = 0; i < s_nalias; i++) if (strcmp(s_alias[i].dir, dir) == 0) return &s_alias[i];
    return NULL;
}

static void quita_alias(const char* dir) {
    int i;
    for (i = 0; i < s_nalias; i++)
        if (strcmp(s_alias[i].dir, dir) == 0) { s_alias[i] = s_alias[--s_nalias]; return; }
}

static void pon_alias(const char* dir, const ble_addr_t* addr) {
    quita_alias(dir);
    if (s_nalias >= MAX_APARATOS) return;
    snprintf(s_alias[s_nalias].dir, sizeof s_alias[s_nalias].dir, "%s", dir);
    s_alias[s_nalias].addr = *addr;
    s_nalias++;
}

/* ── La tabla de aparatos ──────────────────────────────────────────────────────────────────────────────────────────── */

static aparato_t* por_dir(const char* dir) {
    int i;
    if (!s_ap) return NULL;
    for (i = 0; i < MAX_APARATOS; i++) if (s_ap[i].en_uso && strcmp(s_ap[i].dir, dir) == 0) return &s_ap[i];
    return NULL;
}

static aparato_t* por_conn(uint16_t conn) {
    int i;
    if (!s_ap || conn == BLE_HS_CONN_HANDLE_NONE) return NULL;
    for (i = 0; i < MAX_APARATOS; i++)
        if (s_ap[i].en_uso && s_ap[i].estado >= E_PREPARANDO && s_ap[i].conn == conn) return &s_ap[i];
    return NULL;
}

/* El que se conectó por la lista blanca: por la dirección de la conexión o la de su identidad. */
static aparato_t* por_conexion(const struct ble_gap_conn_desc* d) {
    int i;
    if (!s_ap) return NULL;
    for (i = 0; i < MAX_APARATOS; i++) {
        aparato_t* a = &s_ap[i];
        if (a->en_uso && a->esperado && a->estado == E_NADA &&
            (misma_dir(&a->addr, &d->peer_id_addr) || misma_dir(&a->addr, &d->peer_ota_addr))) return a;
    }
    return NULL;
}

static aparato_t* nuevo(const char* dir, const char* tipo) {
    int i;
    ble_addr_t addr;
    const alias_t* al;
    if (!s_ap || dir_de_texto(dir, tipo, &addr) != 0) return NULL;
    al = alias_de(dir);
    if (al) addr = al->addr;
    for (i = 0; i < MAX_APARATOS; i++) {
        aparato_t* a = &s_ap[i];
        if (a->en_uso) continue;
        memset(a, 0, sizeof *a);
        a->en_uso = 1;
        snprintf(a->dir, sizeof a->dir, "%s", dir);
        a->addr = addr;
        a->conn = BLE_HS_CONN_HANDLE_NONE;
        return a;
    }
    return NULL;
}

static void suelta_mapa(aparato_t* a) {
    if (a->desc) { heap_caps_free(a->desc); a->desc = NULL; }
    a->ndesc = 0;
}

/* Vuelve a «sin conectar» (lo que se sabía de la conexión, fuera). */
static void sin_conexion(aparato_t* a) {
    a->estado = E_NADA;
    a->preparando = 0;
    a->conn = BLE_HS_CONN_HANDLE_NONE;
    a->svc_ini = a->svc_fin = 0;
    a->nchr = 0;
    a->mapa = 0;
    suelta_mapa(a);
}

/* ── El pedido del programa: la respuesta ──────────────────────────────────────────────────────────────────────────── */

static void responde(int r, const char* clase, const char* err) {
    s_pedido.r = r;
    snprintf(s_pedido.clase, sizeof s_pedido.clase, "%s", clase ? clase : "");
    snprintf(s_pedido.err, sizeof s_pedido.err, "%s", err ? err : "");
    if (s_pedido_ap) s_pedido_ap->pedido = 0;
    s_pedido_ap = NULL;
    xSemaphoreGive(s_pedido.listo);
}

/* El aparato `a` no ha llegado a estar listo: se dice a quien le esperaba, si alguien. */
static void falla(aparato_t* a, const char* motivo) {
    if (a->pedido && s_pedido_ap == a) responde(-1, "", motivo);
}

/* ── Las luces de los bloqueos ─────────────────────────────────────────────────────────────────────────────────────── */

/* El informe de salida con las luces de un teclado: la característica de informe (0x2A4D) de salida (tipo 2) con el
 * número que declara su descriptor, y que se pueda escribir. */
static void busca_luces(aparato_t* a) {
    int i;
    a->luces_h = 0;
    a->luces_sin_resp = 0;
    if (!a->hid.luces) return;
    for (i = 0; i < a->nchr; i++) {
        const chr_t* c = &a->chr[i];
        if (c->uuid == 0x2A4D && c->tipo_informe == 2 && c->id_informe == a->hid.luces_id &&
            (c->props & (BLE_GATT_CHR_PROP_WRITE | BLE_GATT_CHR_PROP_WRITE_NO_RSP))) {
            a->luces_h = c->val;
            a->luces_sin_resp = (c->props & BLE_GATT_CHR_PROP_WRITE_NO_RSP) != 0;
            return;
        }
    }
}

static int al_escribir_luces(uint16_t conn, const struct ble_gatt_error* err, struct ble_gatt_attr* attr, void* arg) {
    (void) conn; (void) err; (void) attr; (void) arg;
    return 0;
}

/* Las luces de los bloqueos de ahora, a un teclado listo. */
static void escribe_luces(aparato_t* a) {
    uint8_t v[8];
    int n;
    if (a->estado != E_LISTO || a->luces_h == 0) return;
    n = bpvm_hid_luces(&a->hid, v, (int) sizeof v);
    if (n <= 0) return;
    if (a->luces_sin_resp) (void) ble_gattc_write_no_rsp_flat(a->conn, a->luces_h, v, (uint16_t) n);
    else (void) ble_gattc_write_flat(a->conn, a->luces_h, v, (uint16_t) n, al_escribir_luces, NULL);
}

/* Han cambiado los bloqueos (una tecla, en un teclado): las luces, a todos, que son del SO y no de cada teclado. */
static void luces_a_todos(void) {
    int i;
    for (i = 0; i < MAX_APARATOS; i++) if (s_ap[i].en_uso) escribe_luces(&s_ap[i]);
}

/* ── Preparar un aparato conectado: el servicio HID, sus informes y su descriptor ──────────────────────────────────── */

static void listo(aparato_t* a) {
    char que[112];
    a->estado = E_LISTO;
    /* El descriptor: lo que es (la clase) y qué es cada informe. Leído, se suelta: lo que hace falta queda en a->hid. */
    bpvm_hid_lee_mapa(a->desc, a->desc ? a->ndesc : 0, &a->hid);
    suelta_mapa(a);
    memset(&a->teclado, 0, sizeof a->teclado);
    busca_luces(a);
    snprintf(a->clase, sizeof a->clase, "%s", bpvm_hid_clase(&a->hid));
    bpvm_hid_describe(&a->hid, que, sizeof que);
    /* La marca de la pila de NimBLE, que se queda en sus 4 KB (el fichero de las claves se escribe fuera de ella). */
    log_printf("bt: %s listo (%s: %s; de la pila de NimBLE quedan %u B)", a->dir, a->clase, que,
               (unsigned) uxTaskGetStackHighWaterMark(NULL));
    escribe_luces(a);   /* un teclado: las luces de los bloqueos de ahora (al arrancar, la de Bloq Num) */
    if (a->pedido && s_pedido_ap == a) responde(0, a->clase, "");
    else if (s_aviso) s_aviso(BPVM_BT_AV_VOLVIO, a->dir, a->clase);
    arbitro();
}

static int al_mapa(uint16_t conn, const struct ble_gatt_error* err, struct ble_gatt_attr* attr, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status == 0 && attr) {
        int n = OS_MBUF_PKTLEN(attr->om);
        if (a->desc && a->ndesc + n > MAPA_MAX) n = MAPA_MAX - a->ndesc;
        if (a->desc && n > 0) { os_mbuf_copydata(attr->om, 0, n, a->desc + a->ndesc); a->ndesc += n; }
        return 0;
    }
    if (err->status != BLE_HS_EDONE) log_printf("bt: %s, el descriptor de informes: error %d", a->dir, err->status);
    listo(a);
    return 0;
}

static void lee_mapa(aparato_t* a) {
    if (a->mapa == 0) { listo(a); return; }
    suelta_mapa(a);
    a->desc = (uint8_t*) heap_caps_malloc(MAPA_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!a->desc) a->desc = (uint8_t*) malloc(MAPA_MAX);
    if (!a->desc || ble_gattc_read_long(a->conn, a->mapa, 0, al_mapa, a) != 0) listo(a);
}

static void siguiente_informe(aparato_t* a);

static int al_suscrito(uint16_t conn, const struct ble_gatt_error* err, struct ble_gatt_attr* attr, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    (void) attr;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status != 0) log_printf("bt: %s, suscribirse a h=%u: error %d", a->dir, a->chr[a->paso].val, err->status);
    a->paso++;
    siguiente_informe(a);
    return 0;
}

/* Con la referencia ya leída: si es de entrada (o no la tiene), suscribirse. */
static void suscribe(aparato_t* a) {
    chr_t* c = &a->chr[a->paso];
    if (a->cccd && (c->tipo_informe == 0 || c->tipo_informe == 1)) {
        uint8_t v[2] = { (uint8_t) ((c->props & BLE_GATT_CHR_PROP_NOTIFY) ? 0x01 : 0x02), 0x00 };
        if (ble_gattc_write_flat(a->conn, a->cccd, v, sizeof v, al_suscrito, a) == 0) return;
    }
    a->paso++;
    siguiente_informe(a);
}

static int al_referencia(uint16_t conn, const struct ble_gatt_error* err, struct ble_gatt_attr* attr, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status == 0 && attr && OS_MBUF_PKTLEN(attr->om) >= 2) {
        uint8_t r[2];
        os_mbuf_copydata(attr->om, 0, 2, r);
        a->chr[a->paso].id_informe = r[0];
        a->chr[a->paso].tipo_informe = r[1];
    }
    suscribe(a);                              /* una lectura corta avisa una sola vez */
    return 0;
}

static int al_descriptor(uint16_t conn, const struct ble_gatt_error* err, uint16_t chr_val,
                         const struct ble_gatt_dsc* dsc, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    (void) chr_val;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status == 0 && dsc) {
        uint16_t u = ble_uuid_u16(&dsc->uuid.u);
        if (u == 0x2902) a->cccd = dsc->handle;
        else if (u == 0x2908) a->ref = dsc->handle;
        return 0;
    }
    if (a->ref && ble_gattc_read(a->conn, a->ref, al_referencia, a) == 0) return 0;
    suscribe(a);
    return 0;
}

/* El siguiente informe (0x2A4D): de entrada (con notificación), sus descriptores, su referencia y la suscripción; de
 * salida (se escribe: las luces de un teclado), su referencia. Al acabar, el descriptor de informes. */
static void siguiente_informe(aparato_t* a) {
    while (a->paso < a->nchr && !(a->chr[a->paso].uuid == 0x2A4D &&
                                  (a->chr[a->paso].props & (BLE_GATT_CHR_PROP_NOTIFY | BLE_GATT_CHR_PROP_INDICATE |
                                                            BLE_GATT_CHR_PROP_WRITE | BLE_GATT_CHR_PROP_WRITE_NO_RSP))))
        a->paso++;
    if (a->paso >= a->nchr) { lee_mapa(a); return; }
    {
        chr_t* c = &a->chr[a->paso];
        uint16_t fin = (a->paso + 1 < a->nchr) ? (uint16_t) (a->chr[a->paso + 1].def - 1) : a->svc_fin;
        a->cccd = 0;
        a->ref = 0;
        if (ble_gattc_disc_all_dscs(a->conn, c->val, fin, al_descriptor, a) != 0) { a->paso++; siguiente_informe(a); }
    }
}

static int al_caracteristica(uint16_t conn, const struct ble_gatt_error* err, const struct ble_gatt_chr* chr, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status == 0 && chr) {
        if (a->nchr < MAX_CHR) {
            chr_t* c = &a->chr[a->nchr++];
            memset(c, 0, sizeof *c);
            c->uuid = ble_uuid_u16(&chr->uuid.u);
            c->def = chr->def_handle;
            c->val = chr->val_handle;
            c->props = chr->properties;
            if (c->uuid == 0x2A4B) a->mapa = c->val;
        }
        return 0;
    }
    a->paso = 0;
    siguiente_informe(a);
    return 0;
}

static int al_servicio(uint16_t conn, const struct ble_gatt_error* err, const struct ble_gatt_svc* svc, void* arg) {
    aparato_t* a = (aparato_t*) arg;
    if (!a->en_uso || a->conn != conn) return 0;
    if (err->status == 0 && svc) { a->svc_ini = svc->start_handle; a->svc_fin = svc->end_handle; return 0; }
    if (a->svc_ini == 0) {
        log_printf("bt: %s no tiene el servicio HID: se suelta", a->dir);
        falla(a, "Bluetooth: no es un teclado ni un ratón (no tiene el servicio HID)");
        ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }
    a->nchr = 0;
    a->mapa = 0;
    if (ble_gattc_disc_all_chrs(conn, a->svc_ini, a->svc_fin, al_caracteristica, a) != 0) {
        falla(a, "Bluetooth: no se pudo leer el servicio HID");
        ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
    }
    return 0;
}

/* ── Los sucesos de una conexión ───────────────────────────────────────────────────────────────────────────────────── */

static uint16_t conn_de(const struct ble_gap_event* ev) {
    switch (ev->type) {
    case BLE_GAP_EVENT_DISCONNECT:       return ev->disconnect.conn.conn_handle;
    case BLE_GAP_EVENT_ENC_CHANGE:       return ev->enc_change.conn_handle;
    case BLE_GAP_EVENT_PASSKEY_ACTION:   return ev->passkey.conn_handle;
    case BLE_GAP_EVENT_REPEAT_PAIRING:   return ev->repeat_pairing.conn_handle;
    case BLE_GAP_EVENT_NOTIFY_RX:        return ev->notify_rx.conn_handle;
    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:  return ev->conn_update_req.conn_handle;
    default:                             return BLE_HS_CONN_HANDLE_NONE;
    }
}

static int al_conectar(struct ble_gap_event* ev, aparato_t* directo) {
    struct ble_gap_conn_desc d;
    aparato_t* a = directo;
    if (!directo) { s_espera_activa = 0; s_espera_n = 0; }
    if (ev->connect.status != 0) {
        if (directo) {
            /* BLE_HS_EAPP: la cancelamos nosotros (llega otra cosa); si no, no contestó en su plazo. */
            sin_conexion(directo);
            if (ev->connect.status != BLE_HS_EAPP) falla(directo, "Bluetooth: el aparato no contesta (¿está encendido?)");
        } else if (ev->connect.status != BLE_HS_EAPP) {
            /* La espera acabó sola, con un error: se rehace, pero a la tercera seguida se deja (no dar vueltas). */
            s_espera_fallos++;
            log_printf("bt: la espera de los recordados acaba con error (%d)%s", ev->connect.status,
                       s_espera_fallos >= 3 ? ": se deja" : "");
        }
        arbitro();
        return 0;
    }
    s_espera_fallos = 0;
    if (ble_gap_conn_find(ev->connect.conn_handle, &d) != 0) return 0;
    if (!a) a = por_conexion(&d);
    if (!a) {
        log_printf("bt: conecta un aparato que no se espera: se suelta");
        ble_gap_terminate(ev->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        arbitro();
        return 0;
    }
    a->conn = ev->connect.conn_handle;
    a->estado = E_PREPARANDO;
    log_printf("bt: %s conectado (%s); se pide el cifrado", a->dir, directo ? "lo pidió un programa" : "volvió solo");
    if (ble_gap_security_initiate(a->conn) != 0) {
        falla(a, "Bluetooth: no se pudo pedir el cifrado");
        ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
    }
    arbitro();                    /* lo que estuviera pendiente (una búsqueda) no espera a que éste esté listo */
    return 0;
}

static int al_gap(struct ble_gap_event* ev, void* arg) {
    aparato_t* a;
    struct ble_gap_conn_desc d;
    if (ev->type == BLE_GAP_EVENT_CONNECT) return al_conectar(ev, (aparato_t*) arg);
    a = por_conn(conn_de(ev));
    if (!a) return 0;
    switch (ev->type) {
    case BLE_GAP_EVENT_DISCONNECT: {
        int estaba = a->estado;
        log_printf("bt: %s desconectado (motivo 0x%X)", a->dir, ev->disconnect.reason);
        sin_conexion(a);
        if (estaba == E_LISTO) { if (s_aviso) s_aviso(BPVM_BT_AV_SE_FUE, a->dir, NULL); }
        else falla(a, "Bluetooth: el aparato cortó la conexión mientras se preparaba");
        if (!a->esperado && !a->pedido) a->en_uso = 0;   /* ni recordado ni pedido: fuera de la tabla */
        arbitro();
        return 0;
    }
    case BLE_GAP_EVENT_ENC_CHANGE:
        if (ev->enc_change.status != 0) {
            log_printf("bt: %s, el cifrado falla (%d)", a->dir, ev->enc_change.status);
            falla(a, "Bluetooth: el emparejamiento falla");
            ble_gap_terminate(a->conn, BLE_ERR_AUTH_FAIL);
            return 0;
        }
        if (ble_gap_conn_find(a->conn, &d) == 0 && d.sec_state.bonded && !misma_dir(&a->addr, &d.peer_id_addr)) {
            /* Se vinculó con otra dirección que la que conoce el programa: la radio la traduce desde ahora. */
            a->addr = d.peer_id_addr;
            pon_alias(a->dir, &d.peer_id_addr);
            prepara_claves();
        }
        /* El aviso puede llegar dos veces (11-nov, el teclado al volver a emparejarlo): la preparación, una vez. */
        if (a->preparando) return 0;
        log_printf("bt: %s cifrado (vinculado %d)", a->dir,
                   ble_gap_conn_find(a->conn, &d) == 0 ? (int) d.sec_state.bonded : -1);
        a->preparando = 1;
        a->svc_ini = 0;
        {
            int rc = ble_gattc_disc_svc_by_uuid(a->conn, BLE_UUID16_DECLARE(UUID_HID), al_servicio, a);
            if (rc != 0) {
                log_printf("bt: %s, buscar el servicio HID no arranca (%d)", a->dir, rc);
                falla(a, "Bluetooth: no se pudo buscar el servicio HID");
                ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
            }
        }
        return 0;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* Ya estaba vinculado con otra clave (lo olvidó él): se borra el vínculo de aquí y se repite. */
        if (ble_gap_conn_find(ev->repeat_pairing.conn_handle, &d) == 0) ble_store_util_delete_peer(&d.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    case BLE_GAP_EVENT_PASSKEY_ACTION: {
        struct ble_sm_io io;
        memset(&io, 0, sizeof io);
        io.action = ev->passkey.params.action;
        if (io.action == BLE_SM_IOACT_NUMCMP) io.numcmp_accept = 1;
        (void) ble_sm_inject_io(ev->passkey.conn_handle, &io);
        return 0;
    }
    case BLE_GAP_EVENT_NOTIFY_RX:
        if (a->estado == E_LISTO) {
            /* Un informe: a bpvm_hid.c, con su número (el de la referencia de su característica). */
            uint8_t v[64];
            int i, n = (int) OS_MBUF_PKTLEN(ev->notify_rx.om);
            for (i = 0; i < a->nchr; i++) if (a->chr[i].val == ev->notify_rx.attr_handle) break;
            if (i < a->nchr && n > 0) {
                if (n > (int) sizeof v) n = (int) sizeof v;
                if (os_mbuf_copydata(ev->notify_rx.om, 0, n, v) == 0 &&
                    bpvm_hid_informe(&a->hid, a->chr[i].id_informe, v, n, &a->teclado))
                    luces_a_todos();   /* han cambiado los bloqueos */
                bpvm_bt_actividad();   /* la pieza 5: una tecla o el ratón cuentan como uso (no la batería: no es HID) */
            }
        }
        return 0;
    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        return BLE_ERR_CONN_PARMS;   /* los de la placa: 7,5–15 ms, sin latencia */
    default:
        return 0;
    }
}

/* ── El árbitro ────────────────────────────────────────────────────────────────────────────────────────────────────── */

static int al_descubrir(struct ble_gap_event* ev, void* arg);

/* Los recordados que no están: la lista de la espera. */
static int esperados(ble_addr_t* lb, int* hay_prep) {
    int i, n = 0;
    *hay_prep = 0;
    for (i = 0; i < MAX_APARATOS; i++) {
        aparato_t* a = &s_ap[i];
        if (!a->en_uso) continue;
        if (a->estado == E_CONECTANDO || a->estado == E_PREPARANDO) *hay_prep = 1;
        else if (a->estado == E_NADA && a->esperado) { if (lb) lb[n] = a->addr; n++; }
    }
    return n;
}

static void arbitro(void) {
    int hay_prep = 0, nlb = 0;
    ble_addr_t lb[MAX_APARATOS];
    if (!s_encendida || !s_lista || !s_ap) return;
    if (s_buscando) return;
    /* Algo que hacer que no es esperar, o la lista de la espera ha cambiado: se cancela, y su suceso vuelve aquí. */
    if (s_espera_activa && (s_buscar_ms > 0 || (s_pedido_ap && s_pedido_ap->estado == E_NADA) ||
                            esperados(NULL, &hay_prep) != s_espera_n)) {
        ble_gap_conn_cancel();
        return;
    }
    if (ble_gap_conn_active()) return;            /* un connect del programa en marcha */
    if (s_buscar_ms > 0) {
        struct ble_gap_disc_params p;
        memset(&p, 0, sizeof p);
        p.passive = 0;                            /* activo: el nombre suele venir en la respuesta al escaneo */
        p.filter_duplicates = 1;
        p.itvl = 0x50;                            /* 50 ms, y escucha 30 de cada 50 (lo del 14-oct) */
        p.window = 0x30;
        {
            int rc = ble_gap_disc(s_propia, s_buscar_ms, &p, al_descubrir, NULL);
            if (rc == 0) { s_buscando = 1; log_printf("bt: busca %d ms", s_buscar_ms); }
            else log_printf("bt: la búsqueda no arranca (%d; conectando %d, buscando %d, esperando %d)", rc,
                            ble_gap_conn_active(), ble_gap_disc_active(), s_espera_activa);
        }
        s_buscar_ms = 0;
        return;
    }
    if (s_pedido_ap && s_pedido_ap->estado == E_NADA) {
        aparato_t* a = s_pedido_ap;
        int rc = ble_gap_connect(s_propia, &a->addr, s_pedido.ms, &CONEXION, al_gap, a);
        if (rc == 0) a->estado = E_CONECTANDO;
        else {
            log_printf("bt: %s, la radio no conecta (%d; conectando %d, buscando %d)", a->dir, rc,
                       ble_gap_conn_active(), ble_gap_disc_active());
            responde(-1, "", "Bluetooth: la radio no puede conectar ahora");
        }
        return;
    }
    /* La espera de los recordados: sólo sin nadie preparándose (como el 14-oct). */
    nlb = esperados(lb, &hay_prep);
    if (hay_prep || nlb == 0 || s_espera_fallos >= 3) return;
    {
        int rc = ble_gap_wl_set(lb, (uint8_t) nlb);
        if (rc == 0) rc = ble_gap_connect(s_propia, NULL, BLE_HS_FOREVER, &CONEXION, al_gap, NULL);
        if (rc == 0) {
            if (nlb != s_espera_n) log_printf("bt: esperando a %d recordado(s) por su dirección", nlb);
            s_espera_activa = 1;
            s_espera_n = nlb;
        } else {
            log_printf("bt: la espera de los recordados no arranca (%d)", rc);
            s_espera_fallos++;
        }
    }
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
    if (ev->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        s_buscando = 0;
        arbitro();
        return 0;
    }
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

/* ── Los encargos de otros hilos, en la tarea de NimBLE ────────────────────────────────────────────────────────────── */

static void al_pedido(struct ble_npl_event* ev) {
    pedido_t* p = (pedido_t*) ble_npl_event_get_arg(ev);
    aparato_t* a;
    if (p->id != s_pedido_id) return;             /* uno viejo, de antes de apagar */
    s_espera_fallos = 0;                          /* algo nuevo: la espera de los recordados vuelve a intentarse */
    a = por_dir(p->dir);
    if (!a) a = nuevo(p->dir, p->tipo);
    if (!a) { responde(-1, "", "Bluetooth: no caben más aparatos, o la dirección no vale"); return; }
    a->esperado = 1;                              /* recordado: desde ahora se le espera */
    if (a->estado == E_LISTO) { responde(0, a->clase, ""); return; }
    if (p->ms == 0) {                             /* el pase al encender: no se espera; lo hará la lista blanca */
        responde(-1, "", "no está ahora: se le espera");
        arbitro();
        return;
    }
    if (a->estado != E_NADA) {                    /* ya se está conectando (por la lista blanca): se espera a eso */
        a->pedido = 1;
        s_pedido_ap = a;
        return;
    }
    a->pedido = 1;
    s_pedido_ap = a;
    arbitro();
}

static orden_t* orden_libre(void) {
    int i;
    orden_t* o = NULL;
    if (!s_ordenes_m) return NULL;
    xSemaphoreTake(s_ordenes_m, portMAX_DELAY);
    for (i = 0; i < ORDENES; i++) if (s_ordenes[i].libre) { o = &s_ordenes[i]; o->libre = 0; break; }
    xSemaphoreGive(s_ordenes_m);
    return o;
}

static void al_orden(struct ble_npl_event* ev) {
    orden_t* o = (orden_t*) ble_npl_event_get_arg(ev);
    aparato_t* a = por_dir(o->dir);
    switch (o->que) {
    case O_BUSCAR:
        s_buscar_ms = o->ms > 0 ? o->ms : 1;
        s_nvistos = 0;
        if (s_buscando) {                         /* una búsqueda nueva sustituye a la de antes */
            ble_gap_disc_cancel();
            s_buscando = 0;
        }
        arbitro();
        break;
    case O_DESCONECTAR:
        if (a) {
            a->esperado = 0;                      /* no vuelve solo hasta otro connect o el siguiente arranque */
            if (a->estado >= E_PREPARANDO) ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
            else if (!a->pedido) a->en_uso = 0;
            arbitro();
        }
        break;
    case O_OLVIDAR: {
        ble_addr_t addr;
        const alias_t* al = alias_de(o->dir);
        int hay = 0;
        if (a) { addr = a->addr; hay = 1; }
        else if (al) { addr = al->addr; hay = 1; }
        else if (dir_de_texto(o->dir, "public", &addr) == 0) hay = 1;
        if (a) {
            a->esperado = 0;
            if (a->estado >= E_PREPARANDO) ble_gap_terminate(a->conn, BLE_ERR_REM_USER_CONN_TERM);
            else if (!a->pedido) a->en_uso = 0;
        }
        quita_alias(o->dir);
        if (hay) {
            ble_addr_t id = addr;
            id.type = BLE_ADDR_PUBLIC;
            (void) ble_store_util_delete_peer(&id);
            id.type = BLE_ADDR_RANDOM;
            (void) ble_store_util_delete_peer(&id);
        }
        prepara_claves();
        arbitro();
        xSemaphoreGive(s_orden_hecha);           /* quien olvida, escribe el fichero (vuelca_claves) */
        break;
    }
    default:
        break;
    }
    xSemaphoreTake(s_ordenes_m, portMAX_DELAY);
    o->libre = 1;
    xSemaphoreGive(s_ordenes_m);
}

static int manda(int que, const char* dir, int ms) {
    orden_t* o;
    if (!s_encendida) return -1;
    o = orden_libre();
    if (!o) return -1;
    o->que = que;
    o->ms = ms;
    snprintf(o->dir, sizeof o->dir, "%s", dir ? dir : "");
    ble_npl_event_init(&o->ev, al_orden, o);
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &o->ev);
    return 0;
}

/* ── Encender y apagar ─────────────────────────────────────────────────────────────────────────────────────────────── */

static void al_sincronizar(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc == 0) rc = ble_hs_id_infer_auto(0, &s_propia);
    if (rc != 0) {
        log_printf("bt: NimBLE sin dirección propia (%d)", rc);
        return;
    }
    /* El almacén de las claves, ya iniciado por la pila: el de la RAM, y además el fichero; y lo guardado, dentro. */
    ble_hs_cfg.store_write_cb = al_escribir_clave;
    ble_hs_cfg.store_delete_cb = al_borrar_clave;
    carga_claves();
    /* Y lo que la pila hace con lo guardado justo antes de avisar (ble_hs.c), otra vez, ya con lo nuestro dentro: las
     * IRK de los aparatos, a la lista de resolución del controlador. Sin ellas, uno que vuelve con una dirección privada
     * no casa con la lista blanca (11-nov: el teclado volvía y el ratón no). */
    (void) ble_hs_misc_restore_irks();
    s_lista = 1;
    {
        /* La dirección propia: la que ven los aparatos, y con la que se vincularon (el 14-oct, la pública de la C6). */
        ble_addr_t yo;
        char t[18];
        memset(&yo, 0, sizeof yo);
        yo.type = s_propia;
        (void) ble_hs_id_copy_addr(s_propia, yo.val, NULL);
        dir_texto(&yo, t, sizeof t);
        log_printf("bt: NimBLE en marcha (la propia, %s, %s; %d alias; claves: %u B)", t,
                   s_propia == BLE_OWN_ADDR_PUBLIC ? "pública" : "aleatoria", s_nalias, (unsigned) s_claves_len);
    }
    if (s_aviso) s_aviso(BPVM_BT_AV_LISTA, NULL, NULL);
    arbitro();
}

static void al_reiniciar(int motivo) {
    s_lista = 0;
    s_espera_activa = 0;
    s_buscando = 0;
    log_printf("bt: NimBLE se reinicia (motivo %d)", motivo);
}

static void tarea_nimble(void* arg) {
    (void) arg;
    nimble_port_run();            /* hasta que apagar la pare */
    nimble_port_freertos_deinit();
}

static int r_encender(bpvm_bt_aviso_fn aviso, char* err, size_t n) {
    esp_err_t e;
    int i;
    if (s_encendida) return 0;
    s_aviso = aviso;
    if (!s_pedido.listo) s_pedido.listo = xSemaphoreCreateBinary();
    if (!s_ordenes_m) s_ordenes_m = xSemaphoreCreateMutex();
    if (!s_orden_hecha) s_orden_hecha = xSemaphoreCreateBinary();
    if (!s_claves_m) s_claves_m = xSemaphoreCreateMutex();
    if (!s_pedido.listo || !s_ordenes_m || !s_orden_hecha || !s_claves_m) {
        snprintf(err, n, "Bluetooth: sin memoria");
        return -1;
    }
    if (!s_claves_buf) s_claves_buf = (uint8_t*) heap_caps_malloc(CLAVES_CAP, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_claves_buf) s_claves_buf = (uint8_t*) malloc(CLAVES_CAP);
    s_claves_sucias = 0;
    for (i = 0; i < ORDENES; i++) s_ordenes[i].libre = 1;
    s_ap = (aparato_t*) heap_caps_calloc(MAX_APARATOS, sizeof *s_ap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_ap) s_ap = (aparato_t*) calloc(MAX_APARATOS, sizeof *s_ap);
    if (!s_ap) { snprintf(err, n, "Bluetooth: sin memoria para los aparatos"); return -1; }
    s_espera_activa = s_espera_n = s_espera_fallos = s_buscar_ms = s_buscando = 0;
    s_pedido_ap = NULL;
    if (s_enlace && s_enlace->abrir(err, n) != 0) {   /* la P4: el enlace con la C6 y su controlador */
        heap_caps_free(s_ap);
        s_ap = NULL;
        return -1;
    }
    e = nimble_port_init();
    if (e != ESP_OK) {
        if (s_enlace) s_enlace->cerrar();
        heap_caps_free(s_ap);
        s_ap = NULL;
        snprintf(err, n, "Bluetooth: la radio no arranca (%s)", esp_err_to_name(e));
        return -1;
    }
    ble_hs_cfg.sync_cb = al_sincronizar;
    ble_hs_cfg.reset_cb = al_reiniciar;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;   /* Just Works: la placa no tiene con qué enseñar ni teclear un código */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_store_config_init();
    lee_fichero_claves();         /* al búfer; al almacén, al sincronizar (carga_claves) */
    s_lista = 0;
    s_encendida = 1;
    nimble_port_freertos_init(tarea_nimble);
    return 0;
}

static void r_apagar(void) {
    if (!s_encendida) return;
    s_encendida = 0;              /* el árbitro ya no empieza nada */
    if (ble_gap_disc_active()) ble_gap_disc_cancel();
    if (ble_gap_conn_active()) ble_gap_conn_cancel();
    nimble_port_stop();           /* corta las conexiones y para la tarea */
    nimble_port_deinit();         /* la pila y el controlador: la RAM vuelve */
    if (s_enlace) s_enlace->cerrar();   /* la P4: el controlador de la C6, y el enlace si nadie más lo usa */
    if (s_pedido_ap) {
        /* Quien espere a un connect, que no espere más. Ya sin la tarea de NimBLE: nadie más toca el pedido. */
        s_pedido.r = -1;
        snprintf(s_pedido.err, sizeof s_pedido.err, "Bluetooth: apagado");
        s_pedido_ap = NULL;
        xSemaphoreGive(s_pedido.listo);
    }
    vuelca_claves();              /* lo que quedara sin escribir */
    if (s_ap) {
        int i;
        for (i = 0; i < MAX_APARATOS; i++) if (s_ap[i].desc) heap_caps_free(s_ap[i].desc);
        heap_caps_free(s_ap);
        s_ap = NULL;
    }
    s_lista = 0;
    log_printf("bt: apagado");
}

/* ── La tabla 'BT  ' ───────────────────────────────────────────────────────────────────────────────────────────────── */

static int r_buscar(int ms, bpvm_bt_visto_fn visto, char* err, size_t n) {
    if (!s_encendida || !s_lista) { snprintf(err, n, "Bluetooth.scan: está apagado (turnOn primero)"); return -1; }
    s_visto = visto;
    if (manda(O_BUSCAR, NULL, ms) != 0) { snprintf(err, n, "Bluetooth.scan: la radio está ocupada"); return -1; }
    return 0;
}

/* Desde la tarea del Bluetooth de lo común, una a la vez: se lo pasa a la de NimBLE y espera su respuesta. */
static int r_conectar(const char* dir, const char* tipo, int ms, char* clase, size_t nclase, char* err, size_t n) {
    TickType_t espera;
    if (nclase) clase[0] = '\0';
    if (!s_encendida || !s_lista) { snprintf(err, n, "Bluetooth.connect: está apagado (turnOn primero)"); return -1; }
    (void) xSemaphoreTake(s_pedido.listo, 0);   /* una respuesta vieja que nadie recogió, fuera */
    s_pedido_id++;
    s_pedido.id = s_pedido_id;
    snprintf(s_pedido.dir, sizeof s_pedido.dir, "%s", dir);
    snprintf(s_pedido.tipo, sizeof s_pedido.tipo, "%s", tipo ? tipo : "public");
    s_pedido.ms = ms;
    s_pedido.r = -1;
    s_pedido.clase[0] = '\0';
    s_pedido.err[0] = '\0';
    ble_npl_event_init(&s_pedido.ev, al_pedido, &s_pedido);
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_pedido.ev);
    /* Lo que tarda de más lo de después de conectar (cifrar, el servicio HID): hasta 15 s. */
    espera = pdMS_TO_TICKS(ms > 0 ? ms + 15000 : 5000);
    if (xSemaphoreTake(s_pedido.listo, espera) != pdTRUE) {
        snprintf(err, n, "Bluetooth: la radio no contesta");
        return -1;
    }
    vuelca_claves();              /* si se emparejó, sus claves, al fichero: aquí, no en la tarea de NimBLE */
    if (s_pedido.r == 0) snprintf(clase, nclase, "%s", s_pedido.clase);
    else snprintf(err, n, "%s", s_pedido.err);
    return s_pedido.r;
}

static void r_desconectar(const char* dir) { (void) manda(O_DESCONECTAR, dir, 0); }

/* Desde el hilo del programa: lo hace la tarea de NimBLE, y aquí se espera (poco) para escribir el fichero. */
static void r_olvidar(const char* dir) {
    if (!s_orden_hecha) return;
    (void) xSemaphoreTake(s_orden_hecha, 0);
    if (manda(O_OLVIDAR, dir, 0) != 0) return;
    (void) xSemaphoreTake(s_orden_hecha, pdMS_TO_TICKS(3000));
    vuelca_claves();
}

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

void bt_esp32_publicar(const bt_esp32_enlace_t* enlace) {
    int r;
    s_enlace = enlace;
    r = bpvm_bios_publica(BPVM_BT_MARCA, &TABLA);
    if (r != 0) log_printf("bt: la radio no se pudo publicar (%d)", r);
    /* La pieza 3: con la radio, la placa trae teclado y ratón; la tabla 'ENTR', para la GUI del pack. */
    r = bpvm_entrada_publicar();
    if (r != 0) log_printf("bt: la entrada ('ENTR') no se pudo publicar (%d)", r);
}
