/* wire_v1_tcp.h — servidor del wire BPVM v1 sobre TCP para el ESP32-P4 (VM.3).
 *
 * Solo crea/abre el socket de escucha. El resto de la API es la compartida
 * wire_v1.h (recv_line / try_getchar / recv_bulk / send_*), implementada en
 * wire_v1_tcp.c sobre ese socket. El accept()/reconnect vive DENTRO de las
 * funciones de lectura, así que el dispatcher repl_esp32.c se reutiliza sin
 * cambios. */
#ifndef BPVM_P4_WIRE_V1_TCP_H
#define BPVM_P4_WIRE_V1_TCP_H

/* Crea socket + bind(INADDR_ANY:port) + listen. No bloquea (el accept ocurre
 * dentro de wire_v1_recv_line al primer mensaje). Llamar una vez, con la red
 * ya levantada. */
void wire_v1_tcp_server_init(int port);

#endif /* BPVM_P4_WIRE_V1_TCP_H */
