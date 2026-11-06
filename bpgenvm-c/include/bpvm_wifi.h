/*
 * bpvm_wifi.h — V8/H3 (6-nov): LA WIFI, el enlace. Lo común de la HAL BP.
 *
 * Eduardo (19-oct): «el wifi es una capa de transporte del stack TCP/IP… separar lo que es la conexión wifi del resto de
 * los protocolos TCP/IP. Así limitamos el alcance de cada cosa». Esto es SÓLO el enlace: encender la radio, conectarse a
 * una red con su clave, saber cómo va y apagarla. La IP es de bpvm_red.h.
 *
 * Lo COMÚN (bpvm_wifi.c): /sys/wifi.json, los estados, los reintentos, cuándo se rinde con una clave mala y el apagado por
 * inactividad. Lo de debajo, LA RADIO, es una tabla por nombre con la marca 'WIFI', como 'PANT' con las pantallas: la
 * publica quien la tenga. Hoy, la imagen de las ESP (la radio está en el chip, o en la C6 de la P4). Mañana, un pack en la
 * Pico 2 W, porque la mayoría de las Pico 2 no llevan radio (Eduardo, 6-nov: «OK que en las ESP vaya integrado y en la
 * Pico 2 sea opcional»). Aquí nadie sabe quién la da: sin tabla, la placa no tiene WiFi y se dice.
 *
 * LA TABLA 'WIFI' (bpvm_pack_api_t: nombre → función):
 *
 *   obligatorias
 *     "encender"  int  encender(const char* pais, bpvm_wifi_aviso_fn aviso, char* err, size_t n)
 *                 La radio, lista para conectarse. Bloquea lo que haga falta (en la P4, 2,1 s: abrir el enlace con la C6).
 *                 `pais`: dos letras, o "" para el de fábrica. `aviso`: por donde contará lo que pase, desde la tarea
 *                 que sea. 0, o -1 con el motivo en `err`.
 *     "apagar"    void apagar(void)
 *                 Apagada DEL TODO: es el estado de reposo (Eduardo, 19-oct: manda el ahorro de energía). Vale también
 *                 tras un `encender` a medias.
 *     "conectar"  int  conectar(const char* ssid, const char* clave, char* err, size_t n)
 *                 Empieza a conectarse y VUELVE EN SEGUIDA; lo que pase llega por `aviso`. 0, o -1 con el motivo.
 *
 *   opcionales
 *     "rssi"      int  rssi(void)   La señal de la red a la que está conectada, en dBm; 0 si no se sabe.
 *
 * Añadir una función en una versión futura (buscar redes, por ejemplo) no rompe a quien ya la publica: va por nombre.
 *
 * LOS ESTADOS, con sus nombres de cara al IDE y a los programas: off, starting, connecting, connected, noNetwork y badKey.
 *
 * LA MÁQUINA va aparte y sin hilos ni reloj (bpvm_wifi_maq_*): recibe lo que pasa y la hora, y dice qué hay que hacer. Así
 * se prueba en el PC sin esperar los minutos de verdad (make test-wifi).
 */
#ifndef BPVM_WIFI_H
#define BPVM_WIFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_WIFI_MARCA    0x57494649u   /* 'WIFI' */
/* La versión es una puerta EXACTA (bpvm_pack_api_verify): sube sólo si cambia lo que ya existe. Una función nueva, no. */
#define BPVM_WIFI_API_VER  1u

#define BPVM_WIFI_FICHERO           "/sys/wifi.json"
#define BPVM_WIFI_AUTO_OFF_DEFECTO  600     /* s: Eduardo, 19-oct: «si hay 10 min de inactividad se apaga» */
#define BPVM_WIFI_INTENTOS_CLAVE    3       /* con la clave mala se para: seguir no arregla nada y gasta */
#define BPVM_WIFI_ESPERA_MIN_MS     1000    /* los reintentos: 1, 2, 4… */
#define BPVM_WIFI_ESPERA_MAX_MS     60000   /* …hasta 60 s */
#define BPVM_WIFI_CONECTANDO_MS     30000   /* sin noticias de la radio tras esto, se da el intento por perdido */

enum {
    BPVM_WIFI_OFF = 0,
    BPVM_WIFI_STARTING,      /* la radio se está encendiendo */
    BPVM_WIFI_CONNECTING,    /* buscando la red y presentando la clave */
    BPVM_WIFI_CONNECTED,     /* el enlace está arriba (la IP es otra cosa: bpvm_red.h) */
    BPVM_WIFI_NO_NETWORK,    /* encendida, sin red: se reintenta */
    BPVM_WIFI_BAD_KEY        /* la clave no vale: apagada, hasta que alguien la vuelva a encender */
};
const char* bpvm_wifi_estado_nombre(int estado);

/* Lo que cuenta la radio por su `aviso`. `dato`: el motivo en números de la familia, para el log. */
enum {
    BPVM_WIFI_AV_CONECTADA = 1,   /* el enlace está arriba */
    BPVM_WIFI_AV_SIN_RED,         /* no aparece la red */
    BPVM_WIFI_AV_CLAVE,           /* la red rechaza la clave */
    BPVM_WIFI_AV_CAIDA,           /* se ha caído, o no se ha podido, por otra cosa */
    BPVM_WIFI_AV_ROTA             /* la radio ya no contesta (en la P4, el enlace con la C6) */
};
typedef void (*bpvm_wifi_aviso_fn)(int aviso, int dato);

