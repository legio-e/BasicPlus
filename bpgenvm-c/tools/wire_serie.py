"""wire_serie.py — cliente del wire v1 POR SERIE, y que NO es el IDE.

POR QUE EXISTE (#379). La ficha decia que el wire «se desincroniza tras el Stop»
y su paso 1 era: *¿esta colgado el device o el IDE?* — y nunca se hizo, porque no
habia forma de preguntarle a la placa sin el IDE por medio. Con esto, un cuelgue
se parte en dos en diez segundos: si la placa contesta, lo colgado es el IDE.

No pretende ser un cliente completo: son los verbos que hacen falta para
diagnosticar (HELLO, PING, LIST, INFO, RUN, KILL, LOG_DUMP).

    python tools/wire_serie.py COM3 info
    python tools/wire_serie.py COM3 log
    python tools/wire_serie.py COM3 ciclo /app/Bench.mod 8

⚠️ Desde Git Bash, un argumento que empieza por "/" lo convierte MSYS en una ruta
de Windows y la placa contesta NOT_FOUND. Usa `MSYS_NO_PATHCONV=1` o cmd.

⚠️ ABRIR EL PUERTO REINICIA LAS ESP (12-sep): el puente USB-UART del P4 (y el de la
S3) lleva DTR/RTS al EN del chip, como espera esptool, y pyserial los mueve al
abrir y cerrar. Cada invocacion de esta herramienta es un arranque nuevo: el
`uptimeMs` del INFO sale de 2-3 s, el log muestra un boot por comando y lo que un
RUN dejo en RAM (el pack cargado) no sobrevive al siguiente comando. `tanda.py`
no lo sufre porque mantiene UNA conexion. Para encadenar verbos sin reinicio,
hacerlo en un solo proceso.

Verbos de packs y env (F1, 12-sep): `packs` (PACK_LS), `pack <f.pack> [arm|riscv]`
(graba; con arch PODA y SELLA como el panel del IDE, via tools/packtool/PackTool),
`packfmt` (PACK_FORMAT: pide OK a Eduardo), `env [clave valor | clave -]`, `reset`.
"""
import json, sys, time

try:
    import serial
except ImportError:
    sys.exit("falta pyserial — usa el python del ESP-IDF, que ya lo trae")


class Wire:
    def __init__(self, port, baud=115200):
        self.s = serial.Serial(port, baud, timeout=0.2)
        self.buf = b""
        self.id = 0
        self.otros = []
        self.basura = []
        self.pend = []
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def send(self, typ, **f):
        self.id += 1
        f.update(type=typ, id=self.id)
        self.s.write((json.dumps(f) + "\n").encode())
        return self.id

    def lines(self, secs):
        out, t0 = self.pend, time.time()
        self.pend = []
        while time.time() - t0 < secs:
            c = self.s.read(4096)
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
        """Espera un tipo. GUARDA lo demas en self.otros: tirar la respuesta que
        no encaja es como se pierden los diagnosticos (me paso escribiendo esto:
        la placa contestaba ERROR/NOT_FOUND y yo veia 'sin respuesta')."""
        self.otros = []
        t0 = time.time()
        while time.time() - t0 < secs:
            lote = self.lines(0.2)
            for n, m in enumerate(lote):
                if m.get("type") == typ:
                    # la COLA del lote es respuesta tambien: tirarla fue el bug
                    self.pend = lote[n+1:]
                    return m
                self.otros.append(m)
        return None

    def hola(self):
        self.send("HELLO", client="wire_serie", version=1)
        return self.esperar("HELLO_REPLY", 3)

    def close(self):
        self.s.close()


def cmd_info(w, _):
    print(json.dumps(w.esperar("INFO_REPLY", 5) if w.send("INFO") else None, indent=2,
                     ensure_ascii=False))


def cmd_log(w, _):
    w.send("LOG_DUMP")
    r = w.esperar("LOG_DUMP_REPLY", 10)
    print(r.get("text", "") if r else "sin respuesta")


