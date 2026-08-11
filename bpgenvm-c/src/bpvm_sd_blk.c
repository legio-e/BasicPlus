/*
 * bpvm_sd_blk.c — V5/H6: la SD por SPI cumpliendo el contrato de bloque.
 *
 * Fino a propósito: aquí no hay protocolo. Todo lo que sabe de tarjetas está en
 * `bpvm_sd.c` (H1) y no se ha tocado — el criterio del paso 1 de H6 es que la
 * Metro se comporte EXACTAMENTE igual que antes.
 *
 * Lo único que hace este fichero es quedarse con el estado que `fs_fat.c` no
 * debería tener nunca (los pines y el `bpvm_sd_info_t`) y traducir «lee N
 * bloques» a N llamadas de un bloque.
 */
#include "bpvm_sd_blk.h"

#include <string.h>

/* ── Estado: una tarjeta. ─────────────────────────────────────────────────── */
static bpvm_sd_pines_t s_pines;
static bpvm_sd_info_t  s_info;
static int             s_configurado = 0;
static int             s_arrancada   = 0;
static bpvm_sd_res_t   s_ultimo      = BPVM_SD_OK;

/*
 * El reloj de trabajo. Estaba metido en `bpvm_fs_fat_montar`, donde no pintaba
 * nada: es una propiedad del BUS, no del sistema de ficheros. 25 MHz es el tope
 * del modo SPI estándar de la SD; el arranque negocia por debajo de 400 kHz y
 * sube aquí al final (lo hace `bpvm_sd_init`, ver su cabecera).
 */
#define SD_BAUD_RAPIDO 25000000

static int sd_init(void) {
    if (!s_configurado) { s_ultimo = BPVM_SD_E_PINES; return -1; }
    s_ultimo = bpvm_sd_init(&s_pines, SD_BAUD_RAPIDO, &s_info);
    s_arrancada = (s_ultimo == BPVM_SD_OK);
    return s_arrancada ? 0 : -1;
}

static int sd_leer(uint32_t lba, uint32_t n, uint8_t* dst) {
    if (!s_arrancada || !dst) return -1;
    for (uint32_t i = 0; i < n; i++) {
        s_ultimo = bpvm_sd_leer_bloque(&s_pines, &s_info, lba + i,
                                       dst + (size_t) i * BPVM_BLK_TAM);
        if (s_ultimo != BPVM_SD_OK) return -1;
    }
    return 0;
}

static int sd_escribir(uint32_t lba, uint32_t n, const uint8_t* src) {
    if (!s_arrancada || !src) return -1;
    for (uint32_t i = 0; i < n; i++) {
        s_ultimo = bpvm_sd_escribir_bloque(&s_pines, &s_info, lba + i,
                                           src + (size_t) i * BPVM_BLK_TAM);
        if (s_ultimo != BPVM_SD_OK) return -1;
    }
    return 0;
}

static int sd_hay_medio(void) {
    if (!s_configurado) return 0;
    return bpvm_sd_hay_tarjeta(&s_pines);
}

static uint32_t sd_bloques(void) {
    return s_arrancada ? s_info.bloques : 0u;
}

static const char* sd_motivo(void) {
    return bpvm_sd_res_str(s_ultimo);
}

/* `sincronizar` va a NULL EXPRESAMENTE, y no es un hueco por rellenar:
 * `bpvm_sd_escribir_bloque` no vuelve hasta que la tarjeta suelta la línea de
 * ocupada, o sea que cuando una escritura termina ya está grabada. Poner aquí
 * una función que no hiciera nada diría lo contrario a quien lo lea. */
static const bpvm_blk_backend_t s_backend = {
    .init        = sd_init,
    .leer        = sd_leer,
    .escribir    = sd_escribir,
    .hay_medio   = sd_hay_medio,
    .bloques     = sd_bloques,
    .sincronizar = NULL,
    .motivo      = sd_motivo,
};

const bpvm_blk_backend_t* bpvm_sd_blk(const bpvm_sd_pines_t* pines) {
    if (pines) {
        /* Si cambian los pines, lo que hubiera arrancado ya no vale. */
        if (!s_configurado || memcmp(&s_pines, pines, sizeof s_pines) != 0) {
            s_pines     = *pines;
            s_arrancada = 0;
            memset(&s_info, 0, sizeof s_info);
        }
        s_configurado = 1;
    }
    return &s_backend;
}

const bpvm_sd_info_t* bpvm_sd_blk_info(void) {
    return s_arrancada ? &s_info : NULL;
}
