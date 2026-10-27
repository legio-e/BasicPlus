/*
 * bpvm_env_linea.h — las líneas `clave:valor` del ENV, los dos ayudantes que las leen.
 *
 * La línea `sd` en SPI (bpvm_sd.c) y en SDIO (bpvm_blk_sdmmc_cfg.c) los tenían duplicados a propósito, con la nota:
 * «Si aparece un tercer parseo, ése es el momento de sacarlos a un sitio común, no antes». El tercero es la línea
 * `audio` del sonido (V8/H4, #708, bpvm_sonido.c). Sólo texto: ni SDK ni hardware.
 */
#ifndef BPVM_ENV_LINEA_H
#define BPVM_ENV_LINEA_H

/* `a` y luego `b`, en `dst` (cabe lo que cabe, y siempre acaba en NUL): el motivo de un error. */
static inline void bpvm_linea_di(char* dst, unsigned cap, const char* a, const char* b) {
    unsigned i = 0;
    if (!dst || cap == 0) return;
    while (a && *a && i + 1 < cap) dst[i++] = *a++;
    while (b && *b && i + 1 < cap) dst[i++] = *b++;
    dst[i] = '\0';
}

/* Si `s` empieza por la clave `k` y dos puntos, lo que va detrás; si no, NULL. */
static inline const char* bpvm_linea_tras_clave(const char* s, const char* k) {
    while (*k) { if (*s != *k) return 0; s++; k++; }
    return (*s == ':') ? s + 1 : 0;
}

#endif /* BPVM_ENV_LINEA_H */
