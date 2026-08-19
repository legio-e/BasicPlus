/*
 * packglue.h — la tabla BIOS de HOST para probar el pack en el PC.
 *
 * En la placa, quien le presta la tabla al pack es el firmware
 * (`pico/bios_pico.c`, `esp32p4/main/bios_p4.c`). En el PC no hay firmware, asi
 * que este fichero hace de doble: las mismas 29 ranuras, implementadas sobre la
 * libreria estandar.
 *
 * REESCRITO el 19-ago-2026. El original vivia en `notas/v5-sqlite-prueba/H/` —
 * fuera del arbol y fuera de git— y se perdio al limpiar. Eduardo, al saberlo:
 * *«un source de VM-C que no esta en su sitio y tampoco se ha subido al Git no
 * es un fallo, son como minimo 2»*. Ahora esta en los dos.
 */
#ifndef PACKGLUE_H
#define PACKGLUE_H

#include "bpvm_bios.h"

/** La tabla, ya rellena. Su direccion no cambia: se le puede prestar al pack. */
const bpvm_bios_t* packglue_bios(void);

/** Silencia el log del BIOS (1) o lo devuelve (0). El programa de prueba lo
 *  apaga porque sus lineas taparian la salida del propio programa. */
void packglue_callar(int callar);

#endif /* PACKGLUE_H */
