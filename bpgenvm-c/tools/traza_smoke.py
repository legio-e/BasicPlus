#!/usr/bin/env python3
"""traza_smoke.py — V8 #582: DÓNDE falló un programa, por el cable.

Un error que nadie atrapa dice su cadena de llamadas en el `stackTrace` del EXITED, y el `.dbg` la convierte en líneas.
Aquí, por el micro simulado (el RUN común de las placas), contra las líneas marcadas en el fuente con `// @`:

  1. un error de la VM (división por cero), tres llamadas por debajo, entre dos módulos;
  2. un `throw` del programa, con su clase;
  3. con más de ocho llamadas: las ocho de dentro;
  4. un error en un hilo: su cadena, y el programa sigue y acaba con RUNTIME_ERROR;
  5. un error atrapado y un programa que acaba bien: sin `stackTrace`;
  6. un KILL: sin `stackTrace`;
  7. las dos VMs y el sim dicen lo mismo, byte a byte (`=== traza: … ===` por stderr, y el EXITED);
  8. el error nace en un módulo de la stdlib: en el sim corre desde el pack, sin copiarse a la RAM, y en el PC desde su
     .mod; la posición tiene que ser la misma, y el `.dbg` de la stdlib darle su línea.

    python tools/traza_smoke.py            (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import os, re, shutil, subprocess, sys, tempfile

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import SIM, STDLIB_PACK, STDLIB_DIR, JAR   # noqa: E402 — el mismo simulador y el mismo compilador
import run_smoke                                          # noqa: E402 — su cable y su arranque
from traza_dbg import traduce                             # noqa: E402

run_smoke.PORT = 5131
VM_C = os.path.join("build", "bpgenvm-c.exe" if os.name == "nt" else "bpgenvm-c")
VM_JAVA = os.path.join("..", "miVM", "target", "bpgenvm-1.0.jar")
fails = 0


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond:
        fails += 1


LIB = """module TrazaLib
  public function divide(a: integer, b: integer): integer
    return a / b                       // @1
  end divide
end TrazaLib
"""

UNO = """module TrazaUno
  import TrazaLib

  function intermedia(x: integer): integer
    var r: integer := TrazaLib.divide(x, 0)   // @2
    return r
  end intermedia

  function Main()
    print "antes"
    print intermedia(3)                // @3
    print "no llega"
  end Main
end TrazaUno
"""

DOS = """module TrazaDos
  public class MiError extends Exception
    public function MiError(m: string)
      super(m)
    end MiError
  end MiError

  function lanza()
    throw MiError("a proposito")       // @1
  end lanza

  function Main()
    lanza()                            // @2
  end Main
end TrazaDos
"""

TRES = """module TrazaTres
  function baja(n: integer): integer
    if n == 0 then
      return 1 / n                     // @1
    endif
    return baja(n - 1) + 1             // @2
  end baja

  function Main()
    print baja(20)
  end Main
end TrazaTres
"""

CUATRO = """module TrazaCuatro
  class Falla extends Thread
    public function Falla()
      super(4096)
    end Falla

    public function run()
      this.parte()                     // @2
    end run

    function parte()
      var a: integer[] := [1, 2]
      print a[9]                       // @1
    end parte
  end Falla

  function Main()
    var h: Falla := Falla()
    h.start()
    h.join()
    print "el main sigue"
  end Main
end TrazaCuatro
"""

CINCO = """module TrazaCinco
  function Main()
    try
      var a: integer[] := [1]
      print a[9]
    catch e: Exception
      print "atrapada"
    endtry
  end Main
end TrazaCinco
"""

SEIS = """module TrazaSeis
  function Main()
    print "arrancado"
    while true do
      sleep(10)
    endwh
  end Main
end TrazaSeis
"""


SIETE = """module TrazaSiete
  import Collections

  function Main()
    var m: Collections.Map := Collections.Map()
    print m.get(null)                  // @1
    print "no llega"
  end Main
