/*
 * bpvm_red.h — V8/H3 (6-nov): LA IP DE LA PLACA, lo común. El enlace es de bpvm_wifi.h: esto va encima y no sabe de radios
 * (Eduardo, 19-oct: «separar lo que es la conexión wifi del resto de los protocolos TCP/IP»).
 *
 * /sys/net.json dice cómo se pide la IP:
 *
 *     {"hostname":"taller"}                       por DHCP, con ese nombre en la red
 *     {"ip":"192.168.1.50","mask":"255.255.255.0","gateway":"192.168.1.1","dns":"192.168.1.1"}     fija
 *
 * Sin fichero, DHCP con el nombre de la placa (basicplus-<micro>). Quien la aplica es el backend de bpvm_net.h, que es
 * de la familia. Si el fichero está mal se dice en el log y se va por DHCP: una errata no debe dejar la placa sin red,
 * porque la red es por donde se arregla.
 *
 * Las direcciones viajan en un uint32_t con el primer número en el byte bajo (192.168.1.50 → 0x3201A8C0): es como las
 * guarda lwIP en un micro little-endian, y así la familia las pasa tal cual.
 */
#ifndef BPVM_RED_H
#define BPVM_RED_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_RED_FICHERO "/sys/net.json"

typedef struct {
    char     nombre[33];                  /* el nombre en la red; vacío: el de la placa */
    int      fija;                        /* 1: valen las cuatro de abajo */
    uint32_t ip, mascara, puerta, dns;    /* puerta y dns pueden ser 0 */
} bpvm_red_cfg_t;

/* 0, o -1 con el motivo. */
int bpvm_red_cfg_parse(const char* json, size_t len, bpvm_red_cfg_t* c, char* motivo, unsigned motivo_cap);

/* "192.168.1.50" → 0 y *ip; -1 si no son cuatro números de 0 a 255. */
int  bpvm_red_ip_parse(const char* texto, uint32_t* ip);
void bpvm_red_ip_texto(uint32_t ip, char* buf, size_t n);

/* Con el enlace a punto de conectarse: lee el fichero y se lo dice al backend. */
void bpvm_red_preparar(void);

/* Lo que pasa, según la familia (hay IP) y según el enlace (se ha caído: ya no la hay). */
void bpvm_red_hay_ip(int hay);

/* La IP de ahora, en texto. 1, o 0 y "" si no hay. */
int  bpvm_red_ip(char* buf, size_t n);

/* Quien quiera saber cuándo sube y baja la IP: el cable por la red, que abre su puerto con ella. Uno. */
void bpvm_red_al_cambiar(void (*fn)(int hay));

#ifdef __cplusplus
}
#endif

#endif /* BPVM_RED_H */
