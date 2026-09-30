/* bpvm_npack.h — el PACK NATIVO en flash: cabecera + escalera de validación.
 *
 * Un pack nativo es código C ajeno pre-enlazado en el PC, grabado en la zona de
 * packs y ejecutado XIP. Antes de saltar a él hay SIETE cosas que pueden estar
 * mal, y este fichero existe para que cada una falle CON SU NOMBRE en vez de
 * colgar la placa.
 *
 * Criterio de Eduardo (7-ago): *"hay que prever que algo puede fallar, así que
 * hay que poner chivatos que nos digan qué es lo que no funciona, para saber
 * dónde y no sólo funciona/no funciona"*.
 *
 * ─── LA ESCALERA ───
 *
 * Misma forma que `bpvm_boot`: se sube peldaño a peldaño y se para en el primero
 * que falla, dejando dicho CUÁL y POR QUÉ. Los seis primeros son comprobaciones
 * baratas sobre datos; el séptimo —saltar— es el único que puede colgar, y por
 * eso va aparte y con miga de pan (el llamante escribe en el log ANTES de saltar
 * y DESPUÉS: si se cuelga, el siguiente arranque muestra el paso sin resultado).
 *
 * ─── EL SELLO ───
 *
 * `linked_flash` / `linked_ram` dicen para qué direcciones se realojó el pack.
 * Si no coinciden con dónde está de verdad, el código lleva dentro punteros a
 * sitios equivocados y saltar sería un desastre MUDO. Comparar dos enteros lo
 * convierte en un mensaje: "regrábalo desde el IDE".
 *
 * Esto pasa cuando el usuario cambia `SQLite=<MB>` o `packRam=<KB>` en el
 * entorno (mueve o encoge la RAM) o cuando el pack se copia a otra placa con
 * otro reparto de flash. Es un caso NORMAL, no un error raro: por eso se detecta
 * y se explica en vez de confiar.
 *
 * ─── Y DESDE V7/H6.c (G1), EL SELLO DE RAM ES TAMBIÉN EL REPARTO ───
 *
 * Con UN motor en la zona, su sello de RAM era la base del bloque y la escalera
 * lo comparaba con ella. Con VARIOS (el de LVGL, el de SQLite, un driver de
 * pantalla…) cada uno necesita su trozo, y la pregunta era quién lo decide.
 *
 * Lo decide la PLACA, al grabarlo: `PACK_BURN_BEGIN` ya le decía al IDE contra
 * qué RAM sellar, y ahora le dice «lo primero libre por encima de los motores
 * vivos» (`bpvm_npack_ram_tope`). El reparto queda ESCRITO en cada sello, y el
 * cargador no calcula direcciones: comprueba que cada sello cae DENTRO del
 * bloque y que no pisa el de otro. Se consideró —y se descartó el 23-sep, con
 * Eduardo— repartir por el orden de la cadena con un `ram_bytes` en la
 * cabecera: habría pedido un campo nuevo y tocar el IDE, y borrar un pack de en
 * medio habría movido (y roto) a todos los de detrás. Así, lo ya grabado sigue
 * valiendo tal cual, y la RAM de un pack borrado se queda de hueco hasta
 * formatear, igual que su flash.
 */
#ifndef BPVM_NPACK_H
#define BPVM_NPACK_H

#include <stddef.h>
#include <stdint.h>

#include "bpvm_pack.h"       /* BPVM_PACK_NAME_LEN: el nombre del pack y de su entrada */

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_NPACK_MAGIC   0x42504E50u   /* 'BPNP' */
#define BPVM_NPACK_FORMAT  1u

/*
 * Tamaño EXACTO de la cabecera, y con él el reparto del pack en flash:
 *
 *     pack_base + 0                          cabecera  (64 B)
 *     pack_base + BPVM_NPACK_HDR_BYTES       imagen de flash  (flash_bytes)
 *     ... + flash_bytes                      imagen inicial de .data (data_bytes)
 *
 * Son 64 y no 56 —lo que suman los campos— a propósito: la imagen de código
 * empieza JUSTO detrás, así que este número es la base del código. Redondo,
 * alineado, y con dos huecos de reserva ya pagados para campos futuros sin
 * mover nada. El test comprueba que la struct mide esto de verdad.
 */
#define BPVM_NPACK_HDR_BYTES 64u

/* Bits de `flags`. */
#define BPVM_NPACK_F_THUMB 0x1u          /* la entrada es Thumb → bit 0 al saltar */

