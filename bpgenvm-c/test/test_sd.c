/*
 * test_sd.c — V5/H1: los decodificadores del CSD y el CID de una tarjeta SD.
 *
 * QUÉ SE PRUEBA Y POR QUÉ ÉSTO. El diálogo con la tarjeta necesita tarjeta, pero
 * la parte donde de verdad se esconden los errores es PURA: sacar la capacidad
 * del CSD son dos fórmulas distintas (v1 y v2) con campos partidos entre bytes,
 * y equivocarse ahí no revienta — devuelve un número. Una tarjeta de 32 GB que
 * dice tener 4 pasa desapercibida hasta que el FS escribe fuera.
 *
 * Por eso los CSD de aquí no son inventados: están construidos campo a campo con
 * el layout del estándar y su capacidad calculada A MANO en el comentario, para
 * que la prueba compare contra una cuenta independiente y no contra el propio
 * código.
 *
 *   make test-sd
 */
#include "bpvm_sd.h"
#include <stdio.h>
#include <string.h>

/* Tapón del reloj. Este test NO ejercita `bpvm_sd_init` —eso necesita tarjeta—,
 * pero su código vive en el mismo .c y el enlazador pide el símbolo. Enlazar el
 * `platform_pthread.c` de verdad arrastraría el alocador de la VM entera para
 * probar dos funciones puras, así que se tapa aquí. Y se deja a CERO a
 * propósito: si algún día alguien mete en este test una llamada que espere
 * tiempo, se colgará en el acto en vez de pasar por casualidad. */
int64_t bpvm_platform_now_ms(void) { return 0; }

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

