/*
 * bpvm_app.h — arranque de la VM BasicPlus en el STM32U575 (H9.1.2).
 */
#ifndef BPVM_APP_H
#define BPVM_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Arranca la VM: registra la salida por el VCP del ST-LINK (COM1 del BSP),
 * carga el Hello.mod embebido y lo ejecuta UNA vez. La salida del programa
 * BP (print ...) sale por el puerto serie, byte-idéntica a la VM-Java/host.
 *
 * Llamar UNA sola vez, DESPUÉS de BSP_COM_Init(COM1, ...) (el VCP ya inicializado).
 */
void bpvm_app_run_hello(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_APP_H */
