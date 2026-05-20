package com.mycompany.bpide;

/**
 * Comando que el IDE envía a la {@link DebugSession} (y de ahí al hook
 * del VM bloqueado) para que el thread BP continúe.
 *
 * <dl>
 *   <dt>{@link #CONTINUE}</dt><dd>Reanudar normalmente; sólo parar si
 *      otro breakpoint dispara.</dd>
 *   <dt>{@link #STEP_INTO}</dt><dd>Parar en el próximo cambio de línea
 *      (entra en funciones llamadas).</dd>
 *   <dt>{@link #STEP_OVER}</dt><dd>Parar en el próximo cambio de línea
 *      dentro del MISMO frame o un frame padre (no profundiza en
 *      funciones que se llamen).</dd>
 *   <dt>{@link #STEP_OUT}</dt><dd>Parar al volver a un frame padre.</dd>
 *   <dt>{@link #STOP}</dt><dd>Terminar la sesión de depuración (no
 *      implementado todavía: hoy continúa hasta el final).</dd>
 * </dl>
 */
public enum StepCommand {
    CONTINUE, STEP_INTO, STEP_OVER, STEP_OUT, STOP
}
