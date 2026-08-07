/*
 * test_npack.c — V5/I: la escalera de validación del pack nativo.
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
#include "mdn_loader.h"
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
    return bpvm_npack_check(h, AQUI_FLASH, AQUI_RAM, SITIO_FLASH, SITIO_RAM,
                            bios_falta);
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
     * El caso REAL: alguien cambia SQLite=<MB>, el bloque de RAM se mueve, y el
     * pack sigue grabado con los punteros viejos. */
    { bpvm_npack_hdr_t h = buena(); h.linked_ram = AQUI_RAM + 0x100000;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SELLO,
            "la RAM se movio (cambio SQLite=<MB>) -> SELLO"); }
    { bpvm_npack_hdr_t h = buena(); h.linked_flash = AQUI_FLASH - 0x1000;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SELLO, "grabado en otro offset -> SELLO"); }

    /* ── Peldaño 5: ¿pasó por el IDE? ── */
    { bpvm_npack_hdr_t h = buena(); h.reloc_count = 4;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_SIN_RELOC,
            "trae su tabla de relocs -> SIN_RELOC (no paso por el IDE)"); }

    /* ── Peldaño 6: tamaños ── */
    { bpvm_npack_hdr_t h = buena(); h.flash_bytes = SITIO_FLASH + 1;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO, "mas grande que la zona -> TAMANO"); }
    { bpvm_npack_hdr_t h = buena(); h.bss_bytes = SITIO_RAM;
      CHECK(chk(&h, 0) == BPVM_NPACK_OK,
            ".data+.bss ocupan EXACTAMENTE la RAM -> cabe (el borde entra)"); }
    { bpvm_npack_hdr_t h = buena(); h.bss_bytes = SITIO_RAM + 1;
      CHECK(chk(&h, 0) == BPVM_NPACK_E_TAMANO,
            "UN BYTE mas de RAM de la que hay -> TAMANO"); }
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

    /* ── EL REPARTO DE LA CABECERA, campo a campo ──
     *
     * Esta cabecera la ESCRIBE el empaquetador (Python, en el PC) y la LEE el
     * cargador (C, en la placa). Son dos lenguajes distintos leyendo los mismos
     * bytes: el reparto es un CONTRATO, y un contrato que no está escrito en
     * ningún sitio se rompe en silencio — el pack se leería con los campos
     * corridos y el primer sintoma seria un salto a ninguna parte.
     *
     * Se fija por los dos lados: aqui los offsets REALES que dice el compilador,
     * y en pack.py un assert sobre el tamaño de su formato. Si alguien reordena
     * un campo, este test dice CUÁL — igual que la tabla de la BIOS. */
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

    printf("\n[status=%s]\n", g_fail == 0 ? "OK" : "FAIL");
    return g_fail != 0;
}
