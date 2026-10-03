/*
 * bpvm_flash_zona.h — V7/H10, el board_mgr común (#637, 3-oct): LA REGLA DE LO QUE SE PUEDE ESCRIBIR DE LA FLASH, en un
 * sitio y probada en el PC (`make test-flash`).
 *
 * Cada familia escribe su flash por su cintura (pico_flash.c, stm32_flash.c; las ESP32, por particiones del IDF, que
 * ya guardan). La cintura es quien guarda: así la guarda vale para TODOS los que escriben —el ENV, el log, el FS, los
 * packs— y para cuando se escriben, también en el ARRANQUE (la `imagen.fecha` de #626). Hasta el 3-oct la STM32 sólo
 * comprobaba su `.ld` en el `handle` del gestor: su primitiva borraba cualquier página que existiera, firmware
 * incluido (#637).
 *
 * Dos piezas, sin estado:
 *   - una ZONA es [ini, fin): lo que se escribe tiene que caer ENTERO dentro de una;
 *   - el layout CASA si los símbolos del `.ld` son los de la cabecera de la familia: si no, no se escribe nada (un
 *     `.ld` y una cabecera que divergen borrarían código en silencio).
 */
#ifndef BPVM_FLASH_ZONA_H
#define BPVM_FLASH_ZONA_H

#include <stdint.h>

typedef struct {
    uintptr_t ini;   /* la primera dirección que se puede escribir */
    uintptr_t fin;   /* la primera que ya no */
} bpvm_flash_zona_t;

/* ¿[dir, dir + len) cae ENTERO en una de las `n` zonas? 1 sí, 0 no. Un tramo vacío vale; uno que da la vuelta, no. */
static inline int bpvm_flash_zona_ok(uintptr_t dir, uintptr_t len, const bpvm_flash_zona_t* zonas, int n) {
    if (len == 0) return 1;
    uintptr_t fin = dir + len;
    if (fin < dir) return 0;
    for (int i = 0; i < n; i++)
        if (dir >= zonas[i].ini && fin <= zonas[i].fin) return 1;
    return 0;
}

/* ¿Casan el `.ld` y la cabecera? Lo que da el enlazador (`ld_*`) contra lo que dice la cabecera: el log empieza donde
 * dice, las particiones también, y el firmware acaba antes del log. 1 sí, 0 no. */
static inline int bpvm_flash_layout_casa(uintptr_t ld_log, uintptr_t ld_part, uintptr_t ld_fin_firmware,
                                         uintptr_t log, uintptr_t part) {
    return ld_log == log && ld_part == part && ld_fin_firmware <= log && log < part;
}

#endif /* BPVM_FLASH_ZONA_H */
