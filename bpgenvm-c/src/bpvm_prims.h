/*
 * bpvm_prims.h — las primitivas que COMPARTEN un builtin (o un opcode) del intérprete y su helper tipado del AOT
 * (V7/H5 P2b, 26-sep: la opción 3 de Eduardo).
 *
 * El AOT traduce unos pocos builtins calientes (charAt, charCodeAt, substring, intToString, len, los newXArray)
 * a helpers tipados, más rápidos que el despachador genérico (medido en el PC: 44 ms frente a 96, 5 millones de
 * charCodeAt). Eran COPIAS de su builtin, y una copia se queda atrás sin que nada lo diga (#569: charAt fuera de
 * rango daba "" con el AOT y un RuntimeError interpretado). Ahora hay UNA implementación y dos entradas: el
 * helper conserva su velocidad y la paridad sale por construcción. Cada entrada sólo pone lo suyo: de dónde
 * saca los argumentos (la pila BP o los de C) y cómo lanza un error (builtin_throw, BPVM_RT_THROW, o el límite de
 * fallos de la native).
 *
 * Las cadenas llegan como la DIRECCIÓN física del objeto (0 = null): cada entrada resuelve su handle. Las que
 * reservan devuelven 0 y dejan en *out el handle nuevo, ya registrado; si fallan devuelven -1 y dejan en `err` el
 * texto EXACTO del error (el de miVM).
 */
#ifndef BPVM_PRIMS_H
#define BPVM_PRIMS_H

#include <stddef.h>
#include "bpvm_internal.h"

/* `len(s)`: la longitud en codepoints (H2). */
int32_t bpvm_prim_str_len(bpvm_t* vm, uint32_t s);

/* `charCodeAt(s, i)`: el codepoint i; 0 si s es null o i está fuera de rango. */
int32_t bpvm_prim_char_code_at(bpvm_t* vm, uint32_t s, int32_t i);

/* `charAt(s, i)`: una cadena con el codepoint i ("" si s es null); fuera de rango, un error (BUG-7b). */
int bpvm_prim_char_at(bpvm_t* vm, uint32_t s, int32_t i, bpref_t* out, char* err, size_t n);

/* `substring(s, desde, hasta)`: [desde, hasta) en codepoints, recortado como BP ("" si s es null). LEE el origen
 * DESPUÉS de reservar: quien la llama tiene que mantenerlo vivo para el GC (el builtin lo deja en la pila BP,
 * #350; la native lo tiene en sus variables de C). Por eso recibe el handle y lo vuelve a resolver. */
int bpvm_prim_substring(bpvm_t* vm, bpref_t s, int32_t desde, int32_t hasta, bpref_t* out, char* err, size_t n);

/* `intToString(v)`. */
int bpvm_prim_int_to_string(bpvm_t* vm, int32_t v, bpref_t* out, char* err, size_t n);

/* Un array nuevo de `size` elementos de `ancho` bytes y tipo `tipo`, a cero. Con un tamaño negativo, el texto
 * del error es pre + el número + post: el de miVM para cada forma de crearlo (el builtin `newIntArray`, el opcode
 * `NEWARRAY_I8`…). Sin memoria —del heap o de la tabla de handles (#430)—, «No space in heap». */
int bpvm_prim_nuevo_array(bpvm_t* vm, int32_t size, uint32_t ancho, int tipo, const char* pre, const char* post,
                          bpref_t* out, char* err, size_t n);

/* #573 — el texto de un índice fuera de rango al leer o escribir un elemento de array: el de miVM para el opcode
 * `op` que el intérprete ejecuta en ese acceso (OP_ALOAD, OP_ALOAD_I64, OP_ASTORE_I8…; bpvm_opcodes.h). Un array null
 * tiene longitud 0. Lo usan los diez opcodes (interp.c) y los quince helpers array_load_* / array_store_* del AOT,
 * cada uno con SU opcode: el que el compilador emitiría para ese tipo de elemento. */
void bpvm_prim_texto_indice(char* err, size_t n, int op, int32_t idx, uint32_t len);

#endif /* BPVM_PRIMS_H */
