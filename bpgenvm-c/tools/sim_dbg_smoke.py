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
import json, os, socket, subprocess, sys, tempfile, time

STEPS = 8          # con 2 bastaria para el sintoma; 8 da margen para ver patron
TIMEOUT = 5.0

def free_port():
    s = socket.socket(); s.bind(("127.0.0.1", 0)); p = s.getsockname()[1]; s.close()
    return p

class Wire:
    def __init__(self, sock): self.s = sock; self.buf = b""
    def send(self, o): self.s.sendall((json.dumps(o) + "\n").encode())
    def recv(self, timeout=TIMEOUT):
        self.s.settimeout(timeout)
        while b"\n" not in self.buf:
            c = self.s.recv(4096)
            if not c: raise EOFError("el sim cerro la conexion")
            self.buf += c
        line, self.buf = self.buf.split(b"\n", 1)
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
            if m.get("type") in ("ERROR", "FATAL"):
                raise RuntimeError("esperaba %s y llego %s: %s" % (mtype, m.get("type"), m))
        raise TimeoutError("%s no llego; por el camino: %s" % (mtype, seen))

def main():
    mod = sys.argv[1] if len(sys.argv) > 1 else "samples/Arith.mod"
    if not os.path.exists(mod):
        print("no existe", mod); return 2
    data = open(mod, "rb").read()
    name = os.path.basename(mod)

    port = free_port()
    img  = os.path.join(tempfile.gettempdir(), "sim_dbg_fs.img")
    if os.path.exists(img): os.remove(img)
    exe = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build", "bpvm-sim.exe"))
    sim = subprocess.Popen([exe, "--port=%d" % port, "--fs=" + img],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    time.sleep(0.6)
    fails = []
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=5)
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
            w.recv_until("EXITED", timeout=10.0)
            print("  ok  : CONTINUE -> EXITED")
    except Exception as e:
        fails.append("excepcion: %r" % (e,))
    finally:
        sim.terminate()
        try:
            out = sim.stdout.read().decode(errors="replace") if sim.stdout else ""
        except Exception:
            out = ""
        if out.strip():
            print("--- salida del sim ---")
            print(out[-1500:])

    print("\n[status=%s]" % ("OK" if not fails else "FALLA"))
    for f in fails: print("  !!", f)
    return 0 if not fails else 1

sys.exit(main())
