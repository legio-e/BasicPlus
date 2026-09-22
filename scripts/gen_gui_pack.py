"""gen_gui_pack.py — GENERA la costura entre la VM y el pack de la GUI (V7/H6 P2).

Lee `bpgenvm-c/include/bpvm_gui.h` (el contrato VM → GUI: ~90 funciones) y escribe:

  bpgenvm-c/src/gui_pack_api.h      la TABLA (`bpvm_gui_tabla_t`: un puntero por función) y
                                    los NOMBRES con los que el pack las publica ("create_label"…)
  bpgenvm-c/src/gui_pack.c          los TRAMPOLINES: cada `bpvm_gui_*` de la VM llama a la tabla,
                                    resuelta en el primer uso con `busca('LVGL')` (imágenes SIN
                                    LVGL dentro: `-DBPVM_GUI_PACK`)
  bpgenvm-c/src/gui_pack_publics.c  lo que el PACK publica: la tabla `bpvm_pack_api_t` con esos
                                    nombres apuntando a las funciones de verdad (se compila DENTRO
                                    del pack, junto a gui.c)
  bpgenvm-c/test/gui_pack_stubs.c   un DOBLE del pack (cada función con su firma, contando y
                                    devolviendo valores marcados) para probar la costura en el PC

Un solo origen —la cabecera— y tres salidas: si la cabecera cambia, se vuelve a generar y
los tres se mueven juntos (si se escribieran a mano, el día que uno se olvide el pack llama
a la ranura equivocada, sin error de compilación: #299/#315). Generado ≠ parcheado a mano.

    python scripts/gen_gui_pack.py            # escribe los tres
    python scripts/gen_gui_pack.py --comprobar   # sólo dice si están al día
"""
import io, re, sys, os

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H    = os.path.join(RAIZ, 'bpgenvm-c', 'include', 'bpvm_gui.h')
OUT_API   = os.path.join(RAIZ, 'bpgenvm-c', 'src', 'gui_pack_api.h')
OUT_TRAMP = os.path.join(RAIZ, 'bpgenvm-c', 'src', 'gui_pack.c')
OUT_PUBL  = os.path.join(RAIZ, 'bpgenvm-c', 'src', 'gui_pack_publics.c')
OUT_STUBS = os.path.join(RAIZ, 'bpgenvm-c', 'test', 'gui_pack_stubs.c')   # dobles para test_guipack

# Las que NO cruzan: son del host/simulador (ventana SDL) o internas del pack.
FUERA = {
    'disp_set_headless', 'disp_set_exit_on_close', 'disp_set_title',   # host: la ventana
    'set_screen_size',                                                  # host/sim: --screen=
    'disp_init',                                                        # la llama gui.c por dentro
    'disponible', 'pack_reset',                                         # las define gui_pack.c mismo
    'dump_free',                                                        # la extra de la tabla (va aparte)
}
# Las que devuelven memoria del que las implementa: en el pack la reserva SU arena y la VM no
# puede liberarla con su free. Se copian y se devuelve la original con `dump_free`.
ESPECIALES = {'dump_tree'}

proto_re = re.compile(r'^\s*((?:const\s+)?[A-Za-z_][A-Za-z_0-9]*\s*\**)\s*bpvm_gui_([A-Za-z_0-9]+)\s*\((.*?)\)\s*;')

def partir_params(s):
    """Divide por comas de PRIMER nivel (un puntero a función lleva comas dentro)."""
    out, nivel, cur = [], 0, ''
    for ch in s:
        if ch == '(': nivel += 1
        if ch == ')': nivel -= 1
        if ch == ',' and nivel == 0:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip(): out.append(cur.strip())
    return out

def nombre_param(p):
    m = re.search(r'\(\s*\*\s*([A-Za-z_][A-Za-z_0-9]*)\s*\)', p)   # int (*visit)(...)
    if m: return m.group(1)
    m = re.search(r'([A-Za-z_][A-Za-z_0-9]*)\s*$', p)
    return m.group(1) if m else None

def leer():
    fns = []
    with io.open(H, encoding='utf-8') as f:
        for ln in f:
            ln = ln.split('/*')[0]
            m = proto_re.match(ln)
            if not m: continue
            ret, nom, params = m.group(1).strip(), m.group(2), m.group(3).strip()
            ret = re.sub(r'\s+', ' ', ret).replace(' *', '*')
            ps = [] if params in ('', 'void') else partir_params(params)
            args = [nombre_param(p) for p in ps]
            assert all(args), (nom, ps)
            fns.append((ret, nom, ps, args))
    return fns