def cmd_list(w, _):
    w.send("LIST")
    r = w.esperar("LIST_REPLY", 10)
    for e in (r.get("entries") or []) if r else []:
        print("  %8s  %s" % (e.get("size"), e.get("name")))


def cmd_put(w, args):
    """Sube un fichero local a la placa. `put <local> [/ruta/remota]`."""
    import os
    local = args[0]
    remoto = args[1] if len(args) > 1 else ("/app/" + os.path.basename(local))
    data = open(local, "rb").read()
    w.id += 1
    cab = json.dumps({"type": "PUT", "id": w.id, "path": remoto, "bulk": len(data)})
    w.s.write((cab + "\n").encode() + data)        # cabecera + bytes CRUDOS detras

    r = w.esperar("PUT_REPLY", 20)
    print("put %s -> %s : %s" % (local, remoto,
          "ok (%d B)" % len(data) if r else "FALLO %s" % [m.get("message") for m in w.otros][:2]))
    return r is not None


def cmd_puts(w, args):
    """#294 - PUT por TROZOS, para ficheros que no caben en el scratch de la placa
    (Gui.mod son 44 KB y el PUT de un tiron los rechaza con «demasiado grande»).
    `puts <local> [/ruta/remota] [trozo]`."""
    import os
    local = args[0]
    remoto = args[1] if len(args) > 1 else ("/app/" + os.path.basename(local))
    trozo = int(args[2]) if len(args) > 2 else 4096
    data = open(local, "rb").read()
    w.send("PUT_BEGIN", path=remoto, size=len(data))
    if w.esperar("PUT_BEGIN_REPLY", 15) is None:
        print("PUT_BEGIN fallo:", [(m.get("code"), m.get("message")) for m in w.otros][:2]); return
    enviados = 0
    while enviados < len(data):
        cacho = data[enviados:enviados + trozo]
        w.id += 1
        cab = json.dumps({"type": "PUT_DATA", "id": w.id, "bulk": len(cacho)})
        w.s.write((cab + chr(10)).encode() + cacho)
        if w.esperar("PUT_DATA_REPLY", 15) is None:
            print("PUT_DATA fallo en %d B" % enviados); return
        enviados += len(cacho)
    w.send("PUT_END")
    r = w.esperar("PUT_END_REPLY", 20)
    print("puts %s -> %s : %s" % (local, remoto,
          ("ok (%s B)" % r.get("size")) if r else "FALLO al cerrar"))

def cmd_run(w, args):
    """Ejecuta un modulo y vuelca su salida hasta el EXITED."""
    mod = args[0]
    w.send("RUN", path=mod)
    if w.esperar("RUN_REPLY", 10) is None:
        print("el RUN no arranco:", [(m.get("code"), m.get("message")) for m in w.otros][:2]); return
    t0 = time.time(); otros = []
    while time.time() - t0 < (float(args[1]) if len(args) > 1 else 30):
        for m in w.lines(0.3):
            if m.get("type") == "OUTPUT": print(m.get("data", ""), end="")
            elif m.get("type") == "EXITED":
                # el POR QUE: tirarlo es como se pierden los diagnosticos.
                print("\n[EXITED %s exit=%s %s ms] %s" % (m.get("status"),
                      m.get("exitCode"), m.get("elapsedMs"),
                      m.get("errorMessage") or ""))
                return
            else:
                otros.append(m.get("type"))
    # el POR QUE: callar aqui es no distinguir "no ha pasado nada" de "se colgo".
    print("")
    print("[sin EXITED tras %s s] tipos que llegaron: %s" % (
          args[1] if len(args) > 1 else 30, sorted(set(otros)) or "ninguno"))
    for b in w.basura[-5:]:
        print("   basura:", b)


