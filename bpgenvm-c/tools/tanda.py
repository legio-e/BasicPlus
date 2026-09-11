# -*- coding: utf-8 -*-
"""tanda.py — T1 fase 1: las placas CONDUCIDAS. Una tanda de pruebas, POR PLACA.

USO
    python tools/tanda.py [--lista tools/tanda_prueba.json]
                          [--puertos auto|COM3,COM14] [--sim host:puerto]
                          [--prueba NOMBRE[,NOMBRE]] [--informe RUTA.md]
                          [--sin-oraculo]

    --puertos auto   enumera los puertos serie (salta COM1), hace HELLO con 3 s
                     de espera y se queda con los que contestan. Puede haber
                     varias placas a la vez.
    --sim host:pto   ademas (o en vez) de las placas, el micro simulado
                     (bpvm-sim) por TCP: es la prueba del propio arnes sin
                     hardware.
    --informe        Markdown que se ABRE EN MODO ANADIR: una seccion por
                     invocacion (fecha/hora). Al lado, un .json con lo medido.
    --sin-oraculo    no ejecuta el host (compila igual, porque el .mod que se
                     sube sale de ahi); el criterio 'identico' queda sin juez y
                     se dice.

QUE HACE, EN ORDEN
    1. Lee el manifiesto (lista de pruebas: fuente .bp, criterio, requisitos,
       recursos, artefactos) y comprueba que los .bp existen.
    2. ORACULO, una vez por prueba y en el PC: compila el .bp con el frontend en
       build/tanda/<nombre>/ (con la stdlib FRESCA copiada antes, como hace
       compat/compat.sh), ejecuta la VM-C host y guarda stdout + codigo de
       salida, normalizados como compat.sh filt().
    3. POR PLACA: KILL y esperar a que este en reposo; HELLO + INFO -> sello
       (imagen: arch/variant/cpuFreqHz/resetReason/uptimeMs/fs/heap); decide si
       tiene pantalla; por prueba: salta con motivo o sube el modulo (y lo que
       falte de la stdlib, comprobado COMO LO RESUELVE EL RUN), borra los
       artefactos que pudieran quedar de otra tanda, RUN, acumula OUTPUT de ESA
       sesion hasta EXITED o timeout (KILL), INFO (¿se reinicio?), baja
       artefactos, veredicto.
    4. Informe: sello + tabla por placa, subidos con CRC32, matriz pruebas x
       placas, SALTADA:/NECESITA OJOS: al final. Nunca un salto silencioso.
       Lo que no es verde lleva su evidencia: la salida que SI llego, las
       lineas NO-JSON del wire (panics, avisos) y la cola del stderr del host.

VEREDICTOS
    IDENTICO / NO-PETA (verdes) - DIFIERE / PETA - COLGADA (sin EXITED en el
    timeout) - ERROR (no se pudo medir: subida, transporte, dependencia
    sombreada, fallo de CARGA/ENLACE en la placa) - SIN ORACULO - SALTADA -
    SIN ARTEFACTO (verde pero sin la evidencia) - DUDOSO (verde pero el
    artefacto podia ser de otra tanda) - REINICIO (la placa se reinicio
    durante la prueba: uptimeMs decrecio).
    Dos fallos solo son IDENTICO si fallan IGUAL: mismo exitCode que el rc
    del host (misma tabla, bpvm_exit_code). Un LINK_ERROR o un «load:» /
    «falta el modulo» de la placa es fallo del ARNES, nunca paridad.

POR QUE EL BUCLE ES POR PLACA (decision de Eduardo, 11-sep)
    Todas las pruebas sobre una placa, luego la siguiente. Eduardo va
    conectando y desconectando placas entre tandas (no caben todas a la vez
    en la mesa ni en los USB), asi que el informe se ANADE: cada tanda es una
    seccion y la matriz final se lee tanda a tanda. Ademas, el estado que hay
    que preparar (dependencias en /app) se prepara una vez por placa, no una
    vez por prueba.

QUE PRUEBA EL INFORME (para que no sea un «OK» de fe)
    - el SELLO de la placa (boardName, uniqueId, serverBuild, transporte, puerto)
    - el CRC32 y tamano de cada fichero subido (el modulo y sus dependencias)
    - ejecutadas vs listadas, y cada salto con su porque
    - el oraculo con el que se comparo: jar, exe, sabor (build/.flavor-*) y
      sus horas — verificar el ARTEFACTO, no el log.

TRAMPAS CONOCIDAS
    - BUSY: si la placa esta ejecutando algo (un Run del IDE que nadie paro,
      o una prueba COLGADA cuyo KILL no dio EXITED), contesta ERROR/BUSY a
      todo menos HELLO/KILL. Al conectar se manda un KILL y se ESPERA a que
      este en reposo (EXITED, o INFO sin BUSY, hasta 10 s; el sello dice si
      habia algo corriendo). Un BUSY al subir o al RUN: KILL, esperar el
      EXITED hasta 10 s y UN reintento; si persiste, la PLACA se aborta («la
      placa sigue ejecutando X tras KILL») y las pruebas restantes quedan
      «no se llego a ejecutar». Un ERROR con el id de la peticion corta la
      espera en seco (antes eran 20 s por PUT).
    - SESIONES: RUN_REPLY, OUTPUT y EXITED llevan `session`. Un EXITED
      rezagado (el KILL del run anterior, que en la Pico sale tras drenar la
      cola) NO es de esta prueba: se descarta y se anota («rezagado de la
      sesion N»).
    - DEPENDENCIAS: el RUN resuelve basedir -> tal cual -> /app -> /lib ->
      /sys (bpvm.c bpvm_entry_resolve) y el overlay de packs contesta al stat
      de /app/X.mod sin salir en LIST (#493). Mirar el CRC de /lib/X.mod no
      dice nada de lo que va a cargar el RUN: se pregunta con STAT name=
      (mismo resolvedor) y se exige que resuelva a /lib/X o /app/X con el
      crc local; si no, se sube a /app y se vuelve a preguntar; si sigue
      resolviendo a otro sitio, «dependencia X sombreada por <path>» = ERROR.
    - /lib de la DK2 (STM32U5G9J-DK2): se VACIA en cada arranque y los modulos
      de la GUI no van embebidos. Por eso las dependencias se comprueban en
      cada tanda y lo que falte se sube a /app (donde el RUN lo busca antes
      que en /lib).
    - ARTEFACTOS RANCIOS: un GuiShot.shot de otra tanda se bajaria como
      captura de esta. Antes del RUN se borra cada candidato (/app/X, /X, X)
      con DEL y se confirma con STAT que NO existe; si no se puede garantizar,
      lo bajado es DUDOSO y no va a NECESITA OJOS.
    - TRANSPORTE CAIDO a mitad del RUN: la salida ya recibida se conserva
      (sale en el informe como «salida recibida, sin EXITED»); «ejecutadas»
      cuenta solo las que dieron EXITED, «no terminaron» las demas que
      arrancaron; la placa se aborta.
    - REINICIOS: INFO tras cada prueba; si uptimeMs decrece, «la placa SE
      REINICIO durante X» y la prueba es REINICIO. (En fallo de CARGA la Pico
      manda el ordinal del enum como exitCode, no la tabla comun: se anota.)
    - puts > 4 KB: el PUT de un tiron tiene tope en el scratch de la placa
      (Gui.mod son 60 KB y lo rechaza). Por encima de 4 KB va por trozos con
      PUT_BEGIN / PUT_DATA / PUT_END (#294).
    - MSYS_NO_PATHCONV: desde Git Bash, un argumento que empieza por "/" lo
      convierte MSYS en una ruta de Windows ("/app/X.mod" -> "C:/Program
      Files/Git/app/X.mod") y la placa contesta NOT_FOUND. Este script no pasa
      rutas de placa por la linea de comandos (van dentro del JSON del wire),
      asi que no le afecta; pero si se lanza wire_serie.py a mano desde Git
      Bash, `MSYS_NO_PATHCONV=1 python tools/wire_serie.py COM3 run /app/X.mod`.
    - el GET no se puede leer «por lineas»: la cabecera GET_REPLY trae "bulk":N
      y detras vienen N bytes CRUDOS (un .shot tiene saltos de linea dentro).
      Se leen contados (ver cmd_get de wire_serie.py, que se comio una medida
      por esto).
    - la consola de Windows es cp1252: los prints de aqui son ASCII; el informe
      va en UTF-8 (la salida de los programas puede traer acentos).

El cliente del wire (send/lines/esperar/hola) es el de tools/wire_serie.py,
copiado aqui con un transporte intercambiable (serie o TCP) para que el mismo
codigo hable con una placa y con el simulador.
"""
import argparse
import datetime
import difflib
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import zlib

try:
    import serial
    import serial.tools.list_ports
except ImportError:          # solo hace falta para las placas; el --sim no lo usa
    serial = None

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))            # C:/lenguajes/pm
BPGENVM = os.path.join(RAIZ, "bpgenvm-c")
JAR = os.path.join(RAIZ, "lexer-java", "target", "basicplus-frontend.jar")
EXE = os.path.join(BPGENVM, "build", "bpgenvm-c.exe")
if not os.path.exists(EXE):
    EXE = EXE[:-4]
STDLIB = os.path.join(RAIZ, "bpstdlib")
BUILD_TANDA = os.path.join(BPGENVM, "build", "tanda")
SHOT2PNG = os.path.join(AQUI, "shot2png.py")

