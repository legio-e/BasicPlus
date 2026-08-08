/*
 * bpvm_sd.c — V5/H1: el diálogo con una tarjeta SD en modo SPI.
 *
 * Contrato y motivos en bpvm_sd.h. Aquí sólo hay protocolo, y NI UN #ifdef de
 * familia: todo sale por las fachadas portables `bpvm_spi_*` y `bpvm_gpio_*`.
 */
#include "bpvm_sd.h"
#include "bpvm_spi.h"
#include "bpvm_gpio.h"
#include "bpvm_platform.h"

#include <string.h>

/* ── Comandos que usamos. El resto de la baraja no hace falta para arrancar. ── */
#define CMD0_GO_IDLE        0
#define CMD8_SEND_IF_COND   8
#define CMD9_SEND_CSD       9
#define CMD10_SEND_CID     10
#define CMD17_READ_SINGLE  17
#define CMD24_WRITE_SINGLE 24
#define CMD55_APP          55
#define CMD58_READ_OCR     58
#define ACMD41_SEND_OP_COND 41

#define R1_IDLE            0x01
#define TOKEN_DATO         0xFE   /* precede a un bloque de datos */

/* Plazos. El de ACMD41 es el del estándar (1 s); el resto son generosos a
 * propósito: si se agotan es que algo está roto, no que iba justo. */
#define PLAZO_ARRANQUE_MS  1000
#define PLAZO_R1_BYTES       10   /* la tarjeta contesta en <=8; damos 10      */
#define PLAZO_TOKEN_MS      200
/* Grabar puede llevarse cientos de ms si a la tarjeta le toca borrar. */
#define PLAZO_OCUPADA_MS    2000

/* Reloj de negociación. NO es una precaución de manual: por encima de 400 kHz
 * la tarjeta no está obligada a entenderte mientras negocia, y el fallo sale
 * como "unas tarjetas sí y otras no" — el peor de todos. */
#define BAUD_LENTO       400000

/* ─────────────────────────────────────────────────────────────────────────
 * Bajo nivel
 * ───────────────────────────────────────────────────────────────────────── */

static void cs_bajo(const bpvm_sd_pines_t* p)  { bpvm_gpio_write(p->cs, 0); }
static void cs_alto(const bpvm_sd_pines_t* p)  { bpvm_gpio_write(p->cs, 1); }

/* Un byte de ida y vuelta. En SPI no hay "sólo leer": para recibir hay que
 * mandar algo, y ese algo es 0xFF (línea en reposo). */
static uint8_t intercambia(int bus, uint8_t tx) {
    uint8_t rx = 0xFF;
    bpvm_spi_transfer(bus, &tx, &rx, 1);
    return rx;
}

static void reloj_en_vacio(int bus, int bytes) {
    for (int i = 0; i < bytes; i++) (void) intercambia(bus, 0xFF);
}

/*
 * Manda un comando y devuelve su R1. 0xFF = no contestó.
 *
 * El CRC sólo importa en CMD0 y CMD8: en modo SPI la tarjeta arranca con la
 * comprobación apagada, así que a partir de ahí cualquier valor vale. Por eso
 * van horneados los dos que hacen falta en vez de calcular CRC7.
 */
static uint8_t manda_cmd(int bus, uint8_t cmd, uint32_t arg, uint8_t crc,
                         bpvm_sd_info_t* diag) {
    if (diag) {
        diag->ultimo_cmd = cmd;
        for (int i = 0; i < 8; i++) diag->traza[i] = 0xAA;  /* 0xAA = no leído */
    }
    (void) intercambia(bus, 0xFF);          /* un hueco antes de hablar */
    (void) intercambia(bus, (uint8_t) (0x40 | cmd));
    (void) intercambia(bus, (uint8_t) (arg >> 24));
    (void) intercambia(bus, (uint8_t) (arg >> 16));
    (void) intercambia(bus, (uint8_t) (arg >> 8));
    (void) intercambia(bus, (uint8_t) arg);
    (void) intercambia(bus, crc);
    for (int i = 0; i < PLAZO_R1_BYTES; i++) {
        uint8_t r = intercambia(bus, 0xFF);
        if (diag && i < 8) diag->traza[i] = r;
        if ((r & 0x80) == 0) return r;      /* el bit 7 a 0 marca la respuesta */
    }
    return 0xFF;
}

