/*
 * wire_v1.h — framing del wire BPVM v1 sobre USB CDC.
 *
 * Implementa el contrato de docs/BPVM_WIRE_PROTOCOL.md §2 (framing):
 *   - Mensajes JSON line-delimited (terminados con `\n`, UTF-8).
 *   - Bulk binario inline: tras el `\n` del JSON declara `"bulk":N`,
 *     vienen exactamente N bytes raw, sin separador.
 *
 * Las funciones de lectura usan getchar_timeout_us + vTaskDelay
 * (mismo patrón que repl.c) para no bloquear el scheduler FreeRTOS.
 *
 * Las funciones de escritura usan fwrite/fflush a stdout (que
 * stdio_usb redirige a USB CDC). Atómicas a nivel de mensaje porque
 * todo el wire v1 corre en una sola task (repl_v1).
 *
 * IMPORTANTE: en modo wire v1 NO se hace eco de los caracteres
 * entrantes (a diferencia del repl-texto humano). El IDE no lo
 * espera y rompería el framing.
 */
#ifndef BPVM_PICO_WIRE_V1_H
#define BPVM_PICO_WIRE_V1_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Buffer máximo para una línea JSON entrante. Los mensajes v1
 * típicos rondan 100-300 bytes; 2 KB deja margen para mensajes con
 * paths largos o specs de prompt grandes. */
#define WIRE_V1_LINE_MAX  2048

/* Lee una línea JSON desde stdin hasta encontrar '\n'. NO hace eco.
 *
 * `first_char_already_read` permite pasar un carácter que el caller
 * ya consumió (p.ej. el `{` que detectó el dispatcher). Si es -1,
 * lee la línea entera desde stdin.
 *
 * Devuelve la longitud (sin contar el '\n', sin null terminator) o
 * -1 si la línea excede WIRE_V1_LINE_MAX. El buffer NO se
 * null-terminate — el parser JSON usa (buf, len). */
int wire_v1_recv_line(int first_char_already_read,
                       char* buf, size_t buf_max);

/* Lee exactamente N bytes raw desde stdin. Usado tras un mensaje
 * con campo "bulk":N. Bloquea (con vTaskDelay 5ms) hasta tener
 * todos. Devuelve N si OK, -1 si N excede buf_max. */
int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max);

/* P-autorun (#256) — mutex de transmisión del wire. Cada línea enviada
 * por wire_v1_send_* es atómica frente a otros escritores (comm task con
 * OUTPUTs vs poll contestando HELLO/BUSY en-run). Un escritor que emita
 * una línea POR PARTES (v1_output_sink) debe envolverla en lock/unlock. */
void wire_v1_tx_lock(void);
void wire_v1_tx_unlock(void);

/* Escribe `len` bytes a stdout, seguidos de '\n', y hace fflush.
 * Llamar UNA vez por mensaje JSON. Línea atómica (tx_lock). */
void wire_v1_send_line(const char* data, size_t len);

/* Variante de conveniencia: escribe una C-string entera. */
void wire_v1_send_cstr(const char* cstr);

/* Escribe N bytes binarios raw (sin separador antes ni después).
 * Usar inmediatamente tras un wire_v1_send_line que haya declarado
 * "bulk":N. Hace fflush al final. */
void wire_v1_send_bulk(const uint8_t* data, size_t n);

/* ============================================================
 * Helpers de construcción de mensajes JSON.
 *
 * Todos escriben en `buf[off..buf_max)` y devuelven la nueva
 * posición del cursor o -1 si no cabe. El caller acumula encadenando
 * llamadas y al final llama wire_v1_send_line(buf, off).
 *
 * Diseñados para mensajes pequeños construidos linealmente —
 * mensajes grandes/dinámicos pueden usar snprintf directamente. */

/* Empieza un mensaje: "{\"type\":\"<type>\",\"id\":<id>".
 * No cierra la llave. */
int wire_v1_msg_begin(char* buf, size_t buf_max, size_t off,
                       const char* type, long id);

/* Empieza un evento (sin id): "{\"type\":\"<type>\"". */
int wire_v1_msg_begin_event(char* buf, size_t buf_max, size_t off,
                             const char* type);

/* Añade ",\"key\":<long>". */
int wire_v1_field_long(char* buf, size_t buf_max, size_t off,
                        const char* key, long value);

/* Añade ",\"key\":true|false". */
int wire_v1_field_bool(char* buf, size_t buf_max, size_t off,
                        const char* key, int value);

/* Añade ",\"key\":\"<value escaped>\"" — escapa \\, \", \n, \r, \t. */
int wire_v1_field_string(char* buf, size_t buf_max, size_t off,
                          const char* key, const char* value);

/* Añade ",\"bulk\":<n>". Llamar JUSTO antes de cerrar el mensaje
 * cuando va seguido de bytes binarios. */
int wire_v1_field_bulk(char* buf, size_t buf_max, size_t off, size_t n);

/* Cierra el mensaje con '}'. Devuelve la nueva posición o -1. */
int wire_v1_msg_end(char* buf, size_t buf_max, size_t off);

/* ============================================================
 * Senders pre-empaquetados para casos comunes.
 *
 * Cubren el 80% de mensajes salientes (reply OK simple, ERROR, etc.)
 * con una sola llamada. */

/* Envía un reply con solo el id y nada más en el cuerpo:
 *   {"type":"<type>","id":<id>}
 */
void wire_v1_send_reply_empty(const char* type, long id);

/* Envía un ERROR como respuesta a un request id:
 *   {"type":"ERROR","id":<id>,"code":"<code>","message":"<msg>"} */
void wire_v1_send_error(long id, const char* code, const char* message);

/* Envía un FATAL evento (sin id) — el servidor lo manda antes de
 * cerrar la conexión por error de protocolo:
 *   {"type":"FATAL","code":"<code>","message":"<msg>"} */
void wire_v1_send_fatal(const char* code, const char* message);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_WIRE_V1_H */