TROZO_PUT = 4096          # por encima de esto, PUT por trozos (#294)
HELLO_SEGS = 3.0

# Placas que se SABE que llevan panel, cuando ni HELLO ni INFO lo dicen. Se
# anota en el informe como "supuesto por boardName": es una tabla nuestra, no
# un dato de la placa.
CON_PANTALLA = ("esp32c6", "esp32p4", "stm32u5")
SIN_PANTALLA = ("rp2350a", "rp2350b", "pico", "esp32s3", "esp32c3", "host")


def log(msg):
    """Print en ASCII puro: la consola es cp1252 y no se fia de nadie."""
    print(msg.encode("ascii", "replace").decode("ascii"), flush=True)


# ── transporte: serie o TCP, misma cara ──────────────────────────────────────

class TransSerie:
    def __init__(self, puerto, baud=115200):
        self.nombre = "serie"
        self.puerto = puerto
        self.s = serial.Serial(puerto, baud, timeout=0.2)
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def read(self, n):
        return self.s.read(n)

    def write(self, b):
        self.s.write(b)

    def close(self):
        self.s.close()


class TransTcp:
    def __init__(self, host, port):
        self.nombre = "tcp"
        self.puerto = "%s:%d" % (host, port)
        self.s = socket.create_connection((host, port), timeout=3.0)
        self.s.settimeout(0.2)

    def read(self, n):
        try:
            c = self.s.recv(n)
        except socket.timeout:
            return b""
        if not c:
            raise EOFError("el otro lado cerro la conexion")
        return c

    def write(self, b):
        self.s.sendall(b)

    def close(self):
        try:
            self.s.close()
        except Exception:
            pass


# ── el cliente del wire (de tools/wire_serie.py, clase Wire) ─────────────────

class Wire:
    def __init__(self, trans):
        self.t = trans
        self.buf = b""
        self.id = 0
        self.otros = []
        self.basura = []
        self.pend = []
        self.roto = None             # la excepcion del transporte, si se cayo (hallazgo 5)
        self.session = None          # la del ultimo RUN_REPLY (hallazgo 3)
        self.ultima_prueba = None    # la que corrio la ultima (para decir QUE sigue corriendo)

    def send(self, typ, **f):
        self.id += 1
        f.update(type=typ, id=self.id)
        self.t.write((json.dumps(f) + "\n").encode())
        return self.id

    def lines(self, secs):
        """Los mensajes JSON que llegan en `secs`. Si el transporte se cae, lo
        YA recibido se entrega en esta llamada y la siguiente lanza la excepcion
        (hallazgo 5: antes el raise se llevaba por delante los OUTPUT que
        venian en el mismo chunk que el corte)."""
        if self.roto is not None and not self.pend:
            raise self.roto
        out, t0 = self.pend, time.time()
        self.pend = []
        while time.time() - t0 < secs:
            try:
                c = self.t.read(4096)
            except (EOFError, OSError) as e:
                self.roto = e
                return out
            if c:
                self.buf += c
            while b"\n" in self.buf:
                ln, self.buf = self.buf.split(b"\n", 1)
                ln = ln.strip()
                if ln.startswith(b"{"):
                    try:
                        out.append(json.loads(ln.decode("utf-8", "replace")))
                    except Exception:
                        self.basura.append(ln[:200])
                else:
                    self.basura.append(ln[:200])
        return out

    def esperar(self, typ, secs):
        """Espera un tipo. GUARDA lo demas en self.otros (wire_serie.py: tirar la
        respuesta que no encaja es como se pierden los diagnosticos).
        Si llega un ERROR con el id de la ULTIMA peticion, la respuesta buena ya
        no va a venir (BUSY, NOT_FOUND...): se corta en seco en vez de agotar
        el plazo — con BUSY eran 20 s por PUT (hallazgo 4)."""
        self.otros = []
        t0 = time.time()
        while time.time() - t0 < secs:
            lote = self.lines(0.2)
            for n, m in enumerate(lote):
                if m.get("type") == typ:
                    self.pend = lote[n + 1:]
                    return m
                self.otros.append(m)
                if m.get("type") == "ERROR" and m.get("id") == self.id:
                    self.pend = lote[n + 1:]
                    return None
        return None

    def codigo_error(self):
        """El code del ERROR que contesto a la ultima peticion, o None."""
        for m in self.otros:
            if m.get("type") == "ERROR" and m.get("id") == self.id:
                return m.get("code")
        return None

    def vaciar_basura(self):
        self.basura = []

    def basura_texto(self, n=20):
        """Las ultimas n lineas NO-JSON que soltó el wire (panics del SDK, hard
        faults, avisos [bpvm-c]), como texto (hallazgo 6)."""
        out = []
        for b in self.basura[-n:]:
            out.append(b.decode("utf-8", "replace") if isinstance(b, bytes) else str(b))
        return out

    def errores(self):
        """Los ERROR que llegaron mientras se esperaba otra cosa, legibles."""
        return "; ".join("%s: %s" % (m.get("code"), m.get("message"))
                         for m in self.otros if m.get("type") == "ERROR") or \
               ("llego " + ",".join(sorted(set(m.get("type", "?") for m in self.otros)))
                if self.otros else "sin respuesta")

    def hola(self, secs=HELLO_SEGS):
        self.send("HELLO", protoVersion=1, clientName="tanda", client="tanda", version=1)
        return self.esperar("HELLO_REPLY", secs)

    def llamar(self, typ, reply, secs, **f):
        self.send(typ, **f)
        return self.esperar(reply, secs)

    def put(self, data, remoto):
        """Sube bytes a la placa. <= TROZO_PUT de un tiron (cabecera + bytes
        crudos); por encima, PUT_BEGIN/PUT_DATA/PUT_END (cmd_put/cmd_puts de
        wire_serie.py). Devuelve (ok, detalle)."""
        if len(data) <= TROZO_PUT:
            self.id += 1
            cab = json.dumps({"type": "PUT", "id": self.id, "path": remoto, "bulk": len(data)})
            self.t.write((cab + "\n").encode() + data)
            r = self.esperar("PUT_REPLY", 20)
            return (r is not None), ("ok" if r else "PUT fallo: " + self.errores())
        self.send("PUT_BEGIN", path=remoto, size=len(data))
        if self.esperar("PUT_BEGIN_REPLY", 15) is None:
            return False, "PUT_BEGIN fallo: " + self.errores()
        enviados = 0
        while enviados < len(data):
            cacho = data[enviados:enviados + TROZO_PUT]
            self.id += 1
            cab = json.dumps({"type": "PUT_DATA", "id": self.id, "bulk": len(cacho)})
            self.t.write((cab + "\n").encode() + cacho)
            if self.esperar("PUT_DATA_REPLY", 15) is None:
                return False, "PUT_DATA fallo en el byte %d: %s" % (enviados, self.errores())
            enviados += len(cacho)
        self.send("PUT_END", size=len(data))
        r = self.esperar("PUT_END_REPLY", 20)
        if r is None:
            return False, "PUT_END fallo: " + self.errores()
        if r.get("size") not in (None, len(data)):
            return False, "PUT_END dice %s B y se mandaron %d" % (r.get("size"), len(data))
        return True, "ok (por trozos)"

    def _linea_cruda(self, secs):
        """Una linea del buffer, byte a byte si hace falta. Para el GET: aqui
        NO vale lines(), que trocea por saltos y se comeria el bulk."""
        tope = time.time() + secs
        while time.time() < tope:
            if b"\n" in self.buf:
                ln, self.buf = self.buf.split(b"\n", 1)
                return ln.strip()
            c = self.t.read(1)
            if c:
                self.buf += c
        return None

    def get(self, remoto, secs=60):
        """Baja un fichero: (bytes | None, detalle). Cabecera GET_REPLY con
        "bulk":N y N bytes crudos detras (cmd_get de wire_serie.py)."""
        if self.pend:
            self.basura.append(("pendientes antes del GET: %d" % len(self.pend)).encode())
            self.pend = []
        self.send("GET", path=remoto)
        tope = time.time() + 15
        r = None
        while time.time() < tope:
            ln = self._linea_cruda(15)
            if ln is None:
                return None, "GET %s: sin cabecera" % remoto
            if not ln.startswith(b"{"):
                self.basura.append(ln[:200])
                continue
            try:
                m = json.loads(ln.decode("utf-8", "replace"))
            except Exception:
                self.basura.append(ln[:200])
                continue
            if m.get("type") == "GET_REPLY":
                r = m
                break
            if m.get("type") == "ERROR":
                return None, "GET %s: %s %s" % (remoto, m.get("code"), m.get("message", ""))
            self.otros.append(m)
        if r is None:
            return None, "GET %s: sin GET_REPLY" % remoto
        n = int(r.get("bulk", 0))
        tope = time.time() + secs
        while len(self.buf) < n and time.time() < tope:
            c = self.t.read(min(4096, n - len(self.buf)))
            if c:
                self.buf += c
        if len(self.buf) < n:
            datos, self.buf = self.buf, b""
            return None, "GET %s: INCOMPLETO %d/%d B" % (remoto, len(datos), n)
        datos, self.buf = self.buf[:n], self.buf[n:]
        return datos, "ok (%d B)" % n

    # -- sesiones (hallazgo 3): RUN_REPLY, OUTPUT y EXITED llevan `session` en
    # las cuatro familias y en el sim. Un EXITED rezagado (el KILL de un run
    # anterior, que en la Pico sale tras drenar la cola) NO es de esta prueba.
    def _de_otra_sesion(self, m, session, rezagados):
        s = m.get("session")
        if session is None or s is None or s == session:
            return False
        rezagados.append("%s rezagado de la sesion %s%s" % (
            m.get("type"), s, (" (%s)" % m.get("status")) if m.get("status") else ""))
        return True

    def esperar_exited(self, secs, session=None, out=None):
        """Espera el EXITED de `session` (None = cualquiera) hasta secs,
        acumulando los OUTPUT de esa sesion en `out`. Devuelve (exited, rezagados)."""
        rezagados = []
        t0 = time.time()
        while time.time() - t0 < secs:
            for m in self.lines(0.3):
                typ = m.get("type")
                if typ in ("OUTPUT", "EXITED") and self._de_otra_sesion(m, session, rezagados):
                    continue
                if typ == "OUTPUT" and out is not None:
                    out.append(m.get("data", ""))
                elif typ == "EXITED":
                    return m, rezagados
        return None, rezagados

    def info(self, secs=6):
        return self.llamar("INFO", "INFO_REPLY", secs)

    def limpiar_arranque(self, secs=10):
        """KILL al conectar y ESPERAR a que la placa este en reposo (hallazgo 3):
        hasta el EXITED, o hasta que INFO conteste sin BUSY, o `secs`.
        Devuelve (corria, detalle): corria = True/False/None (no se supo)."""
        kid = self.send("KILL")
        t0 = time.time()
        vio_kill_reply = False
        while time.time() - t0 < secs:
            for m in self.lines(0.3):
                typ = m.get("type")
                if typ == "EXITED":
                    return True, "habia algo corriendo: EXITED %s (sesion %s) tras %d ms" % (
                        m.get("status"), m.get("session"), int((time.time() - t0) * 1000))
                if typ == "KILL_REPLY":
                    vio_kill_reply = True
                if typ == "ERROR" and m.get("id") == kid:
                    if m.get("code") == "NO_SESSION":
                        return False, "nada corria (KILL: NO_SESSION)"
                    return None, "KILL rechazado: %s %s" % (m.get("code"), m.get("message"))
            if vio_kill_reply:
                # KILL_REPLY sin EXITED: o reposo (el sim contesta asi en vacio) o
                # el ack diferido con el EXITED aun por drenar. INFO lo desempata:
                # si contesta, no hay nada corriendo; si BUSY, se sigue esperando.
                r = self.info(3)
                ex = [m for m in self.otros if m.get("type") == "EXITED"]
                if ex:
                    return True, "habia algo corriendo: EXITED %s (sesion %s) tras %d ms" % (
                        ex[0].get("status"), ex[0].get("session"), int((time.time() - t0) * 1000))
                if r is not None:
                    return False, "nada corria (KILL_REPLY e INFO contesta)"
                if self.codigo_error() != "BUSY":
                    return None, "tras KILL, INFO no contesta: " + self.errores()
        return None, "tras KILL, ni EXITED ni INFO sin BUSY en %d s" % secs

    def correr(self, path, timeout_s):
        """RUN -> RUN_REPLY -> OUTPUT... -> EXITED. Devuelve un dict con
        salida, exited, colgada, error, tipos (lo demas que llego), session,
        rezagados (mensajes de OTRA sesion, descartados), arranco.
        Si el transporte se cae a mitad (hallazgo 5) NO se pierde lo recibido:
        vuelve con error + salida parcial + transporte_roto=True."""
        res = {"salida": "", "exited": None, "colgada": False, "error": None,
               "tipos": [], "ms_wire": None, "session": None, "rezagados": [],
               "arranco": False, "transporte_roto": False}
        out = []
        try:
            self.send("RUN", path=path)
            r = self.esperar("RUN_REPLY", 10)
            if r is None:
                err = self.errores()
                if "BUSY" in err:
                    # la trampa: algo seguia corriendo. KILL, esperar el reposo y UNA vez mas.
                    corria, det_k = self.limpiar_arranque(10)
                    res["rezagados"].append("RUN contesto BUSY; tras KILL: %s" % det_k)
                    self.send("RUN", path=path)
                    r = self.esperar("RUN_REPLY", 10)
                    if r is None:
                        res["error"] = "el RUN no arranco (tras KILL por BUSY): " + self.errores()
                        return res
                else:
                    res["error"] = "el RUN no arranco: " + err
                    return res
            res["arranco"] = True
            session = r.get("session")
            res["session"] = session
            self.session = session
            t0 = time.time()
            otros = set()
            while time.time() - t0 < timeout_s:
                for m in self.lines(0.3):
                    typ = m.get("type")
                    if typ in ("OUTPUT", "EXITED") and self._de_otra_sesion(m, session, res["rezagados"]):
                        continue
                    if typ == "OUTPUT":
                        out.append(m.get("data", ""))
                    elif typ == "EXITED":
                        res["salida"] = "".join(out)
                        res["exited"] = m
                        res["ms_wire"] = int((time.time() - t0) * 1000)
                        res["tipos"] = sorted(otros)
                        return res
                    else:
                        otros.add(typ or "?")
            # timeout: KILL y esperar el EXITED (de ESTA sesion) 5 s
            self.send("KILL")
            ex, rz = self.esperar_exited(5, session, out)
            res["rezagados"].extend(rz)
            res["salida"] = "".join(out)
            res["exited"] = ex
            res["colgada"] = True
            res["ms_wire"] = int((time.time() - t0) * 1000)
            res["tipos"] = sorted(otros)
            return res
        except (EOFError, OSError) as e:
            res["salida"] = "".join(out)
            res["error"] = "transporte durante el RUN: %s" % e
            res["transporte_roto"] = True
            return res

    def close(self):
        self.t.close()


