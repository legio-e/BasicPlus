/* bpvm_examen.h — V7/H8 E1 (1-oct): ¿CARGARÍA ESTA VM ESTE FICHERO?
 *
 * Eduardo: «En rojo mostrar módulos/packs dañados o con un formato no válido.» El `LIST`
 * examina los ficheros cuyo formato conoce la VM —`.mod`, `.mdn`, `.pack`— mirando SÓLO la
 * cabecera y el tamaño, y la entrada lleva el motivo (`"mal"`) si no los cargaría. Cada examen
 * vive al lado de su cargador y usa SU regla (la misma función de cabecera), para que no haya
 * dos: lo que el árbol pinta en rojo es lo que la carga rechazaría. El contenido no se mira (un
 * `.mod` no lleva checksum; el CRC del contenido de un pack lo comprueba la grabación).
 *
 * Todos devuelven 0 si el fichero está bien y 1 si no, con el motivo en `motivo` (en prosa, lo
 * que el IDE enseña). `cab` son los primeros `n` bytes del fichero y `tam` su tamaño. Diseño:
 * V7_IDEAS §H8 «E1 — el rojo del árbol».
 */
#ifndef BPVM_EXAMEN_H
#define BPVM_EXAMEN_H

#include <stddef.h>
#include <stdint.h>

#include "bpvm_pack.h"   /* bpvm_pack_src_t */

#ifdef __cplusplus
extern "C" {
#endif

/** Lo que el examen lee del principio de cada fichero: la cabecera más larga (la de un pack). */
#define BPVM_EXAMEN_CABECERA 128u

/** Un `.mod`: el MAGIC que carga esta VM y que la cabecera y el tamaño cuadren (loader.c). */
int bpvm_mod_examina(const uint8_t* cab, size_t n, uint32_t tam, char* motivo, size_t cap);

/** Un `.mdn`: la cabecera que exige su cargador —magic, versión, ABI de helpers y, en una placa,
 *  arquitectura y receta— y que el fichero tenga lo que dice (mdn_loader.c). */
int bpvm_mdn_examina(const uint8_t* cab, size_t n, uint32_t tam, char* motivo, size_t cap);

/** Un `.pack` (uno o varios encadenados): la cabecera de cada pack —magic, versión de formato,
 *  CRC y tamaño— y que la cadena acabe donde acaba el fichero (bpvm_pack.c). */
int bpvm_pack_examina(const bpvm_pack_src_t* src, char* motivo, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_EXAMEN_H */
