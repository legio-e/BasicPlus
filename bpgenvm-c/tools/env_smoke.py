# -*- coding: utf-8 -*-
"""env_smoke.py — V7/H10 #626: Machine.getEnv y Rtc.Clock.isSet() contra el micro simulado, con su ENV.

El programa ve el ENV de la FLASH —V7/H10, el board_mgr común, D1 (Eduardo, 3-oct: «deberían de leerse siempre de la
flash»)—: un ENV_SET se ve en el mismo arranque, también el SEGUNDO seguido (en la Pico, el primero no se veía: #638).
Reiniciar es parar el simulador y lanzarlo otra vez sobre la MISMA flash (el fichero A/B del ENV y el FS):

  1. sin la clave: getEnv da "" e isSet() dice «en hora» aunque el reloj marque 1970 (sin fecha no hay regla). Dos
     ENV_SET seguidos —imagen.fecha=1 y luego 1790899200 (el 2-oct-2026)— y el programa ve cada uno sin reiniciar.
  2. tras reiniciar, la fecha sigue: a 1970, fuera de hora; medio día antes, dentro del margen, en hora; un día
     después, en hora. Y se escribe una que no es una fecha: imagen.fecha=abc.
  3. con «abc», isSet() LANZA, con el valor en el mensaje (errores sí, silenciosos no).
  4. con la flash que no escribe (`--flash-falla`, D2): el ENV_SET contesta FLASH_ERROR y el valor de antes sigue —en
     el ENV_GET y en el programa—; y 5, relanzado sin la opción, también: no se escribió nada (#636).

El simulador no anota la fecha de su imagen al arrancar —no sube por el arranque común (Boot 2)—: la pone esta
prueba. La regla de «una vez» la prueba test_boot2 (make test-boot2), y en placa, la tanda (RtcEnHora).

    python tools/env_smoke.py            (desde bpgenvm-c; BPVM_SIM= para otro simulador)
"""
import os, sys, socket, subprocess, tempfile, time, shutil

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import Wire, SIM, STDLIB_PACK, JAR   # noqa: E402 — el mismo cable y el mismo compilador

STDLIB = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "bpstdlib"))

PORT = 5126
fails = 0


def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond:
        fails += 1


FUENTE = """module EnHora
  import Machine
  import Rtc

  function dice(clk: Rtc.Clock, que: string)
    if clk.isSet() then
      print que + ": en hora"
    else
      print que + ": fuera de hora"
    endif
  end dice

  function Main()
    var clk: Rtc.Clock := Rtc.Clock()
    print "fecha=[" + Machine.getEnv("imagen.fecha") + "] nada=[" + Machine.getEnv("no.esta") + "]"
    clk.setEpochSec(1000)
    dice(clk, "1970")
    clk.setEpochSec(1790899200 - 43200)
    dice(clk, "medio dia antes")
    clk.setEpochSec(1790899200 + 86400)
    dice(clk, "un dia despues")
  end Main
end EnHora
"""


def compila(dest):
    """El programa, contra la stdlib de bpstdlib/ (importa Machine y Rtc). None si no se puede."""
    if not os.path.exists(JAR) or shutil.which("java") is None:
        return None
    src = os.path.join(dest, "EnHora.bp")
    with open(src, "w", encoding="utf-8") as f:
        f.write(FUENTE)
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm", "--stdlibDir=" + STDLIB],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, "EnHora.mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None


def arranca(flash, extra=()):
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash,
                             "--mem=1M", "--flash=4M", "--pack=" + STDLIB_PACK] + list(extra),
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    sock = None
    for _ in range(50):
        try:
            sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0)
            break
        except OSError:
            time.sleep(0.1)
    if sock is None:
        proc.terminate()
        raise RuntimeError("el sim no acepta conexion")
    sock.settimeout(60.0)
    w = Wire(sock)
    w.call("HELLO", protoVersion=1, clientName="env_smoke")
    return proc, sock, w


def para(proc, sock):
    try:
        sock.close()
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except Exception:
            proc.kill()


def corre(w):
    r = w.call("RUN", path="/app/EnHora.mod")
    if r.get("type") != "RUN_REPLY":
        return "", {"status": "sin RUN: %s" % r}
    text, exited = w.drain_run()
    return text, (exited or {})


