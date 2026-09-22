/* pack_esp32.h — el PACK NATIVO en la familia ESP32 (V7/H6 P1): la tabla BIOS,
 * el ancla, el cargador y el bloque de RAM, COMUNES a S3, C3, C6, C5 y P4.
 *
 * Vivían en `esp32p4/main/` (`bios_p4.c`, `pack_p4.c`, V5/H7) y nunca fueron
 * del P4: lo único que cambia entre estos micros es si tienen PSRAM y cuántas
 * direcciones da su MMU, y las dos cosas ya se deciden en el común (el mapeo
 * subió el 8-sep; el bloque de RAM y la tabla, hoy). Mientras vivieron en el
 * directorio del vecino, el C3, el C6 y la C5 no podían cargar un pack nativo
 * —y `board_mgr_esp32.c` publicaba la RAM del pack bajo `#if
 * CONFIG_IDF_TARGET_ESP32P4`, que es nombrar un silicio en vez de una
 * capacidad (H4 §6). Corrección del mismo criterio de Eduardo (8-sep): «el
 * sistema debería ser idéntico, igual que el environment».
 */
#ifndef PACK_ESP32_H
#define PACK_ESP32_H

#include <stdint.h>

struct bpvm_bios;
struct bpvm_ancla;

/* La tabla BIOS de esta familia, YA VERIFICADA (NULL si tiene huecos, y en ese
 * caso `bios_esp32.c` ya dejó dicho en el log cuál falta). La usan los dos que
 * van juntos: el arranque, para comprobarla, y el cargador, para prestársela
 * al pack. */
const struct bpvm_bios*  bios_esp32_get(void);
/* El ancla, para que el arranque compare lo que ENCUENTRA con lo que SABE. */
const struct bpvm_ancla* bios_esp32_ancla(void);

/*
 * Lo que el firmware OFRECE al pack para sus estáticos, NO lo que el pack pide:
 * eso lo declara él en su cabecera (`data_bytes` + `bss_bytes`) y la escalera de
 * `bpvm_npack_check` los compara. Si un pack pidiera más, sale con su peldaño
 * TAMAÑO en vez de escribir fuera — o sea que quedarse corto NO corrompe nada,
 * sólo hace que el pack no cargue.
 *
 * ─── POR QUÉ 16 KB Y NO LOS 8192 DE LA PICO ───
 *
 * Porque el pack de RISC-V MIDE MÁS, y está medido (12-ago, `sqlite_rv.npack`):
 *
 *     .data 6.352 + .sdata   564 = 6.916 B de imagen inicial
 *     .sbss    68 + .bss   1.300 = 1.368 B a cero
 *                               ── 8.284 B
 *
 * Con los 8192 de la Pico el pack RISC-V sería RECHAZADO — por 92 bytes. Y el
 * mismo número ya se quedó corto antes: 7168 se pasó por OCHO al añadirle el
 * VFS. Dos veces justo es una advertencia, no una casualidad.
 *
 * ⚠️ `.sdata`/`.sbss` son de RISC-V y NO EXISTEN en ARM (datos pequeños). Es
 * justo el tipo de sección que se olvida al portar y que aquí ya está contada.
 *
 * 16 KB da ~2x sobre lo medido. Criterio de Eduardo (12-ago): *"16K está bien,
 * con margen para crecer si es necesario"*.
 */
#define PACK_RAM_BYTES   16384u

/*
 * EL BLOQUE DE RAM DEL CÓDIGO NATIVO — `[estáticos | arena]`, decidido en el
 * ARRANQUE y ANTES que el heap de la VM, para que sea el MISMO en cada arranque:
 * es la dirección contra la que el pack está SELLADO (`linked_ram`). Si saliera
 * de lo que quede libre podría bailar, y entonces un pack grabado funcionaría
 * una vez y luego no — el peor síntoma posible. 0 = no hay bloque (no se pidió
 * o no cupo): tampoco hay dónde poner los estáticos del pack, y el cargador lo
 * dice.
 *
 * ─── DE DÓNDE SALE, Y CON QUÉ CLAVE (V7/H6) ───
 *
 * Del ENV, con dos claves que nombran LA MISMA zona:
 *   · `packRam=<KB>` — la genérica: RAM para el código nativo, el que sea
 *     (LVGL, un driver de pantalla, SQLite…). Es la que manda si está.
 *   · `SQLite=<MB>`  — la de V5, que sigue valiendo: es el nombre público de
 *     la única arena que había, con su regla medida (`bpvm_sqlmem.h`: mínimo
 *     2 MB, ~40 % útil con MEMSYS5). Sin `packRam`, decide ésta.
 * Con PSRAM sale de la PSRAM (alineado a página, ver `pack_esp32_ram_init`);
 * sin PSRAM, de la SRAM interna dejando lo que la VM necesita como mínimo.
 * Hoy es UN bloque para UN pack (el primero válido de la zona); el reparto
 * entre varios códigos nativos —`ram_bytes` en la cabecera de cada `.npk`—
 * es el P4 del plan de H6 (V7_IDEAS §H6 2.5).
 */
extern uint8_t* s_pack_ram_base;
extern uint32_t s_pack_ram_size;    /* bytes del bloque entero (estáticos + arena) */
extern int      s_pack_ram_res;     /* bpvm_sqlite_res_t: por qué hay o no hay bloque */

/* Reserva el bloque (o dice por qué no). ANTES del heap de la VM. */
void    pack_esp32_ram_init(void);

/*
 * Mapea la zona de packs y la PUBLICA (para el IDE y para que se vean los .mod
 * y .mdn que lleve dentro). Lo llama el arranque; es barato y no salta a nada.
 * 0 si la zona quedó mapeada, -1 si no hay zona o falló el mapeo.
 */
int32_t pack_esp32_mapear(void);

/*
 * Carga y ejecuta el pack nativo de la zona de packs. Se dispara en el primer
 * `Run` (ver repl_esp32.c): es el único paso que puede colgar, y un cuelgue en
 * el arranque se repite en cada arranque (decisión de Eduardo, V5).
 *
 *   >= 0  se saltó, y es lo que devolvió el pack
 *   <  0  no se saltó; -valor es el peldaño (bpvm_npack_res_t)
 */
int32_t pack_esp32_cargar(void);

#endif /* PACK_ESP32_H */
