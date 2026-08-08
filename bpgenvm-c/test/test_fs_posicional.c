/*
 * test_fs_posicional.c — V5/H2: `write_at` y `truncate` de la fachada.
 *
 * ─── POR QUÉ ESTE TEST EXISTE ───
 *
 * Estas dos operaciones nacieron para que quepa una base de datos encima del
 * sistema de ficheros, y una BD las usa de la forma que MÁS se equivoca uno al
 * implementarlas: escribir en medio sin tocar lo de alrededor. El fallo típico
 * —abrir en modo que trunca— no da error: deja el fichero a cero y se descubre
 * mucho después, cuando la BD ya no abre.
 *
 * Así que aquí no se comprueba "devuelve 0": se comprueba **lo que quedó
 * escrito alrededor**, que es lo único que distingue una escritura posicional
 * de una que se ha llevado por delante el resto.
 *
 * Corre sobre el backend de HOST (ficheros de verdad). Las placas tienen sus
 * propios motores —FatFs y littlefs—, pero el CONTRATO que se prueba aquí es
 * el mismo que ellas deben cumplir, así que un rojo aquí es un rojo allí.
 */
#include "bpvm_fs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fallos = 0;
static int total  = 0;

static void ok(int cond, const char* que) {
    total++;
    if (!cond) { fallos++; printf("  FAIL: %s\n", que); }
}

/* Lee el fichero entero a `dst` y devuelve cuántos bytes había. */
static long leer_todo(const char* path, uint8_t* dst, uint32_t cap) {
    return bpvm_fs_read(path, dst, cap);
}

int main(void) {
    printf("--- V5/H2: escritura posicional y recorte (backend host) ---\n");
    bpvm_fs_register_host();

    const char* F = "test_posicional.bin";
    bpvm_fs_remove(F);

    uint8_t buf[64];

    /* 1 — write_at CREA el fichero si no existe. Una BD hace justo esto: crea
     *     el suyo y escribe la página 0. */
    ok(bpvm_fs_write_at(F, 0, (const uint8_t*) "ABCDEFGH", 8) == 8,
       "write_at crea el fichero y escribe 8 bytes");
    uint32_t sz = 0;
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 8, "el tamaño es 8");

    /* 2 — EL CASO QUE IMPORTA: escribir EN MEDIO no toca lo de alrededor.
     *     Si el backend abriera truncando, aquí saldrían 3 bytes en vez de 8, y
     *     ése es exactamente el bug que se lleva por delante una base de datos. */
    ok(bpvm_fs_write_at(F, 3, (const uint8_t*) "xyz", 3) == 3,
       "write_at en el desplazamiento 3");
    memset(buf, 0, sizeof buf);
    ok(leer_todo(F, buf, sizeof buf) == 8, "sigue midiendo 8 (NO se truncó)");
    ok(memcmp(buf, "ABCxyzGH", 8) == 0, "quedó ABCxyzGH: sólo cambió el medio");

    /* 3 — read_at y write_at tienen que verse la una a la otra. */
    memset(buf, 0, sizeof buf);
    ok(bpvm_fs_read_at(F, 3, buf, 3) == 3 && memcmp(buf, "xyz", 3) == 0,
       "read_at lee justo lo que write_at dejó");

    /* 4 — escribir MÁS ALLÁ del final extiende. El contenido del hueco es
     *     INDEFINIDO por contrato (FatFs deja lo que hubiera, littlefs pone
     *     ceros), así que se comprueba el TAMAÑO y el dato del final — nunca
     *     el relleno, que sería probar algo que no se ha prometido. */
    ok(bpvm_fs_write_at(F, 16, (const uint8_t*) "FIN", 3) == 3,
       "write_at más allá del final");
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 19, "el fichero creció a 19");
    memset(buf, 0, sizeof buf);
    ok(bpvm_fs_read_at(F, 16, buf, 3) == 3 && memcmp(buf, "FIN", 3) == 0,
       "el dato del final está donde se pidió");
    memset(buf, 0, sizeof buf);
    ok(bpvm_fs_read_at(F, 0, buf, 8) == 8 && memcmp(buf, "ABCxyzGH", 8) == 0,
       "y lo de antes SIGUE ahí tras extender");

    /* 5 — truncate encoge y CONSERVA lo que sobrevive. */
    ok(bpvm_fs_truncate(F, 5) == 0, "truncate a 5");
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 5, "ahora mide 5");
    memset(buf, 0, sizeof buf);
    ok(leer_todo(F, buf, sizeof buf) == 5 && memcmp(buf, "ABCxy", 5) == 0,
       "conservó los 5 primeros, no otros 5 cualesquiera");

    /* 6 — truncate a cero: vacío, pero EXISTE. Vaciar y borrar no es lo mismo,
     *     y una BD que reinicia su fichero cuenta con la diferencia. */
    ok(bpvm_fs_truncate(F, 0) == 0, "truncate a 0");
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 0, "mide 0 y SIGUE EXISTIENDO");

    /* 7 — y después de vaciarlo se puede volver a escribir. */
    ok(bpvm_fs_write_at(F, 0, (const uint8_t*) "otra vez", 8) == 8,
       "se puede reescribir tras vaciarlo");
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 8, "vuelve a medir 8");

    bpvm_fs_remove(F);

    printf("--- %d/%d OK, %d FALLO(S) ---\n", total - fallos, total, fallos);
    return fallos ? 1 : 0;
}
