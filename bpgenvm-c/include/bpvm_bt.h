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
 * LA TABLA 'BT  ' (bpvm_pack_api_t: nombre → función):
 *     "encender"     int  encender(bpvm_bt_aviso_fn aviso, char* err, size_t n)
 *                    Empieza a encender la radio; lo que pase (lista, un aparato que se conecta o se va) llega por `aviso`,
 *                    desde la tarea que sea. 0, o -1 con el motivo.
 *     "apagar"       void apagar(void)            Apagada del todo: corta las conexiones (el estado de reposo).
 *     "buscar"       int  buscar(int ms, bpvm_bt_visto_fn visto, char* err, size_t n)
 *                    Busca durante `ms` los aparatos en modo de emparejar; cada uno, por `visto`. Vuelve en seguida.
 *     "conectar"     int  conectar(const char* dir, const char* tipo, char* clase, size_t nclase, char* err, size_t n)
 *                    Conecta (la primera vez, empareja) y dice qué es, en `clase`: "keyboard", "mouse" o "unknown". 0 o -1.
 *     "desconectar"  void desconectar(const char* dir)
 *     "olvidar"      void olvidar(const char* dir)      La cierra y borra sus claves.
 *
 * LOS ESTADOS, para los programas: 0 off, 1 starting, 2 on, 3 scanning (mientras dura una búsqueda).
 */
#ifndef BPVM_BT_H
#define BPVM_BT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_BT_MARCA    0x42542020u   /* 'BT  ' */
#define BPVM_BT_API_VER  1u
#define BPVM_BT_FICHERO  "/sys/bluetooth.json"

enum { BPVM_BT_OFF = 0, BPVM_BT_STARTING = 1, BPVM_BT_ON = 2, BPVM_BT_SCANNING = 3 };

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

/* Lo que cuenta la radio por su `aviso`. `dir`: el aparato, o NULL. */
enum { BPVM_BT_AV_LISTA = 1, BPVM_BT_AV_SE_FUE = 2 };
typedef void (*bpvm_bt_aviso_fn)(int aviso, const char* dir);
typedef void (*bpvm_bt_visto_fn)(const bpvm_bt_aparato_t* a);

/* Lo que llaman los builtins (Air.Bluetooth). Con el cerrojo de esta parte: valen desde cualquier hilo. */
int  bpvm_bt_hay(void);                                   /* ¿hay radio? */
int  bpvm_bt_estado(void);                                /* BPVM_BT_OFF… */
int  bpvm_bt_programa_enciende(char* det, size_t n);      /* 0, o -1 con el motivo */
void bpvm_bt_programa_apaga(void);
int  bpvm_bt_buscar(int ms, char* det, size_t n);
int  bpvm_bt_encontrados(void);
int  bpvm_bt_encontrado(int i, bpvm_bt_aparato_t* out);   /* 0, o -1 si `i` no está */
int  bpvm_bt_conectar(const char* dir, const char* tipo, char* det, size_t n);   /* 1 conectado, 0 no, -1 error */
void bpvm_bt_desconectar(const char* dir);
void bpvm_bt_olvidar(const char* dir);
int  bpvm_bt_conectado(const char* dir);
void bpvm_bt_clase_de(const char* dir, char* out, size_t n);   /* "" si no está conectado */
int  bpvm_bt_hay_teclado(void);   /* V8/H2, paso 2: ¿hay un teclado conectado? (en el PC, la puerta de las teclas de la ventana) */

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