end TrazaSiete
"""


def marcas(fuente, nombre):
    """Las líneas marcadas, «@1» la de dentro: lo que tiene que salir, en ese orden."""
    d = {}
    for i, l in enumerate(fuente.split("\n"), 1):
        m = re.search(r"// @(\d+)", l)
        if m:
            d[int(m.group(1))] = "%s.bp:%d" % (nombre, i)
    return [d[k] for k in sorted(d)]


def vm_cli(cmd, cwd):
    """Lo que dice una VM del PC por stderr: la línea `=== traza: … ===`, o None."""
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
    m = re.search(r"^=== traza: (.*) ===$", r.stderr, re.M)
    return m.group(1) if m else None


def main():
    if not os.path.exists(JAR) or shutil.which("java") is None or not os.path.exists(STDLIB_PACK):
        print("FAIL: hacen falta el compilador (%s) y la librería de packs (%s)" % (JAR, STDLIB_PACK))
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="traza_smoke_")
    print("=== traza_smoke (#582): dónde falló, por el cable ===")
    fuentes = (("TrazaLib", LIB), ("TrazaUno", UNO), ("TrazaDos", DOS), ("TrazaTres", TRES), ("TrazaCuatro", CUATRO),
               ("TrazaCinco", CINCO), ("TrazaSeis", SEIS), ("TrazaSiete", SIETE))
    for f in os.listdir(STDLIB_DIR):
        if f.endswith(".mod"):
            shutil.copy(os.path.join(STDLIB_DIR, f), tmp)
    mods = {}
    for nombre, fuente in fuentes:
        mods[nombre] = run_smoke.compila(tmp, nombre, fuente)
        check(mods[nombre] is not None, "compila %s" % nombre)
    if None in mods.values():
        print("[status=FAIL: %d]" % fails)
        return 1
    proc, sock, c = run_smoke.arranca(os.path.join(tmp, "sim.flash"))
    trazas = {}
    try:
        for nombre, mod in mods.items():
            with open(mod, "rb") as f:
                check(c.call_bulk("PUT", f.read(), path="/app/%s.mod" % nombre).get("type") == "PUT_REPLY", "PUT de %s" % nombre)

        def corre(nombre):
            r, tramas, ex = c.run("/app/%s.mod" % nombre)
            ex = ex or {}
            print("       %s: status=%r stackTrace=%r" % (nombre, ex.get("status"), ex.get("stackTrace")))
            trazas[nombre] = ex.get("stackTrace") or ""
            return ex, b"".join(tramas)

        ex, _ = corre("TrazaUno")
        lineas = traduce(ex.get("stackTrace"), [tmp])
        quiero = marcas(LIB, "TrazaLib") + marcas(UNO, "TrazaUno")
        check(ex.get("status") == "RUNTIME_ERROR" and lineas == quiero, "1. un error de la VM, entre dos módulos: %s" % lineas)

        ex, _ = corre("TrazaDos")
        lineas = traduce(ex.get("stackTrace"), [tmp])
        check(ex.get("status") == "RUNTIME_ERROR" and lineas == marcas(DOS, "TrazaDos"), "2. un throw del programa: %s" % lineas)

        ex, _ = corre("TrazaTres")
        lineas = traduce(ex.get("stackTrace"), [tmp])
        m = marcas(TRES, "TrazaTres")
        check(len(ex.get("stackTrace", "").split()) == 8 and lineas == [m[0]] + [m[1]] * 7,
              "3. con 21 llamadas, las ocho de dentro: %s" % lineas)

        ex, salida = corre("TrazaCuatro")
        lineas = traduce(ex.get("stackTrace"), [tmp])
        check(ex.get("status") == "RUNTIME_ERROR" and b"el main sigue" in salida and lineas == marcas(CUATRO, "TrazaCuatro"),
              "4. un error en un hilo: %s" % lineas)

        ex, _ = corre("TrazaCinco")
        check(ex.get("status") == "OK" and "stackTrace" not in ex, "5. un error atrapado y fin bueno: sin stackTrace")

        ex, _ = corre("TrazaSiete")
        lineas = traduce(ex.get("stackTrace"), [tmp, STDLIB_DIR])
        m = marcas(SIETE, "TrazaSiete")
        check(ex.get("status") == "RUNTIME_ERROR" and len(lineas) >= 2 and lineas[0].startswith("Collections.bp:")
              and lineas[-1] == m[0], "8. un error dentro de la stdlib: %s" % lineas)

        c.id += 1
        c.s.sendall(('{"type":"RUN","path":"/app/TrazaSeis.mod","id":%d}\n' % c.id).encode())
        ex = None
        pedido = False
        while ex is None:
            m = c.linea()
            if m.get("type") == "OUTPUT" and not pedido:
                pedido = True
                c.id += 1
                c.s.sendall(('{"type":"KILL","id":%d}\n' % c.id).encode())
            elif m.get("type") == "EXITED":
                ex = m
        check(ex.get("status") == "KILLED" and "stackTrace" not in ex, "6. un KILL: sin stackTrace (dice %r)" % ex.get("status"))
    finally:
        sock.close()
        proc.kill()
        proc.wait()

    vm_c = os.path.abspath(VM_C)
    vm_j = os.path.abspath(VM_JAVA)
    for nombre in ("TrazaUno", "TrazaDos", "TrazaTres", "TrazaCuatro", "TrazaSiete"):
        en_c = vm_cli([vm_c, nombre + ".mod"], tmp)
        en_j = vm_cli(["java", "-Djava.awt.headless=true", "-jar", vm_j, nombre + ".mod"], tmp)
        check(en_c == en_j == trazas[nombre], "7. %s: las dos VMs y el sim, lo mismo (%s)" % (nombre, en_j))
    shutil.rmtree(tmp, ignore_errors=True)
    print("[status=OK]" if fails == 0 else "[status=FAIL: %d]" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