# ── manifiesto y fuentes ─────────────────────────────────────────────────────

def leer_manifiesto(ruta):
    with open(ruta, encoding="utf-8") as f:
        doc = json.load(f)
    entradas = doc["pruebas"] if isinstance(doc, dict) else doc
    lista = []
    for e in entradas:
        p = {
            "nombre": e["nombre"],
            "fuente": e["fuente"],
            "criterio": e.get("criterio", "identico"),
            "timeout_s": int(e.get("timeout_s", 30)),
            "necesita": list(e.get("necesita", [])),
            "recursos": list(e.get("recursos", [])),
            "artefactos": list(e.get("artefactos", [])),
        }
        if p["criterio"] not in ("identico", "no-peta"):
            sys.exit("manifiesto: criterio desconocido '%s' en %s" % (p["criterio"], p["nombre"]))
        bp = os.path.join(RAIZ, p["fuente"])
        if not os.path.exists(bp):
            sys.exit("manifiesto: no existe %s (prueba %s)" % (bp, p["nombre"]))
        for r in p["recursos"]:
            if not os.path.exists(os.path.join(RAIZ, r)):
                sys.exit("manifiesto: no existe el recurso %s (prueba %s)" % (r, p["nombre"]))
        p["bp"] = bp
        p["modulo"] = nombre_modulo(bp)
        if not p["modulo"]:
            sys.exit("manifiesto: no se le ve el 'module' a %s" % bp)
        lista.append(p)
    return lista


RE_MODULE = re.compile(r"^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)", re.M)
RE_IMPORT = re.compile(r"^\s*import\s+([A-Za-z_][A-Za-z0-9_.]*)", re.M)


def nombre_modulo(bp):
    """El modulo raiz se lee del 'module X' del .bp, como hace compat.sh: el
    nombre del fichero no vale (counter.bp declara CounterSample)."""
    with open(bp, encoding="utf-8", errors="replace") as f:
        m = RE_MODULE.search(f.read())
    return m.group(1) if m else None


def imports_de(bp):
    with open(bp, encoding="utf-8", errors="replace") as f:
        return [m.group(1).split(".")[0] for m in RE_IMPORT.finditer(f.read())]


def dependencias(bp):
    """Los imports del .bp, cerrados transitivamente sobre la stdlib (Gui importa
    Json y Collections, y el import NO es transitivo en el lenguaje pero el
    cargador si necesita los .mod). Core siempre."""
    vistos = []
    cola = ["Core"] + imports_de(bp)
    while cola:
        x = cola.pop(0)
        if x in vistos:
            continue
        vistos.append(x)
        src = os.path.join(STDLIB, x + ".bp")
        if os.path.exists(src):
            cola.extend(imports_de(src))
    return vistos


# ── normalizacion (compat.sh filt()) ─────────────────────────────────────────

RE_RUIDO = re.compile(r"INICIANDO|FIN DE|heapStart")


def normalizar(texto):
    texto = texto.replace("\r\n", "\n").replace("\r", "\n")
    lineas = [l for l in texto.split("\n")
              if not RE_RUIDO.search(l) and not l.startswith("config:")]
    while lineas and not lineas[0].strip():
        lineas.pop(0)
    while lineas and not lineas[-1].strip():
        lineas.pop()
    return "\n".join(lineas)


def crc_de(ruta):
    with open(ruta, "rb") as f:
        return zlib.crc32(f.read()) & 0xFFFFFFFF


def hora_de(ruta):
    if not os.path.exists(ruta):
        return "NO EXISTE"
    return datetime.datetime.fromtimestamp(os.path.getmtime(ruta)).strftime("%Y-%m-%d %H:%M:%S")


