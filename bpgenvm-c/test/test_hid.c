/*
 * test_hid.c — [V8/H2, paso 5, la pieza 2] LO QUE MANDA UN TECLADO O UN RATÓN HID (bpvm_hid.c), en el PC: con los
 * descriptores y los informes de verdad del teclado y del ratón de Eduardo (los registros del 14-oct, en
 * C:\tmp\v8bt\consola_hid*.log), lo que llega al teclado y al ratón del SO. El descriptor dice qué es cada informe (el
 * ratón declara también un teclado, y el teclado, un ratón); el teclado, con la tabla americana y las reglas del PC; los
 * bloqueos y sus luces; y el ratón, que suma sin salirse de la pantalla, y sin pantalla no hace nada.
 */
#include "bpvm_hid.h"
#include "bpvm_bios.h"
#include "bpvm_pant.h"
#include "bpvm_teclado.h"
#include "bpvm_raton.h"

#include <stdio.h>
#include <string.h>

static int fallos = 0, buenas = 0;
static void ok(const char* que, int cond) {
    printf("%s %s\n", cond ? "ok   " : "FALLO", que);
    if (cond) buenas++; else fallos++;
}

/* Un driver de pantalla de mentira, el de la P4 (480 x 800), que cuenta cuántas veces se le pregunta el tamaño. */
static int pd_preguntas = 0;
static int  pd_init(void) { return 0; }
static int  pd_size(int* w, int* h) { pd_preguntas++; *w = 480; *h = 800; return 1; }
static void pd_flush(int a, int b, int c, int e, const uint8_t* p) { (void) a; (void) b; (void) c; (void) e; (void) p; }
static const bpvm_pack_sym_t PANT_SYMS[] = {
    { "init", (bpvm_pack_fn_t) pd_init }, { "size", (bpvm_pack_fn_t) pd_size }, { "flush", (bpvm_pack_fn_t) pd_flush } };
static const bpvm_pack_api_t PANT_P4 = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, 3u, PANT_SYMS };

/* «BT5.2 Mouse», 189 B (consola_hid.log): 1, el ratón; 2, un teclado, con sus luces; 3, multimedia; 4, encendido. */
static const char* const MAPA_RATON =
    "05 01 09 02 A1 01 85 01 09 01 A1 00 05 09 19 01"
    "29 08 15 00 25 01 75 01 95 08 81 02 05 01 09 30"
    "09 31 16 01 F8 26 FF 07 75 10 95 02 81 06 09 38"
    "15 81 25 7F 75 08 95 01 81 06 05 0C 0A 38 02 95"
    "01 81 06 C0 C0 05 01 09 06 A1 01 85 02 75 01 95"
    "08 05 07 19 E0 29 E7 15 00 25 01 81 02 95 01 75"
    "08 81 03 95 05 75 01 05 08 19 01 29 05 91 02 95"
    "01 75 03 91 03 95 06 75 08 15 00 26 FF 00 05 07"
    "19 00 29 FF 81 00 C0 05 0C 09 01 A1 01 85 03 19"
    "00 2A FF 03 15 00 26 FF 03 95 01 75 10 81 00 C0"
    "05 01 09 80 A1 01 85 04 15 00 25 01 75 01 95 03"
    "19 81 29 83 81 02 95 01 75 05 81 03 C0";

/* «Bluetooth Keyboard», 160 B (consola_hid2.log): 1, el teclado, con sus luces; 2, multimedia; 3, un ratón. */
static const char* const MAPA_TECLADO =
    "05 01 09 06 A1 01 85 01 75 01 95 08 05 07 19 E0"
    "29 E7 15 00 25 01 81 02 95 01 75 08 81 03 95 05"
    "75 01 05 08 19 01 29 05 91 02 95 01 75 03 91 03"
    "95 06 75 08 15 00 26 FF 00 05 07 19 00 29 FF 81"
    "00 C0 05 0C 09 01 A1 01 85 02 19 00 2A FF 03 15"
    "00 26 FF 03 95 01 75 10 81 00 C0 05 01 09 02 A1"
    "01 85 03 09 01 A1 00 05 09 19 01 29 08 15 00 25"
    "01 75 01 95 08 81 02 05 01 09 30 09 31 16 01 80"
    "26 FF 7F 75 10 95 02 81 06 09 38 15 81 25 7F 75"
    "08 95 01 81 06 05 0C 0A 38 02 95 01 81 06 C0 C0";

static int hex(const char* s, uint8_t* d, int max) {
    int n = 0;
    while (*s && n < max) {
        unsigned v;
        while (*s == ' ') s++;
        if (!*s || sscanf(s, "%2x", &v) != 1) break;
        d[n++] = (uint8_t) v;
        s += 2;
    }
    return n;
}

