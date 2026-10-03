#!/usr/bin/env python3
# sd_smoke.py — V7/H3 (24-sep): LA SD DEL MICRO SIMULADO, por el wire. El sim con `--sd=IMAGEN` registra un
# fichero como tarjeta (el mismo contrato de bloque que el SPI de la Metro o el SDMMC del P4) y todo lo de
# encima es el código de las placas: el registro común (`bpvm_sd_placa`), FAT y los dos verbos. Tres sims:
#   · CON tarjeta (una imagen de `fat_imagen.py`: MBR + FAT16 en el bloque 2048, etiqueta y un fichero):
#     SD_INFO la identifica y lee el MBR; el arranque ya la montó; SD_MOUNT da la etiqueta y el fichero; y el
#     FS de la fachada lee, escribe y lista en /sd, y lo escrito SIGUE ahí tras volver a montar;
#   · SIN tarjeta: los dos verbos dicen que no hay SD, con el motivo —no «type no implementado»—;
#   · con una imagen ROTA (sin FS): SD_MOUNT falla con el motivo y el LBA, y el wire sigue vivo;
#   · y un PROGRAMA que copia dentro de la tarjeta (#473, 236): con la stdlib del pack del IDE y el compilador.
#
#   python tools/sd_smoke.py
import json, os, shutil, socket, subprocess, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sim_smoke import Wire, STDLIB_PACK, STDLIB_DIR, JAR   # noqa: E402 — el mismo cable (con drain_run) y el mismo compilador

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

