# io_smoke.py — V6/A1.1: lo que el hilo `io` tiene que demostrar, contra el simulador.
#
# El arnes de paridad ya prueba que la SALIDA sigue siendo byte-identica con los
# dos hilos. Lo que no prueba es lo que `io` viene a arreglar, que son dos cosas
# de COMPORTAMIENTO y una de FORMA:
#
#   1. El KILL llega mientras el programa CALCULA sin imprimir. Antes lo leia la
#      propia VM entre cuantos; ahora lo lee `io` y la VM solo mira una bandera.
#   2. El KILL llega mientras el programa IMPRIME A CHORRO. Ese era el caso malo:
#      la escritura al transporte era sincrona y se hacia desde la VM, asi que un
#      KILL esperaba a que el chorro drenase.
#   3. Un `print` con varios argumentos sale en UN mensaje OUTPUT (una linea),
#      no en uno por argumento. En la C6 eso eran 6 mensajes por linea y 122 KB
#      de texto viajando como 816 KB.
#
# Y el control de todo esto: la salida entregada tiene que ser EXACTA — ni una
# linea perdida al parar, que es el fallo que un apagado mal hecho produce.
import json, os, socket, subprocess, sys, tempfile, time, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(ROOT)
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM): SIM = os.path.join(ROOT, "build", "bpvm-sim")
JAR  = os.path.join(REPO, "lexer-java", "target", "basicplus-frontend.jar")
PORT = 5117
fails = 0

def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

