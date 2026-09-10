/*
 * bpvm_wire_v1.h — EL CONTRATO del wire v1, separado del cable.
 *
 * ─── Por qué existe (V6/U2.1, 24-ago-2026) ───────────────────────────────
 *
 * `pico/wire_v1.c`, `esp32/main/wire_v1.c` y `esp32p4/main/wire_v1_tcp.c` se
 * llaman casi igual y llevaban fichado que «el 100 % de sus líneas difieren; son
 * dos programas distintos con el mismo nombre». **Medido palabra por palabra, es
 * al revés**: exponen las mismas 15 funciones, y ONCE son IDÉNTICAS en las tres
 * — más los cuatro helpers estáticos del JSON, también idénticos.
 *
 * Las cuatro que difieren de verdad son exactamente las cuatro que tocan el
 * cable: `recv_line`, `recv_bulk`, `send_line`, `send_bulk`. USB-CDC en la Pico,
 * UART0 con el driver de ESP-IDF en el S3, sockets lwIP en la P4.
 *
 * O sea que el fichero mezclaba DOS CAPAS y la frontera cae en un sitio exacto.
 * Esta cabecera la nombra:
 *
 *   ── PROTOCOLO ──  construir JSON. Cero hardware. Vive UNA vez, en
 *                    `src/wire_v1_proto.c`.
 *   ── TRANSPORTE ── mover bytes. Lo pone cada familia, y es lo único que
 *                    debería diferir.
 *
 * ⚠️ **La mina que esto desactiva**: los tres ficheros llevaban escrito *«los
 * builders JSON son COPIA… si cambia el formato del protocolo, mantener las tres
 * copias en sync»*. Estaban en sync — alguien lo hacía a mano y le salía bien.
 * Un comentario no es un mecanismo; esto sí.
 *
 * 📌 Y el criterio con el que se toca esto, de Eduardo: *«las comunicaciones son
 * nuestro cordón umbilical entre el PC y el micro»*. Pasitos pequeños, cada uno
 * verificado en placa.
 */
#ifndef BPVM_WIRE_V1_H
#define BPVM_WIRE_V1_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Buffer máximo para una línea JSON entrante. Los mensajes v1 típicos rondan
 * 100-300 bytes; 2 KB deja margen para paths largos o specs de prompt grandes. */
#define WIRE_V1_LINE_MAX  2048

/* ══════════════ TRANSPORTE — lo pone CADA FAMILIA ══════════════
 *
 * Estas cuatro son el cable, y son las únicas que deben diferir. Quien porte a
 * una placa nueva implementa exactamente esto y no toca nada más. */

/* V6/#473 — CUANTO SE ESPERA A UNA LINEA QUE SE HA QUEDADO A MEDIAS.
 *
 * El POR QUE, y no es teorico: sin plazo, un mensaje truncado (un byte perdido en
 * el VCP, un cliente que se cae a mitad de envio, ruido que empieza por '{') deja
 * el lector esperando un '\n' que no va a llegar — y entonces el SIGUIENTE mensaje
 * se concatena al trozo estancado, su '\n' cierra la linea, y el resultado ya no
 * empieza por '{': se descarta ENTERO. O sea que un truncado se COME el mensaje que
 * viene detras. Si el que se traga es un KILL, el sintoma es «le he dado a parar y
 * no ha parado», que no se diagnostica nunca.
 *
 * Lo tenia SOLO el STM32, y su header lo llamaba «matiz de esta familia que el
 * contrato no exige». Lo exige: es del protocolo, no del cable. */
#define WIRE_V1_ESTANCADA_MS 300u

/** Lee una línea JSON hasta el '\n'. NO hace eco (el IDE no lo espera y
 *  rompería el framing). `first_char_already_read` permite pasar un carácter ya
 *  consumido por el dispatcher; -1 = leer la línea entera.
 *  @return longitud sin el '\n' y sin terminador;
 *          **-1** si excede `buf_max` (linea demasiado larga → hay que avisar);
 *          **-2** si la linea se estanca `WIRE_V1_ESTANCADA_MS` sin recibir un byte
 *          (→ descartar EN SILENCIO lo acumulado y seguir; el IDE reintenta).
 *  ⚠️ Los dos negativos NO son lo mismo: contestar «linea demasiado larga» a una
 *  linea estancada manda a mirar donde no esta el problema. */
int  wire_v1_recv_line(int first_char_already_read, char* buf, size_t buf_max);

/** Lee exactamente `n` bytes crudos (tras un mensaje con campo `"bulk":N`).
 *  @return n si OK, -1 si `n` excede `buf_max`. */
int  wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max);

/** Escribe `len` bytes seguidos de '\n' y hace flush. UNA vez por mensaje JSON.
 *  La línea debe ser ATÓMICA frente a otros escritores. */
void wire_v1_send_line(const char* data, size_t len);

/** Escribe `n` bytes crudos, sin '\n' (la carga de un bulk). */
void wire_v1_send_bulk(const uint8_t* data, size_t n);

/* ══════════════ PROTOCOLO — común, en src/wire_v1_proto.c ══════════════
 *
 * Construyen JSON dentro de un buffer del llamante y devuelven el offset nuevo,
 * o -1 si no cabe. El patrón de uso es siempre el mismo:
 *
 *     off = wire_v1_msg_begin(buf, sizeof buf, 0, "TIPO", id);
 *     off = wire_v1_field_long(buf, sizeof buf, off, "clave", valor);
 *     off = wire_v1_msg_end(buf, sizeof buf, off);
 *     wire_v1_send_line(buf, (size_t) off);        <- el cable, de la familia
 */
int  wire_v1_msg_begin(char* buf, size_t buf_max, size_t off,
                       const char* type, long id);
int  wire_v1_msg_begin_event(char* buf, size_t buf_max, size_t off,
                             const char* type);
int  wire_v1_field_long(char* buf, size_t buf_max, size_t off,
                        const char* key, long value);
/* #466 — para lo que es un uint32 (CRC): `long` es de 32 bits en las placas y en
 * MinGW, y (long) 0x9C63… salía NEGATIVO por el wire. Sin signo, siempre. */
int  wire_v1_field_ulong(char* buf, size_t buf_max, size_t off,
                         const char* key, unsigned long value);
int  wire_v1_field_bool(char* buf, size_t buf_max, size_t off,
                        const char* key, int value);
int  wire_v1_field_string(char* buf, size_t buf_max, size_t off,
                          const char* key, const char* value);
int  wire_v1_field_bulk(char* buf, size_t buf_max, size_t off, size_t n);
int  wire_v1_msg_end(char* buf, size_t buf_max, size_t off);

/** Envía una cadena ya terminada en '\0' como una línea. */
void wire_v1_send_cstr(const char* cstr);

/** Respuesta sin campos: `{"type":"...","id":N}`. */
void wire_v1_send_reply_empty(const char* type, long id);

/** ERROR con código y mensaje. Si no cabe, cae a un ERROR mínimo — nunca calla. */
void wire_v1_send_error(long id, const char* code, const char* message);

/** FATAL (evento, sin id). Mismo criterio de degradación que `send_error`. */
void wire_v1_send_fatal(const char* code, const char* message);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_WIRE_V1_H */
