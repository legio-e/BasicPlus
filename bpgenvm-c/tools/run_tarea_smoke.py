#!/usr/bin/env python3
"""run_tarea_smoke.py — V7/H12, etapas 0 y 1 (4-oct): LA TAREA `run`, en el micro simulado.

Desde H12, el programa corre en una tarea propia y permanente (`run`) y la del REPL hace de `io` durante el RUN
(`V7_IDEAS` §H12 «Etapas 0 y 1 en la Pico 2»; el protocolo, en src/bpvm_repl.c). Lo que se puede romper es el ORDEN
entre las dos: que la última línea salga antes del EXITED, que la VM no se suelte con el programa dentro, que un RUN
que llega nada más acabar el anterior encuentre a `run` esperando. Aquí se prueba eso, como pide el plan (las reglas
de todas las etapas): provocando la carrera, ensanchando la ventana y saboteando el arreglo.

  A. mil RUN seguidos de un programa que imprime veinte líneas y acaba: todas, en orden, el EXITED detrás y su sesión;
  B. la salida TARDÍA: una línea, medio segundo sin escribir y la última: las dos, y el EXITED después;
  C. un KILL justo cuando el programa acaba, barriendo el instante: un EXITED por RUN, una respuesta por KILL (su
     KILL_REPLY o NO_SESSION, si ya había acabado) y el micro sigue contestando;
  D. lo mismo con la ventana ENSANCHADA (`--ensayo-hecho=150`: `run` espera entre el final del programa y su aviso);
  E. el SABOTAJE (`--sabotaje-cierre`: `io` cierra la cola sin esperar a `run`): la salida tardía se pierde, y B tiene
     que cazarlo —si con el sabotaje B sale verde, la prueba no vale—;
  F. `--run-viejo` (el interruptor: el RUN de antes de H12) sigue funcionando;
  G. un programa que NO imprime acaba tan deprisa como con `--run-viejo`: al acabar, `run` despierta a `io` (si no, `io`
     sólo lo ve al vencer su espera de 5 ms —en Windows, ~15—: saboteado el 4-oct, 18 ms por RUN en vez de 1,2).

Desde la etapa 2a.2 (4-oct; `V7_IDEAS` §H12 «Etapa 2a.2») `run` espera en LA COLA y el RUN cuelga ahí el programa: A-E
lo prueban así. Y además:

  H. EL KILL DESPIERTA: un programa dormido una hora se para enseguida (con `io`, el planificador ya no se despierta cada
     50 ms a mirar: lo despierta el aviso del KILL);
  I. su SABOTAJE (`--sabotaje-despertar`: el KILL no avisa a la cola): el programa dormido no se para, y H tiene que
     cazarlo;
  J. el interruptor de la 2a.2 (`--run-etapa1`: `run` espera el «trabajo», como en la etapa 1): A, B, C y H.

Y desde la etapa 2b (5-oct), DOS runs permanentes (`--runs=2`, como `vm.runs=2` en el ENV de una placa):

  K. A, B, C y H con dos runs, y un programa de dos hilos que se reparten un contador bajo un Mutex (los dos runs
     trabajan a la vez): el total exacto en cada RUN.

Y desde V8 (#812, 13-nov), el aviso que llega antes de que `io` espere:

  L. con `io` tarde a su espera (`--ensayo-io=20`: el programa acaba y avisa antes), el aviso no se pierde: deja su
     marca y `io` no espera su plazo. Saboteado (`--sabotaje-aviso`: sin la marca, como antes), lo espera en cada RUN,
     y la prueba lo caza comparando los dos.

Y desde V8 (#840, 18-nov; la tabla de la robustez), el RESET aunque el programa no pare:

  M. dos KILL seguidos (dos clics en Stop): un EXITED y una respuesta por KILL (tras el primero, la placa sigue leyendo);
     un RESET a un programa dormido, sin más: se para (EXITED KILLED) y el RESET_REPLY llega enseguida, como siempre;
     con `--sabotaje-sinparar` (el KILL no llega a la VM: el programa no para, como uno metido en C que no vuelve), un
     KILL no lo para y un RESET después se sirve igual, a los ~2 s (el plazo); y saboteado el plazo
     (`--sabotaje-sinplazo`: el RESET espera al programa, como antes de #840), no se sirve nunca, y la prueba lo caza.

    python tools/run_tarea_smoke.py [--vueltas=N]      (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import os, shutil, sys, tempfile, time

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_smoke                                            # noqa: E402 — el mismo cable y el mismo arranque
from run_smoke import Cable, compila                        # noqa: E402
from sim_smoke import SIM, STDLIB_PACK, JAR                 # noqa: E402

import socket, subprocess

PORT = 5131
fails = 0
VUELTAS = 1000
for a in sys.argv[1:]:
    if a.startswith("--vueltas="):
        VUELTAS = int(a.split("=", 1)[1])


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg, flush=True)
    if not cond:
        fails += 1
    return cond


VEINTE = """module RunVeinte
  function Main()
    var i: integer := 1
    while i <= 20 do
      print "linea " + i
      i := i + 1
    endwh
    print "fin"
  end Main
