/*
 * bpvm_mods.h — LA REGLA del /lib de la imagen, común a las tres familias (#466).
 *
 * Cada firmware embebe su stdlib y la instala en /lib al arrancar. Antes lo hacía
 * cada familia con su bucle («sólo si falta»), y el aviso del /lib rancio (#422)
 * vivía en un fichero GENERADO del ESP32: la siguiente regeneración (#446) se lo
 * llevó sin que nadie lo viera. Aquí va la DECISIÓN, una vez; los generados y la
 * tabla de la Pico sólo recorren su tabla y logean lo que esto les devuelve.
 *
 * La regla (Eduardo, 2-sep): la versión de la stdlib la resuelve el SO, no la
 * comunicación. Por cada módulo embebido, en /lib:
 *   - falta                              → se instala
 *   - está y es de VERSIÓN anterior      → se repone     (versión = MAGIC: MOD6 < MOD7)
 *   - está, MISMA versión, CRC distinto  → se repone
 *   - está y es MÁS NUEVO                → se deja (y se dice)
 *   - está y es idéntico                 → nada, en silencio
 * Fuera de /lib (el Hello.mod de muestra de la Pico, en /app) sólo «si falta»:
 * /app es del usuario. Los ficheros del FS no tienen fecha: tienen versión y CRC,
 * y con eso se decide.
 *
 * Lee por la fachada bpvm_fs (stat/read_at/crc32), que es la misma en las tres;
 * ESCRIBE por el `put` de la familia, porque cada una tiene su cintura alrededor
 * (el ESP32 agrupa las escrituras para no reescribir la partición 14 veces).
 */
#ifndef BPVM_MODS_H
#define BPVM_MODS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BPVM_MODS_IGUAL     = 0,   /* ya estaba, idéntico: nada */
    BPVM_MODS_INSTALADO = 1,   /* faltaba: escrito */
    BPVM_MODS_REPUESTO  = 2,   /* estaba viejo o distinto: reescrito */
    BPVM_MODS_SE_DEJA   = 3,   /* estaba y no se toca: más nuevo, o fuera de /lib */
    BPVM_MODS_EN_ZONA   = 4,   /* #468: faltaba, y la zona de packs lo sirve con los mismos bytes: no se instala */
    BPVM_MODS_QUITADO   = 5,   /* #468: estaba la copia de la imagen, y la zona lo sirve igual: fuera de /lib */
    BPVM_MODS_FALLO     = -1   /* no se pudo escribir */
} bpvm_mods_res_t;

/* El `put` de la familia: 0 = escrito, -1 = fallo. Sobrescribe si ya existe. */
typedef int (*bpvm_mods_put_fn)(const char* path, const uint8_t* data, uint32_t len);

/* Sincroniza UN módulo embebido con el FS según la regla de arriba. `linea`
 * (opcional) recibe qué hizo y por qué, lista para el log; "" si no hay nada
 * que decir (el caso normal: idéntico). */
bpvm_mods_res_t bpvm_mods_sincronizar(const char* path, const uint8_t* data, uint32_t len,
                                      bpvm_mods_put_fn put, char* linea, size_t cap);

/* La versión de un módulo por su MAGIC ("MOD7" → 7); 0 si no empieza por "MOD". */
int bpvm_mods_version(const uint8_t* cabecera4);
/* V7/H2 (#523) — bytes de cabecera que hace falta leer para tener también la generación. */
#define BPVM_MODS_CABECERA 40
/* La generación (`gen`, be32 en el offset 36) de un módulo MOD8+; 0 si es anterior o no cabe. */
uint32_t bpvm_mods_gen(const uint8_t* m, uint32_t len);

/* V6/U4 — la tabla de la stdlib embebida de una imagen: la genera
 * scripts/regen_mods.sh (UN fichero de SÓLO DATOS por familia) y la recorre
 * bpvm_mods_instalar_tabla, una vez para las tres familias. */
typedef struct {
    const char*          path;   /* "/lib/Math.mod"; o "/app/Hello.mod", la muestra de la Pico */
    const unsigned char* data;
    uint32_t             len;
} bpvm_mod_embebido_t;

typedef void (*bpvm_mods_log_fn)(const char* linea);

/* Recorre la tabla aplicando la regla a cada entrada; cada línea que la regla
 * devuelve va a `log` (si no es NULL). Devuelve cuántos ESCRIBIÓ (instalados +
 * repuestos + quitados): el ESP32 lo usa para guardar la partición una sola vez. */
unsigned bpvm_mods_instalar_tabla(const bpvm_mod_embebido_t* t, unsigned n,
                                  bpvm_mods_put_fn put, bpvm_mods_log_fn log);

/* V7/H6 (#468) — CUANDO CAMBIA LA ZONA, /lib SE PONE AL DÍA EN EL ACTO. La familia
 * registra su instalación de la stdlib (la suya: la ESP32 agrupa las escrituras) y
 * el wire común de packs la vuelve a correr tras grabar, borrar o formatear un
 * pack: lo que la zona deja de servir vuelve a /lib desde la imagen sin esperar al
 * siguiente arranque, y lo que pasa a servir deja de tener copia. Sin registro (el
 * micro simulado, el host), no hace nada. */
void bpvm_mods_set_resync(void (*instalar)(void));
void bpvm_mods_resync(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_MODS_H */
