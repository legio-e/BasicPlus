#!/usr/bin/env python3
# tools/sim_dbg_smoke.py — #326: banco de pruebas del DEPURADOR contra el micro
# simulado, sin placa.
#
# POR QUE EXISTE. En el Pico el 1er breakpoint va bien y a partir del 2o se
# cuelga, y solo desde que se migro al nucleo portable bpvm_dbg_wire (con su
# codigo viejo el Pico funciona). Buscarlo a ciegas en la placa costo una tarde
# y acabo en revert. Este guion monta el MISMO nucleo en el simulado:
#   - si aqui tambien se cuelga -> el bug esta en bpvm_dbg_wire y se caza en el
#     PC con depurador de verdad.
#   - si aqui NO se cuelga      -> es especifico del Pico (pila, memoria,
#     temporizacion) y ya sabemos por donde mirar.
#
# Guion: HELLO -> PUT del .mod -> PAUSE -> RUN -> BP_HIT -> STEP xN.
# El STEP repetido es la reproduccion mas barata de "la 2a vez se cuelga":
# ejercita el mismo bucle de pausa una y otra vez. Si algun STEP no contesta
# dentro del timeout, esta reproducido.
#
# V7/H10 (3-oct, el RUN comun, paso 0): A LA RED. Usaba un samples/Arith.mod MOD5 de mayo que ninguna VM carga ya,
# asi que llevaba meses sin probar nada. Ahora compila su propio programa (con la stdlib del pack del IDE, como
# env_smoke), usa el simulador de la red (BPVM_SIM) y prueba ademas #634 por el cable: un KILL con el programa en
# pausa acaba en KILLED (antes, en placa, no paraba).
import json, os, shutil, socket, subprocess, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import SIM, STDLIB_PACK, STDLIB_DIR, JAR   # noqa: E402 — el simulador y el compilador de la red

PASOS = """module DbgPasos
  function Main()
    var s: integer := 0
    var i: integer := 0
    while i < 50 do
      s := s + i
      i := i + 1
    endwh
    print "suma=" + s
  end Main
end DbgPasos
"""

STEPS = 8          # con 2 bastaria para el sintoma; 8 da margen para ver patron
TIMEOUT = 5.0

def free_port():
    s = socket.socket(); s.bind(("127.0.0.1", 0)); p = s.getsockname()[1]; s.close()
    return p

class Wire:
    def __init__(self, sock): self.s = sock; self.buf = b""
    def send(self, o):
        print("    >>", json.dumps(o)[:150])
        self.s.sendall((json.dumps(o) + "\n").encode())
    def recv(self, timeout=TIMEOUT):
        self.s.settimeout(timeout)
        while b"\n" not in self.buf:
            c = self.s.recv(4096)
            if not c: raise EOFError("el sim cerro la conexion")
            self.buf += c
        line, self.buf = self.buf.split(b"\n", 1)
        print("    <<", line.decode()[:150])
        return json.loads(line.decode())
    def recv_until(self, mtype, timeout=TIMEOUT):
        # NO descartar en silencio: si esperamos X y llega otra cosa (un ERROR,
        # por ejemplo), hay que DECIRLO. Descartar callado convierte "el sim me
        # contesto que no existe el fichero" en "timeout", que no dice nada.
        seen = []
        t0 = time.time()
        while time.time() - t0 < timeout:
            m = self.recv(timeout)
            if m.get("type") == mtype: return m
            seen.append(m)
            # ERROR/FATAL es obvio; EXITED tambien TERMINA la espera: si aguardas
            # un BP_HIT y el programa ya ha acabado, ese BP_HIT no va a llegar
            # jamas. Tragarselo convertia "el modulo peto por deps que faltan"
            # en un timeout mudo — me costo dos vueltas averiguarlo.
            if m.get("type") in ("ERROR", "FATAL", "EXITED"):
                raise RuntimeError("esperaba %s y llego %s: %s" % (mtype, m.get("type"), m))
        raise TimeoutError("%s no llego; por el camino: %s" % (mtype, seen))

def compila(dest):
    """El programa de los pasos, contra la stdlib de bpstdlib/. None si no se puede."""
    if not os.path.exists(JAR) or shutil.which("java") is None:
        return None
    src = os.path.join(dest, "DbgPasos.bp")
    with open(src, "w", encoding="utf-8") as f:
        f.write(PASOS)
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm", "--stdlibDir=" + STDLIB_DIR],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, "DbgPasos.mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