def sabor_host():
    d = os.path.join(BPGENVM, "build")
    sabores = [f[len(".flavor-"):] for f in os.listdir(d) if f.startswith(".flavor-")] \
        if os.path.isdir(d) else []
    return ",".join(sorted(sabores)) or "sin fichero .flavor-*"


# ── el oraculo (PC) ──────────────────────────────────────────────────────────

def compilar(p, work):
    """Compila el .bp en work con la stdlib fresca al lado. Devuelve (ok, log).
    Primero --compile a secas; si no, por proyecto (.bpbuild), como compat.sh."""
    os.makedirs(work, exist_ok=True)
    for f in os.listdir(work):
        if f.endswith(".mod") or f.endswith(".dbg") or f.endswith(".slots") or f == "p.bpbuild":
            os.remove(os.path.join(work, f))
    for f in os.listdir(STDLIB):
        if f.endswith(".mod"):
            shutil.copy2(os.path.join(STDLIB, f), work)
    for r in p["recursos"]:
        shutil.copy2(os.path.join(RAIZ, r), work)
    r = subprocess.run(["java", "-jar", JAR, p["bp"], "--compile", work, "--backend=mivm"],
                       capture_output=True, cwd=work)
    mod = os.path.join(work, p["modulo"] + ".mod")
    if r.returncode == 0 and os.path.exists(mod):
        return True, "compilado suelto"
    log1 = r.stdout.decode("utf-8", "replace")[-800:] + r.stderr.decode("utf-8", "replace")[-800:]
    src = os.path.join(work, "src")
    os.makedirs(src, exist_ok=True)
    shutil.copy2(p["bp"], src)
    with open(os.path.join(work, "p.bpbuild"), "w", encoding="utf-8") as f:
        f.write(json.dumps({"sourceDir": "src", "outDir": ".", "main": p["modulo"],
                            "dependencies": [STDLIB.replace("\\", "/")]}) + "\n")
    r = subprocess.run(["java", "-jar", JAR, "--project", "p.bpbuild", "--backend=mivm"],
                       capture_output=True, cwd=work)
    if r.returncode == 0 and os.path.exists(mod):
        return True, "compilado por proyecto (.bpbuild)"
    log2 = r.stdout.decode("utf-8", "replace")[-800:] + r.stderr.decode("utf-8", "replace")[-800:]
    return False, "no compila. suelto:\n" + log1 + "\npor proyecto:\n" + log2


def oraculo(p, sin_oraculo, extra_args=()):
    """Compila y (si toca) ejecuta el host. Devuelve el dict del oraculo.
    extra_args: p.ej. ("--screen=800x480",) — el modelo de la GUI mide lo que
    mide el panel («no hay otra», 4-sep), asi que el oraculo de una prueba con
    pantalla se calcula POR PLACA con su tamano; sin eso, GuiWinJson daria
    DIFIERE en toda placa que no mida 480x320 por el propio tamano del screen."""
    work = os.path.join(BUILD_TANDA, p["nombre"])
    o = {"work": work, "mod": None, "crc": None, "size": None, "compilado": False,
         "salida": None, "rc": None, "ms": None, "error": None, "ejecutado": False}
    ok, detalle = compilar(p, work)
    o["compilacion"] = detalle
    if not ok:
        o["error"] = detalle
        return o
    o["compilado"] = True
    o["mod"] = os.path.join(work, p["modulo"] + ".mod")
    o["crc"] = crc_de(o["mod"])
    o["size"] = os.path.getsize(o["mod"])
    if sin_oraculo:
        return o
    t0 = time.time()
    try:
        r = subprocess.run([EXE] + list(extra_args) + [p["modulo"] + ".mod"], capture_output=True,
                           cwd=work, timeout=p["timeout_s"])
        o["args"] = list(extra_args)
        o["rc"] = r.returncode
        o["salida"] = normalizar(r.stdout.decode("utf-8", "replace"))
        o["salida_cruda"] = r.stdout.decode("utf-8", "replace")
        o["stderr_cola"] = r.stderr.decode("utf-8", "replace")[-600:]
    except subprocess.TimeoutExpired as e:
        o["rc"] = None
        o["salida"] = normalizar((e.stdout or b"").decode("utf-8", "replace"))
        o["error"] = "el host no termino en %d s" % p["timeout_s"]
    o["ms"] = int((time.time() - t0) * 1000)
    o["ejecutado"] = True
    return o


# ── placas ───────────────────────────────────────────────────────────────────

def abrir_puertos(spec):
    """Devuelve una lista de Wire ya saludados (HELLO_REPLY guardado en w.hello)."""
    if serial is None:
        log("  falta pyserial: no se pueden abrir puertos serie")
        return []
    if spec == "auto":
        puertos = [p.device for p in serial.tools.list_ports.comports()]
        puertos = [p for p in sorted(puertos) if p.upper() != "COM1"]
        log("  puertos serie enumerados (sin COM1): %s" % (", ".join(puertos) or "ninguno"))
    else:
        puertos = [p.strip() for p in spec.split(",") if p.strip()]
    wires = []
    for pt in puertos:
        try:
            w = Wire(TransSerie(pt))
        except Exception as e:
            log("  %s: no se pudo abrir (%s)" % (pt, e))
            continue
        h = w.hola()
        if h is None:
            log("  %s: sin HELLO_REPLY en %.0f s (%s)" % (pt, HELLO_SEGS, w.errores()))
            w.close()
            continue
        w.hello = h
        log("  %s: HELLO_REPLY de %s (%s)" % (pt, h.get("serverName"), h.get("serverBuild")))
        wires.append(w)
    return wires


def abrir_sim(spec):
    host, _, port = spec.rpartition(":")
    try:
        w = Wire(TransTcp(host or "127.0.0.1", int(port)))
    except Exception as e:
        log("  sim %s: no se pudo conectar (%s)" % (spec, e))
        return None
    h = w.hola()
    if h is None:
        log("  sim %s: sin HELLO_REPLY (%s)" % (spec, w.errores()))
        w.close()
        return None
    w.hello = h
    log("  sim %s: HELLO_REPLY de %s (%s)" % (spec, h.get("serverName"), h.get("serverBuild")))
    return w


def sello_de(w, arranque=None):
    info = w.info(6) or {}
    h = w.hello
    return {
        "boardName": info.get("boardName", "?"),
        "uniqueId": info.get("uniqueId", "?"),
        "serverName": h.get("serverName", "?"),
        "serverBuild": h.get("serverBuild", "?"),
        "capabilities": h.get("capabilities", []),
        # hallazgo 8: lo que identifica la IMAGEN y el estado de la placa
        "arch": info.get("arch"),
        "variant": info.get("variant"),
        "cpuFreqHz": info.get("cpuFreqHz"),
        "resetReason": info.get("resetReason"),
        "uptimeMs": info.get("uptimeMs"),
        "fsUsedBytes": info.get("fsUsedBytes"),
        "fsTotalBytes": info.get("fsTotalBytes"),
        "vmHeapBytes": info.get("vmHeapBytes"),
        "uptime_ultimo": info.get("uptimeMs") if isinstance(info.get("uptimeMs"), int) else None,
        "arranque": arranque,          # (corria, detalle) del KILL de conexion
        "transporte": w.t.nombre,
        "puerto": w.t.puerto,
        "info": info,
    }


def tiene_pantalla(sello):
    """(bool, fuente). Primero lo que DICE la placa (capacidades, screenW/H del
    INFO); si no dice nada, la tabla por boardName, y se rotula 'supuesto'."""
    caps = [str(c).upper() for c in sello.get("capabilities") or []]
    for c in caps:
        if c in ("GUI", "DISPLAY", "SCREEN", "LVGL"):
            return True, "capacidad %s en HELLO" % c
    info = sello.get("info") or {}
    if "screenW" in info and "screenH" in info:
        w_, h_ = info.get("screenW") or 0, info.get("screenH") or 0
        return (w_ > 0 and h_ > 0), "INFO screenW x screenH = %sx%s" % (w_, h_)
    nombre = str(sello.get("boardName", "")).lower()
    for pref in CON_PANTALLA:
        if nombre.startswith(pref):
            return True, "supuesto por boardName '%s'" % nombre
    for pref in SIN_PANTALLA:
        if nombre.startswith(pref):
            return False, "supuesto por boardName '%s'" % nombre
    return False, "supuesto: boardName '%s' desconocido, se asume sin pantalla" % nombre


def etiqueta_placa(sello):
    return "%s_%s" % (re.sub(r"[^A-Za-z0-9]+", "", sello["boardName"]) or "placa",
                      re.sub(r"[^A-Za-z0-9]+", "", sello["uniqueId"])[:12] or "sinid")


def inventario(w):
    """LIST una vez: {ruta: size} de /lib y /app. (LIST da crc -1: el CRC se
    pide con STAT name=... crc=true, que ademas resuelve como el RUN.)"""
    r = w.llamar("LIST", "LIST_REPLY", 20)
    if r is None:
        return None, "LIST sin respuesta: " + w.errores()
    inv = {}
    for e in r.get("entries") or []:
        n = e.get("name", "")
        if n.startswith("/lib/") or n.startswith("/app/"):
            inv[n] = e.get("size")
    return inv, "%d ficheros en /lib+/app, omitidos=%s" % (len(inv), r.get("omitted", 0))


