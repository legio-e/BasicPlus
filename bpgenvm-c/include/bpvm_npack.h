/* bpvm_npack.h — el PACK NATIVO en flash: cabecera + escalera de validación.
 *
 * Un pack nativo es código C ajeno pre-enlazado en el PC, grabado en la zona de
 * packs y ejecutado XIP. Antes de saltar a él hay SIETE cosas que pueden estar
 * mal, y este fichero existe para que cada una falle CON SU NOMBRE en vez de
 * colgar la placa.
 *
 * Criterio de Eduardo (7-ago): *"hay que prever que algo puede fallar, así que
 * hay que poner chivatos que nos digan qué es lo que no funciona, para saber
 * dónde y no sólo funciona/no funciona"*.
 *
 * ─── LA ESCALERA ───
 *
 * Misma forma que `bpvm_boot`: se sube peldaño a peldaño y se para en el primero
 * que falla, dejando dicho CUÁL y POR QUÉ. Los seis primeros son comprobaciones
 * baratas sobre datos; el séptimo —saltar— es el único que puede colgar, y por
 * eso va aparte y con miga de pan (el llamante escribe en el log ANTES de saltar
 * y DESPUÉS: si se cuelga, el siguiente arranque muestra el paso sin resultado).
 *
 * ─── EL SELLO ───
 *
 * `linked_flash` / `linked_ram` dicen para qué direcciones se realojó el pack.
 * Si no coinciden con dónde está de verdad, el código lleva dentro punteros a
 * sitios equivocados y saltar sería un desastre MUDO. Comparar dos enteros lo
 * convierte en un mensaje: "regrábalo desde el IDE".
 *
 * Esto pasa cuando el usuario cambia `SQLite=<MB>` en el entorno (mueve la RAM)
 * o cuando el pack se copia a otra placa con otro reparto de flash. Es un caso
 * NORMAL, no un error raro: por eso se detecta y se explica en vez de confiar.
 */
#ifndef BPVM_NPACK_H
#define BPVM_NPACK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_NPACK_MAGIC   0x42504E50u   /* 'BPNP' */
#define BPVM_NPACK_FORMAT  1u

/*
 * Tamaño EXACTO de la cabecera, y con él el reparto del pack en flash:
 *
 *     pack_base + 0                          cabecera  (64 B)
 *     pack_base + BPVM_NPACK_HDR_BYTES       imagen de flash  (flash_bytes)
 *     ... + flash_bytes                      imagen inicial de .data (data_bytes)
 *
 * Son 64 y no 56 —lo que suman los campos— a propósito: la imagen de código
 * empieza JUSTO detrás, así que este número es la base del código. Redondo,
 * alineado, y con dos huecos de reserva ya pagados para campos futuros sin
 * mover nada. El test comprueba que la struct mide esto de verdad.
 */
#define BPVM_NPACK_HDR_BYTES 64u

/* Bits de `flags`. */
#define BPVM_NPACK_F_THUMB 0x1u          /* la entrada es Thumb → bit 0 al saltar */

/*
 * Cabecera, al principio del pack en flash. TODO en little-endian nativo: el
 * pack se realoja en el PC PARA ESTA placa, así que no hay cruce de endianness
 * que valga (a diferencia del .pack de datos, que sí es big-endian portable).
 *
 * Tras la cabecera vienen, en orden: imagen de flash, imagen inicial de .data.
 * La tabla de relocalizaciones NO viaja: la consume el IDE al grabar. Un pack
 * grabado tiene `reloc_count == 0` — y eso mismo es la prueba de que ya se
 * realojó, sin campo aparte que pueda contradecirlo.
 */
typedef struct {
    uint32_t magic;          /* BPVM_NPACK_MAGIC                              */
    uint16_t format;         /* BPVM_NPACK_FORMAT                             */
    uint16_t arch;           /* MDN_ARCH_* (mismo catálogo que el .mdn)       */
    char     float_abi[8];   /* "softfp"/"hard"/"ilp32d"… NUL-terminado       */
    uint32_t entry_off;      /* offset de la entrada, SIN el bit Thumb        */
    uint32_t flags;          /* BPVM_NPACK_F_*                                */
    uint32_t flash_bytes;    /* tamaño de la imagen de flash                  */
    uint32_t data_bytes;     /* imagen inicial de .data (se copia a RAM)      */
    uint32_t bss_bytes;      /* a poner a CERO en RAM tras .data              */
    uint32_t linked_flash;   /* EL SELLO: para qué dirección se realojó       */
    uint32_t linked_ram;     /* ídem, lado RAM                                */
    uint32_t reloc_count;    /* 0 = ya realojado (lo que se graba)            */
    uint32_t reserved[4];    /* 0 — hasta BPVM_NPACK_HDR_BYTES                */
} bpvm_npack_hdr_t;

