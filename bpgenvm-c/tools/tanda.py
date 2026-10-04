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
    --ensayo-colgada NOMBRE[,NOMBRE]
                     para PROBAR la recuperacion de una placa colgada sin
                     colgarla de verdad (el KILL llega hasta a una native en un
                     bucle sin fin): esas pruebas se tratan como COLGADA sin
                     EXITED al acabar, y la placa SE REINICIA y sigue. Su
                     veredicto sale COLGADA (y la tanda, roja) a proposito.

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
    IDENTICO / NO-PETA / ASERCIONES (verdes; ASERCIONES = criterio "aserciones":
    el programa se juzga por SUS comprobaciones —ninguna linea que empiece por
    ERROR o FALLO, y salida 0—, no por la salida del PC, que depende del
    planificador; V7_IDEAS §4, «no determinista»; V7/H9, 2-oct) - DIFIERE / PETA - FALLO (la salida de la placa
    tiene una linea que empieza por FALLO: el sample dice que algo sale mal,
    aunque salga igual en el PC; V7/H9, 2-oct) - COLGADA (sin EXITED en el
    timeout) - ERROR (no se pudo medir: subida, transporte, dependencia
    sombreada, fallo de CARGA/ENLACE en la placa) - SIN ORACULO - SALTADA -
    SIN ARTEFACTO (verde pero sin la evidencia) - DUDOSO (verde pero el
    artefacto podia ser de otra tanda) - REINICIO (la placa se reinicio
    durante la prueba: uptimeMs decrecio) - SIN NATIVO (una prueba "aot" que
    corrio interpretada: los numeros salen IGUALES, asi que la salida no basta;
    lo dice el campo `nativos` del EXITED) - SIN COMPROBAR (una prueba "aot" en
    una imagen de antes de V7/H5, sin `nativos` en el EXITED) - PARADA (verde) /
    NO PARA: una prueba con "kill_tras_ms": N no tiene que ACABAR sino PARARSE —a los
    N ms se le manda un KILL y tiene que salir KILLED en "kill_max_ms" (2 s si no se
    dice); el detalle dice cuánto tardó el EXITED, fechado al llegar—. NO PARA si acaba
    sola antes, con otro estado o tarde (lo que acaba a los 8 s del KILL no ha parado);
    sin EXITED tras el KILL, COLGADA. El PC sólo la compila (no acabaría). Es lo que
    ninguna prueba había hecho en placa con una native (#580, 3-oct).

EL AOT (V7/H5, 25-sep; V7_IDEAS §H5)
    Una prueba con "aot": true se sube con su codigo nativo DENTRO: por cada
    receta (la `aotReceta` del INFO de la placa) el .mod del oraculo se funde
    UNA vez con AotSingle —el mismo camino que el Run del IDE—, y la placa tiene
    que decir en su EXITED que registro tantos thunks como compilo el
    compilador. Sin receta (el micro simulado, la S3): SALTADA con el motivo.
    "ignorar": [regex...] quita esas lineas de las dos salidas antes de comparar
    (el tiempo que imprime NatV7).
    Dos fallos solo son IDENTICO si fallan IGUAL: mismo exitCode que el rc
    del host (misma tabla, bpvm_exit_code). Un LINK_ERROR o un «load:» /
    «falta el modulo» de la placa es fallo del ARNES, nunca paridad.

MEMORIA APRETADA (V7_IDEAS §4; V7/H9, 2-oct)
    Una prueba con "memoria": "apretada" no es verde hasta haberlo sido en las
    placas que APRIETAN: sin PSRAM y con 1 MB de SRAM o menos, segun su INFO
    (la Pico 2, la C6, la C3). En TODAS las de la tanda —el verde de una no
    exonera a otra que aprieta mas: la C6 da 64 KB de heap a la VM y la Pico,
    251—. El orden de ejecucion lo decide la mesa; el veredicto, las placas que
    aprietan. Si en la tanda no hay ninguna, o alguna no salio verde, la tanda
    NO es verde y el informe lo dice.
    Y una prueba puede decir cuanta memoria NECESITA: "heap_min_kb" (medido;
    Eduardo, 2-oct). En una placa cuya VM da menos (vmHeapBytes del INFO) sale
    SALTADA con los dos numeros, y esa placa no hace de juez de la memoria
    apretada: no puede correrla por diseno. synclisttest pide 80 (con 64 KB de
    heap, «No space in heap»: la C6).

UNA PLACA COLGADA (V7_IDEAS §4; V7/H9, 2-oct)
    Si una prueba no acaba en su plazo y el KILL tampoco da EXITED, la placa
    se RECUPERA en vez de abandonarla: RESET por el wire (se atiende aunque
    corra un programa, #452), volver a abrir el puerto (el USB de la Pico y de
    la C6 se va y vuelve), HELLO, y el log post-mortem (LOG_DUMP) al informe
    como evidencia; y se sigue con las demas pruebas. Si el wire no contesta
    al RESET, en las ESP32 el reinicio por el cable (RTS, como esptool); en la
    Pico y la STM32 no lo hay sin manos: se aborta la placa y se dice que hacer.

LIMPIEZA (Eduardo, 2-oct: «la tanda que no borra lo que sube»)
    Al acabar con una placa se borra lo que la tanda dejo en /app —los
    programas, sus modulos y los artefactos que bajo—. Lo de /lib se queda: es
    la libreria de la placa, puesta al dia, y el informe dice que se subio.

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
    - DEPENDENCIAS: el RUN resuelve pack en ejecucion -> /app -> /lib -> zona
      de packs (V7/H2 #493: sin cwd ni /sys, y la zona ya no contesta al stat
      de /app/X.mod por basename). Mirar el CRC de /lib/X.mod no dice lo que
      va a cargar el RUN si hay una copia en /app: se pregunta con STAT name=
      (el mismo resolvedor) y se exige que resuelva a /lib/X o /app/X con el
      crc local; si no, se sube a /lib y se vuelve a preguntar; si sigue
      resolviendo a otro sitio, «dependencia X sombreada por <path>» = ERROR.
      A /lib y no a /app: una copia en /app TAPA a /lib y desde #493 el
      programa lo dice por OUTPUT, lo que romperia el oraculo de la prueba.
    - /lib de la DK2 (STM32U5G9J-DK2): se VACIA en cada arranque y los modulos
      de la GUI no van embebidos. Por eso las dependencias se comprueban en
      cada tanda y lo que falte se sube a /lib.
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
import tempfile
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
HELLO_SEGS = 8.0          # 24-sep: con 3 s el P4 no llegaba — abrir su CH343 lo reinicia (DTR/RTS) y
                          # tarda más en contestar (el resto de T1: «> 3 s tras un reset en P4/DK2»).
                          # Sólo cuesta tiempo en un puerto que no contesta.

# Placas que se SABE que llevan panel, SOLO para un firmware tan viejo que no
# conteste al INFO. Desde T1 todo build con LVGL publica screenW/screenH en el
# INFO, asi que un INFO sin esos campos ES «sin pantalla» — y esta tabla no vale
# para decidirlo: `stm32u5` es la Nucleo (sin panel) y la DK2 (con panel) con el
# mismo boardName. F1 (12-sep) lanzo las graficas en la Nucleo por fiarse de
# ella: «builtin 135 no soportado». Se anota en el informe como "supuesto".
CON_PANTALLA = ("esp32c6", "esp32p4", "stm32u5")
SIN_PANTALLA = ("rp2350a", "rp2350b", "pico", "esp32s3", "esp32c3", "esp32c5", "host")   # V7/H11: la C5, 1.ª etapa SIN pantalla (LVGL llegará en un pack)


def log(msg):
    """Print en ASCII puro: la consola es cp1252 y no se fia de nadie."""
    print(msg.encode("ascii", "replace").decode("ascii"), flush=True)


# ── transporte: serie o TCP, misma cara ──────────────────────────────────────

def pide_dtr(puerto):
    """¿El dispositivo de este puerto necesita DTR para transmitir? El CDC de TinyUSB (Pico 2,
    Metro: VID 2E8A de Raspberry Pi) no manda nada si el ordenador no lo activa."""
    for p in serial.tools.list_ports.comports():
        if p.device == puerto:
            return p.vid == 0x2E8A
    return False


class TransSerie:
    def __init__(self, puerto, baud=115200):
        self.nombre = "serie"
        self.puerto = puerto
        # 24-sep: DTR y RTS apagados ANTES de abrir, como wire_serie desde el 23-sep: abrir el puerto
        # ya no reinicia las ESP (el EN del puente CH343 de S3/P4, la lógica del USB-Serial-JTAG), que
        # luego tardaban más que HELLO_SEGS en contestar. Y la Pico/Metro, con DTR: sin él no transmite.
        self.s = serial.Serial()
        self.s.port, self.s.baudrate, self.s.timeout = puerto, baud, 0.2
        self.s.dtr = False
        self.s.rts = False
        self.s.open()
        if pide_dtr(puerto):
            self.s.dtr = True
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def read(self, n):
        return self.s.read(n)

    def read_disponible(self):
        """#495: lo que haya en cuanto haya algo (o a los 0,2 s si no llega nada); read(4096) esperaba el plazo entero."""
        return self.s.read(self.s.in_waiting or 1)

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

    def read_disponible(self):
        return self.read(4096)   # recv ya vuelve en cuanto llega algo

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
        el plazo — con BUSY eran 20 s por PUT (hallazgo 4).
        #495 (1-oct): vuelve EN CUANTO llega la respuesta. Antes leía el puerto a vueltas de lines(0.2), y un
        read(4096) con plazo de 0,2 s no vuelve hasta tener 4096 bytes o agotarlo: cada respuesta costaba 0,2 s
        aunque llegara en un milisegundo, y una medida hecha con esto medía su propia espera."""
        self.otros = []
        t0 = time.time()
        lote, self.pend = self.pend, []
        while True:
            for n, m in enumerate(lote):
                if m.get("type") == typ:
                    self.pend = lote[n + 1:]
                    return m
                self.otros.append(m)
                if m.get("type") == "ERROR" and m.get("id") == self.id:
                    self.pend = lote[n + 1:]
                    return None
            if time.time() - t0 >= secs:
                return None
            lote = self._bocado()

    def _bocado(self):
        """Lo que haya en el transporte EN CUANTO haya algo, en mensajes (#495). Si el transporte se cae, lo ya
        recibido se entrega y la vuelta siguiente lanza la excepcion (como lines(), hallazgo 5)."""
        if self.roto is not None:
            raise self.roto
        try:
            c = self.t.read_disponible()
        except (EOFError, OSError) as e:
            self.roto = e
            c = b""
        if c:
            self.buf += c
        out = []
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

    def reabrir(self, segs=25):
        """Tras un reinicio, el transporte otra vez: el puerto USB de la Pico y de la C6 desaparece y vuelve; el de la
        P4 y la DK2 se queda. True si abre y la placa saluda."""
        viejo = self.t
        try:
            viejo.close()
        except Exception:
            pass
        fin = time.time() + segs
        while time.time() < fin:
            time.sleep(1.0)
            try:
                if viejo.nombre == "serie":
                    self.t = TransSerie(viejo.puerto)
                else:
                    host, _, port = viejo.puerto.rpartition(":")
                    self.t = TransTcp(host, int(port))
            except Exception:
                continue
            self.buf, self.pend, self.otros, self.roto, self.session = b"", [], [], None, None
            if self.hola() is not None:
                return True
            try:
                self.t.close()
            except Exception:
                pass
        return False

    def recuperar(self, sello):
        """V7/H9 (V7_IDEAS §4) — una placa COLGADA (el KILL no dio EXITED): RESET por el wire (se atiende aunque corra
        un programa, #452), reabrir, HELLO y el log post-mortem. Si el wire no contesta al RESET, en las ESP32 el
        reinicio POR EL CABLE (RTS 200 ms con DTR apagado, como esptool); en la Pico y la STM32 no lo hay sin manos.
        Devuelve (ok, texto, cola_del_log)."""
        via = "RESET por el wire"
        self.send("RESET")
        if self.esperar("RESET_REPLY", 3) is None:
            placa = sello.get("boardName") or "?"
            if self.t.nombre != "serie" or not placa.startswith("esp32"):
                return False, ("no contesta al RESET por el wire, y %s no se reinicia sin manos: %s" % (
                    placa, "desenchufa y enchufa la placa" if placa.startswith("rp2350")
                    else "reiniciala con la sonda (STM32_Programmer_CLI -hardRst) o su boton")), ""
            via = "reinicio por el cable (RTS; el wire no contesto al RESET)"
            s = self.t.s
            s.dtr = False
            s.rts = True
            s.dtr = s.dtr        # usbser.sys no manda el RTS nuevo sin tocar DTR (esptool)
            time.sleep(0.2)
            s.rts = False
            s.dtr = s.dtr
            time.sleep(0.2)
        if not self.reabrir():
            return False, "%s: la placa no vuelve a contestar en 25 s" % via, ""
        info = self.info(6) or {}
        self.send("LOG_DUMP")
        lg = self.esperar("LOG_DUMP_REPLY", 10) or {}
        cola = "\n".join((lg.get("text") or "").splitlines()[-40:])
        return True, "%s: vuelve (resetReason=%s, uptimeMs=%s)" % (
            via, info.get("resetReason"), info.get("uptimeMs")), cola

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

    def correr(self, path, timeout_s, kill_tras_ms=0):
        """RUN -> RUN_REPLY -> OUTPUT... -> EXITED. Devuelve un dict con
        salida, exited, colgada, error, tipos (lo demas que llego), session,
        rezagados (mensajes de OTRA sesion, descartados), arranco.
        #580: con `kill_tras_ms`, a los N ms del RUN_REPLY (como pronto: una vuelta lee hasta 0,2 s) manda un
        KILL y apunta cuánto tarda el EXITED (`ms_kill`), fechado AL LLEGAR; si el EXITED llega antes, `acabo_antes`.
        Si el transporte se cae a mitad (hallazgo 5) NO se pierde lo recibido:
        vuelve con error + salida parcial + transporte_roto=True."""
        res = {"salida": "", "exited": None, "colgada": False, "error": None,
               "tipos": [], "ms_wire": None, "session": None, "rezagados": [],
               "arranco": False, "transporte_roto": False, "ms_kill": None, "acabo_antes": False}
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
            t_kill = None
            otros = set()
            while time.time() - t0 < timeout_s:
                if kill_tras_ms and t_kill is None and (time.time() - t0) * 1000 >= kill_tras_ms:
                    self.send("KILL")                # #580: la prueba es PARARLA
                    t_kill = time.time()
                if kill_tras_ms:
                    # #580: lo que llegue, EN CUANTO llegue (`_bocado`, como `esperar`, #495). Con lines(), el EXITED se
                    # fechaba al volver un read de 0,2 s: el «ms despues» del KILL salía de ~200 en el sim y en la C3,
                    # y era la espera del cronómetro, no la de la placa.
                    lote, self.pend = self.pend + self._bocado(), []
                else:
                    lote = self.lines(0.3)
                for m in lote:
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
                        if kill_tras_ms:
                            if t_kill is None:
                                res["acabo_antes"] = True
                            else:
                                res["ms_kill"] = int((time.time() - t_kill) * 1000)
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
            "aot": bool(e.get("aot", False)),                # V7/H5: subir con su nativo DENTRO
            "ignorar": list(e.get("ignorar", [])),           # V7/H5: lineas fuera de la comparacion
            "memoria": e.get("memoria", ""),                  # V7/H9: "apretada" — verde solo con la placa que aprieta
            "heap_min_kb": int(e.get("heap_min_kb", 0)),      # V7/H9: lo que la prueba necesita de heap de la VM
            "kill_tras_ms": int(e.get("kill_tras_ms", 0)),    # #580: PARARLA con un KILL a los N ms (0 = no)
            "kill_max_ms": int(e.get("kill_max_ms", 2000)),   # #580: y el EXITED, como mucho a estos ms del KILL
        }
        if p["criterio"] not in ("identico", "no-peta", "aserciones"):
            sys.exit("manifiesto: criterio desconocido '%s' en %s" % (p["criterio"], p["nombre"]))
        if p["kill_tras_ms"] < 0 or p["kill_max_ms"] <= 0:
            sys.exit("manifiesto: kill_tras_ms negativo o kill_max_ms no positivo en %s" % p["nombre"])
        if p["memoria"] not in ("", "apretada"):
            sys.exit("manifiesto: memoria desconocida '%s' en %s (vale \"apretada\")" % (p["memoria"], p["nombre"]))
        bp = os.path.join(RAIZ, p["fuente"])
        if not os.path.exists(bp):
            sys.exit("manifiesto: no existe %s (prueba %s)" % (bp, p["nombre"]))
        for r in p["recursos"]:
            if not os.path.exists(os.path.join(RAIZ, r)):
                sys.exit("manifiesto: no existe el recurso %s (prueba %s)" % (r, p["nombre"]))
        try:
            p["ignorar_re"] = [re.compile(x) for x in p["ignorar"]]
        except re.error as ex:
            sys.exit("manifiesto: 'ignorar' con una regex mala en %s: %s" % (p["nombre"], ex))
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
    cargador si necesita los .mod). Core siempre.
    V7 (24-sep, #530): y los modulos de la APP —hermanos del .bp, como HiloLib de
    HiloUsa—, recorridos igual; su .mod lo deja el oraculo en su carpeta (el
    frontend compila los hermanos) y va a /app con el programa (modulos_de_app)."""
    vistos = []
    cola = ["Core"] + imports_de(bp)
    dir_app = os.path.dirname(bp)
    while cola:
        x = cola.pop(0)
        if x in vistos:
            continue
        vistos.append(x)
        for src in (os.path.join(STDLIB, x + ".bp"), os.path.join(dir_app, x + ".bp")):
            if os.path.exists(src):
                cola.extend(imports_de(src))
                break
    return vistos


def modulos_de_app(deps, work):
    """Las dependencias que NO son de la stdlib y cuyo .mod compilo el oraculo en
    `work`: los modulos de la app, que van a /app junto al programa."""
    return [d for d in deps
            if not os.path.exists(os.path.join(STDLIB, d + ".mod"))
            and os.path.exists(os.path.join(work, d + ".mod"))]


# ── normalizacion (compat.sh filt()) ─────────────────────────────────────────

RE_RUIDO = re.compile(r"INICIANDO|FIN DE|heapStart")


def normalizar(texto, ignorar=()):
    texto = texto.replace("\r\n", "\n").replace("\r", "\n")
    lineas = [l for l in texto.split("\n")
              if not RE_RUIDO.search(l) and not l.startswith("config:")
              and not any(rx.search(l) for rx in ignorar)]      # V7/H5: el "ignorar" de la prueba
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
    if sin_oraculo or p.get("kill_tras_ms"):   # #580: una prueba que se PARA no acabaría en el PC: sólo se compila
        return o
    t0 = time.time()
    try:
        r = subprocess.run([EXE] + list(extra_args) + [p["modulo"] + ".mod"], capture_output=True,
                           cwd=work, timeout=p["timeout_s"])
        o["args"] = list(extra_args)
        o["rc"] = r.returncode
        o["salida"] = normalizar(r.stdout.decode("utf-8", "replace"), p.get("ignorar_re", ()))
        o["salida_cruda"] = r.stdout.decode("utf-8", "replace")
        o["stderr_cola"] = r.stderr.decode("utf-8", "replace")[-600:]
    except subprocess.TimeoutExpired as e:
        o["rc"] = None
        o["salida"] = normalizar((e.stdout or b"").decode("utf-8", "replace"), p.get("ignorar_re", ()))
        o["error"] = "el host no termino en %d s" % p["timeout_s"]
    o["ms"] = int((time.time() - t0) * 1000)
    o["ejecutado"] = True
    return o


# ── el AOT (V7/H5) ───────────────────────────────────────────────────────────

PACKTOOL_DIR = os.path.join(BPGENVM, "build", "packtool")
RE_THUNKS = re.compile(r"(\d+) thunk\(s\)")


def jar_del_ide():
    """El fat-jar del IDE (trae AotBuild y el frontend): el mas reciente de BpIde/target."""
    import glob
    c = [j for j in glob.glob(os.path.join(RAIZ, "BpIde", "target", "BpIde-*.jar"))
         if "original" not in os.path.basename(j)]
    return max(c, key=os.path.getmtime) if c else None


def fundir_aot(p, o, receta):
    """El .mod del oraculo con su nativo DENTRO, para ESA receta: UNA vez por receta (varias placas
    con la misma comparten el fundido). Por el mismo camino que el Run del IDE: AotSingle ->
    AotBuild.buildSingle, con `receta:<NOMBRE>` para que el catalogo siga en Java. Devuelve
    {"mod", "thunks", "crc", "size"} o {"error"}."""
    cache = o.setdefault("aot", {})
    if receta in cache:
        return cache[receta]
    fa = {"receta": receta}
    cache[receta] = fa
    jar = jar_del_ide()
    if not jar:
        fa["error"] = "no encuentro el jar del IDE (BpIde/target/BpIde-*.jar): hace falta para compilar el nativo"
        return fa
    if not os.path.isfile(os.path.join(PACKTOOL_DIR, "AotSingle.class")) or \
            os.path.getmtime(os.path.join(PACKTOOL_DIR, "AotSingle.class")) < \
            os.path.getmtime(os.path.join(BPGENVM, "tools", "packtool", "AotSingle.java")):
        os.makedirs(PACKTOOL_DIR, exist_ok=True)
        jc = subprocess.run(["javac", "-cp", jar, "-d", PACKTOOL_DIR,
                             os.path.join(BPGENVM, "tools", "packtool", "AotSingle.java")],
                            capture_output=True)
        if jc.returncode != 0:
            fa["error"] = "javac AotSingle: " + jc.stderr.decode("utf-8", "replace")[-300:]
            return fa
    d = os.path.join(o["work"], "aot-" + receta)
    os.makedirs(d, exist_ok=True)
    fundido = os.path.join(d, p["modulo"] + ".mod")
    shutil.copyfile(o["mod"], fundido)            # AotSingle funde en el .mod que encuentra en su carpeta
    r = subprocess.run(["java", "-cp", jar + os.pathsep + PACKTOOL_DIR, "AotSingle", p["bp"], d,
                        "receta:" + receta], capture_output=True)
    salida = r.stdout.decode("utf-8", "replace")
    fa["log"] = salida[-1500:]
    thunks = sum(int(m.group(1)) for m in RE_THUNKS.finditer(salida))
    if r.returncode != 0 or "lleva dentro el nativo" not in salida or thunks == 0:
        fa["error"] = "el nativo no compila para %s (rc %s): %s" % (
            receta, r.returncode, (salida + r.stderr.decode("utf-8", "replace"))[-600:])
        return fa
    fa["mod"] = fundido
    fa["thunks"] = thunks
    fa["crc"] = crc_de(fundido)
    fa["size"] = os.path.getsize(fundido)
    return fa


def abi_del_compilador():
    """V7/H5 (27-sep): la ABI de helpers que genera el compilador (MDN_ABI_VERSION de mdn_format.h; que MdnPack diga
    lo mismo lo vigila compat/packs_abi.py). None si no se puede leer."""
    try:
        txt = open(os.path.join(BPGENVM, "src", "mdn_format.h"), encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    m = re.search(r"^#define\s+MDN_ABI_VERSION\s+(\d+)", txt, re.M)
    return int(m.group(1)) if m else None


def juzgar_nativo(r, fa, exited, abi_placa=None):
    """V7/H5: la salida sale IGUAL interpretada (H13-40: el «fib(28) AOT» daba lo mismo que el
    interpretado), asi que una prueba "aot" sólo es verde si la placa dice en su EXITED que registró
    los thunks que compilo el compilador. No se mira el log: con log=0 (lo de fabrica) esa linea
    no se escribe, y un aviso que no distingue «no paso» de «no se apunto» no sirve.
    `abi_placa` (el `mdnAbi` de su INFO, 27-sep): si no es la del compilador, el SIN NATIVO dice por qué."""
    nat = (exited or {}).get("nativos")
    r["aot"] = {"receta": fa["receta"], "thunks": fa["thunks"], "nativos": nat,
                "crc": fa["crc"], "size": fa["size"]}
    if r["veredicto"] not in VERDES:
        return
    if nat is None:
        r["veredicto"] = "SIN COMPROBAR"
        r["detalle"] += ("; la imagen no dice 'nativos' en su EXITED (es de antes de V7/H5): no se sabe si "
                         "corrio el nativo o el interprete, que da los mismos numeros")
    elif nat != fa["thunks"]:
        r["veredicto"] = "SIN NATIVO"
        r["detalle"] += ("; el nativo NO se uso entero: la placa registro %s de %d thunks (%s) y lo demas corrio "
                         "interpretado, que da los mismos numeros" % (nat, fa["thunks"], fa["receta"]))
        mia = abi_del_compilador()
        if abi_placa and mia and abi_placa != mia:
            r["detalle"] += ("; la placa habla la ABI de helpers %d y el compilador genera la %d: su cargador rechaza "
                             "el nativo (regrabala con la imagen de esta version)" % (abi_placa, mia))
    else:
        r["detalle"] += "; nativo %s/%d thunks (%s)" % (nat, fa["thunks"], fa["receta"])


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
        "imageCrc": h.get("imageCrc"),       # V7/H9: el sello de la IMAGEN (None: una imagen de antes)
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
        "aotReceta": info.get("aotReceta") or "",      # V7/H5: con ella se funde el nativo de las pruebas "aot"
        "mdnAbi": info.get("mdnAbi"),                  # V7/H5 (27-sep): la ABI de helpers (None: la imagen no lo dice)
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
    if info:
        # Hay INFO y no trae screenW/H: la imagen no lleva LVGL. Es un DATO de
        # la placa, no una suposicion (todo build con LVGL los publica).
        return False, "INFO sin screenW/screenH: la imagen no lleva LVGL"
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
    """STAT name= : la placa resuelve con bpvm_entry_resolve (basedir -> tal cual
    -> /app -> /lib; desde V7/H2 #493 sin /sys y sin que la zona de packs conteste
    por basename), y dice path y crc. El RUN busca sus dependencias en ese
    mismo orden salvo el «tal cual». Devuelve (path, crc, magic, gen) o (None, None, None, None).
    V7/H9 #624: y el formato (MAGIC) y la versión (`gen`, de la cabecera desde MOD8; -1 si no la dice)."""
    st = w.llamar("STAT", "STAT_REPLY", 10, name=fich, crc=True)
    if st is None:
        return None, None, None, None
    crc = st.get("crc")
    if isinstance(crc, int) and crc != -1:
        crc &= 0xFFFFFFFF
    gen = st.get("gen")
    return st.get("path"), crc, st.get("magic"), (gen if isinstance(gen, int) else -1)


def cabecera_local(ruta):
    """V7/H9 #624 — el formato (MAGIC) y la versión (`gen`, +36, big-endian) de un .mod local; -1 si no la lleva."""
    with open(ruta, "rb") as f:
        h = f.read(44)
    if len(h) < 4 or h[:3] != b"MOD":
        return None, -1
    magic = h[:4].decode("ascii", "replace")
    if len(h) >= 44 and magic[3] in "89":
        return magic, int.from_bytes(h[36:40], "big")
    return magic, -1


def anotar(notas, n):
    """Una nota por placa, no una por prueba (las deps se preguntan en cada prueba)."""
    if n not in notas:
        notas.append(n)


def asegurar_deps(w, deps, inv, subidos, notas, verificadas):
    """Cada dependencia se comprueba COMO LA RESUELVE EL RUN (STAT por nombre,
    hallazgo 2): si resuelve a /lib/X o /app/X con el crc local, vale; si no
    (no resuelve, resuelve a otro sitio, o el crc difiere), se sube a /lib y se
    vuelve a preguntar; si aun asi no resuelve a /lib/X con el crc local, la
    dependencia esta SOMBREADA (por algo en /app) y la prueba es ERROR.

    V7/H2 (#493): se sube a /lib, no a /app. La stdlib vive en /lib; una copia
    en /app la TAPA y desde #493 el programa lo dice por OUTPUT («aviso: 'Str'
    cargado desde /app, tapa al de /lib»), que rompería el oráculo de cada
    prueba. Y la zona de packs ya no contesta al STAT de /app/X.mod: lo que
    diga el STAT es lo que hay en el FS."""
    for d in deps:
        if d in verificadas:
            continue
        fich = d + ".mod"
        local = os.path.join(STDLIB, fich)
        if not os.path.exists(local):
            anotar(notas, "dependencia %s: no es de bpstdlib: modulo de la app, va a /app con el programa" % d)
            continue
        crc_local = crc_de(local)
        path, crc_placa, magic_placa, gen_placa = stat_por_nombre(w, fich)
        if path in ("/lib/" + fich, "/app/" + fich) and crc_placa == crc_local:
            anotar(notas, "dependencia %s: ya en %s (crc igual; resuelto por nombre, como el RUN)" % (d, path))
            verificadas.add(d)
            continue
        # V7/H2 (#493): si solo la tiene un pack grabado (path pack:<P>/X) con el crc
        # local, la placa la ejecuta XIP desde ahi: verificada, y no se sube nada.
        if isinstance(path, str) and path.startswith("pack:") and crc_placa == crc_local:
            anotar(notas, "dependencia %s: en la zona de packs (%s, crc igual): se ejecuta XIP, no se sube" % (d, path))
            verificadas.add(d)
            continue
        crc_txt = ("%08x" % crc_placa) if isinstance(crc_placa, int) and crc_placa >= 0 else str(crc_placa)
        # V7/H9 #624 — la regla de Eduardo (la de Java): en /lib o en un pack, con el MISMO formato y una versión
        # igual o superior a la mínima (la de bpstdlib, contra la que se compila la prueba), la de la placa VALE: no se
        # sube —antes se subía a /lib por el CRC, y /lib se carga en RAM: la C6 sin memoria, #623—. Pero entonces la
        # placa no prueba la stdlib de HOY: se anota, para regrabar el pack o la imagen. (/app no entra: una copia en
        # /app distinta de la de /lib la TAPA y el programa lo dice por OUTPUT, que rompería el oráculo.)
        magic_local, gen_local = cabecera_local(local)
        if (isinstance(path, str) and (path.startswith("/lib/") or path.startswith("pack:"))
                and magic_placa is not None and magic_placa == magic_local
                and gen_placa >= 0 and gen_local >= 0 and gen_placa >= gen_local):
            anotar(notas, "dependencia %s: DESFASE de compilación —la placa tiene en %s la versión %d (>= la mínima %d), "
                          "crc %s != el de hoy %08x: vale y NO se sube, pero la prueba corre con la suya; para probar "
                          "la de hoy, regraba %s—" % (d, path, gen_placa, gen_local, crc_txt, crc_local,
                                                     "el pack" if path.startswith("pack:") else "la imagen"))
            verificadas.add(d)
            continue
        if path is None:
            anotar(notas, "dependencia %s: el RUN no la resuelve (%s): se sube a /lib" % (d, w.errores()))
        elif path not in ("/lib/" + fich, "/app/" + fich):
            anotar(notas, "dependencia %s: el RUN la resuelve a %s (fuera de /lib y /app), crc %s: se sube a /lib"
                         % (d, path, crc_txt))
        elif path.startswith("/lib/"):
            anotar(notas, "dependencia %s: en /lib con crc %s != local %08x (DESFASE, se sube la de bpstdlib a /lib)"
                         % (d, crc_txt, crc_local))
        else:
            # Una copia RANCIA en /app tapa a /lib y desde #493 el programa lo dice por
            # OUTPUT (rompe el oraculo): se BORRA, y la buena va a /lib.
            anotar(notas, "dependencia %s: en /app con crc %s != local %08x (una copia rancia que TAPA a /lib: se borra y se sube a /lib)"
                         % (d, crc_txt, crc_local))
            try:
                w.llamar("DEL", "DEL_REPLY", 10, path="/app/" + fich)
            except Exception as e:
                anotar(notas, "dependencia %s: no se pudo borrar /app/%s (%s)" % (d, fich, e))
        with open(local, "rb") as f:
            data = f.read()
        ok, det = put_con_busy(w, data, "/lib/" + fich)
        subidos.append({"remoto": "/lib/" + fich, "local": os.path.relpath(local, RAIZ),
                        "size": len(data), "crc": "%08x" % crc_local, "ok": ok, "detalle": det})
        if not ok:
            raise RuntimeError("no se pudo subir %s: %s" % (fich, det))
        inv["/lib/" + fich] = len(data)
        path2, crc2, _, _ = stat_por_nombre(w, fich)
        if path2 != "/lib/" + fich or crc2 != crc_local:
            n = "dependencia %s: SOMBREADA por %s (crc %s) aunque se acaba de subir /lib/%s con crc %08x" % (
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


# V7/H9 (2-oct) — la imagen COMPILADA en el PC para cada placa, para comparar su CRC con el `imageCrc` del HELLO (las
# mismas reglas que el firmware: bpvm_repl.h, `image_crc`). boardName -> (carpeta, fichero, como se mide).
IMAGENES = {
    "rp2350a": ("pico/build", "bpvm_pico.elf", "zona3"), "rp2350b": ("pico/build", "bpvm_pico.elf", "zona3"),
    "esp32s3": ("esp32/build", "bpvm_esp32.bin", "bin"), "esp32c3": ("esp32c3/build", "bpvm_esp32c3.bin", "bin"),
    "esp32c5": ("esp32c5/build", "bpvm_esp32c5.bin", "bin"), "esp32c6": ("esp32c6/build", "bpvm_esp32c6.bin", "bin"),
    "esp32p4": ("esp32p4/build", "bpvm_esp32p4.bin", "bin"),
    "stm32u5g9": ("stm32/Discovery_u5g9j/Discovery_u5g9j/Debug", "Discovery_u5g9j.elf", "stm32"),
    "stm32u575": ("stm32/Nucleo_u575b/Nucleo_u575b/Debug", "Nucleo_u575b.elf", "stm32"),
}
ARM_BIN = r"C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin"


def crc_imagen_local(board):
    """(crc, texto) de la imagen compilada en el PC para esta placa, o (None, por que no)."""
    e = IMAGENES.get(board)
    if not e:
        return None, "no se que imagen compila el PC para %s" % board
    f = os.path.join(RAIZ, "bpgenvm-c", e[0], e[1])
    if not os.path.exists(f):
        return None, "no esta %s" % os.path.relpath(f, RAIZ)
    hora = time.strftime("%d-%m %H:%M", time.localtime(os.path.getmtime(f)))
    try:
        if e[2] == "bin":
            data = open(f, "rb").read()
        else:
            def nm(sim):
                out = subprocess.run([os.path.join(ARM_BIN, "arm-none-eabi-nm.exe"), f],
                                     capture_output=True, text=True).stdout
                return next(int(l.split()[0], 16) for l in out.splitlines()
                            if len(l.split()) == 3 and l.split()[2] == sim)
            d = tempfile.mkdtemp(prefix="tanda_img_")
            b = os.path.join(d, "img.bin")
            subprocess.run([os.path.join(ARM_BIN, "arm-none-eabi-objcopy.exe"), "-O", "binary", f, b], check=True)
            img = open(b, "rb").read()
            if e[2] == "zona3":
                data = img[0x14000: nm("__flash_binary_end") - 0x10000000]
            else:
                data = img[:(nm("_sidata") - 0x08000000) + (nm("_edata") - nm("_sdata"))]
    except Exception as ex:
        return None, "no se pudo medir %s: %s" % (os.path.relpath(f, RAIZ), ex)
    return "%08x" % (zlib.crc32(data) & 0xFFFFFFFF), "%s (%s)" % (os.path.relpath(f, RAIZ).replace("\\", "/"), hora)


def aprieta(sello):
    """¿La placa APRIETA de memoria? (V7_IDEAS §4): sin PSRAM y con 1 MB de SRAM o menos, segun su INFO —la Pico 2, la
    C6, la C3—. (bool, motivo)."""
    info = sello.get("info") or {}
    sram, psram = info.get("sramBytes"), info.get("psramBytes")
    if not isinstance(sram, int) or not isinstance(psram, int):
        return False, "INFO sin sramBytes/psramBytes"
    si = (psram == 0 and 0 < sram <= 1024 * 1024)
    return si, "SRAM %d KB, PSRAM %d KB" % (sram // 1024, psram // 1024)


def memoria_apretada(tanda):
    """Las pruebas con "memoria": "apretada" y si alguna placa que APRIETA las dio por verdes. [(prueba, ok, texto)]."""
    out = []
    for p in tanda["pruebas"]:
        if p.get("memoria") != "apretada":
            continue
        # los jueces: las placas que aprietan y la CORRIERON (una SALTADA —por heap_min_kb, por pantalla— no puede)
        en = [(pl, rs) for pl, rs in ((pl, [r for r in pl["resultados"] if r["prueba"] == p["nombre"]])
                                      for pl in tanda["placas"] if (pl.get("aprieta") or (False,))[0])
              if not (rs and rs[0]["veredicto"] == "SALTADA")]
        def nom(pl):
            return "%s@%s" % (pl["sello"]["boardName"], pl["sello"]["puerto"])
        verdes = [nom(pl) for pl, rs in en if rs and rs[0]["veredicto"] in VERDES]
        no = ["%s: %s" % (nom(pl), rs[0]["veredicto"] if rs else "-") for pl, rs in en
              if not (rs and rs[0]["veredicto"] in VERDES)]
        if not en:
            out.append((p["nombre"], False, "NO ES VERDE: ninguna placa de la tanda que aprieta la ha corrido (sin PSRAM "
                                            "y <= 1 MB de SRAM: la Pico 2, la C6, la C3)"))
        elif no:
            out.append((p["nombre"], False, "NO ES VERDE: no salio verde en todas las que aprietan (%s%s)" % (
                ", ".join(no), ("; verde en " + ", ".join(verdes)) if verdes else "")))
        else:
            out.append((p["nombre"], True, "verde en las que aprietan: %s" % ", ".join(verdes)))
    return out


def limpiar_lo_subido(w, subidos, artefactos):
    """Al acabar con una placa (Eduardo, 2-oct: «la tanda que no borra lo que sube»): borra lo que la tanda dejo en
    /app —los programas, sus modulos y los artefactos que bajo—. Lo de /lib se queda: es la libreria de la placa.
    Devuelve las lineas para las notas del informe."""
    notas = []
    rutas = sorted({x["remoto"] for x in subidos if x.get("ok") and x["remoto"].startswith("/app/")})
    borrados, fallos = 0, []
    for r in rutas:
        if w.llamar("DEL", "DEL_REPLY", 10, path=r) is not None:
            borrados += 1
        elif w.codigo_error() != "NOT_FOUND":
            fallos.append("%s (%s)" % (r, w.errores()))
    previos = borrar_artefactos(w, sorted(set(artefactos)))
    malos_art = ["%s: %s" % (a, v["dudoso"]) for a, v in previos.items() if v["dudoso"]]
    lib = sorted({x["remoto"] for x in subidos if x.get("ok") and x["remoto"].startswith("/lib/")})
    notas.append("limpieza: %d fichero(s) de /app borrados%s%s" % (
        borrados, (" y los artefactos (%s)" % ", ".join(sorted(set(artefactos)))) if artefactos else "",
        ("; quedan en /lib, subidos por la tanda: %s" % ", ".join(lib)) if lib else ""))
    if fallos or malos_art:
        notas.append("limpieza INCOMPLETA: %s" % "; ".join(fallos + malos_art))
    return notas


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
    if p.get("kill_tras_ms") and ex is not None:
        # #580: la prueba es PARARLA. Verde sólo si sale KILLED después del KILL.
        est = "EXITED %s exit=%s%s" % (ex.get("status"), ex.get("exitCode"),
                                        (" (%s)" % ex.get("errorMessage")) if ex.get("errorMessage") else "")
        if res.get("acabo_antes"):
            return "NO PARA", "acabo sola antes del KILL de los %d ms: %s" % (p["kill_tras_ms"], est)
        if ex.get("status") == "KILLED":
            ms = res.get("ms_kill")
            if ms is not None and ms > p.get("kill_max_ms", 2000):
                return "NO PARA", "el KILL de los %d ms llego, pero el EXITED tardo %d ms (el maximo, %d): %s" % (
                    p["kill_tras_ms"], ms, p.get("kill_max_ms", 2000), est)
            return "PARADA", "KILL a los %d ms; el EXITED, %s ms despues: %s" % (
                p["kill_tras_ms"], ms, est)
        return "NO PARA", "tras el KILL de los %d ms: %s" % (p["kill_tras_ms"], est)
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
        # (La nota de que la Pico mandaba el ORDINAL del enum como exitCode se fue con el RUN común, V7/H10: las cuatro
        # mandan el 2 de la tabla común.)
        return "ERROR", "fallo del arnes en la placa, no del programa: %s" % estado
    # V7/H9 (2-oct; el aviso del 7-sep: «el runner compara host contra placa, no contra lo esperado») — un sample que
    # se comprueba a sí mismo imprime `FALLO …` cuando algo sale mal; si sale IGUAL en el PC y en la placa, la paridad
    # es verde y el programa está mal. La misma regla que la red (`compat.sh`: «paridad OK, pero el sample dice que
    # algo sale mal»): una línea que empieza por FALLO no es verde nunca.
    if p["criterio"] == "aserciones":
        # V7/H9 (V7_IDEAS §4, «no determinista»): la salida depende del planificador —el reparto entre hilos—, así que
        # el PC no hace de juez. Se juzga por las comprobaciones del propio programa: ninguna línea ERROR/FALLO, salida 0.
        lineas = (res["salida"] or "").splitlines()
        mala = next((l for l in lineas if l.startswith("ERROR") or l.startswith("FALLO")), None)
        if mala is not None:
            return "FALLO", "la salida dice que algo sale mal: %s (%s)" % (mala[:160], estado)
        if st == "OK" and code == 0:
            return "ASERCIONES", "%d comprobaciones OK y ninguna mala (%s)" % (
                sum(1 for l in lineas if l.startswith("OK")), estado)
        return "PETA", estado
    fallo = next((l for l in (res["salida"] or "").splitlines() if l.startswith("FALLO")), None)
    if fallo is not None and (p["criterio"] == "no-peta" or sin_oraculo or o.get("ejecutado")):
        return "FALLO", "la salida dice que algo sale mal: %s (%s)" % (fallo[:160], estado)
    if p["criterio"] == "no-peta":
        if st == "OK" and code == 0:
            return "NO-PETA", estado
        return "PETA", estado
    if sin_oraculo or not o.get("ejecutado"):
        return "SIN ORACULO", estado
    salida_placa = normalizar(res["salida"], p.get("ignorar_re", ()))
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


VERDES = ("IDENTICO", "NO-PETA", "ASERCIONES", "PARADA")


# --ensayo-colgada: las pruebas que se TRATAN como colgadas sin EXITED (ver la cabecera).
ENSAYO_COLGADA = set()


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
    if p.get("heap_min_kb"):
        heap = (sello.get("info") or {}).get("vmHeapBytes")
        if isinstance(heap, int) and heap < p["heap_min_kb"] * 1024:
            r["veredicto"] = "SALTADA"
            r["detalle"] = "necesita %d KB de heap y la VM de esta placa da %d KB (vmHeapBytes del INFO)" % (
                p["heap_min_kb"], heap // 1024)
            return r
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
    fa = None
    mod_a_subir = o["mod"]
    if p["aot"]:
        receta = sello.get("aotReceta") or ""
        if not receta:
            r["veredicto"] = "SALTADA"
            r["detalle"] = ("prueba de AOT y esta placa no tiene receta (aotReceta vacia en su INFO: x86 o Xtensa): "
                            "aqui el codigo nativo no se puede probar")
            return r
        log("    AOT: fundiendo el nativo para %s" % receta)
        fa = fundir_aot(p, o, receta)
        if fa.get("error"):
            r["veredicto"] = "ERROR"
            r["detalle"] = fa["error"]
            return r
        mod_a_subir = fa["mod"]
    w.vaciar_basura()          # hallazgo 6: la basura del wire, POR prueba
    try:
        deps = dependencias(p["bp"])
        asegurar_deps(w, deps, inv, subidos, notas, verificadas)
        # 3-oct (la Nucleo: 504 KB de FS en bloques de 8 KB, y cada .mod ocupa uno): lo que sube ESTA prueba a /app se
        # borra al acabarla (main), no al final de la placa —con 40 pruebas, la Nucleo se llenaba en la 29.ª—. Cada
        # prueba vuelve a subir lo suyo; las dependencias de la stdlib (asegurar_deps) se quedan: van verificadas.
        r["propios"] = []
        for rec in p["recursos"]:
            r["propios"].append("/app/" + os.path.basename(rec))
            subir_fichero(w, os.path.join(RAIZ, rec), r["propios"][-1], subidos)
        for d in modulos_de_app(deps, o["work"]):   # V7 (#530): HiloLib de HiloUsa
            r["propios"].append("/app/" + d + ".mod")
            subir_fichero(w, os.path.join(o["work"], d + ".mod"), r["propios"][-1], subidos)
        r["propios"].append("/app/" + p["modulo"] + ".mod")
        subir_fichero(w, mod_a_subir, r["propios"][-1], subidos)   # V7/H5: el fundido, si es "aot"
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
    log("    RUN /app/%s.mod (timeout %d s%s)" % (p["modulo"], p["timeout_s"],
        (", KILL a los %d ms" % p["kill_tras_ms"]) if p["kill_tras_ms"] else ""))
    w.ultima_prueba = p["nombre"]
    res = w.correr("/app/" + p["modulo"] + ".mod", p["timeout_s"], p["kill_tras_ms"])
    if p["nombre"] in ENSAYO_COLGADA and not res["error"]:
        log("    ENSAYO: se trata como COLGADA sin EXITED (--ensayo-colgada)")
        res["colgada"], res["exited"] = True, None
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
    if fa is not None and res["exited"] is not None:
        juzgar_nativo(r, fa, res["exited"], sello.get("mdnAbi"))
    if res["transporte_roto"]:
        r["abortar_placa"] = "transporte roto durante %s: %s" % (p["nombre"], res["error"])
        r["basura"] = w.basura_texto()
        return r
    if res["colgada"] and res["exited"] is None:
        # V7/H9 (V7_IDEAS §4): ni EXITED en el plazo ni tras el KILL. Antes la placa se arrastraba a la siguiente
        # prueba (BUSY) y se abortaba; ahora se reinicia, se baja su log post-mortem y se sigue.
        log("    COLGADA sin EXITED tras el KILL: se recupera la placa")
        try:
            ok, texto, cola = w.recuperar(sello)
        except (EOFError, OSError) as e:
            ok, texto, cola = False, "el transporte se cayo al recuperarla: %s" % e, ""
        r["detalle"] += "; recuperacion: " + texto
        r["postmortem"] = cola
        notas.append("%s: COLGADA; %s" % (p["nombre"], texto))
        log("    recuperacion: %s" % texto)
        if not ok:
            r["abortar_placa"] = "colgada en %s y no se recupera: %s" % (p["nombre"], texto)
            r["basura"] = w.basura_texto()
            return r
        verificadas.clear()                 # tras un arranque, lo verificado ya no vale (la DK2 vacia /lib)
        sello["uptime_ultimo"] = None       # el reinicio lo hemos pedido: no es un REINICIO de la prueba
    if r["veredicto"] == "DIFIERE" and p["criterio"] == "identico" and o.get("salida") is not None:
        r["diff"] = diff_corto(o["salida"], normalizar(res["salida"], p.get("ignorar_re", ())))
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
        # V7/H12 (4-oct): y las marcas de pila —la de `run`, la de `io` y, en la Pico, la de la tarea del REPL—: con la
        # tarea `run`, cuál de las pruebas hunde cada una, prueba a prueba (son marcas de agua: sólo bajan).
        r["info_tras"] = {k: info2.get(k) for k in ("uptimeMs", "fsUsedBytes", "fsTotalBytes", "vmHeapBytes",
                                                    "runStackFree", "ioStackFree", "vmTaskStackFreeBytes")}
        up, up0 = info2.get("uptimeMs"), sello.get("uptime_ultimo")
        if isinstance(up, int) and isinstance(up0, int) and up < up0:
            r["reinicio"] = True
            # V7/H2 (#493): tras un reinicio, lo verificado ya no vale (la DK2 vacia
            # /lib en cada arranque): se vuelve a preguntar en la siguiente prueba.
            verificadas.clear()
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
        L.append("- sello: boardName=`%s` uniqueId=`%s` serverName=`%s` serverBuild=`%s` imageCrc=`%s` capacidades=%s" % (
            s["boardName"], s["uniqueId"], s["serverName"], s["serverBuild"],
            s.get("imageCrc") or "sin (imagen de antes de V7/H9)", json.dumps(s["capabilities"])))
        if s.get("imagen_local"):
            L.append("- imagen: %s" % s["imagen_local"])
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
                L.append("- tras %s: uptimeMs=%s fsUsed=%s vmHeapBytes=%s pila libre: run=%s io=%s tareaREPL=%s" % (
                    r["prueba"], it.get("uptimeMs"), it.get("fsUsedBytes"), it.get("vmHeapBytes"),
                    it.get("runStackFree"), it.get("ioStackFree"), it.get("vmTaskStackFreeBytes")))
            if r.get("postmortem"):
                L.append("<details><summary>%s: log post-mortem de la placa, tras recuperarla</summary>" % r["prueba"])
                L.append("")
                L.append("```")
                L.append(r["postmortem"])
                L.append("```")
                L.append("</details>")
                L.append("")
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
    for nombre, ok, texto in memoria_apretada(tanda):
        L.append("MEMORIA APRETADA: %s: %s  " % (nombre, texto))
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
    ap.add_argument("--ensayo-colgada", default=None,
                    help="estas pruebas (coma) se tratan como COLGADA sin EXITED: prueba la recuperacion (REINICIA la placa)")
    a = ap.parse_args()
    if a.ensayo_colgada:
        ENSAYO_COLGADA.update(x.strip() for x in a.ensayo_colgada.split(",") if x.strip())

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
                                       "recursos", "artefactos", "modulo", "memoria", "heap_min_kb")} for p in pruebas],
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
            pl["aprieta"] = aprieta(sello)
            # V7/H9: ¿lleva la imagen que hay compilada en el PC?
            if sello.get("imageCrc"):
                local, de = crc_imagen_local(sello["boardName"])
                if local is None:
                    sello["imagen_local"] = "sin comparar: %s" % de
                elif local == sello["imageCrc"]:
                    sello["imagen_local"] = "la COMPILADA en el PC: %s" % de
                else:
                    sello["imagen_local"] = "OTRA: la compilada en el PC (%s) da %s" % (de, local)
                    pl["notas"].append("la placa NO lleva la imagen compilada en el PC: imageCrc %s, %s da %s" % (
                        sello["imageCrc"], de, local))
                log("  imagen: %s" % sello["imagen_local"])
            log("placa %s id=%s build=%s imagen=%s (%s %s) pantalla=%s [%s]" % (
                sello["boardName"], sello["uniqueId"], sello["serverBuild"], sello.get("imageCrc") or "?",
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
                for ruta in r.get("propios", []):   # 3-oct: lo suyo, al acabar (ejecutar_en_placa)
                    if w.llamar("DEL", "DEL_REPLY", 10, path=ruta) is None and w.codigo_error() != "NOT_FOUND":
                        pl["notas"].append("%s: no se pudo borrar %s al acabar (%s)" % (r["prueba"], ruta, w.errores()))
            pl["notas"].extend(limpiar_lo_subido(w, pl["subidos"], [a for p in pruebas for a in p["artefactos"]]))
            log("  " + pl["notas"][-1])
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
                if r["veredicto"] in ("DIFIERE", "PETA", "FALLO", "COLGADA", "ERROR", "SIN ARTEFACTO", "DUDOSO",
                                      "REINICIO", "SIN NATIVO", "SIN COMPROBAR"))
    for nombre, ok, texto in memoria_apretada(tanda):
        log("  memoria APRETADA: %s: %s" % (nombre, texto))
        if not ok:
            malos += 1
    return 1 if malos else 0


if __name__ == "__main__":
    sys.exit(main())
