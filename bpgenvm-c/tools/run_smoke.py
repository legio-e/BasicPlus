#!/usr/bin/env python3
"""run_smoke.py — V7/H10, el RUN común (3-oct): LO QUE HACE EL RUN en los casos que la red no cubre.

El RUN está escrito cuatro veces (la Pico, la ESP32, la STM32 y el micro simulado) y va a quedar en uno
(`V7_IDEAS` §H10 «RUN en un solo sitio»), con unas reglas que Eduardo decidió el 2-oct. Antes de tocar nada, esto FIJA
la conducta del micro simulado —el primero que estrena el RUN común— en los casos que ninguna otra prueba mira. El paso 0
la fijó tal como era; el paso 2 (el RUN común) cambió aquí las cuatro expectativas que cambian sus reglas, en el mismo
commit —el diff dice qué cambió—:

  1. un fallo de CARGA (falta un módulo): RUNTIME_ERROR, exitCode 2 —el «no se pudo cargar» de la tabla común— y
     «falta el modulo 'X'…» (antes, en el sim, exitCode -2);
  2. un NUL en mitad de una línea: sale \\u0000 y el resto de la línea (antes, el sim cortaba ahí, salto incluido);
  3. una línea de 512 caracteres de control (cada uno escapa a 6 bytes: 3072): entera, en tramas de 1 KB (la STM32
     la tiraba entera);
  4. un carácter UTF-8 que cae en el byte 512 (el hilo `io` trocea a 512 B sin mirar caracteres): el sink común guarda
     el byte suelto y el carácter sale ENTERO en la trama siguiente (antes, partido en dos tramas);
  5. un KILL sin programa: NO_SESSION, como en las placas (antes, en el sim, KILL_REPLY);
  6. un PING con un programa en marcha: PONG, como en la VM-Java (#803: iba a la cola del depurador, y sólo se leía en
     una pausa). Es el latido de un cliente por la red: sin respuesta, da por perdida una conexión viva.
  7. V8/H2 (8-nov): el Bluetooth simulado, también en el micro simulado (Eduardo: «ponlo»): AirBluetooth, la prueba del
     corpus, dice lo mismo que en la VM-C del PC;
  8. y, en otro arranque sobre la misma flash, con /sys/bluetooth.json en autoStart: encendido, el recordado conectado
     y uno que no está, sin conectar y sin error.

Lee el cable en BYTES (un carácter partido no es UTF-8 válido y un lector estricto lo tiraría). Necesita el compilador
(lexer-java) y la librería de packs (packs/Stdlib.pack), como env_smoke.

    python tools/run_smoke.py            (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import json, os, shutil, socket, subprocess, sys, tempfile, time

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import SIM, STDLIB_PACK, STDLIB_DIR, JAR   # noqa: E402 — el mismo simulador y el mismo compilador

PORT = 5127
fails = 0


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond:
        fails += 1


class Cable:
    """El wire en BYTES: cada línea se parsea con surrogateescape, y `crudo()` devuelve los bytes de un campo."""
    def __init__(self, sock):
        self.s = sock; self.buf = b""; self.id = 0

    def linea(self):
        while b"\n" not in self.buf:
            c = self.s.recv(65536)
            if not c:
                raise EOFError("el sim cerró la conexión")
            self.buf += c
        l, self.buf = self.buf.split(b"\n", 1)
        return json.loads(l.decode("utf-8", "surrogateescape"))

    def call(self, typ, **campos):
        self.id += 1
        campos.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(campos) + "\n").encode())
        return self.linea()

    def call_bulk(self, typ, datos, **campos):
        self.id += 1
        campos.update(type=typ, id=self.id, bulk=len(datos))
        self.s.sendall((json.dumps(campos) + "\n").encode() + datos)
        return self.linea()

    def run(self, path, timeout=60.0):
        """RUN y lo que sale hasta el EXITED: (respuesta, [datos de cada OUTPUT, en bytes], exited)."""
        r = self.call("RUN", path=path)
        if r.get("type") != "RUN_REPLY":
            return r, [], None
        tramas, t0 = [], time.time()
        while time.time() - t0 < timeout:
            m = self.linea()
            if m.get("type") == "OUTPUT":
                tramas.append(m.get("data", "").encode("utf-8", "surrogateescape"))
            elif m.get("type") == "EXITED":
                return r, tramas, m
        return r, tramas, None


CASOS = """module RunCasos
  import Str

  function Main()
    var nul: string := "a" + Str.fromCharCode(0) + "b"
    print "nul=[" + nul + "]"
    var ctl: string := ""
    var i: integer := 0
    while i < 512 do
      ctl := ctl + Str.fromCharCode(1)
      i := i + 1
    endwh
    print ctl
    var largo: string := ""
    i := 0
    while i < 511 do
      largo := largo + "a"
      i := i + 1
    endwh
    print largo + Str.fromCharCode(241) + "z"
    print "fin"
  end Main
