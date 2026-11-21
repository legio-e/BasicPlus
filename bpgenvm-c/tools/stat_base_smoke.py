#!/usr/bin/env python3
# stat_base_smoke.py — V7 #654 (7-oct): una PREGUNTA del IDE no cambia la carpeta del programa que corre.
#
# El IDE, antes de subir un proyecto, pregunta a la placa por cada dependencia «¿tienes Math.mod, mirando desde la
# carpeta /app/riego?» (STAT por nombre con `base`). La placa contestaba poniendo esa carpeta en la GLOBAL del
# proyecto un momento —la que usa el programa en marcha para sus rutas relativas, `App.projectPath()` y
# `pathAbsolute`— y la devolvía después; y desde H12 esa pregunta la atiende `io` MIENTRAS el programa corre. Ahora la
# carpeta va como parámetro al resolvedor, sin tocar la global (Eduardo, 7-oct: «la (a)»).
#
# La prueba, en el micro simulado (el camino exacto del IDE): un programa en /app/estacion mira su carpeta sin parar
# y cuenta las veces que no es la suya; mientras corre, el cable le hace 300 STAT con base=/app/riego. Tiene que
# acabar con «cambios: 0», y con la mayoría de los STAT contestados con el programa en marcha (si no, no ha probado
# nada). Sale 0 si pasa.
#
#   python tools/stat_base_smoke.py
import json, socket, subprocess, sys, os, time, tempfile, shutil

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

PORT = 5112
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]
SIM  = os.environ.get("BPVM_SIM") or SIM   # compat.sh check-sim: el sim que construye la red
JAR  = os.path.join(ROOT, "..", "toolchain", "lexer-java", "target", "basicplus-frontend.jar")
STDLIB_PACK = os.path.normpath(os.path.join(ROOT, "..", "packs", "Stdlib.pack"))
STDLIB_DIR  = os.path.normpath(os.path.join(ROOT, "..", "bpstdlib"))
N_STAT = 300

# Las vueltas (29-oct, con #736): con 9 M el programa duraba lo que ~150 STAT —el umbral mismo— y la prueba salía roja
# una vez de cada pocas (152–161 antes de #736, 147–158 después). Con 27 M dura ~8 s y los 300 STAT, ~5: aguanta una VM
# 1,6 veces más rápida sin perder ninguno, y 3 veces antes de bajar del umbral.
CARPETA_BP = """module Carpeta
  import App

  function Main()
    var primera: string := App.projectPath()
    var cambios: integer := 0
    var i: integer := 0
    while i < 27000000 do
      if App.projectPath() != primera then
        cambios := cambios + 1
      endif
      i := i + 1
    endwh
    print "carpeta: " + primera
    print "cambios: " + cambios
  end Main
end Carpeta
"""

fails = 0
def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

class Wire:
    def __init__(self, port):
        self.s = socket.create_connection(("127.0.0.1", port), timeout=60)
        self.buf = b""
        self.id = 0
    def line(self):
        while b"\n" not in self.buf:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerró la conexión")
            self.buf += chunk
        l, self.buf = self.buf.split(b"\n", 1)
        return json.loads(l.decode())
    def call_bulk(self, typ, data, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self.line()
    def send(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self.id

def compila(dest):
    if not os.path.exists(JAR) or shutil.which("java") is None: return None
    src = os.path.join(dest, "Carpeta.bp")
    with open(src, "w", encoding="utf-8") as f: f.write(CARPETA_BP)
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm", "--stdlibDir=" + STDLIB_DIR],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, "Carpeta.mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

def main():
    if not os.path.exists(STDLIB_PACK):
        print("  FAIL: no está la librería de packs (%s)" % STDLIB_PACK)
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="stat_base_")
    mod = compila(tmp)
    if mod is None:
        print("  FAIL: no se pudo compilar el programa de la prueba (¿java, el frontend?)")
        print("[status=FAIL]")
        return 1
    flash = os.path.join(tmp, "sim.flash")
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash, "--mem=1M", "--psram=8M", "--flash=4M",
                             "--pack=" + STDLIB_PACK], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        w = None
        for _ in range(100):
            try:
                w = Wire(PORT)
                break
            except OSError:
                time.sleep(0.1)
        check(w is not None, "el sim escucha en el puerto %d" % PORT)
        if w is None:
            print("[status=FAIL]")
            return 1
        with open(mod, "rb") as f: blob = f.read()
        r = w.call_bulk("PUT", blob, path="/app/estacion/Carpeta.mod")
        check(r.get("type") == "PUT_REPLY", "PUT del programa a /app/estacion")
        w.send("RUN", path="/app/estacion/Carpeta.mod")
        # Los STAT, uno tras otro, mientras corre: cada respuesta se espera saltando los OUTPUT del programa.
        salida, exited, en_marcha, contestados = [], None, 0, 0
        for _ in range(N_STAT):
            sid = w.send("STAT", name="Core.mod", base="/app/riego")
            while True:
                m = w.line()
                if m.get("type") == "OUTPUT": salida.append(m.get("data", ""))
                elif m.get("type") == "EXITED": exited = m
                if m.get("id") == sid:
                    contestados += 1
                    if exited is None: en_marcha += 1
                    break
        t0 = time.time()
        while exited is None and time.time() - t0 < 120:
            m = w.line()
            if m.get("type") == "OUTPUT": salida.append(m.get("data", ""))
            elif m.get("type") == "EXITED": exited = m
        texto = "".join(salida)
        print("  (la salida del programa: %s; %d STAT contestados, %d con el programa en marcha)"
              % (texto.strip().replace("\n", " | "), contestados, en_marcha))
        check(exited is not None, "el programa acaba (EXITED)")
        check("carpeta: /app/estacion" in texto, "el programa corre en su carpeta, /app/estacion")
        check(en_marcha >= N_STAT // 2, "%d de %d preguntas del cable con el programa en marcha (el caso, ejercitado)"
              % (en_marcha, N_STAT))
        check("cambios: 0" in texto, "su carpeta no se movió mientras el cable preguntaba por /app/riego (#654)")
    finally:
        proc.kill()
        proc.wait()
        shutil.rmtree(tmp, ignore_errors=True)
    print("[status=%s]" % ("OK" if fails == 0 else "FAIL"))
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
