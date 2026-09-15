#!/usr/bin/env python3
# mezcla.py -- V7/H2: MEZCLAR VERSIONES DE VERDAD y ver lo que pasa, en las DOS VMs.
#
# Eduardo (15-sep): «Tu defiendes que con los cambios nada de esto es posible, pero yo
# no termino de creer. Lo que se puede hacer son los cambios necesarios y despues
# mezclar versiones y ver lo que ocurre de verdad.»
#
# Una libreria pequena (Lib: clase Base con campos, property, sobrecargas, funcion de
# modulo, enum) en N VARIANTES (metodo anadido al final / en medio, quitado, firma
# cambiada, campo anadido / quitado, property anadida, reordenada, manejador privado,
# sobrecarga con enum de cabeza, solo el cuerpo) x generaciones (6 < 7 < 8), y tres
# apps: Usa (usa la clase), Ext (la extiende) y Puente (dos modulos sellados contra
# versiones DISTINTAS que se pasan un objeto: A lo crea, B le asigna una property y
# llama a un metodo). Dos bloques:
#
#   I  MISMA VERSION: cada app compilada contra la variante X y ejecutada contra X.
#      Tiene que correr SIEMPRE con la salida de X. Si no, es el compilador (#514).
#   II MEZCLA: la app sellada contra v1 (gen 7) y ejecutada contra cada variante.
#      La norma (#500) predice: mas antigua -> rechazo; misma gen y otra interfaz ->
#      rechazo; mas nueva y la extiende -> rechazo; mas nueva y solo la usa -> corre
#      si enlaza (los marcadores) o error de enlace nombrando el simbolo.
#
# Veredicto por fila: ok (lo predicho, en las dos VMs, mismo texto) / SILENCIOSO
# (corre, exit 0, y NO imprime lo esperado: lo que nadie ve) / RUIDOSO-otro (rechaza
# por otra razon que la predicha) / PARIDAD (las VMs difieren) / CRASH.
#
#   python compat/mezcla.py [--solo=I|II|P] [--keep] [--work=DIR]
#
# Sale 0 si todo es «ok»; 1 si hay algun SILENCIOSO/PARIDAD/CRASH/RUIDOSO-otro.
import io, os, re, shutil, subprocess, sys, tempfile

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FE   = os.path.join(ROOT, 'lexer-java', 'target', 'basicplus-frontend.jar')
JVM  = os.path.join(ROOT, 'miVM', 'target', 'bpgenvm-1.0.jar')
CVM  = os.path.join(ROOT, 'bpgenvm-c', 'build', 'bpgenvm-c.exe')
CORE = os.path.join(ROOT, 'bpstdlib', 'Core.mod')

# ------------------------------------------------------------------ la libreria
LIB_V1 = """module Lib
import Core
public enum Color
  ROJO
  VERDE
end Color
public class Base
  var n: integer
  private var s: string
  public property p: integer
  public function Base(n: integer)
    this.n := n
    this.s := "s"
    this.p := n * 10
  end Base
  public function k(): string
    return "k" + this.n
  end k
  public function m(v: integer): integer
    return v + 1
  end m
  public function m(t: string): string
    return t + "!"
  end m
  public function suma(o: Base): integer
    return this.n + o.n
  end suma
  public function z(): integer
    return 99
  end z
end Base
public function f(): integer
  return 7
end f
end Lib
"""

def _sub(src, viejo, nuevo, veces=1):
    assert src.count(viejo) == veces, (viejo, src.count(viejo))
    return src.replace(viejo, nuevo)

