/*
 * pack_pico.h — dónde vive la RAM del pack nativo, y quién más tiene que saberlo.
 *
 * Aquí van el `.data` y el `.bss` del pack. Tiene que estar en una dirección
 * FIJA porque el SELLO del pack son DOS direcciones —flash y RAM— y las dos se
 * calculan en el PC al realojar. Un `static` normal viviría en `.bss` y se
 * movería en cada enlace; el PC no podría saberla y el sello no cuadraría nunca.
 *
 * ─── DE DÓNDE SALE: LA PSRAM SI LA HAY, Y LA SRAM SÓLO SI NO ───
 *
 * Criterio de Eduardo (7-ago): *"la reserva solamente hace falta si SQLite=0"*.
 * Y tiene razón — donde hay PSRAM sobra sitio, así que quitarle 4 KB a la SRAM
 * de la VM es un peaje que sólo hay que pagar cuando no queda otra:
 *
 *   CON PSRAM  → muerde del principio de la ventana PSRAM, ANTES que el bloque
 *                de la BD y que el heap de la VM. Misma disciplina que la BD:
 *                mordiendo primero, su dirección no depende de cuánta PSRAM
 *                lleve la placa ni de lo que valga `SQLite=<MB>`.
 *   SIN PSRAM  → los últimos 4 KB de la SRAM principal, y la VM se para justo
 *                debajo. Es el caso degenerado, no el normal.
 *
 * ⚠️ Y AQUÍ ESTUVO EL ERROR QUE CAZÓ EDUARDO, que conviene no repetir: la
 * primera versión ponía el bloque "arriba del todo" de la SRAM sin más. Pero en
 * una placa SIN PSRAM `vm_sram_region` le da a la VM **todo lo que hay entre el
 * final del `.bss` y el techo**, así que el bloque caía DENTRO del heap de la
 * VM. En la Metro no se habría visto nunca —ahí el heap se va a la PSRAM y la
 * SRAM queda libre—, sólo habría reventado en la Pico 2, con la MISMA imagen.
 * Por eso el techo de la VM y la base del bloque salen de la misma constante:
 * romper uno sin ver el otro deja de ser posible.
 */
#ifndef PACK_PICO_H
#define PACK_PICO_H

#include <stdint.h>

/* Techo de la SRAM principal del RP2350; coincide con `__HeapLimit`. El arranque
 * lo COMPRUEBA en vez de darlo por hecho: si algún día no coincidiera, el
 * reparto dejaría de ser cierto EN SILENCIO. */
#define PACK_SRAM_TECHO  0x20080000u

/* 7168 = lo que MIDIÓ la prueba A que necesita SQLite de estáticos (`ram=7168`
 * en el manifest). No es un número redondo a ojo: es el que hay.
 *
 * Y esto es lo que el firmware OFRECE, no lo que el pack necesita — eso lo
 * declara el pack en su cabecera (`data_bytes` + `bss_bytes`) y la escalera los
 * compara: si un pack pidiera más, sale con su peldaño TAMAÑO en vez de escribir
 * fuera. El pack mínimo pide 8 bytes y le sobra casi todo; SQLite pedirá esto. */
#define PACK_RAM_BYTES   7168u

/* Sólo para el caso SIN PSRAM: la cola de la SRAM principal. Con PSRAM la base
 * es el principio del bloque de la BD, y la fija el arranque en
 * `s_pack_ram_base`. Alineado a 8 porque ahí van estructuras del pack. */
#define PACK_RAM_SRAM_BASE  ((PACK_SRAM_TECHO - PACK_RAM_BYTES) & ~7u)

/* La base REAL, decidida en el arranque (PSRAM o SRAM). 0 = no hay sitio. Es la
 * que se le pasa a la escalera como `aqui_ram`, y contra la que el pack tiene
 * que estar sellado. */
extern uint8_t* s_pack_ram_base;

/* Carga y ejecuta el pack de la zona XIP. Ver pack_pico.c.
 *   >= 0  se saltó, y es lo que devolvió el pack
 *   <  0  no se saltó; -valor es el peldaño (bpvm_npack_res_t) */
int32_t pack_pico_cargar(void);

#endif /* PACK_PICO_H */
