/*
 * test_wirerecorte.c — [V7 #557] el campo de texto del wire que se RECORTA en vez de hundir el
 * mensaje (`wire_v1_field_string_recortado`), que es lo que hace que el EXITED salga siempre.
 *
 * El fallo que lo motiva: `wire_v1_field_string` devuelve -1 si el texto no cabe, y el EXITED
 * se abandonaba entero. Un error de enlace llega a 1.536 B; el buffer del EXITED de la Pico
 * tiene 1.024 y el del STM32 tenía 320: con un error largo el programa terminaba y el IDE no se
 * enteraba nunca.
 *
 * Lo que se mira, y por qué cada cosa: un recorte que partiera un carácter UTF-8 o un escape
 * (`\n`, `\u001f`) dejaría un JSON INVÁLIDO — el IDE lo descartaría, y sería el mismo silencio
 * con otra cara. Así que no basta con «sale algo»: tiene que salir JSON bien formado, que
 * termine en `..."}` y quepa en el buffer con su `}`.
 */
#include "bpvm_wire_v1.h"
#include <stdio.h>
#include <string.h>

/* Los envíos son de cada familia (el cable); aquí no se envía nada. */
void wire_v1_send_line(const char* data, size_t len) { (void) data; (void) len; }
void wire_v1_send_bulk(const uint8_t* data, size_t n) { (void) data; (void) n; }

static int fallos = 0;
static void ok(int cond, const char* que) {
    printf("  %s  : %s\n", cond ? "ok" : "FALLO", que);
    if (!cond) fallos++;
}

/* ¿UTF-8 bien formado y sin un escape a medias? (lo mínimo para que un parser JSON lo acepte) */
static int json_texto_sano(const char* s, size_t n) {
    for (size_t i = 0; i < n; ) {
        unsigned char c = (unsigned char) s[i];
        if (c == '\\') {
            if (i + 1 >= n) return 0;
            char e = s[i + 1];
            if (e == 'u') {
                if (i + 6 > n) return 0;
                for (int k = 2; k < 6; k++) if (!strchr("0123456789abcdef", s[i + k])) return 0;
                i += 6;
            } else if (strchr("\"\\nrt", e)) {
                i += 2;
            } else return 0;
            continue;
        }
        size_t m = (c < 0x80) ? 1 : ((c & 0xE0) == 0xC0) ? 2 : ((c & 0xF0) == 0xE0) ? 3
                 : ((c & 0xF8) == 0xF0) ? 4 : 0;
        if (m == 0 || i + m > n) return 0;
        for (size_t k = 1; k < m; k++) if (((unsigned char) s[i + k] & 0xC0) != 0x80) return 0;
        i += m;
    }
    return 1;
}

/* Construye {"type":"EXITED","errorMessage":"<valor>"} en un buffer de `cap` bytes. */
static int exited(char* buf, size_t cap, const char* valor) {
    int off = wire_v1_msg_begin_event(buf, cap, 0, "EXITED");
    if (off >= 0) off = wire_v1_field_string_recortado(buf, cap, (size_t) off, "errorMessage", valor, 1);
    if (off >= 0) off = wire_v1_msg_end(buf, cap, (size_t) off);
    return off;
}

/* El texto del campo (lo que va entre las comillas de errorMessage). */
static const char* texto(const char* buf, int n, size_t* len) {
    const char* k = strstr(buf, "\"errorMessage\":\"");
    if (!k) return NULL;
    const char* ini = k + strlen("\"errorMessage\":\"");
    const char* fin = buf + n - 2;               /* `"}` del final */
    *len = (size_t) (fin - ini);
    return ini;
}

int main(void) {
    char buf[64];
    size_t n_t;
    const char* t;

    /* 1. Cabe entero: lo mismo que field_string, sin marca. */
    int n = exited(buf, sizeof buf, "corto");
    ok(n > 0 && (size_t) n <= sizeof buf && memcmp(buf, "{\"type\":\"EXITED\",\"errorMessage\":\"corto\"}", (size_t) n) == 0,
       "si cabe, va entero y sin «...»");

    /* 2. No cabe: sale, recortado, con la marca, y cabe con su `}`. */
    char largo[400];
    memset(largo, 'x', sizeof largo - 1); largo[sizeof largo - 1] = '\0';
    n = exited(buf, sizeof buf, largo);
    ok(n > 0 && (size_t) n <= sizeof buf, "si no cabe, el mensaje SALE igual (antes: -1 y ningún EXITED)");
    ok(n > 5 && memcmp(buf + n - 5, "...\"}", 5) == 0, "y termina en ...\"}");
    t = texto(buf, n, &n_t);
    ok(t && json_texto_sano(t, n_t), "y el texto es JSON sano");

    /* 3. El corte cae en medio de una «ñ» (C3 B1): no puede quedar medio carácter. Se prueban
     *    todas las alineaciones: una ñ tras 0..9 bytes de relleno. */
    int sano = 1, salio = 1;
    for (int pad = 0; pad < 10; pad++) {
        char v[200]; size_t k = 0;
        for (int i = 0; i < pad; i++) v[k++] = 'a';
        while (k + 2 < sizeof v - 1) { v[k++] = (char) 0xC3; v[k++] = (char) 0xB1; }
        v[k] = '\0';
        n = exited(buf, sizeof buf, v);
        if (n <= 0 || (size_t) n > sizeof buf) { salio = 0; continue; }
        t = texto(buf, n, &n_t);
        if (!t || !json_texto_sano(t, n_t)) sano = 0;
    }
    ok(salio, "con «ñ» (2 bytes) en todas las alineaciones, sale");
    ok(sano, "y nunca deja media «ñ»");

    /* 4. Lo mismo con escapes: \n (2 bytes) y un carácter de control (\u001f, 6 bytes). */
    sano = 1; salio = 1;
    for (int pad = 0; pad < 8; pad++) {
        char v[200]; size_t k = 0;
        for (int i = 0; i < pad; i++) v[k++] = 'b';
        while (k + 2 < sizeof v - 1) { v[k++] = '\n'; v[k++] = (char) 0x1f; }
        v[k] = '\0';
        n = exited(buf, sizeof buf, v);
        if (n <= 0 || (size_t) n > sizeof buf) { salio = 0; continue; }
        t = texto(buf, n, &n_t);
        if (!t || !json_texto_sano(t, n_t)) sano = 0;
    }
    ok(salio, "con \\n y \\u001f en todas las alineaciones, sale");
    ok(sano, "y nunca deja un escape a medias");

    /* 5. Ni la clave cabe: -1 (lo único que no se puede arreglar recortando). */
    char chico[24];
    n = exited(chico, sizeof chico, "lo que sea");
    ok(n < 0, "si no cabe ni la clave, -1");

    /* 6. `value` NULL: como una cadena vacía. */
    int off = wire_v1_msg_begin_event(buf, sizeof buf, 0, "EXITED");
    off = wire_v1_field_string_recortado(buf, sizeof buf, (size_t) off, "errorMessage", NULL, 1);
    ok(off > 0, "value NULL: como \"\"");

    if (fallos) { printf("ROJO: %d fallo(s)\n", fallos); return 1; }
    printf("[status=OK] el campo recortado: sale siempre, y en JSON sano\n");
    return 0;
}
