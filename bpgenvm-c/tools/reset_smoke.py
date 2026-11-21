# -*- coding: utf-8 -*-
"""reset_smoke.py — #452: un verbo mandado CON UN RUN VIVO, contra bpvm-sim.

    python tools/reset_smoke.py RESET    el arreglo de #452
    python tools/reset_smoke.py KILL     el camino que ya funcionaba (control)
    python tools/reset_smoke.py STATE    un verbo que SIGUE fuera de la lista (BUSY)

    python tools/reset_smoke.py RESET --serie=COM12   contra una PLACA de verdad

Se ejecuta desde la raiz del repo. Sin --serie levanta el simulador; con --serie
habla por el cable con la placa (115200, el wire v1 es el mismo JSON por lineas).
Sube el programa como hace el IDE, arranca el RUN y manda el verbo a mitad.

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

import socket, json, subprocess, sys, time, os, select, tempfile

REPO  = r"C:\lenguajes\pm"
SIM   = os.path.join(REPO, "bpgenvm-c", "build", "bpvm-sim.exe")
SIM   = os.environ.get("BPVM_SIM") or SIM   # compat.sh check-sim: el sim que construye la red
# V7/H8 (28-sep) — el programa se COMPILA al vuelo con el compilador de hoy (como sim_smoke): el .mod versionado era
# un MOD7 del 13-sep que ninguna VM de hoy enlazaba, y la prueba salía INVALIDA sin que nadie lo viera.
JAR   = os.path.join(REPO, "toolchain", "lexer-java", "target", "basicplus-frontend.jar")
def _compila():
    d = tempfile.mkdtemp(prefix="reset_smoke_mod_")
    src = os.path.join(REPO, "samples", "benchmarks", "VivoLargo.bp")
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", d, "--backend=mivm"],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    m = os.path.join(d, "VivoLargo.mod")
    if rc.returncode != 0 or not os.path.exists(m):
        sys.exit("INVALIDA: VivoLargo.bp no compila con el compilador de hoy (%s)" % JAR)
    return m
MOD   = _compila()
VERBO = (sys.argv[1] if len(sys.argv) > 1 else "RESET").upper()
PORT  = 5117
SERIE = None
for _a in sys.argv[2:]:
    if _a.startswith("--serie="): SERIE = _a.split("=", 1)[1]
    elif _a.isdigit():           PORT = int(_a)

class _CableCaido(Exception):
    pass

if SERIE:
    # Contra una PLACA: el wire v1 es el mismo JSON por lineas, sobre 115200.
    import serial                                   # pyserial
    sim = None
    _ser = serial.Serial(SERIE, 115200, timeout=0)
    time.sleep(0.4); _ser.reset_input_buffer()
    class _Cable(object):                           # misma cara que el socket
        def sendall(self, b): _ser.write(b); _ser.flush()
        def recv(self, n):    return _ser.read(4096)
        def close(self):      _ser.close()
    s = _Cable()
    def esperar():
        time.sleep(0.05)
        try:
            return [s] if _ser.in_waiting else []
        except Exception:
            # El USB CDC de la Pico VIVE EN EL FIRMWARE: al reiniciar, el puerto
            # desaparece. Eso no es un fallo del arnes, es la placa reiniciandose
            # — pero por si solo no prueba nada: la prueba es que VUELVA (abajo).
            raise _CableCaido()
else:
    # V7/H8 (28-sep) — como el IDE (SimRunner) y sim_smoke: la stdlib del micro simulado es packs/Stdlib.pack.
    # Antes se subian a /lib seis modulos de una lista a mano, y el dia que VivoLargo empezo a importar Machine
    # el programa ya no enlazaba y la prueba salia INVALIDA (el .mod, ademas, era MOD7).
    _flash = os.path.join(tempfile.mkdtemp(prefix="reset_smoke_"), "sim.flash")
    sim = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + _flash,
                            "--mem=1M", "--psram=8M", "--flash=4M",
                            "--pack=" + os.path.join(REPO, "packs", "Stdlib.pack")], cwd=REPO,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.2)
    s = socket.create_connection(("127.0.0.1", PORT), timeout=20)
    def esperar():
        r, _, _ = select.select([s], [], [], 0.15)
        return r

def parar_sim():
    if sim is not None: sim.terminate()

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
        try:
            hay = esperar()
        except _CableCaido:
            out.append(("CABLE", "<<el puerto DESAPARECIO: la placa se reinicia>>", time.time()))
            break
        if not hay: continue
        try: d = s.recv(65536)
        except Exception: break
        if not d:
            if SERIE: continue      # en serie, un read vacio es "aun no hay bytes"
            out.append(("CERRADA", "<<CERRO la conexion>>", time.time())); break
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
# La stdlib: en una placa, la de su imagen; en el simulador, la del Stdlib.pack con el que se arranca (arriba).

send({"type": "RUN", "id": sig(), "path": "/app/P.mod"})
ini = leer(1.5)
if not any(r[0] == "RUN_REPLY" for r in ini):
    parar_sim(); sys.exit("el RUN no arranco: " + str(ini[:2]))

# --- CONTROL: ¿sigue vivo? ---
mitad = leer(1.5)
salida = sum(1 for r in ini + mitad if r[0] == "OUTPUT")
if any(r[0] == "EXITED" for r in ini + mitad):
    parar_sim(); sys.exit("INVALIDA: el programa ya habia terminado antes del %s" % VERBO)
if salida == 0:
    parar_sim(); sys.exit("INVALIDA: no hay senal de vida (0 OUTPUT) antes del %s" % VERBO)
print("control: el programa esta VIVO (%d OUTPUT, ningun EXITED)" % salida)

print("\n--- %s con el RUN vivo ---" % VERBO)
t0 = time.time()
send({"type": VERBO, "id": sig()})
tras = leer(15.0)
for tipo, l, ts in tras:
    if tipo == "OUTPUT": continue
    print("   [+%6.1f ms] %s" % ((ts - t0) * 1000.0, l[:150]))

# V7/H8 (28-sep) — contra el simulador, COMPROBADO y no sólo impreso (la prueba está en check-sim; fuera de la red
# se pudrió: su .mod era MOD7 y le faltaba un módulo por subir). Con una placa se mira a mano, como siempre.
fallos = 0
if not SERIE:
    def _j(tipo):
        return [json.loads(l) for t_, l, ts in tras if t_ == tipo]
    def _chk(ok, msg):
        global fallos
        print(("  ok  " if ok else "  FAIL ") + msg)
        if not ok: fallos += 1
    ex = _j("EXITED")
    if VERBO in ("RESET", "KILL"):
        _chk(len(_j(VERBO + "_REPLY")) == 1, "el %s tiene su %s_REPLY" % (VERBO, VERBO))
        _chk(len(ex) == 1 and ex[0].get("status") == "KILLED", "el programa acaba KILLED: %s" % ex)
        msg = ex[0].get("errorMessage") if ex else None
        if VERBO == "RESET":
            _chk(msg == "reiniciado por RESET", "y el EXITED dice que lo paro un RESET, no un KILL: %r" % msg)
        else:
            _chk(msg != "reiniciado por RESET", "un KILL no dice RESET: %r" % msg)
    if fallos == 0:
        print("[status=OK] %s con el RUN vivo, contra el simulador" % VERBO)
try: s.close()
except Exception: pass

# La prueba de que hubo reinicio no es que el puerto se caiga —eso lo hace un
# cable flojo— sino que VUELVA y salude con un arranque nuevo.
if SERIE and VERBO == "RESET":
    print("")
    print("--- vuelve la placa? ---")
    _hola = ("{" + chr(34) + "type" + chr(34) + ":" + chr(34) + "HELLO" + chr(34)
             + "," + chr(34) + "id" + chr(34) + ":99}").encode() + bytes([10])
    volvio, fin2 = False, time.time() + 25
    while time.time() < fin2 and not volvio:
        time.sleep(1.0)
        try: v = serial.Serial(SERIE, 115200, timeout=0)
        except Exception: continue
        time.sleep(0.5); v.reset_input_buffer()
        v.write(_hola); v.flush()
        b2, f2 = b"", time.time() + 3
        while time.time() < f2 and bytes([10]) not in b2:
            d = v.read(4096)
            if d: b2 += d
            else: time.sleep(0.05)
        v.close()
        for l in b2.decode("utf-8", "replace").split(chr(10)):
            if "HELLO_REPLY" in l:
                print("   VOLVIO tras %.1f s: %s" % (time.time() - t0, l[:120]))
                volvio = True
                break
    if not volvio: print("   NO volvio en 25 s")
parar_sim()
sys.exit(1 if fallos else 0)
