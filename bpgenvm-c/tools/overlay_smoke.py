#!/usr/bin/env python3
"""overlay_smoke.py — V8 H9 (#786): un programa que carga a otro, lo ejecuta y lo descarga (Overlays.bp).

Cada caso, un programa que lanza a otro, en las dos VMs del PC y en el micro simulado: lo que imprime tiene que ser lo
esperado, y lo mismo byte a byte en los tres. Lo que el programa dice empieza por «» »; lo demás es de la VM.

   1. el ejemplo de samples/overlay: A carga a B dos veces, con M1 compartido y M4 suyo;
   2. todo lo que puede acabar mal en B, con su error, su clase y su traza: una división por cero, un null, un throw, una
      clase de excepción suya, la pila desbordada y sin memoria;
   3. B con clases abiertas sobre las de Core y sobre las de un módulo que trae él, e interfaces suyas y de A;
   4. los objetos de A sobreviven a moverse: cadenas, objetos y `gc()` antes y después;
   5. cincuenta vueltas, con el heap igual en cada una, y sus globales de nuevo a cero;
   6. un objeto de B en una global de un módulo que se queda: unload lo nombra, y una vez suelto se puede descargar;
   7. cada condición de load, tryExe y unload, con su texto;
   8. no existe, no enlaza, se llama como uno cargado y no cabe: el programa sigue como estaba;
   9. lo que B deja en un módulo que se queda sigue vivo cuando otro programa ocupa su sitio;
  10. dos clases en la misma dirección, una con destructor y la otra sin él;
  11. los hilos de B: uno que acaba antes que su Main, y otro que sigue y no deja descargar hasta que acaba;
  12. la traza del error, traducida con el .dbg del programa cargado;
  13. dentro de un programa cargado no valen ni otro load ni Heap.compact: los dos se niegan solos;
  14. un programa que viene en un pack, con un módulo suyo dentro;
  15. el mismo módulo, traído por dos programas, en dos direcciones;
  16. y que todo esto sabe fallar: cada pieza rota a propósito (BPVM_SABOTAJE_OVERLAY) la caza algún caso.

    python tools/overlay_smoke.py            (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import os, re, shutil, subprocess, sys, tempfile

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import SIM, STDLIB_PACK, STDLIB_DIR, JAR   # noqa: E402 — el mismo simulador y el mismo compilador
import run_smoke                                          # noqa: E402 — su cable y su arranque
from traza_dbg import traduce                             # noqa: E402

run_smoke.PORT = 5133
VM_C = os.path.abspath(os.path.join("build", "bpgenvm-c.exe" if os.name == "nt" else "bpgenvm-c"))
VM_JAVA = os.path.abspath(os.path.join("..", "miVM", "target", "bpgenvm-1.0.jar"))
EJEMPLO = os.path.abspath(os.path.join("..", "samples", "overlay"))
fails = 0


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond:
        fails += 1


COMUN = """module OvComun
  public class Caja
    public property valor: integer
    public property datos: integer[]
    public function Caja(v: integer)
      this.valor := v
    end Caja
  end Caja

  var nombre: string := "sin nombre"
  var guardado: Object

  public function setNombre(n: string)
    nombre := n
  end setNombre
  public function getNombre(): string
    return nombre
  end getNombre
  public function guarda(o: Object)
    guardado := o
  end guarda
end OvComun
"""

HOLA = """module OvHola
  import OvComun
  var veces: integer := 0

  function Main()
    veces := veces + 1
    print "» hola " + intToString(veces)
  end Main
end OvHola
"""

FALLA = """module OvFalla
  import OvComun

  public class MiError extends Exception
    public function MiError(m: string)
      super(m)
    end MiError
  end MiError

  function divide(a: integer, b: integer): integer
    return a / b
  end divide

  function hondo(n: integer): integer
    return hondo(n + 1) + 1
  end hondo

  function Main(arg: string)
    if arg == "cero" then
      print divide(1, 0)
    endif
    if arg == "nulo" then
      var c: OvComun.Caja := OvComun.Caja(1)
      c := null
      print c.valor
    endif
    if arg == "throw" then
      throw RuntimeError("lanzado por B")
    endif
    if arg == "propia" then
      throw MiError("una clase de B")
    endif
    if arg == "pila" then
      print hondo(0)
    endif
    if arg == "memoria" then
      var s: string := "xxxxxxxx"
      while true do
        s := s + s
      endwh
    endif
    print "» bien con '" + arg + "'"
  end Main