def cmd_get(w, args):
    """Baja un fichero y CRONOMETRA (V6/#473 punto 4: ¿abre una vez o una por trozo?).
    `get <remoto> [local]`: con segundo argumento, lo guarda (V6/C1).

    OJO, y me costo una medida invalida: NO se puede usar esperar() aqui. Ese trocea
    por lineas, asi que se COMIA el bulk crudo y lo tiraba a la basura — el control
    (littlefs, que si tiene read_stream) salia peor que el caso sospechoso, que es
    la senal de que el instrumento miente. Aqui se lee el byte crudo a mano."""
    import json as _json
    remoto = args[0]
    w.s.reset_input_buffer(); w.buf = b""
    t0 = time.time()
    w.send("GET", path=remoto)
    # 1. la linea de cabecera, byte a byte hasta el salto
    linea = b""
    tope = time.time() + 15
    while time.time() < tope:
        c = w.s.read(1)
        if not c: continue
        if c == b"\n":
            if linea.strip().startswith(b"{"): break
            linea = b""; continue
        linea += c
    try:
        r = _json.loads(linea.strip().decode("utf-8", "replace"))
    except Exception:
        print("GET %s: sin cabecera valida (%r)" % (remoto, linea[:120])); return
    if r.get("type") != "GET_REPLY":
        print("GET %s: %s %s" % (remoto, r.get("type"), r.get("message", ""))); return
    n = int(r.get("bulk", 0))
    # 2. exactamente n bytes crudos, sin interpretar nada
    datos = b""
    tope = time.time() + 120
    while len(datos) < n and time.time() < tope:
        c = w.s.read(min(4096, n - len(datos)))
        if c: datos += c
    ms = (time.time() - t0) * 1000.0
    kbs = (len(datos) / 1024.0) / (ms / 1000.0) if ms > 0 else 0
    estado = "completo" if len(datos) >= n else ("INCOMPLETO %d/%d" % (len(datos), n))
    print("GET %-22s %7d B  %8.0f ms  %6.1f KB/s  %s" % (remoto, n, ms, kbs, estado))
    # V6/C1 — `get <remoto> <local>` ademas GUARDA (la captura .shot se baja asi).
    # Solo si llego completo: un fichero corto con nombre bueno es el verde falso.
    if len(args) > 1:
        if len(datos) >= n:
            open(args[1], "wb").write(datos)
            print("   guardado en %s" % args[1])
        else:
            print("   NO guardado: incompleto")

def cmd_ciclo(w, args):
    """#379 — run -> stop -> INFO, N veces. Si el wire se desincronizara tras el
    Stop, el INFO de despues no llegaria."""
    mod = args[0] if args else "/app/Bench.mod"
    n = int(args[1]) if len(args) > 1 else 5
    w.send("KILL"); w.lines(1.5)          # por si quedo algo de una prueba anterior
    malos = 0
    for i in range(1, n + 1):
        w.send("RUN", path=mod)
        if w.esperar("RUN_REPLY", 6) is None:
            print("ciclo %d: el RUN no arranco. Llego: %s" % (
                i, [(m.get("type"), m.get("code"), m.get("message")) for m in w.otros][:3]))
            malos += 1
            continue
        time.sleep(0.6)                    # que este calculando de verdad
        t0 = time.time(); w.send("KILL"); ex = w.esperar("EXITED", 12)
        t_kill = (time.time() - t0) * 1000
        t1 = time.time(); w.send("INFO"); inf = w.esperar("INFO_REPLY", 6)
        t_info = (time.time() - t1) * 1000
        if ex is None or inf is None:
            malos += 1
        print("ciclo %d: EXITED %-8s (%4.0f ms)  INFO %-3s (%4.0f ms)  uptime=%s" % (
            i, (ex.get("status") if ex else "SIN RESPUESTA"), t_kill,
            ("ok" if inf else "NO"), t_info, (inf.get("uptimeMs") if inf else "-")))
    print("--- %d ciclos, %d con fallo ---" % (n, malos))