def main():
    if not os.path.exists(STDLIB_PACK):
        print("FAIL: no esta la libreria de packs (%s)" % STDLIB_PACK)
        return 1
    tmp = tempfile.mkdtemp(prefix="env_smoke_")
    flash = os.path.join(tmp, "sim.flash")
    print("=== V7/H10 #626: Machine.getEnv y Rtc.Clock.isSet() en el micro simulado ===")
    try:
        mod = compila(tmp)
        if mod is None:
            check(False, "compilar EnHora: no encuentro java o basicplus-frontend.jar")
            print("[status=FAIL]")
            return 1
        with open(mod, "rb") as f:
            data = f.read()

        # 1. sin la clave
        proc, sock, w = arranca(flash)
        try:
            check(w.call("FORMAT", confirm="YES").get("type") == "FORMAT_REPLY", "1: FORMAT")
            check(w.call_bulk("PUT", data, path="/app/EnHora.mod").get("type") == "PUT_REPLY", "1: PUT del programa")
            text, ex = corre(w)
            check(ex.get("status") == "OK", "1: RUN acaba bien (%s)" % ex.get("status"))
            check("fecha=[] nada=[]" in text, "1: sin la clave, getEnv da \"\" (y una que no esta, tambien)")
            check("1970: en hora" in text and "un dia despues: en hora" in text,
                  "1: sin fecha no hay regla: en hora aunque marque 1970")
            r = w.call("ENV_SET", key="imagen.fecha", value="1")
            check(r.get("type") == "ENV_SET_REPLY", "1: ENV_SET imagen.fecha=1")
            text, ex = corre(w)
            check("fecha=[1]" in text, "1: en el mismo arranque el programa YA la ve (D1: el ENV de la flash)")
            r = w.call("ENV_SET", key="imagen.fecha", value="1790899200")
            check(r.get("type") == "ENV_SET_REPLY", "1: el segundo ENV_SET, imagen.fecha=1790899200")
            text, ex = corre(w)
            check("fecha=[1790899200]" in text, "1: y tambien el segundo, sin reiniciar (#638)")
        finally:
            para(proc, sock)

        # 2. con la fecha
        proc, sock, w = arranca(flash)
        try:
            text, ex = corre(w)
            check(ex.get("status") == "OK", "2: RUN acaba bien (%s)" % ex.get("status"))
            check("fecha=[1790899200]" in text, "2: tras reiniciar, getEnv la ve")
            check("1970: fuera de hora" in text, "2: a 1970, fuera de hora")
            check("medio dia antes: en hora" in text, "2: medio dia antes de la fecha, dentro del margen: en hora")
            check("un dia despues: en hora" in text, "2: un dia despues: en hora")
            r = w.call("ENV_SET", key="imagen.fecha", value="abc")
            check(r.get("type") == "ENV_SET_REPLY", "2: ENV_SET imagen.fecha=abc")
        finally:
            para(proc, sock)

        # 3. con una fecha que no lo es
        proc, sock, w = arranca(flash)
        try:
            text, ex = corre(w)
            todo = text + " " + str(ex)
            check(ex.get("status") not in (None, "OK"), "3: el RUN no acaba bien (%s)" % ex.get("status"))
            check("imagen.fecha=abc" in todo and "que no son segundos desde 1970" in todo,
                  "3: isSet() lanza, y el mensaje dice el valor")
        finally:
            para(proc, sock)

        # 4. la flash no escribe (D2, #636): FLASH_ERROR, y el valor de antes
        proc, sock, w = arranca(flash, ["--flash-falla"])
        try:
            r = w.call("ENV_SET", key="imagen.fecha", value="1790899200")
            check(r.get("type") == "ERROR" and r.get("code") == "FLASH_ERROR",
                  "4: ENV_SET con la flash que no escribe: ERROR FLASH_ERROR (%s)" % r)
            r = w.call("ENV_GET", key="imagen.fecha")
            check(r.get("value") == "abc", "4: el ENV_GET sigue con el valor de antes (%s)" % r.get("value"))
            text, ex = corre(w)
            check("fecha=[abc]" in text, "4: y el programa tambien")
        finally:
            para(proc, sock)

        # 5. relanzado sin la opción: no se escribió nada
        proc, sock, w = arranca(flash)
        try:
            r = w.call("ENV_GET", key="imagen.fecha")
            check(r.get("value") == "abc", "5: relanzado, el ENV sigue con el valor de antes (%s)" % r.get("value"))
        finally:
            para(proc, sock)
    except Exception as e:
        check(False, "excepcion: %s" % e)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("[status=%s]" % ("OK" if fails == 0 else "FAIL"))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