/* Peldaños. El orden IMPORTA: cada uno supone que el anterior pasó. */
typedef enum {
    BPVM_NPACK_OK = 0,       /* los 6 peldaños de datos, superados            */
    BPVM_NPACK_E_MAGIC,      /* 1 — no es un pack nativo                      */
    BPVM_NPACK_E_FORMAT,     /* 2 — formato de otra época                     */
    BPVM_NPACK_E_ARCH,       /* 3 — compilado para otra ISA                   */
    BPVM_NPACK_E_ABI,        /* 4 — otra convención de coma flotante          */
    BPVM_NPACK_E_SELLO,      /* 5 — realojado para OTRA dirección             */
    BPVM_NPACK_E_SIN_RELOC,  /* 6 — llegó sin realojar (reloc_count != 0)     */
    BPVM_NPACK_E_TAMANO,     /* 7 — no cabe / entrada fuera de la imagen      */
    BPVM_NPACK_E_BIOS        /* 8 — la tabla que le íbamos a prestar cojea    */
} bpvm_npack_res_t;

/*
 * Sube la escalera. NO salta: sólo dice si se puede.
 *
 *   h            — la cabecera leída de flash (o NULL)
 *   aqui_flash   — dónde está el CÓDIGO de verdad (dirección que ve la CPU).
 *                  ⚠️ NO es la base del pack: es `pack_base + HDR_BYTES`. La
 *                  cabecera va delante, y `entry_off` cuenta desde el código.
 *                  Confundirlos son 64 bytes de desfase y un salto a mitad de
 *                  instrucción — por eso está dicho aquí y no se deduce.
 *   aqui_ram     — base del bloque de RAM que se le va a dar
 *   sitio_flash  — bytes disponibles en la zona de packs
 *   sitio_ram    — bytes disponibles en el bloque de RAM
 *   bios_falta   — NULL si la BIOS está entera; si no, el nombre que falta
 *                  (se pasa ya calculado: quien conoce la BIOS es la placa)
 */
bpvm_npack_res_t bpvm_npack_check(const bpvm_npack_hdr_t* h,
                                  uint32_t aqui_flash, uint32_t aqui_ram,
                                  uint32_t sitio_flash, uint32_t sitio_ram,
                                  const char* bios_falta);

/* Texto del peldaño, para el log. Incluye QUÉ HACER cuando lo hay: un "no" que
 * no dice cómo salir de él obliga a leer el código. Nunca NULL. */
const char* bpvm_npack_res_str(bpvm_npack_res_t r);

/*
 * ─── BUSCAR EL PACK ─── idea de Eduardo (7-ago-2026)
 *
 * Simétrico del ANCLA de `bpvm_bios.h`: aquello arregla «el firmware se mueve»,
 * esto arregla «el pack se mueve». Misma medicina.
 *
 * Hasta ahora el cargador tenía que SABER dónde está el pack —al principio de la
 * partición— y esa clase de suposición nos costó, en una tarde, un cuelgue de la
 * placa y dos diagnósticos equivocados. Aquí no se supone: se BARRE la zona
 * buscando el magic y se sube la escalera sobre cada candidato.
 *
 * No hay validación nueva: los mismos siete peldaños de `bpvm_npack_check`. El
 * barrido sólo los alimenta, así que la regla sigue en UN sitio.
 *
 * ⚠️ Y EL SELLO NO SOBRA, CAMBIA DE PAPEL. Al encontrarlo barriendo,
 * `aqui_flash` es *dónde ha aparecido*, y el sello comprueba que se realojó
 * PARA AHÍ. Deja de servir para encontrarlo y pasa a servir para confirmar que
 * no se ha movido desde que se grabó — el caso de «alguien cambió SQLite=<MB>
 * y la RAM bailó».
 */
typedef struct {
    uint32_t          addr;        /* dirección de la CABECERA hallada; 0 = ninguna */
    uint32_t          candidatos;  /* cuántas veces apareció el magic en la zona    */
    bpvm_npack_res_t  motivo;      /* por qué se rechazó el ÚLTIMO candidato        */
} bpvm_npack_hallazgo_t;

/*
 * Barre [base, base+bytes) de 4 en 4 buscando un pack VÁLIDO. Devuelve el
 * primero que supera la escalera entera.
 *
 * `candidatos` y `motivo` son el chivato, y no son adorno: "no lo encuentro" y
 * "encontré uno pero el sello no cuadra" mandan a sitios MUY distintos, y sin
 * separarlos habría que adivinar cuál de los dos es. Con candidatos==0 no hay
 * pack grabado; con candidatos>0 lo hay y `motivo` dice qué le pasa.
 *
 * No escribe, no aloca, no salta: sólo mira.
 */
bpvm_npack_hallazgo_t bpvm_npack_buscar(const void* base, uint32_t bytes,
                                        uint32_t aqui_ram, uint32_t sitio_ram,
                                        const char* bios_falta);

/* La dirección a la que hay que saltar: entrada + base + bit Thumb si toca.
 * En UN sitio porque olvidar el bit 0 es hard fault inmediato, y es justo el
 * tipo de detalle que se copia mal al segundo sitio donde se escriba. */
uint32_t bpvm_npack_entry_addr(const bpvm_npack_hdr_t* h, uint32_t aqui_flash);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_NPACK_H */
