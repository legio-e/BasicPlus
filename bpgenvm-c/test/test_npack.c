/*
 * test_npack.c — V5/I: la escalera de validación del pack nativo. Y desde
 * V7/H6.c (G1), el PLAN: varios motores en la zona, cada uno con su trozo de RAM.
 *
 * Lo que se prueba NO es que un pack bueno pase — eso es lo fácil. Es que CADA
 * forma de estar mal se detecte ANTES de saltar y se distinga de las demás:
 * "no cabe" y "es de otra arquitectura" llevan a sitios distintos, y un cargador
 * que sólo dijera "no se pudo" obligaría a adivinar.
 *
 * Requisito de Eduardo: *"chivatos que nos digan qué es lo que no funciona, para
 * saber dónde y no sólo funciona/no funciona"*.
 *
 *   make test-npack
 */
#include "bpvm_npack.h"
#include "bpvm_pack.h"
#include "mdn_loader.h"
#include "bpvm_bmgr_wire.h"   /* V7/H7 (Boot 2, fase 2): el STATE con el «casi» de un motor */
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Direcciones REALES de la Metro, leidas de su INFO. */
#define AQUI_FLASH   0x10BC4000u
#define AQUI_RAM     0x11000000u
#define SITIO_FLASH  (4u * 1024u * 1024u)
#define SITIO_RAM    (2u * 1024u * 1024u)

/* Cabecera BUENA. Cada caso la estropea de UNA manera: así se sabe qué causó
 * qué (si un caso cambiara dos cosas, su resultado no diría cuál fue). */
static bpvm_npack_hdr_t buena(void) {
    bpvm_npack_hdr_t h;
    memset(&h, 0, sizeof h);
    h.magic = BPVM_NPACK_MAGIC;
    h.format = BPVM_NPACK_FORMAT;
    h.arch = bpvm_mdn_host_arch();
    snprintf(h.float_abi, sizeof h.float_abi, "%s", bpvm_mdn_host_float_abi());
    h.entry_off = 0;
    h.flags = BPVM_NPACK_F_THUMB;
    h.flash_bytes = 190;          /* los del pack minimo real */
    h.data_bytes = 0;
    h.bss_bytes = 8;
    h.linked_flash = AQUI_FLASH;
    h.linked_ram = AQUI_RAM;
    h.reloc_count = 0;            /* ya realojado */
    return h;
}

static bpvm_npack_res_t chk(const bpvm_npack_hdr_t* h, const char* bios_falta) {
    return bpvm_npack_escalera(h, AQUI_FLASH, SITIO_FLASH,
                               AQUI_RAM, AQUI_RAM + SITIO_RAM, bios_falta);
}

/* ─────────── Una ZONA de verdad, con packs BPAK armados aquí ───────────
 *
 * El plan recorre la CADENA, así que no basta con dejar cabeceras de motor
 * sueltas entre basura (como hacía el test del barrido): hacen falta packs con
 * su cabecera, su CRC y sus entradas. Se arman con el formato de
 * `pack/PackFormat.java` — y si algo de aquí no cuadrara con el lector de
 * `bpvm_pack.c`, el recorrido no vería ningún pack y los casos de abajo
 * saldrían en rojo: el armador se prueba solo. */
#define BLOQUE    4096u
#define N_BLOQUES 16u
static union { uint8_t b[BLOQUE * N_BLOQUES]; uint32_t alinea; } g_zona;
static uint32_t g_fin;              /* dónde irá el siguiente pack */

static void be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t) (v >> 24); p[1] = (uint8_t) (v >> 16);
    p[2] = (uint8_t) (v >> 8);  p[3] = (uint8_t) v;
}
static void be16(uint8_t* p, uint16_t v) { p[0] = (uint8_t) (v >> 8); p[1] = (uint8_t) v; }

static void zona_vacia(void) { memset(g_zona.b, 0xFF, sizeof g_zona.b); g_fin = 0; }

/* Dónde empezará el CÓDIGO del motor del siguiente pack que se arme: su
 * `aqui_flash`, que es contra lo que hay que sellarlo. Cabecera del pack (128)
 * + cabecera de la entrada (48) + cabecera del motor (64). */
static uint32_t codigo_siguiente(void) {
    return (uint32_t) (uintptr_t) (g_zona.b + g_fin + 128u + 48u) + BPVM_NPACK_HDR_BYTES;
}

/* Arma un pack con una entrada `npk` (la cabecera `h` y ceros detrás) y lo pone
 * al final de la cadena, midiendo los bloques que haga falta. `recorta` != 0
 * fuerza la longitud de la entrada (para el motor roto). Devuelve su offset. */
