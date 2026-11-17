#!/usr/bin/env python3
"""raices_smoke.py — V8 #799 (13-nov): EN LA PILA DE UN HILO BP, UN ENTERO YA NO RETIENE UN OBJETO.

El GC recorre la pila de cada hilo palabra a palabra. Una palabra sin TAG nunca es ahí una referencia (las referencias
van como handles; una dirección cruda sólo vive en las pilas de C de las natives), pero la tomaba por una si caía justo
en un objeto. Con Core en un pack —las placas y este micro simulado—, el heap empieza en unos 3 KB, donde viven los
enteros pequeños: un File muerto se quedaba abierto tras gc() porque un entero de la pila valía su dirección (medido el
13-nov: 2944). Ahora, en la pila, sólo cuentan los handles (heap.c, gc_mark_phase, paso 1; miVM, scanRegion).

La prueba: un File muere sin close(), y durante el gc() está en la pila un entero `x`, el argumento de ejecución (con
seis cifras, para que nada se mueva de un RUN a otro).
  1. El SABOTAJE (--sabotaje-raices: el recorrido de antes): se barre x hasta dar con el que retiene el File —su
     dirección—. Si no lo encuentra, la prueba no vale: el rango ya no cubre el heap.
  2. Sin sabotaje, ese mismo x ya no lo retiene, ni x=0.
(El otro arreglo de #799, ENTER con las locales a cero, lo prueba GcHuecoPila, en el corpus.)

V8 #833 (17-nov): LO MISMO EN UNA GLOBAL. El bloque de datos de un módulo (sus globales y sus constantes) se recorría
de forma conservadora: en la prueba de resistencia, `minLibre` (una global) retuvo tres bloques en la C6 y TOPE_LOG
(una constante), uno en la Pico 2. Ahora, en la pila y en el bloque de datos, sólo cuenta una referencia viva: con la
marca de handle y su contador, el del slot (heap.c, gc_es_referencia). La prueba, la misma con el número en una global
`g` (y no en la pila durante el gc()):
  3. con el sabotaje (el recorrido de antes), se barre g hasta dar con el que retiene el File;
  4. sin sabotaje, ese mismo g ya no lo retiene.
(La referencia caducada, que tampoco retiene, la prueban test_raices.c y RaicesTest: desde BP no se puede preparar.)

    python tools/raices_smoke.py      (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import os, shutil, sys, tempfile

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_tarea_smoke as rts                               # noqa: E402 — el mismo arranque y el mismo cable

rts.PORT = 5141
DESDE, HASTA = 1024, 65536
fails = 0


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg, flush=True)
    if not cond:
        fails += 1


PROGRAMA = """module RaicesFalsas
  import IO

  const R: string := "raicesfalsas.bin"

  function sinCerrar()
    var f: IO.File := IO.File(R, "w")                 // muere sin close()
    f.write(toBytes("123"))
  end sinCerrar

  function prueba(x: integer)
    gc()                                              // con x en la pila
    sleep(0)                                          // cede: su destructor corre y lo cierra
    try
      var otra: IO.File := IO.File(R, "w")
      print "x=" + intToString(x) + " abre bien"
      otra.close()
    catch e: RuntimeError
      print "x=" + intToString(x) + " RETENIDO: " + e.msg
    endtry
  end prueba

  public function Main(arg: string := "000000")
    sinCerrar()
    prueba(parseInt(arg))
  end Main
end RaicesFalsas
"""

# V8 #833 — el número, en una global: durante el gc() no está en la pila.
PROGRAMA_GLOBAL = """module GlobalesFalsas
  import IO

  const R: string := "globalesfalsas.bin"
  var g: integer := 0                                 // un número en el bloque de datos del módulo

  function sinCerrar()
    var f: IO.File := IO.File(R, "w")                 // muere sin close()
    f.write(toBytes("123"))
  end sinCerrar

  function prueba()
    gc()                                              // con g en su global, y no en la pila
    sleep(0)                                          // cede: su destructor corre y lo cierra
    try
      var otra: IO.File := IO.File(R, "w")
      print "g=" + intToString(g) + " abre bien"
      otra.close()
    catch e: RuntimeError
      print "g=" + intToString(g) + " RETENIDO: " + e.msg
    endtry
  end prueba

  public function Main(arg: string := "000000")
    g := parseInt(arg)
    sinCerrar()
    prueba()
  end Main