end OvFalla
"""

ERRORES = """module OvErrores
  import Overlays

  function caso(i: integer): string
    if i == 0 then
      return "cero"
    endif
    if i == 1 then
      return "nulo"
    endif
    if i == 2 then
      return "throw"
    endif
    if i == 3 then
      return "propia"
    endif
    if i == 4 then
      return "pila"
    endif
    if i == 5 then
      return "memoria"
    endif
    return "nada"
  end caso

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    var i: integer := 0
    while i < 7 do
      ov.load("OvFalla.mod")
      if ov.tryExe(caso(i)) then
        print "» " + caso(i) + ": bien"
      else
        print "» " + caso(i) + ": " + ov.getError() + " [" + ov.getErrorClass() + "] " + ov.getTrace()
      endif
      ov.unload()
      i := i + 1
    endwh
    print "» A sigue"
  end Main
end OvErrores
"""

CLASES_B = """module OvClases
  import Collections
  import OvComun
  import OvInterfaz

  public interface Propia
    function doble(): integer
  end Propia

  public class Lista extends List
    public function primero(): string
      return string(this.get(0))
    end primero
  end Lista

  public class Mapa extends Collections.Map
    public function dame(): string
      return string(this.get("clave"))
    end dame
  end Mapa

  public class Fallo extends RuntimeError
    public function Fallo(m: string)
      super(m)
    end Fallo
  end Fallo

  public class Mia implements OvInterfaz.Saludo, Propia
    @implement{ Saludo }
    public function saluda(): string
      return "hola desde B"
    end saluda
    @implement{ Propia }
    public function doble(): integer
      return 42
    end doble
  end Mia

  function Main()
    var l: Lista := Lista()
    l.add("uno")
    print "» " + l.primero() + " " + intToString(l.length())
    var mapa: Mapa := Mapa()
    mapa.put("clave", "de un mapa")
    print "» " + mapa.dame()
    var m: Mia := Mia()
    var s: OvInterfaz.Saludo := m
    var p: Propia := m
    print "» " + s.saluda() + " " + intToString(p.doble())
    print "» " + OvInterfaz.dile(m)
    try
      throw Fallo("fallo atrapado en B")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
  end Main
end OvClases
"""

INTERFAZ = """module OvInterfaz
  public interface Saludo
    function saluda(): string
  end Saludo

  public class DeA implements Saludo
    @implement{ Saludo }
    public function saluda(): string
      return "hola desde A"
    end saluda
  end DeA

  public function dile(s: Saludo): string
    return "dice: " + s.saluda()
  end dile
end OvInterfaz
"""

CLASES_A = """module OvClasesA
  import Overlays
  import OvInterfaz

  function Main()
    var a: OvInterfaz.DeA := OvInterfaz.DeA()
    print "» " + OvInterfaz.dile(a)
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvClases.mod")
    print "» " + OvInterfaz.dile(a)
    if not ov.tryExe("") then
      print "» mal: " + ov.getError()
    endif
    ov.unload()
    var l: List := List()
    var i: integer := 0
    while i < 2000 do
      l.add("relleno " + intToString(i))
      i := i + 1
    endwh
    print "» " + OvInterfaz.dile(a)
  end Main
