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
#   python compat/mezcla.py [--solo=I|II|P|B|V] [--keep] [--work=DIR] [--formato=8|9] [--vms=ambas|java|c]
#
# Bloque V (solo con --formato=9, V7/H2 #523 V4, 17-sep): las filas que faltaban de la matriz. El
# override HUERFANO (H redefine z() y Base pierde z(): E2 nombrando el metodo), el ABUELO en tres
# modulos (Lib -> Mid -> Abu, sellados contra v1, cambia solo Lib: 21 variantes), DOS averias a la
# vez (dos_mueven corre; dos_una_rompe: el mismo primer error en las dos VMs) y MOD8 x MOD9 en las
# dos direcciones (nunca corre, nunca peta; E9 «recompila 'Lib'» cuando la app MOD9 esta sellada
# contra la Lib MOD8, la norma #500 cuando las generaciones no casan). Y V6: el GC con los bitmaps
# que el cargador reconstruye (app Gc: 300 objetos vivos de una clase abierta con dos campos de
# referencia propios, gc() forzado; Base con un campo de referencia nuevo delante, con 33 campos
# —la segunda palabra del bitmap—, y barajada/rellena en el bloque B).
#
# Bloque B (solo con --formato=9, V7/H2 #523 V1): EL GUARDIAN. La misma Lib compilada con
# `--barajar=<semilla>` (las ranuras propias de cada clase permutadas) y con `--relleno=3`
# (tres properties privadas de relleno por clase); las apps selladas contra la semilla 0
# tienen que dar el MISMO stdout contra cualquier semilla, en las dos VMs. Caza cualquier
# numero horneado que se escape del censo, sin leer el censo. Se comprueba el ARTEFACTO:
# si dos Lib.mod barajadas salen identicas, el flag no ha hecho nada y se aborta.
#
# La fila «derived_deja_de_extender» (A1): Lib tiene `Derived extends Base`; la app UsaD hace
# `var b: Lib.Base := Lib.Derived(1)` y llama b.k(). En la variante, Derived deja de extender
# Base (con los mismos miembros). Con MOD9 el .mod de UsaD lleva la asuncion (ref SUBTIPO) y el
# cargador rechaza nombrandola (E10); con MOD8 se mide lo que pasa.
#
# --formato=9 (V7/H2 #523, ENLACE TARDIO): compila todo como MOD9 y predice con EL CRITERIO
# DE JAVA — donde Java correria (miembro anadido en cualquier sitio, property nueva, campo
# nuevo, reorden, manejador privado, sobrecarga delante de la cabeza; TAMBIEN para la
# subclase Ext), corre con la salida correcta; donde Java lanzaria (quitado, firma o retorno
# cambiados, tipo de property cambiado), rechazo AL ENLAZAR nombrando el simbolo. Una
# dependencia mas antigua que la sellada sigue rechazandose (gen).
# --vms=java mientras la VM-C no cargue MOD9 (la paridad no se comprueba: una sola VM).
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
public class Derived extends Base
  var d: integer
  public function Derived(n: integer)
    super(n)
    this.d := n + 100
  end Derived
  public function dd(): integer
    return this.d
  end dd