end RunCasos
"""

FALTA = """module RunFalta
  public function hola(): integer
    return 1
  end hola
end RunFalta
"""

USA = """module RunUsa
  import RunFalta

  function Main()
    print "no deberia llegar: " + RunFalta.hola()
  end Main
end RunUsa
"""

CALLADO = """module RunCallado
  import Core

  function Main()
    sleep(1500)
    print "despierto"
  end Main
end RunCallado
"""


# 7 y 8 — V8/H2 (8-nov): el Bluetooth simulado, también en el micro simulado (Eduardo: «ponlo»). La prueba del corpus,
# AirBluetooth, tiene que decir aquí lo mismo que en la VM-C del PC; y al arrancar con /sys/bluetooth.json en autoStart,
# los recordados vuelven solos, y uno que no está no es un error (Eduardo: «al menos que lo intenten pero si no están o
# no conectan que no se produzca un error»).
AIR_BT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "samples", "AirBluetooth.bp")
VM_C = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build",
                    "bpgenvm-c.exe" if os.name == "nt" else "bpgenvm-c")

BT_ARRANQUE = """module RunBtArranque
  import Air

  function Main()
    var bt: Air.Bluetooth := Air.Bluetooth()
    print "encendido al arrancar: " + bt.isOn()
    var l: List := bt.listDevices()
    var i: integer := 0
    while i < l.length() do
      var d: Air.BtDevice := Air.BtDevice(l.get(i))
      print d.getName() + " conectado=" + d.isConnected()
      i := i + 1
    endwh
  end Main
end RunBtArranque
"""

BT_JSON = (b'{"autoStart":true,"devices":{"02:00:00:00:00:01":{"name":"Teclado simulado","addressType":"random",'
           b'"kind":"keyboard"},"02:00:00:00:00:07":{"name":"Otro","addressType":"public","kind":"unknown"}}}')


def salida_pc(mod, dest):
    """Lo que dice la VM-C del PC con ese .mod, con una flash nueva y sin sus líneas «=== …»; los saltos, LF."""
    flash = os.path.join(dest, "flash_pc")
    shutil.rmtree(flash, ignore_errors=True)
    r = subprocess.run([VM_C, "--flash-dir=" + flash, "--libdir=" + STDLIB_DIR, mod], cwd=dest,
                       capture_output=True, timeout=120)
    lineas = r.stdout.replace(b"\r\n", b"\n").split(b"\n")
    return b"\n".join(l for l in lineas if not l.startswith(b"=== ")).strip(b"\n") + b"\n"


def compila(dest, nombre, fuente):
    src = os.path.join(dest, nombre + ".bp")
    with open(src, "w", encoding="utf-8") as f:
        f.write(fuente)
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm", "--stdlibDir=" + STDLIB_DIR],
                        cwd=dest, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, nombre + ".mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None


def arranca(flash, extra=()):
    """`extra`: opciones más para el sim (overlay_smoke, V8 #833: --sabotaje-raices)."""
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash, "--mem=1M", "--flash=4M",
                             "--pack=" + STDLIB_PACK] + list(extra), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    for _ in range(80):
        try:
            sock = socket.create_connection(("127.0.0.1", PORT), timeout=2.0)
            sock.settimeout(60.0)
            c = Cable(sock)
            c.call("HELLO", protoVersion=1, clientName="run_smoke")
            return proc, sock, c
        except OSError:
            time.sleep(0.1)
    proc.kill()
    raise RuntimeError("el sim no acepta conexión")


