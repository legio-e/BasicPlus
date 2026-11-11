/*
 * bt_esp32.h — V8/H2, paso 5 (11-nov): la radio Bluetooth de la familia ESP (NimBLE como anfitrión), publicada como
 * 'BT  '. El contrato, en bpvm_bt.h; el porqué, en bt_esp32.c.
 */
#ifndef BT_ESP32_H
#define BT_ESP32_H

/* Publica la tabla 'BT  ' (no enciende nada: eso lo pide lo común, bpvm_bt.c). */
void bt_esp32_publicar(void);

#endif /* BT_ESP32_H */
