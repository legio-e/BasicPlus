/*
 * wire_v1.h (ESP32-S3 / ESP32-P4 por UART0) — la API del wire para estas placas.
 *
 * [V6/U2.1, 24-ago-2026] Casi todo lo que declaraba está ahora en
 * `include/bpvm_wire_v1.h`, que es el contrato de las dos capas: el CABLE (lo
 * pone cada familia) y el PROTOCOLO (común, `src/wire_v1_proto.c`). Aquí queda
 * sólo lo propio de este transporte.
 *
 * El I/O va por el driver UART de ESP-IDF en UART0 —el puerto del bridge
 * USB-UART, donde conecta el IDE— en vez de stdio/USB-CDC. Los logs de ESP-IDF
 * salen por la consola (USB-Serial-JTAG), así que el binario del wire en UART0
 * queda limpio.
 *
 * 📌 La P4 usa ESTE fichero por defecto (`BPVM_P4_WIRE=uart`); su variante TCP
 * es opcional y vive en `esp32p4/main/wire_v1_tcp.c`.
 *
 * ⚠️ Su guarda de inclusión decía `BPVM_PICO_WIRE_V1_H` — la del Pico, copiada
 * con el resto. Nunca mordió porque los dos ficheros no coinciden en ninguna
 * unidad de compilación, pero era una mina: incluir los dos habría hecho
 * desaparecer uno EN SILENCIO.
 */
#ifndef BPVM_ESP32_WIRE_V1_H
#define BPVM_ESP32_WIRE_V1_H

#include "bpvm_wire_v1.h"   /* el contrato: cable + protocolo */

#ifdef __cplusplus
extern "C" {
#endif

/* P-run-stop (#257) — lectura NO bloqueante de un byte del wire. Devuelve el
 * byte o -1 si no hay nada pendiente. La usa el poll de KILL que la VM invoca
 * entre quanta durante un RUN. Propia del transporte: no todas las familias la
 * resuelven igual (la Pico lo hace dentro de su recv). */
int wire_v1_try_getchar(void);

/* Configura UART0 (115200 8N1) y su driver. Llamar una vez, antes de usar el
 * wire. Propia de este transporte. */
void wire_v1_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_WIRE_V1_H */
