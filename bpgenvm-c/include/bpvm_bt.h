/*
 * bpvm_bt.h — V8/H2 (8-nov): EL BLUETOOTH. Lo común de la HAL BP, para la clase Air.Bluetooth (V8_IDEAS §E1, «Lo que ve
 * el programa»). Eduardo: «Funcionalidades, encender y apagar, scan, conectar y desconectar».
 *
 * Lo COMÚN (bpvm_bt.c): el estado, lo encontrado en la última búsqueda, lo conectado y lo recordado de /sys/bluetooth.json
 * (lo escribe la clase, en BP; aquí sólo se lee, para esperar a los recordados al encender y al arrancar). Lo de debajo, LA
 * RADIO, es una tabla por nombre con la marca 'BT  ', como 'WIFI': la publica quien la tenga. Hoy, el PC (bt_host.c: un
 * Bluetooth simulado, con dos aparatos de mentira, igual que la VM-Java; también en el micro simulado del IDE); mañana, la
 * imagen de las ESP (NimBLE). Sin tabla, la placa no tiene Bluetooth y se dice.
 *
 * LA TABLA 'BT  ' (bpvm_pack_api_t: nombre → función). La versión 2 (V8/H2, paso 5, 11-nov), con la radio de una placa:
 *     "encender"     int  encender(bpvm_bt_aviso_fn aviso, char* err, size_t n)
 *                    Empieza a encender la radio; lo que pase llega por `aviso`, desde la tarea que sea: la radio lista, un
 *                    aparato que se va, o uno recordado que vuelve solo (con lo que es). 0, o -1 con el motivo.
 *     "apagar"       void apagar(void)            Apagada del todo: corta las conexiones (el estado de reposo).
 *     "buscar"       int  buscar(int ms, bpvm_bt_visto_fn visto, char* err, size_t n)
 *                    Busca durante `ms` los aparatos en modo de emparejar; cada uno, por `visto`. Vuelve en seguida.
 *     "conectar"     int  conectar(const char* dir, const char* tipo, int ms, char* clase, size_t nclase,
 *                                  char* err, size_t n)
 *                    Conecta (la primera vez, empareja) y dice qué es, en `clase`: "keyboard", "mouse" o "unknown". 0 o -1.
 *                    Espera hasta `ms`. Con 0 no espera (al encender, los recordados): si no está ahora, la radio lo sigue
 *                    esperando por su dirección, y cuando vuelve lo avisa (BPVM_BT_AV_VOLVIO).
 *     "desconectar"  void desconectar(const char* dir)   La cierra, y la radio deja de esperarlo.
 *     "olvidar"      void olvidar(const char* dir)      La cierra y borra sus claves.
 * La versión 1 (8-nov) era la del PC: conectar sin `ms`, y el aviso sin la clase.
 *
 * LA TAREA DEL BLUETOOTH (paso 5, como la de la WiFi): la única que llama a `conectar`. Conectar en una placa tarda
 * segundos (emparejar, el servicio HID, suscribirse), y la VM corre todos sus hilos en uno del sistema: un builtin que
 * esperara los pararía todos, la GUI incluida. Así que los builtins dejan el encargo y vuelven; el programa espera en BP,
 * con sleep (Air.Bluetooth.connect, mientras el estado es 4), como en turnOn y scan.
 *
 * LOS ESTADOS, para los programas: 0 off, 1 starting, 2 on, 3 scanning (mientras dura una búsqueda), 4 connecting
 * (mientras hay un connect del programa sin acabar; en el PC, un momento).
 */
#ifndef BPVM_BT_H
#define BPVM_BT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_BT_MARCA    0x42542020u   /* 'BT  ' */
#define BPVM_BT_API_VER  2u
#define BPVM_BT_FICHERO  "/sys/bluetooth.json"

enum { BPVM_BT_OFF = 0, BPVM_BT_STARTING = 1, BPVM_BT_ON = 2, BPVM_BT_SCANNING = 3, BPVM_BT_CONNECTING = 4 };

#define BPVM_BT_MAX_ENCONTRADOS  16
#define BPVM_BT_MAX_CONECTADOS   8
#define BPVM_BT_NOMBRE           32    /* con el NUL; uno más largo se corta */
#define BPVM_BT_DIR              18    /* "AA:BB:CC:DD:EE:FF" y el NUL */
#define BPVM_BT_TIPO             8     /* "public", "random" */
#define BPVM_BT_CLASE            10    /* "keyboard", "mouse", "unknown" */

typedef struct {
    char    nombre[BPVM_BT_NOMBRE];
    char    dir[BPVM_BT_DIR];
    char    tipo[BPVM_BT_TIPO];
    char    clase[BPVM_BT_CLASE];
    int32_t rssi;
} bpvm_bt_aparato_t;

/* Lo que cuenta la radio por su `aviso`. `dir`: el aparato, o NULL; `clase`, lo que es, con BPVM_BT_AV_VOLVIO (si no,
 * NULL). */
enum { BPVM_BT_AV_LISTA = 1, BPVM_BT_AV_SE_FUE = 2, BPVM_BT_AV_VOLVIO = 3 };
typedef void (*bpvm_bt_aviso_fn)(int aviso, const char* dir, const char* clase);
typedef void (*bpvm_bt_visto_fn)(const bpvm_bt_aparato_t* a);

/* Lo que llaman los builtins (Air.Bluetooth). Con el cerrojo de esta parte: valen desde cualquier hilo. */
int  bpvm_bt_hay(void);                                   /* ¿hay radio? */
int  bpvm_bt_estado(void);                                /* BPVM_BT_OFF… */
int  bpvm_bt_programa_enciende(char* det, size_t n);      /* 0, o -1 con el motivo */
void bpvm_bt_programa_apaga(void);
int  bpvm_bt_buscar(int ms, char* det, size_t n);
int  bpvm_bt_encontrados(void);
int  bpvm_bt_encontrado(int i, bpvm_bt_aparato_t* out);   /* 0, o -1 si `i` no está */
/* Empieza a conectar, en la tarea del Bluetooth, y vuelve: 1 ya conectado, 2 empezado (el estado, 4 hasta que acabe; si
 * conecta, después lo dice bpvm_bt_conectado), 0 no se pudo empezar (la cola, llena), -1 error (apagado). */
int  bpvm_bt_conectar(const char* dir, const char* tipo, char* det, size_t n);
void bpvm_bt_desconectar(const char* dir);
void bpvm_bt_olvidar(const char* dir);
int  bpvm_bt_conectado(const char* dir);
void bpvm_bt_clase_de(const char* dir, char* out, size_t n);   /* "" si no está conectado */
int  bpvm_bt_hay_teclado(void);   /* V8/H2, paso 2: ¿hay un teclado conectado? (en el PC, la puerta de las teclas de la ventana) */
int  bpvm_bt_hay_raton(void);     /* V8/H2, paso 3: ¿hay un ratón conectado? (en el PC, la puerta del ratón de la ventana) */

/* El arranque y el fin de un RUN, como la WiFi: al arrancar, si el fichero dice "autoStart", se enciende y espera a los
 * recordados (si alguno no está o no conecta, no es un error); al acabar un programa que lo tocó, vuelve a lo que diga el
 * fichero. */
void bpvm_bt_arranque(void);
void bpvm_bt_fin_de_run(void);

/* El PC: la radio simulada (bt_host.c). */
void bpvm_bt_register_host(void);

#ifdef __cplusplus
}
#endif
#endif
