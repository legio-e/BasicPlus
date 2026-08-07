/* bpvm_sqlmem.c — la regla del bloque de la BD. Ver bpvm_sqlmem.h para el
 * porqué de cada número (todos MEDIDOS, no estimados).
 *
 * Sin dependencias a propósito: sólo <stddef.h>. Así el test es host-only y no
 * arrastra la VM entera — mismo criterio que bpvm_env/bpvm_part/bpvm_boot (H9).
 */
#include "bpvm_sqlmem.h"

bpvm_sqlite_res_t bpvm_sqlite_region(long env_mb, size_t total_bytes,
                                     size_t* out_bytes)
{
    if (out_bytes) *out_bytes = 0;

    /* No se pide BD. Ni ruido ni aviso: es el caso normal de casi todas las
     * placas. `bpvm_env_get_long(env,"SQLite",0)` ya da 0 si la clave no está. */
    if (env_mb <= 0) return BPVM_SQLITE_OFF;

    /* Se pide, pero por debajo del suelo MEDIDO. No se activa a medias. */
    if (env_mb < BPVM_SQLITE_MIN_MB) return BPVM_SQLITE_MUY_POCO;

    /* Cordura antes de multiplicar: en un size_t de 32 bits, 4096 MB desborda
     * a 0 y "no cabe" se convertiría en "cabe de sobra". */
    if (env_mb > BPVM_SQLITE_MAX_MB) return BPVM_SQLITE_NO_CABE;

    size_t want = (size_t) env_mb * 1024u * 1024u;

    /* Dos comprobaciones, no una: la resta de abajo se desbordaría si `want`
     * fuese mayor que el total. */
    if (want > total_bytes) return BPVM_SQLITE_NO_CABE;
    if (total_bytes - want < BPVM_VM_MIN_BYTES) return BPVM_SQLITE_NO_CABE;

    /* No hace falta alinear: un múltiplo de 1 MB ya lo está de sobra para
     * MEMSYS5 (8 B) y para la ventana XIP (4 KB). */
    if (out_bytes) *out_bytes = want;
    return BPVM_SQLITE_OK;
}

const char* bpvm_sqlite_res_str(bpvm_sqlite_res_t r)
{
    switch (r) {
        case BPVM_SQLITE_OFF:       return "no solicitada";
        case BPVM_SQLITE_OK:        return "reservada";
        case BPVM_SQLITE_MUY_POCO:  return "por debajo del minimo";
        case BPVM_SQLITE_NO_CABE:   return "no cabe dejando la VM viable";
        default:                    return "?";
    }
}

const char* bpvm_sqlite_res_code(bpvm_sqlite_res_t r)
{
    /* Tokens CORTOS y ESTABLES: los parsea el IDE. Cambiar uno rompe la
     * compatibilidad con firmwares ya grabados — no se tocan. */
    switch (r) {
        case BPVM_SQLITE_OFF:       return "off";
        case BPVM_SQLITE_OK:        return "ok";
        case BPVM_SQLITE_MUY_POCO:  return "low";
        case BPVM_SQLITE_NO_CABE:   return "nofit";
        default:                    return "?";
    }
}