class PlacaColgada(RuntimeError):
    """La placa contesta BUSY a todo y el KILL no la saca de ahi: se aborta la
    PLACA entera, no solo la prueba (hallazgo 4)."""


class DepSombreada(RuntimeError):
    """Una dependencia que el RUN va a resolver a OTRO fichero que el que
    creemos haber comprobado (hallazgo 2)."""


def put_con_busy(w, data, remoto):
    """w.put con la trampa del BUSY (hallazgo 4): si la placa contesta BUSY es
    que sigue ejecutando algo (una prueba COLGADA cuyo KILL no dio EXITED).
    KILL, esperar el EXITED hasta 10 s, reintentar UNA vez; si persiste, la
    placa se da por colgada."""
    ok, det = w.put(data, remoto)
    if ok or "BUSY" not in det:
        return ok, det
    corria, det_k = w.limpiar_arranque(10)      # KILL y esperar el reposo (EXITED o INFO sin BUSY)
    ok, det2 = w.put(data, remoto)
    if ok:
        return ok, det2 + " (tras BUSY: KILL, %s, reintento)" % det_k
    if "BUSY" in det2:
        raise PlacaColgada("la placa sigue ejecutando %s tras KILL (BUSY al subir %s; %s)" % (
            w.ultima_prueba or "algo que no arranco esta tanda", remoto, det_k))
    return ok, det2 + " (tras BUSY: KILL, %s, reintento)" % det_k


def stat_por_nombre(w, fich):
    """STAT name= : la placa resuelve con bpvm_entry_resolve, EL MISMO que usa el
    RUN (basedir -> tal cual -> /app -> /lib -> /sys), y dice path y crc.
    Devuelve (path, crc) o (None, None) si no resuelve."""
    st = w.llamar("STAT", "STAT_REPLY", 10, name=fich, crc=True)
    if st is None:
        return None, None
    crc = st.get("crc")
    if isinstance(crc, int) and crc != -1:
        crc &= 0xFFFFFFFF
    return st.get("path"), crc


def anotar(notas, n):
    """Una nota por placa, no una por prueba (las deps se preguntan en cada prueba)."""
    if n not in notas:
        notas.append(n)


def asegurar_deps(w, deps, inv, subidos, notas, verificadas):
    """Cada dependencia se comprueba COMO LA RESUELVE EL RUN (STAT por nombre,
    hallazgo 2): si resuelve a /lib/X o /app/X con el crc local, vale; si no
    (no resuelve, resuelve a otro sitio, o el crc difiere), se sube a /app y se
    vuelve a preguntar; si aun asi no resuelve a /app/X con el crc local, la
    dependencia esta SOMBREADA y la prueba es ERROR. Mirar el CRC del fichero
    de /lib no vale: el RUN busca /app antes que /lib, y el overlay de packs
    contesta al stat de /app/X.mod sin salir en LIST (#493)."""
    for d in deps:
        if d in verificadas:
            continue
        fich = d + ".mod"
        local = os.path.join(STDLIB, fich)
        if not os.path.exists(local):
            anotar(notas, "dependencia %s: no hay %s en bpstdlib (¿modulo de la app?)" % (d, fich))
            continue
        crc_local = crc_de(local)
        path, crc_placa = stat_por_nombre(w, fich)
        if path in ("/lib/" + fich, "/app/" + fich) and crc_placa == crc_local:
            anotar(notas, "dependencia %s: ya en %s (crc igual; resuelto por nombre, como el RUN)" % (d, path))
            verificadas.add(d)
            continue
        crc_txt = ("%08x" % crc_placa) if isinstance(crc_placa, int) and crc_placa >= 0 else str(crc_placa)
        if path is None:
            anotar(notas, "dependencia %s: el RUN no la resuelve (%s): se sube a /app" % (d, w.errores()))
        elif path not in ("/lib/" + fich, "/app/" + fich):
            anotar(notas, "dependencia %s: el RUN la resuelve a %s (fuera de /lib y /app), crc %s: se sube a /app"
                         % (d, path, crc_txt))
        elif path.startswith("/lib/"):
            anotar(notas, "dependencia %s: en /lib con crc %s != local %08x (DESFASE, se sube a /app por delante)"
                         % (d, crc_txt, crc_local))
        else:
            anotar(notas, "dependencia %s: en /app con crc %s != local %08x (rancio, se vuelve a subir)"
                         % (d, crc_txt, crc_local))
        with open(local, "rb") as f:
            data = f.read()
        ok, det = put_con_busy(w, data, "/app/" + fich)
        subidos.append({"remoto": "/app/" + fich, "local": os.path.relpath(local, RAIZ),
                        "size": len(data), "crc": "%08x" % crc_local, "ok": ok, "detalle": det})
        if not ok:
            raise RuntimeError("no se pudo subir %s: %s" % (fich, det))
        inv["/app/" + fich] = len(data)
        path2, crc2 = stat_por_nombre(w, fich)
        if path2 != "/app/" + fich or crc2 != crc_local:
            n = "dependencia %s: SOMBREADA por %s (crc %s) aunque se acaba de subir /app/%s con crc %08x" % (
                d, path2, ("%08x" % crc2) if isinstance(crc2, int) and crc2 >= 0 else crc2, fich, crc_local)
            anotar(notas, n)
            raise DepSombreada("dependencia %s sombreada por %s" % (d, path2))
        verificadas.add(d)


def subir_fichero(w, local, remoto, subidos):
    with open(local, "rb") as f:
        data = f.read()
    ok, det = put_con_busy(w, data, remoto)
    subidos.append({"remoto": remoto, "local": os.path.relpath(local, RAIZ) if local.startswith(RAIZ) else local,
                    "size": len(data), "crc": "%08x" % (zlib.crc32(data) & 0xFFFFFFFF),
                    "ok": ok, "detalle": det})
    if not ok:
        raise RuntimeError("no se pudo subir %s: %s" % (remoto, det))


def borrar_artefactos(w, artefactos):
    """Antes del RUN (hallazgo 1): que no quede un artefacto de OTRA tanda que
    luego se baje como si fuera de esta. Borra cada candidato (DEL) y confirma
    con STAT que NO existe. Devuelve {nombre: {"existia": [rutas], "dudoso": txt|None}}."""
    previos = {}
    for art in artefactos:
        existia, problemas = [], []
        for remoto in ("/app/" + art, "/" + art, art):
            r = w.llamar("DEL", "DEL_REPLY", 10, path=remoto)
            del_fallo = None
            if r is not None:
                existia.append(remoto + " (borrado)")
            elif w.codigo_error() != "NOT_FOUND":
                del_fallo = "DEL %s: %s" % (remoto, w.errores())
            # lo que manda es el STAT: que NO exista, diga lo que diga el DEL
            st = w.llamar("STAT", "STAT_REPLY", 10, path=remoto)
            if st is not None:
                existia.append(remoto + " (SIGUE AHI, %s B)" % st.get("size"))
                problemas.append("%s sigue existiendo tras el DEL%s" % (remoto, (" [%s]" % del_fallo) if del_fallo else ""))
            elif w.codigo_error() != "NOT_FOUND":
                problemas.append("STAT %s no confirma que no exista: %s%s" % (
                    remoto, w.errores(), (" [%s]" % del_fallo) if del_fallo else ""))
        previos[art] = {"existia": existia, "dudoso": "; ".join(problemas) or None}
    return previos


def diff_corto(a, b, n=10):
    d = list(difflib.unified_diff(a.split("\n"), b.split("\n"), "oraculo (host)", "placa", lineterm=""))
    return "\n".join(d[:n]) + ("\n..." if len(d) > n else "")


def veredicto_de(p, o, res, sin_oraculo):
    """Devuelve (veredicto, detalle). Los veredictos de aqui: IDENTICO, NO-PETA,
    DIFIERE, PETA, COLGADA, ERROR, SIN ORACULO (SALTADA, SIN ARTEFACTO, DUDOSO y
    REINICIO los pone ejecutar_en_placa)."""
    ex = res["exited"]
    if res["error"]:
        return "ERROR", res["error"]
    if res["colgada"]:
        return "COLGADA", "sin EXITED en %d s; tras KILL: %s" % (
            p["timeout_s"], ("EXITED %s" % ex.get("status")) if ex else "tampoco EXITED en 5 s")
    if ex is None:
        return "ERROR", "sin EXITED"
    st, code = ex.get("status"), ex.get("exitCode")
    emsg = ex.get("errorMessage") or ""
    estado = "EXITED %s exit=%s%s" % (st, code, (" (%s)" % emsg) if emsg else "")
    # Hallazgo 7: un fallo de CARGA/ENLACE en la placa es fallo del ARNES (una
    # dependencia que falta o sombreada, un .mod que no se pudo leer), no una
    # paridad que medir: nunca IDENTICO, ni NO-PETA/PETA como si fuera el programa.
    fallo_arnes = (st == "LINK_ERROR" or emsg.startswith("load:") or emsg.startswith("falta el modulo"))
    if fallo_arnes:
        return "ERROR", "fallo del arnes en la placa, no del programa: %s%s" % (
            estado, " [ojo: en fallo de carga la Pico manda el ORDINAL del enum como exitCode, no la tabla comun]"
            if emsg.startswith("load:") or emsg.startswith("falta el modulo") else "")
    if p["criterio"] == "no-peta":
        if st == "OK" and code == 0:
            return "NO-PETA", estado
        return "PETA", estado
    if sin_oraculo or not o.get("ejecutado"):
        return "SIN ORACULO", estado
    salida_placa = normalizar(res["salida"])
    igual_texto = (salida_placa == o["salida"])
    ok_host = (o["rc"] == 0)
    ok_placa = (st == "OK" and code == 0)
    igual_estado = (ok_host == ok_placa)
    # Hallazgo 7: si los dos fallan, que fallen IGUAL: el exitCode del EXITED y
    # el rc del host salen de la misma tabla (bpvm_exit_code); dos fallos
    # distintos con stdout vacio salian IDENTICO.
    if igual_estado and not ok_host and code != o["rc"]:
        igual_estado = False
    if igual_texto and igual_estado:
        return "IDENTICO", "%s; host rc=%s" % (estado, o["rc"])
    partes = []
    if not igual_texto:
        partes.append("texto distinto")
    if not igual_estado:
        partes.append("estado no casa (host rc=%s, %s)" % (o["rc"], estado))
    return "DIFIERE", "; ".join(partes)