# nombre -> (descripcion, transformacion del fuente v1)
VARIANTES = {
  'igual':        ('la misma fuente',                                  lambda s: s),
  'cuerpo':       ('solo cambia el CUERPO de k() (misma interfaz)',    lambda s: _sub(s, 'return "k" + this.n', 'return "K" + this.n')),
  'met_final':    ('metodo NUEVO al final de Base',                    lambda s: _sub(s, "  end z\nend Base", "  end z\n  public function extra(): integer\n    return 1\n  end extra\nend Base")),
  'met_medio':    ('metodo NUEVO insertado ANTES de k() (mueve todo)', lambda s: _sub(s, "  end Base\n  public function k()", "  end Base\n  public function primero(): integer\n    return 0\n  end primero\n  public function k()")),
  'met_quitado':  ('se QUITA k(), que las apps usan',                  lambda s: _sub(s, '  public function k(): string\n    return "k" + this.n\n  end k\n', '')),
  'met_quitado_nousado': ('se QUITA z(), el ultimo, que nadie usa',    lambda s: _sub(s, '  public function z(): integer\n    return 99\n  end z\n', '')),
  'firma':        ('m(v: integer) pasa a m(v: long): cambia la FIRMA de la cabeza', lambda s: _sub(s, 'public function m(v: integer): integer\n    return v + 1', 'public function m(v: long): integer\n    return 2')),
  'campo_mas':    ('CAMPO nuevo en Base (var extra) delante de s',      lambda s: _sub(s, '  var n: integer\n', '  var n: integer\n  var extra: integer\n')),
  'campo_menos':  ('se QUITA el campo privado s',                       lambda s: _sub(_sub(s, '  private var s: string\n', ''), '    this.s := "s"\n', '')),
  'prop_mas':     ('PROPERTY nueva (dos accesores delante de todo)',     lambda s: _sub(s, '  public property p: integer\n', '  public property p: integer\n  public property q: integer\n')),
  'reorden':      ('k() y suma() intercambian su orden',                lambda s: _sub(_sub(_sub(s, '  public function k(): string\n    return "k" + this.n\n  end k\n', '@@K@@'), '  public function suma(o: Base): integer\n    return this.n + o.n\n  end suma\n', '  public function k(): string\n    return "k" + this.n\n  end k\n'), '@@K@@', '  public function suma(o: Base): integer\n    return this.n + o.n\n  end suma\n')),
  'ev_privado':   ('un `function event` PRIVADO delante de k() (ocupa ranura, no se publica)', lambda s: _sub(s, "  end Base\n  public function k()", "  end Base\n  private function event pulsado(x: integer)\n    print \"handler \" + x\n  end pulsado\n  public function k()")),
  'enum_cabeza':  ('sobrecarga m(c: Color) DELANTE de m(integer) (enum: no exportable)', lambda s: _sub(s, "  public function m(v: integer): integer", "  public function m(c: Color): integer\n    return 100\n  end m\n  public function m(v: integer): integer")),
  'retorno':      ('k() pasa de devolver string a integer (misma ranura, mismo nombre)', lambda s: _sub(s, 'public function k(): string\n    return "k" + this.n', 'public function k(): integer\n    return this.n')),
  'prop_tipo':    ('la property p pasa de integer a string (mismos accesores)',        lambda s: _sub(_sub(s, 'public property p: integer', 'public property p: string'), 'this.p := n * 10', 'this.p := "p"')),
  'f_firma':      ('la funcion de modulo f() pasa a f(x: integer)',                    lambda s: _sub(s, 'public function f(): integer\n  return 7', 'public function f(x: integer): integer\n  return x')),
}

# ------------------------------------------------------------------ las apps
USA = """module Usa
import Core
import Lib
public function toca(x: Lib.Base): string
  x.p := 8
  return x.k() + "/" + x.p + "/" + x.m(1)
end toca
public function main()
  var b: Lib.Base := Lib.Base(3)
  print b.k()
  print b.m(4)
  print b.m("x")
  var c: Lib.Base := Lib.Base(5)
  print b.suma(c)
  print Lib.f()
  var o: Lib.Base := c
  print toca(o)
  print "fin"
end main
end Usa
"""
EXT = """module Ext
import Core
import Lib
public class Hija extends Lib.Base
  var q: integer
  public function Hija(n: integer)
    super(n)
    this.q := n * 2
  end Hija
  public function k(): string
    return "hija/" + super.k() + "/" + this.n + "/" + this.q
  end k
  public function propio(): integer
    return this.q + this.n
  end propio
end Hija
public function main()
  var h: Hija := Hija(3)
  print h.k()
  print h.propio()
  print h.m(4)
  var b: Lib.Base := h
  print b.k()
  b.p := 9
  print b.p
  print "fin"
end main
end Ext
"""
A_SRC = """module A
import Core
import Lib
public function crea(): Lib.Base
  return Lib.Base(3)
end crea
end A
"""
B_SRC = """module B
import Core
import Lib
public function usa(x: Lib.Base): string
  x.p := 4
  return x.k() + "/" + x.p + "/" + x.m(1)
end usa
end B
"""
PUENTE = """module Puente
import Core
import Lib
import A
import B
public function main()
  var o: Lib.Base := A.crea()
  print B.usa(o)
  print "fin"
end main
end Puente
"""

