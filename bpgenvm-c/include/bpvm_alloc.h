/*
 * bpvm_alloc.h — reservas de C del núcleo, con GUARDIÁN DE FIN DE RUN (#339).
 *
 * IDEA DE EDUARDO: no le preguntes al sistema cuánta memoria libre queda —
 * pon un CONTADOR. Se marca el contador al arrancar el programa y, al acabar,
 * *todo bloque con secuencia posterior a esa marca es memoria que se quedó sin
 * limpiar*. No hay que restar nada ni fiarse de nadie: o está en la lista, o no.
 *
 * Por qué un contador y no el reloj: la hora obligaría a una cintura por micro
 * (cada familia tiene su fuente de tiempo) y además no hace falta ordenar en el
 * tiempo — sólo distinguir "antes / después del arranque". El contador es
 * exacto, monotónico y gratis. Resultado: el guardián es **el mismo código en
 * las 5 familias**, sin una sola línea propietaria. Es la diferencia entre
 * medir y adivinar: preguntar la memoria libre del sistema da un número que
 * ensucian otras tareas y la fragmentación; esto da la lista de bloques.
 *
 * Y no dice sólo CUÁNTO falta: dice QUIÉN. Cada bloque guarda el fichero y la
 * línea donde se reservó, así que el aviso es "loader.c linea 175 se dejó 3
 * bloques (240 B)" y no "faltan 240 B".
 *
 * DÓNDE ENGANCHA: bpvm_init marca, bpvm_destroy barre. Son UNA función portable
 * cada una, así que las 5 implementaciones del RUN (test/main.c, bpvm_sim,
 * pico/repl_v1, esp32/repl_esp32, stm32_repl) lo heredan sin tocarlas — y sigue
 * valiendo igual cuando #344 las unifique en una sola.
 *
 * DOS FAMILIAS DE BLOQUE, DOS CONTADORES (idea de Eduardo): no es lo mismo lo
 * que reserva la VM para el programa que lo que reserva la plataforma para el
 * SO (mutex, cond, handles de thread). Se mezclarían en la misma lista y el
 * aviso sería confuso. Con dos bases MUY separadas, el rango de la secuencia
 * ya dice de quién es el bloque, sin un campo extra ni una segunda lista:
 *
 *     [1 .. 2^62)          reservas de PLATAFORMA  (bpvm_malloc_os y compañía)
 *     [2^62 .. )           reservas de la VM       (bpvm_malloc y compañía)
 *
 * Así el guardián puede decir "de la VM no quedó nada" con independencia de lo
 * que tenga viva la plataforma — y de paso resuelve solo el problema del huevo
 * y la gallina: el candado del propio registro se crea con un mutex, que
 * reserva memoria; como esa reserva es de PLATAFORMA, nunca se cuenta como fuga
 * del programa.
 *
 * REGLA DE USO: dentro del núcleo se reserva y se libera SIEMPRE con estas
 * macros. Mezclar (reservar con malloc y liberar con bpvm_free, o al revés)
 * corrompe la lista — el bloque lleva cabecera y el puntero que ve el llamante
 * NO es el que devolvió el sistema. bpvm_free vale para las dos familias: sabe
 * de cuál es por la secuencia que lleva dentro.
 */
#ifndef BPVM_ALLOC_H
#define BPVM_ALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Familia del bloque = de qué contador sale su secuencia. */
typedef enum {
    BPVM_ALLOC_VM = 0,   /* memoria de la VM para el programa: la que vigilamos */
    BPVM_ALLOC_OS = 1    /* primitivas de la plataforma: mutex, cond, threads */
} bpvm_alloc_kind_t;

/* --- Reservas del núcleo. Misma semántica que las de libc. --------------- */
void* bpvm_alloc_raw  (size_t n, int zero, bpvm_alloc_kind_t k, const char* file, int line);
void* bpvm_realloc_at (void* p, size_t n, const char* file, int line);
char* bpvm_strdup_at  (const char* s, const char* file, int line);
void  bpvm_free       (void* p);   /* vale para las dos familias */

#define bpvm_malloc(n)      bpvm_alloc_raw((n), 0, BPVM_ALLOC_VM, __FILE__, __LINE__)
#define bpvm_calloc(n, sz)  bpvm_alloc_raw((size_t)(n) * (size_t)(sz), 1, BPVM_ALLOC_VM, __FILE__, __LINE__)
#define bpvm_realloc(p, n)  bpvm_realloc_at((p), (n), __FILE__, __LINE__)
#define bpvm_strdup(s)      bpvm_strdup_at((s), __FILE__, __LINE__)

/* Las de PLATAFORMA. Sólo las usa la capa platform_*.c de cada familia. */
#define bpvm_malloc_os(n)   bpvm_alloc_raw((n), 0, BPVM_ALLOC_OS, __FILE__, __LINE__)

/* --- Guardián ------------------------------------------------------------ */

/* Marca el contador DE LA VM ahora y devuelve la marca. La llama bpvm_init como
 * primera cosa que hace, antes de reservar nada suyo: así su propia estructura
 * entra dentro de la ventana y su fuga (si la hubiera) se vería. */
uint64_t bpvm_alloc_mark(void);

/* Barre los bloques DE LA VM vivos con secuencia >= `mark` y REPORTA. La llama
 * bpvm_destroy al final del todo, cuando ya ha liberado hasta la propia
 * estructura del vm — de ahí que la marca se copie a una local antes.
 *
 * Los bloques de PLATAFORMA se cuentan aparte y se mencionan como dato, nunca
 * como fuga: su ciclo de vida no es el del programa.
 *
 * Habla SIEMPRE, haya fuga o no. Un guardián que sólo abre la boca cuando hay
 * problema no se distingue de uno averiado (lección de #326: todo instrumento
 * necesita su control). Devuelve los bytes de la VM que se quedaron por el
 * camino (0 = la memoria volvió a su sitio).
 */
uint64_t bpvm_alloc_sweep(uint64_t mark);

/* Dónde se cuenta. Por defecto stderr; el firmware lo enchufa al log
 * persistente (log_printf) para que el aviso sobreviva a un reset. La línea
 * llega SIN newline. */
typedef void (*bpvm_alloc_report_fn)(const char* linea);
void bpvm_alloc_set_report(bpvm_alloc_report_fn fn);

/* Bloques vivos ahora mismo, por familia — para tests y diagnóstico. */
uint32_t bpvm_alloc_live_blocks(bpvm_alloc_kind_t k);
uint64_t bpvm_alloc_live_bytes (bpvm_alloc_kind_t k);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ALLOC_H */