/*
 * Cabecera, al principio del pack en flash. TODO en little-endian nativo: el
 * pack se realoja en el PC PARA ESTA placa, así que no hay cruce de endianness
 * que valga (a diferencia del .pack de datos, que sí es big-endian portable).
 *
 * Tras la cabecera vienen, en orden: imagen de flash, imagen inicial de .data.
 * La tabla de relocalizaciones NO viaja: la consume el IDE al grabar. Un pack
 * grabado tiene `reloc_count == 0` — y eso mismo es la prueba de que ya se
 * realojó, sin campo aparte que pueda contradecirlo.
 */
typedef struct {
    uint32_t magic;          /* BPVM_NPACK_MAGIC                              */
    uint16_t format;         /* BPVM_NPACK_FORMAT                             */
    uint16_t arch;           /* MDN_ARCH_* (mismo catálogo que el .mdn)       */
    char     float_abi[8];   /* "softfp"/"hard"/"ilp32d"… NUL-terminado       */
    uint32_t entry_off;      /* offset de la entrada, SIN el bit Thumb        */
    uint32_t flags;          /* BPVM_NPACK_F_*                                */
    uint32_t flash_bytes;    /* tamaño de la imagen de flash                  */
    uint32_t data_bytes;     /* imagen inicial de .data (se copia a RAM)      */
    uint32_t bss_bytes;      /* a poner a CERO en RAM tras .data              */
    uint32_t linked_flash;   /* EL SELLO: para qué dirección se realojó       */
    uint32_t linked_ram;     /* ídem, lado RAM — y desde G1, SU trozo de RAM  */
    uint32_t reloc_count;    /* 0 = ya realojado (lo que se graba)            */
    uint32_t reserved[4];    /* 0 — hasta BPVM_NPACK_HDR_BYTES                */
} bpvm_npack_hdr_t;

/* Peldaños. El orden IMPORTA: cada uno supone que el anterior pasó. */
typedef enum {
    BPVM_NPACK_OK = 0,       /* los 6 peldaños de datos, superados            */
    BPVM_NPACK_E_MAGIC,      /* 1 — no es un pack nativo                      */
    BPVM_NPACK_E_FORMAT,     /* 2 — formato de otra época                     */
    BPVM_NPACK_E_ARCH,       /* 3 — compilado para otra ISA                   */
    BPVM_NPACK_E_ABI,        /* 4 — otra convención de coma flotante          */
    BPVM_NPACK_E_SELLO,      /* 5 — realojado para OTRA dirección             */
    BPVM_NPACK_E_SIN_RELOC,  /* 6 — llegó sin realojar (reloc_count != 0)     */
    BPVM_NPACK_E_TAMANO,     /* 7 — no cabe / entrada fuera de la imagen      */
    BPVM_NPACK_E_BIOS        /* 8 — la tabla que le íbamos a prestar cojea    */
} bpvm_npack_res_t;

/*
 * Sube la escalera. NO salta: sólo dice si se puede.
 *
 *   h            — la cabecera leída de flash (o NULL)
 *   aqui_flash   — dónde está el CÓDIGO de verdad (dirección que ve la CPU).
 *                  ⚠️ NO es la base del pack: es `pack_base + HDR_BYTES`. La
 *                  cabecera va delante, y `entry_off` cuenta desde el código.
 *                  Confundirlos son 64 bytes de desfase y un salto a mitad de
 *                  instrucción — por eso está dicho aquí y no se deduce.
 *   sitio_flash  — bytes que tiene el motor DETRÁS de su cabecera: ahí tienen
 *                  que caber la imagen de código y la de `.data`
 *   ram_lo/hi    — el bloque de RAM del código nativo, `[ram_lo, ram_hi)`. El
 *                  sello de RAM tiene que caer DENTRO, con su `.data` + `.bss`.
 *   bios_falta   — NULL si la BIOS está entera; si no, el nombre que falta
 *                  (se pasa ya calculado: quien conoce la BIOS es la placa)
 *
 * V7/H6.c G1 — se llamaba `bpvm_npack_check` y comparaba el sello de RAM con la
 * base del bloque (`==`). Cambia de nombre A PROPÓSITO además de cambiar de
 * regla: tiene los mismos seis parámetros, todos enteros, y un llamante que no
 * se enterase del cambio seguiría compilando con los argumentos en otro orden.
 */
bpvm_npack_res_t bpvm_npack_escalera(const bpvm_npack_hdr_t* h,
                                     uint32_t aqui_flash, uint32_t sitio_flash,
                                     uint32_t ram_lo, uint32_t ram_hi,
                                     const char* bios_falta);

