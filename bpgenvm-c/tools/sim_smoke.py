#!/usr/bin/env python3
# sim_smoke.py — H10: smoke end-to-end del MICRO SIMULADO (tools/bpvm_sim.c).
# Hermano de boardsim_smoke.py: aquél cubre la GESTIÓN DE PLACA, éste el resto del
# wire v1 — FILES (PUT/LIST/STAT/GET/DF/DEL/MKDIR/FORMAT + streaming) y TERMINAL
# (RUN → OUTPUT → EXITED, y KILL). Es el camino EXACTO que recorre el IDE al hacer
# "Run on Device" contra el sim, así que si esto pasa, el IDE funciona sin placa.
# No necesita hardware ni el IDE. Sale 0 si todo pasa.
#
# EL SIM ARRANCA COMO LO ARRANCA EL IDE: con la librería de packs grabada
# (`SimRunner`: un --pack= por cada pack de la librería; aquí, packs/Stdlib.pack).
# Desde H1 P3 (656ec7d5, 20-sep) el parser pone `import Core` en TODO módulo, así
# que hasta el Hello F1 —sin un solo import escrito— llama a Core.__init: con el FS
# vacío y sin packs el RUN moría con «falta el modulo 'Core'», y así estuvo este
# arnés del 20 al 23-sep sin que nadie lo corriera. Que el pack sea la stdlib de
# HOY lo vigila compat/stdlib_pack.sh; aquí se comprueba por el wire con Core.
#
#   python tools/sim_smoke.py
import json, socket, subprocess, sys, os, time, tempfile, shutil, zlib

try: sys.stdout.reconfigure(encoding="utf-8")   # consola Windows cp1252 → UTF-8
except Exception: pass

PORT = 5108
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]  # sin .exe (Linux/mac)
SIM  = os.environ.get("BPVM_SIM") or SIM   # compat.sh check-sim: el sim que construye la red
JAR  = os.path.join(ROOT, "..", "lexer-java", "target", "basicplus-frontend.jar")
# La librería de packs del IDE (la que autodetecta IdePrefs.packsDirEffective en el
# repo) y la stdlib canónica con la que se compara lo que el sim sirve.
STDLIB_PACK = os.path.normpath(os.path.join(ROOT, "..", "packs", "Stdlib.pack"))
STDLIB_DIR  = os.path.normpath(os.path.join(ROOT, "..", "bpstdlib"))

fails = 0
def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

