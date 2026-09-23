/* hal_stubs.c — dobles de la VM para los tests que enlazan la HAL SUELTA (la SD,
 * el contrato de bloque): `bpvm_out` vive en interp.c y `bpvm_diag` en bpvm.c, y
 * enlazar la VM entera para probar la HAL arrastraría también la plataforma de
 * verdad — y esos tests tapan el reloj A CERO a propósito (ver test_sd.c).
 *
 * V7/H6.c G4.a1: hacía falta desde #478 (bpvm_out) y H4 (la tabla de
 * capacidades): con la lista de ficheros a mano, test-sd y test-blk llevaban
 * tiempo sin enlazar, fuera de las redes y sin que nadie lo viera. */
#include <stdarg.h>
#include <stdio.h>

void bpvm_out(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
void bpvm_diag(const char* fmt, ...) { va_list ap; va_start(ap, fmt); printf("[diag] "); vprintf(fmt, ap); printf("\n"); va_end(ap); }