VERDES = ("IDENTICO", "NO-PETA")


def resultado_vacio(nombre, ms_host=None):
    return {"prueba": nombre, "veredicto": None, "detalle": "", "ms_placa": None,
            "ms_host": ms_host, "exited": None, "salida_placa": None, "diff": None,
            "artefactos": [], "necesita_ojos": [], "arranco": False, "termino": False,
            "rezagados": [], "basura": [], "artefactos_previos": {}, "info_tras": None,
            "reinicio": False, "abortar_placa": None}


def ejecutar_en_placa(w, sello, pantalla, p, o, sin_oraculo, inv, subidos, notas, verificadas):
    """Una prueba en una placa. Devuelve el dict del resultado. Si la placa ya
    no sirve para las siguientes (transporte roto, colgada tras KILL), lo dice
    en r["abortar_placa"] y main corta la placa."""
    r = resultado_vacio(p["nombre"], o.get("ms"))
    for req in p["necesita"]:
        if req == "pantalla":
            if not pantalla[0]:
                r["veredicto"] = "SALTADA"
                r["detalle"] = "necesita pantalla y la placa no la tiene (%s)" % pantalla[1]
                return r
        else:
            r["veredicto"] = "SALTADA"
            r["detalle"] = "requisito desconocido '%s' (no se como comprobarlo)" % req
            return r
    if not o.get("compilado"):
        r["veredicto"] = "ERROR"
        r["detalle"] = "no hay .mod: " + (o.get("error") or "?")
        return r
    w.vaciar_basura()          # hallazgo 6: la basura del wire, POR prueba
    try:
        asegurar_deps(w, dependencias(p["bp"]), inv, subidos, notas, verificadas)
        for rec in p["recursos"]:
            subir_fichero(w, os.path.join(RAIZ, rec), "/app/" + os.path.basename(rec), subidos)
        subir_fichero(w, o["mod"], "/app/" + p["modulo"] + ".mod", subidos)
        r["artefactos_previos"] = borrar_artefactos(w, p["artefactos"])
    except PlacaColgada as e:
        r["veredicto"] = "ERROR"
        r["detalle"] = "placa colgada: %s" % e
        r["abortar_placa"] = str(e)
        r["basura"] = w.basura_texto()
        return r
    except DepSombreada as e:
        r["veredicto"] = "ERROR"
        r["detalle"] = str(e)
        r["basura"] = w.basura_texto()
        return r
    except (EOFError, OSError) as e:
        r["veredicto"] = "ERROR"
        r["detalle"] = "transporte al subir: %s" % e
        r["abortar_placa"] = "transporte roto al subir %s: %s" % (p["nombre"], e)
        r["basura"] = w.basura_texto()
        return r
    except RuntimeError as e:
        r["veredicto"] = "ERROR"
        r["detalle"] = "subida: %s" % e
        r["basura"] = w.basura_texto()
        return r
    for art, pv in r["artefactos_previos"].items():
        if pv["dudoso"]:
            notas.append("%s: artefacto %s DUDOSO antes del RUN: %s" % (p["nombre"], art, pv["dudoso"]))
    log("    RUN /app/%s.mod (timeout %d s)" % (p["modulo"], p["timeout_s"]))
    w.ultima_prueba = p["nombre"]
    res = w.correr("/app/" + p["modulo"] + ".mod", p["timeout_s"])
    r["exited"] = res["exited"]
    r["salida_placa"] = res["salida"]          # hallazgo 5: tambien si hubo error
    r["tipos_extra"] = res["tipos"]
    r["ms_wire"] = res["ms_wire"]
    r["session"] = res["session"]
    r["arranco"] = res["arranco"]
    r["termino"] = res["exited"] is not None
    r["rezagados"] = res["rezagados"]
    if res["rezagados"]:
        notas.append("%s: descartado %s" % (p["nombre"], "; ".join(res["rezagados"])))
    if res["exited"]:
        r["ms_placa"] = res["exited"].get("elapsedMs")
    r["veredicto"], r["detalle"] = veredicto_de(p, o, res, sin_oraculo)
    if res["transporte_roto"]:
        r["abortar_placa"] = "transporte roto durante %s: %s" % (p["nombre"], res["error"])
        r["basura"] = w.basura_texto()
        return r
    if r["veredicto"] == "DIFIERE" and p["criterio"] == "identico" and o.get("salida") is not None:
        r["diff"] = diff_corto(o["salida"], normalizar(res["salida"]))
    if r["veredicto"] in ("DIFIERE", "ERROR", "PETA") and o.get("stderr_cola"):
        r["stderr_host"] = o["stderr_cola"]
    # Hallazgo 8: INFO tras cada prueba. Si uptimeMs decrece, la placa SE REINICIO
    # (un panic, un watchdog) y esa prueba no vale lo que dice su EXITED.
    try:
        info2 = w.info(6)
    except (EOFError, OSError) as e:
        info2 = None
        r["abortar_placa"] = "transporte roto tras %s (INFO): %s" % (p["nombre"], e)
    if info2 is not None:
        r["info_tras"] = {k: info2.get(k) for k in ("uptimeMs", "fsUsedBytes", "fsTotalBytes", "vmHeapBytes")}
        up, up0 = info2.get("uptimeMs"), sello.get("uptime_ultimo")
        if isinstance(up, int) and isinstance(up0, int) and up < up0:
            r["reinicio"] = True
            r["detalle"] += "; la placa SE REINICIO durante la prueba (uptimeMs %d -> %d)" % (up0, up)
            notas.append("%s: la placa SE REINICIO durante la prueba (uptimeMs %d -> %d, resetReason=%s)" % (
                p["nombre"], up0, up, info2.get("resetReason")))
            if r["veredicto"] in VERDES:
                r["veredicto"] = "REINICIO"
        if isinstance(up, int):
            sello["uptime_ultimo"] = up
    elif r["abortar_placa"] is None:
        r["detalle"] += "; INFO tras la prueba no contesta (%s)" % w.errores()
    if r["abortar_placa"]:
        r["basura"] = w.basura_texto()
        return r
    # artefactos: se bajan aunque el veredicto sea malo (son la evidencia)
    if res["exited"] is not None:
        destino = os.path.join(o["work"], etiqueta_placa(sello))
        os.makedirs(destino, exist_ok=True)
        for art in p["artefactos"]:
            pv = r["artefactos_previos"].get(art) or {"existia": [], "dudoso": None}
            a = {"nombre": art, "ok": False, "detalle": "", "local": None, "png": None,
                 "previo": ("existia: %s" % ", ".join(pv["existia"]))
                           if pv["existia"] else "no existia",
                 "dudoso": pv["dudoso"]}
            datos, intentos = None, []
            for remoto in ("/app/" + art, "/" + art, art):
                try:
                    datos, det = w.get(remoto)
                except (EOFError, OSError) as e:
                    datos, det = None, "transporte: %s" % e
                    intentos.append(det)
                    r["abortar_placa"] = "transporte roto bajando %s de %s: %s" % (art, p["nombre"], e)
                    break
                intentos.append(det)
                if datos is not None:
                    a["remoto"] = remoto
                    break
            a["detalle"] = "; ".join(intentos)
            if datos is not None:
                local = os.path.join(destino, art)
                with open(local, "wb") as f:
                    f.write(datos)
                a["ok"] = True
                a["local"] = local
                a["size"] = len(datos)
                a["crc"] = "%08x" % (zlib.crc32(datos) & 0xFFFFFFFF)
                if art.lower().endswith(".shot"):
                    png = local[:-5] + ".png"
                    c = subprocess.run([sys.executable, SHOT2PNG, local, png], capture_output=True)
                    if c.returncode == 0 and os.path.exists(png):
                        a["png"] = png
                        if not a["dudoso"]:
                            r["necesita_ojos"].append(png)
                    else:
                        a["detalle"] += "; shot2png fallo: " + c.stderr.decode("utf-8", "replace")[-300:]
            r["artefactos"].append(a)
        # Un artefacto que no esta es un FALLO, no una nota: GuiShot atrapa su
        # propio RuntimeError e imprime «captura: sin pantalla» con exit 0, y
        # con criterio no-peta eso saldria verde sin captura. La evidencia es
        # parte de la prueba.
        faltan = [a["nombre"] for a in r["artefactos"] if not a["ok"]]
        if faltan and r["veredicto"] in VERDES:
            r["veredicto"] = "SIN ARTEFACTO"
            r["detalle"] += "; falta %s" % ", ".join(faltan)
        # Hallazgo 1: si no se pudo garantizar que el artefacto no estaba antes
        # del RUN, lo bajado puede ser de OTRA tanda: DUDOSO, y sin NECESITA OJOS.
        dudosos = [a["nombre"] for a in r["artefactos"] if a["ok"] and a["dudoso"]]
        if dudosos:
            if r["veredicto"] in VERDES:
                r["veredicto"] = "DUDOSO"
            r["detalle"] += "; artefacto %s DUDOSO (no se pudo garantizar que no existiera antes del RUN)" % ", ".join(dudosos)
    r["basura"] = w.basura_texto()
    return r