/* Texto del peldaño, para el log. Incluye QUÉ HACER cuando lo hay: un "no" que
 * no dice cómo salir de él obliga a leer el código. Nunca NULL. */
const char* bpvm_npack_res_str(bpvm_npack_res_t r);

/* Lo que un motor ocupa de RAM: su `.data` + su `.bss`, redondeado a 16, que es
 * el paso con que se reparten los trozos (la `.data` pide alineación 8 por los
 * `double`; 16 es la de la pila y no cuesta nada). */
uint32_t bpvm_npack_ram_bytes(const bpvm_npack_hdr_t* h);

/*
 * ─── ENCONTRAR LOS MOTORES: la CADENA, no un barrido ─── (V7/H6.c G1)
 *
 * Hasta G1 el cargador BARRÍA la zona de 4 en 4 buscando el magic —la idea del
 * ancla de Eduardo (7-ago): «buscar, no acertar la dirección»— y se quedaba con
 * el PRIMER pack válido. Aquello arregló «el pack se mueve», pero tenía dos
 * cosas que con varios motores dejan de valer:
 *
 *   · barría a ciegas lo que la zona ya dice dónde está. Un `.npk` vive SIEMPRE
 *     dentro de un pack, y la cadena de packs es un índice: cada cabecera dice
 *     dónde empieza la siguiente y cada entrada dónde están sus bytes. Recorrerla
 *     es seguir buscando —ninguna dirección se supone—, pero leyendo el índice
 *     en vez de 13,7 MB (la zona de la C5) de 4 en 4;
 *   · se quedaba con el PRIMERO, y los `.mod` y los `.mdn` se resuelven con el
 *     ÚLTIMO (`bpvm_pack_find`: «el flujo de actualización es add nuevo +
 *     tombstone viejo»). Grabar un pack nuevo sin borrar el viejo ejecutaba el
 *     motor viejo con el `.mod` nuevo. Ahora la regla es la misma para los tres:
 *     si dos motores vivos se llaman igual, gana el de más adelante.
 */
#define BPVM_NPACK_MAX 8     /* motores que se describen de una zona: los mismos
                              * que marcas caben en el punto de encuentro
                              * (BPVM_BIOS_PACK_MAX) — más no podrían publicar */

typedef struct {
    const bpvm_npack_hdr_t* hdr;   /* su cabecera, por la vista de LECTURA      */
    uint32_t codigo;               /* dónde empieza su código: el `aqui_flash`  */
    uint32_t pack_off;             /* offset de SU pack en la zona              */
    char     pack[BPVM_PACK_NAME_LEN + 1];   /* el pack: el nombre que ve el usuario */
    char     npk [BPVM_PACK_NAME_LEN + 1];   /* la entrada `npk` (la del «gana el último») */
    uint32_t ram;                  /* su sello de RAM (0 si no hay cabecera)    */
    uint32_t ram_bytes;            /* bpvm_npack_ram_bytes (0 si no hay)        */
    bpvm_npack_res_t motivo;       /* el peldaño; OK = la escalera entera       */
    int8_t   eclipsado_por;        /* -1, o el índice del motor de MÁS ADELANTE
                                    * que se llama igual (y es el que se carga) */
    int8_t   pisa_a;               /* -1, o el índice del motor cargable cuya
                                    * RAM pisaría (dos sellos solapados: uno de
                                    * los dos se grabó con una imagen de antes
                                    * de G1, que daba a todos la base)          */
} bpvm_npack_carga_t;

/* ¿Se salta a éste? La condición en UN sitio: la leen las dos familias. */
int bpvm_npack_cargable(const bpvm_npack_carga_t* c);

typedef struct {
    int      vistos;     /* motores en packs VIVOS (puede pasar de `max`)      */
    int      cargables;  /* de los descritos, cuántos se pueden saltar         */
    uint32_t ram_tope;   /* la primera dirección libre por encima de todo sello
                          * vivo que cae dentro del bloque (alineada a 16). Es a
                          * la vez DÓNDE IRÁ el siguiente motor que se grabe y
                          * DÓNDE EMPIEZA la arena. Sin motores, la base.       */
} bpvm_npack_plan_t;

