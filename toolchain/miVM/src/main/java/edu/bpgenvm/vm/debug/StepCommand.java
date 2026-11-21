// ============================================================
// StepCommand.java
// Comando que el cliente del debugger envía a la VM cuando ésta está
// pausada, para indicar cómo continuar. Vive del lado VM (no del IDE)
// porque la VM es la autoridad: bloquea al worker BP hasta recibirlo.
//
// <dl>
//   <dt>CONTINUE</dt><dd>Reanudar normalmente; sólo parar si otro
//      breakpoint dispara.</dd>
//   <dt>STEP_INTO</dt><dd>Parar en el próximo cambio de línea (entra
//      en funciones llamadas).</dd>
//   <dt>STEP_OVER</dt><dd>Parar en el próximo cambio de línea dentro
//      del MISMO frame o un frame padre (no profundiza en funciones
//      llamadas).</dd>
//   <dt>STEP_OUT</dt><dd>Parar al volver a un frame padre.</dd>
//   <dt>STOP</dt><dd>Terminar la sesión (TODO: hoy continúa hasta el
//      final natural; la señal de shutdown no está implementada).</dd>
// </dl>
// ============================================================
package edu.bpgenvm.vm.debug;

public enum StepCommand {
    CONTINUE, STEP_INTO, STEP_OVER, STEP_OUT, STOP
}