def cmd_packs(w, _):
    """Que packs hay grabados (PACK_LS)."""
    w.send("PACK_LS")
    r = w.esperar("PACK_LS_REPLY", 10)
    if r is None:
        print("PACK_LS:", [(m.get("code"), m.get("message")) for m in w.otros][:2]); return
    print("zona %s B, libres %s, cadena %s, %s pack(s)" % (r.get("regionSize"), r.get("free"),
          "ok" if r.get("chainOk") else "ROTA", r.get("count")))
    for pk in r.get("packs") or []:
        print("  %-12s v%-8s %8s B  %s ficheros" % (pk.get("name"), pk.get("version"), pk.get("size"), pk.get("files")))


def cmd_pack(w, args):
    """Graba un .pack en la zona de packs (F1, 12-sep) por el MISMO camino que el
    panel de Packs del IDE: `pack <fichero.pack> [arch]`. Con `arch` (arm | riscv)
    se hace lo que hace el IDE con un pack que lleva motor nativo — PODAR las
    entradas de las otras familias (el tamano final se declara en el BEGIN) y
    SELLAR el motor para la direccion que la placa elige (flashAddr/ramBase del
    BEGIN_REPLY) — llamando a PackBurn.podar/sellar del frontend via un PackTool
    de scratchpad (`java -cp lexer-java/target/classes;pack/target/classes`).
    Sin `arch` se graba tal cual: vale para un pack SIN nativo. Un pack universal
    con .npk grabado tal cual NO sirve: la placa lo ve «SIN realojar».
    Variables: PACKTOOL_CP y PACKTOOL_DIR (donde este PackTool.class)."""
    import os, subprocess, tempfile
    arch = args[1] if len(args) > 1 else None
    raiz = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    cp = os.environ.get("PACKTOOL_CP", os.path.join(raiz, "lexer-java", "target", "classes") + ";"
                        + os.path.join(raiz, "pack", "target", "classes"))
    # PackTool.java vive en tools/packtool/ y se compila a build/packtool/ si falta
    pt = os.environ.get("PACKTOOL_DIR", os.path.join(raiz, "bpgenvm-c", "build", "packtool"))
    if not os.path.isfile(os.path.join(pt, "PackTool.class")):
        os.makedirs(pt, exist_ok=True)
        jc = subprocess.run(["javac", "-cp", cp, "-d", pt,
                             os.path.join(raiz, "bpgenvm-c", "tools", "packtool", "PackTool.java")],
                            capture_output=True, text=True)
        if jc.returncode != 0: print("javac PackTool:", jc.stderr[-400:]); return
    tmpd = tempfile.mkdtemp(prefix="pack_")
    if arch:
        podado = os.path.join(tmpd, "podado.pack")
        out = subprocess.run(["java", "-cp", cp + ";" + pt, "PackTool", "podar", args[0], arch, podado],
                             capture_output=True, text=True)
        print(out.stdout.strip()); 
        if out.returncode != 0: print(out.stderr[-400:]); return
        img = open(podado, "rb").read()
    else:
        img = open(args[0], "rb").read()
    w.send("PACK_BURN_BEGIN", size=len(img))
    r = w.esperar("PACK_BURN_BEGIN_REPLY", 15)
    if r is None:
        print("PACK_BURN_BEGIN:", [(m.get("code"), m.get("message")) for m in w.otros][:2]); return
    if arch:
        flash, ram = r.get("flashAddr"), r.get("ramBase")
        if not flash:
            print("la placa no dice flashAddr en el BEGIN: no se puede sellar un motor nativo"); return
        sellado = os.path.join(tmpd, "sellado.pack")
        out = subprocess.run(["java", "-cp", cp + ";" + pt, "PackTool", "sellar", args[0], arch,
                              str(flash), str(ram or 0), sellado], capture_output=True, text=True)
        print(out.stdout.strip())
        if out.returncode != 0: print(out.stderr[-400:]); return
        sell = open(sellado, "rb").read()
        if len(sell) != len(img):
            print("el sellado cambio el tamano (%d -> %d): no se envia" % (len(img), len(sell))); return
        img = sell
    trozo = int(r.get("chunkMax") or 1024)
    enviados = 0
    while enviados < len(img):
        cacho = img[enviados:enviados + trozo]
        w.id += 1
        cab = json.dumps({"type": "PACK_BURN_DATA", "id": w.id, "bulk": len(cacho)})
        w.s.write((cab + chr(10)).encode() + cacho)
        if w.esperar("PACK_BURN_DATA_REPLY", 20) is None:
            print("PACK_BURN_DATA fallo en %d B:" % enviados,
                  [(m.get("code"), m.get("message")) for m in w.otros][:2]); return
        enviados += len(cacho)
    w.send("PACK_BURN_END")
    r = w.esperar("PACK_BURN_END_REPLY", 30)
    print("pack %s : %s" % (args[0], ("grabado en offset %s (%d B)" % (r.get("offset"), len(img))) if r
          else "FALLO al cerrar %s" % [(m.get("code"), m.get("message")) for m in w.otros][:2]))


