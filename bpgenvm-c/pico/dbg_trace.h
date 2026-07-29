/*
 * dbg_trace.h — migas de pan que SOBREVIVEN al reset (#326).
 *
 * POR QUÉ NO VALE EL LOG. El log persistente es la herramienta correcta para un
 * cuelgue, pero aquí la placa se RESETEA y para que el log llegue a la flash hay
 * que llamar a log_flush(), que "bloquea ~50 ms con IRQs OFF" (log.h). Poner eso
 * en el camino del depurador —cuatro veces por comando, con el USB CDC de por
 * medio— cambiaría los tiempos de lo que quiero medir, y encima podría provocar
 * el propio síntoma que investigo. El instrumento no puede falsear la medida.
 *
 * QUÉ SE HACE EN SU LUGAR. Una marca = un `store` de 32 bits en RAM que el
 * arranque NO borra (sección .uninitialized_data del SDK; el propio SDK la usa
 * para el doble-reset del BOOTSEL). Coste: nada. Sobrevive al reset por
 * watchdog/software, que es exactamente lo que pasa. Al arrancar, main() vuelca
 * el rastro AL LOG y limpia el magic: así el LOG de después del reset dice por
 * dónde iba el depurador cuando murió, sin haber tocado los tiempos.
 *
 * TEMPORAL: fuera en cuanto #326 esté cerrado.
 */
#ifndef BPVM_PICO_DBG_TRACE_H
#define BPVM_PICO_DBG_TRACE_H

#include <stdint.h>
#include "pico.h"                        /* get_core_num */
#include "hardware/structs/watchdog.h"   /* scratch[] = 2º portador */

#ifdef __cplusplus
extern "C" {
#endif

#define BP_TRACE_MAGIC 0x42505452u   /* "BPTR" — RAM sin inicializar = basura */
#define BP_TRACE_N     48u           /* últimas 48 marcas (anillo) */

/* Definidas en main.c, en RAM que el arranque no limpia. */
extern volatile uint32_t g_bp_trace_magic;
extern volatile uint32_t g_bp_trace_cnt;               /* total, puede pasar de N */
extern volatile uint32_t g_bp_trace[BP_TRACE_N];

/* Códigos. El byte alto lleva el núcleo: importa saber si quien murió era el
 * worker (core 1) o el de comunicaciones (core 0). */
#define BPT_RUN_ENTER     0x01u   /* +detalle = debugging (0/1) */
#define BPT_RUN_ARMED     0x02u
#define BPT_RUN_RETURNED  0x03u
#define BPT_RUN_DISARMED  0x04u
#define BPT_REPL_ENTRY    0x05u   /* CONTROL: se marca SIEMPRE, en todo arranque */
#define BPT_SEND_ENTER    0x10u
#define BPT_SEND_DONE     0x11u
#define BPT_CMD_WAIT      0x20u   /* justo ANTES de la lectura bloqueante */
#define BPT_CMD_GOT       0x21u   /* la lectura volvió */
#define BPT_CMD_PARSED    0x22u   /* JSON parseado; +kind en el byte 1 */
#define BPT_CMD_BADLINE   0x23u
#define BPT_CMD_BADJSON   0x24u

static inline void bp_trace2(uint32_t code, uint32_t detail) {
    if (g_bp_trace_magic != BP_TRACE_MAGIC) {   /* arranque en frío: estrenar */
        g_bp_trace_magic = BP_TRACE_MAGIC;
        g_bp_trace_cnt   = 0;
    }
    uint32_t v = (get_core_num() << 24) | ((detail & 0xFFu) << 8) | (code & 0xFFu);
    g_bp_trace[g_bp_trace_cnt % BP_TRACE_N] = v;
    g_bp_trace_cnt++;
    /* 2º portador, INDEPENDIENTE de la RAM: los scratch del watchdog (el SDK
     * sólo usa 4..7, los 0..3 están libres). Sobreviven a cosas distintas que
     * la RAM, así que si uno se pierde y el otro no, sabremos cuál falló. */
    watchdog_hw->scratch[0] = BP_TRACE_MAGIC;
    watchdog_hw->scratch[1] = v;
    watchdog_hw->scratch[2] = g_bp_trace_cnt;
}

static inline void bp_trace(uint32_t code) { bp_trace2(code, 0); }

/* ── El latido (#326) ──────────────────────────────────────────────────────
 * Eduardo NO tiene botón de reset, y con la placa colgada el reset del IDE
 * tampoco responde: su única salida es desenchufar. Y desenchufar borra la RAM
 * y los scratch, o sea que mis dos primeros portadores no podían sobrevivir a
 * la forma REAL de recuperar la placa. En vez de buscar un tercer portador, se
 * ataca la causa: que la placa se resetee SOLA. Un reset sí conserva el rastro
 * — y de paso Eduardo recupera la placa sin tocar el cable. */
void bp_wdt_arm(void);    /* arranca el latido (entrada del REPL) */
void bp_wdt_feed(void);   /* "sigo vivo" — desde las esperas legítimas */

/* Vuelca el rastro de la sesión ANTERIOR al log y lo da por consumido.
 * Llamar una sola vez al arrancar, antes de que nadie deje marcas nuevas. */
void bp_trace_report(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_DBG_TRACE_H */
