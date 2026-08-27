/*
 * bpvm_repl.h — EL CONTRATO del REPL wire v1, separado de la familia.
 *
 * ─── Por qué existe (V6/U3, 26-ago-2026) ─────────────────────────────────────
 *
 * El REPL estaba TRIPLICADO: `repl_v1.c` (Pico, 2054 líneas), `repl_esp32.c`
 * (S3+P4, 1344) y `stm32_repl.c` (920) — 4.318 líneas sin contrato, y el 100 %
 * de las asimetrías que nos han mordido: verbos que faltan en una familia,
 * respuestas distintas al mismo caso, arreglos que no viajan.
 *
 * El censo U3.0/U3.0b (FICHAS) midió las dos cosas que hacían falta ANTES de
 * escribir esto: qué verbos tiene cada familia (la Pico es superconjunto
 * estricto de las tres), y qué hay DEBAJO de cada verbo ausente — porque un
 * verbo enrutado a nada no es un verbo (pregunta de Eduardo, y hasta la Pico
 * tenía verbos-fachada).
 *
 * ─── El reparto ──────────────────────────────────────────────────────────────
 *
 *   COMÚN (src/bpvm_repl.c)  lo que solo toca contratos ya comunes: el wire
 *                            (bpvm_wire_v1.h), el FS (bpvm_fs.h), el log
 *                            (bpvm_log.h), el RTC... Interpretar "PING" no es
 *                            hardware.
 *   FAMILIA                  el cable, RUN (por ahora), y los verbos de SU
 *                            hardware (BOOTSEL...). Donde no hay soporte, la
 *                            respuesta es un ERROR CON NOMBRE, nunca silencio.
 *
 * ─── Cómo se usa ─────────────────────────────────────────────────────────────
 *
 *   El dispatcher de la familia, tras parsear type/id/obj, pregunta PRIMERO:
 *
 *       if (bpvm_repl_dispatch(type, id, &obj)) return;   // atendido
 *       ...sigue su cadena de verbos propios...
 *
 *   Si una familia necesita sombrear un verbo del común (no debería), lo
 *   atiende ANTES de llamar aquí. La migración es por GRUPOS y cada grupo se
 *   verifica en placa antes del siguiente — criterio de Eduardo para todo el
 *   cordón: *«pasitos pequeños, cada cambio verificado en placa; nos importa
 *   más la seguridad que la velocidad»*.
 *
 * ─── Qué verbos atiende hoy el común ─────────────────────────────────────────
 *
 *   Grupo 1 (meta):  PING · TIME · LOG_DUMP · LOG_CLEAR
 *
 *   Divergencias que este grupo UNIFICA (medidas el 26-ago, U3.0b):
 *     · TIME sin `epochSec`: la Pico daba INVALID_PARAM; S3/STM32 contestaban
 *       OK en silencio. Gana la Pico: el error se DICE.
 *     · LOG_CLEAR: la Pico dejaba la marca «LOG cleared via wire v1» en el log
 *       nuevo; el STM32 no. Se deja siempre: fecha el corte al leer un dump.
 *     · el sink de LOG_DUMP del STM32 PERDÍA chunks en silencio si el escape
 *       no cabía en su buffer; el del común trocea y no pierde.
 */
#ifndef BPVM_REPL_H
#define BPVM_REPL_H

#include "json_min.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════ LA CINTURA: lo que cada familia pone ══════════════
 *
 * Medido antes de diseñarla (26-ago), comparando el INFO de las tres: **18
 * campos son comunes a todas** y 11 son sólo del RP2350 (variante, packs/XIP,
 * los cinco de SQLite, `floatAbi`, dos del RTOS). De ahí la forma:
 *
 *   · los 18 comunes viajan como VALORES en `bpvm_repl_info_t` — la familia
 *     rellena una struct, no implementa 18 funciones;
 *   · lo propio entra por UN gancho (`info_extra`), que añade sus campos al
 *     mismo mensaje;
 *   · y sólo son punteros a función las ACCIONES (formatear, guardar) y los
 *     contadores del FS, que no tienen fachada común todavía.
 *
 * ⚠️ Un campo que una familia no sepa: se deja a 0 / cadena vacía, NO se omite.
 * El IDE lee por nombre y un campo ausente y uno a cero se distinguen; que la
 * forma del mensaje sea SIEMPRE la misma es justo lo que este hito persigue. */