/* ── La configuración: /sys/wifi.json ──────────────────────────────────────────────────────────────────────────────── */

typedef struct {
    char    ssid[33];
    char    clave[65];      /* vacía: una red abierta */
    char    pais[3];        /* vacío: el de fábrica */
    int     auto_start;
    int32_t auto_off;       /* s; 0: no se apaga sola */
} bpvm_wifi_cfg_t;

/* {"ssid":"…","password":"…","country":"ES","autoStart":true,"autoOff":600}: sólo `ssid` es obligatorio. 0, o -1 con el
 * motivo (que nunca lleva la clave). */
int bpvm_wifi_cfg_parse(const char* json, size_t len, bpvm_wifi_cfg_t* c, char* motivo, unsigned motivo_cap);

/* ── La máquina ────────────────────────────────────────────────────────────────────────────────────────────────────── */

typedef struct {
    int      estado;
    int      fallos_clave;    /* rechazos seguidos */
    uint32_t espera_ms;       /* lo que se esperará tras el próximo fallo */
    int64_t  reintento_en;    /* cuándo reintentar; 0: no toca */
    int64_t  conectando_en;   /* desde cuándo se está conectando */
    int64_t  actividad_en;    /* lo último que contó como actividad */
    int      conexiones;      /* conexiones TCP abiertas */
    int32_t  auto_off;        /* s; 0: no se apaga sola */
} bpvm_wifi_maq_t;

/* Lo que entra. */
enum {
    BPVM_WIFI_E_ENCENDER = 1,   /* alguien pide encenderla */
    BPVM_WIFI_E_APAGAR,         /* alguien pide apagarla */
    BPVM_WIFI_E_ARRIBA,         /* el `encender` de la radio ha ido bien */
    BPVM_WIFI_E_NO_ARRANCA,     /* el `encender` de la radio ha fallado */
    BPVM_WIFI_E_CONECTADA,      /* desde aquí, los avisos de la radio */
    BPVM_WIFI_E_SIN_RED,
    BPVM_WIFI_E_CLAVE,
    BPVM_WIFI_E_CAIDA,
    BPVM_WIFI_E_ROTA,
    BPVM_WIFI_E_TIEMPO          /* ha llegado la hora que dijo bpvm_wifi_maq_cuando */
};
/* Lo que sale: lo que la tarea tiene que hacerle a la radio. */
enum {
    BPVM_WIFI_A_NADA = 0,
    BPVM_WIFI_A_ENCENDER,
    BPVM_WIFI_A_CONECTAR,
    BPVM_WIFI_A_APAGAR
};

void    bpvm_wifi_maq_iniciar(bpvm_wifi_maq_t* m);
int     bpvm_wifi_maq_paso(bpvm_wifi_maq_t* m, int entrada, int64_t ahora_ms);
/* La próxima hora a la que hay que volver a mirar; 0: ninguna, hasta que pase algo. */
int64_t bpvm_wifi_maq_cuando(const bpvm_wifi_maq_t* m);
/* Lo que cuenta como actividad (Eduardo, «1 al 7 OK», 6-nov): hay actividad mientras haya una conexión TCP abierta, del
 * programa o del IDE, y durante `autoOff` segundos desde que se cerró la última, o desde una búsqueda o una conexión. */
void    bpvm_wifi_maq_actividad(bpvm_wifi_maq_t* m, int64_t ahora_ms);
void    bpvm_wifi_maq_conexion(bpvm_wifi_maq_t* m, int abierta, int64_t ahora_ms);

/* ── La WiFi de la placa ───────────────────────────────────────────────────────────────────────────────────────────── */

/* El gancho `red` del arranque. `env_wifi`: la clave `wifi` del Entorno (1, 0, o -1 si no está). Se enciende con `wifi=1`
 * o con "autoStart": true en el fichero, y `wifi=0` manda sobre los dos; sin decir nada, apagada y sin gastar RAM. No
 * bloquea: la enciende su tarea, sin hacer esperar al cable ni al autorun. */
void bpvm_wifi_arranque(int env_wifi);

/* Enciende y se conecta con lo que diga el fichero, y VUELVE EN SEGUIDA. 0, o -1 con el motivo en `det`. */
int  bpvm_wifi_encender(char* det, size_t n);
void bpvm_wifi_apagar(void);

/* Lo mismo, pedido por un programa (Air.Wifi): al acabar, bpvm_wifi_fin_de_run la deja como diga la configuración. */
int  bpvm_wifi_programa_enciende(char* det, size_t n);
void bpvm_wifi_programa_apaga(void);
void bpvm_wifi_fin_de_run(void);

/* El PC: no lee fichero, y usa esta configuración. */
void bpvm_wifi_cfg_fija(const bpvm_wifi_cfg_t* c);
/* El PC (wifi_host.c): una radio que se conecta en el acto. */
void bpvm_wifi_register_host(void);

int         bpvm_wifi_hay(void);       /* 1 si alguien ha publicado la radio */
const char* bpvm_wifi_motivo(void);    /* por qué no la hay */
int         bpvm_wifi_estado(void);
void        bpvm_wifi_ssid(char* buf, size_t n);   /* la red de la configuración en uso; "" si no hay */
int         bpvm_wifi_rssi(void);      /* dBm, conectada; 0 si no */

/* Lo que lleva la cuenta de la actividad: quien abre y cierra conexiones, y quien busca. */
void bpvm_wifi_actividad(void);
void bpvm_wifi_conexion(int abierta);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_WIFI_H */
