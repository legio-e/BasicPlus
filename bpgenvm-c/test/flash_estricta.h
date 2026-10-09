/* flash_estricta.h — V7 #647 (9-oct): LA FLASH DE MENTIRA, tan estricta como la placa más estricta.
 *
 * El micro simulado (tools/bpvm_sim.c) y las pruebas del gestor (test/test_bmgr.c, test/test_pack.c) llevan una «flash»
 * en RAM para el código común de los packs (bpvm_pack_flash_t: grabar, borrar, formatear). Hasta hoy la del simulado y
 * la de test-bmgr COPIABAN ENCIMA: aceptaban escribir sobre lo ya escrito, que una flash de verdad no hace —una NOR sólo
 * pasa bits de 1 a 0 sin borrar, y en la STM32U5 (flash con ECC) cada quadword de 16 bytes se programa UNA vez entre
 * borrados—. Si el gestor escribiera dos veces un trozo sin borrarlo, la Pico o una ESP leerían basura y la U5 daría
 * error; el PC, OK. Eduardo (9-oct): «OK, V7», con la propuesta: el simulado, la placa MÁS ESTRICTA de todas —lo que
 * pase en él pasa en todas—.
 *
 * Las reglas, la unión de las de las familias:
 *   - borrar: por bloques enteros (`bloque`: 4 KB la Pico y las ESP, 8 KB la U5) → 0xFF;
 *   - programar: alineado a 16 y en múltiplos de 16 (el contrato de bpvm_pack.h: «obligatorio en flash con ECC como el
 *     U5»); cada quadword, UNA vez entre borrados, y entero borrado antes; uno cuyos datos son todo 0xFF no se programa
 *     —queda borrado, como hace la cintura de la U5 (board_mgr_stm32.c) y como da una NOR (programar 1 es no tocar)—.
 * Lo que lo incumple NO se escribe: la función devuelve -1 (la cintura falló: BPVM_PACK_ERR_IO arriba) y deja el porqué,
 * con la dirección, en `error` —un error con nombre, no basura en silencio—.
 *
 * Sólo para el PC (las pruebas y el simulado); cabecera sola, sin alta en los builds. */
#ifndef FLASH_ESTRICTA_H
#define FLASH_ESTRICTA_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FLASH_ESTRICTA_QUADWORD 16u

typedef struct {
    uint8_t*    mem;          /* la «flash»: 0xFF = borrada */
    uint32_t    size;         /* sus bytes */
    uint32_t    bloque;       /* el bloque de borrado */
    char        error[200];   /* el porqué del último -1 */
    unsigned    fallos;       /* cuántas operaciones se han negado (las pruebas lo miran) */
} flash_estricta_t;

static int flash_estricta_niega(flash_estricta_t* f, const char* fmt, uint32_t a, uint32_t b) {
    snprintf(f->error, sizeof f->error, fmt, (unsigned) a, (unsigned) b);
    f->fallos++;
    return -1;
}

/* Borra [off, off+len): bloques enteros. */
static int flash_estricta_erase(flash_estricta_t* f, uint32_t off, uint32_t len) {
    if ((off % f->bloque) != 0u || (len % f->bloque) != 0u)
        return flash_estricta_niega(f, "flash: borrar fuera de bloque (off 0x%06X, len %u)", off, len);
    if (len > f->size || off > f->size - len)
        return flash_estricta_niega(f, "flash: borrar fuera de la flash (off 0x%06X, len %u)", off, len);
    memset(f->mem + off, 0xFF, len);
    return 0;
}

/* Programa [off, off+len): quadwords enteros, cada uno UNA vez entre borrados. Comprueba TODO antes de escribir nada:
 * lo que se niega no deja la flash a medias (la placa sí podría, pero así el error dice dónde y no hay segundo fallo). */
static int flash_estricta_program(flash_estricta_t* f, uint32_t off, const uint8_t* d, uint32_t len) {
    if ((off % FLASH_ESTRICTA_QUADWORD) != 0u || (len % FLASH_ESTRICTA_QUADWORD) != 0u)
        return flash_estricta_niega(f, "flash: programar sin alinear a 16 (off 0x%06X, len %u)", off, len);
    if (len > f->size || off > f->size - len)
        return flash_estricta_niega(f, "flash: programar fuera de la flash (off 0x%06X, len %u)", off, len);
    for (uint32_t q = 0; q < len; q += FLASH_ESTRICTA_QUADWORD) {
        int todo_ff = 1, borrado = 1;
        for (uint32_t i = 0; i < FLASH_ESTRICTA_QUADWORD; i++) {
            if (d[q + i] != 0xFFu) todo_ff = 0;
            if (f->mem[off + q + i] != 0xFFu) borrado = 0;
        }
        if (!todo_ff && !borrado)
            return flash_estricta_niega(f, "flash: programar sin borrar en 0x%06X (ese quadword ya estaba programado: "
                                           "sólo se programa una vez entre borrados; %u bytes pedidos)", off + q, len);
    }
    for (uint32_t q = 0; q < len; q += FLASH_ESTRICTA_QUADWORD) {
        int todo_ff = 1;
        for (uint32_t i = 0; i < FLASH_ESTRICTA_QUADWORD; i++) if (d[q + i] != 0xFFu) { todo_ff = 0; break; }
        if (!todo_ff) memcpy(f->mem + off + q, d + q, FLASH_ESTRICTA_QUADWORD);
    }
    return 0;
}

#endif /* FLASH_ESTRICTA_H */