# ── informe ──────────────────────────────────────────────────────────────────

def escribir_informe(ruta, tanda):
    L = []
    L.append("")
    L.append("## Tanda %s" % tanda["fecha"])
    L.append("")
    o = tanda["oraculo_info"]
    L.append("- manifiesto: `%s` (%d pruebas listadas%s)" % (
        tanda["lista"], tanda["n_listadas"],
        (", filtro --prueba %s" % tanda["filtro"]) if tanda["filtro"] else ""))
    L.append("- oraculo: jar `%s` (%s) - exe `%s` (%s) - sabor `%s`%s" % (
        o["jar"], o["jar_hora"], o["exe"], o["exe_hora"], o["sabor"],
        " - **--sin-oraculo: el host NO se ejecuto**" if tanda["sin_oraculo"] else ""))
    L.append("- placas conectadas: %d (%s)" % (
        len(tanda["placas"]),
        ", ".join("%s@%s" % (pl["sello"]["boardName"], pl["sello"]["puerto"]) for pl in tanda["placas"]) or "ninguna"))
    L.append("")
    L.append("### Oraculo por prueba (PC)")
    L.append("")
    L.append("| prueba | modulo | crc .mod | tamano | compilacion | host rc | ms host | lineas salida |")
    L.append("|---|---|---|---|---|---|---|---|")
    for p in tanda["pruebas"]:
        oo = tanda["oraculos"][p["nombre"]]
        L.append("| %s | %s | %s | %s | %s | %s | %s | %s |" % (
            p["nombre"], p["modulo"],
            ("%08x" % oo["crc"]) if oo["crc"] is not None else "-",
            oo["size"] if oo["size"] is not None else "-",
            "ok" if oo["compilado"] else "FALLO",
            oo["rc"] if oo["ejecutado"] else "-",
            oo["ms"] if oo["ejecutado"] else "-",
            len(oo["salida"].split("\n")) if oo.get("salida") else "-"))
        if oo.get("error"):
            L.append("")
            L.append("  - **%s**: %s" % (p["nombre"], oo["error"].split("\n")[0]))
    L.append("")
    for pl in tanda["placas"]:
        s = pl["sello"]
        L.append("### Placa %s - %s" % (s["boardName"], s["uniqueId"]))
        L.append("")
        L.append("- sello: boardName=`%s` uniqueId=`%s` serverName=`%s` serverBuild=`%s` capacidades=%s" % (
            s["boardName"], s["uniqueId"], s["serverName"], s["serverBuild"], json.dumps(s["capabilities"])))
        L.append("- imagen: arch=%s variant=%s cpuFreqHz=%s resetReason=%s uptimeMs=%s fs=%s/%s B vmHeapBytes=%s" % (
            s.get("arch"), s.get("variant"), s.get("cpuFreqHz"), s.get("resetReason"), s.get("uptimeMs"),
            s.get("fsUsedBytes"), s.get("fsTotalBytes"), s.get("vmHeapBytes")))
        arr = s.get("arranque")
        if arr:
            L.append("- al conectar: %s%s" % (
                arr[1], " **(habia algo corriendo)**" if arr[0] else (" (no se supo)" if arr[0] is None else "")))
        L.append("- transporte: %s %s" % (s["transporte"], s["puerto"]))
        L.append("- pantalla: %s (%s)" % ("si" if pl["pantalla"][0] else "no", pl["pantalla"][1]))
        L.append("- inventario /lib+/app: %s" % pl["inventario_detalle"])
        if pl.get("error"):
            L.append("- **ERROR de placa**: %s" % pl["error"])
        L.append("")
        L.append("| prueba | veredicto | ms placa | ms host | detalle |")
        L.append("|---|---|---|---|---|")
        for r in pl["resultados"]:
            L.append("| %s | **%s** | %s | %s | %s |" % (
                r["prueba"], r["veredicto"],
                r["ms_placa"] if r["ms_placa"] is not None else "-",
                r["ms_host"] if r["ms_host"] is not None else "-",
                r["detalle"].replace("|", "\\|").replace("\n", " ")))
        L.append("")
        for r in pl["resultados"]:
            if r.get("reinicio"):
                L.append("- **la placa SE REINICIO durante %s** (uptimeMs decrecio; ver notas)" % r["prueba"])
            if r.get("info_tras"):
                it = r["info_tras"]
                L.append("- tras %s: uptimeMs=%s fsUsed=%s vmHeapBytes=%s" % (
                    r["prueba"], it.get("uptimeMs"), it.get("fsUsedBytes"), it.get("vmHeapBytes")))
            if r["veredicto"] not in VERDES + ("SALTADA",):
                # hallazgo 5: la salida que SI llego, aunque no hubiera EXITED
                if r.get("salida_placa") and not r.get("diff"):
                    L.append("<details><summary>%s: salida recibida de la placa (%s)</summary>" % (
                        r["prueba"], "sin EXITED" if not r.get("termino") else "con EXITED"))
                    L.append("")
                    L.append("```")
                    L.append(normalizar(r["salida_placa"]))
                    L.append("```")
                    L.append("</details>")
                    L.append("")
                # hallazgo 6: lo NO-JSON que solto el wire durante la prueba
                if r.get("basura"):
                    L.append("<details><summary>%s: %d lineas NO-JSON del wire (panics, hard faults, avisos)</summary>" % (
                        r["prueba"], len(r["basura"])))
                    L.append("")
                    L.append("```")
                    L.extend(r["basura"])
                    L.append("```")
                    L.append("</details>")
                    L.append("")
                if r.get("stderr_host"):
                    L.append("<details><summary>%s: cola del stderr del host</summary>" % r["prueba"])
                    L.append("")
                    L.append("```")
                    L.append(r["stderr_host"])
                    L.append("```")
                    L.append("</details>")
                    L.append("")
            if r.get("diff"):
                L.append("<details><summary>%s: diff oraculo vs placa</summary>" % r["prueba"])
                L.append("")
                L.append("```diff")
                L.append(r["diff"])
                L.append("```")
                L.append("")
                L.append("oraculo (host):")
                L.append("")
                L.append("```")
                L.append(tanda["oraculos"][r["prueba"]].get("salida") or "")
                L.append("```")
                L.append("")
                L.append("placa:")
                L.append("")
                L.append("```")
                L.append(normalizar(r.get("salida_placa") or ""))
                L.append("```")
                L.append("</details>")
                L.append("")
            for a in r.get("artefactos", []):
                L.append("- artefacto %s de %s: %s%s; antes del RUN: %s%s" % (
                    a["nombre"], r["prueba"],
                    ("bajado a `%s` (%s B, crc %s)" % (a["local"], a.get("size"), a.get("crc"))) if a["ok"] else "NO bajado: " + a["detalle"],
                    (" -> PNG `%s`" % a["png"]) if a.get("png") else "",
                    a.get("previo", "?"),
                    (" — **DUDOSO**: %s" % a["dudoso"]) if a.get("dudoso") else ""))
        # hallazgo 5: «ejecutada» = con EXITED. Lo que arranco y no termino se cuenta aparte.
        terminaron = sum(1 for r in pl["resultados"] if r.get("termino"))
        no_terminaron = sum(1 for r in pl["resultados"] if r.get("arranco") and not r.get("termino"))
        saltadas = sum(1 for r in pl["resultados"] if r["veredicto"] == "SALTADA")
        sin_llegar = len(pl["resultados"]) - terminaron - no_terminaron - saltadas
        L.append("- ejecutadas (con EXITED) %d de %d listadas; no terminaron %d; saltadas %d; no llegaron a arrancar %d" % (
            terminaron, len(pl["resultados"]), no_terminaron, saltadas, sin_llegar))
        L.append("")
        L.append("subido a /app:")
        L.append("")
        if pl["subidos"]:
            L.append("| remoto | origen | tamano | crc32 | resultado |")
            L.append("|---|---|---|---|---|")
            for u in pl["subidos"]:
                L.append("| %s | %s | %s | %s | %s |" % (u["remoto"], u["local"], u["size"], u["crc"], u["detalle"]))
        else:
            L.append("(nada)")
        L.append("")
        if pl["notas"]:
            L.append("notas de dependencias:")
            L.append("")
            for n in pl["notas"]:
                L.append("- " + n)
            L.append("")
    # matriz
    L.append("### Matriz pruebas x placas")
    L.append("")
    cab = ["prueba"] + ["%s@%s" % (pl["sello"]["boardName"], pl["sello"]["puerto"]) for pl in tanda["placas"]]
    L.append("| " + " | ".join(cab) + " |")
    L.append("|" + "---|" * len(cab))
    for p in tanda["pruebas"]:
        fila = [p["nombre"]]
        for pl in tanda["placas"]:
            v = [r["veredicto"] for r in pl["resultados"] if r["prueba"] == p["nombre"]]
            fila.append(v[0] if v else "-")
        L.append("| " + " | ".join(fila) + " |")
    L.append("")
    for pl in tanda["placas"]:
        pn = "%s@%s" % (pl["sello"]["boardName"], pl["sello"]["puerto"])
        for r in pl["resultados"]:
            if r["veredicto"] == "SALTADA":
                L.append("SALTADA: %s en %s: %s  " % (r["prueba"], pn, r["detalle"]))
        for r in pl["resultados"]:
            for png in r.get("necesita_ojos", []):
                L.append("NECESITA OJOS: %s en %s: %s  " % (r["prueba"], pn, png))
    if not tanda["placas"]:
        L.append("(sin placas: no se ejecuto nada en dispositivo)  ")
    L.append("")
    nuevo = not os.path.exists(ruta)
    with open(ruta, "a", encoding="utf-8") as f:
        if nuevo:
            f.write("# Tandas T1 — las placas conducidas\n")
        f.write("\n".join(L) + "\n")
    # el JSON crudo al lado: lista de tandas, se relee y se anade
    rj = os.path.splitext(ruta)[0] + ".json"
    todas = []
    if os.path.exists(rj):
        try:
            with open(rj, encoding="utf-8") as f:
                todas = json.load(f)
            if not isinstance(todas, list):
                todas = [todas]
        except Exception:
            todas = []
    todas.append(tanda)
    with open(rj, "w", encoding="utf-8") as f:
        json.dump(todas, f, ensure_ascii=False, indent=1)
    return rj


