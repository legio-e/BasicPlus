/*
 * bpvm_fallo_cm33.c — el informe de un HardFault en Cortex-M33 (ver bpvm_fallo_cm33.h). Sólo compila en ARMv8-M
 * Mainline: el STM32 compila TODO src/, y en las demás familias este fichero queda vacío.
 */
#if defined(__ARM_ARCH_8M_MAIN__)

#include "bpvm_fallo_cm33.h"
#include "bpvm_log.h"

#include "FreeRTOS.h"
#include "task.h"

/* Lo que se registra, y por qué (de #447):
 *   · CFSR — el desglose del fallo. Son TRES registros apilados, y confundirlos hace que el informe MIENTA:
 *       MMFSR = CFSR[7:0] (memoria, MPU), BFSR = CFSR[15:8] (bus), UFSR = CFSR[31:16] (uso).
 *   · MMFAR/BFAR — la DIRECCIÓN que se intentó tocar, sólo si su bit VALID lo avala (si no, traen basura de antes).
 *   · PC y LR del marco apilado — DÓNDE reventó.
 *   · V7/H5 P2b.3 — la TAREA en curso y su pila con su límite: con los ejecutores de natives, «se desbordó la pila de
 *     bpvm-nat» es la mitad del diagnóstico. En ARMv8-M la tarea lleva su límite en PSPLIM (FreeRTOS lo pone al fondo
 *     de su pila): pasarlo es STKOF (UFSR bit 4 = CFSR bit 20), y si ocurre al apilar el marco de la excepción, el
 *     marco queda a medias —el PC puede no estar—.
 * `log_printf` escribe al anillo de RAM y `log_flush` lo persiste: se imprime todo y se vuelca UNA vez al final. */
void bpvm_cm33_fallo_informe(const struct bpvm_fault_frame* f, uint32_t exc_return, const char* cual) {
    volatile uint32_t* SCB_CFSR  = (volatile uint32_t*) 0xE000ED28u;
    volatile uint32_t* SCB_HFSR  = (volatile uint32_t*) 0xE000ED2Cu;
    volatile uint32_t* SCB_MMFAR = (volatile uint32_t*) 0xE000ED34u;
    volatile uint32_t* SCB_BFAR  = (volatile uint32_t*) 0xE000ED38u;
    uint32_t cfsr = *SCB_CFSR;
    uint32_t sp, lim;
    int en_psp = (exc_return & 4u) != 0u;

    /* Un fallo FATAL se registra aunque el log esté apagado (#423): es el post-mortem para el que el log existe, y
     * el contrato de #423 ya lo dice del arranque —«si la placa se muere… el rastro está»—. */
    bpvm_log_set_enabled(1);
    log_printf("FALLO %s: CFSR=%08lX HFSR=%08lX", cual,
               (unsigned long) cfsr, (unsigned long) *SCB_HFSR);
#if defined(configNUMBER_OF_CORES) && ( configNUMBER_OF_CORES > 1 )
    /* V7/H12 3.1, paso 4 — con dos núcleos, `xTaskGetSchedulerState` coge los cerrojos del kernel, y desde un fallo
     * —con el otro núcleo parado, quizá con ellos cogidos— se quedaría esperando, mudo. La tarea en curso se lee sin
     * cerrojo (NULL: todavía ninguna). */
    {
        TaskHandle_t t = xTaskGetCurrentTaskHandle();
        if (t != NULL) log_printf("FALLO  en la tarea '%s' (nucleo %lu)", pcTaskGetName(t),
                                  (unsigned long) portGET_CORE_ID());
    }
#else
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        log_printf("FALLO  en la tarea '%s'", pcTaskGetName(NULL));
#endif
    if (en_psp) {
        __asm volatile ("mrs %0, psp"    : "=r" (sp));
        __asm volatile ("mrs %0, psplim" : "=r" (lim));
    } else {
        __asm volatile ("mrs %0, msp"    : "=r" (sp));
        __asm volatile ("mrs %0, msplim" : "=r" (lim));
    }
    log_printf("FALLO  pila %s=%08lX, su limite %s=%08lX", en_psp ? "PSP" : "MSP", (unsigned long) sp,
               en_psp ? "PSPLIM" : "MSPLIM", (unsigned long) lim);
    if (f != 0)
        log_printf("FALLO  PC=%08lX LR=%08lX PSR=%08lX",
                   (unsigned long) f->pc, (unsigned long) f->lr, (unsigned long) f->psr);
    else
        log_printf("FALLO  PC y LR: no se saben (marco en la pila del manejador)");
    /* --- MMFSR: memoria (lo que dispara el MPU) --- */
    if (cfsr & (1u << 0))  log_printf("FALLO  causa: IACCVIOL (ejecutar donde no se puede)");
    if (cfsr & (1u << 1))  log_printf("FALLO  causa: DACCVIOL  <-- ESCRITURA/LECTURA prohibida (MPU)");
    if (cfsr & (1u << 3))  log_printf("FALLO  causa: MUNSTKERR");
    if (cfsr & (1u << 4))  log_printf("FALLO  causa: MSTKERR");
    if (cfsr & (1u << 5))  log_printf("FALLO  causa: MLSPERR (guardando la FPU)");
    if (cfsr & (1u << 7))  log_printf("FALLO  MMFAR=%08lX (la direccion prohibida)", (unsigned long) *SCB_MMFAR);
    else if (cfsr & 0x000000FFu) log_printf("FALLO  MMFAR NO valido");
    /* --- BFSR: bus --- */
    if (cfsr & (1u << 8))  log_printf("FALLO  causa: IBUSERR");
    if (cfsr & (1u << 9))  log_printf("FALLO  causa: PRECISERR");
    if (cfsr & (1u << 10)) log_printf("FALLO  causa: IMPRECISERR");
    if (cfsr & (1u << 11)) log_printf("FALLO  causa: UNSTKERR (desapilando el marco de la excepcion)");
    if (cfsr & (1u << 12)) log_printf("FALLO  causa: STKERR (apilando el marco de la excepcion)");
    if (cfsr & (1u << 13)) log_printf("FALLO  causa: LSPERR (guardando la FPU)");
    if (cfsr & (1u << 15)) log_printf("FALLO  BFAR=%08lX (direccion que se toco)", (unsigned long) *SCB_BFAR);
    else if (cfsr & 0x0000FF00u) log_printf("FALLO  BFAR NO valido");
    /* --- UFSR: uso --- */
    if (cfsr & (1u << 16)) log_printf("FALLO  causa: UNDEFINSTR (instruccion invalida)");
    if (cfsr & (1u << 17)) log_printf("FALLO  causa: INVSTATE");
    if (cfsr & (1u << 18)) log_printf("FALLO  causa: INVPC");
    if (cfsr & (1u << 19)) log_printf("FALLO  causa: NOCP (coprocesador ausente)");
    if (cfsr & (1u << 20)) log_printf("FALLO  causa: STKOF  <-- DESBORDAMIENTO DE PILA (paso su limite %s)",
                                      en_psp ? "PSPLIM" : "MSPLIM");
    if (cfsr & (1u << 24)) log_printf("FALLO  causa: UNALIGNED  <-- acceso desalineado");
    if (cfsr & (1u << 25)) log_printf("FALLO  causa: DIVBYZERO");
    log_flush();
    for (;;) { /* parados, pero habiendo dicho por que */ }
}

#endif /* __ARM_ARCH_8M_MAIN__ */