def generar(fns):
    cross = [f for f in fns if f[1] not in FUERA]
    # ── la tabla y los nombres ──
    a = []
    a.append('/* gui_pack_api.h — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.')
    a.append(' * NO EDITAR: la cabecera es el origen; esto se regenera. V7/H6 P2.')
    a.append(' *')
    a.append(' * La TABLA con la que el pack de la GUI sirve a la VM (un puntero por función de')
    a.append(' * bpvm_gui.h que cruza la frontera) y los NOMBRES bajo los que las publica en su')
    a.append(' * bpvm_pack_api_t (marca \'LVGL\'). Los trampolines de gui_pack.c la rellenan por')
    a.append(' * nombre en el primer uso; el pack la construye en gui_pack_publics.c. */')
    a.append('#ifndef GUI_PACK_API_H')
    a.append('#define GUI_PACK_API_H')
    a.append('#include <stddef.h>')
    a.append('#include <stdint.h>')
    a.append('')
    a.append('#define BPVM_GUI_MARCA    0x4C56474Cu   /* \'LVGL\' */')
    a.append('#define BPVM_GUI_API_VER  1u            /* sube si cambia la FIRMA de algo de abajo */')
    a.append('#define BPVM_GUI_N_FUNCS  %d' % (len(cross) + 1))
    a.append('')
    a.append('typedef struct bpvm_gui_tabla {')
    for ret, nom, ps, args in cross:
        a.append('    %s (*%s)(%s);' % (ret, nom, ', '.join(ps) if ps else 'void'))
    a.append('    void (*dump_free)(char* p);   /* pack: libera lo que dump_tree reservó en SU arena */')
    a.append('} bpvm_gui_tabla_t;')
    a.append('')
    a.append('/* En el MISMO orden que la struct: el resolvedor recorre las dos a la par. */')
    a.append('static const char* const BPVM_GUI_NOMBRES[BPVM_GUI_N_FUNCS] = {')
    for ret, nom, ps, args in cross:
        a.append('    "%s",' % nom)
    a.append('    "dump_free",')
    a.append('};')
    a.append('')
    a.append('#endif /* GUI_PACK_API_H */')
    api = '\n'.join(a) + '\n'

    # ── los trampolines ──
    t = []
    t.append('/* gui_pack.c — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.')
    t.append(' * NO EDITAR. V7/H6 P2: la GUI de la VM cuando LVGL NO va dentro de la imagen.')
    t.append(' *')
    t.append(' * Cada función de bpvm_gui.h es aquí un TRAMPOLÍN a la tabla que publica el pack de')
    t.append(' * la GUI bajo la marca \'LVGL\' (bpvm_pack_api_t con los nombres de gui_pack_api.h).')
    t.append(' * La tabla se resuelve UNA vez, en el primer uso, POR NOMBRE (bpvm_pack_api_find):')
    t.append(' * un símbolo que falte se dice, y una versión de API distinta también. Sin pack,')
    t.append(' * `bpvm_gui_disponible()` devuelve 0 con el motivo y el builtin __guiScreenActive')
    t.append(' * lanza un RuntimeError CON NOMBRE; los demás trampolines, si se llaman sin tabla,')
    t.append(' * no hacen nada y devuelven 0/NULL (nunca saltan a ninguna parte).')
    t.append(' *')
    t.append(' * Se compila con -DBPVM_GUI_PACK en vez de gui.c; en el host y en las imágenes con')
    t.append(' * LVGL dentro no existe: ahí bpvm_gui_* son las funciones de gui.c. */')
    t.append('#include "bpvm_gui.h"')
    t.append('#include "gui_pack_api.h"')
    t.append('#include "bpvm_bios.h"')
    t.append('#include "bpvm_pack_api.h"')
    t.append('#include "bpvm_alloc.h"')
    t.append('#include "bpvm.h"')
    t.append('#include <string.h>')
    t.append('#include <stdio.h>')
    t.append('')
    t.append('static bpvm_gui_tabla_t s_t;')
    t.append('static int  s_estado = 0;          /* 0 = sin resolver · 1 = resuelta · -1 = no hay / no vale */')
    t.append('static char s_motivo[128];')
    t.append('')
    t.append('static int resolver(void) {')
    t.append('    if (s_estado != 0) return s_estado > 0;')
    t.append('    const bpvm_pack_api_t* api = (const bpvm_pack_api_t*) bpvm_bios_busca(BPVM_GUI_MARCA);')
    t.append('    if (api == 0) {')
    t.append('        snprintf(s_motivo, sizeof s_motivo, "esta placa no tiene el pack \'LVGL\' grabado (la GUI llega por el pack)");')
    t.append('        s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;')
    t.append('    }')
    t.append('    const char* mal = bpvm_pack_api_verify(api, BPVM_GUI_MARCA, BPVM_GUI_API_VER);')
    t.append('    if (mal) {')
    t.append('        snprintf(s_motivo, sizeof s_motivo, "el pack \'LVGL\' no vale: %s (API v%u, se esperaba v%u)", mal, (unsigned) api->version, (unsigned) BPVM_GUI_API_VER);')
    t.append('        s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;')
    t.append('    }')
    t.append('    bpvm_pack_fn_t* ranuras = (bpvm_pack_fn_t*) (void*) &s_t;   /* la struct son punteros a función, en orden */')
    t.append('    for (int i = 0; i < BPVM_GUI_N_FUNCS; i++) {')
    t.append('        bpvm_pack_fn_t f = bpvm_pack_api_find(api, BPVM_GUI_NOMBRES[i]);')
    t.append('        if (f == 0) {')
    t.append('            snprintf(s_motivo, sizeof s_motivo, "al pack \'LVGL\' le falta \'%s\'", BPVM_GUI_NOMBRES[i]);')
    t.append('            s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;')
    t.append('        }')
    t.append('        ranuras[i] = f;')
    t.append('    }')
    t.append('    s_estado = 1;')
    t.append('    bpvm_diag("GUI: pack \'LVGL\' resuelto (%d funciones, API v%u)", (int) BPVM_GUI_N_FUNCS, (unsigned) api->version);')
    t.append('    return 1;')
    t.append('}')
    t.append('')
    t.append('int bpvm_gui_disponible(const char** motivo) {')
    t.append('    int ok = resolver();')
    t.append('    if (motivo) *motivo = ok ? "" : s_motivo;')
    t.append('    return ok;')
    t.append('}')
    t.append('')
    t.append('/* Para las pruebas de host y el simulador: olvidar la tabla (otro pack, otra vez). */')
    t.append('void bpvm_gui_pack_reset(void) { s_estado = 0; memset(&s_t, 0, sizeof s_t); }')
    t.append('')
    for ret, nom, ps, args in cross:
        firma = '%s bpvm_gui_%s(%s)' % (ret, nom, ', '.join(ps) if ps else 'void')
        llamada = 's_t.%s(%s)' % (nom, ', '.join(args))
        if nom in ESPECIALES:
            t.append(firma + ' {')
            t.append('    /* la memoria la reserva el PACK en su arena: se copia a la de la VM (que es lo')
            t.append('     * que el que llama va a liberar) y se le devuelve la suya con dump_free. */')
            t.append('    if (!resolver()) { if (out) *out = 0; return 0; }')
            t.append('    char* suyo = 0; size_t n = s_t.dump_tree(&suyo);')
            t.append('    char* mio = (char*) bpvm_malloc(n + 1);')
            t.append('    if (mio) { if (suyo && n) memcpy(mio, suyo, n); mio[n] = 0; }')
            t.append('    if (suyo) s_t.dump_free(suyo);')
            t.append('    if (out) *out = mio;')
            t.append('    return mio ? n : 0;')
            t.append('}')
            continue
        if ret == 'void':
            t.append('%s { if (resolver()) %s; }' % (firma, llamada))
        elif ret.endswith('*'):
            t.append('%s { return resolver() ? %s : 0; }' % (firma, llamada))
        else:
            t.append('%s { return resolver() ? %s : 0; }' % (firma, llamada))
    t.append('')
    t.append('/* Las del host/simulador no cruzan: en un micro no hay ventana ni --screen=. */')
    t.append('void bpvm_gui_set_screen_size(int w, int h) { (void) w; (void) h; }')
    t.append('#ifdef BPVM_GUI_RENDER')
    t.append('void bpvm_gui_disp_set_headless(int on) { (void) on; }')
    t.append('void bpvm_gui_disp_set_exit_on_close(int on) { (void) on; }')
    t.append('void bpvm_gui_disp_set_title(const char* title) { (void) title; }')
    t.append('#endif')
    tramp = '\n'.join(t) + '\n'

    # ── lo que publica el pack ──
    p = []
    p.append('/* gui_pack_publics.c — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.')
    p.append(' * NO EDITAR. V7/H6 P2: la tabla que el PACK de la GUI publica bajo \'LVGL\'.')
    p.append(' *')
    p.append(' * Se compila DENTRO del pack, junto a gui.c (que define las funciones de verdad).')
    p.append(' * La entrada del pack hace `bios->publica(BPVM_GUI_MARCA, &bpvm_gui_api)` y la VM,')
    p.append(' * por los trampolines de gui_pack.c, resuelve cada nombre contra esta tabla. */')
    p.append('#include "bpvm_gui.h"')
    p.append('#include "gui_pack_api.h"')
    p.append('#include "bpvm_pack_api.h"')
    p.append('')
    p.append('static const bpvm_pack_sym_t s_publics[BPVM_GUI_N_FUNCS] = {')
    for ret, nom, ps, args in cross:
        p.append('    { "%s", (bpvm_pack_fn_t) bpvm_gui_%s },' % (nom, nom))
    p.append('    { "dump_free", (bpvm_pack_fn_t) bpvm_gui_dump_free },')
    p.append('};')
    p.append('')
    p.append('const bpvm_pack_api_t bpvm_gui_api = { BPVM_GUI_MARCA, BPVM_GUI_API_VER, BPVM_GUI_N_FUNCS, s_publics };')
    publ = '\n'.join(p) + '\n'

    # ── los DOBLES para la prueba de host: la misma firma, un contador y un valor marcado ──
    e = []
    e.append('/* gui_pack_stubs.c — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.')
    e.append(' * NO EDITAR. V7/H6 P2: un DOBLE del pack de la GUI para test_guipack.c: cada')
    e.append(' * función con la firma de verdad, que cuenta la llamada y devuelve un valor marcado')
    e.append(' * (7, "stub", 240x320…). Así la costura se prueba en el PC sin pack ni LVGL. */')
    e.append('#include "bpvm_gui.h"')
    e.append('#include "gui_pack_api.h"')
    e.append('#include "bpvm_pack_api.h"')
    e.append('#include <stdlib.h>')
    e.append('#include <string.h>')
    e.append('int g_stub_llamadas = 0;')
    e.append('int g_stub_dump_free = 0;')
    for ret, nom, ps, args in cross:
        firma = 'static %s stub_%s(%s)' % (ret, nom, ', '.join(ps) if ps else 'void')
        silencio = ' '.join('(void) %s;' % a for a in args)
        if nom == 'dump_tree':
            cuerpo = 'g_stub_llamadas++; { const char* d = "stub-dump"; char* b = (char*) malloc(strlen(d) + 1); strcpy(b, d); *out = b; return strlen(d); }'
        elif nom == 'visit_roots':
            cuerpo = 'g_stub_llamadas++; visit(ctx, 42u);'
        elif nom == 'disp_native_size':
            cuerpo = 'g_stub_llamadas++; *w = 240; *h = 320; return 1;'
        elif ret == 'void':
            cuerpo = 'g_stub_llamadas++; %s' % silencio
        elif ret == 'const char*':
            cuerpo = 'g_stub_llamadas++; %s return "stub";' % silencio
        else:
            cuerpo = 'g_stub_llamadas++; %s return 7;' % silencio
        e.append('%s { %s }' % (firma, cuerpo))
    e.append('static void stub_dump_free(char* p) { g_stub_dump_free++; free(p); }')
    e.append('')
    e.append('const bpvm_pack_sym_t bpvm_gui_stub_syms[BPVM_GUI_N_FUNCS] = {')
    for ret, nom, ps, args in cross:
        e.append('    { "%s", (bpvm_pack_fn_t) stub_%s },' % (nom, nom))
    e.append('    { "dump_free", (bpvm_pack_fn_t) stub_dump_free },')
    e.append('};')
    e.append('const bpvm_pack_api_t bpvm_gui_api_stubs = { BPVM_GUI_MARCA, BPVM_GUI_API_VER, BPVM_GUI_N_FUNCS, bpvm_gui_stub_syms };')
    stubs = '\n'.join(e) + '\n'
    return api, tramp, publ, stubs, cross

def main():
    fns = leer()
    api, tramp, publ, stubs, cross = generar(fns)
    salidas = [(OUT_API, api), (OUT_TRAMP, tramp), (OUT_PUBL, publ), (OUT_STUBS, stubs)]
    if '--comprobar' in sys.argv:
        mal = 0
        for path, txt in salidas:
            actual = io.open(path, encoding='utf-8').read().replace('\r\n', '\n') if os.path.exists(path) else ''
            if actual != txt: print('DESACTUALIZADO:', os.path.relpath(path, RAIZ)); mal = 1
        print('gui_pack: %s (%d funciones cruzan, %d fuera)' % ('al dia' if not mal else 'HAY QUE REGENERAR', len(cross), len(fns) - len(cross)))
        sys.exit(mal)
    for path, txt in salidas:
        io.open(path, 'w', encoding='utf-8', newline='\n').write(txt)
        print('escrito', os.path.relpath(path, RAIZ))
    print('%d funciones de bpvm_gui.h; %d cruzan al pack, %d se quedan en el host' % (len(fns), len(cross), len(fns) - len(cross)))

if __name__ == '__main__':
    main()
