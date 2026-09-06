# -*- coding: utf-8 -*-
"""reset_smoke.py — #452: un verbo mandado CON UN RUN VIVO, contra bpvm-sim.

    python tools/reset_smoke.py RESET    el arreglo de #452
    python tools/reset_smoke.py KILL     el camino que ya funcionaba (control)
    python tools/reset_smoke.py STATE    un verbo que SIGUE fuera de la lista (BUSY)

Se ejecuta desde la raiz del repo. Levanta el simulador, sube el programa y sus
dependencias como hace el IDE, arranca el RUN y manda el verbo a mitad.

>>> POR QUE ESTE ARNES TIENE UN CONTROL, Y POR QUE NO ES OPCIONAL <<<

La primera version de esta prueba salio VERDE con el bug dentro. Mandaba el
verbo "un segundo despues del RUN", y contra el simulador los bancos terminan
en decenas de milisegundos (Bench.mod: 80 ms) — asi que el RESET llegaba con la
VM EN REPOSO y lo atendia el despachador de siempre. Un camino ejecutado no es
un camino probado.

Por eso: antes de mandar el verbo se COMPRUEBA que el programa sigue vivo (que
han llegado mensajes OUTPUT y que NO ha llegado el EXITED), y si no, la prueba
se declara INVALIDA en vez de dar verde. Y por eso el programa es
samples/benchmarks/VivoLargo.bp, que late e insiste 30 s.
"""

import socket, json, subprocess, sys, time, os, select

REPO  = r"C:\lenguajes\pm"
SIM   = os.path.join(REPO, "bpgenvm-c", "build", "bpvm-sim.exe")
MOD   = os.path.join(REPO, "samples", "benchmarks", "VivoLargo.mod")
VERBO = (sys.argv[1] if len(sys.argv) > 1 else "RESET").upper()
PORT  = int(sys.argv[2]) if len(sys.argv) > 2 else 5117

sim = subprocess.Popen([SIM, "--port=%d" % PORT], cwd=REPO,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.2)
s = socket.create_connection(("127.0.0.1", PORT), timeout=20)
buf = b""; n = [0]
def sig(): n[0] += 1; return n[0]
def send(o): s.sendall((json.dumps(o) + "\n").encode())
def send_bulk(o, d):
    o = dict(o); o["bulk"] = len(d)
    s.sendall((json.dumps(o) + "\n").encode() + d)
def leer(t):
    global buf
    fin, out = time.time() + t, []
    while time.time() < fin:
        r, _, _ = select.select([s], [], [], 0.15)
        if not r: continue
        try: d = s.recv(65536)
        except Exception: break
        if not d: out.append(("CERRADA", "<<el sim CERRO la conexion>>", time.time())); break
        buf += d
        while b"\n" in buf:
            l, buf = buf.split(b"\n", 1)
            x = l.decode("utf-8", "replace").strip()
            if not x: continue
            try: tipo = json.loads(x).get("type", "?")
            except Exception: tipo = "?"
            out.append((tipo, x, time.time()))
    return out

send({"type": "HELLO", "id": sig()}); leer(2.0)
send_bulk({"type": "PUT", "id": sig(), "path": "/app/P.mod"}, open(MOD, "rb").read()); leer(3.0)
for d in ["Core", "Str", "Math", "Collections", "IO", "Pico"]:
    o = os.path.join(REPO, "bpstdlib", d + ".mod")
    if os.path.exists(o):
        send_bulk({"type": "PUT", "id": sig(), "path": "/lib/" + d + ".mod"}, open(o, "rb").read()); leer(3.0)

send({"type": "RUN", "id": sig(), "path": "/app/P.mod"})
ini = leer(1.5)
if not any(r[0] == "RUN_REPLY" for r in ini):
    sim.terminate(); sys.exit("el RUN no arranco: " + str(ini[:2]))

# --- CONTROL: ¿sigue vivo? ---
mitad = leer(1.5)
salida = sum(1 for r in ini + mitad if r[0] == "OUTPUT")
if any(r[0] == "EXITED" for r in ini + mitad):
    sim.terminate(); sys.exit("INVALIDA: el programa ya habia terminado antes del %s" % VERBO)
if salida == 0:
    sim.terminate(); sys.exit("INVALIDA: no hay senal de vida (0 OUTPUT) antes del %s" % VERBO)
print("control: el programa esta VIVO (%d OUTPUT, ningun EXITED)" % salida)

print("\n--- %s con el RUN vivo ---" % VERBO)
t0 = time.time()
send({"type": VERBO, "id": sig()})
for tipo, l, ts in leer(15.0):
    if tipo == "OUTPUT": continue
    print("   [+%6.1f ms] %s" % ((ts - t0) * 1000.0, l[:150]))
try: s.close()
except Exception: pass
sim.terminate()
