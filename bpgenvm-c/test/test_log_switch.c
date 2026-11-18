/* #423 — el interruptor: apagado no anade, y lo ya escrito se conserva.
 * V8 (18-nov, la tabla de la robustez): y lo urgente, siempre —log_printf_siempre y los avisos urgentes de la VM
 * (bpvm_diag_urgente) se apuntan aunque el log este apagado; lo normal de despues, no—. */
#include "bpvm_log.h"
#include "bpvm.h"      /* bpvm_diag_set_sink, bpvm_diag, bpvm_diag_urgente */
#include <stdio.h>
#include <string.h>
static uint8_t region[4096];
static uint32_t ahora(void) { return 0; }
static int rd(uint8_t* d, uint32_t n) { (void)d;(void)n; return -1; }
static int wr(const uint8_t* d, uint32_t n) { (void)d;(void)n; return 0; }
static char cap[4096]; static size_t caplen;
static void sink(const char* d, size_t n, void* u) { (void)u; memcpy(cap+caplen,d,n); caplen+=n; }
static const char* dump(void){ caplen=0; memset(cap,0,sizeof cap); log_dump(sink,0); cap[caplen]=0; return cap; }
static void diag_al_log(const char* linea) { log_printf("%s", linea); }   /* como el sink de las placas */
int main(void){
    bpvm_log_cintura_t c; memset(&c,0,sizeof c);
    c.now_ms=ahora; c.flash_read=rd; c.flash_write=wr;
    c.region_buf=region; c.region_size=sizeof region;
    bpvm_log_init(&c);
    int fallos=0;
    log_printf("arranque-1");                       /* encendido por defecto */
    if (!strstr(dump(),"arranque-1")) { puts("FAIL: el arranque no se registro"); fallos++; }
    else puts("  ok  : por defecto ENCENDIDO (el arranque siempre queda)");

    bpvm_log_set_enabled(0);
    log_printf("ejecucion-que-no-debe-salir");
    const char* d = dump();
    if (strstr(d,"ejecucion-que-no-debe-salir")) { puts("FAIL: apagado y aun escribe"); fallos++; }
    else puts("  ok  : apagado NO anade");
    if (!strstr(d,"arranque-1")) { puts("FAIL: apagar borro lo anterior"); fallos++; }
    else puts("  ok  : apagar NO borra lo ya escrito");

    bpvm_log_set_enabled(1);
    log_printf("depurando");
    if (!strstr(dump(),"depurando")) { puts("FAIL: no se vuelve a encender"); fallos++; }
    else puts("  ok  : se puede volver a encender");

    /* V8 (18-nov, la tabla de la robustez): lo urgente, siempre. */
    bpvm_log_set_enabled(0);
    log_printf_siempre("siempre-que-debe-salir");
    d = dump();
    if (!strstr(d,"siempre-que-debe-salir")) { puts("FAIL: apagado, log_printf_siempre no escribe"); fallos++; }
    else puts("  ok  : apagado, log_printf_siempre SI anade (el arranque, el autorun que no arranca)");
    bpvm_diag_set_sink(diag_al_log);
    bpvm_diag_urgente("urgente-que-debe-salir %d", 7);
    bpvm_diag("normal-que-no-debe-salir");
    log_printf("otra-que-no-debe-salir");
    d = dump();
    if (!strstr(d,"urgente-que-debe-salir 7")) { puts("FAIL: apagado, un aviso urgente de la VM no se apunta"); fallos++; }
    else puts("  ok  : apagado, un aviso urgente de la VM SI se apunta");
    if (strstr(d,"normal-que-no-debe-salir") || strstr(d,"otra-que-no-debe-salir")) {
        puts("FAIL: tras el aviso urgente, lo normal tambien se apunta con el log apagado"); fallos++;
    } else puts("  ok  : y lo normal de despues, sigue sin apuntarse");
    printf("[status=%s]\n", fallos?"FAIL":"OK");
    return fallos?1:0;
}