static int manda(const bpvm_hid_mapa_t* m, uint8_t id, const char* informe, bpvm_hid_teclado_t* t) {
    uint8_t d[64];
    int n = hex(informe, d, (int) sizeof d);
    return bpvm_hid_informe(m, id, d, n, t);
}

/* La única tecla nueva del búfer del SO, con su código entero y su carácter. */
static int una(int32_t k, int32_t cp) {
    int32_t lk, lcp;
    if (!bpvm_teclado_hay()) return 0;
    lk = bpvm_teclado_lee(1);
    lcp = bpvm_teclado_caracter(1);
    if (lk != k || lcp != cp) printf("      (llegó k=0x%08X cp=%d; se esperaba k=0x%08X cp=%d)\n",
                                     (unsigned) lk, (int) lcp, (unsigned) k, (int) cp);
    return lk == k && lcp == cp && !bpvm_teclado_hay();
}
static int ninguna(void) { return !bpvm_teclado_hay(); }

#define NUM    BPVM_TECLA_BLOQ_NUM
#define MAYUS  BPVM_TECLA_BLOQ_MAYUS
#define SUELTA "00 00 00 00 00 00 00 00"

static int en(int32_t x, int32_t y) {
    if (bpvm_raton_x() != x || bpvm_raton_y() != y)
        printf("      (el ratón está en %d,%d; se esperaba %d,%d)\n", (int) bpvm_raton_x(), (int) bpvm_raton_y(),
               (int) x, (int) y);
    return bpvm_raton_x() == x && bpvm_raton_y() == y;
}

