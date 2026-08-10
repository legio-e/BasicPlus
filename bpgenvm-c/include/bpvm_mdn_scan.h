/*
 * bpvm_mdn_scan.h — buscar el `.mdn` de cada módulo cargado y registrar sus
 * thunks. UN sitio, no uno por familia.
 *
 * ─── POR QUÉ EXISTE ────────────────────────────────────────────────────────
 *
 * Este bucle estaba escrito CUATRO veces —Pico, ESP32, STM32 y el micro
 * simulado— con el mismo cuerpo y distinta cintura. Mientras sólo buscaba en el
 * FS eso era duplicación tolerable; en el momento en que hay que buscar también
 * en la ZONA DE PACKS deja de serlo: cuatro copias es cuatro oportunidades de
 * que una se quede atrás, y el día que pasara no lo notarías hasta que ESA
 * familia fallara. (Mismo patrón que «un .c nuevo del core: alta en los 5
 * builds».)
 *
 * Así que el bucle, el orden de búsqueda y los mensajes viven AQUÍ. Lo único
 * que cambia por micro es de dónde sale la RAM ejecutable cuando el `.mdn`
 * viene del FS: arena en el Pico y el STM32, `heap_caps_malloc(MALLOC_CAP_EXEC)`
 * + mantenimiento de cachés en el ESP32, un puntero del FS simulado en el
 * boardsim. Eso se pasa como callback y ya.
 *
 * ⚠️ ESTADO (10-ago): sólo el Pico llama a esto. Las otras tres familias
 * conservan su bucle propio, INTACTO. Se migran cuando toque tocarlas con placa
 * delante — entonces es cambiar su bucle por una llamada, no reescribirlo.
 * Mientras tanto: tres copias vivas, y está dicho aquí para que no sorprenda.
 *
 * ─── EL ORDEN DE BÚSQUEDA: EL PUENTE SIGUE A SU MÓDULO ─────────────────────
 *
 *   módulo del PACK  →  1º el pack,  2º el FS
 *   módulo del FS    →  1º el FS,    2º el pack
 *
 * Criterio de Eduardo, y el motivo es de COHERENCIA, no de comodidad: *«lo que
 * está en un pack busca primero dentro del pack y si no lo encuentra, busca
 * fuera. Es lo que garantiza la coherencia; si por lo que sea hay un .mdn
 * fuera y lo encuentra primero, cosas que funcionan siempre podrían dejar de
 * funcionar.»*
 *
 * Un pack se graba como un CONJUNTO —motor, módulo y puente, una versión— y en
 * eso está su valor: mientras nadie pueda meter una pieza suelta en medio, es
 * imposible que el puente no case con su motor. La regla contraria (el FS
 * gana siempre) reabriría justo el agujero que el pack venía a cerrar.
 *
 * Y el reverso vale igual: si el módulo lo subiste tú al FS, su puente se
 * busca en el FS primero. Cada uno manda en lo suyo.
 *
 * Si hay dos, **se dice cuál se toma y cuál queda tapado**. Un eclipse mudo es
 * la receta para pasarse media hora mirando código que no se ejecuta.
 *
 * ─── LO QUE EL PACK REGALA: CERO RAM ───────────────────────────────────────
 *
 * Del FS hay que copiar los bytes a un sitio ejecutable, porque el registro es
 * ZERO-COPY: los thunks se apuntan DENTRO del buffer y tiene que seguir vivo
 * todo el run. Del pack no hace falta nada: la zona ya está mapeada y es
 * ejecutable (XIP), así que el `.mdn` se ejecuta EN SITIO. No es un detalle
 * menor en un micro — es la diferencia entre gastar arena y no gastarla. Y
 * precedente hay: el motor entero de SQLite ya corre XIP desde su pack.
 */
#ifndef BPVM_MDN_SCAN_H
#define BPVM_MDN_SCAN_H

#include <stdint.h>

struct bpvm;

/**
 * Trae del FS los bytes del `.mdn` de un módulo, si lo hay.
 *
 * @param user    lo que se pasó a bpvm_mdn_escanear
 * @param nombre  el nombre YA compuesto ("Modulo.mdn"), listo para resolver
 * @param len     [out] tamaño en bytes de lo devuelto
 * @return puntero a los bytes en memoria **ejecutable**, que debe seguir vivo
 *         durante todo el run; o NULL si no existe o no cupo. Si devuelve NULL
 *         por no caber, que lo DIGA la propia cintura: aquí se trata igual que
 *         "no está", y un descarte por tamaño en silencio no se distinguiría.
 */
typedef const uint8_t* (*bpvm_mdn_del_fs_fn)(void* user, const char* nombre,
                                             uint32_t* len);

/** Saca un mensaje por donde corresponda en esa familia (log, consola, wire).
 *  Puede ser NULL: entonces el escaneo es mudo, que es peor pero legal. */
typedef void (*bpvm_mdn_decir_fn)(void* user, const char* msg);

/**
 * Recorre los módulos cargados en `vm`, busca el `.mdn` de cada uno —en el orden
 * de arriba, que depende de dónde salió el módulo— y registra sus thunks con
 * bpvm_load_mdn().
 *
 * NO limpia el registro AOT: eso es del ciclo de vida del llamante, que en unas
 * familias además libera buffers. Llama tú a bpvm_aot_clear() antes.
 *
 * @return cuántos .mdn se cargaron con éxito (0 es normal: un programa que no
 *         usa native no lleva ninguno).
 */
int bpvm_mdn_escanear(struct bpvm* vm,
                      bpvm_mdn_del_fs_fn del_fs,
                      bpvm_mdn_decir_fn decir,
                      void* user);

#endif /* BPVM_MDN_SCAN_H */
