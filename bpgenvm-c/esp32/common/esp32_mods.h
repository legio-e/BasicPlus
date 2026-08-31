/*
 * esp32_mods.h — stdlib core embebida (pre-instalada en /lib al boot).
 *
 * La implementación (arrays de bytes + tabla) está en esp32_mods.c, GENERADO
 * por scripts/regen_esp32_mods.sh desde los .mod de bpstdlib. No editar el .c
 * a mano: re-ejecuta el script tras recompilar la stdlib.
 *
 * Gemelo de stm32_mods.h. El ESP32 no embebía la stdlib (solo hello_mod) y
 * dependía de que el IDE subiera cada .mod; pero el IDE no resuelve las deps
 * TRANSITIVAS (p.ej. Gpio -> Pico/Core), así que importar Gpio fallaba al
 * enlazar. Embeberla como en las otras dos familias lo arregla.
 */
#ifndef ESP32_MODS_H
#define ESP32_MODS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Pre-instala la stdlib core en /lib del FS (si no está ya). Llamar una
 * vez al boot, tras fs_init(), antes del REPL. Idempotente. */
void esp32_mods_install(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_MODS_H */
