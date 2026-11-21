#!/usr/bin/env python3
# compat/v10_casos.py -- V7/H2 (#523), V10 (16-sep): las reproducciones de los hallazgos de la revision
# adversarial que se ARREGLARON, como casos ejecutables en las dos VMs (MOD9). Uso:
#   python compat/v10_casos.py <dir con la stdlib compilada como MOD9>   (sin el dir: se saltan los casos con Gui)
import io, os, re, shutil, subprocess, sys, tempfile
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
ROOT = 'C:/lenguajes/pm'
FE = ROOT + '/toolchain/lexer-java/target/basicplus-frontend.jar'
JVM = ROOT + '/toolchain/miVM/target/bpgenvm-1.0.jar'
CVM = ROOT + '/bpgenvm-c/build/bpgenvm-c.exe'
STD9 = sys.argv[1] if len(sys.argv) > 1 else None   # dir con la stdlib compilada como MOD9 (para Gui)
W = tempfile.mkdtemp(prefix='v10r_')

def sh(args, cwd):
    p = subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    return p.returncode, p.stdout.decode('utf-8', 'replace'), p.stderr.decode('utf-8', 'replace')
def filt(t):
    out = []
    for ln in t.splitlines():
        if re.search(r'INICIANDO|FIN DE|heapStart|^config:|^\[bpvm|^\[miVM|^\s*$', ln): continue
        out.append(ln.rstrip())
    return '\n'.join(out)
def libs(d, mods, gen, formato=9):
    """compila varios modulos como proyecto (main = el ultimo); devuelve el dir con los .mod"""
    os.makedirs(d + '/src', exist_ok=True)
    for name, src in mods: io.open(d + '/src/' + name + '.bp', 'w', encoding='utf-8', newline='\n').write(src)
    io.open(d + '/p.bpbuild', 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "%s", "version": %d, "dependencies": ["%s"] }\n' % (mods[-1][0], gen, STD9 or ROOT + '/bpstdlib'))
    shutil.copy((STD9 or ROOT + '/bpstdlib') + '/Core.mod', d)   # como mezcla.py: la VM la busca al lado
    rc, o, e = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', '--formato=%d' % formato], d)
    return rc, o + e
def app(d, name, src, libdirs, formato=9):
    os.makedirs(d, exist_ok=True)
    shutil.copy((STD9 or ROOT + '/bpstdlib') + '/Core.mod', d)
    for ld in libdirs:
        for f in os.listdir(ld):
            if f.endswith('.mod'): shutil.copy(os.path.join(ld, f), d)
    io.open(d + '/' + name + '.bp', 'w', encoding='utf-8', newline='\n').write(src)
    rc, o, e = sh(['java', '-jar', FE, name + '.bp', '--compile', '.', '--backend=mivm', '--formato=%d' % formato], d)
    return rc, o + e
def run(d, mod):
    rj, oj, ej = sh(['java', '-Djava.awt.headless=true', '-jar', JVM, '--no-config', mod], d)
    rc, oc, ec = sh([CVM, '--libdir=' + (STD9 or ROOT + '/bpstdlib'), mod], d)
    return filt(oj), filt(oc), rj, rc, ej, ec

res = []
def caso(nombre, ok, detalle):
    res.append((nombre, ok))
    print(('  ok  ' if ok else '  !!  ') + nombre + ('' if ok else '\n        ' + detalle.replace('\n', '\n        ')))