end RunVeinte
"""

TARDE = """module RunTarde
  function Main()
    print "a"
    sleep(500)
    print "fin"
  end Main
end RunTarde
"""

# Calcula ~T ms sin escribir y acaba: el KILL se barre alrededor de su final.
CORTO = """module RunCorto
  function Main()
    var t0: integer := now()
    var i: integer := 0
    while now() - t0 < 60 do
      i := i + 1
    endwh
    print "fin"
  end Main
end RunCorto
"""

# No imprime nada: el caso en que `io` no tiene una última línea que lo despierte.
VACIO = """module RunVacio
  function Main()
    var i: integer := 1
  end Main
end RunVacio
"""

# K (2b): dos hilos que suman bajo un Mutex; con dos runs, cada uno en el suyo. El total, exacto.
DOS_HILOS = """module RunDosHilos
  class Sumador extends Thread
    var m: Mutex
    var veces: integer
    var hechos: integer
    public function Sumador(mx: Mutex, n: integer)
      super(4096)
      this.m := mx
      this.veces := n
      this.hechos := 0
    end Sumador
    public function run()
      var i: integer := 0
      while i < this.veces do
        this.m.lock()
        this.hechos := this.hechos + 1
        this.m.unlock()
        i := i + 1
      endwh
    end run
    public function total(): integer
      return this.hechos
    end total
  end Sumador

  function Main()
    var mtx: Mutex := Mutex()
    var a: Sumador := Sumador(mtx, 3000)
    var b: Sumador := Sumador(mtx, 3000)
    a.start()
    b.start()
    a.join()
    b.join()
    print "total = " + (a.total() + b.total())
  end Main
end RunDosHilos
"""

# Se duerme una hora: el KILL tiene que despertarlo (H).
DORMIDO = """module RunDormido
  function Main()
    print "durmiendo"
    sleep(3600000)
    print "nunca llega"
  end Main