end GlobalesFalsas
"""


def corre(c, x, mod="RaicesFalsas"):
    r = c.call("RUN", path="/app/%s.mod" % mod, arg="%06d" % x)
    if r.get("type") != "RUN_REPLY":
        return "RUN rechazado: %r" % r
    trozos = []
    while True:
        m = c.linea()
        if m.get("type") == "OUTPUT":
            trozos.append(m.get("data", ""))
        elif m.get("type") == "EXITED":
            return "".join(trozos).strip()


def main():
    if not os.path.exists(rts.JAR) or not os.path.exists(rts.STDLIB_PACK):
        print("FAIL: hacen falta el compilador (%s) y la librería de packs (%s)" % (rts.JAR, rts.STDLIB_PACK))
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="raices_")
    print("=== raices_smoke (V8 #799 y #833): en la pila y en una global, un número no retiene un objeto "
          "(Core en el pack) ===", flush=True)
    mods = {"RaicesFalsas": rts.compila(tmp, "RaicesFalsas", PROGRAMA),
            "GlobalesFalsas": rts.compila(tmp, "GlobalesFalsas", PROGRAMA_GLOBAL)}
    for m in mods:
        check(mods[m] is not None, "compila " + m)
    if None in mods.values():
        print("[status=FAIL: %d]" % fails)
        return 1

    # 1. el sabotaje: el x que retiene el File (su dirección); 3. y el g, en la global
    retiene = None
    retiene_g = None
    proc, sock, c = rts.arranca(os.path.join(tmp, "s.flash"), ["--sabotaje-raices"])
    try:
        check(rts.subir(c, mods), "PUT (con --sabotaje-raices)")
        check("abre bien" in corre(c, 0), "con el sabotaje, x=0 no retiene nada (lo que retenga, será el entero)")
        for x in range(DESDE, HASTA, 4):
            if "RETENIDO" in corre(c, x):
                retiene = x
                break
        check("abre bien" in corre(c, 0, "GlobalesFalsas"),
              "con el sabotaje, g=0 no retiene nada (lo que retenga, será la global)")
        for g in range(DESDE, HASTA, 4):
            if "RETENIDO" in corre(c, g, "GlobalesFalsas"):
                retiene_g = g
                break
    finally:
        rts.para(proc, sock)
    check(retiene is not None,
          "con el sabotaje (el recorrido de antes), un entero en la pila retiene el File muerto: x=%s, su dirección "
          "(si no aparece, la prueba no vale: el heap ya no está en [%d, %d))" % (retiene, DESDE, HASTA))
    check(retiene_g is not None,
          "con el sabotaje, un número en una global retiene el File muerto: g=%s, su dirección (#833; si no aparece, "
          "la prueba no vale)" % retiene_g)

    # 2. sin sabotaje, ese mismo entero ya no lo retiene; 4. ni el número de la global
    proc, sock, c = rts.arranca(os.path.join(tmp, "a.flash"), [])
    try:
        check(rts.subir(c, mods), "PUT (sin sabotaje)")
        s0 = corre(c, 0)
        check("abre bien" in s0, "x=0: el File muerto se cierra en el primer gc() (%s)" % s0)
        if retiene is not None:
            sx = corre(c, retiene)
            check("abre bien" in sx, "x=%d, su dirección: ya no lo retiene, en la pila sólo cuentan los handles (%s)"
                  % (retiene, sx))
        g0 = corre(c, 0, "GlobalesFalsas")
        check("abre bien" in g0, "g=0: el File muerto se cierra en el primer gc() (%s)" % g0)
        if retiene_g is not None:
            sg = corre(c, retiene_g, "GlobalesFalsas")
            check("abre bien" in sg, "g=%d, su dirección: ya no lo retiene, en el bloque de datos sólo cuentan las "
                  "referencias vivas (#833; %s)" % (retiene_g, sg))
    finally:
        rts.para(proc, sock)

    shutil.rmtree(tmp, ignore_errors=True)
    print("[status=OK]" if fails == 0 else "[status=FAIL: %d]" % fails)
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