CORE = 'import Core\n'
# ---------------------------------------------------------------- EC-1: property por la clase estatica del receptor
LIB1 = 'module Lib\n' + CORE + '''public class Base
  public property p: integer
  public function Base(n: integer)
    this.p := n
  end Base
end Base
public class Derived extends Base
  public function Derived(n: integer)
    super(n)
  end Derived
end Derived
end Lib
'''
LIB2 = 'module Lib\n' + CORE + '''public class Base
  public property p: integer
end Base
public class Derived
  public property q: integer
  public property p: integer
  public function Derived(n: integer)
    this.q := 999
    this.p := n
  end Derived
end Derived
end Lib
'''
LIB3 = LIB2.replace('  public property p: integer\n  public function Derived', '  public function Derived').replace('    this.p := n\n', '')
APP_P = 'module App\n' + CORE + '''import Lib
public function main()
  var d: Lib.Derived := Lib.Derived(7)
  print d.p
  d.p := 8
  print d.p
  print "fin"
end main
end App
'''
libs(W + '/ec1_v1', [('Lib', LIB1)], 7); libs(W + '/ec1_v2', [('Lib', LIB2)], 8); libs(W + '/ec1_v3', [('Lib', LIB3)], 8)
rc, log = app(W + '/ec1_app', 'App', APP_P, [W + '/ec1_v1']); assert rc == 0, log
for v, esp in (('v1', '7\n8\nfin'), ('v2', '7\n8\nfin'), ('v3', "necesita 'Lib.Derived.getP()i'")):
    d = W + '/ec1_run_' + v; os.makedirs(d); shutil.copy(W + '/ec1_app/App.mod', d); shutil.copy(W + '/ec1_' + v + '/Lib.mod', d); shutil.copy((STD9 or ROOT + '/bpstdlib') + '/Core.mod', d)
    oj, oc, rj, rcc, ej, ec = run(d, 'App.mod')
    caso('EC-1 property via Derived, Lib ' + v, oj == oc and (oj.strip() == esp or esp in oj), 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- F1/REF-2: privados manda la declaracion mas cercana
LIBG = 'module Lib\n' + CORE + '''public class G
  private function event h(x: integer)
    print "G.h " + x
  end h
  public function g(): integer
    return 3
  end g
end G
public class P extends G
  public function h(x: integer)
    print "P.h " + x
  end h
end P
end Lib
'''
APP_G = 'module App\n' + CORE + '''import Lib
public class H extends Lib.P
  public function h(x: integer)
    print "H.h " + x
  end h
end H
public function main()
  var p: Lib.P := Lib.P()
  p.h(7)
  var hh: H := H()
  hh.h(8)
  var ph: Lib.P := hh
  ph.h(9)
  print "fin"
end main
end App
'''
libs(W + '/f1_lib', [('Lib', LIBG)], 7)
rc, log = app(W + '/f1_app', 'App', APP_G, [W + '/f1_lib']); assert rc == 0, log
oj, oc, rj, rcc, ej, ec = run(W + '/f1_app', 'App.mod')
caso('F1 privado del abuelo publicado por el padre: corre en las dos', oj == oc and oj.strip() == 'P.h 7\nH.h 8\nH.h 9\nfin', 'java=%r\nc=%r\nej=%s' % (oj, oc, ej[-300:]))

# ---------------------------------------------------------------- E-R2-2/F10: un privado del ancestro no es override
LIBB = 'module Lib\n' + CORE + '''public class B
  private property q: integer
  private function h(): integer
    return 1
  end h
  public function g(): integer
    return this.h() + this.q
  end g
end B
end Lib
'''
APP_B = 'module App\n' + CORE + '''import Lib
public class C extends Lib.B
  public property q: integer
  public function h(): integer
    return 2
  end h
end C
public function main()
  var c: C := C()
  c.q := 5
  print c.g()
  print c.h()
  print c.q
  print "fin"
end main
end App
'''
libs(W + '/e2_lib', [('Lib', LIBB)], 7)
rc, log = app(W + '/e2_app', 'App', APP_B, [W + '/e2_lib']); assert rc == 0, log
oj, oc, rj, rcc, ej, ec = run(W + '/e2_app', 'App.mod')
caso('E-R2-2 homonimo de un privado del padre: no es override, no hay E2', oj == oc and oj.strip() == '1\n2\n5\nfin', 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- H2: evento con parametro de clase, suscrito desde otro modulo
LIBE = 'module Lib\n' + CORE + '''public class Emisor
  event propio(e: Emisor)
  public function dispara()
    raise propio(this)
  end dispara
end Emisor
end Lib
'''
APP_E = 'module App\n' + CORE + '''import Lib
public class Oyente
  public function event alPropio(e: Lib.Emisor)
    print "oyente"
  end alPropio
end Oyente
public function main()
  var em: Lib.Emisor := Lib.Emisor()
  var o: Oyente := Oyente()
  em.propio := o::alPropio
  em.dispara()
  sleep(50)
  print "fin"
end main
end App
'''
libs(W + '/h2_lib', [('Lib', LIBE)], 7)
rc, log = app(W + '/h2_app', 'App', APP_E, [W + '/h2_lib'])
if rc != 0: caso('H2 evento con parametro de clase: compila', False, log[-600:])
else:
    oj, oc, rj, rcc, ej, ec = run(W + '/h2_app', 'App.mod')
    caso('H2 evento con parametro de clase: suscribe y dispara', oj == oc and 'oyente' in oj, 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- H1: clase de una dep TRANSITIVA (App no importa Lib)
LIBT = 'module Lib\n' + CORE + '''public class Base
  public function k(): string
    return "Lk"
  end k
end Base
end Lib
'''
MIDT = 'module Mid\n' + CORE + '''import Lib
public function dame(): Lib.Base
  return Lib.Base()
end dame
public function toma(b: Lib.Base): string
  return b.k()
end toma
end Mid
'''
APP_T = 'module App\n' + CORE + '''import Mid
public function main()
  print Mid.toma(Mid.dame())
  print "fin"
end main
end App
'''
libs(W + '/h1_lib', [('Lib', LIBT), ('Mid', MIDT)], 7)
rc, log = app(W + '/h1_app', 'App', APP_T, [W + '/h1_lib'])
if rc != 0: caso('H1 dep transitiva: compila', False, log[-600:])
else:
    oj, oc, rj, rcc, ej, ec = run(W + '/h1_app', 'App.mod')
    caso('H1 dep transitiva: corre', oj == oc and oj.strip() == 'Lk\nfin', 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- E-09/EC-3: throw de una clase que deja de extender Exception
LIBX1 = 'module Lib\n' + CORE + '''public class MiError extends Core.Exception
  public function MiError(m: string)
    super(m)
  end MiError
end MiError
end Lib
'''
LIBX2 = 'module Lib\n' + CORE + '''public class MiError
  var m: string
  public function MiError(m: string)
    this.m := m
  end MiError
end MiError
end Lib
'''
APP_X = 'module App\n' + CORE + '''import Lib
public function main()
  try
    throw Lib.MiError("x")
  catch e: Core.Exception
    print "cogida"
  endtry
  print "fin"
end main
end App
'''
libs(W + '/x_v1', [('Lib', LIBX1)], 7); libs(W + '/x_v2', [('Lib', LIBX2)], 8)
rc, log = app(W + '/x_app', 'App', APP_X, [W + '/x_v1']); assert rc == 0, log
for v, esp in (('v1', 'cogida\nfin'), ('v2', "asume que 'Lib.MiError' extiende 'Core.Exception'")):
    d = W + '/x_run_' + v; os.makedirs(d); shutil.copy(W + '/x_app/App.mod', d); shutil.copy(W + '/x_' + v + '/Lib.mod', d); shutil.copy((STD9 or ROOT + '/bpstdlib') + '/Core.mod', d)
    oj, oc, rj, rcc, ej, ec = run(d, 'App.mod')
    caso('E-09 throw de MiError, Lib ' + v, oj == oc and (oj.strip() == esp or esp in oj), 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- H7: la clase del PROPIO modulo en una funcion importada (homonimos)
MIDA = 'module MidA\n' + CORE + '''public class Caja
  public function uno(): string
    return "A.uno"
  end uno
end Caja
public function crea(): Caja
  return Caja()
end crea
end MidA
'''
MIDB = 'module MidB\n' + CORE + '''public class Caja
  public function tres(): string
    return "B.tres"
  end tres
end Caja
public function crea(): Caja
  return Caja()
end crea
end MidB
'''
APP_H7 = 'module App\n' + CORE + '''import MidA
import MidB
public function main()
  print MidA.crea().uno()
  print MidB.crea().tres()
  print "fin"
end main
end App
'''
libs(W + '/h7_a', [('MidA', MIDA)], 7); libs(W + '/h7_b', [('MidB', MIDB)], 7)
rc, log = app(W + '/h7_app', 'App', APP_H7, [W + '/h7_a', W + '/h7_b'])
if rc != 0: caso('H7 homonimos: compila', False, log[-600:])
else:
    oj, oc, rj, rcc, ej, ec = run(W + '/h7_app', 'App.mod')
    caso('H7 homonimos: cada crea() devuelve la Caja de su modulo', oj == oc and oj.strip() == 'A.uno\nB.tres\nfin', 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- E-01/F5: «(tiene …)» de un campo, mismo texto en las dos VMs
LIBN1 = 'module Lib\n' + CORE + '''public class R
  var n: string
end R
public class Base extends R
  var n: integer
end Base
end Lib
'''
LIBN2 = LIBN1.replace('  var n: integer\n', '  var n: long\n')
APP_N = 'module App\n' + CORE + '''import Lib
public class Hija extends Lib.Base
  public function f(): integer
    return this.n
  end f
end Hija
public function main()
  print Hija().f()
end main
end App
'''
rc, log = libs(W + '/n_v1', [('Lib', LIBN1)], 7)
if rc != 0: caso('E-01 (campo homonimo en la cadena no compila en Lib: caso no aplicable)', True, log[-300:])
else:
    libs(W + '/n_v2', [('Lib', LIBN2)], 8)
    rc, log = app(W + '/n_app', 'App', APP_N, [W + '/n_v1']); assert rc == 0, log
    d = W + '/n_run'; os.makedirs(d); shutil.copy(W + '/n_app/App.mod', d); shutil.copy(W + '/n_v2/Lib.mod', d); shutil.copy((STD9 or ROOT + '/bpstdlib') + '/Core.mod', d)
    oj, oc, rj, rcc, ej, ec = run(d, 'App.mod')
    caso('E-01 E4 con dos candidatas: mismo texto', oj == oc and "(tiene 'n:j', 'n:s')" in oj, 'java=%r\nc=%r' % (oj, oc))

# ---------------------------------------------------------------- E-10: MOD8 extendiendo una dep bpi 9 → error, no traza
rc, log = app(W + '/e10_app', 'App', APP_G, [W + '/f1_lib'], formato=8)
caso('E-10 MOD8 sobre dep bpi 9: error dicho, sin traza Java', rc != 0 and 'Exception in thread' not in log and 'bpi 9' in log, log[-500:])

# ---------------------------------------------------------------- F6/REF-1: reflexion por nombre = ranura minima en las dos VMs
if STD9:
    LIBR = 'module Lib\n' + CORE + '''public class Base
  public function m(t: string): string
    return "ms" + t
  end m
  public function m(v: integer): integer
    return v + 100
  end m
end Base
end Lib
'''
    APP_R = 'module MiApp\n' + CORE + '''import Lib
import Gui
public class H extends Lib.Base
  public function m(v: integer): integer
    return v + 200
  end m
end H
public function main()
  var h: H := H()
  print h.m(1)
  print h.m("x")
  print Gui.__guiSlotOf(h, "m")
  print "fin"
end main
end MiApp
'''
    libs(W + '/r_lib', [('Lib', LIBR)], 7)
    rc, log = app(W + '/r_app', 'MiApp', APP_R, [W + '/r_lib', STD9])
    if rc != 0: caso('F6 reflexion: compila', False, log[-600:])
    else:
        oj, oc, rj, rcc, ej, ec = run(W + '/r_app', 'MiApp.mod')
        caso('F6 __guiSlotOf con sobrecarga redefinida: misma ranura en las dos', oj == oc and oj.strip().splitlines()[2:3] == ['2'], 'java=%r\nc=%r\nej=%s\nec=%s' % (oj, oc, ej[-300:], ec[-300:]))

print('\n== %d casos, %d ok ==' % (len(res), sum(1 for _, o in res if o)))
print('trabajo en', W)