end RunDormido
"""

ESPERADO_VEINTE = "".join("linea %d\n" % i for i in range(1, 21)) + "fin\n"


def arranca(flash, extra):
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash, "--mem=1M", "--flash=4M",
                             "--pack=" + STDLIB_PACK] + list(extra),
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    for _ in range(80):
        try:
            sock = socket.create_connection(("127.0.0.1", PORT), timeout=2.0)
            sock.settimeout(30.0)
            c = Cable(sock)
            c.call("HELLO", protoVersion=1, clientName="run_tarea_smoke")
            return proc, sock, c
        except OSError:
            time.sleep(0.1)
    proc.kill()
    raise RuntimeError("el sim no acepta conexión")


def para(proc, sock):
    try:
        sock.close()
    finally:
        proc.terminate()
        try: proc.wait(timeout=5)
        except Exception: proc.kill()


def subir(c, mods):
    for nombre, mod in mods.items():
        with open(mod, "rb") as f:
            if c.call_bulk("PUT", f.read(), path="/app/%s.mod" % nombre).get("type") != "PUT_REPLY":
                return False
    return True


def correr(c, nombre):
    """(salida en texto, exited, sesión del RUN_REPLY)"""
    r, tramas, ex = c.run("/app/%s.mod" % nombre, timeout=30.0)
    return b"".join(tramas).decode("utf-8", "replace"), ex, r.get("session")


def caso_a(c, vueltas, etiqueta):
    malos = []
    t0 = time.time()
    for k in range(vueltas):
        texto, ex, ses = correr(c, "RunVeinte")
        if ex is None or ex.get("status") != "OK" or ex.get("session") != ses or texto != ESPERADO_VEINTE:
            malos.append((k, ex and ex.get("status"), texto[-40:]))
            if len(malos) >= 3:
                break
    check(not malos, "%sA. %d RUN seguidos: las 21 líneas en orden, el EXITED detrás y con su sesión (%.1f s)%s"
          % (etiqueta, vueltas, time.time() - t0, "" if not malos else " — mal: %r" % malos))


def caso_b(c, etiqueta, se_espera_bien=True):
    texto, ex, _ = correr(c, "RunTarde")
    bien = ex is not None and ex.get("status") == "OK" and texto == "a\nfin\n"
    if se_espera_bien:
        check(bien, "%sB. la salida tardía: «a», medio segundo y «fin»; las dos y el EXITED detrás (sale %r, %s)"
              % (etiqueta, texto, ex and ex.get("status")))
    return bien, texto


def caso_c(c, etiqueta):
    """El KILL barriendo el final de RunCorto (~60 ms de cálculo)."""
    problemas = []
    vistos = {"KILLED": 0, "OK": 0}
    for retraso_ms in list(range(30, 100, 5)) * 2:
        r = c.call("RUN", path="/app/RunCorto.mod")
        if r.get("type") != "RUN_REPLY":
            problemas.append("RUN %r" % r.get("type"))
            continue
        time.sleep(retraso_ms / 1000.0)
        c.id += 1
        kid = c.id
        c.s.sendall(('{"type":"KILL","id":%d}\n' % kid).encode())
        exiteds, resp_kill, t0 = [], [], time.time()
        # lo que llegue hasta tener el EXITED y la respuesta del KILL (en cualquier orden: si el programa ya había
        # acabado, el KILL llega con el micro en reposo y contesta NO_SESSION después del EXITED)
        while time.time() - t0 < 10 and (not exiteds or not resp_kill):
            m = c.linea()
            if m.get("type") == "EXITED":
                exiteds.append(m)
            elif m.get("id") == kid:
                resp_kill.append(m)
        if len(exiteds) != 1 or len(resp_kill) != 1:
            problemas.append("retraso %d: %d EXITED, %d respuestas al KILL" % (retraso_ms, len(exiteds), len(resp_kill)))
            continue
        st = exiteds[0].get("status")
        rk = resp_kill[0]
        coherente = (st == "KILLED" and rk.get("type") == "KILL_REPLY") or \
                    (st == "OK" and (rk.get("type") == "KILL_REPLY" or rk.get("code") == "NO_SESSION"))
        if not coherente:
            problemas.append("retraso %d: EXITED %s con %s %s" % (retraso_ms, st, rk.get("type"), rk.get("code")))
        vistos[st] = vistos.get(st, 0) + 1
    hello = c.call("HELLO", protoVersion=1, clientName="run_tarea_smoke")
    check(not problemas and hello.get("type") == "HELLO_REPLY",
          "%sC. KILL barriendo el final del programa: un EXITED por RUN y una respuesta por KILL (%d parados, %d acabados "
          "solos), y el micro sigue contestando%s" % (etiqueta, vistos.get("KILLED", 0), vistos.get("OK", 0),
                                                      "" if not problemas else " — mal: %r" % problemas[:3]))
    check(vistos.get("KILLED", 0) > 0 and vistos.get("OK", 0) > 0,
          "%sC. y el barrido cae a los dos lados del final (si no, no ha barrido el instante)" % etiqueta)


def caso_h(c, etiqueta, se_espera_bien=True):
    """El KILL a un programa dormido una hora (2a.2: el aviso a la cola lo despierta). (bien, ms hasta el EXITED)."""
    r = c.call("RUN", path="/app/RunDormido.mod")
    if r.get("type") != "RUN_REPLY":
        check(False, "%sH. RUN de RunDormido: %r" % (etiqueta, r.get("type")))
        return False, None
    t0, visto = time.time(), False
    while time.time() - t0 < 10 and not visto:
        m = c.linea()
        if m.get("type") == "OUTPUT" and "durmiendo" in m.get("data", ""):
            visto = True
    time.sleep(0.3)                       # que esté dormido de verdad
    c.id += 1
    kid = c.id
    tk = time.time()
    c.s.sendall(('{"type":"KILL","id":%d}\n' % kid).encode())
    ex, resp, ms = None, None, None
    c.s.settimeout(3.0)
    try:
        while (ex is None or resp is None) and time.time() - tk < 3.0:
            m = c.linea()
            if m.get("type") == "EXITED":
                ex, ms = m, (time.time() - tk) * 1000.0
            elif m.get("id") == kid:
                resp = m
    except (socket.timeout, OSError):
        pass
    finally:
        c.s.settimeout(30.0)
    bien = visto and ex is not None and ex.get("status") == "KILLED" and resp is not None \
        and resp.get("type") == "KILL_REPLY" and ms is not None and ms < 1000.0
    if se_espera_bien:
        check(bien, "%sH. el KILL despierta a un programa dormido una hora: el EXITED KILLED en %s ms, con su KILL_REPLY"
              % (etiqueta, "%.0f" % ms if ms is not None else "—"))
    return bien, ms


def espera_durmiendo(c):
    t0 = time.time()
    while time.time() - t0 < 10:
        m = c.linea()
        if m.get("type") == "OUTPUT" and "durmiendo" in m.get("data", ""):
            return True
    return False


def manda(c, tipo):
    c.id += 1
    c.s.sendall(('{"type":"%s","id":%d}\n' % (tipo, c.id)).encode())
    return c.id


def lo_que_llegue(c, segs, hasta):
    """Las líneas que lleguen en `segs` (o hasta que `hasta(m)` diga basta). La conexión puede cerrarse (el RESET del
    simulado la corta): lo recibido vale."""
    vistos, t0 = [], time.time()
    c.s.settimeout(0.5)
    try:
        while time.time() - t0 < segs:
            try:
                m = c.linea()
            except socket.timeout:
                continue
            vistos.append((time.time() - t0, m))
            if hasta(m):
                break
    except (OSError, EOFError):                   # la conexión, cortada (el RESET del simulado): lo recibido vale
        pass
    finally:
        c.s.settimeout(30.0)
    return vistos


def caso_m_dos_kill(c):
    """V8 #840: dos KILL seguidos (dos clics en Stop): tras el primero la placa sigue leyendo el cable, y el segundo
    también tiene su respuesta (su KILL_REPLY con el primero, o NO_SESSION si llega con el programa ya parado)."""
    if c.call("RUN", path="/app/RunDormido.mod").get("type") != "RUN_REPLY" or not espera_durmiendo(c):
        return check(False, "M. RUN de RunDormido (dos KILL)")
    time.sleep(0.2)
    k1 = manda(c, "KILL")
    k2 = manda(c, "KILL")
    resp = {}
    vistos = lo_que_llegue(c, 3.0, lambda m: False)
    for _, m in vistos:
        if m.get("id") in (k1, k2):
            resp.setdefault(m.get("id"), []).append(m.get("type") if m.get("type") != "ERROR" else m.get("code"))
    ex = [m for _, m in vistos if m.get("type") == "EXITED"]
    hello = c.call("HELLO", protoVersion=1, clientName="run_tarea_smoke")
    bien = len(ex) == 1 and ex[0].get("status") == "KILLED" and len(resp.get(k1, [])) == 1 \
        and len(resp.get(k2, [])) == 1 and resp[k1][0] == "KILL_REPLY" \
        and resp[k2][0] in ("KILL_REPLY", "NO_SESSION") and hello.get("type") == "HELLO_REPLY"
    return check(bien, "M. dos KILL seguidos: un EXITED KILLED y una respuesta por KILL (%s y %s), y el micro sigue "
                       "contestando" % (resp.get(k1), resp.get(k2)))


def caso_m0(c):
    """V8 #840: un RESET a un programa dormido, sin sabotaje: el EXITED KILLED y el RESET_REPLY, enseguida y en orden."""
    if c.call("RUN", path="/app/RunDormido.mod").get("type") != "RUN_REPLY" or not espera_durmiendo(c):
        return check(False, "M. RUN de RunDormido")
    time.sleep(0.2)
    rid = manda(c, "RESET")
    vistos = lo_que_llegue(c, 3.0, lambda m: m.get("type") == "RESET_REPLY")
    ex = [(t, m) for t, m in vistos if m.get("type") == "EXITED"]
    rr = [(t, m) for t, m in vistos if m.get("type") == "RESET_REPLY" and m.get("id") == rid]
    return check(len(ex) == 1 and ex[0][1].get("status") == "KILLED" and len(rr) == 1 and rr[0][0] < 1.0
                 and ex[0][0] <= rr[0][0],
                 "M. un RESET a un programa dormido: EXITED KILLED y el RESET_REPLY a los %s s, detrás (como siempre: el "
                 "plazo no lo retrasa)" % ("%.2f" % rr[0][0] if rr else "—"))