def main():
    tmp = tempfile.mkdtemp(prefix="sim_dbg_")
    mod = sys.argv[1] if len(sys.argv) > 1 else compila(tmp)
    if not mod or not os.path.exists(mod):
        print("  FAIL: no hay programa (sin el compilador %s, o no compila)" % JAR)
        print("\n[status=FALLA]")
        return 1
    if not os.path.exists(STDLIB_PACK):
        print("  FAIL: no esta la libreria de packs (%s)" % STDLIB_PACK)
        print("\n[status=FALLA]")
        return 1
    data = open(mod, "rb").read()
    name = os.path.basename(mod)

    port = free_port()
    img  = os.path.join(tmp, "sim_dbg_fs.img")
    sim = subprocess.Popen([SIM, "--port=%d" % port, "--fs=" + img, "--pack=" + STDLIB_PACK],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    fails = []
    try:
        s = None
        for _ in range(60):
            try:
                s = socket.create_connection(("127.0.0.1", port), timeout=5)
                break
            except OSError:
                time.sleep(0.1)
        if s is None:
            raise RuntimeError("el sim no acepta conexion")
        w = Wire(s)
        w.send({"type": "HELLO", "id": 1}); w.recv_until("HELLO_REPLY")

        # PUT del modulo (bulk detras de la linea)
        w.s.sendall((json.dumps({"type": "PUT", "id": 2, "path": "/app/" + name,
                                 "bulk": len(data)}) + "\n").encode())
        w.s.sendall(data)
        w.recv_until("PUT_REPLY")
        print("  ok  : PUT %s (%d B)" % (name, len(data)))

        # PAUSE antes del RUN -> rompe en el 1er opcode
        w.send({"type": "PAUSE", "id": 3}); w.recv_until("PAUSE_REPLY")
        print("  ok  : PAUSE aceptado")

        w.send({"type": "RUN", "id": 4, "path": "/app/" + name})
        w.recv_until("RUN_REPLY")
        hit = w.recv_until("BP_HIT")
        print("  ok  : BP_HIT inicial en pc=%s" % hit.get("pc"))

        # EL NUDO: repetir STEP. Si el estado del nucleo no se limpia entre
        # pausas, aqui es donde deja de contestar.
        for k in range(STEPS):
            w.send({"type": "STEP", "id": 100 + k})
            try:
                m = w.recv_until("BP_HIT", timeout=TIMEOUT)
                print("  ok  : STEP %d -> pc=%s" % (k + 1, m.get("pc")))
            except (TimeoutError, EOFError) as e:
                fails.append("STEP %d SIN RESPUESTA (%s) <<< REPRODUCIDO" % (k + 1, type(e).__name__))
                break
        if not fails:
            w.send({"type": "CONTINUE", "id": 200})
            ex = w.recv_until("EXITED", timeout=10.0)
            if ex.get("status") == "OK":
                print("  ok  : CONTINUE -> EXITED OK")
            else:
                fails.append("CONTINUE -> EXITED %r (esperado OK)" % ex.get("status"))

        # #634 (3-oct) — un KILL con el programa EN PAUSA: el Stop del IDE depurando. Tiene que acabar, y en KILLED.
        if not fails:
            w.send({"type": "PAUSE", "id": 300}); w.recv_until("PAUSE_REPLY")
            w.send({"type": "RUN", "id": 301, "path": "/app/" + name})
            w.recv_until("RUN_REPLY")
            w.recv_until("BP_HIT")
            w.send({"type": "KILL", "id": 302})
            ex = w.recv_until("EXITED", timeout=10.0)
            if ex.get("status") == "KILLED":
                print("  ok  : #634: KILL en pausa -> EXITED KILLED")
            else:
                fails.append("#634: KILL en pausa -> EXITED %r (esperado KILLED)" % ex.get("status"))
    except Exception as e:
        fails.append("excepcion: %r" % (e,))
    finally:
        sim.terminate()
        try:
            out = sim.stdout.read().decode(errors="replace") if sim.stdout else ""
        except Exception:
            out = ""
        if fails and out.strip():
            print("--- salida del sim ---")
            print(out[-1500:])
        shutil.rmtree(tmp, ignore_errors=True)

    print("\n[status=%s]" % ("OK" if not fails else "FALLA"))
    for f in fails: print("  !!", f)
    return 0 if not fails else 1

sys.exit(main())