# Salida esperada de cada app contra la v1; las variantes con OTRO comportamiento
# la sobreescriben (solo el cuerpo cambia lo que se imprime; el resto es layout).
ESPERADO = {
  'Usa':    "k3\n5\nx!\n8\n7\nk5/8/2\nfin",
  'Ext':    "hija/k3/3/6\n9\n5\nhija/k3/3/6\n9\nfin",
  'Puente': "k3/4/2\nfin",
}
def esperado(app, variante):
    s = ESPERADO[app]
    if variante == 'cuerpo': s = s.replace('k3', 'K3').replace('k5', 'K5')
    if variante == 'firma':  s = s.replace('\n5\n', '\n2\n', 1)   # m(4) -> 2 (m(1) ya daba 2)
    return s

# ------------------------------------------------------------------ infra
def sh(args, cwd, timeout=60):
    p = subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    return p.returncode, p.stdout.decode('utf-8', 'replace'), p.stderr.decode('utf-8', 'replace')

def filt(texto):
    # el mismo filtro que compat.sh: fuera el banner y los blancos que lo preceden
    out, pend, vista = [], [], False
    for ln in texto.splitlines():
        if re.search(r'INICIANDO|FIN DE|heapStart|^config:', ln): pend = []; continue
        if not vista:
            if ln.strip() == '': continue
            vista = True
        if ln.strip() == '': pend.append(ln); continue
        out.extend(pend); pend = []; out.append(ln)
    return '\n'.join(out)

def run_vm(vm, d, mod):
    if vm.endswith('.jar'): args = ['java', '-Djava.awt.headless=true', '-jar', vm, '--no-config', mod]
    else:                   args = [vm, mod]
    try: rc, out, err = sh(args, d)
    except subprocess.TimeoutExpired: return -99, '<timeout>'
    return rc, filt(out)

def lib(work, variante, gen):
    d = os.path.join(work, 'lib_%s_g%d' % (variante, gen))
    if os.path.isdir(d): return d
    os.makedirs(os.path.join(d, 'src'))
    io.open(os.path.join(d, 'src', 'Lib.bp'), 'w', encoding='utf-8', newline='\n').write(VARIANTES[variante][1](LIB_V1))
    io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "Lib", "version": %d, "dependencies": [] }\n' % gen)
    shutil.copy(CORE, d)
    rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm'], d)
    if rc != 0 or not os.path.isfile(os.path.join(d, 'Lib.mod')):
        raise SystemExit('la variante %s (gen %d) no compila:\n%s%s' % (variante, gen, out, err))
    io.open(os.path.join(d, 'build.log'), 'w', encoding='utf-8').write(out + err)
    return d

def app(work, nombre, src, libdir, etiqueta):
    """Compila `nombre` contra los .mod de libdir; devuelve el dir con el .mod y los avisos."""
    d = os.path.join(work, 'app_%s_%s' % (nombre, etiqueta))
    if os.path.isdir(d): return d
    os.makedirs(d)
    for f in os.listdir(libdir):
        if f.endswith('.mod'): shutil.copy(os.path.join(libdir, f), d)
    io.open(os.path.join(d, nombre + '.bp'), 'w', encoding='utf-8', newline='\n').write(src)
    rc, out, err = sh(['java', '-jar', FE, nombre + '.bp', '--compile', '.', '--backend=mivm'], d)
    io.open(os.path.join(d, 'compile.log'), 'w', encoding='utf-8').write(out + err)
    if rc != 0 or not os.path.isfile(os.path.join(d, nombre + '.mod')):
        raise SystemExit('%s no compila contra %s:\n%s%s' % (nombre, etiqueta, out, err))
    return d

def caso_dir(work, etiqueta, libdir, mods):
    """Un directorio con la lib de runtime + los .mod de las apps (cada uno de su dir)."""
    d = os.path.join(work, 'caso_' + etiqueta)
    if os.path.isdir(d): shutil.rmtree(d)
    os.makedirs(d)
    for f in os.listdir(libdir):
        if f.endswith('.mod'): shutil.copy(os.path.join(libdir, f), d)
    for m in mods: shutil.copy(m, d)
    return d

RECHAZOS = [('mas antiguo', r'es mas antiguo que el que'), ('otra interfaz', r'no es la interfaz con la que se compilo'),
            ('mas reciente+extiende', r'es mas reciente que .* extiende una clase suya'), ('no enlaza', r'ERROR DE LINK|no exporta')]
def clasificar(rc, out):
    for nombre, rx in RECHAZOS:
        if re.search(rx, out): return nombre
    return 'corre' if rc == 0 else 'crash(rc=%d)' % rc