def main():
    if not os.path.exists(JAR) or shutil.which("java") is None or not os.path.exists(STDLIB_PACK):
        print("FAIL: hacen falta el compilador (%s) y la librería de packs (%s)" % (JAR, STDLIB_PACK))
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="run_smoke_")
    print("=== run_smoke (el RUN común): lo que hace el RUN del micro simulado ===")
    mods = {}
    with open(AIR_BT, encoding="utf-8") as f:
        air_bt = f.read()
    for nombre, fuente in (("RunCasos", CASOS), ("RunFalta", FALTA), ("RunUsa", USA), ("RunCallado", CALLADO),
                           ("AirBluetooth", air_bt), ("RunBtArranque", BT_ARRANQUE)):
        mods[nombre] = compila(tmp, nombre, fuente)
        check(mods[nombre] is not None, "compila %s" % nombre)
    if None in mods.values():
        print("[status=FAIL: %d]" % fails)
        return 1
    proc, sock, c = arranca(os.path.join(tmp, "sim.flash"))
    try:
        # 5 — el KILL sin programa (antes de nada: la placa está en reposo)
        r = c.call("KILL")
        check(r.get("type") == "ERROR" and r.get("code") == "NO_SESSION",
              "5. KILL sin programa -> NO_SESSION, como en las placas (antes: KILL_REPLY) (dice %r %r)"
              % (r.get("type"), r.get("code")))

        # 1 — el fallo de carga: se sube RunUsa y NO RunFalta
        with open(mods["RunUsa"], "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/RunUsa.mod").get("type") == "PUT_REPLY", "1. PUT de RunUsa")
        r, tramas, ex = c.run("/app/RunUsa.mod")
        ex = ex or {}
        msg = ex.get("errorMessage") or ""
        print("       EXITED: status=%r exitCode=%r errorMessage=%r" % (ex.get("status"), ex.get("exitCode"), msg))
        check(ex.get("status") == "RUNTIME_ERROR" and ex.get("exitCode") == 2
              and msg.startswith("falta el modulo 'RunFalta': "),
              "1. falta un módulo -> RUNTIME_ERROR, exitCode 2 y «falta el modulo 'RunFalta': …» (antes: exitCode -2)")
        check(tramas == [], "1. y no sale nada del programa")

        # 2, 3, 4 — la salida
        with open(mods["RunCasos"], "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/RunCasos.mod").get("type") == "PUT_REPLY", "PUT de RunCasos")
        r, tramas, ex = c.run("/app/RunCasos.mod")
        ex = ex or {}
        check(ex.get("status") == "OK", "RunCasos acaba bien (dice %r %r)" % (ex.get("status"), ex.get("errorMessage")))
        todo = b"".join(tramas)
        print("       %d tramas de OUTPUT, %d bytes: %s" % (len(tramas), len(todo), [len(t) for t in tramas]))
        check(b"nul=[a\x00b]\n" in todo,
              "2. un NUL -> sale \\u0000 y el resto de la línea, salto incluido (antes: se cortaba en el NUL)")
        ctl = b"\x01" * 512
        check(ctl + b"\n" in todo, "3. 512 controles -> enteros, en tramas de 1 KB (la STM32 los tiraba)")
        ene = "ñ".encode("utf-8")
        partida = any(t.endswith(ene[:1]) for t in tramas) or any(t.startswith(ene[1:]) for t in tramas)
        check(not partida and any(ene + b"z" in t for t in tramas),
              "4. una ñ en el byte 512 -> ENTERA en una trama (antes: partida en dos)")
        check(todo.endswith(b"fin\n"), "y lo último, «fin»")

        # 6 — el PING con un programa en marcha (#803)
        with open(mods["RunCallado"], "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/RunCallado.mod").get("type") == "PUT_REPLY", "PUT de RunCallado")
        r = c.call("RUN", path="/app/RunCallado.mod")
        check(r.get("type") == "RUN_REPLY", "6. el programa arranca")
        c.id += 1
        ping = c.id
        c.s.sendall((json.dumps({"type": "PING", "id": ping}) + "\n").encode())
        pong, ex = None, None
        while ex is None:
            m = c.linea()
            if m.get("id") == ping:
                pong = m
            elif m.get("type") == "EXITED":
                ex = m
        check(pong is not None and pong.get("type") == "PONG",
              "6. un PING en marcha -> PONG antes del EXITED, como en la VM-Java (dice %r)" % (pong,))
        check(ex.get("status") == "OK", "6. y el programa acaba bien (dice %r)" % ex.get("status"))

        # 7 — el Bluetooth simulado del micro simulado (V8/H2): AirBluetooth, lo mismo que en la VM-C del PC
        with open(mods["AirBluetooth"], "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/AirBluetooth.mod").get("type") == "PUT_REPLY",
                  "PUT de AirBluetooth")
        r, tramas, ex = c.run("/app/AirBluetooth.mod")
        ex = ex or {}
        check(ex.get("status") == "OK", "7. AirBluetooth acaba bien (dice %r %r)" % (ex.get("status"), ex.get("errorMessage")))
        sim, pc = b"".join(tramas), salida_pc(mods["AirBluetooth"], tmp)
        check(sim == pc and b"vistos: 2\n" in sim,
              "7. el Bluetooth del micro simulado: AirBluetooth dice lo mismo que en la VM-C del PC (%d líneas)"
              % sim.count(b"\n"))
        if sim != pc:
            for a, b in zip(sim.split(b"\n"), pc.split(b"\n")):
                if a != b:
                    print("       sim: %r\n       pc:  %r" % (a, b))
                    break
        # y para el 8: lo recordado, con un aparato que no está, y el programa que mira el arranque
        check(c.call_bulk("PUT", BT_JSON, path="/sys/bluetooth.json").get("type") == "PUT_REPLY",
              "PUT de /sys/bluetooth.json, con autoStart")
        with open(mods["RunBtArranque"], "rb") as f:
            check(c.call_bulk("PUT", f.read(), path="/app/RunBtArranque.mod").get("type") == "PUT_REPLY",
                  "PUT de RunBtArranque")
    finally:
        try:
            sock.close()
        finally:
            proc.terminate()
            try: proc.wait(timeout=5)
            except Exception: proc.kill()

    # 8 — otro arranque sobre la misma flash, como una placa: con autoStart, el Bluetooth encendido y los recordados
    #     conectados, y el que no está, sin conectar y sin error
    proc, sock, c = arranca(os.path.join(tmp, "sim.flash"))
    try:
        r, tramas, ex = c.run("/app/RunBtArranque.mod")
        ex = ex or {}
        todo = b"".join(tramas).decode("utf-8", "replace")
        check(ex.get("status") == "OK", "8. RunBtArranque acaba bien (dice %r %r)" % (ex.get("status"), ex.get("errorMessage")))
        check(todo == "encendido al arrancar: true\nTeclado simulado conectado=true\nOtro conectado=false\n",
              "8. al arrancar con autoStart: encendido, el recordado conectado y el que no está, sin error (dice %r)" % todo)
    finally:
        try:
            sock.close()
        finally:
            proc.terminate()
            try: proc.wait(timeout=5)
            except Exception: proc.kill()
    print("[status=OK]" if fails == 0 else "[status=FAIL: %d]" % fails)
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
