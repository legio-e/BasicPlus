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
                           ("RunDormido", DORMIDO)):
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

    shutil.rmtree(tmp, ignore_errors=True)
    print("[status=OK]" if fails == 0 else "[status=FAIL: %d]" % fails)
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