end OvClasesA
"""

OBJETOS = """module OvObjetos
  import Heap
  import Overlays
  import OvComun

  function comprueba(l: List): integer
    var mal: integer := 0
    var i: integer := 0
    while i < l.length() do
      var c: OvComun.Caja := OvComun.Caja(l.get(i))
      if c.valor != i then
        mal := mal + 1
      endif
      if c.datos[1] != i * 2 then
        mal := mal + 1
      endif
      i := i + 1
    endwh
    return mal
  end comprueba

  function Main()
    var l: List := List()
    var t: List := List()
    var i: integer := 0
    while i < 300 do
      var c: OvComun.Caja := OvComun.Caja(i)
      c.datos := newIntArray(3)
      c.datos[1] := i * 2
      l.add(c)
      t.add("cadena " + intToString(i) + " ñandú")
      var basura: string := "basura " + intToString(i)
      i := i + 1
    endwh
    var ov: Overlays.Overlay := Overlays.Overlay()
    print "» antes " + intToString(comprueba(l))
    ov.load("OvHola.mod")
    print "» cargado " + intToString(comprueba(l))
    gc()
    print "» tras gc " + intToString(comprueba(l))
    ov.tryExe("")
    ov.unload()
    print "» descargado " + intToString(comprueba(l))
    gc()
    Heap.compact()
    print "» compactado " + intToString(comprueba(l))
    i := 0
    var mal: integer := 0
    while i < 300 do
      if string(t.get(i)) != "cadena " + intToString(i) + " ñandú" then
        mal := mal + 1
      endif
      i := i + 1
    endwh
    print "» cadenas " + intToString(mal)
  end Main
end OvObjetos
"""

VUELTAS = """module OvVueltas
  import Heap
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    var referencia: integer := 0
    var distintas: integer := 0
    var i: integer := 0
    while i < 50 do
      ov.load("OvHola.mod")
      ov.tryExe("")
      ov.unload()
      gc()
      Heap.compact()
      if i == 1 then
        referencia := Heap.getFree()
      endif
      if (i > 1) and (Heap.getFree() != referencia) then
        distintas := distintas + 1
      endif
      i := i + 1
    endwh
    print "» distintas " + intToString(distintas)
  end Main
end OvVueltas
"""

QUEDA_B = """module OvQueda
  import OvComun
  public class Suya
  end Suya

  function Main(arg: string)
    OvComun.guarda(Suya())
  end Main
end OvQueda
"""

QUEDA_A = """module OvQuedaA
  import Overlays
  import OvComun

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvQueda.mod")
    ov.tryExe("objeto")
    try
      ov.unload()
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.tryExe("")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    OvComun.guarda(null)
    ov.unload()
    print "» descargado"
  end Main
