/*
 * stm32_mods.h — stdlib core embebida (pre-instalada en /lib al boot).
 *
 * La implementación (arrays de bytes + tabla) está en stm32_mods.c, GENERADO
 * por scripts/regen_stm32_mods.sh desde los .mod de bpstdlib. No editar el .c
 * a mano: re-ejecuta el script tras recompilar la stdlib.
 */
#ifndef STM32_MODS_H
#define STM32_MODS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Pre-instala la stdlib core en /lib del FS (si no está ya). Llamar una
 * vez al boot, antes del REPL. Idempotente. */
void stm32_mods_install(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_MODS_H */
