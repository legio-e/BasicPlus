/*
 * c3_board_id.h — la identidad de placa del ESP32-C3 (ver el .c).
 *
 * Existe por lo mismo que el del P4: `main` tiene que LLAMAR a la instalacion
 * para que el enlazador meta el `.o`. En ESP-IDF un simbolo weak/strong no basta
 * — el objeto de un componente no entra si nadie lo referencia.
 */
#ifndef BPVM_C3_BOARD_ID_H
#define BPVM_C3_BOARD_ID_H

/* Instala la identidad del C3. Llamar UNA vez, antes de arrancar el REPL. */
void c3_install_board_id(void);

#endif /* BPVM_C3_BOARD_ID_H */
