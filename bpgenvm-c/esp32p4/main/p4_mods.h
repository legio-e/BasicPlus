/* p4_mods.h — instala la stdlib embebida (Core, fresco) en /lib del FS.
 * Llamar tras fs_init(). Instala if-absent (no pisa lo que ya esté). */
#ifndef BPVM_P4_MODS_H
#define BPVM_P4_MODS_H

void esp32p4_mods_install(void);

#endif /* BPVM_P4_MODS_H */
