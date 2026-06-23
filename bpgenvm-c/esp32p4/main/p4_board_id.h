/* p4_board_id.h — instala la identidad de placa del ESP32-P4 (esp32p4) en el
 * REPL del wire para que INFO/HELLO no reporten el default S3. Llamar una vez,
 * antes de repl_esp32_run(). */
#ifndef BPVM_P4_BOARD_ID_H
#define BPVM_P4_BOARD_ID_H

void p4_install_board_id(void);

#endif /* BPVM_P4_BOARD_ID_H */