end Derived
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
  'derived_k_propio': ('Derived declara su PROPIO k(): el super.k() de ExtD (sellado cuando k() vivia en el abuelo) tiene que ir al padre',
                   lambda s: _sub(s, '  public function dd(): integer\n', '  public function k(): string\n    return "dk" + this.n\n  end k\n  public function dd(): integer\n')),
  'derived_deja_de_extender': ('Derived DEJA de extender Base (conserva k() propio): la asuncion S<:T del consumidor ya no vale',
                   lambda s: _sub(_sub(s, 'public class Derived extends Base\n', 'public class Derived\n'),
                                  '    super(n)\n    this.d := n + 100\n  end Derived\n',
                                  '    this.d := n + 100\n  end Derived\n  public function k(): string\n    return "dk" + this.d\n  end k\n')),
  # V4 (17-sep): DOS averias a la vez. Las dos estructurales -> corre; una estructural y un miembro usado
  # quitado -> el rechazo nombra el miembro (y las dos VMs dicen el MISMO primer error).
  'dos_mueven':   ('met_medio + campo_mas (dos cambios de forma a la vez)',   lambda s: VARIANTES['campo_mas'][1](VARIANTES['met_medio'][1](s))),
  'dos_una_rompe': ('met_medio + met_quitado (se mueve todo Y falta k())',    lambda s: VARIANTES['met_quitado'][1](VARIANTES['met_medio'][1](s))),
  # V6 (17-sep): el GC con los bitmaps que el cargador reconstruye. Un campo de REFERENCIA nuevo en Base delante de
  # los del hijo, y 33 campos enteros (los del hijo caen en la SEGUNDA palabra del bitmap).
  'campo_ref_mas': ('campo de REFERENCIA nuevo en Base (var r: string) delante de s, inicializado',
                   lambda s: _sub(_sub(s, '  var n: integer\n', '  var n: integer\n  var r: string\n'), '    this.s := "s"\n', '    this.s := "s"\n    this.r := "r" + n\n')),
  'campos_33':    ('33 campos enteros nuevos en Base delante de s (el hijo cruza la palabra del bitmap)',
                   lambda s: _sub(s, '  var n: integer\n', '  var n: integer\n  var f0: integer\n  var f1: integer\n  var f2: integer\n  var f3: integer\n  var f4: integer\n  var f5: integer\n  var f6: integer\n  var f7: integer\n  var f8: integer\n  var f9: integer\n  var f10: integer\n  var f11: integer\n  var f12: integer\n  var f13: integer\n  var f14: integer\n  var f15: integer\n  var f16: integer\n  var f17: integer\n  var f18: integer\n  var f19: integer\n  var f20: integer\n  var f21: integer\n  var f22: integer\n  var f23: integer\n  var f24: integer\n  var f25: integer\n  var f26: integer\n  var f27: integer\n  var f28: integer\n  var f29: integer\n  var f30: integer\n  var f31: integer\n  var f32: integer\n')),
}

# ------------------------------------------------------------------ V8 H7.3 (#756): las interfaces
# Una libreria con dos interfaces (Medible hereda de Forma y lleva una property), una clase suya que implementa Forma y
# una funcion que llama por ella; y una app que implementa Medible, llama por las dos y asigna la clase de la libreria a
# una Forma (la ref SUBTIPO). La app, sellada contra v1; cada variante, con el criterio de MOD9: lo que no cambia una
# clave usada corre (las posiciones las pone el cargador: reordenar, un padre nuevo, un metodo nuevo que la clase ya
# tiene); lo que deja a la clase sin un metodo de su interfaz (E11), sin la interfaz de lo que dice implementar (E12),
# sin la interfaz (E1), con una clase donde habia una interfaz, o la asuncion «Punto implementa Forma» rota, NO ENLAZA,
# nombrandolo, en las dos VMs con el mismo texto.
ILIB_V1 = """module ILib
import Core
public interface Forma
  function area(): double
  function nombre(): string
end Forma
public interface Medible extends Forma
  property escala: integer
end Medible
public class Punto implements Forma
  public function Punto()
  end Punto
  @implement{ Forma }
  public function area(): double
    return 0.0
  end area
  @implement{ Forma }
  public function nombre(): string
    return "punto"
  end nombre
end Punto
public function describe(f: Forma): string
  return f.nombre() + " " + f.area()
end describe
end ILib
"""
IVARIANTES = {
  'igual':            ('la misma fuente', lambda s: s),
  'ifaz_reorden':     ('los dos metodos de Forma, en el otro orden',
                       lambda s: _sub(s, '  function area(): double\n  function nombre(): string\n', '  function nombre(): string\n  function area(): double\n')),
  'ifaz_met_mas':     ('Forma gana perimetro(), que la clase de la app no tiene (E11)',
                       lambda s: _sub(_sub(s, '  function nombre(): string\nend Forma', '  function nombre(): string\n  function perimetro(): double\nend Forma'),
                                      '    return "punto"\n  end nombre\n', '    return "punto"\n  end nombre\n  @implement{ Forma }\n  public function perimetro(): double\n    return 0.0\n  end perimetro\n')),
  'ifaz_met_mas_lo_tiene': ('Forma gana extra(), que la clase de la app ya tiene (publico)',
                       lambda s: _sub(_sub(s, '  function nombre(): string\nend Forma', '  function nombre(): string\n  function extra(): integer\nend Forma'),
                                      '    return "punto"\n  end nombre\n', '    return "punto"\n  end nombre\n  @implement{ Forma }\n  public function extra(): integer\n    return 0\n  end extra\n')),
  'ifaz_padre_mas':   ('nombre() pasa a un padre nuevo de Forma (Nombrado): cambian las posiciones, no las claves',
                       lambda s: _sub(s, 'public interface Forma\n  function area(): double\n  function nombre(): string\nend Forma\n',
                                      'public interface Nombrado\n  function nombre(): string\nend Nombrado\npublic interface Forma extends Nombrado\n  function area(): double\nend Forma\n')),
  'ifaz_prop_quitada': ('Medible pierde la property que la clase de la app dice implementar (E12)',
                       lambda s: _sub(s, '  property escala: integer\n', '')),
  'ifaz_quitada':     ('se quita la interfaz Medible, que la clase de la app implementa (E1)',
                       lambda s: _sub(s, 'public interface Medible extends Forma\n  property escala: integer\nend Medible\n', '')),
  'ifaz_es_clase':    ('Medible pasa de interfaz a clase',
                       lambda s: _sub(s, 'public interface Medible extends Forma\n  property escala: integer\nend Medible\n',
                                      'public class Medible\n  public property escala: integer\nend Medible\n')),
  'deja_de_implementar': ('Punto deja de implementar Forma: la asuncion de la app (Punto en una Forma) ya no vale',
                       lambda s: _sub(_sub(s, 'public class Punto implements Forma\n', 'public class Punto\n'), '  @implement{ Forma }\n', '', 2)),
}
IROMPE = {'ifaz_met_mas', 'ifaz_prop_quitada', 'ifaz_quitada', 'ifaz_es_clase', 'deja_de_implementar'}
IUSA = """module IUsa
import Core
import ILib
public class Rect implements ILib.Medible
  var an: double
  @implement{ Medible }
  public property escala: integer
  public function Rect(a: double)
    this.an := a
    this.escala := 1
  end Rect
  @implement{ Forma }
  public function area(): double
    return this.an * this.escala
  end area
  @implement{ Forma }
  public function nombre(): string
    return "rect"
  end nombre
  public function extra(): integer
    return 5
  end extra
end Rect
function main()
  var r: Rect := Rect(2.0)
  var f: ILib.Forma := r
  print ILib.describe(f)
  var m: ILib.Medible := r
  m.escala := 3
  print "escala", m.escala, f.area()
  var p: ILib.Forma := ILib.Punto()
  print ILib.describe(p), p instanceof ILib.Medible
end main
end IUsa
"""
IESPERADO = "rect 2\nescala 3 6\npunto 0 false"