filas = []
def fila(bloque, etiqueta, d, mod, predicho, esperado_out=None):
    """predicho: 'corre' | uno de RECHAZOS[*][0]. esperado_out: stdout exacto si corre."""
    rj, oj = run_vm(JVM, d, mod + '.mod')
    rc_, oc = run_vm(CVM, d, mod + '.mod')
    cj, cc = clasificar(rj, oj), clasificar(rc_, oc)
    if oj != oc:
        veredicto = 'PARIDAD'
    elif cj.startswith('crash'):
        veredicto = 'CRASH'
    elif predicho == 'corre':
        if cj != 'corre': veredicto = 'RUIDOSO-otro'
        elif esperado_out is not None and oj.strip() != esperado_out.strip(): veredicto = 'SILENCIOSO'
        else: veredicto = 'ok'
    else:
        if cj == 'corre': veredicto = 'SILENCIOSO' if (esperado_out is None or oj.strip() != esperado_out.strip()) else 'SILENCIOSO(corre y acierta)'
        elif cj == predicho: veredicto = 'ok'
        else: veredicto = 'RUIDOSO-otro'
    linea = (oj.strip().splitlines() or [''])[0] if cj != 'corre' else oj.strip().replace('\n', '|')
    filas.append((bloque, etiqueta, predicho, cj if cj == cc else cj + ' / ' + cc, veredicto, linea[:110]))
    marca = {'ok': '  ok  ', 'SILENCIOSO': '  !!  ', 'PARIDAD': '  !!  ', 'CRASH': '  !!  ', 'RUIDOSO-otro': '  ??  '}.get(veredicto, '  !!  ')
    print('%s[%s] %-46s predicho=%-22s real=%-22s %s' % (marca, bloque, etiqueta, predicho, cj if cj == cc else cj + '/' + cc, veredicto))
    if veredicto != 'ok': print('         ' + linea[:150])
    return veredicto

# ------------------------------------------------------------------ los bloques
# Lo que la norma predice para la MEZCLA (app sellada contra v1 gen 7, solo USA):
#   mueve ranuras usadas -> no enlaza; lo demas -> corre (gen 8) / otra interfaz (gen 7)
MUEVE = {'met_medio', 'met_quitado', 'prop_mas', 'reorden', 'ev_privado'}
# Lo que HOY se sabe que el marcador NO ve (la cabeza `m` es un nombre pelado sin firma):
#   'firma' y 'enum_cabeza' dejan `Base#m#N` en su sitio con OTRA firma -> la prediccion
#   de la norma es «no enlaza», y lo que pase de verdad es lo que se mide.
def prediccion_usa(variante, gen):
    if gen < 7: return 'mas antiguo'
    if gen == 7: return 'corre' if variante in ('igual', 'cuerpo') else 'otra interfaz'
    if variante in MUEVE or variante in ('firma', 'enum_cabeza', 'met_quitado', 'retorno', 'prop_tipo', 'f_firma'): return 'no enlaza'
    return 'corre'
def prediccion_ext(variante, gen):
    if gen < 7: return 'mas antiguo'
    if gen == 7: return 'corre' if variante in ('igual', 'cuerpo') else 'otra interfaz'
    return 'mas reciente+extiende'