def caso_m(c):
    """V8 #840: con el programa que no para (--sabotaje-sinparar), un KILL y un RESET. (sin EXITED tras el KILL,
    segundos hasta el RESET_REPLY o None)."""
    if c.call("RUN", path="/app/RunDormido.mod").get("type") != "RUN_REPLY" or not espera_durmiendo(c):
        check(False, "M. RUN de RunDormido")
        return False, None
    manda(c, "KILL")
    tras_kill = lo_que_llegue(c, 1.0, lambda m: m.get("type") == "EXITED")
    sin_exited = not any(m.get("type") == "EXITED" for _, m in tras_kill)
    rid = manda(c, "RESET")
    vistos = lo_que_llegue(c, 5.0, lambda m: m.get("type") == "RESET_REPLY")
    rr = [t for t, m in vistos if m.get("type") == "RESET_REPLY" and m.get("id") == rid]
    return sin_exited, (rr[0] if rr else None)


def cuanto_vacio(c, vueltas=200):
    """Segundos de `vueltas` RUN de RunVacio (y cuántos acabaron mal)."""
    t0, malos = time.time(), 0
    for _ in range(vueltas):
        texto, ex, ses = correr(c, "RunVacio")
        if ex is None or ex.get("status") != "OK" or texto:
            malos += 1
    return time.time() - t0, malos