int main(void) {
    uint8_t d[512], luces[8];
    char s[160];
    int n;
    bpvm_hid_mapa_t mr, mt;
    bpvm_hid_teclado_t t, t2;
    const bpvm_hid_informe_t* r;

    printf("--- el descriptor del ratón ---\n");
    n = hex(MAPA_RATON, d, (int) sizeof d);
    ok("se copió entero: 189 B", n == 189);
    ok("cuatro informes de entrada", bpvm_hid_lee_mapa(d, n, &mr) == 4);
    ok("la clase, el ratón", strcmp(bpvm_hid_clase(&mr), "mouse") == 0);
    r = &mr.inf[0];
    ok("el 1 es el ratón", r->id == 1 && r->que == BPVM_HID_RATON);
    ok("sus botones, del bit 0 al 4", r->boton[0] == 0 && r->boton[1] == 1 && r->boton[4] == 4);
    ok("X, de 16 bits con signo desde el bit 8", r->x.bit == 8 && r->x.bits == 16 && r->x.con_signo);
    ok("Y, desde el 24", r->y.bit == 24 && r->y.bits == 16 && r->y.con_signo);
    ok("la rueda, de 8 bits con signo desde el 40", r->rueda.bit == 40 && r->rueda.bits == 8 && r->rueda.con_signo);
    r = &mr.inf[1];
    ok("el 2 es un teclado", r->id == 2 && r->que == BPVM_HID_TECLADO);
    ok("sus modificadores, del bit 0 al 7", r->mod[0] == 0 && r->mod[1] == 1 && r->mod[7] == 7);
    ok("y seis teclas desde el bit 16", r->teclas.bit == 16 && r->teclas.bits == 8 && r->nteclas == 6);
    ok("el 3 y el 4, otros", mr.inf[2].id == 3 && mr.inf[2].que == BPVM_HID_OTRO &&
                              mr.inf[3].id == 4 && mr.inf[3].que == BPVM_HID_OTRO);
    ok("las luces, en el informe de salida 2, de un byte", mr.luces && mr.luces_id == 2 && mr.luces_bits == 8 &&
                                                         mr.luz[0] == 0 && mr.luz[1] == 1 && mr.luz[2] == 2);
    bpvm_hid_describe(&mr, s, sizeof s);
    printf("      (%s)\n", s);
    ok("y lo que dice el log", strcmp(s, "1 ratón, 2 teclado, 3 otro, 4 otro; luces en el 2") == 0);

    printf("--- el descriptor del teclado ---\n");
    n = hex(MAPA_TECLADO, d, (int) sizeof d);
    ok("se copió entero: 160 B", n == 160);
    ok("tres informes de entrada", bpvm_hid_lee_mapa(d, n, &mt) == 3);
    ok("la clase, el teclado", strcmp(bpvm_hid_clase(&mt), "keyboard") == 0);
    ok("el 1 es el teclado, con seis teclas", mt.inf[0].id == 1 && mt.inf[0].que == BPVM_HID_TECLADO &&
                                              mt.inf[0].teclas.bit == 16 && mt.inf[0].nteclas == 6);
    ok("el 2, otro (multimedia)", mt.inf[1].id == 2 && mt.inf[1].que == BPVM_HID_OTRO);
    ok("el 3 es un ratón: X de 16 bits con signo desde el 8", mt.inf[2].id == 3 && mt.inf[2].que == BPVM_HID_RATON &&
                                                              mt.inf[2].x.bit == 8 && mt.inf[2].x.con_signo);
    ok("las luces, en el informe de salida 1", mt.luces && mt.luces_id == 1);
    bpvm_hid_describe(&mt, s, sizeof s);
    printf("      (%s)\n", s);

    printf("--- el teclado: lo del 14-oct, 1 2 3 4 5 ---\n");
    bpvm_hid_reinicia();
    memset(&t, 0, sizeof t);
    ok("al arrancar, Bloq Num encendido", bpvm_hid_bloqueos() == NUM);
    manda(&mt, 1, "00 00 1E 00 00 00 00 00", &t); ok("00 00 1E: la tecla 1, '1'", una(0x1E | NUM, '1'));
    manda(&mt, 1, SUELTA, &t);                    ok("soltarla no avisa", ninguna());
    manda(&mt, 1, "00 00 1F 00 00 00 00 00", &t); ok("00 00 1F: '2'", una(0x1F | NUM, '2'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 20 00 00 00 00 00", &t); ok("00 00 20: '3'", una(0x20 | NUM, '3'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 21 00 00 00 00 00", &t); ok("00 00 21: '4'", una(0x21 | NUM, '4'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 22 00 00 00 00 00", &t); ok("00 00 22: '5'", una(0x22 | NUM, '5'));
    manda(&mt, 1, SUELTA, &t);

    printf("--- el teclado: los modificadores y la tabla americana ---\n");
    manda(&mt, 1, "02 00 1E 00 00 00 00 00", &t);
    ok("Mayús izquierda + 1: '!'", una(0x1E | BPVM_TECLA_SHIFT_IZQ | NUM, '!'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "20 00 04 00 00 00 00 00", &t);
    ok("Mayús derecha + A: 'A'", una(0x04 | BPVM_TECLA_SHIFT_DER | NUM, 'A'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "01 00 06 00 00 00 00 00", &t);
    ok("Ctrl + C: la tecla C con Ctrl, sin carácter", una(0x06 | BPVM_TECLA_CTRL_IZQ | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "04 00 04 00 00 00 00 00", &t); ok("Alt + A: sin carácter", una(0x04 | BPVM_TECLA_ALT | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "40 00 04 00 00 00 00 00", &t);
    ok("AltGr + A: sin carácter (el americano no tiene esa capa)", una(0x04 | BPVM_TECLA_ALTGR | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "02 00 00 00 00 00 00 00", &t); ok("Mayús sola: nada", ninguna());
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 28 00 00 00 00 00", &t); ok("Enter: sin carácter", una(0x28 | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 2C 00 00 00 00 00", &t); ok("el espacio: ' '", una(0x2C | NUM, ' '));
    manda(&mt, 1, SUELTA, &t);
    ok("los signos, sin Mayús y con ella", bpvm_hid_caracter(0x2D, 0, 0) == '-' && bpvm_hid_caracter(0x2D, 1, 0) == '_' &&
       bpvm_hid_caracter(0x2E, 0, 0) == '=' && bpvm_hid_caracter(0x2E, 1, 0) == '+' &&
       bpvm_hid_caracter(0x2F, 0, 0) == '[' && bpvm_hid_caracter(0x30, 1, 0) == '}' &&
       bpvm_hid_caracter(0x31, 0, 0) == '\\' && bpvm_hid_caracter(0x31, 1, 0) == '|' &&
       bpvm_hid_caracter(0x33, 1, 0) == ':' && bpvm_hid_caracter(0x34, 0, 0) == '\'' &&
       bpvm_hid_caracter(0x34, 1, 0) == '"' && bpvm_hid_caracter(0x35, 1, 0) == '~' &&
       bpvm_hid_caracter(0x36, 1, 0) == '<' && bpvm_hid_caracter(0x37, 1, 0) == '>' &&
       bpvm_hid_caracter(0x38, 0, 0) == '/' && bpvm_hid_caracter(0x38, 1, 0) == '?' &&
       bpvm_hid_caracter(0x64, 0, 0) == '\\' && bpvm_hid_caracter(0x27, 1, 0) == ')');
    ok("el teclado numérico: / * - + = siempre, los números con Bloq Num",
       bpvm_hid_caracter(0x54, 0, 0) == '/' && bpvm_hid_caracter(0x57, 0, 0) == '+' &&
       bpvm_hid_caracter(0x67, 0, 0) == '=' && bpvm_hid_caracter(0x62, 0, 1) == '0' &&
       bpvm_hid_caracter(0x63, 0, 1) == '.' && bpvm_hid_caracter(0x59, 0, 0) == 0);
    ok("las teclas de función y las flechas, sin carácter", bpvm_hid_caracter(0x3A, 0, 1) == 0 &&
       bpvm_hid_caracter(0x4F, 1, 1) == 0 && bpvm_hid_caracter(0x58, 0, 1) == 0);

    printf("--- el teclado: mantener no repite; ErrorRollOver; un informe corto ---\n");
    manda(&mt, 1, "00 00 04 00 00 00 00 00", &t); ok("A: 'a'", una(0x04 | NUM, 'a'));
    manda(&mt, 1, "00 00 04 05 00 00 00 00", &t); ok("con la A apretada, B: sólo 'b'", una(0x05 | NUM, 'b'));
    manda(&mt, 1, "00 00 05 00 00 00 00 00", &t); ok("se suelta la A: nada", ninguna());
    manda(&mt, 1, "00 00 01 01 01 01 01 01", &t); ok("ErrorRollOver: el informe no vale", ninguna());
    manda(&mt, 1, "00 00 05 06 00 00 00 00", &t); ok("y la B sigue apretada: sólo 'c'", una(0x06 | NUM, 'c'));
    manda(&mt, 1, "00 00 07", &t);               ok("un informe de 3 B: 'd'", una(0x07 | NUM, 'd'));
    manda(&mt, 1, "00", &t);                     ok("y de 1 B: todas sueltas", ninguna() && t.n == 0);

    printf("--- los bloqueos y sus luces ---\n");
    n = bpvm_hid_luces(&mt, luces, (int) sizeof luces);
    ok("las luces de ahora: un byte, la de Bloq Num", n == 1 && luces[0] == 0x01);
    ok("Bloq Mayús: cambian las luces", manda(&mt, 1, "00 00 39 00 00 00 00 00", &t) == 1);
    ok("y es una tecla, con el bloqueo ya puesto", una(0x39 | MAYUS | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    n = bpvm_hid_luces(&mt, luces, (int) sizeof luces);
    ok("las luces: Bloq Num y Bloq Mayús", n == 1 && luces[0] == 0x03);
    n = bpvm_hid_luces(&mr, luces, (int) sizeof luces);
    ok("también para el teclado del ratón", n == 1 && luces[0] == 0x03);
    manda(&mt, 1, "00 00 04 00 00 00 00 00", &t); ok("con Bloq Mayús, A: 'A'", una(0x04 | MAYUS | NUM, 'A'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "02 00 04 00 00 00 00 00", &t);
    ok("y con Mayús, 'a'", una(0x04 | BPVM_TECLA_SHIFT_IZQ | MAYUS | NUM, 'a'));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 1E 00 00 00 00 00", &t); ok("a los números no les toca: '1'", una(0x1E | MAYUS | NUM, '1'));
    manda(&mt, 1, SUELTA, &t);
    ok("Bloq Mayús otra vez: se apaga", manda(&mt, 1, "00 00 39 00 00 00 00 00", &t) == 1 &&
                                        bpvm_hid_bloqueos() == NUM && una(0x39 | NUM, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 59 00 00 00 00 00", &t); ok("con Bloq Num, el 1 del teclado numérico: '1'", una(0x59 | NUM, '1'));
    manda(&mt, 1, SUELTA, &t);
    ok("Bloq Num: se apaga", manda(&mt, 1, "00 00 53 00 00 00 00 00", &t) == 1 && bpvm_hid_bloqueos() == 0 &&
                             una(0x53, 0));
    manda(&mt, 1, SUELTA, &t);
    manda(&mt, 1, "00 00 59 00 00 00 00 00", &t); ok("y sin él, el 1 del numérico no da carácter", una(0x59, 0));
    manda(&mt, 1, SUELTA, &t);
    n = bpvm_hid_luces(&mt, luces, (int) sizeof luces);
    ok("las luces, todas apagadas", n == 1 && luces[0] == 0x00);
    ok("Bloq Num otra vez: encendido", manda(&mt, 1, "00 00 53 00 00 00 00 00", &t) == 1 && bpvm_hid_bloqueos() == NUM &&
                                       una(0x53 | NUM, 0));
    manda(&mt, 1, SUELTA, &t);

    printf("--- el teclado que declara el ratón, y un informe que no está ---\n");
    memset(&t2, 0, sizeof t2);
    manda(&mr, 2, "00 00 1E 00 00 00 00 00", &t2); ok("el informe 2 del ratón es un teclado: '1'", una(0x1E | NUM, '1'));
    manda(&mr, 2, SUELTA, &t2);
    manda(&mt, 9, "00 00 1E 00 00 00 00 00", &t);  ok("un informe 9, que el teclado no declara: nada", ninguna());
    manda(&mt, 2, "E9 00", &t);                    ok("el 2 del teclado, multimedia: nada", ninguna());

    printf("--- el ratón: sin pantalla no hace nada ---\n");
    manda(&mr, 1, "00 E0 FF 0C 00 00 00", NULL);
    manda(&mr, 1, "01 00 00 00 00 00 00", NULL);
    ok("ni se mueve ni aprieta (en el PC no hay driver de pantalla)", en(0, 0) && bpvm_raton_botones() == 0);

    printf("--- el ratón: la pantalla, del driver; suma desde el centro, sin salirse ---\n");
    {
        int lr, izq = 0;
        int32_t x, y, rueda = 0;
        ok("se publica un driver de pantalla (el de la P4, 480 x 800)", bpvm_bios_publica(BPVM_PANT_MARCA, &PANT_P4) == 0);
        manda(&mr, 1, "00 E0 FF 0C 00 00 00", NULL);
        ok("quien mueve el ratón (la radio) no le pregunta: sigue quieto", en(0, 0) && pd_preguntas == 0);
        lr = bpvm_raton_lector_abre();
        ok("quien lo va a leer (la GUI, un Mice.Mouse) sí: empieza en el centro", lr >= 0 && pd_preguntas == 1 &&
                                                                                 en(240, 400));
        manda(&mr, 1, "00 E0 FF 0C 00 00 00", NULL);   /* lo del 14-oct */
        ok("00 E0 FF 0C: X -32, Y +12", en(208, 412));
        manda(&mr, 1, "00 05 00 FD FF 00 00", NULL);
        ok("00 05 00 FD FF: X +5, Y -3", en(213, 409));
        manda(&mr, 1, "00 FF 07 00 00 00 00", NULL);
        ok("X +2047: se queda en el borde derecho", en(479, 409));
        manda(&mr, 1, "00 00 00 01 F8 00 00", NULL);
        ok("Y -2047: en el de arriba", en(479, 0));
        bpvm_raton_lector_lee(lr, &x, &y, &izq, &rueda);   /* lo de antes, leído */
        manda(&mr, 1, "01 00 00 00 00 00 00", NULL);
        ok("el izquierdo, apretado", bpvm_raton_botones() == BPVM_RATON_IZQ);
        ok("y lo ve un lector (la GUI)", bpvm_raton_lector_lee(lr, &x, &y, &izq, &rueda) && izq && x == 479 && y == 0);
        manda(&mr, 1, "00 00 00 00 00 00 00", NULL);
        manda(&mr, 1, "FF 00 00 00 00 00 00", NULL);
        ok("los ocho botones: sólo cuentan los cinco del ratón del SO", bpvm_raton_botones() == BPVM_RATON_BOTONES);
        manda(&mr, 1, "00 00 00 00 00 00 00", NULL);
        bpvm_raton_lector_lee(lr, &x, &y, &izq, &rueda);
        manda(&mr, 1, "00 00 00 00 00 01 00", NULL);
        ok("la rueda, una muesca hacia arriba", bpvm_raton_lector_lee(lr, &x, &y, &izq, &rueda) && rueda == 1);
        manda(&mr, 1, "00 00 00 00 00 FF 00", NULL);
        ok("y una hacia abajo", bpvm_raton_lector_lee(lr, &x, &y, &izq, &rueda) && rueda == -1);
        manda(&mt, 3, "00 F6 FF 0A 00 00 00", NULL);
        ok("el ratón que declara el teclado (su informe 3): X -10, Y +10", en(469, 10));
        bpvm_raton_pantalla(320, 240);
        ok("una pantalla más pequeña: el punto se queda dentro", en(319, 10));
        bpvm_raton_fin_de_run();
        manda(&mr, 1, "00 F6 FF 00 00 00 00", NULL);
        ok("y al acabar un programa sigue la pantalla, y el ratón se mueve", en(309, 10));
    }

    printf("[status=%s] %d/%d\n", fallos ? "FALLO" : "OK", buenas, buenas + fallos);
    return fallos ? 1 : 0;
}
