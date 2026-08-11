/*
 * test_blk.c — V5/H6: el contrato de dispositivo de bloque.
 *
 * QUÉ SE PRUEBA Y POR QUÉ ÉSTO. Al separar FatFs del driver de SPI salió a la
 * luz una función que llevaba escondida dentro del montaje: **dónde empieza el
 * sistema de ficheros**. Es exactamente la clase de código que este proyecto
 * ya sabe que hay que probar en el PC —como los decodificadores del CSD
 * (`test_sd.c`)— porque equivocarse ahí **no revienta**: devuelve un número.
 *
 * Y un número equivocado aquí se manifiesta como «esta tarjeta no tiene
 * FAT32», que manda al usuario a reformatear una tarjeta que estaba bien.
 *
 * Los dos desplazamientos de abajo no son inventados: son los MEDIDOS en las
 * dos tarjetas que han pasado por el banco — 2048 en la tarjeta formateada a
 * FAT32, y 32768 en la SanDisk de 128 GB tal como viene de fábrica.
 *
 *   make test-blk
 */
#include "bpvm_blk.h"
#include "bpvm_sd_blk.h"

#include <stdio.h>
#include <string.h>

/* Tapón del reloj, por el mismo motivo que en test_sd.c: el símbolo lo pide el
 * enlazador y enlazar la plataforma de verdad arrastraría media VM. A CERO a
 * propósito — ver abajo por qué este test NO arranca ninguna tarjeta. */
int64_t bpvm_platform_now_ms(void) { return 0; }

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Un MBR de verdad: código de arranque (que NO es ASCII en los bytes 3..10),
 * una entrada de partición y la firma. */
static void mbr(uint8_t s[512], int ranura, uint8_t tipo, uint32_t lba) {
    memset(s, 0, 512);
    s[0] = 0x33; s[1] = 0xC0; s[2] = 0x8E;      /* xor ax,ax / mov es,ax ... */
    s[3] = 0xD0; s[4] = 0xBC; s[5] = 0x00; s[6] = 0x7C;
    s[7] = 0xFB; s[8] = 0x50; s[9] = 0x07; s[10] = 0x50;
    uint8_t* e = s + 446 + ranura * 16;
    e[0] = 0x00;                                 /* no arrancable */
    e[4] = tipo;
    e[8]  = (uint8_t) (lba      );
    e[9]  = (uint8_t) (lba >>  8);
    e[10] = (uint8_t) (lba >> 16);
    e[11] = (uint8_t) (lba >> 24);
    s[510] = 0x55; s[511] = 0xAA;
}

int main(void)
{
    printf("== V5/H6: contrato de bloque y tabla de particiones ==\n\n");

    /* ── Dónde empieza el FS ─────────────────────────────────────────────── */
    {
        uint8_t s[512];

        mbr(s, 0, 0x0C, 2048);          /* FAT32 LBA, el caso normal */
        CHECK(bpvm_blk_lba0_de_mbr(s) == 2048u,
              "MBR: particion en el bloque 2048 (tarjeta formateada a FAT32)");

        mbr(s, 0, 0x07, 32768);         /* exFAT/NTFS, la de 128 GB de fabrica */
        CHECK(bpvm_blk_lba0_de_mbr(s) == 32768u,
              "MBR: particion en el bloque 32768 (SanDisk 128 GB de fabrica)");

        /* La primera ranura vacía no puede tapar a la segunda: si se leyera la
         * tabla a ciegas saldría 0, y montar en el bloque 0 es EL error. */
        mbr(s, 1, 0x0C, 8192);
        CHECK(bpvm_blk_lba0_de_mbr(s) == 8192u,
              "MBR: se salta la ranura vacia y coge la primera con tipo");

        /* Un MBR con la tabla entera vacía sí es un 0 legítimo. */
        memset(s, 0, 512);
        s[0] = 0x33; s[1] = 0xC0; s[2] = 0x8E; s[3] = 0xD0;
        s[510] = 0x55; s[511] = 0xAA;
        CHECK(bpvm_blk_lba0_de_mbr(s) == 0u,
              "MBR sin particiones -> 0");
    }

    /* ── Lo que NO es un MBR ─────────────────────────────────────────────── */
    {
        uint8_t s[512];

        /* Un sector de arranque de FAT lleva la MISMA firma 0x55AA al final.
         * Lo que lo distingue es el nombre del formateador en los bytes 3..10.
         * Confundirlos leería una tabla de particiones que no existe. */
        memset(s, 0, 512);
        s[0] = 0xEB; s[1] = 0x58; s[2] = 0x90;
        memcpy(s + 3, "MSDOS5.0", 8);
        /* Justo donde estaría la 1ª entrada, un valor que NO debe salir. */
        s[446 + 4] = 0x0C; s[446 + 8] = 0xFF; s[446 + 9] = 0xFF;
        s[510] = 0x55; s[511] = 0xAA;
        CHECK(bpvm_blk_lba0_de_mbr(s) == 0u,
              "sector de arranque de FAT (superfloppy) -> 0, no lee la tabla");

        memset(s, 0, 512);
        memcpy(s + 3, "mkfs.fat", 8);
        s[510] = 0x55; s[511] = 0xAA;
        CHECK(bpvm_blk_lba0_de_mbr(s) == 0u,
              "otro formateador ('mkfs.fat') tambien se reconoce");

        /* Sin firma no hay nada que interpretar. */
        memset(s, 0, 512);
        mbr(s, 0, 0x0C, 2048);
        s[511] = 0x00;
        CHECK(bpvm_blk_lba0_de_mbr(s) == 0u,
              "sin la firma 0x55AA no se lee la tabla");

        CHECK(bpvm_blk_lba0_de_mbr(NULL) == 0u, "NULL -> 0, sin reventar");
    }

    /* ── La forma del backend de SPI ─────────────────────────────────────
     *
     * ⚠️ Aquí NO se arranca ninguna tarjeta, y no es pereza: `init()` con pines
     * puestos hablaría con la fachada SPI sin backend (que sólo imprime) y
     * subiría la escalera contra un bus mudo, esperando plazos con el reloj
     * tapado a cero. O sea que lo que se prueba es lo que se PUEDE probar sin
     * hierro: que el dispositivo sin configurar se niegue a arrancar en vez de
     * salir con pines a cero, y que el contrato esté completo. */
    {
        const bpvm_blk_backend_t* b = bpvm_sd_blk(NULL);
        CHECK(b != NULL,                     "el backend existe sin configurar");
        CHECK(b->init && b->leer && b->escribir && b->hay_medio
              && b->bloques && b->motivo,
                                             "el contrato esta completo");
        CHECK(b->sincronizar == NULL,
              "sincronizar es NULL A PROPOSITO (la escritura de SPI ya espera)");
        CHECK(b->init() != 0,                "sin pines NO arranca");
        CHECK(b->motivo() && *b->motivo(),   "y lo dice: el motivo nunca es mudo");
        CHECK(b->bloques() == 0u,            "sin arrancar, capacidad 0");
        CHECK(b->hay_medio() == 0,           "sin configurar, no hay medio");
        CHECK(bpvm_sd_blk_info() == NULL,    "sin arrancar no hay datos de tarjeta");

        /* Configurar devuelve el mismo backend: es UN dispositivo. */
        bpvm_sd_pines_t p = { 0, 34, 35, 36, 39, 40, 1 };
        CHECK(bpvm_sd_blk(&p) == b,          "configurar no cambia el backend");
        CHECK(b->bloques() == 0u,            "recien configurado, sigue sin capacidad");
    }

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