def main():
    if not os.path.exists(JAR) or shutil.which("java") is None or not os.path.exists(STDLIB_PACK):
        print("FAIL: hacen falta el compilador (%s) y la librería de packs (%s)" % (JAR, STDLIB_PACK))
        print("[status=FAIL]")
        return 1
    tmp = tempfile.mkdtemp(prefix="run_tarea_")
    print("=== run_tarea_smoke (V7/H12, etapas 0, 1 y 2a.2): la tarea `run` del micro simulado ===", flush=True)
    mods = {}
    for nombre, fuente in (("RunVeinte", VEINTE), ("RunTarde", TARDE), ("RunCorto", CORTO), ("RunVacio", VACIO),
                           ("RunDormido", DORMIDO), ("RunDosHilos", DOS_HILOS)):
        mods[nombre] = compila(tmp, nombre, fuente)
        check(mods[nombre] is not None, "compila %s" % nombre)
    if None in mods.values():
        print("[status=FAIL: %d]" % fails)
        return 1

    # A, B y C: la tarea `run`, tal cual
    proc, sock, c = arranca(os.path.join(tmp, "a.flash"), [])
    try:
        check(subir(c, mods), "PUT de los programas")
        caso_a(c, VUELTAS, "")
        caso_b(c, "")
        caso_c(c, "")
        caso_h(c, "")
        t_tarea, malos_tarea = cuanto_vacio(c)
    finally:
        para(proc, sock)

    # D: la ventana ensanchada
    proc, sock, c = arranca(os.path.join(tmp, "d.flash"), ["--ensayo-hecho=150"])
    try:
        check(subir(c, mods), "D. PUT (con --ensayo-hecho=150)")
        caso_a(c, max(20, VUELTAS // 20), "D (ventana ensanchada) ")
        caso_b(c, "D (ventana ensanchada) ")
        caso_c(c, "D (ventana ensanchada) ")
    finally:
        para(proc, sock)

    # E: el sabotaje —B tiene que salir mal—
    proc, sock, c = arranca(os.path.join(tmp, "e.flash"), ["--sabotaje-cierre"])
    try:
        check(subir(c, mods), "E. PUT (con --sabotaje-cierre)")
        bien, texto = caso_b(c, "E (sabotaje) ", se_espera_bien=False)
        check(not bien, "E. el sabotaje (io cierra la cola sin esperar a run) SE CAZA: la salida tardía se pierde (sale %r)"
              % texto)
    finally:
        para(proc, sock)

    # F: el interruptor
    proc, sock, c = arranca(os.path.join(tmp, "f.flash"), ["--run-viejo"])
    try:
        check(subir(c, mods), "F. PUT (con --run-viejo)")
        caso_a(c, 20, "F (--run-viejo) ")
        caso_b(c, "F (--run-viejo) ")
        t_viejo, malos_viejo = cuanto_vacio(c)
    finally:
        para(proc, sock)

    # G: lo que tarda en acabar un programa que no imprime, contra el RUN de antes
    check(malos_tarea == 0 and malos_viejo == 0 and t_tarea <= 2 * t_viejo + 0.3,
          "G. 200 RUN de un programa que no imprime: %.2f s con la tarea run, %.2f s con --run-viejo (si `run` no "
          "despierta a `io` al acabar, cada uno espera los 5 ms de `io`)" % (t_tarea, t_viejo))

    # I: el sabotaje del aviso —H tiene que salir mal—
    proc, sock, c = arranca(os.path.join(tmp, "i.flash"), ["--sabotaje-despertar"])
    try:
        check(subir(c, mods), "I. PUT (con --sabotaje-despertar)")
        bien, ms = caso_h(c, "I (sabotaje) ", se_espera_bien=False)
        check(not bien, "I. el sabotaje (el KILL no avisa a la cola) SE CAZA: el programa dormido no se para (%s)"
              % ("EXITED en %.0f ms" % ms if ms is not None else "sin EXITED en 3 s"))
    finally:
        para(proc, sock)

    # J: el interruptor de la 2a.2 —`run` espera el «trabajo», como en la etapa 1—
    proc, sock, c = arranca(os.path.join(tmp, "j.flash"), ["--run-etapa1"])
    try:
        check(subir(c, mods), "J. PUT (con --run-etapa1)")
        caso_a(c, 20, "J (--run-etapa1) ")
        caso_b(c, "J (--run-etapa1) ")
        caso_c(c, "J (--run-etapa1) ")
        caso_h(c, "J (--run-etapa1) ")
    finally:
        para(proc, sock)

    # K: dos runs permanentes (2b)
    proc, sock, c = arranca(os.path.join(tmp, "k.flash"), ["--runs=2"])
    try:
        check(subir(c, mods), "K. PUT (con --runs=2)")
        caso_a(c, max(50, VUELTAS // 5), "K (dos runs) ")
        caso_b(c, "K (dos runs) ")
        caso_c(c, "K (dos runs) ")
        caso_h(c, "K (dos runs) ")
        malos = []
        for k in range(50):
            texto, ex, _ = correr(c, "RunDosHilos")
            if ex is None or ex.get("status") != "OK" or texto != "total = 6000\n":
                malos.append((k, ex and ex.get("status"), texto[-40:]))
                if len(malos) >= 3:
                    break
        check(not malos, "K (dos runs) dos hilos bajo un Mutex, 50 RUN: el total exacto (6000) en todos%s"
              % ("" if not malos else " — mal: %r" % malos))
    finally:
        para(proc, sock)

    # L (V8 #812): el aviso de `run` no se pierde aunque `io` llegue tarde a su espera. Con --ensayo-io, `io` se
    # entretiene antes de esperar y el programa (que no imprime) acaba y avisa antes; el aviso deja su marca, y `io`
    # no espera su plazo. Saboteado (--sabotaje-aviso: el aviso sin la marca, como antes de #812), lo espera entero en
    # cada RUN: 5 ms en cualquier sistema, hasta 15,6 en Windows (medido el 13-nov: 34 ms por RUN con la marca, 52,6
    # sin ella). Se comparan los dos con la misma espera de `io`, así que la del sistema se va en la resta.
    t_l = {}
    for nombre, extra in (("con la marca", ["--ensayo-io=20"]), ("saboteado", ["--ensayo-io=20", "--sabotaje-aviso"])):
        proc, sock, c = arranca(os.path.join(tmp, "l.flash"), extra)
        try:
            check(subir(c, mods), "L. PUT (%s)" % " ".join(extra))
            t_l[nombre] = cuanto_vacio(c, 30)
        finally:
            para(proc, sock)
    de_mas = (t_l["saboteado"][0] - t_l["con la marca"][0]) * 1000.0 / 30
    check(t_l["con la marca"][1] == 0 and t_l["saboteado"][1] == 0 and de_mas >= 3.0,
          "L. con `io` tarde a su espera (20 ms), el aviso de `run` no se pierde: 30 RUN, %.1f ms cada uno con la marca "
          "y %.1f saboteado (el aviso sin marca): %.1f ms de más, la espera de `io` (tiene que ser 3 o más)"
          % (t_l["con la marca"][0] * 1000.0 / 30, t_l["saboteado"][0] * 1000.0 / 30, de_mas))

    # M (V8 #840): el RESET aunque el programa no pare. Cada sim acaba con su RESET (el del simulado corta la conexión).
    proc, sock, c = arranca(os.path.join(tmp, "m0.flash"), [])
    try:
        check(subir(c, mods), "M. PUT")
        caso_m_dos_kill(c)
        caso_m0(c)
    finally:
        para(proc, sock)
    proc, sock, c = arranca(os.path.join(tmp, "m.flash"), ["--sabotaje-sinparar"])
    try:
        check(subir(c, mods), "M. PUT (con --sabotaje-sinparar)")
        sin_exited, seg = caso_m(c)
        check(sin_exited, "M. con --sabotaje-sinparar, el KILL no para al programa (sin EXITED en 1 s): el caso que se prueba")
        check(seg is not None and 1.8 <= seg <= 3.0,
              "M. y un RESET después se sirve igual, a los %s s (el plazo, 2): la placa vuelve sin manos"
              % ("%.2f" % seg if seg is not None else "—"))
    finally:
        para(proc, sock)
    proc, sock, c = arranca(os.path.join(tmp, "m2.flash"), ["--sabotaje-sinparar", "--sabotaje-sinplazo"])
    try:
        check(subir(c, mods), "M. PUT (con --sabotaje-sinplazo)")
        sin_exited, seg = caso_m(c)
        check(sin_exited and seg is None,
              "M. el sabotaje (el RESET espera al programa, como antes de #840) SE CAZA: sin RESET_REPLY en 5 s (%s)"
              % ("llegó a los %.2f s" % seg if seg is not None else "no llegó"))
    finally:
        para(proc, sock)

    shutil.rmtree(tmp, ignore_errors=True)
    print("[status=OK]" if fails == 0 else "[status=FAIL: %d]" % fails)
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