/* ACMD = CMD55 y luego el comando de aplicación. */
static uint8_t manda_acmd(int bus, uint8_t cmd, uint32_t arg, bpvm_sd_info_t* diag) {
    (void) manda_cmd(bus, CMD55_APP, 0, 0x65, NULL);
    return manda_cmd(bus, cmd, arg, 0x77, diag);
}

/* Espera el token de dato y recoge `n` bytes + los 2 de CRC. */
static int lee_bloque(int bus, uint8_t* dst, int n) {
    int64_t t0 = bpvm_platform_now_ms();
    for (;;) {
        uint8_t t = intercambia(bus, 0xFF);
        if (t == TOKEN_DATO) break;
        if (t != 0xFF) return -1;                    /* token de error */
        if (bpvm_platform_now_ms() - t0 > PLAZO_TOKEN_MS) return -1;
    }
    for (int i = 0; i < n; i++) dst[i] = intercambia(bus, 0xFF);
    (void) intercambia(bus, 0xFF);                   /* CRC16, que ignoramos */
    (void) intercambia(bus, 0xFF);
    return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Decodificadores PUROS — separados para poder probarlos en el PC
 * ───────────────────────────────────────────────────────────────────────── */

/* Extrae los bits [hi:lo] del CSD/CID, numerados como en el estándar: el bit
 * 127 es el más significativo de `b[0]` y el bit 0 el menos de `b[15]`. */
static uint32_t bits(const uint8_t b[16], int hi, int lo) {
    uint32_t v = 0;
    for (int i = hi; i >= lo; i--) {
        int byte = 15 - (i >> 3);
        int bit  = i & 7;
        v = (v << 1) | ((b[byte] >> bit) & 1u);
    }
    return v;
}

bpvm_sd_res_t bpvm_sd_csd_capacidad(const uint8_t csd[16],
                                    uint32_t* bloques, uint8_t* version)
{
    uint32_t ver = bits(csd, 127, 126);
    if (ver == 0) {
        /* CSD v1 (SDSC). La capacidad sale de TRES campos y una potencia:
         *   bloques_de_lectura = (C_SIZE + 1) * 2^(C_SIZE_MULT + 2)
         *   bytes              = bloques_de_lectura * 2^READ_BL_LEN
         * Se calcula en bloques de 512 directamente para no desbordar 32 bits
         * en tarjetas de 2 GB (READ_BL_LEN puede ser 10 u 11). */
        uint32_t c_size      = bits(csd, 73, 62);
        uint32_t c_size_mult = bits(csd, 49, 47);
        uint32_t read_bl_len = bits(csd, 83, 80);
        if (read_bl_len < 9 || read_bl_len > 11) return BPVM_SD_E_CSD_VER;
        uint32_t mult = 1u << (c_size_mult + 2);
        uint32_t b512 = (c_size + 1u) * mult;
        b512 <<= (read_bl_len - 9);      /* de bloques de 2^READ_BL_LEN a 512 */
        *bloques = b512;
        *version = 1;
        return BPVM_SD_OK;
    }
    if (ver == 1) {
        /* CSD v2 (SDHC/SDXC): un solo campo y la cuenta es directa —
         * capacidad = (C_SIZE + 1) * 512 KB = (C_SIZE + 1) * 1024 bloques. */
        uint32_t c_size = bits(csd, 69, 48);
        *bloques = (c_size + 1u) * 1024u;
        *version = 2;
        return BPVM_SD_OK;
    }
    /* ver == 2 es SDUC (>2 TB) y ver == 3 no existe. Ninguna de las dos se
     * adivina: negarse con nombre es mejor que devolver una capacidad falsa. */
    return BPVM_SD_E_CSD_VER;
}

void bpvm_sd_cid_desglosar(const uint8_t cid[16], bpvm_sd_info_t* info)
{
    info->fabricante = cid[0];
    info->oem[0] = (char) cid[1];
    info->oem[1] = (char) cid[2];
    info->oem[2] = '\0';
    for (int i = 0; i < 5; i++) info->producto[i] = (char) cid[3 + i];
    info->producto[5] = '\0';
    info->rev_mayor = (uint8_t) (cid[8] >> 4);
    info->rev_menor = (uint8_t) (cid[8] & 0x0F);
    info->serie = ((uint32_t) cid[9]  << 24) | ((uint32_t) cid[10] << 16)
                | ((uint32_t) cid[11] <<  8) |  (uint32_t) cid[12];
    /* MDT ocupa los bits [19:8]: 8 de año (desde 2000) y 4 de mes. */
    uint32_t mdt = (((uint32_t) cid[13] & 0x0Fu) << 8) | cid[14];
    info->anno = (uint16_t) (2000u + (mdt >> 4));
    info->mes  = (uint8_t) (mdt & 0x0Fu);
}

/* ─────────────────────────────────────────────────────────────────────────
 * La entrada `sd` del ENV — también pura, también probada en el PC
 * ───────────────────────────────────────────────────────────────────────── */

/* Copia literales al buffer del llamante sin traerse `snprintf`: en el micro
 * eso arrastra la maquinaria de printf entera para un mensaje de diagnóstico. */
static void di(char* dst, unsigned cap, const char* a, const char* b) {
    if (!dst || cap == 0) return;
    unsigned i = 0;
    while (a && *a && i + 1 < cap) dst[i++] = *a++;
    while (b && *b && i + 1 < cap) dst[i++] = *b++;
    dst[i] = '\0';
}

/* ¿Empieza `s` por la clave `k` seguida de ':'? Devuelve el valor o NULL. */
static const char* tras_clave(const char* s, const char* k) {
    while (*k) { if (*s != *k) return 0; s++; k++; }
    return (*s == ':') ? s + 1 : 0;
}

int bpvm_sd_pines_parse(const char* valor, bpvm_sd_pines_t* p,
                        char* motivo, unsigned motivo_cap)
{
    if (motivo && motivo_cap) motivo[0] = '\0';
    if (!p) return -1;
    /* Valores por defecto ANTES de leer nada: lo que no venga se queda así, y
     * -1 en un pin obligatorio es lo que luego delata que faltaba. */
    p->bus = 0;
    p->sck = p->mosi = p->miso = p->cs = -1;
    p->cd = -1;
    p->cd_activo_bajo = 1;

    if (!valor || !*valor) { di(motivo, motivo_cap, "la entrada 'sd' esta vacia", 0); return -1; }

    const char* s = valor;
    while (*s) {
        while (*s == ' ' || *s == ',' || *s == '\t') s++;
        if (!*s) break;
        static const struct { const char* clave; int campo; } tabla[] = {
            { "sck", 0 }, { "mosi", 1 }, { "miso", 2 }, { "cs", 3 },
            { "cd",  4 }, { "bus",  5 }, { "cdalto", 6 }
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
        case 0: p->sck  = n; break;
        case 1: p->mosi = n; break;
        case 2: p->miso = n; break;
        case 3: p->cs   = n; break;
        case 4: p->cd   = n; break;
        case 5: p->bus  = n; break;
        case 6: p->cd_activo_bajo = n ? 0 : 1; break;
        default: break;
        }
        s = v;
    }

    /* Y el chivato que justifica las etiquetas: decir CUÁL falta. */
    const char* falta = 0;
    if      (p->sck  < 0) falta = "sck";
    else if (p->mosi < 0) falta = "mosi";
    else if (p->miso < 0) falta = "miso";
    else if (p->cs   < 0) falta = "cs";
    if (falta) { di(motivo, motivo_cap, "a la entrada 'sd' le falta ", falta); return -1; }
    return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Arranque
 * ───────────────────────────────────────────────────────────────────────── */

int bpvm_sd_hay_tarjeta(const bpvm_sd_pines_t* p)
{
    if (!p || p->cd < 0) return 1;      /* sin detector no se puede saber */

    /* ⚠️ EL PIN SE CONFIGURA AQUÍ, Y ESO NO ES REDUNDANCIA — ES EL ARREGLO.
     *
     * Antes esto sólo leía, y la configuración vivía dentro de `bpvm_sd_init`.
     * Parecía suficiente porque desde init se llama DESPUÉS de configurar. Pero
     * se llama de otros dos sitios que ocurren ANTES de que init exista: el
     * arranque (¿monto la SD?) y `disk_status`. Ahí se leía un pad que nadie
     * había tocado.
     *
     * Y los pads del RP2350 arrancan en pull-DOWN, así que ese pad lee 0 — que
     * con `cd_activo_bajo` significa "hay tarjeta". O sea que el detector decía
     * SIEMPRE QUE SÍ, y el guardián del arranque («sin tarjeta no se monta») no
     * guardaba nada: con el zócalo vacío intentaba montar igual.
     *
     * Es la MISMA trampa que la MISO de aquí al lado, y por el mismo motivo: un
     * pin sin pull no calla, MIENTE. La lección es que configurar y leer no
     * pueden vivir en sitios distintos — quien lee el pin lo deja usable, y
     * entonces da igual quién llame y en qué orden. Es idempotente y cuesta dos
     * escrituras a registro. */
    bpvm_gpio_init(p->cd, 0);           /* entrada */
    bpvm_gpio_pull(p->cd, 1);           /* pull-UP: sin tarjeta => 1 => "no hay" */

    int v = bpvm_gpio_read(p->cd);
    return p->cd_activo_bajo ? (v == 0) : (v != 0);
}

bpvm_sd_res_t bpvm_sd_init(const bpvm_sd_pines_t* p, int baud_rapido,
                           bpvm_sd_info_t* info)
{
    if (!p || !info || p->cs < 0 || p->sck < 0 || p->mosi < 0 || p->miso < 0)
        return BPVM_SD_E_PINES;
    memset(info, 0, sizeof *info);

    /* CS y card-detect son GPIO normales, no del periférico (ver la cabecera:
     * el SS_N del SPI se levantaría entre tramas y aquí hace falta abajo). */
    bpvm_gpio_init(p->cs, 1);
    cs_alto(p);
    /* El pin `cd` lo configura `bpvm_sd_hay_tarjeta` — UN solo sitio. Tenerlo
     * también aquí es lo que hacía que pareciera correcto: desde init se leía
     * un pin bien puesto, y desde los otros dos llamantes no. */
    if (!bpvm_sd_hay_tarjeta(p)) return BPVM_SD_E_SIN_TARJETA;

    bpvm_spi_init(p->bus, p->sck, p->mosi, p->miso, BAUD_LENTO, 0);

    /* ⚠️ PULL-UP EN MISO, y va DESPUÉS de `spi_init` a propósito.
     *
     * En SPI la línea sólo la tira la tarjeta cuando le toca; el resto del
     * tiempo flota. Y los pads del RP2350 arrancan en pull-DOWN, así que una
     * MISO flotante se lee como 0x00 — que tiene el bit 7 a cero y por tanto
     * PASA POR RESPUESTA VÁLIDA. El resultado sería creerse un R1 = 0x00 que
     * nadie ha enviado. Por eso el pull-up no es una precaución: es lo que hace
     * que "no contesta" se pueda distinguir de "contesta esto".
     *
     * Poner el pull NO desmonta la función SPI del pin: `gpio_pull_up` sólo
     * toca los bits de pull del pad, no el funcsel. */
    bpvm_gpio_pull(p->miso, 1);

    /* 1 — Los 74 pulsos con CS ALTO. La tarjeta los necesita para despertar;
     *     saltárselos es el error clásico que deja el bus mudo. */
    cs_alto(p);
    reloj_en_vacio(p->bus, 10);          /* 80 pulsos */

    /* 2 — CMD0: a estado IDLE. Aquí es donde se ve si el cableado va. */
    cs_bajo(p);
    uint8_t r1 = manda_cmd(p->bus, CMD0_GO_IDLE, 0, 0x95, info);
    if (r1 == 0xFF) { cs_alto(p); return BPVM_SD_E_MUDA; }
    if (r1 != R1_IDLE) { cs_alto(p); return BPVM_SD_E_NO_IDLE; }

    /* 3 — CMD8: ¿es v2 y aguanta 2,7-3,6 V? Se manda 0x1AA y la tarjeta tiene
     *     que devolver el MISMO patrón: es un eco, o sea un control del bus,
     *     no sólo una pregunta. */
    r1 = manda_cmd(p->bus, CMD8_SEND_IF_COND, 0x000001AAu, 0x87, info);
    if (r1 == 0xFF) { cs_alto(p); return BPVM_SD_E_MUDA; }
    int v2 = 0;
    if ((r1 & 0x04) == 0) {              /* comando aceptado => tarjeta v2 */
        uint8_t eco[4];
        for (int i = 0; i < 4; i++) eco[i] = intercambia(p->bus, 0xFF);
        if (eco[2] != 0x01 || eco[3] != 0xAA) { cs_alto(p); return BPVM_SD_E_VOLTAJE; }
        v2 = 1;
    }
    /* Si contestó "comando ilegal" es una v1: no es un fallo, es su respuesta. */

    /* 4 — ACMD41 hasta que salga de IDLE. HCS sólo tiene sentido en v2. */
    int64_t t0 = bpvm_platform_now_ms();
    for (;;) {
        r1 = manda_acmd(p->bus, ACMD41_SEND_OP_COND, v2 ? 0x40000000u : 0, info);
        if (r1 == 0) break;
        if (bpvm_platform_now_ms() - t0 > PLAZO_ARRANQUE_MS) {
            cs_alto(p); return BPVM_SD_E_NO_ARRANCA;
        }
    }

    /* 5 — CMD58 (OCR): el bit CCS dice si direcciona por BLOQUE (alta
     *     capacidad) o por BYTE. Equivocarse aquí lee el sitio equivocado
     *     multiplicado por 512, así que no se supone: se pregunta. */
    info->alta_cap = 0;
    if (v2) {
        r1 = manda_cmd(p->bus, CMD58_READ_OCR, 0, 0xFD, info);
        if (r1 != 0) { cs_alto(p); return BPVM_SD_E_OCR; }
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) ocr[i] = intercambia(p->bus, 0xFF);
        info->alta_cap = (ocr[0] & 0x40) ? 1 : 0;
    }

    /* 6 — Quién es (CID) y cuánto mide (CSD). */
    r1 = manda_cmd(p->bus, CMD10_SEND_CID, 0, 0xFD, info);
    if (r1 != 0 || lee_bloque(p->bus, info->cid, 16) != 0) {
        cs_alto(p); return BPVM_SD_E_CID;
    }
    r1 = manda_cmd(p->bus, CMD9_SEND_CSD, 0, 0xFD, info);
    if (r1 != 0 || lee_bloque(p->bus, info->csd, 16) != 0) {
        cs_alto(p); return BPVM_SD_E_CSD;
    }
    cs_alto(p);
    (void) intercambia(p->bus, 0xFF);    /* que suelte la línea */

    bpvm_sd_cid_desglosar(info->cid, info);
    bpvm_sd_res_t r = bpvm_sd_csd_capacidad(info->csd, &info->bloques,
                                            &info->version);
    if (r != BPVM_SD_OK) return r;

    /* 7 — Y AHORA sí, a velocidad de trabajo. Ni un byte antes. */
    if (baud_rapido > BAUD_LENTO)
        bpvm_spi_init(p->bus, p->sck, p->mosi, p->miso, baud_rapido, 0);
    return BPVM_SD_OK;
}

bpvm_sd_res_t bpvm_sd_leer_bloque(const bpvm_sd_pines_t* p, bpvm_sd_info_t* info,
                                  uint32_t lba, uint8_t dst[512])
{
    if (!p || !info || !dst) return BPVM_SD_E_PINES;

    /* Alta capacidad → el argumento es el BLOQUE. Baja → el BYTE. Confundirlos
     * no da error: lee otro sitio ×512. Por eso sale del OCR y no de una
     * suposición sobre el tamaño de la tarjeta. */
    uint32_t arg = info->alta_cap ? lba : (lba * 512u);

    cs_bajo(p);
    uint8_t r1 = manda_cmd(p->bus, CMD17_READ_SINGLE, arg, 0xFF, info);
    if (r1 != 0) { cs_alto(p); return BPVM_SD_E_LECTURA; }
    if (lee_bloque(p->bus, dst, 512) != 0) { cs_alto(p); return BPVM_SD_E_LECTURA; }
    cs_alto(p);
    (void) intercambia(p->bus, 0xFF);       /* que suelte la línea */
    return BPVM_SD_OK;
}

bpvm_sd_res_t bpvm_sd_escribir_bloque(const bpvm_sd_pines_t* p, bpvm_sd_info_t* info,
                                      uint32_t lba, const uint8_t src[512])
{
    if (!p || !info || !src) return BPVM_SD_E_PINES;
    uint32_t arg = info->alta_cap ? lba : (lba * 512u);

    cs_bajo(p);
    uint8_t r1 = manda_cmd(p->bus, CMD24_WRITE_SINGLE, arg, 0xFF, info);
    if (r1 != 0) { cs_alto(p); return BPVM_SD_E_ESCRITURA; }

    (void) intercambia(p->bus, 0xFF);            /* un hueco antes del token   */
    (void) intercambia(p->bus, TOKEN_DATO);
    for (int i = 0; i < 512; i++) (void) intercambia(p->bus, src[i]);
    (void) intercambia(p->bus, 0xFF);            /* CRC16 apagado, pero va     */
    (void) intercambia(p->bus, 0xFF);

    /* La tarjeta contesta si ACEPTA los datos, y el veredicto está en 5 bits. */
    uint8_t resp = intercambia(p->bus, 0xFF);
    if ((resp & 0x1F) != 0x05) { cs_alto(p); return BPVM_SD_E_ESCRITURA; }

    /* Y AHORA se queda ocupada grabando: tira de MISO a 0 y no lo suelta hasta
     * terminar. Puede ser mucho —si le toca borrar un bloque interno, cientos
     * de ms—, así que el plazo es generoso: si se agota es que algo está roto,
     * no que iba justo. Volver antes de tiempo deja el comando siguiente
     * hablando con una tarjeta sorda, y eso se manifiesta LEJOS de aquí. */
    int64_t t0 = bpvm_platform_now_ms();
    while (intercambia(p->bus, 0xFF) == 0x00) {
        if (bpvm_platform_now_ms() - t0 > PLAZO_OCUPADA_MS) {
            cs_alto(p); return BPVM_SD_E_OCUPADA;
        }
    }
    cs_alto(p);
    (void) intercambia(p->bus, 0xFF);
    return BPVM_SD_OK;
}

const char* bpvm_sd_res_str(bpvm_sd_res_t r)
{
    switch (r) {
    case BPVM_SD_OK:             return "OK";
    case BPVM_SD_E_PINES:        return "pines de la SD sin configurar";
    case BPVM_SD_E_SIN_TARJETA:  return "el zocalo esta vacio (card-detect)";
    case BPVM_SD_E_MUDA:         return "la tarjeta no contesta (cableado o alimentacion)";
    case BPVM_SD_E_NO_IDLE:      return "contesta pero no entra en IDLE";
    case BPVM_SD_E_VOLTAJE:      return "CMD8: voltaje no aceptado o eco incorrecto";
    case BPVM_SD_E_NO_ARRANCA:   return "no termina de arrancar (ACMD41 agoto el plazo)";
    case BPVM_SD_E_OCR:          return "no se pudo leer el OCR (CMD58)";
    case BPVM_SD_E_CID:          return "no se pudo leer el CID (CMD10)";
    case BPVM_SD_E_CSD:          return "no se pudo leer el CSD (CMD9)";
    case BPVM_SD_E_CSD_VER:      return "el CSD es de una version que no sabemos leer";
    case BPVM_SD_E_LECTURA:      return "arranco pero no entrega datos (CMD17)";
    case BPVM_SD_E_ESCRITURA:    return "no acepta los datos al escribir (CMD24)";
    case BPVM_SD_E_OCUPADA:      return "acepto los datos pero no termina de grabar";
    }
    return "desconocido";
}