class Wire:
    def __init__(self, sock):
        self.s = sock; self.buf = b""; self.id = 0
    def _line(self):
        while b"\n" not in self.buf:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerró la conexión")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode())
    def _bulk(self, n):
        while len(self.buf) < n:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerró la conexión durante el bulk")
            self.buf += chunk
        data, self.buf = self.buf[:n], self.buf[n:]
        return data
    def call(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self._line()
    def call_bulk(self, typ, data, **fields):
        """Línea JSON con "bulk":N + N bytes crudos detrás (framing del PUT)."""
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self._line()
    def get(self, path):
        """GET = reply con "bulk":N y los bytes pegados detrás."""
        r = self.call("GET", path=path)
        if r.get("type") != "GET_REPLY": return r, b""
        return r, self._bulk(r["bulk"])
    def msg(self):
        """#495 paso 4 — un mensaje CUALQUIERA: la línea y, si anuncia "bulk", sus bytes (en "_bulk")."""
        m = self._line()
        if m.get("bulk"): m["_bulk"] = self._bulk(m["bulk"])
        return m
    def send(self, typ, **fields):
        """Manda sin esperar respuesta; devuelve su id."""
        return self.send_many((typ, fields))[0]
    def send_many(self, *peticiones):
        """Varias peticiones en UN envío (llegan juntas al cable): (type, campos)... → sus ids."""
        ids, out = [], b""
        for typ, fields in peticiones:
            self.id += 1
            f = dict(fields); f.update(type=typ, id=self.id)
            out += (json.dumps(f) + "\n").encode()
            ids.append(self.id)
        self.s.sendall(out)
        return ids
    def pide(self, typ, **fields):
        """Manda y espera SU respuesta, saltando lo demás (los OUTPUT de un programa en marcha)."""
        i = self.send(typ, **fields)
        while True:
            m = self.msg()
            if m.get("id") == i: return m
    def drain_run(self, timeout=30.0):
        """Consume los eventos OUTPUT hasta el EXITED. Devuelve (texto, exited)."""
        out, t0 = [], time.time()
        while time.time() - t0 < timeout:
            m = self._line()
            if m.get("type") == "OUTPUT": out.append(m.get("data", ""))
            elif m.get("type") == "EXITED": return "".join(out), m
        return "".join(out), None

# V7/H8 E1 — ficheros de prueba para el rojo del árbol, construidos aquí (sin depender de nada de fuera).
def mod_cab(magic=b"MOD9", data=0, imports=0, exports=0, code=0, library=0, iface=0, native=0):
    """La cabecera de un .mod (big-endian): v6 32 B, v7 36, v8/v9 44 (MOD_FORMAT §1)."""
    import struct
    ver = magic[3] - ord("0")
    campos = [data, 0xFFFFFFFF, imports, exports, code, library]
    if ver >= 6: campos.append(iface)
    if ver >= 7: campos.append(native)
    if ver >= 8: campos += [0, 0]
    return magic + struct.pack(">%dI" % len(campos), *campos)

def crc16_ccitt(data, crc=0xFFFF):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def pack_cab(nombre, size_total=128, verfmt=1):
    """Un pack que es sólo su cabecera (128 B; bpvm_pack.c: big-endian, crc_cab sobre [0,6)+[8,126))."""
    import struct
    h = bytearray(128)
    h[0:4] = b"BPAK"
    h[4:6] = struct.pack(">H", verfmt)
    h[6:8] = struct.pack(">H", 0x0001)                       # flags: vivo
    h[8:12] = struct.pack(">I", size_total)
    h[12:12 + len(nombre)] = nombre.encode()
    crc = crc16_ccitt(bytes(h[0:6]) + bytes(h[8:126]))
    h[126:128] = struct.pack(">H", crc)
    return bytes(h) + b"\0" * (size_total - 128)

def mdn_cab(abi, version=2, code=0, syms=0, arch=0):
    """La cabecera de un .mdn (nativa, little-endian): v2 = 24 B."""
    import struct
    return b"MDN\0" + struct.pack("<HHIIII", version, abi, code, syms, arch, 0)

def build_hello(dest):
    """Compila samples/Hello.bp con el frontend. None si no se puede (sin java/jar)."""
    src = os.path.join(ROOT, "samples", "Hello.bp")
    if not os.path.exists(JAR) or not os.path.exists(src): return None
    if shutil.which("java") is None: return None
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm"],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, "Hello.mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

# #495 paso 4 — un programa que imprime SIN PARAR (hasta que lo paren): el GET y el LIST troceados tienen que
# abrirse paso entre su salida, y su salida entre ellos.
CHARLA_BP = """module Charla
  function Main()
    var i: integer := 0
    while true do
      print "charla ", i
      i := i + 1
    endwh
  end Main
end Charla
"""

def build_bp(dest, nombre, fuente):
    """Compila un .bp escrito aquí con el frontend. None si no se puede (sin java/jar)."""
    if not os.path.exists(JAR) or shutil.which("java") is None: return None
    src = os.path.join(dest, nombre + ".bp")
    with open(src, "w", encoding="utf-8") as f: f.write(fuente)
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm"],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, nombre + ".mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

def junta_get(msgs):
    """Los GET_PART de una descarga, juntos por `off`: (bytes, seguidos sin hueco, el trozo más grande)."""
    data, seguidos, mayor = bytearray(), True, 0
    for m in msgs:
        if m.get("type") != "GET_PART": continue
        if m.get("off") != len(data): seguidos = False
        b = m.get("_bulk", b"")
        mayor = max(mayor, len(b))
        data += b
    return bytes(data), seguidos, mayor

def hasta_fin(w, fines, eventos=(), limite=30.0):
    """Lee mensajes hasta que cada id de `fines` (id → los tipos que lo acaban) acabe —o con un ERROR— y hayan
    llegado los `eventos` pedidos (p. ej. el EXITED). Devuelve (los de cada id, la secuencia entera)."""
    por_id, todos, quedan, faltan = {}, [], dict(fines), set(eventos)
    t0 = time.time()
    while (quedan or faltan) and time.time() - t0 < limite:
        m = w.msg()
        todos.append(m)
        faltan.discard(m.get("type"))
        i = m.get("id")
        if i in fines:
            por_id.setdefault(i, []).append(m)
            if m.get("type") in fines[i] or m.get("type") == "ERROR": quedan.pop(i, None)
    return por_id, todos

def ultimo(por_id, i):
    return (por_id.get(i) or [{}])[-1]

def main():
    if not os.path.exists(STDLIB_PACK):
        print("FAIL: no está la librería de packs (%s): el IDE la graba en el sim" % STDLIB_PACK)
        return 1
    tmp   = tempfile.mkdtemp(prefix="sim_smoke_")
    flash = os.path.join(tmp, "sim.flash")
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash,
                             "--mem=1M", "--psram=8M", "--flash=4M",
                             "--pack=" + STDLIB_PACK],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        sock = None
        for _ in range(50):
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0); break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            print("FAIL: el sim no aceptó conexión"); return 1
        sock.settimeout(60.0)
        w = Wire(sock)

        print("=== H10 sim smoke (FILES + TERMINAL) ===")

        # 1. handshake: el sim se anuncia como device completo
        r = w.call("HELLO", protoVersion=1, clientName="smoke")
        caps = r.get("capabilities", [])
        check(r.get("type") == "HELLO_REPLY" and "FILES" in caps and "TERMINAL" in caps,
              "HELLO → capabilities con FILES + TERMINAL")

        # 2. INFO refleja el silicio que le pedimos por línea de comandos (H10)
        r = w.call("INFO")
        check(r.get("type") == "INFO_REPLY"
              and r.get("sramBytes") == 1024 * 1024
              and r.get("psramBytes") == 8 * 1024 * 1024
              and r.get("flashBytes") == 4 * 1024 * 1024,
              "INFO → RAM 1M / PSRAM 8M / flash 4M (los del --mem/--psram/--flash)")
        # V7/H2 (#441): el INFO común publica la receta del .mdn SIEMPRE (el PC no
        # tiene: "" y 0), para que el IDE distinga «sin receta» de «firmware viejo».
        check("aotReceta" in r and "aotFlagsCrc" in r
              and r.get("aotReceta") == "" and r.get("aotFlagsCrc") == 0,
              "INFO → aotReceta/aotFlagsCrc presentes (vacíos en el PC: sin receta)")
        # V7/H5 (27-sep): y la ABI de helpers, `mdnAbi`, con la misma regla: 0 donde no hay receta.
        check("mdnAbi" in r and r.get("mdnAbi") == 0,
              "INFO → mdnAbi presente (0 en el PC: sin receta, no ejecuta .mdn)")
        # V7/H8 (28-sep): floatAbi y la zona de packs, del INFO COMÚN (antes, sólo la Pico): el sim arranca con el
        # Stdlib.pack grabado, así que su zona está montada y no puede salir 0.
        check("floatAbi" in r and r.get("packsXipBase", 0) != 0 and r.get("packsBytes", 0) > 0,
              "INFO → floatAbi y la zona de packs montada (packsXipBase/packsBytes)")
        # V7/H8 #602 (1-oct): el bloque de RAM de los motores nativos, del INFO COMÚN con los mismos nombres en las
        # cuatro familias (era la Pico, como sqlite*). El sim no tiene un bloque como el de las placas: 0, 0 y off.
        check(r.get("packRamBase") == 0 and r.get("packRamBytes") == 0 and r.get("packRamStatus") == "off"
              and r.get("packRamKey") == "" and "packRamAsked" in r and r.get("packRamMinMb") == 2
              and "sqliteBytes" not in r,
              "INFO → el bloque de RAM de los motores, con los nombres comunes (packRam*; en el sim, off)")
        # #495 paso 4: la pila que le sobró a `io` en el último RUN (corre el GET y el LIST troceados); el PC no la sabe.
        check(r.get("ioStackFree") == -1, "INFO → ioStackFree presente (-1: sin RUN, y el PC no lleva marca de agua)")

        # 3. FS virgen
        r = w.call("FORMAT", confirm="YES")
        check(r.get("type") == "FORMAT_REPLY", "FORMAT confirm=YES → OK")
        r = w.call("FORMAT")
        check(r.get("code") == "MISSING_CONFIRM", "FORMAT sin confirm → MISSING_CONFIRM")
        r = w.call("LIST")
        check(r.get("type") == "LIST_REPLY" and r.get("entries") == [],
              "LIST tras formatear → vacío")

        # 4. PUT + LIST + STAT + GET (round-trip de bytes)
        blob = bytes(range(256)) * 5          # 1280 B con todos los valores
        r = w.call_bulk("PUT", blob, path="/app/demo/data.bin")
        check(r.get("type") == "PUT_REPLY", "PUT en ruta anidada → crea los directorios")
        r = w.call("LIST")
        ent = {e["name"]: e for e in r.get("entries", [])}
        check("/app/demo/data.bin" in ent and ent["/app/demo/data.bin"]["size"] == len(blob),
              "LIST → ruta COMPLETA y tamaño correcto")
        check(ent.get("/app/demo/data.bin", {}).get("crc", 0) != 0,
              "LIST → trae crc (el IDE lo usa para saltarse PUTs)")
        r = w.call("STAT", path="/app/demo/data.bin")
        check(r.get("size") == len(blob), "STAT → tamaño")
        r, got = w.get("/app/demo/data.bin")
        check(got == blob, "GET → los MISMOS bytes que subimos")
        r = w.call("DF")
        check(r.get("type") == "DF_REPLY" and r.get("fileCount") == 1
              and r.get("usedBytes") == len(blob),
              "DF → 1 fichero, bytes usados")

        # 5. streaming PUT (#294): BEGIN + N×DATA + END
        big = bytes((i * 7) & 0xFF for i in range(40000))
        r = w.call("PUT_BEGIN", path="/app/big.bin", size=len(big))
        check(r.get("type") == "PUT_BEGIN_REPLY", "PUT_BEGIN → sesión abierta")
        step, sent = 8192, 0
        okd = True
        while sent < len(big):
            chunk = big[sent:sent + step]
            r = w.call_bulk("PUT_DATA", chunk)
            sent += len(chunk)
            if r.get("received") != sent: okd = False
        check(okd, "PUT_DATA → 'received' acumulado correcto en cada trozo")
        r = w.call("PUT_END", size=len(big))
        check(r.get("type") == "PUT_END_REPLY" and r.get("size") == len(big),
              "PUT_END → tamaño verificado")
        r, got = w.get("/app/big.bin")
        check(got == big, "GET del fichero subido por streaming → bytes idénticos")

        # 5c. #495 (V7/H8) — LA SUBIDA QUE NO TERMINA SE BORRA (decisión de Eduardo, 1-oct). La sesión tiene el
        # fichero abierto y a medias; la acaban un HELLO, otro PUT_BEGIN, un PUT_END que no cuadra, un verbo que
        # escribe y un RUN, y lo subido se borra. Un LIST entre medias, no.
        trozo = bytes(range(256)) * 2                      # 512 B de los 4096 anunciados
        def a_medias(path):
            r1 = w.call("PUT_BEGIN", path=path, size=4096)
            r2 = w.call_bulk("PUT_DATA", trozo)
            return r1.get("type") == "PUT_BEGIN_REPLY" and r2.get("received") == len(trozo)
        def no_esta(path):
            return w.call("STAT", path=path).get("code") == "NOT_FOUND"
        check(a_medias("/app/rota.bin"), "#495: una subida a medias (512 de 4096 B)")
        r = w.call("LIST")
        r = w.call_bulk("PUT_DATA", trozo)
        check(r.get("received") == 2 * len(trozo), "#495: un LIST entre medias NO acaba la subida")
        r = w.call("HELLO", protoVersion=1, clientName="smoke-495")
        check(r.get("type") == "HELLO_REPLY" and no_esta("/app/rota.bin"),
              "#495: un HELLO (cliente nuevo) acaba la subida y borra lo subido")
        r = w.call_bulk("PUT_DATA", trozo)
        check(r.get("code") == "NO_SESSION", "#495: tras el HELLO no queda sesión: PUT_DATA → NO_SESSION")
        check(a_medias("/app/rota.bin"), "#495: otra subida a medias")
        r = w.call("PUT_BEGIN", path="/app/otra.bin", size=len(trozo))
        check(no_esta("/app/rota.bin"), "#495: otro PUT_BEGIN acaba la primera y la borra")
        w.call_bulk("PUT_DATA", trozo)
        r = w.call("PUT_END", size=len(trozo))
        check(r.get("type") == "PUT_END_REPLY", "#495: ... y la segunda sube entera")
        check(a_medias("/app/rota.bin"), "#495: una subida a medias, y un PUT_END con menos de lo anunciado")
        r = w.call("PUT_END", size=4096)
        check(r.get("code") == "SIZE_MISMATCH" and no_esta("/app/rota.bin"),
              "#495: SIZE_MISMATCH, y el fichero que no está entero, borrado")
        check(a_medias("/app/rota.bin"), "#495: una subida a medias, y un DEL de otro fichero")
        r = w.call("DEL", path="/app/otra.bin")
        check(r.get("type") == "DEL_REPLY" and no_esta("/app/rota.bin"),
              "#495: un verbo que escribe en el FS acaba la subida (y hace lo suyo)")
        check(a_medias("/app/rota.bin"), "#495: una subida a medias, y un RUN")
        r = w.call("RUN", path="/app/NoExiste.mod")
        check(r.get("code") == "NOT_FOUND" and no_esta("/app/rota.bin"), "#495: un RUN acaba la subida")

        # 5d. #495 paso 3 — EL TROZO Y LA VENTANA, anunciados en el HELLO (512 B y 4), una subida que los usa (hasta 4
        # trozos sin acuse, como hará el IDE) y el BULK ESTANCADO: al que le faltan bytes se le da por perdido a los
        # WIRE_V1_BULK_ESTANCADO_MS (3 s) y el cable vuelve a estar en sincronía —sin eso, el segundo intento del IDE se
        # leería como datos—.
        r = w.call("HELLO", protoVersion=1, clientName="smoke-495-3")
        check(r.get("putChunk") == 512 and r.get("putWindow") == 4,
              "#495: el HELLO anuncia el trozo y la ventana (putChunk=%s, putWindow=%s)"
              % (r.get("putChunk"), r.get("putWindow")))
        datos = bytes((i * 13 + 7) & 0xFF for i in range(20000))
        r = w.call("PUT_BEGIN", path="/app/ventana.bin", size=len(datos))
        enviados, en_vuelo, bien = 0, 0, True
        while enviados < len(datos) or en_vuelo:
            while enviados < len(datos) and en_vuelo < 4:
                cacho = datos[enviados:enviados + 512]
                w.id += 1
                w.s.sendall((json.dumps({"type": "PUT_DATA", "id": w.id, "bulk": len(cacho)}) + "\n").encode() + cacho)
                enviados += len(cacho); en_vuelo += 1
            m = w._line()
            if m.get("type") != "PUT_DATA_REPLY": bien = False
            en_vuelo -= 1
        r = w.call("PUT_END", size=len(datos))
        r2, got = w.get("/app/ventana.bin")
        check(bien and r.get("type") == "PUT_END_REPLY" and got == datos,
              "#495: subida de 20000 B en trozos de 512 con 4 en vuelo: llega entera")
        w.call("DEL", path="/app/ventana.bin")
        w.id += 1
        t0 = time.time()
        w.s.sendall((json.dumps({"type": "PUT_DATA", "id": w.id, "bulk": 100}) + "\n").encode() + b"x" * 40)
        m = w._line()
        dt = time.time() - t0
        check(m.get("type") == "FATAL" and "bulk" in m.get("message", "") and 2.5 <= dt <= 6.0,
              "#495: un bulk al que le faltan bytes se da por perdido a los 3 s (%s «%s», %.1f s)"
              % (m.get("type"), m.get("message"), dt))
        r = w.call("PING")
        check(r.get("type") == "PONG", "#495: ... y el cable vuelve a estar en sincronía (PING → PONG)")
        w.s.sendall(b'{"type":"PI')                          # una línea que se queda a medias
        time.sleep(0.6)                                      # > WIRE_V1_ESTANCADA_MS (300 ms)
        r = w.call("PING")
        check(r.get("type") == "PONG",
              "#495: una línea a medias que se estanca se tira, y la siguiente llega entera (como en las placas)")

        # 5e. V7/H8 E1 — EL ROJO DEL ÁRBOL (Eduardo: «en rojo, módulos/packs dañados o con un formato no válido»): el LIST
        # examina la cabecera de cada .mod, .mdn y .pack con la regla de su cargador y marca con "mal" los que no
        # cargaría; los buenos, sin marca. El .mdn bueno se hace con la ABI que la propia VM dice (del motivo del malo).
        casos = {
            "/app/e1/bien.mod":     mod_cab(),
            "/app/e1/corto.mod":    mod_cab(code=100) + b"x" * 50,
            "/app/e1/largo.mod":    mod_cab() + b"x" * 10,
            "/app/e1/viejo.mod":    mod_cab(magic=b"MOD5"),
            "/app/e1/basura.mod":   b"esto no es un modulo de ninguna clase, de verdad",
            "/app/e1/mini.mod":     b"MOD9\0\0\0\0",
            "/app/e1/bien.pack":    pack_cab("Uno"),
            "/app/e1/dos.pack":     pack_cab("Uno") + pack_cab("Dos", 256),
            "/app/e1/tocado.pack":  bytearray(pack_cab("Uno")),
            "/app/e1/cortado.pack": pack_cab("Uno", 512)[:300],
            "/app/e1/nuevo.pack":   pack_cab("Uno", verfmt=2),
            "/app/e1/basura.pack":  b"Z" * 200,
            "/app/e1/otraabi.mdn":  mdn_cab(abi=1),
            "/app/e1/corto.mdn":    mdn_cab(abi=1, code=100),
        }
        casos["/app/e1/tocado.pack"][20] ^= 0xFF                  # un byte del nombre: el CRC deja de cuadrar
        casos["/app/e1/tocado.pack"] = bytes(casos["/app/e1/tocado.pack"])
        for ruta, datos in casos.items():
            w.call_bulk("PUT", datos, path=ruta)
        def marcas():
            r = w.call("LIST")
            return {e["name"]: e.get("mal") for e in r.get("entries", []) if e["name"].startswith("/app/e1/")}
        m = marcas()
        abi = None
        motivo = m.get("/app/e1/otraabi.mdn") or ""
        if "habla la " in motivo:
            abi = int(motivo.split("habla la ")[1].split(":")[0])
        arch = None
        if abi is not None:                                    # la arquitectura que ejecuta, del motivo de una imposible
            w.call_bulk("PUT", mdn_cab(abi=abi, arch=0xFFFF), path="/app/e1/arch.mdn")
            motivo = marcas().get("/app/e1/arch.mdn") or ""
            if "ejecuta la " in motivo:
                arch = int(motivo.split("ejecuta la ")[1].split(")")[0])
        if arch is not None:
            w.call_bulk("PUT", mdn_cab(abi=abi, arch=arch), path="/app/e1/bien.mdn")
            w.call_bulk("PUT", mdn_cab(abi=abi, code=100, arch=arch), path="/app/e1/corto.mdn")
        m = marcas()
        esperado = {
            "/app/e1/bien.mod": None, "/app/e1/bien.pack": None, "/app/e1/dos.pack": None, "/app/e1/bien.mdn": None,
            "/app/e1/corto.mod": "dañado: la cabecera dice 144 B y el fichero tiene 94",
            "/app/e1/largo.mod": "dañado: la cabecera dice 44 B y el fichero tiene 54",
            "/app/e1/viejo.mod": "formato no válido: MOD5",
            "/app/e1/basura.mod": "formato no válido: no es un .mod",
            "/app/e1/mini.mod": "dañado: más corto que su cabecera",
            "/app/e1/tocado.pack": "dañado: la cabecera del pack 1 no cuadra (CRC)",
            "/app/e1/cortado.pack": "dañado: el pack 1 dice 512 B y en el fichero quedan 300",
            "/app/e1/nuevo.pack": "formato no válido: versión de formato 2",
            "/app/e1/basura.pack": "formato no válido: no es un .pack",
            "/app/e1/otraabi.mdn": "formato no válido: ABI de helpers 1",
            "/app/e1/arch.mdn": "formato no válido para esta placa: código de otra arquitectura (65535; esta VM ejecuta",
            "/app/e1/corto.mdn": "dañado: más corto que lo que su cabecera dice",
        }
        for ruta, quiere in esperado.items():
            tiene = m.get(ruta, "(no está en el LIST)")
            bien = (tiene is None) if quiere is None else (tiene is not None and tiene.startswith(quiere))
            check(bien, "E1: %s → %s" % (ruta.split("/")[-1], "sin marca" if quiere is None else "«%s»" % tiene))
        for ruta in list(casos) + ["/app/e1/bien.mdn", "/app/e1/arch.mdn"]:
            w.call("DEL", path=ruta)

        # 5f. #495 paso 4 — EL GET Y EL LIST TROCEADOS Y MEZCLABLES (Eduardo: «el get tiene que trocearse y los paquetes
        # de los diferentes comandos se pueden mezclar [...] no puede ser que una comunicación monopolice la
        # comunicación»). Con "chunked":true, GET_PART (off y un bulk de 512 B como mucho) … GET_END (size, crc), y
        # LIST_PART … LIST_END: un trozo por vuelta, con lo demás entre medias. Sin acuse: aquí recibe el PC.
        gid = w.send("GET", path="/app/big.bin", chunked=True)
        por, _ = hasta_fin(w, {gid: ("GET_END",)})
        datos, seguidos, mayor = junta_get(por.get(gid, []))
        fin = ultimo(por, gid)
        check(fin.get("type") == "GET_END" and datos == big and seguidos and 0 < mayor <= 512
              and fin.get("size") == len(big) and fin.get("crc") == zlib.crc32(big) and "changed" not in fin,
              "#495 paso 4: GET troceado → GET_PART de 512 B como mucho, seguidos por off, y GET_END con tamaño y CRC-32")
        r = w.call("GET", path="/app/no_esta.bin", chunked=True)
        check(r.get("code") == "NOT_FOUND", "#495 paso 4: GET troceado de lo que no está → NOT_FOUND")
        w.call_bulk("PUT", b"", path="/app/vacio.bin")
        gid = w.send("GET", path="/app/vacio.bin", chunked=True)
        por, _ = hasta_fin(w, {gid: ("GET_END",)})
        check([m.get("type") for m in por.get(gid, [])] == ["GET_END"] and ultimo(por, gid).get("size") == 0
              and ultimo(por, gid).get("crc") == 0,
              "#495 paso 4: GET troceado de un fichero vacío → sólo el GET_END (0 B, CRC 0)")
        w.call("DEL", path="/app/vacio.bin")
        # Tres de una vez: dos descargas (la segunda, BUSY: «1 a 1») y un LIST, que sí va con la primera, mezclado.
        a, b, l = w.send_many(("GET", dict(path="/app/big.bin", chunked=True)),
                              ("GET", dict(path="/app/demo/data.bin", chunked=True)),
                              ("LIST", dict(chunked=True)))
        por, todos = hasta_fin(w, {a: ("GET_END",), b: ("GET_END",), l: ("LIST_END",)})
        datos, seguidos, _ = junta_get(por.get(a, []))
        check(datos == big and seguidos and ultimo(por, a).get("type") == "GET_END",
              "#495 paso 4: con otras peticiones detrás, la descarga llega entera")
        check(ultimo(por, b).get("code") == "BUSY", "#495 paso 4: una segunda descarga a la vez → BUSY (Eduardo: «1 a 1»)")
        nombres = sorted(e["name"] for m in por.get(l, []) if m.get("type") == "LIST_PART" for e in m.get("entries", []))
        entero = sorted(e["name"] for e in w.call("LIST").get("entries", []))
        check(ultimo(por, l).get("type") == "LIST_END" and nombres == entero and len(entero) > 0,
              "#495 paso 4: LIST troceado → las mismas entradas que el entero (%d), y LIST_END" % len(entero))
        orden = [(m.get("id"), m.get("type")) for m in todos]
        trozos_a = [k for k, (i, t) in enumerate(orden) if i == a and t == "GET_PART"]
        partes_l = [k for k, (i, t) in enumerate(orden) if i == l and t == "LIST_PART"]
        check(bool(trozos_a) and bool(partes_l) and trozos_a[0] < partes_l[0] < trozos_a[-1],
              "#495 paso 4: el LIST sale ENTRE los trozos de la descarga (mezclados, nadie monopoliza)")
        # Mientras se baja uno, el GET entero de otro: BUSY (un fichero cada vez).
        a, b = w.send_many(("GET", dict(path="/app/big.bin", chunked=True)), ("GET", dict(path="/app/demo/data.bin")))
        por, _ = hasta_fin(w, {a: ("GET_END",), b: ("GET_REPLY",)})
        check(ultimo(por, a).get("type") == "GET_END" and ultimo(por, b).get("code") == "BUSY",
              "#495 paso 4: un GET entero con una descarga en curso → BUSY")
        # Lo que la corta: un verbo que escribe (puede tocar lo que se baja) y un cliente nuevo; el cable, en sincronía.
        w.call_bulk("PUT", b"x" * 10, path="/app/tmp_p4.bin")
        a, d = w.send_many(("GET", dict(path="/app/big.bin", chunked=True)), ("DEL", dict(path="/app/tmp_p4.bin")))
        por, _ = hasta_fin(w, {a: ("GET_END",), d: ("DEL_REPLY",)})
        check(ultimo(por, a).get("code") == "ABORTED" and ultimo(por, d).get("type") == "DEL_REPLY",
              "#495 paso 4: un DEL en mitad de una descarga → la corta (ABORTED) y se hace")
        a, l, h = w.send_many(("GET", dict(path="/app/big.bin", chunked=True)), ("LIST", dict(chunked=True)),
                              ("HELLO", dict(protoVersion=1, clientName="smoke")))
        por, _ = hasta_fin(w, {a: ("GET_END",), l: ("LIST_END",), h: ("HELLO_REPLY",)})
        check(ultimo(por, a).get("code") == "ABORTED" and ultimo(por, l).get("code") == "ABORTED"
              and ultimo(por, h).get("type") == "HELLO_REPLY",
              "#495 paso 4: un HELLO (un cliente nuevo) corta la descarga y el listado del de antes")
        r = w.call("PING")
        check(r.get("type") == "PONG", "#495 paso 4: …y el cable sigue en sincronía")

        # 5b. U3.24 — EL RESTO DEL REPL COMUN.
        #
        # Desde que el sim usa `src/bpvm_repl.c`, cada linea de aqui vale para las
        # CUATRO placas: lo que no se compruebe aqui hay que ir a comprobarlo
        # flasheando. Estos verbos no los tenia el sim (LIST_DIR, RMDIR) o no los
        # miraba nadie en el host (MKDIR, RENAME, SAVE, PING, TIME, LOG_*).
        r = w.call("PING")
        check(r.get("type") == "PONG", "PING → PONG")
        r = w.call("TIME")
        check(r.get("code") == "INVALID_PARAM",
              "TIME sin epochSec → INVALID_PARAM (se DICE, U3.1)")
        r = w.call("TIME", epochSec=1735689600)
        check(r.get("type") == "TIME_REPLY", "TIME con epochSec → OK")

        r = w.call("MKDIR", path="/app/sub")
        check(r.get("type") == "MKDIR_REPLY", "MKDIR → OK")
        r = w.call("LIST_DIR", path="/app")
        names = {e["name"]: e for e in r.get("entries", [])}
        check(r.get("type") == "LIST_DIR_REPLY" and names.get("sub", {}).get("isDir") is True,
              "LIST_DIR → nombres SUELTOS (no rutas) y marca los directorios")
        check("demo" in names and "/" not in "".join(names),
              "LIST_DIR → un solo nivel, sin descender")
        r = w.call("RMDIR", path="/app/sub")
        check(r.get("type") == "RMDIR_REPLY", "RMDIR → OK")
        r = w.call("RMDIR", path="/app/sub")
        check(r.get("code") == "NOT_FOUND", "RMDIR repetido → NOT_FOUND")
        r = w.call("LIST_DIR", path="/no/existe")
        check(r.get("code") == "NOT_FOUND", "LIST_DIR de lo que no hay → NOT_FOUND")

        r = w.call("RENAME", **{"from": "/app/big.bin", "to": "/app/otro.bin"})
        check(r.get("type") == "RENAME_REPLY", "RENAME → OK")
        r = w.call("STAT", path="/app/otro.bin")
        check(r.get("size") == len(big), "RENAME → el fichero está en el nombre nuevo")
        r = w.call("RENAME", **{"from": "/app/otro.bin", "to": "/app/big.bin"})
        check(r.get("type") == "RENAME_REPLY", "RENAME de vuelta")

        r = w.call("SAVE")
        check(r.get("type") == "SAVE_REPLY", "SAVE → OK (littlefs ya persiste)")

        # El PUT tiene tope de buffer en TODAS las familias, y el error se DICE.
        r = w.call_bulk("PUT", b"x" * (96 * 1024), path="/app/enorme.bin")
        check(r.get("code") in ("NO_SPACE", "INVALID_PARAM"),
              "PUT por encima del scratch → error CON NOMBRE, no silencio")
        r = w.call("PING")
        check(r.get("type") == "PONG", "…y el wire sigue en sincronía después (bulk drenado)")

        # 6. DEL
        r = w.call("DEL", path="/app/big.bin")
        check(r.get("type") == "DEL_REPLY", "DEL → OK")
        r = w.call("STAT", path="/app/big.bin")
        check(r.get("code") == "NOT_FOUND", "STAT del borrado → NOT_FOUND")

        # #466 — STAT por NOMBRE de módulo (+ magic): la pregunta del IDE por cada
        # dependencia, contestada por el resolvedor del RUN (proyecto → /app → /lib).
        w.call("MKDIR", path="/lib")
        w.call("MKDIR", path="/app/proj")
        w.call_bulk("PUT", b"MOD7" + bytes(32), path="/lib/Fake.mod")
        w.call_bulk("PUT", b"MOD6" + bytes([1]) * 32, path="/app/proj/Fake.mod")
        w.call_bulk("PUT", b"xyz", path="/app/proj/raw.bin")
        r = w.call("STAT", name="Fake.mod", crc=True)
        check(r.get("path") == "/lib/Fake.mod" and r.get("magic") == "MOD7" and r.get("size") == 36,
              "STAT por nombre → lo encuentra en /lib, con path, magic y tamaño")
        check(r.get("crc") == zlib.crc32(b"MOD7" + bytes(32)),
              "STAT por nombre → el crc, SIN SIGNO (bit 31 puesto: %s)" % r.get("crc"))
        r = w.call("STAT", name="Fake.mod", base="/app/proj")
        check(r.get("path") == "/app/proj/Fake.mod" and r.get("magic") == "MOD6",
              "STAT por nombre con base → el del proyecto PRIMERO (el orden del RUN)")
        r = w.call("STAT", name="Nadie.mod")
        check(r.get("code") == "NOT_FOUND", "STAT por nombre de lo que no hay → NOT_FOUND")
        r = w.call("STAT", path="/app/proj/raw.bin")
        check(r.get("size") == 3 and "magic" not in r, "STAT de lo que no es módulo → sin magic")
        for p in ("/lib/Fake.mod", "/app/proj/Fake.mod", "/app/proj/raw.bin"):
            w.call("DEL", path=p)
        w.call("RMDIR", path="/app/proj")


        # ── #456 — un path LARGO ya no se trunca en silencio ──────────────────
        # Antes esto NO fallaba, que es lo grave: con `char path[64]` en el wire,
        # el PUT escribia en el nombre RECORTADO y contestaba OK. Por eso el caso
        # de abajo (el recortado NO existe) es el que de verdad prueba el arreglo.
        largo = "/app/" + "n" * 200          # 205 > 63 (el tope viejo), < 255 (LFS_NAME_MAX)
        r = w.call_bulk("PUT", b"largo", path=largo)
        check(r.get("type") == "PUT_REPLY", "PUT de un path de 205 caracteres -> OK")
        r = w.call("STAT", path=largo)
        check(r.get("size") == 5, "STAT del path ENTERO -> lo encuentra")
        r, data = w.get(largo)
        check(data == b"largo", "GET del path entero -> el contenido")
        r = w.call("STAT", path=largo[:63])
        check(r.get("code") == "NOT_FOUND", "...y el nombre RECORTADO no existe (no se trunco)")
        # ...y el LISTADO, que si tiene tope (RAM estatica x16), lo DECLARA en vez
        # de emitir el nombre recortado: emitirlo a medias es lo peor de los tres
        # desenlaces, porque el IDE se lo cree y luego pide algo que no existe.
        r = w.call("LIST")
        nombres = [e.get("name", "") for e in (r.get("entries") or [])]
        check(not any(n.startswith("n" * 60) for n in nombres),
              "LIST -> el nombre que no le cabe NO sale recortado")
        check((r.get("omitted") or 0) >= 1,
              "LIST -> ...y lo DECLARA en 'omitted' (dice %r)" % r.get("omitted"))

        r = w.call("RENAME", **{"from": largo, "to": "/app/" + "m" * 200})
        check(r.get("type") == "RENAME_REPLY", "RENAME entre dos paths largos -> OK")
        r = w.call("DEL", path="/app/" + "m" * 200)
        check(r.get("type") == "DEL_REPLY", "DEL de un path largo -> OK")
        # CONTROL del tope que SI queda (la sesion de PUT por trozos guarda el
        # path entre mensajes): por encima tiene que dar error CON NOMBRE.
        r = w.call("PUT_BEGIN", path="/app/" + "z" * 300, size=1)
        check(r.get("code") == "INVALID_PATH",
              "PUT_BEGIN por encima del tope que queda -> error con nombre, no truncado")

        # 7. La LIBRERÍA, como la ve el IDE: #466 pregunta a la placa por cada
        #    dependencia con STAT por nombre, y el resolvedor del RUN contesta. Core
        #    tiene que venir del pack grabado y ser el de bpstdlib/: un pack rancio
        #    enlaza a medias («no exporta Core.__cls_new_StringBuilder()»).
        with open(os.path.join(STDLIB_DIR, "Core.mod"), "rb") as f:
            core_crc = zlib.crc32(f.read())
        r = w.call("STAT", name="Core.mod", crc=True)
        check(r.get("path") == "pack:Stdlib/Core.mod",
              "STAT Core.mod por nombre → del pack grabado (dice %r)" % r.get("path"))
        check(r.get("crc") == core_crc,
              "…y es el Core de bpstdlib/ (crc %s; el de bpstdlib/ %s)" % (r.get("crc"), core_crc))

        # 8. RUN: la VM de verdad ejecutando desde el FS del sim
        mod = build_hello(tmp)
        if mod is None:
            # Un smoke que no puede hacer su prueba principal no da verde.
            check(False, "compilar Hello: no encuentro java o basicplus-frontend.jar — el RUN no se prueba")
        else:
            with open(mod, "rb") as f: data = f.read()
            r = w.call_bulk("PUT", data, path="/app/Hello.mod")
            check(r.get("type") == "PUT_REPLY", "PUT del .mod")
            r = w.call("RUN", path="/app/Hello.mod")
            check(r.get("type") == "RUN_REPLY" and r.get("session", 0) > 0,
                  "RUN → RUN_REPLY con sesión")
            text, exited = w.drain_run()
            check(exited is not None and exited.get("status") == "OK",
                  "EXITED → status OK" + ("" if exited is not None and exited.get("status") == "OK"
                                           else " (dice %s)" % json.dumps(exited, ensure_ascii=False)[:200]))
            nums = [l for l in text.splitlines() if l.strip()]
            check(nums[:3] == ["0", "1", "1"] and nums[-1] == "999",
                  "OUTPUT → la serie de Fibonacci y el 999 final")
            # segunda ejecución: el estado no se arrastra entre RUNs
            r = w.call("RUN", path="/app/Hello.mod")
            text2, exited2 = w.drain_run()
            check(exited2 is not None and exited2.get("status") == "OK"
                  and text2 == text,
                  "2º RUN → misma salida (sin estado arrastrado)")

        # 8b. #495 paso 4 — DURANTE UN RUN, los de lectura troceados: un programa que imprime sin parar y, mientras,
        # una descarga de 40 KB y un LIST. Entre trozo y trozo sale la salida del programa —nadie monopoliza el cable—,
        # y un KILL no espera a que acabe la descarga. Los enteros, LIST_DIR y los que escriben, BUSY.
        p4 = bytes((i * 13 + 7) & 0xFF for i in range(40000))
        r = w.call("PUT_BEGIN", path="/app/p4.bin", size=len(p4))
        for k in range(0, len(p4), 8192):
            w.call_bulk("PUT_DATA", p4[k:k + 8192])
        r = w.call("PUT_END", size=len(p4))
        charla = build_bp(tmp, "Charla", CHARLA_BP)
        if r.get("type") != "PUT_END_REPLY" or charla is None:
            check(False, "#495 paso 4 en RUN: no se pudo preparar (PUT_END %s; Charla %s)" % (r.get("type"), charla))
        else:
            with open(charla, "rb") as f: w.call_bulk("PUT", f.read(), path="/app/Charla.mod")
            r = w.call("RUN", path="/app/Charla.mod")
            check(r.get("type") == "RUN_REPLY", "#495 paso 4: RUN de un programa que imprime sin parar")
            m = w.msg()
            while m.get("type") != "OUTPUT": m = w.msg()          # ya está hablando
            gid = w.send("GET", path="/app/p4.bin", chunked=True)
            por, todos = hasta_fin(w, {gid: ("GET_END",)})
            datos, seguidos, _ = junta_get(por.get(gid, []))
            fin = ultimo(por, gid)
            check(fin.get("type") == "GET_END" and datos == p4 and seguidos and fin.get("crc") == zlib.crc32(p4),
                  "#495 paso 4: GET troceado DURANTE el RUN → llega entero y con su CRC (%s)" % fin.get("type"))
            k = [n for n, m in enumerate(todos) if m.get("type") == "GET_PART"]
            entre = sum(1 for m in todos[k[0]:k[-1]] if m.get("type") == "OUTPUT") if k else 0
            check(entre > 0, "#495 paso 4: …y entre su primer trozo y el último salió la salida del programa (%d OUTPUT)"
                  % entre)
            r = w.pide("STAT", path="/app/p4.bin")
            check(r.get("type") == "STAT_REPLY" and r.get("size") == len(p4), "#495 paso 4: STAT durante el RUN → contesta")
            r = w.pide("GET", path="/app/p4.bin")
            check(r.get("code") == "BUSY", "#495 paso 4: GET ENTERO durante el RUN → BUSY (dejaría a io sin salida ni KILL)")
            r = w.pide("DEL", path="/app/p4.bin")
            check(r.get("code") == "BUSY", "#495 paso 4: DEL durante el RUN → BUSY (los que escriben, no)")
            r = w.pide("LIST_DIR", path="/app")
            check(r.get("code") == "BUSY", "#495 paso 4: LIST_DIR durante el RUN → BUSY (su foto, en la zona del listDir)")
            lid = w.send("LIST", chunked=True)
            por, _ = hasta_fin(w, {lid: ("LIST_END",)})
            nombres = [e["name"] for m in por.get(lid, []) if m.get("type") == "LIST_PART" for e in m.get("entries", [])]
            check(ultimo(por, lid).get("type") == "LIST_END" and "/app/p4.bin" in nombres,
                  "#495 paso 4: LIST troceado durante el RUN → LIST_PART … LIST_END")
            # Una descarga y el KILL detrás, de una vez: el KILL no espera a que acabe, y la descarga sigue tras el RUN.
            gid, kid = w.send_many(("GET", dict(path="/app/p4.bin", chunked=True)), ("KILL", dict()))
            por, todos = hasta_fin(w, {gid: ("GET_END",), kid: ("KILL_REPLY",)}, eventos=("EXITED",))
            orden = [m.get("type") for m in todos]
            datos, seguidos, _ = junta_get(por.get(gid, []))
            check("EXITED" in orden and ultimo(por, kid).get("type") == "KILL_REPLY"
                  and orden.index("KILL_REPLY") < orden.index("GET_END") if "GET_END" in orden and "KILL_REPLY" in orden
                  else False,
                  "#495 paso 4: un KILL con una descarga en curso → KILL_REPLY y EXITED sin esperar a que acabe")
            check(datos == p4 and seguidos and ultimo(por, gid).get("type") == "GET_END",
                  "#495 paso 4: …y la descarga acaba entera después del RUN")
            r = w.call("PING")
            check(r.get("type") == "PONG", "#495 paso 4: …y el cable sigue en sincronía")
            w.call("DEL", path="/app/Charla.mod")
        w.call("DEL", path="/app/p4.bin")

        # 9. errores limpios
        r = w.call("RUN", path="/app/NoExiste.mod")
        check(r.get("code") == "NOT_FOUND", "RUN de un módulo que no está → NOT_FOUND")
        r = w.call("GET", path="/nope")
        check(r.get("code") == "NOT_FOUND", "GET inexistente → NOT_FOUND")

        # 10. el FS PERSISTE entre conexiones (es una imagen en disco, como la flash)
        sock.close()
        sock = socket.create_connection(("127.0.0.1", PORT), timeout=5.0)
        sock.settimeout(30.0)
        w = Wire(sock)
        w.call("HELLO", protoVersion=1, clientName="smoke2")
        r, got = w.get("/app/demo/data.bin")
        check(got == blob, "reconexión: el fichero sigue ahí (FS persistente)")

        print("[status=%s]" % ("OK" if fails == 0 else "FAIL"))
        return 1 if fails else 0
    finally:
        proc.terminate()
        try: proc.wait(timeout=5)
        except Exception: proc.kill()
        shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