int main(void)
{
    printf("== V5/H1: decodificadores de CSD/CID ==\n\n");

    /* ── CSD v2 (SDHC/SDXC) ──────────────────────────────────────────────
     * C_SIZE vive en los bits [69:48], o sea:
     *     [69:64] -> 6 bits bajos de csd[7]
     *     [63:56] -> csd[8]
     *     [55:48] -> csd[9]
     * Con csd[7]=0x00, csd[8]=0xF3, csd[9]=0xFF -> C_SIZE = 0x00F3FF = 62463.
     * Capacidad = (62463 + 1) * 512 KB = 62464 * 1024 bloques = 63.963.136,
     * que por 512 son 32.749.125.632 B — una tarjeta "de 32 GB" de verdad. */
    {
        uint8_t csd[16] = { 0x40, 0x0E, 0x00, 0x32, 0x5B, 0x59, 0x00, 0x00,
                            0xF3, 0xFF, 0x7F, 0x80, 0x0A, 0x40, 0x00, 0x01 };
        uint32_t bloques = 0; uint8_t ver = 0;
        bpvm_sd_res_t r = bpvm_sd_csd_capacidad(csd, &bloques, &ver);
        CHECK(r == BPVM_SD_OK,            "v2: el CSD se acepta");
        CHECK(ver == 2,                   "v2: se reconoce como version 2");
        CHECK(bloques == 63963136u,       "v2: 63.963.136 bloques (32,7 GB)");
    }

    /* ── CSD v1 (SDSC), el caso con TRES campos y una potencia ────────────
     * READ_BL_LEN = [83:80] = csd[5] & 0x0F   -> 0x5A da 10
     * C_SIZE      = [73:62]                   -> 3751
     * C_SIZE_MULT = [49:47]                   -> 7
     * bytes = (3751+1) * 2^(7+2) * 2^10 = 3752 * 512 * 1024 = 1.967.128.576
     * bloques de 512 = 3.842.048.  (Una tarjeta "de 2 GB".) */
    {
        uint8_t csd[16] = { 0x00, 0x2D, 0x0F, 0x59, 0x5B, 0x5A, 0xBF, 0xA9,
                            0xCF, 0x87, 0xC0, 0x40, 0x40, 0x00, 0x00, 0x01 };
        uint32_t bloques = 0; uint8_t ver = 0;
        bpvm_sd_res_t r = bpvm_sd_csd_capacidad(csd, &bloques, &ver);
        CHECK(r == BPVM_SD_OK,            "v1: el CSD se acepta");
        CHECK(ver == 1,                   "v1: se reconoce como version 1");
        CHECK(bloques == 3842048u,        "v1: 3.842.048 bloques (1,97 GB)");
    }

    /* ── v1 con READ_BL_LEN = 9, que es el caso donde el desplazamiento
     * (read_bl_len - 9) vale CERO. Si se hubiera escrito al reves, aqui
     * saldria la mitad o el doble y en el caso de arriba no. ──────────── */
    {
        /* Mismo CSD, cambiando solo READ_BL_LEN de 10 a 9 (csd[5] 0x5A->0x59).
         * Con un bloque de lectura la mitad, la capacidad es la MITAD:
         * 3.842.048 / 2 = 1.921.024 bloques. */
        uint8_t csd[16] = { 0x00, 0x2D, 0x0F, 0x59, 0x5B, 0x59, 0xBF, 0xA9,
                            0xCF, 0x87, 0xC0, 0x40, 0x40, 0x00, 0x00, 0x01 };
        uint32_t bloques = 0; uint8_t ver = 0;
        bpvm_sd_res_t r = bpvm_sd_csd_capacidad(csd, &bloques, &ver);
        CHECK(r == BPVM_SD_OK,            "v1/bl9: se acepta");
        CHECK(bloques == 1921024u,        "v1/bl9: la MITAD, 1.921.024 bloques");
    }

    /* ── Lo que NO se sabe leer se NIEGA, no se adivina ────────────────── */
    {
        uint8_t csd[16]; memset(csd, 0, sizeof csd);
        csd[0] = 0x80;                    /* CSD_STRUCTURE = 2 -> SDUC */
        uint32_t bloques = 12345u; uint8_t ver = 9;
        bpvm_sd_res_t r = bpvm_sd_csd_capacidad(csd, &bloques, &ver);
        CHECK(r == BPVM_SD_E_CSD_VER,     "SDUC (v3): se NIEGA con nombre");
        CHECK(bloques == 12345u,          "SDUC: y no toca la capacidad del llamante");

        csd[0] = 0x00; csd[5] = 0x5F;     /* READ_BL_LEN = 15, fuera de rango */
        r = bpvm_sd_csd_capacidad(csd, &bloques, &ver);
        CHECK(r == BPVM_SD_E_CSD_VER,     "READ_BL_LEN absurdo: tambien se niega");
    }

    /* ── CID: campos empaquetados, y el MDT partido en 8+4 bits ────────── */
    {
        uint8_t cid[16] = { 0x03, 'S', 'D', 'S', 'U', '3', '2', 'G',
                            0x80, 0x12, 0x34, 0x56, 0x78, 0x01, 0x59, 0x01 };
        bpvm_sd_info_t info; memset(&info, 0, sizeof info);
        bpvm_sd_cid_desglosar(cid, &info);
        CHECK(info.fabricante == 0x03,        "CID: MID = 0x03 (SanDisk)");
        CHECK(strcmp(info.oem, "SD") == 0,    "CID: OEM = \"SD\"");
        CHECK(strcmp(info.producto, "SU32G") == 0, "CID: producto = \"SU32G\"");
        CHECK(info.rev_mayor == 8 && info.rev_menor == 0, "CID: revision 8.0");
        CHECK(info.serie == 0x12345678u,      "CID: numero de serie");
        /* MDT = ((0x01 & 0x0F) << 8) | 0x59 = 0x159 = 345
         *   anno = 2000 + (345 >> 4) = 2000 + 21 = 2021 ; mes = 345 & 15 = 9 */
        CHECK(info.anno == 2021,              "CID: anno 2021 (MDT desplazado 4)");
        CHECK(info.mes  == 9,                 "CID: mes 9");
    }

    /* ── La entrada `sd` del ENV: UNA linea con etiquetas ───────────────── */
    {
        bpvm_sd_pines_t p; char m[64];
        int r = bpvm_sd_pines_parse("sck:34,mosi:35,miso:36,cs:39,cd:40",
                                    &p, m, sizeof m);
        CHECK(r == 0, "env: la linea de la Metro se acepta");
        CHECK(p.sck == 34 && p.mosi == 35 && p.miso == 36 && p.cs == 39,
                                          "env: los cuatro de SPI, en su sitio");
        CHECK(p.cd == 40,                 "env: card-detect leido");
        CHECK(p.bus == 0,                 "env: bus por defecto = 0");
        CHECK(p.cd_activo_bajo == 1,      "env: card-detect activo bajo por defecto");
    }
    {   /* El ORDEN da igual — que es justo para lo que estan las etiquetas. */
        bpvm_sd_pines_t p;
        int r = bpvm_sd_pines_parse("cd:40, cs:39 ,miso:36,sck:34,mosi:35", &p, 0, 0);
        CHECK(r == 0 && p.sck == 34 && p.cs == 39 && p.cd == 40,
                                          "env: el orden no importa (ni los espacios)");
    }
    {   /* Y lo que de verdad justifica la decision: el error DICE cual falta. */
        bpvm_sd_pines_t p; char m[64];
        int r = bpvm_sd_pines_parse("sck:34,mosi:35,miso:36,cd:40", &p, m, sizeof m);
        CHECK(r == -1,                    "env: sin 'cs' se rechaza");
        CHECK(strstr(m, "cs") != NULL,    "env: y el mensaje NOMBRA el que falta");
    }
    {   /* `cd` es opcional: sin el, la placa no tiene detector y se dice. */
        bpvm_sd_pines_t p;
        int r = bpvm_sd_pines_parse("sck:34,mosi:35,miso:36,cs:39", &p, 0, 0);
        CHECK(r == 0 && p.cd < 0,         "env: sin 'cd' vale, y queda a -1");
        CHECK(bpvm_sd_hay_tarjeta(&p) == 1,
                                          "env: sin detector NO se afirma que falte tarjeta");
    }
    {   /* Tolerante con lo que no conoce: un firmware viejo no rechaza una
         * linea escrita para uno nuevo. */
        bpvm_sd_pines_t p;
        int r = bpvm_sd_pines_parse("sck:34,mosi:35,miso:36,cs:39,turbo:9", &p, 0, 0);
        CHECK(r == 0 && p.sck == 34,      "env: una clave desconocida se ignora");
    }
    {   /* Casos que tienen que fallar CON motivo, no en silencio. */
        bpvm_sd_pines_t p; char m[64];
        CHECK(bpvm_sd_pines_parse("", &p, m, sizeof m) == -1 && m[0],
                                          "env: vacia -> rechazo con motivo");
        CHECK(bpvm_sd_pines_parse("sck:x,mosi:35,miso:36,cs:39", &p, m, sizeof m) == -1,
                                          "env: valor no numerico -> rechazo");
        CHECK(bpvm_sd_pines_parse(0, &p, m, sizeof m) == -1,
                                          "env: NULL -> rechazo, sin reventar");
    }
    {   /* Y que `cd` no se coma a `cdalto`, que empieza igual. */
        bpvm_sd_pines_t p;
        int r = bpvm_sd_pines_parse("sck:34,mosi:35,miso:36,cs:39,cdalto:1", &p, 0, 0);
        CHECK(r == 0 && p.cd_activo_bajo == 0,
                                          "env: 'cdalto' no lo captura 'cd'");
    }

    /* ── Los peldaños tienen texto, TODOS. Un chivato mudo no sirve. ───── */
    {
        int mudos = 0;
        for (int i = 0; i <= (int) BPVM_SD_E_CSD_VER; i++) {
            const char* s = bpvm_sd_res_str((bpvm_sd_res_t) i);
            if (!s || !*s || strcmp(s, "desconocido") == 0) mudos++;
        }
        CHECK(mudos == 0, "todos los peldanos tienen texto propio");
    }

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
