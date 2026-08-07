/* bpvm_sqlmem.h — el reparto de memoria para la BD (V5).
 *
 * La REGLA de cuánta memoria se le reserva a SQLite, en UN solo sitio, como
 * `bpvm_stack_region_bytes`. La usan el arranque (para reservar), el INFO (para
 * reportar) y el IDE (para saber dónde va a caer el pack). Si se copiara en cada
 * sitio, el día que cambie unos dirían una cosa y otros otra — que es
 * exactamente cómo el INFO acabó enseñando 171+171 (#335).
 *
 * De dónde salen los números (todo MEDIDO en el PC el 7-ago-2026, no estimado):
 *
 *  - MÍNIMO 2 MB. Y el motivo NO es la cantidad: es que el ordenador de SQLite
 *    pide UN bloque CONTIGUO, y MEMSYS5 es un alocador buddy. Con arena de 1 MB
 *    falla aunque el pico en uso sea muy inferior. Medido: 1 MB y 2 MB con el
 *    búfer por defecto (250 págs = 1.024.000 B) aguantan lo mismo — 5.000 filas;
 *    bajando ese búfer a 64 págs, 2 MB rinden IGUAL que 8 (1.000.000 de filas).
 *
 *  - Regla práctica: con MEMSYS5 cuenta con usar ~40% de lo que reserves
 *    (39-42% en todas las medidas). No es desperdicio, es cómo funciona el
 *    buddy: el bloque grande necesita su nivel libre.
 *
 *  - Los ~7 KB de datos estáticos de SQLite salen de ESTE mismo bloque
 *    (`[estáticos | arena]`), así que reservar 2 MB deja una arena de
 *    2 MB − 7 KB. La diferencia es irrelevante y evita un segundo bloque con su
 *    segunda dirección que sellar.
 *
 * Criterio de Eduardo (7-ago): PSRAM es REQUISITO para la BD, el bloque se
 * reserva en el arranque y "nosotros no lo tocamos nunca".
 */
#ifndef BPVM_SQLMEM_H
#define BPVM_SQLMEM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mínimo MEDIDO. Por debajo NO se activa: mejor un no limpio que arrancar y
 * morir a las 6.000 filas (criterio de Eduardo, mismo espíritu que el clamp
 * de #292). */
#define BPVM_SQLITE_MIN_MB   2

/* Lo que la VM tiene que conservar SIEMPRE. 128 KB es el bloque de la placa más
 * justa que hoy funciona (S3/STM32 → 64 KB de stacks + 64 de heap). Si la
 * reserva dejara menos, la BD gana y la VM pierde: eso no se hace en silencio. */
#define BPVM_VM_MIN_BYTES    (128u * 1024u)

/* Tope de cordura para el valor del entorno (lo escribe una persona). 4095 MB
 * evita el desbordamiento al multiplicar por 1<<20 en un size_t de 32 bits. */
#define BPVM_SQLITE_MAX_MB   4095

typedef enum {
    BPVM_SQLITE_OFF = 0,   /* ausente, 0 o negativo: no se pide BD. Silencio. */
    BPVM_SQLITE_OK,        /* reservado; *out_bytes tiene el tamaño            */
    BPVM_SQLITE_MUY_POCO,  /* se pidió por debajo del mínimo MEDIDO           */
    BPVM_SQLITE_NO_CABE    /* no cabe dejando la VM viable                     */
} bpvm_sqlite_res_t;

/*
 * Decide el bloque de la BD.
 *
 *   env_mb      — el valor de la clave `SQLite` del entorno, en MB.
 *                 <= 0 o ausente ⇒ OFF (y `bpvm_env_get_long(env,"SQLite",0)`
 *                 ya devuelve 0 si no está: no hace falta caso aparte).
 *   total_bytes — el bloque de memoria del que se reparte (en la Metro, la
 *                 ventana PSRAM entera; en una placa sin PSRAM, la SRAM libre).
 *   out_bytes   — [salida] bytes a reservar. 0 en todo lo que no sea OK.
 *
 * Devuelve el MOTIVO, no un booleano: el llamante tiene que poder decir POR QUÉ
 * no se activó. "No se pidió" y "se pidió mal" son cosas distintas y el usuario
 * necesita distinguirlas.
 */
bpvm_sqlite_res_t bpvm_sqlite_region(long env_mb, size_t total_bytes,
                                     size_t* out_bytes);

/* Texto fijo del motivo, EN PROSA, para el log de arranque. Nunca NULL. */
const char* bpvm_sqlite_res_str(bpvm_sqlite_res_t r);

/* El mismo motivo como CÓDIGO CORTO y estable ("off"/"ok"/"low"/"nofit"), para
 * el wire. Van separados a propósito:
 *  - la prosa es para que un humano lea el log, y puede cambiar de redacción;
 *  - el código lo parsea el IDE, y cambiarlo ROMPERÍA la compatibilidad.
 * Mezclarlos ataría el texto del log a la compatibilidad del protocolo.
 * Nunca NULL. */
const char* bpvm_sqlite_res_code(bpvm_sqlite_res_t r);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_SQLMEM_H */
