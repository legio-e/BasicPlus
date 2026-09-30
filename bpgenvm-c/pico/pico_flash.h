/*
 * pico_flash.h — LA cintura de flash de la Pico (V7/H7, `U1.4`: decidido por Eduardo
 * el 30-sep, «la pequeña»).
 *
 * Los cuatro que escriben la flash —el FS (littlefs), el ENV A/B, la zona de packs y
 * el log— pasan por estas dos funciones, como en la STM32 (`stm32_flash.c`): un arreglo
 * aquí les llega a los cuatro a la vez. Antes cada uno llamaba al SDK por su cuenta, con
 * el candado en seis sitios.
 *
 * Dentro va lo que cada uno repetía, y lo que casi ninguno comprobaba:
 *  · el candado XIP (`flash_lock`): mientras el controlador de flash está ocupado nadie
 *    puede ejecutar desde ella —el otro núcleo, parado; las IRQ, fuera—;
 *  · las precondiciones del SDK, que en release NO se comprueban (`invalid_params_if`
 *    se compila fuera): una dirección torcida llega a la ROM sin una queja y lo grabado
 *    no es lo mandado (le pasó a los packs: board_mgr_pico.c);
 *  · y DÓNDE: sólo en la zona 2 (el ENV y el log, en el hueco que el UF2 no graba) o
 *    desde BP_PART_BASE (las particiones), y dentro de la flash. Lo demás es la imagen:
 *    se rechaza sin tocarla. Antes, eso sólo lo miraba el ENV.
 *
 * Offsets desde el principio de la flash (lo que pide el SDK), no direcciones XIP.
 * 0 = hecho; -1 = rechazado, sin tocar nada. Aquí no se escribe al log: el log también
 * graba por aquí, y quien llama sabe qué decir.
 */
#ifndef PICO_FLASH_H
#define PICO_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borra `len` bytes desde `off`: los dos, múltiplos del sector (FLASH_SECTOR_SIZE, 4 KB). */
int pico_flash_erase(uint32_t off, uint32_t len);

/* Programa `len` bytes de `src` en `off`: los dos, múltiplos de la página
 * (FLASH_PAGE_SIZE, 256 B). Sobre flash borrada: NOR sólo baja bits. */
int pico_flash_write(uint32_t off, const uint8_t* src, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* PICO_FLASH_H */