/*
 * Recorre la cadena de packs de `zona` y describe sus motores en `out` (hasta
 * `max`), EN ORDEN DE CADENA —que es el orden en que se grabaron y en el que se
 * saltará—, cada uno con su veredicto. Devuelve `vistos`.
 *
 *   · Sólo cuentan los packs VIVOS: un tombstone no se carga ni ocupa RAM (su
 *     hueco se reaprovecha si era el de arriba; si no, hasta formatear).
 *   · El tope se calcula con TODOS los motores vivos, también los que pasan de
 *     `max` o no superan la escalera: la RAM que un sello reclama está
 *     reclamada mientras el pack viva, se cargue o no.
 *   · Un motor no cargable no reserva nada frente a los demás: sólo los
 *     cargables se comparan entre sí por solapes.
 *
 * `plan` puede ser NULL; `out` puede ser NULL con `max` 0 (sólo el tope). No
 * escribe en la zona, no aloca, no salta: sólo mira.
 */
int bpvm_npack_planear(const uint8_t* zona, uint32_t zbytes,
                       uint32_t ram_base, uint32_t ram_bytes,
                       const char* bios_falta,
                       bpvm_npack_carga_t* out, int max,
                       bpvm_npack_plan_t* plan);

/* Sólo el tope. Es lo que `PACK_BURN_BEGIN` le dice al IDE como `ramBase` del
 * motor que va a grabar: la misma regla que el cargador, porque es la misma
 * función por dentro. Sin bloque (`ram_base` 0) devuelve 0. */
uint32_t bpvm_npack_ram_tope(const uint8_t* zona, uint32_t zbytes,
                             uint32_t ram_base, uint32_t ram_bytes);

/* La dirección a la que hay que saltar: entrada + base + bit Thumb si toca.
 * En UN sitio porque olvidar el bit 0 es hard fault inmediato, y es justo el
 * tipo de detalle que se copia mal al segundo sitio donde se escriba. */
uint32_t bpvm_npack_entry_addr(const bpvm_npack_hdr_t* h, uint32_t aqui_flash);

/* ── V7/H7 (Boot 2, fase 2; 30-sep) — EL CARGADOR, común a las tres familias ─────────
 *
 * Era la misma copia en pack_pico.c, pack_esp32.c y pack_stm32.c: el plan, la arena, la
 * `.data` y la `.bss` de cada motor, la miga de pan y el salto —unas 160 líneas iguales—.
 * Lo propio de cada familia son tres datos —la BIOS que presta, el bloque de RAM de los
 * estáticos y cómo volcar el log antes de saltar— y, en la ESP32, releer la cabecera
 * cuando un motor no carga. La zona es la MONTADA (fase 1: el arranque la montó con el FS).
 *
 * La guarda «una vez» va AQUÍ DENTRO. Vivía en el REPL de cada familia, y el ancla de la
 * BIOS apuntaba al cargador SIN ella: una segunda carga copiaría otra vez la `.data` y la
 * `.bss` encima de un motor vivo. Marca cuando se saltó a ALGUNO, como hacían las tres: si
 * no se saltó a ninguno, el siguiente RUN lo reintenta —lo que se quiere tras grabar el
 * primer pack, sin reiniciar—. Un motor que no carga deja el «casi» de Eduardo
 * (30-sep), con su nombre y el porqué (bpvm_pack_aviso_motor). Vive en
 * bpvm_npack_carga.c y no aquí: este fichero sólo decide, no salta. */
struct bpvm_bios;
typedef struct {
    const struct bpvm_bios* bios;   /* NULL: la BIOS de la placa tiene huecos (ya lo dijo quien la dio) */
    uint8_t*                ram;    /* el bloque de RAM de los motores: estáticos y arena */
    uint32_t                ram_bytes;
    void (*log)(const char* linea);
    void (*flush)(void);            /* el log a la flash ANTES de saltar: la miga de pan */
    /* opcional: lo que la familia añade cuando un motor no carga (la ESP32 relee la
     * cabecera por esp_partition_read y la compara con el mapeo) */
    void (*explicar_mas)(const bpvm_npack_carga_t* c, int i, const uint8_t* zona);
} bpvm_npack_familia_t;

/* Carga TODOS los motores de la zona montada. Tras saltar a alguno, las siguientes llamadas
 * devuelven lo mismo sin tocar nada. >= 1: a cuántos saltó; < 0: -peldaño del último
 * rechazado, o MAGIC si no había ninguno. El detalle, al log. */
int32_t bpvm_npack_cargar_zona(const bpvm_npack_familia_t* f);

/* Olvida la guarda y el aviso: SÓLO para las pruebas de host. En placa, cargar otra vez es
 * reiniciar la placa (lo que ya hay en RAM no se descarga). */
void bpvm_npack_carga_reinicia(void);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_NPACK_H */