def ilib(work, variante, gen):
    """La variante de ILib compilada (cache por dir), como lib()."""
    d = os.path.join(work, 'ilib_%s_g%d' % (variante, gen))
    if os.path.isdir(d): return d
    os.makedirs(os.path.join(d, 'src'))
    io.open(os.path.join(d, 'src', 'ILib.bp'), 'w', encoding='utf-8', newline='\n').write(IVARIANTES[variante][1](ILIB_V1))
    io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "ILib", "version": %d, "dependencies": [] }\n' % gen)
    shutil.copy(CORE, d)
    rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', '--formato=9'], d)
    if rc != 0 or not os.path.isfile(os.path.join(d, 'ILib.mod')):
        raise SystemExit('la variante %s de ILib (gen %d) no compila:\n%s%s' % (variante, gen, out, err))
    return d

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
EXT_D = """module ExtD
import Core
import Lib
public class Nieta extends Lib.Derived
  public function Nieta(n: integer)
    super(n)
  end Nieta
  public function k(): string
    return "nieta/" + super.k()
  end k
end Nieta
public function main()
  var x: Nieta := Nieta(2)
  print x.k()
  var b: Lib.Base := x
  print b.k()
  print "fin"
end main
end ExtD
"""
USA_D = """module UsaD
import Core
import Lib
public function main()
  var b: Lib.Base := Lib.Derived(1)
  print b.k()
  print "fin"
end main
end UsaD
"""
# V4 (17-sep) — override HUERFANO: H redefine z() sin llamar a super; si Base pierde z(), el override ya no
# redefine nada. Decision de diseno (V7_IDEAS §H2 E2, fixture e2 de miVM): rechazo nombrando el metodo, no
# «metodo nuevo en silencio» (Java lo dejaria correr; aqui el programador escribio una redefinicion y el
# cargador se lo dice).
HUER = """module Huer
import Core
import Lib
public class H extends Lib.Base
  public function H(n: integer)
    super(n)
  end H
  public function z(): integer
    return 1
  end z
end H
public function main()
  var h: H := H(1)
  print h.z()
  var b: Lib.Base := h
  print b.z()
  print "fin"
end main
end Huer
"""
# V4 (17-sep) — el ABUELO en tres modulos: Lib (Base) -> Mid (Media extends Lib.Base, sellado contra v1)
# -> Abu (Nieta extends Mid.Media, sellado contra v1 + Mid). Cambia SOLO Lib: las dos clases abiertas se
# reconstruyen en cadena (campos, ranuras y el super.k() de cada eslabon).
MID = """module Mid
import Core
import Lib
public class Media extends Lib.Base
  var mm: integer
  public function Media(n: integer)
    super(n)
    this.mm := n + 50
  end Media
  public function k(): string
    return "media/" + super.k()
  end k
  public function getMm(): integer
    return this.mm
  end getMm
end Media
end Mid
"""
ABU = """module Abu
import Core
import Lib
import Mid
public class Nieta extends Mid.Media
  var qq: integer
  public function Nieta(n: integer)
    super(n)
    this.qq := n * 3
  end Nieta
  public function k(): string
    return "nieta/" + super.k() + "/" + this.qq
  end k
end Nieta
public function main()
  var x: Nieta := Nieta(2)
  print x.k()
  print x.getMm()
  print x.m(4)
  var b: Lib.Base := x
  print b.k()
  b.p := 9
  print b.p
  print x.suma(b)
  print "fin"
end main
end Abu
"""
# V6 (17-sep) — el GC con los bitmaps RECONSTRUIDOS: Bolsa (abierta) tiene dos campos de referencia propios detras
# de los del padre; 300 objetos vivos encadenados, basura entre medias y gc() forzado cada 100. Si el bitmap del hijo
# no siguiera al padre (campo nuevo delante, 33 campos, barajado), el GC recogeria un vivo (falla el handle) o
# marcaria un entero como referencia; la suma y el ultimo eslabon dicen si la cadena sobrevivio entera.
GC_SRC = """module Gc
import Core
import Lib
public class Bolsa extends Lib.Base
  var nombre: string
  var otra: Bolsa
  var cnt: integer
  public function Bolsa(n: integer, nombre: string)
    super(n)
    this.nombre := nombre
    this.cnt := n
  end Bolsa
  public function link(o: Bolsa)
    this.otra := o
  end link
  public function sig(): Bolsa
    return this.otra
  end sig
  public function peso(): integer
    return this.cnt
  end peso
  public function describe(): string
    return this.nombre + "/" + this.k() + "/" + this.cnt
  end describe
end Bolsa
public function main()
  var raiz: Bolsa := Bolsa(1, "raiz")
  var prev: Bolsa := raiz
  var i: integer := 2
  while i <= 300 do
    var b: Bolsa := Bolsa(i, "b" + i)
    prev.link(b)
    prev := b
    var basura: string := "x" + i + "y" + i
    if i mod 100 == 0 then
      gc()
    endif
    i := i + 1
  endwh
  gc()
  var p: Bolsa := raiz
  var suma: integer := 0
  var ultimo: string := ""
  var n: integer := 0
  while p != null do
    suma := suma + p.peso()
    ultimo := p.describe()
    n := n + 1
    p := p.sig()
  endwh
  print suma
  print ultimo
  print n
  print "fin"
end main
end Gc
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
  'UsaD':   "k1\nfin",
  'ExtD':   "nieta/k2\nnieta/k2\nfin",
  'Huer':   "1\n1\nfin",
  'Gc':     "45150\nb300/k300/300\n300\nfin",
  'Abu':    "nieta/media/k2/6\n52\n5\nnieta/media/k2/6\n9\n4\nfin",
}
def esperado(app, variante):
    s = ESPERADO[app]
    if variante == 'cuerpo': s = s.replace('k3', 'K3').replace('k5', 'K5').replace('k1', 'K1').replace('k2', 'K2')
    if variante == 'derived_k_propio' and app == 'ExtD': s = s.replace('nieta/k2', 'nieta/dk2')   # J-3: el super va al padre NUEVO
    if variante == 'f_firma' and FORMATO == 9: s = s   # (F3b: se rechazara; hoy se mide)
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

FORMATO = 9   # V11 (17-sep): MOD9 es el formato del compilador; --formato=8 da la foto «antes» (norma #500)
VMS = 'ambas'

def lib(work, variante, gen, extra=(), etiqueta=''):
    """La variante compilada (cache por dir). `extra`: flags mas del compilador (--barajar=, --relleno=);
    `etiqueta` los distingue en el nombre del dir (si no, la cache devolveria la lib sin flags)."""
    d = os.path.join(work, 'lib_%s%s_g%d' % (variante, etiqueta, gen))
    if os.path.isdir(d): return d
    os.makedirs(os.path.join(d, 'src'))
    io.open(os.path.join(d, 'src', 'Lib.bp'), 'w', encoding='utf-8', newline='\n').write(VARIANTES[variante][1](LIB_V1))
    io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "Lib", "version": %d, "dependencies": [] }\n' % gen)
    shutil.copy(core_para(work, FORMATO), d)
    rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', '--formato=%d' % FORMATO] + list(extra), d)
    if rc != 0 or not os.path.isfile(os.path.join(d, 'Lib.mod')):
        raise SystemExit('la variante %s%s (gen %d) no compila:\n%s%s' % (variante, etiqueta, gen, out, err))
    io.open(os.path.join(d, 'build.log'), 'w', encoding='utf-8').write(out + err)
    return d

def core_para(work, formato):
    """H1 P3 (20-sep): el Core.mod que ve una Lib. Desde que Object vive en Core.bp toda clase
    desciende de Core.Object, y en MOD8 el padre externo necesita el LAYOUT que solo publica la
    interfaz bpi 8: el Core versionado (MOD9) no vale para compilar MOD8. Se compila Core.bp como
    MOD8 una vez por trabajo (gen 7, la de la stdlib)."""
    if formato != 8: return CORE
    d = os.path.join(work, 'core8')
    if os.path.isfile(os.path.join(d, 'Core.mod')): return os.path.join(d, 'Core.mod')
    os.makedirs(os.path.join(d, 'src'), exist_ok=True)
    shutil.copy(os.path.join(ROOT, 'bpstdlib', 'Core.bp'), os.path.join(d, 'src', 'Core.bp'))
    io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "Core", "version": 7, "dependencies": [] }\n')
    rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', '--formato=8'], d)
    if rc != 0 or not os.path.isfile(os.path.join(d, 'Core.mod')):
        raise SystemExit('Core.bp no compila como MOD8:\n%s%s' % (out, err))
    return os.path.join(d, 'Core.mod')

def app(work, nombre, src, libdir, etiqueta):
    """Compila `nombre` contra los .mod de libdir; devuelve el dir con el .mod y los avisos."""
    d = os.path.join(work, 'app_%s_%s' % (nombre, etiqueta))
    if os.path.isdir(d): return d
    os.makedirs(d)
    for f in os.listdir(libdir):
        if f.endswith('.mod'): shutil.copy(os.path.join(libdir, f), d)
    io.open(os.path.join(d, nombre + '.bp'), 'w', encoding='utf-8', newline='\n').write(src)
    rc, out, err = sh(['java', '-jar', FE, nombre + '.bp', '--compile', '.', '--backend=mivm', '--formato=%d' % FORMATO], d)
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
            ('mas reciente+extiende', r'es mas reciente que .* extiende una clase suya'),
            ('no enlaza', r'ERROR DE LINK|no exporta|necesita |redefine |asume que ')]
def clasificar(rc, out):
    for nombre, rx in RECHAZOS:
        if re.search(rx, out): return nombre
    return 'corre' if rc == 0 else 'crash(rc=%d)' % rc

filas = []
def fila(bloque, etiqueta, d, mod, predicho, esperado_out=None):
    """predicho: 'corre' | uno de RECHAZOS[*][0]. esperado_out: stdout exacto si corre."""
    if VMS == 'java':   rj, oj = run_vm(JVM, d, mod + '.mod'); rc_, oc = rj, oj
    elif VMS == 'c':    rc_, oc = run_vm(CVM, d, mod + '.mod'); rj, oj = rc_, oc
    else:
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
MUEVE = {'met_medio', 'met_quitado', 'prop_mas', 'reorden', 'ev_privado', 'dos_mueven', 'dos_una_rompe'}
# Lo que HOY se sabe que el marcador NO ve (la cabeza `m` es un nombre pelado sin firma):
#   'firma' y 'enum_cabeza' dejan `Base#m#N` en su sitio con OTRA firma -> la prediccion
#   de la norma es «no enlaza», y lo que pase de verdad es lo que se mide.
# EL CRITERIO DE JAVA (formato 9): estructura cambiada -> corre; miembro usado quitado o con
# otra firma/retorno -> rechazo al enlazar (tambien una funcion de modulo: R4, `Lib.f()i`).
ROMPE_JAVA = {'met_quitado', 'firma', 'retorno', 'prop_tipo', 'f_firma', 'dos_una_rompe'}
def prediccion_java(variante, gen, app):
    if gen < 7: return 'mas antiguo'
    if variante == 'derived_deja_de_extender': return 'no enlaza' if app in ('UsaD', 'ExtD') else 'corre'   # A1: UsaD asume Derived<:Base; ExtD extiende Derived y usa Base
    if variante in ROMPE_JAVA:
        if variante == 'f_firma' and app != 'Usa': return 'corre'   # solo Usa llama a f()
        return 'no enlaza'
    return 'corre'

def prediccion_usa(variante, gen):
    if FORMATO == 9: return prediccion_java(variante, gen, 'Usa')
    if gen < 7: return 'mas antiguo'
    if gen == 7: return 'corre' if variante in ('igual', 'cuerpo') else 'otra interfaz'
    if variante in MUEVE or variante in ('firma', 'enum_cabeza', 'met_quitado', 'retorno', 'prop_tipo', 'f_firma'): return 'no enlaza'
    return 'corre'
def prediccion_usad(variante, gen):
    if FORMATO == 9: return prediccion_java(variante, gen, 'UsaD')
    return prediccion_usa(variante, gen)   # la norma MOD8 no ve la cadena: predice lo mismo que para Usa (se mide)
def prediccion_ext(variante, gen):
    if FORMATO == 9: return prediccion_java(variante, gen, 'Ext')
    if gen < 7: return 'mas antiguo'
    if gen == 7: return 'corre' if variante in ('igual', 'cuerpo') else 'otra interfaz'
    return 'mas reciente+extiende'

def main():
    global FORMATO, VMS
    solo = None; keep = False; work = None
    for a in sys.argv[1:]:
        if a.startswith('--solo='): solo = a[7:]
        elif a == '--keep': keep = True
        elif a.startswith('--work='): work = a[7:]
        elif a.startswith('--formato='): FORMATO = int(a[10:])
        elif a.startswith('--vms='): VMS = a[6:]
        else: raise SystemExit('argumento desconocido: ' + a)
    if solo not in (None, 'I', 'II', 'P', 'B', 'V', 'F'): raise SystemExit('--solo=' + solo + ': bloques I, II, P, B, V o F')
    for f in (FE, JVM, CVM, CORE):
        if not os.path.isfile(f): raise SystemExit('falta ' + f)
    work = work or tempfile.mkdtemp(prefix='mezcla_')
    print('trabajo en', work, '| formato', FORMATO, '| vms', VMS, '| criterio', 'Java (enlace tardio)' if FORMATO == 9 else 'norma #500')
    v1 = lib(work, 'igual', 7)
    usa1 = app(work, 'Usa', USA, v1, 'v1'); ext1 = app(work, 'Ext', EXT, v1, 'v1'); usad1 = app(work, 'UsaD', USA_D, v1, 'v1')
    extd1 = app(work, 'ExtD', EXT_D, v1, 'v1') if FORMATO == 9 else None   # J-3: super.k() por ref SUPER (solo MOD9)

    if solo in (None, 'I'):
        print('\n== BLOQUE I: MISMA VERSION (app compilada contra X, ejecutada contra X): tiene que correr siempre ==')
        for v in VARIANTES:
            d = lib(work, v, 7)
            for nombre, src in (('Usa', USA), ('Ext', EXT)):
                if v in ('met_quitado', 'retorno', 'prop_tipo', 'f_firma', 'dos_una_rompe'): continue   # las apps no compilan contra ellas: no son casos de I
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
                cd = caso_dir(work, 'II_%s_g%d' % (v, gen), d, [os.path.join(usa1, 'Usa.mod'), os.path.join(ext1, 'Ext.mod'), os.path.join(usad1, 'UsaD.mod')])
                fila('II', 'Usa v1 contra %s g%d' % (v, gen), cd, 'Usa', prediccion_usa(v, gen), esperado('Usa', v))
                fila('II', 'Ext v1 contra %s g%d' % (v, gen), cd, 'Ext', prediccion_ext(v, gen), esperado('Ext', v))
                if v in ('igual', 'derived_deja_de_extender') and gen == 8:   # A1: la asuncion Derived<:Base, verdadera y falsa
                    fila('II', 'UsaD v1 contra %s g%d' % (v, gen), cd, 'UsaD', prediccion_usad(v, gen), esperado('UsaD', v))
                if extd1 and v in ('igual', 'cuerpo', 'met_medio', 'reorden', 'derived_k_propio', 'derived_deja_de_extender') and gen == 8:
                    # J-3: super.k() sellado con k() en el ABUELO; con k() movido al padre, o el abuelo reordenado, tiene que seguir
                    shutil.copy(os.path.join(extd1, 'ExtD.mod'), cd)
                    fila('II', 'ExtD v1 contra %s g%d' % (v, gen), cd, 'ExtD', prediccion_java(v, gen, 'ExtD'), esperado('ExtD', v))

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
                    rc, out, err = sh(['java', '-jar', FE, 'Puente.bp', '--compile', '.', '--backend=mivm', '--formato=%d' % FORMATO], pd)
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

    if solo in (None, 'B') and FORMATO == 9:
        print('\n== BLOQUE B: EL GUARDIAN (apps selladas contra la semilla 0; Lib con --barajar=k / --relleno=3): mismo stdout siempre ==')
        lib0 = io.open(os.path.join(v1, 'Lib.mod'), 'rb').read()
        casos = [('--barajar=%d' % k,) for k in range(1, 8)] + [('--barajar=%d' % k, '--relleno=3') for k in range(1, 4)]
        for flags in casos:
            etq = '_'.join(f.lstrip('-').replace('=', '') for f in flags)
            d = lib(work, 'igual', 7, extra=flags, etiqueta='_' + etq)
            if io.open(os.path.join(d, 'Lib.mod'), 'rb').read() == lib0:
                raise SystemExit('Lib.mod con %s es IDENTICA a la de la semilla 0: el flag no ha hecho nada (jar rancio?)' % ' '.join(flags))
            gcb = app(work, 'Gc', GC_SRC, v1, 'v1')   # V6: los campos de referencia del hijo tras un padre barajado/relleno
            cd = caso_dir(work, 'B_' + etq, d, [os.path.join(usa1, 'Usa.mod'), os.path.join(ext1, 'Ext.mod'), os.path.join(usad1, 'UsaD.mod'), os.path.join(gcb, 'Gc.mod')])
            for nombre in ('Usa', 'Ext', 'UsaD', 'Gc'):
                fila('B', '%s v1 contra Lib %s' % (nombre, ' '.join(flags)), cd, nombre, 'corre', esperado(nombre, 'igual'))

    if solo in (None, 'V') and FORMATO == 9:
        print('\n== BLOQUE V (V4 de #523): override huerfano (E2), el abuelo en tres modulos, dos averias, MOD8 x MOD9 ==')
        # override huerfano: H redefine z(); con z() quitado de Base el cargador lo rechaza nombrandolo (E2), no lo cuela como metodo nuevo
        huer1 = app(work, 'Huer', HUER, v1, 'v1')
        for v in ('igual', 'met_medio', 'met_quitado_nousado'):
            d = lib(work, v, 8)
            cd = caso_dir(work, 'V_huer_%s' % v, d, [os.path.join(huer1, 'Huer.mod')])
            fila('V', 'Huer v1 contra %s g8' % v, cd, 'Huer', 'no enlaza' if v == 'met_quitado_nousado' else 'corre', esperado('Huer', v))
        # el abuelo: Mid sellado contra v1; Abu sellado contra v1 + Mid; en runtime cambia SOLO Lib
        mid1 = app(work, 'Mid', MID, v1, 'v1')
        abu1 = app(work, 'Abu', ABU, mid1, 'v1')   # mid1 lleva Lib.mod (v1) y Mid.mod
        for v in VARIANTES:
            d = lib(work, v, 8)
            cd = caso_dir(work, 'V_abu_%s' % v, d, [os.path.join(mid1, 'Mid.mod'), os.path.join(abu1, 'Abu.mod')])
            fila('V', 'Abu v1 (via Mid v1) contra %s g8' % v, cd, 'Abu', prediccion_java(v, 8, 'Abu'), esperado('Abu', v))
        # V6: el GC con los bitmaps reconstruidos (campo de referencia nuevo delante, 33 campos, y las de forma de siempre)
        gc1 = app(work, 'Gc', GC_SRC, v1, 'v1')
        for v in ('igual', 'campo_mas', 'campo_ref_mas', 'campos_33', 'campo_menos', 'met_medio', 'dos_mueven'):
            d = lib(work, v, 8)
            cd = caso_dir(work, 'V_gc_%s' % v, d, [os.path.join(gc1, 'Gc.mod')])
            fila('V', 'Gc v1 contra %s g8 (GC forzado)' % v, cd, 'Gc', 'corre', esperado('Gc', v))
        # MOD8 x MOD9 (medido el 17-sep). Con el dueno MOD8, la norma #500 (que sigue viva para MOD7/8) habla ANTES
        # que E9: misma gen -> «otra interfaz» (el CRC de la bpi 8 nunca es el de la bpi 9); gen mayor -> Ext cae por
        # la regla 3 («mas reciente y extiende») y Usa por el import 3.1 de su constructor con firma («no exporta
        # 'Lib.__cls_new_Base(i)' ... version vieja?»). E9 («'Lib' es MOD8 y 'Ext' necesita su tabla de clases:
        # recompila 'Lib'») solo puede hablar en el caso REALISTA: la app MOD9 sellada contra la propia Lib MOD8 (el
        # usuario recompila su app con el compilador nuevo y deja la libreria vieja). Consumidor MOD8 con dueno MOD9:
        # sus marcadores no existen («no exporta 'Lib.__cls_m_Base#k$'»). Nunca corre, nunca peta; mismo texto en las dos VMs.
        FORMATO = 8
        v1m8 = lib(work, 'igual', 7, etiqueta='_m8')
        usa8 = app(work, 'Usa', USA, v1m8, 'v1_m8'); ext8 = app(work, 'Ext', EXT, v1m8, 'v1_m8')
        lib8 = lib(work, 'igual', 8, etiqueta='_m8')
        FORMATO = 9
        lib9 = lib(work, 'igual', 8)
        usa9c8 = app(work, 'Usa', USA, v1m8, 'v1_m8_app9'); ext9c8 = app(work, 'Ext', EXT, v1m8, 'v1_m8_app9')   # MOD9 sellada contra la MOD8
        casos_m = (('app MOD9 (sellada MOD9) contra Lib MOD8 g7', v1m8, (usa1, ext1), {'Usa': 'otra interfaz', 'Ext': 'otra interfaz'}),
                   # H1 P3 (20-sep): una Lib MOD8 vive con un Core MOD8 (toda clase desciende de Core.Object y el
                   # padre externo pide el layout de bpi 8), y la app MOD9 esta sellada contra el Core MOD9: la
                   # primera dep que no casa es Core («otra interfaz»), antes de que E9 hable de Lib. Es lo real:
                   # una libreria vieja arrastra su Core viejo, y la app nueva lo rechaza con nombre.
                   ('app MOD9 (sellada MOD9) contra Lib MOD8 g8', lib8, (usa1, ext1), {'Usa': 'otra interfaz', 'Ext': 'otra interfaz'}),
                   ('app MOD9 sellada contra ESA Lib MOD8 g7', v1m8, (usa9c8, ext9c8), {'Usa': 'no enlaza', 'Ext': 'no enlaza'}),
                   ('app MOD8 contra Lib MOD9 g8', lib9, (usa8, ext8), {'Usa': 'no enlaza', 'Ext': 'no enlaza'}))
        for etq, libdir, mods, pred in casos_m:
            cd = caso_dir(work, 'V_' + etq.replace(' ', '_'), libdir, [os.path.join(mods[0], 'Usa.mod'), os.path.join(mods[1], 'Ext.mod')])
            for nombre in ('Usa', 'Ext'):
                fila('V', '%s %s' % (nombre, etq), cd, nombre, pred[nombre], esperado(nombre, 'igual'))

    if solo in (None, 'F') and FORMATO == 9:
        print('\n== BLOQUE F (V8 H7.3, #756): las interfaces — la app sellada contra ILib v1, contra cada variante (gen 8) ==')
        iv1 = ilib(work, 'igual', 7)
        iusa1 = app(work, 'IUsa', IUSA, iv1, 'iv1')
        for v in IVARIANTES:
            d = ilib(work, v, 8)
            cd = caso_dir(work, 'F_' + v, d, [os.path.join(iusa1, 'IUsa.mod')])
            fila('F', 'IUsa v1 contra ILib %s g8' % v, cd, 'IUsa', 'no enlaza' if v in IROMPE else 'corre', IESPERADO)

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