def cmd_env(w, args):
    """`env` lista el entorno de la placa; `env clave valor` fija una; `env clave -` la borra."""
    if not args:
        w.send("ENV_LS")
        r = w.esperar("ENV_LS_REPLY", 10)
        for e in (r.get("entries") or r.get("vars") or []) if r else []:
            print("  %s=%s" % (e.get("key"), e.get("value")))
        return
    if len(args) == 2 and args[1] == "-":
        w.send("ENV_DEL", key=args[0]); typ = "ENV_DEL_REPLY"
    else:
        w.send("ENV_SET", key=args[0], value=" ".join(args[1:])); typ = "ENV_SET_REPLY"
    r = w.esperar(typ, 10)
    print("%s %s: %s" % (typ[:-6], args[0], "ok" if r else "FALLO %s" % [(m.get("code"), m.get("message")) for m in w.otros][:2]))


def cmd_packformat(w, _):
    """PACK_FORMAT: deja la zona de packs VACIA (borra todo lo grabado). Pide
    confirm=YES en el wire; aqui la confirmacion es haberlo tecleado."""
    w.send("PACK_FORMAT", confirm="YES")
    r = w.esperar("PACK_FORMAT_REPLY", 120)
    print("packformat:", "zona vacia" if r else "FALLO %s" % [(m.get("code"), m.get("message")) for m in w.otros][:2])


def cmd_reset(w, _):
    """El verbo RESET del wire (reinicia la placa; el USB puede irse y volver)."""
    w.send("RESET")
    r = w.esperar("RESET_REPLY", 5)
    print("reset:", "aceptado" if r else "sin RESET_REPLY (la placa puede haber reiniciado antes de contestar)")


def cmd_packfmt(w, _):
    """PACK_FORMAT: deja la zona de packs VACIA (borra todos). Pide OK a Eduardo antes."""
    w.send("PACK_FORMAT", confirm="YES")
    r = w.esperar("PACK_FORMAT_REPLY", 60)
    print("PACK_FORMAT:", "ok" if r else "FALLO %s" % [(m.get("code"), m.get("message")) for m in w.otros][:2])


COMANDOS = {"info": cmd_info, "log": cmd_log, "list": cmd_list, "ciclo": cmd_ciclo,
            "put": cmd_put, "puts": cmd_puts, "run": cmd_run, "get": cmd_get,
            "packs": cmd_packs, "pack": cmd_pack, "packfmt": cmd_packfmt,
            "env": cmd_env, "reset": cmd_reset}

if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[2] not in COMANDOS:
        sys.exit(__doc__ + "\ncomandos: " + " ".join(COMANDOS))
    w = Wire(sys.argv[1])
    if w.hola() is None:
        print("(sin HELLO_REPLY — la placa no contesta o no es el puerto del wire)")
    COMANDOS[sys.argv[2]](w, sys.argv[3:])
    w.close()
