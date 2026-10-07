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

/* V7 #659 — la traza en varias piezas (bpvm_out.h), también de interp.c: estos tests son de UN hilo, así que cada
 * pieza sale en el acto, como la de bpvm_out. */
#include "bpvm_out.h"
void bpvm_out_linea(bpvm_out_linea_t* l, const char* fmt, ...) { va_list ap; (void) l; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
void bpvm_out_linea_fin(bpvm_out_linea_t* l) { (void) l; }

/* V7/H6.c G4.a2 — el árbitro de los buses usa el mutex y la condvar de la
 * plataforma. Estos tests son de UN hilo y tapan el reloj a cero (ver test_sd.c),
 * así que no enlazan la plataforma de verdad: los dobles no bloquean nada y la
 * espera «vence» en el acto — con un solo hilo, un bus tomado no lo va a soltar
 * nadie, y el árbitro tiene que decirlo con nombre, no colgarse. */
#include "bpvm_platform.h"
int  bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m)    { *m = (void*) 1; return 0; }
void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m) { (void) m; }
void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m)    { (void) m; }
void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m)  { (void) m; }
int  bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c)      { *c = (void*) 1; return 0; }
void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c)   { (void) c; }
int  bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms) {
    (void) c; (void) m; (void) ms; return 1;
}
void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) { (void) c; }