class Wire:
    def __init__(self, sock):
        self.s = sock; self.buf = b""; self.id = 0
    def _line(self, timeout=None):
        if timeout is not None: self.s.settimeout(timeout)
        while b"\n" not in self.buf:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerro la conexion")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode())
    def send(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self.id
    def call(self, typ, **fields):
        self.send(typ, **fields)
        return self._line()
    def call_bulk(self, typ, data, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self._line()

def compila(src_bp, dest):
    if not os.path.exists(JAR) or shutil.which("java") is None: return None
    # OJO: se compila DESDE la raiz del repo. El caso del GUI importa Gui, y el
    # frontend resuelve la stdlib por el BpVM.cfg mas cercano — desde un temporal
    # no lo encuentra y el import falla. Los otros casos no importan nada y por eso
    # el fallo no se veia.
    rc = subprocess.run(["java", "-jar", JAR, src_bp, "--compile", dest, "--backend=mivm"],
                        cwd=REPO,
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, os.path.basename(src_bp).replace(".bp", ".mod"))
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

FUENTE_CALC = """module IoCalc
  function Main()
    var i: integer := 0
    var s: integer := 0
    while i < 400000000 do
      s := s + i
      i := i + 1
    endwh
    print "no deberia llegar aqui: ", s
  end Main
end IoCalc
"""

FUENTE_CHORRO = """module IoChorro
  function Main()
    var i: integer := 0
    while i < 200000 do
      print "linea ", i, " con varios argumentos"
      i := i + 1
    endwh
  end Main
end IoChorro
"""

FUENTE_CORTO = """module IoCorto
  function Main()
    var i: integer := 0
    while i < 5 do
      print "linea ", i, " con varios argumentos"
      i := i + 1
    endwh
    print "fin"
  end Main
end IoCorto
"""

# #462 - el KILL sobre un programa con GUI: el caso que daba «exit 0 (OK)».
# `Gui.run()` es `while __guiRunOnce() do endwh`, y el builtin devuelve FALSO al
# ver el KILL: el bucle BP termina limpiamente, Main retorna y el programa acaba
# «bien». Medido en placa el 5-sep (P4 y Discovery): un KILL daba status OK. Con
# la bandera mandando sobre el estado de salida, tiene que decir KILLED.
FUENTE_GUI = """module IoGui
  import Gui

  function Main()
    var scr: Gui.Screen := Gui.Screen()
    var b: Gui.Button := Gui.Button(scr, "parame")
    b.align(Gui.Align.CENTER, 0, 0)
    print "gui lista"
    Gui.run()
    print "no deberia llegar aqui"
  end Main
end IoGui
"""

def kill_durante(w, mod_remoto, etiqueta, espera_previa):
    """RUN, espera, KILL, y mide cuanto tarda en llegar el EXITED."""
    w.send("RUN", path=mod_remoto)
    r = w._line(timeout=10)
    if r.get("type") != "RUN_REPLY":
        check(False, etiqueta + ": el RUN no arranco (" + json.dumps(r)[:90] + ")")
        return
    time.sleep(espera_previa)
    t0 = time.time()
    w.send("KILL")
    exited = None; kill_reply = False; salida = 0
    while time.time() - t0 < 20:
        m = w._line(timeout=20)
        t = m.get("type")
        if t == "EXITED": exited = m; break
        if t == "KILL_REPLY": kill_reply = True
        elif t == "OUTPUT": salida += 1
    dt = (time.time() - t0) * 1000.0
    check(exited is not None and exited.get("status") == "KILLED",
          etiqueta + ": EXITED KILLED (" + str(exited and exited.get("status")) + ")")
    check(kill_reply, etiqueta + ": KILL_REPLY antes del EXITED")
    check(dt < 3000, etiqueta + ": el KILL se atendio en %.0f ms (tope 3000)" % dt)
    print("        (%.0f ms, %d mensajes OUTPUT tras el KILL)" % (dt, salida))

def main():
    if not os.path.exists(SIM):
        print("no esta el simulador: make sim"); return 1
    tmp = tempfile.mkdtemp(prefix="iosmoke")
    mods = {}
    for nombre, fuente in (("IoCalc", FUENTE_CALC), ("IoChorro", FUENTE_CHORRO),
                           ("IoCorto", FUENTE_CORTO),
                           ("IoGui", FUENTE_GUI)):
        bp = os.path.join(tmp, nombre + ".bp")
        open(bp, "w", encoding="utf-8").write(fuente)
        m = compila(bp, tmp)
        if m is None:
            print("no se pudo compilar " + nombre + " (java/frontend): me salto la prueba")
            return 0
        mods[nombre] = m
    flash = os.path.join(tmp, "flash.bin")
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash,
                             "--mem=1M", "--psram=8M", "--flash=4M"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        sock = None
        for _ in range(50):
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0); break
            except OSError: time.sleep(0.1)
        if sock is None:
            print("el simulador no acepto conexion"); return 1
        w = Wire(sock)
        w.call("HELLO")
        for nombre, ruta in mods.items():
            datos = open(ruta, "rb").read()
            r = w.call_bulk("PUT", datos, path="/app/" + nombre + ".mod")
            if r.get("type") != "PUT_REPLY":
                check(False, "PUT de " + nombre + ": " + json.dumps(r)[:90]); return 1
        # El caso del GUI importa Gui, que a su vez importa Json y Core: el simulador
        # arranca con el FS vacio, asi que hay que subirlas. Sin esto el RUN falla con
        # «falta el modulo Gui» y la prueba se saltaba creyendo que faltaba LVGL.
        for dep in ("Core", "Json", "Gui"):
            origen = os.path.join(REPO, "bpstdlib", dep + ".mod")
            if os.path.exists(origen):
                w.call_bulk("PUT", open(origen, "rb").read(), path="/lib/" + dep + ".mod")

        print("-- 1. KILL con el programa CALCULANDO (sin imprimir) --")
        kill_durante(w, "/app/IoCalc.mod", "calculo", 0.5)

        print("-- 2. KILL con el programa IMPRIMIENDO A CHORRO --")
        kill_durante(w, "/app/IoChorro.mod", "chorro", 0.5)

        # #462 - el KILL sobre `Gui.run()`. Sin LVGL en el simulador este caso no
        # ejerce el camino (el builtin sale por el #else y `Gui.run()` vuelve sola),
        # asi que se salta DICIENDOLO: una prueba que se salta en silencio es una
        # prueba que no existe.
        print("-- 3. KILL sobre un programa con GUI (#462: daba exit 0 OK) --")
        w.send("RUN", path="/app/IoGui.mod")
        r = w._line(timeout=10)
        arranco = r.get("type") == "RUN_REPLY"
        vio_gui = False
        t0 = time.time()
        salio_solo = None
        while arranco and time.time() - t0 < 5:
            m = w._line(timeout=5)
            if m.get("type") == "OUTPUT" and "gui lista" in m.get("data", ""): vio_gui = True; break
            if m.get("type") == "EXITED": break
        # #485 - LA GUARDA MIRABA LO QUE NO ERA. Buscaba "gui lista" para decidir
        # si el sim trae LVGL, y el sim HEADLESS tambien la imprime: la guarda no
        # saltaba nunca, el caso corria igual y fallaba SIEMPRE con `make
        # io-smoke` (que construye el sim sin LVGL). Un caso permanentemente rojo
        # ensena a ignorar el arnes, que es peor que no tenerlo.
        #
        # La senal de headless es otra y es inequivoca: sin LVGL, `Gui.run()` no
        # bloquea, asi que el programa TERMINA SOLO antes de que nadie lo mate.
        # Si el EXITED llega antes del KILL, aqui no hay nada que medir.
        if vio_gui:
            salio_solo = None
            t1 = time.time()
            while time.time() - t1 < 0.4:
                try:
                    m = w._line(timeout=0.4)
                except Exception:
                    break          # con LVGL no llega nada, que es lo esperado
                if m.get("type") == "EXITED": salio_solo = m; break
        if not vio_gui:
            print("        (saltada: el RUN no llego a levantar la GUI)")
        elif salio_solo is not None:
            print("        (SALTADA: el sim no trae LVGL, `Gui.run()` no bloquea y el")
            print("         programa termina solo. Para medir esto: make sim LVGL=1)")
        else:
            w.send("KILL")
            exited, t0 = None, time.time()
            while time.time() - t0 < 15:
                m = w._line(timeout=15)
                if m.get("type") == "EXITED": exited = m; break
            check(exited is not None and exited.get("status") == "KILLED",
                  "GUI: un KILL sobre Gui.run() dice KILLED (" +
                  str(exited and exited.get("status")) + ")")

        print("-- 4. UN mensaje OUTPUT por LINEA, y la salida entera --")
        w.send("RUN", path="/app/IoCorto.mod")
        r = w._line(timeout=10)
        check(r.get("type") == "RUN_REPLY", "el RUN corto arranco")
        msgs, exited, t0 = [], None, time.time()
        while time.time() - t0 < 20:
            m = w._line(timeout=20)
            if m.get("type") == "OUTPUT": msgs.append(m.get("data", ""))
            elif m.get("type") == "EXITED": exited = m; break
        texto = "".join(msgs)
        # OJO: `print "a ", i, " b"` de BasicPlus deja el espacio de cada literal Y
        # el suyo alrededor del entero -> "linea  0  con ...". El texto esperado es el
        # del LENGUAJE, no el que uno supone: y es justo lo que NO cambia al mover la
        # salida de hilo, que es lo que esta prueba defiende.
        esperado = "".join("linea  %d  con varios argumentos\n" % i for i in range(5)) + "fin\n"
        check(exited is not None and exited.get("status") == "OK", "EXITED OK")
        check(texto == esperado,
              "la salida es EXACTA (%d B; ni una linea perdida al parar)" % len(texto))
        check(len(msgs) == 6,
              "6 mensajes OUTPUT para 6 lineas — uno por linea, no por argumento (fueron %d)" % len(msgs))
        if len(msgs) != 6 or texto != esperado:
            print("        mensajes: " + json.dumps(msgs)[:300])
    finally:
        try: sock.close()
        except Exception: pass
        proc.terminate()
        try: proc.wait(timeout=5)
        except Exception: proc.kill()
        shutil.rmtree(tmp, ignore_errors=True)
    print("[status=%s]" % ("OK" if fails == 0 else "FAIL(%d)" % fails))
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