# ── main ─────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="T1: una tanda de pruebas, por placa.")
    ap.add_argument("--lista", default=os.path.join(AQUI, "tanda_prueba.json"))
    ap.add_argument("--puertos", default=None, help="auto | COM3,COM14 (sin esto, no se tocan puertos serie)")
    ap.add_argument("--sim", default=None, help="host:puerto del bpvm-sim")
    ap.add_argument("--prueba", default=None, help="solo estas (coma)")
    ap.add_argument("--informe", default=os.path.join(BUILD_TANDA, "tanda.md"))
    ap.add_argument("--sin-oraculo", action="store_true")
    a = ap.parse_args()

    if not os.path.exists(JAR):
        sys.exit("no existe el frontend: " + JAR)
    if not os.path.exists(EXE):
        sys.exit("no existe la VM-C host: " + EXE)
    if a.puertos is None and a.sim is None:
        sys.exit("di --puertos auto|COMx,... y/o --sim host:puerto")

    pruebas = leer_manifiesto(a.lista)
    n_listadas = len(pruebas)
    if a.prueba:
        quiero = [x.strip() for x in a.prueba.split(",")]
        malas = [q for q in quiero if q not in [p["nombre"] for p in pruebas]]
        if malas:
            sys.exit("--prueba: no estan en el manifiesto: " + ", ".join(malas))
        pruebas = [p for p in pruebas if p["nombre"] in quiero]

    fecha = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log("=== tanda %s ===" % fecha)
    log("oraculo: jar %s (%s)" % (JAR, hora_de(JAR)))
    log("         exe %s (%s) sabor %s" % (EXE, hora_de(EXE), sabor_host()))
    tanda = {
        "fecha": fecha, "lista": os.path.relpath(a.lista, RAIZ), "n_listadas": n_listadas,
        "filtro": a.prueba, "sin_oraculo": a.sin_oraculo,
        "oraculo_info": {"jar": os.path.relpath(JAR, RAIZ), "jar_hora": hora_de(JAR),
                         "exe": os.path.relpath(EXE, RAIZ), "exe_hora": hora_de(EXE),
                         "sabor": sabor_host()},
        "pruebas": [{k: p[k] for k in ("nombre", "fuente", "criterio", "timeout_s", "necesita",
                                       "recursos", "artefactos", "modulo")} for p in pruebas],
        "oraculos": {}, "placas": [],
    }

    # 1. el oraculo, una vez por prueba
    log("-- oraculo (PC) --")
    for p in pruebas:
        o = oraculo(p, a.sin_oraculo)
        tanda["oraculos"][p["nombre"]] = o
        if not o["compilado"]:
            log("  %-16s NO COMPILA: %s" % (p["nombre"], o["error"].split("\n")[0]))
        elif a.sin_oraculo:
            log("  %-16s compilado (crc %08x, %d B); host no ejecutado" % (p["nombre"], o["crc"], o["size"]))
        else:
            log("  %-16s rc=%s %5s ms  %d lineas  (crc %08x, %d B)%s" % (
                p["nombre"], o["rc"], o["ms"], len(o["salida"].split("\n")) if o["salida"] else 0,
                o["crc"], o["size"], ("  " + o["error"]) if o["error"] else ""))

    # 2. las placas
    log("-- placas --")
    wires = []
    if a.puertos:
        wires.extend(abrir_puertos(a.puertos))
    if a.sim:
        w = abrir_sim(a.sim)
        if w:
            wires.append(w)
    if not wires:
        log("  ninguna placa contesta: la tanda queda registrada sin placas")

    for w in wires:
        pl = {"sello": None, "pantalla": None, "resultados": [], "subidos": [], "notas": [],
              "inventario_detalle": "", "error": None}
        try:
            # por si quedo algo corriendo (BUSY): KILL y esperar a que la placa
            # este en reposo, no 1,5 s a ciegas (hallazgo 3)
            arranque = w.limpiar_arranque(10)
            log("  al conectar: %s" % arranque[1])
            sello = sello_de(w, arranque)
            pl["sello"] = sello
            pl["pantalla"] = tiene_pantalla(sello)
            log("placa %s id=%s build=%s (%s %s) pantalla=%s [%s]" % (
                sello["boardName"], sello["uniqueId"], sello["serverBuild"],
                sello["transporte"], sello["puerto"],
                "si" if pl["pantalla"][0] else "no", pl["pantalla"][1]))
            inv, det = inventario(w)
            pl["inventario_detalle"] = det
            if inv is None:
                raise RuntimeError(det)
            pl["inventario"] = sorted(inv)
            verificadas = set()          # deps ya comprobadas en ESTA placa
            for p in pruebas:
                log("  %s" % p["nombre"])
                o = tanda["oraculos"][p["nombre"]]
                if "pantalla" in p["necesita"] and pl["pantalla"][0] and not a.sin_oraculo:
                    info = sello.get("info") or {}
                    sw, sh = info.get("screenW") or 0, info.get("screenH") or 0
                    if sw > 0 and sh > 0:
                        clave = "%s@%dx%d" % (p["nombre"], sw, sh)
                        if clave not in tanda["oraculos"]:
                            log("    oraculo del host con --screen=%dx%d" % (sw, sh))
                            tanda["oraculos"][clave] = oraculo(p, False, ("--screen=%dx%d" % (sw, sh),))
                        o = tanda["oraculos"][clave]
                    else:
                        pl["notas"].append("%s: la placa tiene pantalla pero INFO no dice su tamano; "
                                           "oraculo de 480x320" % p["nombre"])
                r = ejecutar_en_placa(w, sello, pl["pantalla"], p, o,
                                      a.sin_oraculo, inv, pl["subidos"], pl["notas"], verificadas)
                pl["resultados"].append(r)
                log("    -> %s: %s" % (r["veredicto"], r["detalle"].split("\n")[0]))
                if r.get("abortar_placa"):
                    # la placa ya no sirve para las siguientes: se dice y se corta
                    raise RuntimeError(r["abortar_placa"])
        except (RuntimeError, EOFError, OSError) as e:
            pl["error"] = str(e)
            log("  ERROR de placa: %s" % e)
            # las pruebas que no llegaron a correr se anotan, no se callan
            hechas = {r["prueba"] for r in pl["resultados"]}
            for p in pruebas:
                if p["nombre"] not in hechas:
                    rr = resultado_vacio(p["nombre"])
                    rr["veredicto"] = "ERROR"
                    rr["detalle"] = "no se llego a ejecutar: %s" % e
                    pl["resultados"].append(rr)
        finally:
            w.close()
        if pl["sello"] is None:
            pl["sello"] = {"boardName": "?", "uniqueId": "?", "serverName": "?", "serverBuild": "?",
                           "capabilities": [], "transporte": w.t.nombre, "puerto": w.t.puerto, "info": {}}
            pl["pantalla"] = (False, "sin INFO")
        tanda["placas"].append(pl)

    # 3. el informe
    os.makedirs(os.path.dirname(os.path.abspath(a.informe)), exist_ok=True)
    rj = escribir_informe(a.informe, tanda)
    log("-- resumen --")
    for pl in tanda["placas"]:
        for r in pl["resultados"]:
            log("  %-10s %-16s %s" % (pl["sello"]["boardName"], r["prueba"], r["veredicto"]))
    log("informe: %s (+ %s)" % (a.informe, rj))
    malos = sum(1 for pl in tanda["placas"] for r in pl["resultados"]
                if r["veredicto"] in ("DIFIERE", "PETA", "COLGADA", "ERROR", "SIN ARTEFACTO", "DUDOSO", "REINICIO"))
    return 1 if malos else 0


if __name__ == "__main__":
    sys.exit(main())
