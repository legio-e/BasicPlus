/*
 * aot_funcs.c — hook de trazas del cargador .mdn en el Pico.
 *
 * H13 hallazgo 12 (6-ago-2026) — ESTE FICHERO ERA UN AGREGADOR DE ANDAMIOS.
 * Tenía dos etapas de carga AOT anteriores a la buena, y las dos se han
 * retirado (mismo criterio que #340):
 *
 *   - Stage 1 (#157/#160): `aot_Bench_register()` — el AOT de Bench.bp
 *     COMPILADO DENTRO del firmware. Servía para validar que un thunk nativo
 *     podía secuestrar una función BP. Validado hace un año.
 *   - Stage 2 (#158 fase C): `embedded_bench_mdn.c` — un .mdn de 124 bytes
 *     horneado en la imagen, de mayo. Cuando los helpers AOT pasaron a ABI 2
 *     (#302 paso 2) el blob se quedó en ABI 1, así que el gate (#284) lo
 *     rechazaba EN CADA RUN con `MDN: RECHAZADO — ABI 1, esta VM habla 2`:
 *     el gate haciendo su trabajo, pero pareciendo un error en cada ejecución.
 *     Tercer artefacto generado que se queda rancio dentro de una imagen.
 *
 * Lo que hace el AOT de verdad —y lo único que queda— es el **stage 3**
 * (#158 fase D, `repl_v1.c`): para cada módulo cargado busca su `.mdn` en el
 * FS y registra sus thunks. Ese es el camino que usa el IDE, el que se prueba
 * en las tandas y el que dio 113× en ARM y 116× en RISC-V. No necesita nada
 * horneado: no tenía por qué arrastrar dos etapas más por delante.
 *
 * Queda aquí el hook de trazas porque el stage 3 sí lo usa.
 */

#include "log.h"
#include <stdarg.h>
#include <stdio.h>

/* H9.5 — implementación FUERTE del hook de trazas del mdn_loader (que es
 * weak no-op en src/mdn_loader.c, compartido entre ports): en el Pico las
 * trazas del .mdn van al log persistente. */
void bpvm_mdn_log(const char* fmt, ...) {
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    log_printf("%s", buf);
}