typedef struct {
    const char* unique_id;        /* serie del micro, ya formateado */
    const char* board_name;
    const char* reset_reason;
    unsigned    arch;             /* el tag del `.mdn` que ejecuta esta imagen */
    unsigned long cpu_hz;
    unsigned long uptime_ms;
    long        temp_milli_c;
    int         gpio_count, pio_count, pwm_slices, adc_channels;
    unsigned long flash_bytes, sram_bytes, psram_bytes;
    unsigned long vm_heap_bytes, vm_stack_bytes;
    unsigned long fs_total_bytes, fs_used_bytes;
} bpvm_repl_info_t;

typedef struct {
    /** Rellena los 18 campos comunes. Obligatoria. */
    void (*info)(bpvm_repl_info_t* out);

    /* ── Lo que HELLO tiene de propio, y sólo eso ──────────────────────────
     * El resto del saludo (protoVersion, la forma del mensaje) es del común.
     *
     * ⚠️ `server_build` NO se calcula en el común, y es a propósito: un
     * `__DATE__` puesto en `src/bpvm_repl.c` sería la fecha en que se compiló
     * ESE fichero — común y rara vez recompilado—, o sea una fecha que no
     * identifica la imagen. Ya nos costó un diagnóstico falso una vez. Que lo
     * ponga la familia no lo arregla del todo (sigue siendo la fecha de SU
     * fichero, no la del enlace), pero al menos no empeora. */
    const char* server_name;      /* "bpvm-pico", "bpvm-stm32"… */
    const char* server_build;     /* sello de compilación de la familia */
    const char* capabilities;     /* el array JSON literal: "[\"META\",…]" */

    /* ── PUT: el scratch y la política de persistencia ─────────────────────
     * El buffer lo pone la FAMILIA y no el común, por dos razones medidas: su
     * tamaño tiene restricciones propias (el STM32 lo usa además de scratch del
     * sector de env, la Pico lo comprueba con un assert de compilación), y una
     * familia pequeña no tiene por qué pagar 8 KB que no usa. */
    unsigned char* put_buf;
    unsigned long  put_buf_size;

    /* Qué hacer cuando una subida ha aterrizado bien. NO es lo mismo en todas:
     * el STM32 persiste el FS (salvo bajo /lib/, que el embebido reinstala en
     * cada boot — así se ahorra un erase+program por Run), y la Pico no hace
     * nada. Es política de familia, así que la decide la familia. NULL = nada. */
    void (*after_put)(const char* path);

    /** Añade los campos PROPIOS de la familia al mensaje a medio construir.
     *  Recibe el buffer y el offset actual; devuelve el offset nuevo, o -1 si
     *  no cabe. Puede ser NULL: la mayoría de familias no tienen extras. */
    int (*info_extra)(char* buf, unsigned long buf_max, int off);

    /** Contadores del FS para DF. Obligatorias (no hay fachada común aún). */
    unsigned long (*fs_total_bytes)(void);
    unsigned long (*fs_used_bytes)(void);
    int           (*fs_file_count)(void);

    /** Formatear el FS. NULL = esta familia no lo soporta (y el común
     *  responderá con un error CON NOMBRE, no con «type no implementado»). */
    int (*fs_format)(void);

    /** Persistir el FS. NULL = no hace falta (littlefs persiste al cerrar):
     *  el común contesta OK, que es la verdad, no un no-op disfrazado. */
    int (*fs_save)(void);
} bpvm_repl_ops_t;

/** La familia registra su cintura UNA vez, antes de atender el primer mensaje.
 *  Sin esto, los verbos que la necesitan responden error con nombre. */
void bpvm_repl_set_ops(const bpvm_repl_ops_t* ops);

/** Atiende `type` si es un verbo del común. Devuelve 1 si lo atendió (la
 *  respuesta ya salió por el wire), 0 si no es suyo y la familia debe seguir
 *  con su cadena. `obj` es el mensaje ya parseado (para los parámetros). */
int bpvm_repl_dispatch(const char* type, long id, const json_obj_t* obj);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_REPL_H */
