/*
 * bpvm_blk_sdmmc_cfg.c — V5/H6: la línea `sd` del ENV, variante SDIO.
 *
 * Sólo texto: ni SDK ni hardware, para que se pueda ejercer en el PC
 * (`make test-sdio`). El resto de la cintura vive en la familia.
 *
 * Los dos ayudantes de abajo son gemelos de los de `bpvm_sd.c`. Están
 * duplicados A PROPÓSITO: son ocho líneas, y compartirlos ataría este fichero
 * —que es la configuración de SDIO— al fichero del PROTOCOLO SPI, que es
 * justo la mezcla que H6 vino a deshacer. Si aparece un tercer parseo, ése es
 * el momento de sacarlos a un sitio común, no antes.
 */
#include "bpvm_blk_sdmmc.h"

static void di(char* dst, unsigned cap, const char* a, const char* b) {
    if (!dst || cap == 0) return;
    unsigned i = 0;
    while (a && *a && i + 1 < cap) dst[i++] = *a++;
    while (b && *b && i + 1 < cap) dst[i++] = *b++;
    dst[i] = '\0';
}

static const char* tras_clave(const char* s, const char* k) {
    while (*k) { if (*s != *k) return 0; s++; k++; }
    return (*s == ':') ? s + 1 : 0;
}

int bpvm_sdio_pines_parse(const char* valor, bpvm_sdio_pines_t* p,
                          char* motivo, unsigned motivo_cap)
{
    if (motivo && motivo_cap) motivo[0] = '\0';
    if (!p) return -1;

    /* Defectos ANTES de leer: lo que no venga se queda así, y un -1 en un pin
     * obligatorio es lo que luego delata que faltaba. */
    p->slot = 0;
    p->clk = p->cmd = p->d0 = -1;
    p->d1 = p->d2 = p->d3 = -1;
    p->ancho = 1;
    p->pwr = -1;
    /* Activo BAJO. Verificado EN PLACA (P4, 11-ago): con GPIO45 a 0 la tarjeta
     * arranca y monta; a 1, no. Coincide con lo que dice el transistor —un
     * MOSFET de canal P con la fuente en 3V3 conduce con la puerta baja— y
     * NO con una medida de 3,3 V en el zócalo que resultó ser ambigua. */
    p->pwr_activo_alto = 0;
    p->khz = 0;
    /* El LDO interno que alimenta las E/S de SDMMC. Por defecto ENCENDIDO en el
     * canal 4, que es el que usan tanto el kit de Espressif como la Waveshare.
     * `ldo:0` lo desactiva. No es lo mismo que `pwr` — ver bpvm_blk_sdmmc.h. */
    p->ldo = 4;

    if (!valor || !*valor) { di(motivo, motivo_cap, "la entrada 'sd' esta vacia", 0); return -1; }

    const char* s = valor;
    while (*s) {
        while (*s == ' ' || *s == ',' || *s == '\t') s++;
        if (!*s) break;
        /* OJO al ORDEN: 'd0'..'d3' antes que nada que empiece igual, y las
         * claves largas antes que sus prefijos ('pwralto' antes que 'pwr'), o
         * el prefijo se come a la larga y el valor se lee mal EN SILENCIO. */
        static const struct { const char* clave; int campo; } tabla[] = {
            { "clk", 0 }, { "cmd", 1 },
            { "d0", 2 }, { "d1", 3 }, { "d2", 4 }, { "d3", 5 },
            { "pwralto", 6 }, { "pwr", 7 },
            { "slot", 8 }, { "khz", 9 }, { "ldo", 10 }
        };
        const char* v = 0;
        int campo = -1;
        for (unsigned i = 0; i < sizeof tabla / sizeof tabla[0]; i++) {
            v = tras_clave(s, tabla[i].clave);
            if (v) { campo = tabla[i].campo; break; }
        }
        if (!v) {                      /* clave desconocida: saltarla entera */
            while (*s && *s != ',') s++;
            continue;
        }
        int n = 0, digitos = 0;
        while (*v >= '0' && *v <= '9') { n = n * 10 + (*v - '0'); v++; digitos++; }
        if (!digitos) {
            di(motivo, motivo_cap, "valor no numerico en 'sd', cerca de ", s);
            return -1;
        }
        switch (campo) {
        case 0: p->clk  = n; break;
        case 1: p->cmd  = n; break;
        case 2: p->d0   = n; break;
        case 3: p->d1   = n; break;
        case 4: p->d2   = n; break;
        case 5: p->d3   = n; break;
        case 6: p->pwr_activo_alto = n ? 1 : 0; break;
        case 7: p->pwr  = n; break;
        case 8: p->slot = n; break;
        case 9: p->khz  = n; break;
        case 10: p->ldo = n; break;
        default: break;
        }
        s = v;
    }

    /* El chivato que justifica las etiquetas: decir CUÁL falta. */
    const char* falta = 0;
    if      (p->clk < 0) falta = "clk";
    else if (p->cmd < 0) falta = "cmd";
    else if (p->d0  < 0) falta = "d0";
    if (falta) { di(motivo, motivo_cap, "a la entrada 'sd' le falta ", falta); return -1; }

    /* El ancho se DEDUCE, no se escribe. Pedir a la vez `d1..d3` y `ancho:4`
     * sería poder decir dos cosas distintas sobre lo mismo — y el día que no
     * cuadren, el bus se configura de una forma y el cableado es de otra.
     *
     * Y las cuatro líneas se exigen JUNTAS: con tres de las cuatro, el driver
     * quedaría a 4 bits con una línea sin cablear, que no falla al montar —
     * falla al leer un bloque, más tarde y en otro sitio. */
    p->ancho = (p->d1 >= 0 && p->d2 >= 0 && p->d3 >= 0) ? 4 : 1;
    if (p->ancho == 1 && (p->d1 >= 0 || p->d2 >= 0 || p->d3 >= 0)) {
        di(motivo, motivo_cap,
           "en 'sd' hay lineas de datos a medias: pon d1, d2 y d3 (4 bits) o ninguna (1 bit)", 0);
        return -1;
    }
    return 0;
}