HERE = os.path.dirname(os.path.abspath(__file__))
SIM = os.path.join(HERE, "..", "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]
SIM = os.environ.get("BPVM_SIM") or SIM
PORT = 5111

fails = 0
def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

# #473 (lote D, 236) — el programa de la sección 4: copia dentro de la tarjeta (ningún import más que IO; Core va solo).
COPIA_SD = """module CopiaSd
  import IO

  function Main()
    IO.copyFile("/sd/HOLA.TXT", "/sd/COPIA.TXT")
    print "copia=[" + readFile("/sd/COPIA.TXT") + "]"
    IO.copyFile("/sd/grande.bin", "/sd/grande2.bin")
    print "grande2=" + IO.fileSize("/sd/grande2.bin")
    IO.copyFile("/sd/HOLA.TXT", "/sd/encima.bin")
    try
      IO.copyFile("/sd/no_esta.txt", "/sd/x.txt")
      print "MAL: copio un fichero que no existe"
    catch e: RuntimeError
      print "no_esta: lanzo"
    endtry
  end Main
end CopiaSd
"""

def arranca(tmp, nombre, extra):
    """Un sim en una placa virgen, con `extra` en la línea de órdenes. Devuelve (proceso, wire)."""
    flash = os.path.join(tmp, nombre + ".flash")
    proc = subprocess.Popen([SIM, str(PORT), flash, "4194304"] + extra,
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    for _ in range(80):
        try:
            sock = socket.create_connection(("127.0.0.1", PORT), timeout=2.0)
            w = Wire(sock)
            w.call("HELLO", protoVersion=1, clientName="sd_smoke")
            return proc, w
        except OSError:
            time.sleep(0.1)
    proc.kill()
    raise RuntimeError("el sim no acepta conexión")

def para(proc):
    proc.terminate()
    try: proc.wait(timeout=3)
    except subprocess.TimeoutExpired: proc.kill()

def main():
    tmp = tempfile.mkdtemp(prefix="sd_smoke_")
    img = os.path.join(tmp, "tarjeta.img")
    subprocess.run([sys.executable, os.path.join(HERE, "fat_imagen.py"), img], check=True,
                   stdout=subprocess.DEVNULL)
    print("=== sd_smoke (V7/H3: la SD del micro simulado, por el wire) ===")

    # ── 1. CON tarjeta ───────────────────────────────────────────────────────────────────────────────
    proc, w = arranca(tmp, "con", ["--sd=" + img])
    try:
        r = w.call("SD_INFO")
        check(r.get("type") == "SD_INFO_REPLY" and r.get("ok") is True and r.get("medio") == "fichero",
              "SD_INFO con tarjeta → ok, por el medio «fichero» (dice %r, %r)" % (r.get("ok"), r.get("motivo")))
        check(r.get("producto") == "BPSIM" and r.get("oem") == "BP"
              and r.get("anno") == 2026 and r.get("mes") == 9 and r.get("version") == 2,
              "SD_INFO → la identidad de la tarjeta, campo a campo")
        # #550 — el PSN es de 32 bits SIN signo; el de la tarjeta del sim pasa de 2^31, como el de una de verdad
        # (la SanDisk de la Metro: 0xFDB1…). Con `long` de 32 bits (las placas, MinGW) salía NEGATIVO.
        check(r.get("serie") == 0xFDB1A2C3, "SD_INFO → la serie, sin signo: %r (esperado %d)" % (r.get("serie"), 0xFDB1A2C3))
        check(r.get("bloques") == 8 * 1024 * 1024 // 512, "SD_INFO → la capacidad: %r bloques" % r.get("bloques"))
        check(r.get("traza") == "", "SD_INFO → sin traza: el medio no la tiene (un «00 00…» diría «línea flotante»)")
        check(r.get("leeSector0") is True and r.get("firma") == "55 AA" and r.get("clase") == "MBR",
              "SD_INFO → lee el sector 0: 55 AA, una TABLA DE PARTICIONES")
        check(r.get("particiones") == 1 and r.get("parteLba") == 2048 and r.get("parteTipo") == 4
              and r.get("oemFs") == "BPSIM1.0",
              "SD_INFO → la partición: 1, en el bloque 2048, FAT16, formateada por «BPSIM1.0»")

        # el arranque ya la montó: /sd está sin pedir nada
        r = w.call("LIST_DIR", path="/sd")
        nombres = {e.get("name") for e in r.get("entries", [])}
        check(r.get("type") == "LIST_DIR_REPLY" and "HOLA.TXT" in nombres,
              "el ARRANQUE la montó: LIST_DIR /sd ve HOLA.TXT sin un SD_MOUNT (dice %r)" % sorted(nombres))
        r, datos = w.get("/sd/HOLA.TXT")
        check(datos == b"hola desde la SD del micro simulado\n", "GET /sd/HOLA.TXT → su contenido, byte a byte")

        r = w.call("SD_MOUNT")
        check(r.get("type") == "SD_MOUNT_REPLY" and r.get("ok") is True and r.get("prefijo") == "/sd"
              and r.get("lba") == 2048,
              "SD_MOUNT → ok, en /sd, partición en el bloque 2048")
        check(r.get("etiqueta") == "BPSD" and r.get("primera") == "HOLA.TXT" and r.get("entradasRaiz") == 1,
              "SD_MOUNT → la etiqueta y la primera entrada de la raíz (dice %r, %r, %r)"
              % (r.get("etiqueta"), r.get("primera"), r.get("entradasRaiz")))
        check((r.get("kbTotal") or 0) > 6000 and 0 < (r.get("kbLibres") or 0) <= r.get("kbTotal"),
              "SD_MOUNT → el tamaño: %r KB, %r libres" % (r.get("kbTotal"), r.get("kbLibres")))

        # #473 (lote D, 248; 3-oct) — el DF y el INFO son del volumen PROPIO del micro: el total y lo usado son los de
        # littlefs, y la cuenta sumaba los ficheros de la tarjeta (y aquí, en el sim, también lo usado).
        df0, in0 = w.call("DF"), w.call("INFO")
        blob = bytes((i * 7) & 0xFF for i in range(5000))
        r = w.call_bulk("PUT", blob, path="/sd/datos.bin")
        check(r.get("type") == "PUT_REPLY", "PUT /sd/datos.bin (5000 B, 5 clusters) → OK")
        df1, in1 = w.call("DF"), w.call("INFO")
        check(df1.get("fileCount") == df0.get("fileCount") and df1.get("usedBytes") == df0.get("usedBytes"),
              "#473 (248): DF tras escribir 5000 B en /sd → la misma cuenta y lo mismo usado: es la flash "
              "(dice %r → %r ficheros, %r → %r B)" % (df0.get("fileCount"), df1.get("fileCount"),
                                                     df0.get("usedBytes"), df1.get("usedBytes")))
        check(in1.get("fsUsedBytes") == in0.get("fsUsedBytes"),
              "#473 (248): INFO → fsUsedBytes no cambia (dice %r → %r)" % (in0.get("fsUsedBytes"), in1.get("fsUsedBytes")))
        r = w.call_bulk("PUT", b"cuenta", path="/app/cuenta.bin")
        df2 = w.call("DF")
        check(r.get("type") == "PUT_REPLY" and df2.get("fileCount") == (df1.get("fileCount") or 0) + 1,
              "…y un fichero en la flash, sí: la cuenta sube uno (dice %r → %r)" % (df1.get("fileCount"), df2.get("fileCount")))
        r = w.call("SD_MOUNT")   # volver a montar: lo escrito tiene que estar EN LA TARJETA, no en una caché
        r, datos = w.get("/sd/datos.bin")
        check(datos == blob, "…y tras volver a montar, GET lo devuelve igual: está en la tarjeta")
        r = w.call("SD_MOUNT")
        check(r.get("entradasRaiz") == 2, "SD_MOUNT → la raíz tiene ya 2 entradas")

        # #628 (2-oct) — el MOTIVO de un fallo en /sd es el SUYO. FAT no lo anotaba: un PUT que fallaba en la tarjeta
        # contestaba con el motivo de la última operación de littlefs. Primero un fallo de littlefs de OTRA clase
        # (escribir encima de una carpeta: ISDIR, «argumento inválido») y después uno de FAT (una carpeta del camino
        # que es un FICHERO: NO_PATH). Antes, el segundo decía INVALID_PARAM —el del primero—. (El sim arranca como
        # una placa virgen, sin /lib: la carpeta se crea antes.)
        r = w.call("MKDIR", path="/carpeta")
        check(r.get("type") == "MKDIR_REPLY", "MKDIR /carpeta (littlefs) → OK (dice %r)" % r.get("type"))
        r = w.call_bulk("PUT", b"x", path="/carpeta")
        check(r.get("type") == "ERROR" and r.get("code") == "INVALID_PARAM",
              "PUT encima de esa carpeta → INVALID_PARAM (dice %r %r)" % (r.get("code"), r.get("message")))
        r = w.call_bulk("PUT", b"x", path="/sd/HOLA.TXT/x.txt")
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND",
              "PUT a /sd/HOLA.TXT/x.txt (HOLA.TXT es un fichero) → NOT_FOUND, el de FAT y no el de littlefs de antes "
              "(dice %r %r)" % (r.get("code"), r.get("message")))

        # #631 (2-oct) — FAT cumple el contrato de la fachada, como la flash y el PC: el mkdir es RECURSIVO (el PUT crea
        # las carpetas que falten: en la placa, con `uno` nueva, fallaba) y «ya existe» sólo vale si es una carpeta.
        r = w.call_bulk("PUT", b"hondo", path="/sd/uno/dos/tres.txt")
        check(r.get("type") == "PUT_REPLY",
              "PUT a /sd/uno/dos/tres.txt con uno/ nueva → OK: el mkdir de FAT es recursivo (dice %r %r)"
              % (r.get("type"), r.get("message")))
        r, datos = w.get("/sd/uno/dos/tres.txt")
        check(datos == b"hondo", "…y está en la tarjeta")
        r = w.call("MKDIR", path="/sd/uno")
        check(r.get("type") == "MKDIR_REPLY", "MKDIR de una carpeta que ya está → OK")
        r = w.call("MKDIR", path="/sd/HOLA.TXT")
        check(r.get("type") == "ERROR" and r.get("code") == "EXISTS",
              "MKDIR encima del FICHERO /sd/HOLA.TXT → EXISTS, no éxito (dice %r %r)" % (r.get("code"), r.get("message")))

        # #630 (2-oct) — una carpeta NO es un fichero en ninguna unidad: STAT (lo que pregunta fileExists) de una
        # carpeta dice que no, en littlefs y ahora también en FAT, que contestaba «0 bytes».
        r = w.call("STAT", path="/carpeta")
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND",
              "STAT de una carpeta de littlefs → NOT_FOUND (dice %r)" % r.get("type"))
        r = w.call("STAT", path="/sd/uno")
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND",
              "STAT de una carpeta de /sd → NOT_FOUND, como en la flash; contestaba 0 bytes (dice %r, size %r)"
              % (r.get("type"), r.get("size")))
    finally:
        para(proc)

    # ── 2. SIN tarjeta ───────────────────────────────────────────────────────────────────────────────
    proc, w = arranca(tmp, "sin", [])
    try:
        r = w.call("SD_INFO")
        check(r.get("type") == "SD_INFO_REPLY" and r.get("ok") is False
              and "no tiene SD" in (r.get("motivo") or ""),
              "SD_INFO sin tarjeta → ok=false y el porqué: %r" % r.get("motivo"))
        r = w.call("SD_MOUNT")
        check(r.get("type") == "SD_MOUNT_REPLY" and r.get("ok") is False
              and "no tiene SD" in (r.get("motivo") or ""),
              "SD_MOUNT sin tarjeta → lo mismo, no «type no implementado»")
    finally:
        para(proc)

    # ── 3. una tarjeta ROTA: sin sistema de ficheros ─────────────────────────────────────────────────
    rota = os.path.join(tmp, "rota.img")
    with open(rota, "wb") as f:
        f.write(bytes(4 * 1024 * 1024))
    proc, w = arranca(tmp, "rota", ["--sd=" + rota])
    try:
        r = w.call("SD_INFO")
        check(r.get("ok") is True and r.get("leeSector0") is True and r.get("firma") == "00 00",
              "SD_INFO con una tarjeta sin formatear → la identifica, y el sector 0 no es 55 AA")
        r = w.call("SD_MOUNT")
        check(r.get("ok") is False and r.get("motivo") and r.get("lba") == 0,
              "SD_MOUNT → falla con su motivo (%r) y el LBA" % r.get("motivo"))
        r = w.call("PING")
        check(r.get("type") == "PONG", "…y el wire sigue vivo")
    finally:
        para(proc)

    # ── 4. un PROGRAMA que copia dentro de la tarjeta (#473, lote D, 236; 3-oct) ─────────────────────────────────────
    # FAT no tenía `copy` y la fachada no lo suplía: `copyFile` en /sd lanzaba «error al copiar», y en la flash y en el PC
    # copiaba. Un fichero de varios sectores (5000 B, que no es múltiplo del trozo), uno pequeño, uno ENCIMA de otro más
    # largo (tiene que quedar sólo el nuevo) y uno que no existe (tiene que lanzar).
    if not os.path.exists(JAR) or shutil.which("java") is None or not os.path.exists(STDLIB_PACK):
        check(False, "#473 (236): sin el compilador o sin %s: no se puede probar copyFile" % STDLIB_PACK)
    else:
        img2 = os.path.join(tmp, "tarjeta2.img")
        subprocess.run([sys.executable, os.path.join(HERE, "fat_imagen.py"), img2], check=True,
                       stdout=subprocess.DEVNULL)
        src = os.path.join(tmp, "CopiaSd.bp")
        with open(src, "w", encoding="utf-8") as f:
            f.write(COPIA_SD)
        rc = subprocess.run(["java", "-jar", JAR, src, "--compile", tmp, "--backend=mivm", "--stdlibDir=" + STDLIB_DIR],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        mod = os.path.join(tmp, "CopiaSd.mod")
        check(rc.returncode == 0 and os.path.exists(mod), "#473 (236): CopiaSd compila")
        if os.path.exists(mod):
            proc, w = arranca(tmp, "copia", ["--sd=" + img2, "--pack=" + STDLIB_PACK])
            try:
                w.s.settimeout(60.0)
                with open(mod, "rb") as f:
                    check(w.call_bulk("PUT", f.read(), path="/app/CopiaSd.mod").get("type") == "PUT_REPLY",
                          "#473 (236): PUT del programa")
                grande = bytes((i * 13 + 5) & 0xFF for i in range(5000))
                check(w.call_bulk("PUT", grande, path="/sd/grande.bin").get("type") == "PUT_REPLY",
                      "#473 (236): PUT /sd/grande.bin (5000 B)")
                check(w.call_bulk("PUT", b"z" * 3000, path="/sd/encima.bin").get("type") == "PUT_REPLY",
                      "#473 (236): PUT /sd/encima.bin (3000 B, para copiar encima)")
                r = w.call("RUN", path="/app/CopiaSd.mod")
                texto, ex = w.drain_run() if r.get("type") == "RUN_REPLY" else ("", None)
                ex = ex or {}
                check(ex.get("status") == "OK",
                      "#473 (236): el programa acaba bien (dice %r %r; salida %r)"
                      % (ex.get("status"), ex.get("errorMessage"), texto[-300:]))
                check("copia=[hola desde la SD del micro simulado\n]" in texto,
                      "#473 (236): copyFile de /sd/HOLA.TXT, y readFile lee la copia (dice %r)" % texto[:200])
                check("no_esta: lanzo" in texto and "MAL" not in texto,
                      "#473 (236): copyFile de un fichero que no existe → lanza")
                r, datos = w.get("/sd/grande2.bin")
                check(datos == grande, "#473 (236): /sd/grande2.bin es la copia de los 5000 B, byte a byte (%d B)" % len(datos))
                r, datos = w.get("/sd/encima.bin")
                check(datos == b"hola desde la SD del micro simulado\n",
                      "#473 (236): copiar ENCIMA de uno más largo deja sólo el nuevo (%d B)" % len(datos))
            finally:
                para(proc)

    print("[status=OK]" if fails == 0 else f"[status=FAIL: {fails}]")
    return 0 if fails == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