end OvQuedaA
"""

CONDICIONES = """module OvCondiciones
  import IO
  import Overlays

  public class Dormilon
    public function duerme()
      sleep(100)
    end duerme
  end Dormilon

  function carga(ov: Overlays.Overlay)
    ov.load("OvHola.mod")
  end carga

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    var otro: Overlays.Overlay := Overlays.Overlay()
    try
      carga(ov)
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.tryExe("")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.unload()
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    var d: Dormilon := Dormilon()
    var t: Thread := Thread(d::duerme())
    t.start()
    try
      ov.load("OvHola.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    t.join()
    writeFile("ov_condicion.txt", "hola")
    var f: IO.File := IO.File("ov_condicion.txt", "r")
    try
      ov.load("OvHola.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    f.close()
    ov.load("OvHola.mod")
    try
      otro.load("OvHola.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    var largo: string := "x"
    var i: integer := 0
    while i < 8 do
      largo := largo + largo
      i := i + 1
    endwh
    if not ov.tryExe(largo) then
      print "» " + ov.getError()
    endif
    ov.unload()
    print "» fin"
  end Main
end OvCondiciones
"""

CARGA_FALLA = """module OvCargaFalla
  import Heap
  import Overlays
  import OvComun

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    OvComun.setNombre("A")
    try
      ov.load("NoExiste.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.load("OvSinDep.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.load("OvNoEnlaza.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      ov.load("OvComun.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    var l: List := List()
    while Heap.getFree() > 60000 do
      var c: OvComun.Caja := OvComun.Caja(0)
      c.datos := newIntArray(1000)
      l.add(c)
    endwh
    try
      ov.load("OvGrande.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    l := null
    gc()
    print "» sigue " + OvComun.getNombre()
    ov.load("OvHola.mod")
    ov.tryExe("")
    ov.unload()
    print "» fin"
  end Main
end OvCargaFalla
"""

DEJA_1 = """module OvDeja
  import OvComun

  function Main()
    OvComun.setNombre("lo dejó el primero, con acentos: ñandú")
  end Main
end OvDeja
"""

DEJA_2 = """module OvPisa
  import OvComun

  const OTRO: string := "un literal del segundo, en el mismo sitio que el del primero"

  function Main()
    print "» " + OvComun.getNombre()
    var s: string := OTRO
    print "» " + s
  end Main
end OvPisa
"""

DEJA_A = """module OvDejaA
  import Overlays
  import OvComun

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvDeja.mod")
    ov.tryExe("")
    ov.unload()
    print "» " + OvComun.getNombre()
    ov.load("OvPisa.mod")
    ov.tryExe("")
    ov.unload()
    gc()
    print "» " + OvComun.getNombre()
    var i: integer := 0
    var n: integer := 0
    while i < strlen(OvComun.getNombre()) do
      if charAt(OvComun.getNombre(), i) == "ñ" then
        n := n + 1
      endif
      i := i + 1
    endwh
    print "» eñes " + intToString(n)
  end Main
end OvDejaA
"""

# Primero la que no tiene destructor: si la marca por clase sobreviviera a la descarga y las dos cayeran en la misma
# dirección, la segunda se quedaría sin el suyo. (Que caigan en la misma no se puede forzar desde aquí: una clase abierta
# va a la arena, detrás del código, y ~X lleva una referencia y una llamada que otro método no lleva.)
DTOR_1 = """module OvDtorUno
  public class X
    public property n: integer
  end X
  var x: X

  function nunca()
    print "» destruyo X"
  end nunca

  function Main()
    x := X()
    print "» sin dtor X"
  end Main
end OvDtorUno
"""

DTOR_2 = """module OvDtorDos
  public class X
    public property n: integer
    public function ~X()
      print "» destruyo X"
    end ~X
  end X
  var x: X

  function nunca()
    print "» sin dtor X"
  end nunca

  function Main()
    x := X()
  end Main
end OvDtorDos
"""

DTOR_A = """module OvDtorA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvDtorUno.mod")
    ov.tryExe("")
    ov.unload()
    ov.load("OvDtorDos.mod")
    ov.tryExe("")
    ov.unload()
    print "» fin"
  end Main
end OvDtorA
"""

HILOS_B = """module OvHilos
  public class Tarea
    var ms: integer
    public function Tarea(ms: integer)
      this.ms := ms
    end Tarea
    public function corre()
      sleep(this.ms)
      print "» el hilo acaba"
    end corre
  end Tarea

  function Main(arg: string)
    var t: Thread := Thread(Tarea(1)::corre())
    if arg == "sigue" then
      t := Thread(Tarea(200)::corre())
    endif
    t.start()
    if arg != "sigue" then
      t.join()
    endif
    print "» B acaba"
  end Main
end OvHilos
"""

HILOS_A = """module OvHilosA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvHilos.mod")
    ov.tryExe("espera")
    ov.unload()
    ov.load("OvHilos.mod")
    ov.tryExe("sigue")
    try
      ov.unload()
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    sleep(400)
    ov.unload()
    print "» fin"
  end Main
end OvHilosA
"""

TRAZA_B = """module OvTraza
  function divide(a: integer): integer
    return a / 0                       // @1
  end divide

  function Main()
    print divide(3)                    // @2
  end Main
end OvTraza
"""

TRAZA_A = """module OvTrazaA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvTraza.mod")
    ov.tryExe("")
    print "» traza: " + ov.getTrace()
    ov.unload()
  end Main
end OvTrazaA
"""

ANIDA_B = """module OvAnida
  import Heap
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    try
      ov.load("OvHola.mod")
    catch e: RuntimeError
      print "» " + e.msg
    endtry
    try
      Heap.compact()
    catch e: RuntimeError
      print "» " + e.msg
    endtry
  end Main
end OvAnida
"""

ANIDA_A = """module OvAnidaA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvAnida.mod")
    ov.tryExe("")
    ov.unload()
    print "» fin"
  end Main
end OvAnidaA
"""

PACK_LIB = """module OvLibPack
  public function dime(): string
    return "un modulo del pack"
  end dime
end OvLibPack
"""

PACK_MAIN = """module OvEnPack
  import OvLibPack

  function Main()
    print "» " + OvLibPack.dime()
  end Main
end OvEnPack
"""

PACK_BUILD = """{ "sourceDir": ".", "outDir": "out", "main": "OvEnPack", "dependencies": [], "out": "pack",
  "pack": { "name": "OvPack", "version": "1.0" } }
"""

PACK_A = """module OvPackA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    var i: integer := 0
    while i < 2 do
      ov.load("OvPack.pack")
      if not ov.tryExe("") then
        print "» mal: " + ov.getError()
      endif
      ov.unload()
      i := i + 1
    endwh
    print "» fin"
  end Main
end OvPackA
"""

OTRO_LIB = """module OvLib
  public function dime(): string
    return "soy OvLib"
  end dime
end OvLib
"""

OTRO_1 = """module OvProgUno
  import OvLib

  function Main()
    print "» uno: " + OvLib.dime()
  end Main
end OvProgUno
"""

OTRO_2 = """module OvProgDos
  import OvLib

  const RELLENO: string := "un literal largo, para que OvLib caiga en otro sitio: 0123456789012345678901234567890123456789"

  function Main()
    var s: string := RELLENO
    print "» dos: " + OvLib.dime() + " " + intToString(strlen(s))
  end Main
end OvProgDos
"""

OTRO_A = """module OvOtroA
  import Overlays

  function Main()
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load("OvProgUno.mod")
    ov.tryExe("")
    ov.unload()
    ov.load("OvProgDos.mod")
    if not ov.tryExe("") then
      print "» mal: " + ov.getError()
    endif
    ov.unload()
    print "» fin"
  end Main
end OvOtroA
"""

# Lo que hace falta para que falle la carga: un módulo que falta, y uno que espera de OvComun lo que no tiene.
SIN_DEP = """module OvSinDep
  import OvQueFalta
  function Main()
    print OvQueFalta.uno()
  end Main
end OvSinDep
"""
QUE_FALTA = """module OvQueFalta
  public function uno(): integer
    return 1
  end uno
end OvQueFalta
"""
NO_ENLAZA = """module OvNoEnlaza
  import OvComun
  function Main()
    print OvComun.soloEnLaOtra()
  end Main
end OvNoEnlaza
"""
COMUN_MAS = COMUN.replace("end OvComun", """  public function soloEnLaOtra(): integer
    return 1
  end soloEnLaOtra
end OvComun""")
GRANDE = "module OvGrande\n  function Main()\n    print \"" + "g" * 30000 + "\"\n    print \"" + "h" * 30000 + "\"\n  end Main\nend OvGrande\n"

ESPERADO = {
    "ProgramaA": None,   # el suyo, abajo
    "OvErrores": [
        None, None, None, None, None, None,   # los seis errores, abajo
        "» bien con 'nada'", "» nada: bien", "» A sigue"],
}


def compila_todo(tmp):
    """(nombre, fuente), en orden: lo que se importa, antes."""
    fuentes = [("OvComun", COMUN_MAS), ("OvInterfaz", INTERFAZ), ("OvQueFalta", QUE_FALTA), ("OvSinDep", SIN_DEP),
               ("OvNoEnlaza", NO_ENLAZA), ("OvGrande", GRANDE)]
    mods = {}
    for n, f in fuentes:
        mods[n] = run_smoke.compila(tmp, n, f)
    # OvSinDep y OvNoEnlaza se compilan contra lo que luego no estará
    os.remove(os.path.join(tmp, "OvQueFalta.mod"))
    mods.pop("OvQueFalta")
    mods["OvComun"] = run_smoke.compila(tmp, "OvComun", COMUN)
    for n, f in (("OvHola", HOLA), ("OvFalla", FALLA), ("OvErrores", ERRORES), ("OvClases", CLASES_B),
                 ("OvClasesA", CLASES_A), ("OvObjetos", OBJETOS), ("OvVueltas", VUELTAS), ("OvQueda", QUEDA_B),
                 ("OvQuedaA", QUEDA_A), ("OvCondiciones", CONDICIONES), ("OvCargaFalla", CARGA_FALLA), ("OvDeja", DEJA_1),
                 ("OvPisa", DEJA_2), ("OvDejaA", DEJA_A), ("OvDtorUno", DTOR_1), ("OvDtorDos", DTOR_2), ("OvDtorA", DTOR_A),
                 ("OvHilos", HILOS_B), ("OvHilosA", HILOS_A), ("OvTraza", TRAZA_B), ("OvTrazaA", TRAZA_A),
                 ("OvAnida", ANIDA_B), ("OvAnidaA", ANIDA_A), ("OvPackA", PACK_A), ("OvLib", OTRO_LIB),
                 ("OvProgUno", OTRO_1), ("OvProgDos", OTRO_2), ("OvOtroA", OTRO_A)):
        mods[n] = run_smoke.compila(tmp, n, f)
    for n in ("M1", "M2", "M3", "M4", "ProgramaB", "ProgramaA"):
        with open(os.path.join(EJEMPLO, n + ".bp"), encoding="utf-8") as f:
            mods[n] = run_smoke.compila(tmp, n, f.read())
    return mods


def compila_pack(tmp):
    """El pack ejecutable del caso 14, junto a los demás; su ruta, o None."""
    d = os.path.join(tmp, "p")
    os.makedirs(d)
    for n, f in (("OvLibPack.bp", PACK_LIB), ("OvEnPack.bp", PACK_MAIN), ("p.bpbuild", PACK_BUILD)):
        with open(os.path.join(d, n), "w", encoding="utf-8") as g:
            g.write(f)
    subprocess.run(["java", "-jar", JAR, "--project", "p.bpbuild", "--backend=mivm", "--stdlibDir=" + STDLIB_DIR], cwd=d,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    fuera = os.path.join(d, "out")
    packs = [f for f in os.listdir(fuera) if f.endswith(".pack")] if os.path.isdir(fuera) else []
    if not packs:
        return None
    destino = os.path.join(tmp, "OvPack.pack")
    shutil.copy(os.path.join(fuera, packs[0]), destino)
    return destino


def lineas(salida):
    return [l for l in salida.replace("\r\n", "\n").split("\n") if l.startswith("» ") or l.startswith("A: ")
            or l.startswith("B: ") or l.startswith("M4: ")]


def vm_c(nombre, cwd, sabotaje=0):
    env = dict(os.environ)
    if sabotaje:
        env["BPVM_SABOTAJE_OVERLAY"] = str(sabotaje)
    r = subprocess.run([VM_C, nombre + ".mod"], cwd=cwd, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=120, env=env)
    return lineas(r.stdout), r.returncode


def vm_java(nombre, cwd):
    r = subprocess.run(["java", "-jar", VM_JAVA, nombre + ".mod"], cwd=cwd, capture_output=True, text=True,
                       encoding="utf-8", errors="replace", timeout=120)
    return lineas(r.stdout), r.returncode


def caso(num, titulo, nombre, tmp, sim, quiero):
    """El programa en los tres sitios; `quiero`, las líneas, o una función que las mira."""
    c, rc_c = vm_c(nombre, tmp)
    j, rc_j = vm_java(nombre, tmp)
    s = sim(nombre)
    if c != j or rc_c != rc_j:
        print("       VM-C (%s): %s" % (rc_c, c[:12]))
        print("       Java (%s): %s" % (rc_j, j[:12]))
    if s is not None and s != c:
        print("       sim: %s" % s[:12])
    bien = quiero(c) if callable(quiero) else c == quiero
    if not bien:
        print("       dice: %s" % c)
    check(bien and c == j and rc_c == 0 and (s is None or s == c), "%d. %s" % (num, titulo))
    return c


def main():
    if not os.path.exists(JAR) or shutil.which("java") is None or not os.path.exists(STDLIB_PACK):
        print("FAIL: hacen falta el compilador (%s) y la librería de packs (%s)" % (JAR, STDLIB_PACK))
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="overlay_smoke_")
    print("=== overlay_smoke (H9, #786): un programa que carga a otro ===")
    for f in os.listdir(STDLIB_DIR):
        if f.endswith(".mod"):
            shutil.copy(os.path.join(STDLIB_DIR, f), tmp)
    mods = compila_todo(tmp)
    pack = compila_pack(tmp)
    for n, m in mods.items():
        if m is None:
            check(False, "compila %s" % n)
    check(pack is not None, "compila el pack")
    if None in mods.values() or pack is None:
        print("[status=FAIL: %d]" % fails)
        return 1
    proc, sock, c = run_smoke.arranca(os.path.join(tmp, "sim.flash"))
    try:
        for n, m in mods.items():
            with open(m, "rb") as f:
                check(c.call_bulk("PUT", f.read(), path="/app/%s.mod" % n).get("type") == "PUT_REPLY", "PUT de %s" % n)
        with open(pack, "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/OvPack.pack").get("type") == "PUT_REPLY", "PUT del pack")

        def sim(nombre):
            r, tramas, ex = c.run("/app/%s.mod" % nombre)
            if not ex or ex.get("status") != "OK":
                print("       sim %s: %s" % (nombre, ex))
                return []
            return lineas(b"".join(tramas).decode("utf-8", "replace"))

        caso(1, "A carga a B dos veces, con M1 compartido y M4 suyo", "ProgramaA", tmp, sim, [
            "A: un punto de A (3, 7)", "A: primera vez", "B: mi argumento es 'hola'", "B: M1 se llama 'el nombre que puso A'",
            "B: el sensor 'sonda' lee 21", "B: acabo bien", "A: B acabó bien", "M4: destruyo el sensor 'sonda'",
            "A: M1 se llama 'el nombre que puso B', y B ya no está", "A: segunda vez", "B: mi argumento es 'falla'",
            "B: M1 se llama 'el nombre que puso B'", "B: el sensor 'sonda' lee 21", "A: B falló: la sonda no responde",
            "A: la clase del error es M4.ErrorDeSensor", "M4: destruyo el sensor 'sonda'", "A: un punto de A (3, 7)",
            "A: M1 ha recibido 3 llamadas"])

        def errores(l):
            d = {x.split(":")[0][2:]: x for x in l}
            return (len(l) == 9 and "División por cero" in d.get("cero", "") and "[Core.RuntimeError] OvFalla+" in d["cero"]
                    and "referencia nula" in d.get("nulo", "") and "lanzado por B [Core.RuntimeError]" in d.get("throw", "")
                    and "una clase de B [OvFalla.MiError]" in d.get("propia", "") and "desbordamiento de pila" in d.get("pila", "")
                    and "No space in heap" in d.get("memoria", "") and "» A sigue" in l)
        caso(2, "todo lo que puede acabar mal en B, con su error, su clase y su traza", "OvErrores", tmp, sim, errores)

        caso(3, "clases abiertas sobre Core y sobre un módulo que trae B, e interfaces suyas y de A", "OvClasesA", tmp, sim, [
            "» dice: hola desde A", "» dice: hola desde A", "» uno 1", "» de un mapa", "» hola desde B 42", "» dice: hola desde B",
            "» fallo atrapado en B", "» dice: hola desde A"])

        caso(4, "los objetos de A sobreviven a moverse", "OvObjetos", tmp, sim, [
            "» antes 0", "» cargado 0", "» tras gc 0", "» hola 1", "» descargado 0", "» compactado 0", "» cadenas 0"])

        caso(5, "cincuenta vueltas: el heap igual, y sus globales de nuevo a cero", "OvVueltas", tmp, sim,
             ["» hola 1"] * 50 + ["» distintas 0"])

        caso(6, "lo que queda de B en un módulo que se queda: unload lo nombra", "OvQuedaA", tmp, sim, [
            "» Overlay.unload: sigue vivo un objeto de OvQueda.Suya", "» Overlay.tryExe: la descarga ya ha empezado",
            "» descargado"])

        caso(7, "cada condición, con su texto", "OvCondiciones", tmp, sim, [
            "» Overlay.load: sólo se puede llamar desde Main", "» Overlay.tryExe: no hay nada cargado",
            "» Overlay.unload: no hay nada cargado", "» Overlay.load: no se puede cargar con otros hilos vivos",
            "» Overlay.load: hay ficheros abiertos", "» Overlay.load: ya hay un overlay cargado",
            "» el argumento no cabe: 256 bytes (el tope es 127)", "» fin"])

        def carga(l):
            return (len(l) == 8 and "Overlay.load: no encuentro 'NoExiste.mod'" in l[0]
                    and l[1] == "» Overlay.load: falta el modulo 'OvQueFalta'"
                    and "soloEnLaOtra" in l[2] and "OvNoEnlaza" in l[2]
                    and l[3] == "» Overlay.load: ya hay un modulo 'OvComun' cargado"
                    and l[4] == "» Overlay.load: no cabe en la memoria"
                    and l[5:] == ["» sigue A", "» hola 1", "» fin"])
        caso(8, "no existe, no enlaza, se llama como uno cargado y no cabe: A sigue como estaba", "OvCargaFalla", tmp, sim, carga)

        caso(9, "lo que B deja sigue vivo cuando otro programa ocupa su sitio", "OvDejaA", tmp, sim, [
            "» lo dejó el primero, con acentos: ñandú", "» lo dejó el primero, con acentos: ñandú",
            "» un literal del segundo, en el mismo sitio que el del primero", "» lo dejó el primero, con acentos: ñandú",
            "» eñes 1"])

        caso(10, "dos clases en la misma dirección, una con destructor y otra sin él", "OvDtorA", tmp, sim, [
            "» sin dtor X", "» destruyo X", "» fin"])

        caso(11, "los hilos de B: uno acaba antes, y otro no deja descargar hasta que acaba", "OvHilosA", tmp, sim, [
            "» el hilo acaba", "» B acaba", "» B acaba", "» Overlay.unload: no se puede descargar con otros hilos vivos",
            "» el hilo acaba", "» fin"])

        l = caso(12, "la traza del error sale", "OvTrazaA", tmp, sim, lambda x: len(x) == 1 and "OvTraza+" in x[0])
        t = l[0].split(": ", 1)[1] if l else ""
        check(traduce(t, [tmp])[:2] == ["OvTraza.bp:3", "OvTraza.bp:7"], "12b. y el .dbg del programa cargado le da sus líneas: %s" % traduce(t, [tmp]))

        caso(13, "dentro de un programa cargado no valen ni otro load ni Heap.compact", "OvAnidaA", tmp, sim, [
            "» Overlay.load: sólo se puede llamar desde Main", "» Heap.compact: sólo se puede llamar desde Main", "» fin"])

        caso(14, "un programa que viene en un pack, dos veces", "OvPackA", tmp, sim, ["» un modulo del pack"] * 2 + ["» fin"])

        caso(15, "el mismo módulo, traído por dos programas, en dos direcciones", "OvOtroA", tmp, sim, [
            "» uno: soy OvLib", "» dos: soy OvLib 94", "» fin"])

        # Cada pieza rota, y un caso que la caza: la salida cambia, o el programa acaba mal.
        for n, pieza, prog in ((1, "un literal no va al heap", "OvDejaA"), (2, "los handles no siguen a los objetos", "OvObjetos"),
                               (3, "la tabla de interfaces no vuelve", "OvClasesA"),
                               (5, "sus globales siguen siendo raíces", "OvDtorA"), (6, "los símbolos no se van", "OvOtroA"),
                               (7, "rehacer el enlace no acaba donde acabó", "ProgramaA")):
            bien = vm_c(prog, tmp)
            try:
                mal = vm_c(prog, tmp, n)
            except subprocess.TimeoutExpired:
                mal = None
            check(mal != bien, "16.%d. sabotaje: %s" % (n, pieza))
    finally:
        c.close() if hasattr(c, "close") else None
        sock.close()
        proc.terminate()
        proc.wait()
    shutil.rmtree(tmp, ignore_errors=True)
    print("[status=%s]" % ("OK" if fails == 0 else "FAIL: %d" % fails))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
