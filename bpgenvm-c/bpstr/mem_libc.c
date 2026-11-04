/*
 * mem_libc.c — memcpy, memmove, memset, memchr y memcmp con los nombres de la libc, para TODO un firmware (V8 #785).
 *
 * Sólo en la STM32. Su libc es newlib-nano, que las trae de un byte por vuelta, y las paga todo: la VM (el heap, el GC,
 * los arrays, los búferes), el HAL, LVGL y littlefs. El enlazador toma lo que encuentra en un objeto antes que lo de una
 * biblioteca, así que con esto lo gana todo, también lo que no es nuestro. Se compila dentro de src/bpstr_vm.c, detrás
 * de bpstr.c: memcpy es otro nombre de bpstr_copia, y un alias va en el mismo fichero que su destino.
 *
 * En las demás familias, no (Eduardo, 3-nov; #785, el punto 2): en la P4 y la S3 el IDF pone las suyas en la RAM interna
 * (o usa las de la ROM) porque se llaman con la caché de la flash apagada, y en la RP2350 pasa lo mismo con el XIP
 * apagado. Éstas vivirían en la flash, y la placa caería justo al escribir en ella. En la STM32 la flash interna se lee
 * siempre: escribirla sólo detiene un momento a quien lee.
 *
 * Las pruebas, en prueba.c: en el PC con otro nombre (BPSTR_MEM_PRUEBA), para no cambiar las del sistema; y las de
 * verdad, en una imagen de prueba de la STM32. Hay que compilar sin que GCC cambie los bucles por llamadas
 * (-fno-tree-loop-distribute-patterns): un bucle de bpstr_rellena cambiado por un memset sería memset llamándose a sí
 * misma.
 */
#include "bpstr.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef BPSTR_MEM_PRUEBA
#define MEM(nombre) prueba_##nombre
void* prueba_memcpy(void* d, const void* s, size_t n);
void* prueba_memmove(void* d, const void* s, size_t n);
void* prueba_memset(void* d, int c, size_t n);
void* prueba_memchr(const void* p, int c, size_t n);
int   prueba_memcmp(const void* a, const void* b, size_t n);
#else
#define MEM(nombre) nombre
#endif

/* Una palabra en cualquier dirección: la M33 lee y escribe sin alinear por hardware, con la misma instrucción. */
typedef uint32_t __attribute__((__may_alias__, __aligned__(1))) suelta_t;
static inline uint32_t lee_suelta(const uint8_t* p) { return *(const suelta_t*) (const void*) p; }
static inline void escribe_suelta(uint8_t* p, uint32_t w) { *(suelta_t*) (void*) p = w; }

/* memcpy: la de Arm (bpstr.c), que devuelve el destino, como pide la libc. */
#if defined(BPSTR_COPIA_DE_ARM) && !defined(BPSTR_MEM_PRUEBA)
__asm__(
    "    .global     memcpy\n"
    "    .type       memcpy, %function\n"
    "    .thumb_set  memcpy, bpstr_copia\n");
#else
void* MEM(memcpy)(void* d, const void* s, size_t n) {
    bpstr_copia((uint8_t*) d, (const uint8_t*) s, (uint32_t) n);
    return d;
}
#endif

/* Hacia delante, con el destino por debajo del origen o sin solaparse. La de Arm vale: va siempre de abajo arriba, y lee
 * y escribe una palabra cada vez, así que lo que escribe ya lo ha leído. (Si algún día se cambia por otra que lea por
 * delante o copie de arriba abajo, esto deja de valer: las pruebas de memmove, en la placa, lo dirían.) */
static inline void hacia_delante(uint8_t* d, const uint8_t* s, uint32_t n) {
#if defined(BPSTR_COPIA_DE_ARM)
    bpstr_copia(d, s, n);
#else
    while (n--) *d++ = *s++;                               /* en el PC, sólo para las pruebas: su memcpy no promete eso */
#endif
}

void* MEM(memmove)(void* d, const void* s, size_t n) {
    uint8_t* dst = (uint8_t*) d;
    const uint8_t* src = (const uint8_t*) s;
    if (dst == src) return d;
    if ((uintptr_t) dst - (uintptr_t) src >= n) {          /* el destino no cae dentro del origen */
        hacia_delante(dst, src, (uint32_t) n);
        return d;
    }
    /* Hacia atrás, de arriba abajo: lee y escribe sin pisar lo que falta, que queda por debajo. El destino, alineado (lo
     * que más cuesta es escribir), y de 16 en 16. */
    dst += n;
    src += n;
    while (n && ((uintptr_t) dst & 3u)) { *--dst = *--src; n--; }
    while (n >= 16u) {
        uint32_t a = lee_suelta(src - 4), b = lee_suelta(src - 8), c = lee_suelta(src - 12), e = lee_suelta(src - 16);
        escribe_suelta(dst - 4, a);
        escribe_suelta(dst - 8, b);
        escribe_suelta(dst - 12, c);
        escribe_suelta(dst - 16, e);
        dst -= 16;
        src -= 16;
        n -= 16u;
    }
    while (n >= 4u) {
        dst -= 4;
        src -= 4;
        escribe_suelta(dst, lee_suelta(src));
        n -= 4u;
    }
    while (n) { *--dst = *--src; n--; }
    return d;
}

void* MEM(memset)(void* d, int c, size_t n) {
    bpstr_rellena((uint8_t*) d, (uint8_t) c, (uint32_t) n);
    return d;
}

/* C dice que memchr se para en cuanto encuentra, y n puede ser mayor que lo que hay (el `%.*s` de printf lo es):
 * bpstr_busca_byte lee de 8 en 8, y desde una dirección múltiplo de 8 esas lecturas no salen de los 8 bytes del que
 * busca. Así no se lee más allá, ni siquiera al final de la RAM. */
void* MEM(memchr)(const void* p, int c, size_t n) {
    const uint8_t* s = (const uint8_t*) p;
    const uint8_t b = (uint8_t) c;
    int32_t r;
    while (n && ((uintptr_t) s & 7u)) {
        if (*s == b) return (void*) (uintptr_t) s;
        s++;
        n--;
    }
    if (n == 0u) return NULL;
    r = bpstr_busca_byte(s, n > 0x7FFFFFFFu ? 0x7FFFFFFFu : (uint32_t) n, b);
    return r < 0 ? NULL : (void*) (uintptr_t) (s + r);
}

/* Aquí no vale bpstr_compara: con las dos direcciones desalineadas entre sí, ésa llama a memcmp. Una alineada y la otra
 * como venga, que lo hace el hardware. */
int MEM(memcmp)(const void* a, const void* b, size_t n) {
    const uint8_t* x = (const uint8_t*) a;
    const uint8_t* y = (const uint8_t*) b;
    while (n && ((uintptr_t) x & 3u)) {
        if (*x != *y) return *x < *y ? -1 : 1;
        x++;
        y++;
        n--;
    }
    while (n >= 8u && lee_suelta(x) == lee_suelta(y) && lee_suelta(x + 4) == lee_suelta(y + 4)) {
        x += 8;
        y += 8;
        n -= 8u;
    }
    for (; n; x++, y++, n--) if (*x != *y) return *x < *y ? -1 : 1;   /* en los 8 distintos, cuál; o la cola */
    return 0;
}