def main():
    solo = None; keep = False; work = None
    for a in sys.argv[1:]:
        if a.startswith('--solo='): solo = a[7:]
        elif a == '--keep': keep = True
        elif a.startswith('--work='): work = a[7:]
    for f in (FE, JVM, CVM, CORE):
        if not os.path.isfile(f): raise SystemExit('falta ' + f)
    work = work or tempfile.mkdtemp(prefix='mezcla_')
    print('trabajo en', work)
    v1 = lib(work, 'igual', 7)
    usa1 = app(work, 'Usa', USA, v1, 'v1'); ext1 = app(work, 'Ext', EXT, v1, 'v1')

    if solo in (None, 'I'):
        print('\n== BLOQUE I: MISMA VERSION (app compilada contra X, ejecutada contra X): tiene que correr siempre ==')
        for v in VARIANTES:
            d = lib(work, v, 7)
            for nombre, src in (('Usa', USA), ('Ext', EXT)):
                if v in ('met_quitado', 'retorno', 'prop_tipo', 'f_firma'): continue   # las apps no compilan contra ellas: no son casos de I
                ad = app(work, nombre, src, d, v)
                cd = caso_dir(work, 'I_%s_%s' % (v, nombre), d, [os.path.join(ad, nombre + '.mod')])
                fila('I', '%s contra %s' % (nombre, v), cd, nombre, 'corre', esperado(nombre, v))

    if solo in (None, 'II'):
        print('\n== BLOQUE II: MEZCLA (app sellada contra v1 gen 7, ejecutada contra cada variante) ==')
        for v in VARIANTES:
            for gen in (6, 7, 8):
                if gen == 6 and v not in ('igual', 'met_final'): continue          # mas antiguo: dos muestras bastan
                if gen == 7 and v not in ('igual', 'cuerpo', 'met_final', 'campo_mas'): continue
                d = lib(work, v, gen)
                cd = caso_dir(work, 'II_%s_g%d' % (v, gen), d, [os.path.join(usa1, 'Usa.mod'), os.path.join(ext1, 'Ext.mod')])
                fila('II', 'Usa v1 contra %s g%d' % (v, gen), cd, 'Usa', prediccion_usa(v, gen), esperado('Usa', v))
                fila('II', 'Ext v1 contra %s g%d' % (v, gen), cd, 'Ext', prediccion_ext(v, gen), esperado('Ext', v))

    if solo in (None, 'P'):
        print('\n== BLOQUE P: PUENTE (A sellado contra v1 g7 crea el objeto; B sellado contra X lo usa; runtime = X o v1) ==')
        a1 = app(work, 'A', A_SRC, v1, 'v1')
        for v in ('igual', 'met_final', 'met_medio', 'firma', 'enum_cabeza', 'campo_mas', 'cuerpo'):
            for gen in (7, 8):
                if gen == 7 and v not in ('igual', 'cuerpo'): continue
                dx = lib(work, v, gen)
                bx = app(work, 'B', B_SRC, dx, '%s_g%d' % (v, gen))
                # Puente (sellado contra X) importa A y B: se compila en un dir con la lib X + A.mod + B.mod.
                pd = os.path.join(work, 'app_Puente_%s_g%d' % (v, gen))
                if not os.path.isdir(pd):
                    os.makedirs(pd)
                    for f in os.listdir(dx):
                        if f.endswith('.mod'): shutil.copy(os.path.join(dx, f), pd)
                    shutil.copy(os.path.join(a1, 'A.mod'), pd); shutil.copy(os.path.join(bx, 'B.mod'), pd)
                    io.open(os.path.join(pd, 'Puente.bp'), 'w', encoding='utf-8', newline='\n').write(PUENTE)
                    rc, out, err = sh(['java', '-jar', FE, 'Puente.bp', '--compile', '.', '--backend=mivm'], pd)
                    if rc != 0 or not os.path.isfile(os.path.join(pd, 'Puente.mod')):
                        raise SystemExit('Puente no compila contra %s g%d:\n%s%s' % (v, gen, out, err))
                # runtime = X: A (sellado v1) solo construye (por nombre) -> «gen mayor, usa: enlaza»; B y Puente son de X.
                # Prediccion: corre siempre. Lo que se mide es si B, compilado CONTRA X, hace lo que X dice.
                cd = caso_dir(work, 'P_%s_g%d_rtX' % (v, gen), dx, [os.path.join(a1, 'A.mod'), os.path.join(bx, 'B.mod'), os.path.join(pd, 'Puente.mod')])
                fila('P', 'A(v1)+B(%s g%d), runtime %s g%d' % (v, gen, v, gen), cd, 'Puente', 'corre', esperado('Puente', v))
                if gen == 8:
                    # runtime = v1 (gen 7): B esta sellado contra gen 8 -> «mas antiguo».
                    cd = caso_dir(work, 'P_%s_g%d_rtv1' % (v, gen), v1, [os.path.join(a1, 'A.mod'), os.path.join(bx, 'B.mod'), os.path.join(pd, 'Puente.mod')])
                    fila('P', 'A(v1)+B(%s g%d), runtime v1 g7' % (v, gen), cd, 'Puente', 'mas antiguo', esperado('Puente', 'igual'))

    # ---- resumen
    malos = [f for f in filas if f[4] != 'ok']
    print('\n== RESUMEN: %d filas, %d ok, %d con veredicto distinto de ok ==' % (len(filas), len(filas) - len(malos), len(malos)))
    for f in malos: print('  %-12s [%s] %s  (predicho %s, real %s): %s' % (f[4], f[0], f[1], f[2], f[3], f[5]))
    io.open(os.path.join(work, 'mezcla.tsv'), 'w', encoding='utf-8').write('\n'.join('\t'.join(f) for f in filas) + '\n')
    print('tabla en', os.path.join(work, 'mezcla.tsv'))
    if not keep and not malos: shutil.rmtree(work, ignore_errors=True)
    return 1 if malos else 0

if __name__ == '__main__':
    sys.exit(main())