static uint32_t arma_(const char* pack, int vivo, const char* npk,
                      const bpvm_npack_hdr_t* h, uint32_t cuerpo, uint32_t recorta) {
    uint32_t off = g_fin;
    uint8_t* p = g_zona.b + off;
    uint32_t len = recorta ? recorta : BPVM_NPACK_HDR_BYTES + cuerpo;
    uint32_t fin_contenido = 128u + 48u + ((len + 3u) & ~3u);
    /* + 4: el centinela de fin (tipo 0xFFFFFFFF) tiene que caber detrás. */
    uint32_t total = ((fin_contenido + 4u + BLOQUE - 1u) / BLOQUE) * BLOQUE;
    if (off + total > sizeof g_zona.b) {
        printf("  FAIL: la zona de prueba no da para '%s'\n", pack);
        g_fail++;
        return off;
    }
    memset(p, 0xFF, total);
    memset(p, 0, 128);
    be32(p + 0, BPVM_PACK_MAGIC);
    be16(p + 4, 1);
    be16(p + 6, (uint16_t) (vivo ? 0xFFFF : 0xFFFE));
    be32(p + 8, total);
    strncpy((char*) p + 12, pack, 32);
    be32(p + 44, 1700000000u);
    strncpy((char*) p + 48, "1.0", 16);

    uint8_t* e = p + 128;
    memset(e, 0, 48);
    strncpy((char*) e + 0, "npk", 4);
    strncpy((char*) e + 4, npk, 32);
    be32(e + 36, len);
    memset(e + 48, 0, (len + 3u) & ~3u);
    memcpy(e + 48, h, (len < sizeof *h) ? len : sizeof *h);

    be16(p + 64, bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, p + 128, fin_contenido - 128u));
    uint16_t crc = bpvm_pack_crc16(BPVM_PACK_CRC16_INIT, p, 6);
    crc = bpvm_pack_crc16(crc, p + 8, 126 - 8);
    be16(p + 126, crc);

    g_fin += total;
    return off;
}
static uint32_t arma(const char* pack, int vivo, const char* npk,
                     const bpvm_npack_hdr_t* h, uint32_t cuerpo) {
    return arma_(pack, vivo, npk, h, cuerpo, 0);
}

/* Un motor listo para ir en el siguiente pack: sellado donde caerá su código y
 * con la RAM que se le diga. `data`/`bss` son los de SQLite y LVGL medidos. */
static bpvm_npack_hdr_t motor(uint32_t ram, uint32_t data, uint32_t bss) {
    bpvm_npack_hdr_t h = buena();
    h.flash_bytes  = 190;
    h.data_bytes   = data;
    h.bss_bytes    = bss;
    h.linked_flash = codigo_siguiente();
    h.linked_ram   = ram;
    return h;
}

#define RAM_BASE   0x48000000u         /* un bloque de 512 KB, como el de la C5 */
#define RAM_TAM    (512u * 1024u)
#define SQL_DATA   6916u               /* sqlite_rv.npack, medido el 12-ago */
#define SQL_BSS    1368u
#define SQL_RAM    8288u               /* 8 284 redondeado a 16 */
#define GUI_DATA   1024u               /* el de la GUI: pool + modelo + búferes */
#define GUI_BSS    (392u * 1024u)
#define GUI_RAM    ((GUI_DATA + GUI_BSS + 15u) & ~15u)

/* V7/H7 (Boot 2, fase 2) — el log del cargador común, a un búfer. */
static char g_log[4096];
static void captura(const char* l) {
    size_t n = strlen(g_log);
    snprintf(g_log + n, sizeof g_log - n, "%s\n", l);
}

static int plan(bpvm_npack_carga_t* c, bpvm_npack_plan_t* p) {
    return bpvm_npack_planear(g_zona.b, sizeof g_zona.b, RAM_BASE, RAM_TAM, 0,
                              c, BPVM_NPACK_MAX, p);
}

