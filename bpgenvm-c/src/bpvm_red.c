/*
 * bpvm_red.c — V8/H3 (6-nov): la IP de la placa, lo común. La interfaz y el porqué, en bpvm_red.h.
 */
#include "bpvm_red.h"
#include "bpvm.h"
#include "bpvm_fs.h"
#include "bpvm_machine.h"
#include "bpvm_net.h"
#include "json_min.h"

#include <stdio.h>
#include <string.h>

#define RED_JSON_MAX 256

static volatile int s_hay = 0;
static void (*s_al_cambiar)(int hay) = 0;
static char s_nombre[48] = "";

static void di(char* motivo, unsigned cap, const char* texto) {
    if (motivo && cap) snprintf(motivo, cap, "%s", texto);
}

int bpvm_red_ip_parse(const char* t, uint32_t* ip) {
    uint32_t v = 0;
    int i;
    if (!t) return -1;
    for (i = 0; i < 4; i++) {
        int n = 0, digitos = 0;
        while (*t >= '0' && *t <= '9' && digitos < 3) {
            n = n * 10 + (*t++ - '0');
            digitos++;
        }
        if (!digitos || n > 255) return -1;
        v |= (uint32_t) n << (8 * i);
        if (i < 3 && *t++ != '.') return -1;
    }
    if (*t) return -1;
    *ip = v;
    return 0;
}

void bpvm_red_ip_texto(uint32_t ip, char* buf, size_t n) {
    snprintf(buf, n, "%u.%u.%u.%u", (unsigned) (ip & 255u), (unsigned) ((ip >> 8) & 255u),
             (unsigned) ((ip >> 16) & 255u), (unsigned) (ip >> 24));
}

/* Una dirección opcional: sin la clave se queda en 0; mal escrita, error. */
static int direccion(const json_obj_t* o, const char* clave, uint32_t* ip) {
    char t[24];
    int r = json_get_str(o, clave, t, sizeof t);
    *ip = 0;
    if (r == -1) return 0;
    return (r < 0) ? -1 : bpvm_red_ip_parse(t, ip);
}

int bpvm_red_cfg_parse(const char* json, size_t len, bpvm_red_cfg_t* c, char* motivo, unsigned cap) {
    json_obj_t o;
    int r;
    memset(c, 0, sizeof *c);
    if (motivo && cap) motivo[0] = 0;
    if (json_parse(json, len, &o) != 0) { di(motivo, cap, "no es JSON valido"); return -1; }
    r = json_get_str(&o, "hostname", c->nombre, sizeof c->nombre);
    if (r == -2) { di(motivo, cap, "hostname no cabe (32 bytes)"); return -1; }
    if (!json_find(&o, "ip")) return 0;                             /* por DHCP */
    if (direccion(&o, "ip", &c->ip) != 0 || c->ip == 0) { di(motivo, cap, "ip no es una direccion"); return -1; }
    if (direccion(&o, "mask", &c->mascara) != 0 || c->mascara == 0) { di(motivo, cap, "mask no es una direccion, y con ip hace falta"); return -1; }
    if (direccion(&o, "gateway", &c->puerta) != 0) { di(motivo, cap, "gateway no es una direccion"); return -1; }
    if (direccion(&o, "dns", &c->dns) != 0) { di(motivo, cap, "dns no es una direccion"); return -1; }
    c->fija = 1;
    return 0;
}

void bpvm_red_preparar(void) {
    bpvm_red_cfg_t c;
    char json[RED_JSON_MAX], motivo[64], micro[24], t[20];
    memset(&c, 0, sizeof c);
    if (bpvm_fs_exists(BPVM_RED_FICHERO)) {
        long n = bpvm_fs_read(BPVM_RED_FICHERO, (uint8_t*) json, sizeof json);
        if (n < 0) {
            bpvm_diag("red: %s no se puede leer: va por DHCP", BPVM_RED_FICHERO);
        } else {
            while (n > 0 && (json[n - 1] == '\n' || json[n - 1] == '\r' || json[n - 1] == ' ')) n--;
            if (bpvm_red_cfg_parse(json, (size_t) n, &c, motivo, sizeof motivo) != 0) {
                bpvm_diag("red: %s: %s: va por DHCP", BPVM_RED_FICHERO, motivo);
                memset(&c, 0, sizeof c);
            }
        }
    }
    if (c.nombre[0]) {
        snprintf(s_nombre, sizeof s_nombre, "%s", c.nombre);
    } else {
        micro[0] = 0;
        bpvm_machine_micro_name(micro, sizeof micro);
        snprintf(s_nombre, sizeof s_nombre, "basicplus-%s", micro);
    }
    if (c.fija) {
        bpvm_red_ip_texto(c.ip, t, sizeof t);
        if (bpvm_net_ip_fija(c.ip, c.mascara, c.puerta, c.dns) == 0) bpvm_diag("red: IP fija %s", t);
        else                                                         bpvm_diag("red: la IP fija %s no se pudo poner", t);
        return;
    }
    if (bpvm_net_ip_dinamica(s_nombre) == 0) bpvm_diag("red: IP por DHCP, con el nombre %s", s_nombre);
    else                                     bpvm_diag("red: el DHCP no se pudo preparar");
}

const char* bpvm_red_nombre(void) { return s_nombre; }

void bpvm_red_hay_ip(int hay) {
    char t[20];
    hay = hay ? 1 : 0;
    if (hay == s_hay) return;
    s_hay = hay;
    if (hay && bpvm_red_ip(t, sizeof t)) bpvm_diag("red: IP %s", t);
    else if (!hay)                       bpvm_diag("red: sin IP");
    if (s_al_cambiar) s_al_cambiar(hay);
}

int bpvm_red_hay(void) { return s_hay; }

int bpvm_red_ip(char* buf, size_t n) {
    uint32_t ip = 0, m, p, d;
    if (n) buf[0] = 0;
    if (!s_hay || bpvm_net_ip_leer(&ip, &m, &p, &d) != 0 || ip == 0) return 0;
    bpvm_red_ip_texto(ip, buf, n);
    return 1;
}

void bpvm_red_al_cambiar(void (*fn)(int hay)) { s_al_cambiar = fn; }