int main(void) {
    printf("== escalera de validacion del pack nativo (V5/I) ==\n");

    /* ── El caso bueno ── */
    { bpvm_npack_hdr_t h = buena();
      CHECK(chk(&h, 0) == BPVM_NPACK_OK, "pack correcto -> OK"); }

    /* ── Peldaño 1: ¿es un pack? ── */
    CHECK(chk(0, 0) == BPVM_NPACK_E_MAGIC, "cabecera NULL -> MAGIC (sin reventar)");
    { bpvm_npack_hdr_t h = buena(); h.magic = 0xDEADBEEF;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_MAGIC, "magic malo -> MAGIC"); }
    { bpvm_npack_hdr_t h = buena(); h.format = 99;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_FORMAT, "formato futuro -> FORMAT (no MAGIC)"); }

    /* ── Peldaño 2: la ISA ── */
    { bpvm_npack_hdr_t h = buena(); h.arch = (uint16_t)(bpvm_mdn_host_arch() + 1);
      CHECK(chk(&h, 0) == BPVM_NPACK_E_ARCH, "otra arquitectura -> ARCH"); }

    /* ── Peldaño 3: la ABI de coma flotante ──
     * La traicionera: no da error de nada, da numeros mal. Y `arch` sola NO la
     * distingue — por eso es un peldaño aparte y no parte del anterior. */
    { bpvm_npack_hdr_t h = buena(); snprintf(h.float_abi, sizeof h.float_abi, "hard");
      CHECK(chk(&h, 0) == BPVM_NPACK_E_ABI,
            "misma arch pero OTRA ABI de coma flotante -> ABI"); }
    /* Una ABI PARECIDA pero distinta. Se construye A PARTIR de la del anfitrion
     * para que el caso valga en cualquiera.
     *
     * La version anterior de este caso ponia la ABI VACIA y esperaba que fallara
     * — daba por supuesto que el test corria en ARM. En el PC la ABI del host es
     * "" (x86 no tiene pack nativo que sellar), asi que "" COINCIDIA y pasaba
     * bien. Es decir: en el PC este peldano no puede fallar, y el rojo era del
     * test, no del cargador. Donde importa —la placa— la ABI si tiene nombre. */
    { bpvm_npack_hdr_t h = buena();
      snprintf(h.float_abi, sizeof h.float_abi, "x%s", bpvm_mdn_host_float_abi());
      CHECK(chk(&h, 0) == BPVM_NPACK_E_ABI,
            "ABI parecida pero DISTINTA -> ABI (la comparacion es exacta)"); }

    /* ── Peldaño 4: EL SELLO ──
     * El caso REAL: alguien cambia SQLite=<MB> o packRam=<KB>, el bloque de RAM
     * se mueve o encoge, y el pack sigue grabado con los punteros viejos.
     *
     * V7/H6.c G1: en RAM el sello ya no tiene que ser la BASE del bloque sino
     * caer DENTRO — el segundo motor va detrás del primero. Así que el caso de
     * antes («1 MB más allá de la base») ahora es BUENO, y lo que falla es
     * salirse del bloque por arriba o por abajo. */
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM + 0x100000;
      CHECK(chk(&h, 0) == BPVM_NPACK_OK,
            "sello de RAM DENTRO del bloque pero no en su base -> OK (un segundo motor)"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM + SITIO_RAM + 0x1000;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SELLO,
            "sello de RAM por ENCIMA del bloque (encogio packRam=) -> SELLO"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM - 0x1000;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SELLO,
            "sello de RAM por DEBAJO del bloque (se movio) -> SELLO"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_flash = AQUI_FLASH - 0x1000;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SELLO, "grabado en otro offset -> SELLO"); }

    /* ── Peldaño 5: ¿pasó por el IDE? ── */
    { bpvm_npack_hdr_t h = buena(); h.reloc_count = 4;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SIN_RELOC,
            "trae su tabla de relocs -> SIN_RELOC (no paso por el IDE)"); }

    /* EL CASO DE VERDAD (V5/H8, 12-ago): un pack recien construido y SIN
     * realojar no tiene sello puesto — las dos cosas fallan a la vez. El test
     * de arriba no lo cubria: partia de una cabecera con el sello BUENO, que
     * es una combinacion que no se da en la naturaleza.
     *
     * Con el orden anterior (sello antes que reloc_count) esto contestaba
     * "realojado para OTRA direccion", que es MENTIRA: no se ha realojado
     * nunca. Los dos motivos mandan al IDE, asi que el fallo no se notaba en el
     * resultado — solo en el diagnostico, que es justo lo que esta escalera
     * existe para dar.
     *
     * Y con el modelo de H8 deja de ser un caso de laboratorio: un pack
     * DISTRIBUIDO viene asi, y el IDE lo realoja al grabar. */
    { bpvm_npack_hdr_t h = buena();
      h.reloc_count  = 4;      /* trae su tabla                              */
      h.linked_flash = 0;      /* y NO tiene sello: nadie le ha dado sitio    */
      h.linked_ram   = 0;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SIN_RELOC,
            "sin realojar Y sin sello -> SIN_RELOC (no SELLO: la causa es esa)"); }

    /* ── Peldaño 6: tamaños ── */
    { bpvm_npack_hdr_t h = buena(); h.flash_bytes = SITIO_FLASH + 1;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO, "mas grande que la zona -> TAMANO"); }
    { bpvm_npack_hdr_t h = buena(); h.data_bytes = SITIO_FLASH - h.flash_bytes + 1;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "el codigo cabe pero su imagen de .data se sale de la entrada -> TAMANO"); }
    { bpvm_npack_hdr_t h = buena(); h.bss_bytes = SITIO_RAM;
      CHECK(chk(&h, 0) == BPVM_NPACK_OK,
            ".data+.bss ocupan EXACTAMENTE la RAM -> cabe (el borde entra)"); }
    { bpvm_npack_hdr_t h = buena(); h.bss_bytes = SITIO_RAM + 1;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "UN BYTE mas de RAM de la que hay -> TAMANO"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM + SITIO_RAM - 8; h.bss_bytes = 8;
      CHECK(chk(&h, 0) == BPVM_NPACK_OK,
            "un motor que acaba JUSTO en el final del bloque -> cabe"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM + SITIO_RAM - 8; h.bss_bytes = 16;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "el mismo, con 8 bytes mas de .bss -> TAMANO (no escribe fuera)"); }
    { bpvm_npack_hdr_t h = buena(); h.data_bytes = 0xFFFFFFF0u; h.bss_bytes = 0x20;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "cabecera rota con tamanos que DESBORDAN la suma -> TAMANO (no pasa por pequena)"); }
    { bpvm_npack_hdr_t h = buena(); h.entry_off = h.flash_bytes;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO, "entrada FUERA del codigo -> TAMANO"); }
    { bpvm_npack_hdr_t h = buena(); h.entry_off = 3;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "entrada IMPAR (ya sin el bit de modo) -> TAMANO"); }

    /* ── Peldaño 7: la BIOS ── */
    { bpvm_npack_hdr_t h = buena();
      CHECK(chk(&h, "memcpy") == BPVM_NPACK_E_BIOS, "BIOS con huecos -> BIOS"); }

    /* ── El ORDEN importa: lo barato y lo grave, primero ──
     * Con magic malo NO se debe mirar la arch: la cabecera podria ser basura y
     * cualquier campo que se lea de ella es ruido. */
    { bpvm_npack_hdr_t h = buena();
      h.magic = 0; h.arch = 0xFFFF; h.linked_ram = 0; h.reloc_count = 7;
      CHECK(chk(&h, "memcpy") == BPVM_NPACK_E_MAGIC,
            "todo mal a la vez -> MAGIC (lo barato primero, no se lee basura)"); }

    /* ── La direccion de salto: el bit Thumb ──
     * Vive en UNA funcion porque olvidarlo es hard fault inmediato. */
    { bpvm_npack_hdr_t h = buena(); h.entry_off = 0x40;
      CHECK(bpvm_npack_entry_addr(&h, AQUI_FLASH) == (AQUI_FLASH + 0x40 + 1),
            "entrada Thumb -> la direccion lleva el bit 0 PUESTO"); }
    { bpvm_npack_hdr_t h = buena(); h.entry_off = 0x40; h.flags = 0;
      CHECK(bpvm_npack_entry_addr(&h, AQUI_FLASH) == (AQUI_FLASH + 0x40),
            "entrada NO Thumb -> sin el bit (ARM puro / RISC-V)"); }

    /* ── Lo que un motor ocupa de RAM ── */
    { bpvm_npack_hdr_t h = buena(); h.data_bytes = SQL_DATA; h.bss_bytes = SQL_BSS;
      CHECK(bpvm_npack_ram_bytes(&h) == SQL_RAM,
            "SQLite: 6916 + 1368 = 8284 -> 8288 (redondeado a 16)"); }
    { bpvm_npack_hdr_t h = buena(); h.data_bytes = 0; h.bss_bytes = 0;
      CHECK(bpvm_npack_ram_bytes(&h) == 0, "un motor sin estaticos ocupa 0"); }
    { bpvm_npack_hdr_t h = buena(); h.data_bytes = 0xFFFFFFF0u; h.bss_bytes = 0x20;
      CHECK(bpvm_npack_ram_bytes(&h) == 0xFFFFFFF0u,
            "tamanos que desbordan -> el maximo, no un numero pequeno"); }

    /* ── EL REPARTO DE LA CABECERA, campo a campo ──
     *
     * Esta cabecera la ESCRIBE el empaquetador (Java, en el PC) y la LEE el
     * cargador (C, en la placa). Son dos lenguajes distintos leyendo los mismos
     * bytes: el reparto es un CONTRATO, y un contrato que no está escrito en
     * ningún sitio se rompe en silencio — el pack se leería con los campos
     * corridos y el primer sintoma seria un salto a ninguna parte.
     *
     * Se fija por los dos lados: aqui los offsets REALES que dice el compilador,
     * y en el lado PC un assert sobre el tamaño de su formato. Si alguien
     * reordena un campo, este test dice CUÁL — igual que la tabla de la BIOS. */
    {
        static const struct { size_t off; const char* nombre; } CAMPOS[] = {
            {  0, "magic"        }, {  4, "format"      }, {  6, "arch"        },
            {  8, "float_abi"    }, { 16, "entry_off"   }, { 20, "flags"       },
            { 24, "flash_bytes"  }, { 28, "data_bytes"  }, { 32, "bss_bytes"   },
            { 36, "linked_flash" }, { 40, "linked_ram"  }, { 44, "reloc_count" },
            { 48, "reserved"     },
        };
        /* offsetof en el MISMO orden que la tabla. */
        const size_t REAL[] = {
            offsetof(bpvm_npack_hdr_t, magic),        offsetof(bpvm_npack_hdr_t, format),
            offsetof(bpvm_npack_hdr_t, arch),         offsetof(bpvm_npack_hdr_t, float_abi),
            offsetof(bpvm_npack_hdr_t, entry_off),    offsetof(bpvm_npack_hdr_t, flags),
            offsetof(bpvm_npack_hdr_t, flash_bytes),  offsetof(bpvm_npack_hdr_t, data_bytes),
            offsetof(bpvm_npack_hdr_t, bss_bytes),    offsetof(bpvm_npack_hdr_t, linked_flash),
            offsetof(bpvm_npack_hdr_t, linked_ram),   offsetof(bpvm_npack_hdr_t, reloc_count),
            offsetof(bpvm_npack_hdr_t, reserved),
        };
        int n = (int)(sizeof CAMPOS / sizeof CAMPOS[0]), movidos = 0;
        for (int i = 0; i < n; i++)
            if (REAL[i] != CAMPOS[i].off) {
                printf("  FAIL: el campo '%s' se movio: %u esperado, %u real\n",
                       CAMPOS[i].nombre, (unsigned) CAMPOS[i].off, (unsigned) REAL[i]);
                movidos++;
            }
        CHECK(movidos == 0, "los 13 campos de la cabecera, en su sitio");
        /* Y el TAMAÑO, que además es la base del código: la imagen empieza en
         * `pack_base + BPVM_NPACK_HDR_BYTES`. Si la struct creciera sin tocar la
         * constante, el cargador saltaria 64 B mas alla del principio del codigo. */
        CHECK(sizeof(bpvm_npack_hdr_t) == BPVM_NPACK_HDR_BYTES,
              "la cabecera mide exactamente BPVM_NPACK_HDR_BYTES (64 B)");
    }

    /* ── Cada motivo tiene texto, y son distintos ── */
    {
        int distintos = 1;
        for (int i = 0; i <= BPVM_NPACK_E_BIOS; i++)
            for (int j = i + 1; j <= BPVM_NPACK_E_BIOS; j++)
                if (strcmp(bpvm_npack_res_str((bpvm_npack_res_t) i),
                           bpvm_npack_res_str((bpvm_npack_res_t) j)) == 0)
                    distintos = 0;
        CHECK(distintos, "los 9 motivos tienen texto y NINGUNO se repite");
    }


    /* ─────────── V7/H6.c G1 — VARIOS MOTORES EN LA ZONA ───────────
     *
     * El plan recorre la cadena y decide, para cada motor, si se salta a él y
     * qué trozo de RAM es suyo. Lo que se prueba son las formas en que DOS
     * motores pueden estorbarse: pisarse la RAM, llamarse igual, quedar uno
     * muerto debajo del otro. Y el TOPE, que es a la vez dónde irá el siguiente
     * que se grabe y dónde empieza la arena: si se calculara mal, el IDE
     * sellaría encima de otro motor o la arena empezaría dentro de él. */
    printf("\n== el plan: varios motores en la zona (V7/H6.c G1) ==\n");
    bpvm_npack_carga_t c[BPVM_NPACK_MAX];
    bpvm_npack_plan_t  p;

    /* Zona VIRGEN: nada que cargar, y el siguiente irá a la base. */
    zona_vacia();
    CHECK(plan(c, &p) == 0 && p.cargables == 0, "zona borrada -> 0 motores");
    CHECK(p.ram_tope == RAM_BASE, "y el tope es la BASE del bloque");
    CHECK(bpvm_npack_ram_tope(g_zona.b, sizeof g_zona.b, 0, 0) == 0,
          "sin bloque de RAM -> tope 0 (el IDE dira 'esta placa no da RAM')");
    CHECK(bpvm_npack_planear(0, 0, RAM_BASE, RAM_TAM, 0, c, BPVM_NPACK_MAX, &p) == 0
          && p.ram_tope == RAM_BASE, "zona NULL -> 0 motores, sin reventar");

    /* UNO: SQLite, sellado en la base (lo que daban las imagenes de antes de G1
     * y lo que da ahora la primera vez). Es la placa de HOY: sigue valiendo. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    CHECK(plan(c, &p) == 1 && p.cargables == 1 && bpvm_npack_cargable(&c[0]),
          "UN motor sellado en la base -> se carga (lo ya grabado sigue valiendo)");
    CHECK(strcmp(c[0].pack, "SQLite") == 0 && strcmp(c[0].npk, "sqlite") == 0,
          "y se sabe su pack y su entrada (para el log y para la arena)");
    CHECK(c[0].ram == RAM_BASE && c[0].ram_bytes == SQL_RAM, "su trozo: [base, base+8288)");
    CHECK(p.ram_tope == RAM_BASE + SQL_RAM,
          "el tope sube a base+8288: el SIGUIENTE ira detras, y la arena empieza ahi");
    CHECK(c[0].codigo == c[0].hdr->linked_flash,
          "el codigo esta donde dice su sello (cabecera del pack + entrada + la suya)");

    /* DOS: la GUI grabada DESPUÉS, con el sello que le dio la placa (el tope). */
    { bpvm_npack_hdr_t h = motor(RAM_BASE + SQL_RAM, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl", &h, 190 + GUI_DATA); }
    CHECK(plan(c, &p) == 2 && p.cargables == 2,
          "DOS motores, el segundo detras del primero -> se cargan LOS DOS");
    CHECK(strcmp(c[0].npk, "sqlite") == 0 && strcmp(c[1].npk, "lvgl") == 0,
          "en orden de cadena: el orden en que se grabaron");
    CHECK(p.ram_tope == RAM_BASE + SQL_RAM + GUI_RAM, "y el tope, encima de los dos");

    /* El de ANTES DE G1: la imagen vieja daba a todos la base, y dos sellos
     * solapados cargados a la vez se machacarían la .bss uno al otro EN
     * SILENCIO. El primero que se grabó se queda su trozo. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    { bpvm_npack_hdr_t h = motor(RAM_BASE, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl", &h, 190 + GUI_DATA); }
    CHECK(plan(c, &p) == 2 && p.cargables == 1, "dos sellos SOLAPADOS -> se carga UNO");
    CHECK(bpvm_npack_cargable(&c[0]) && c[1].pisa_a == 0,
          "el primero se queda su RAM; el segundo 'pisa a' el 0 (regrabalo)");
    CHECK(c[1].motivo == BPVM_NPACK_OK,
          "y su escalera esta bien: lo que falla es el reparto, no el pack");

    /* Un pack BORRADO debajo de uno vivo: su RAM queda de hueco (no se mueve
     * nada — con el reparto por orden de cadena habría que regrabar al de
     * encima). */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 0, "sqlite", &h, 190 + SQL_DATA); }                 /* tombstone */
    { bpvm_npack_hdr_t h = motor(RAM_BASE + SQL_RAM, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl", &h, 190 + GUI_DATA); }
    CHECK(plan(c, &p) == 1 && p.cargables == 1 && strcmp(c[0].npk, "lvgl") == 0,
          "borrado el de abajo -> el de arriba SIGUE valiendo (sin regrabar)");
    CHECK(p.ram_tope == RAM_BASE + SQL_RAM + GUI_RAM,
          "y el hueco de abajo NO se reaprovecha: el tope sigue arriba");

    /* Y BORRADO el de ARRIBA: su trozo vuelve a estar libre — así «borrar y
     * grabar» un pack actualizado reutiliza su sitio. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    { bpvm_npack_hdr_t h = motor(RAM_BASE + SQL_RAM, GUI_DATA, GUI_BSS);
      arma("Gui", 0, "lvgl", &h, 190 + GUI_DATA); }                     /* tombstone */
    CHECK(plan(c, &p) == 1 && p.ram_tope == RAM_BASE + SQL_RAM,
          "borrado el de ARRIBA -> el tope baja: su trozo se reaprovecha");

    /* ACTUALIZAR sin borrar: el nuevo va detrás del viejo (la placa le dio el
     * tope) y los dos se llaman igual. Gana el de MÁS ADELANTE, como los .mod
     * y los .mdn — antes de G1 ganaba el primero y corría el motor VIEJO. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    { bpvm_npack_hdr_t h = motor(RAM_BASE + SQL_RAM, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    CHECK(plan(c, &p) == 2 && p.cargables == 1, "dos motores con el MISMO nombre -> se carga uno");
    CHECK(c[0].eclipsado_por == 1 && bpvm_npack_cargable(&c[1]),
          "el de MAS ADELANTE (el nuevo), no el viejo");
    CHECK(p.ram_tope == RAM_BASE + 2 * SQL_RAM,
          "pero el viejo sigue reclamando su RAM mientras viva (borrarlo la libera)");

    /* Un sello FUERA del bloque (packRam= encogió): no se carga, y tampoco
     * cuenta para el tope — ese trozo no existe. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE + RAM_TAM, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    CHECK(plan(c, &p) == 1 && c[0].motivo == BPVM_NPACK_E_SELLO,
          "sello de RAM fuera del bloque -> SELLO (regrabalo)");
    CHECK(p.ram_tope == RAM_BASE, "y no sube el tope: esa RAM no es del bloque");

    /* Un motor que NO se carga por otra razón (sello de FLASH malo) SÍ reserva
     * su RAM: el pack está vivo, y si la placa diera ese trozo a otro, el día
     * que se arreglara el primero habría dos encima. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      h.linked_flash += 0x1000;
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    CHECK(plan(c, &p) == 1 && c[0].motivo == BPVM_NPACK_E_SELLO && p.cargables == 0,
          "sello de FLASH malo -> SELLO, no se carga");
    CHECK(p.ram_tope == RAM_BASE + SQL_RAM, "pero su RAM sigue reservada (esta vivo)");

    /* Una entrada `npk` más corta que su propia cabecera no es un motor. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, 0, 8);
      arma_("Roto", 1, "roto", &h, 0, 32); }      /* la entrada mide 32 B */
    CHECK(plan(c, &p) == 1 && c[0].motivo == BPVM_NPACK_E_MAGIC && c[0].hdr == 0,
          "entrada npk mas corta que su cabecera -> MAGIC, sin leer fuera");

    /* Más motores que sitio en `out`: se cuentan TODOS y el tope los ve a todos
     * — si no, la placa daría al siguiente la RAM de uno que no describió. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, SQL_DATA, SQL_BSS);
      arma("SQLite", 1, "sqlite", &h, 190 + SQL_DATA); }
    { bpvm_npack_hdr_t h = motor(RAM_BASE + SQL_RAM, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl", &h, 190 + GUI_DATA); }
    CHECK(bpvm_npack_planear(g_zona.b, sizeof g_zona.b, RAM_BASE, RAM_TAM, 0, c, 1, &p) == 2,
          "con sitio para UNO se cuentan los DOS");
    CHECK(p.ram_tope == RAM_BASE + SQL_RAM + GUI_RAM, "y el tope ve a los dos");
    CHECK(bpvm_npack_ram_tope(g_zona.b, sizeof g_zona.b, RAM_BASE, RAM_TAM) == p.ram_tope,
          "bpvm_npack_ram_tope (lo de PACK_BURN_BEGIN) == el tope del plan: UNA regla");

    /* La BIOS con huecos: ninguno se carga, y el motivo lo dice. */
    CHECK(bpvm_npack_planear(g_zona.b, sizeof g_zona.b, RAM_BASE, RAM_TAM, "memcpy",
                             c, BPVM_NPACK_MAX, &p) == 2 && p.cargables == 0
          && c[0].motivo == BPVM_NPACK_E_BIOS && c[1].motivo == BPVM_NPACK_E_BIOS,
          "BIOS con huecos -> ningun motor, los dos con motivo BIOS");

    /* El bloque LLENO: el segundo no cabe. El IDE lo verá antes de grabar
     * (ramSize); si aun así llegara, la escalera no deja escribir fuera. */
    zona_vacia();
    { bpvm_npack_hdr_t h = motor(RAM_BASE, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl", &h, 190 + GUI_DATA); }
    { bpvm_npack_hdr_t h = motor(RAM_BASE + GUI_RAM, GUI_DATA, GUI_BSS);
      arma("Gui", 1, "lvgl2", &h, 190 + GUI_DATA); }
    CHECK(plan(c, &p) == 2 && bpvm_npack_cargable(&c[0])
          && c[1].motivo == BPVM_NPACK_E_TAMANO,
          "dos de 392 KB en 512 KB -> el segundo NO CABE (TAMANO, no escribe fuera)");
    CHECK(p.ram_tope == RAM_BASE + GUI_RAM,
          "y el que no cabe no cuenta para el tope (no es un trozo del bloque)");

    /* ── V7/H7 (Boot 2, fase 2) — EL CARGADOR COMÚN, en todo menos el salto: en el PC no hay
     * a dónde saltar, así que sólo motores que la escalera rechaza. La zona, MONTADA. ── */
    {
        static int bios_de_prueba;   /* no se presta: ningún motor llega a saltar */
        bpvm_npack_familia_t f;
        memset(&f, 0, sizeof f);
        f.bios      = (const struct bpvm_bios*) (const void*) &bios_de_prueba;
        f.ram       = (uint8_t*) (uintptr_t) RAM_BASE;
        f.ram_bytes = RAM_TAM;
        f.log       = captura;
        char av[200];

        bpvm_npack_carga_reinicia(); g_log[0] = '\0';
        bpvm_pack_mount(NULL, NULL, 0);
        CHECK(bpvm_npack_cargar_zona(&f) == -(int32_t) BPVM_NPACK_E_MAGIC
              && strstr(g_log, "no hay zona de packs montada"),
              "cargador comun: sin zona montada, no carga y lo dice");

        zona_vacia();
        bpvm_pack_mount(g_zona.b, g_zona.b, sizeof g_zona.b);
        f.ram = NULL; g_log[0] = '\0';
        CHECK(bpvm_npack_cargar_zona(&f) == -(int32_t) BPVM_NPACK_E_TAMANO
              && strstr(g_log, "no hay bloque de RAM"),
              "cargador comun: sin bloque de RAM, no carga y lo dice");
        f.ram = (uint8_t*) (uintptr_t) RAM_BASE; g_log[0] = '\0';
        CHECK(bpvm_npack_cargar_zona(&f) == -(int32_t) BPVM_NPACK_E_MAGIC
              && strstr(g_log, "ningun pack grabado"),
              "cargador comun: zona vacia, no se recorre");

        { bpvm_npack_hdr_t h = motor(RAM_BASE, 0, 8);
          h.arch = (uint16_t) (h.arch ^ 0x5Au);
          arma("Otro", 1, "otro", &h, 190); }
        g_log[0] = '\0';
        CHECK(bpvm_npack_cargar_zona(&f) == -(int32_t) BPVM_NPACK_E_ARCH
              && strstr(g_log, "'Otro' (motor 'otro') NO se carga")
              && strstr(g_log, "esta placa ES arch")
              && strstr(g_log, "sin motor utilizable"),
              "cargador comun: un motor de otra arquitectura no se salta, y se dice con los dos arch");
        CHECK(bpvm_pack_aviso_motor_de(av, sizeof av) && strstr(av, "'Otro' no carga su motor"),
              "cargador comun: y deja el casi, con el nombre del pack");
        g_log[0] = '\0';
        CHECK(bpvm_npack_cargar_zona(&f) == -(int32_t) BPVM_NPACK_E_ARCH && strstr(g_log, "NO se carga"),
              "cargador comun: sin saltar a ninguno NO marca: el siguiente RUN lo reintenta");

        /* El STATE: la zona está lista, así que el casi que dice es el del motor. */
        static uint8_t eA[4096], eB[4096], eS[4096];
        memset(eA, 0xFF, sizeof eA); memset(eB, 0xFF, sizeof eB);
        bpvm_bmgr_t bm;
        memset(&bm, 0, sizeof bm);
        bm.a = eA; bm.b = eB; bm.scratch = eS; bm.sector = 4096u;
        bm.packs_base = g_zona.b; bm.packs_size = sizeof g_zona.b;
        bpvm_bmgr_req_t rq;
        memset(&rq, 0, sizeof rq);
        snprintf(rq.type, sizeof rq.type, "STATE"); rq.id = 3;
        char rep[1024];
        int slot = -9;
        int nr = bpvm_bmgr_wire_dispatch(&bm, &rq, rep, sizeof rep, &slot);
        CHECK(nr > 0 && strstr(rep, "\"casi\":true") && strstr(rep, "no carga su motor"),
              "STATE: con la zona lista, el casi de un motor que no carga");
        bpvm_npack_carga_reinicia();
        nr = bpvm_bmgr_wire_dispatch(&bm, &rq, rep, sizeof rep, &slot);
        CHECK(nr > 0 && strstr(rep, "\"casi\":false"), "STATE: sin intento, sin casi");
        bpvm_pack_mount(NULL, NULL, 0);
    }

    printf("\n[status=%s]\n", g_fail == 0 ? "OK" : "FAIL");
    return g_fail != 0;
}
