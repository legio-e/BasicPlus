#!/usr/bin/env python3
# rendimiento_versiones.py — V8 (31-oct): el rendimiento de hoy frente al de una versión publicada, EN PLACA (Eduardo:
# «Me gustaría poder comparar el rendimiento actual con lo que teníamos, ya sea de la versión 7 o incluso anterior.
# Por lo menos poder comparar en la Metro y en la P4»).
#
# Cada versión se mide ENTERA, como se publicó: su compilador, su stdlib y su firmware salen de su ZIP de dist/
# (dist/BasicPlus-N.0-win); «hoy» es el árbol (el compilador de lexer-java, la imagen de bpgenvm-c/pico/build o
# bpgenvm-c/esp32p4/build). Los programas son los de samples/benchmarks que no han cambiado de texto (comprobado el
# 31-oct): los siete clásicos desde V4; Bench, AllocBench, PrintBench y VivoLargo desde V6; BenchMem desde V7.
#
#   python -X utf8 tools/rendimiento_versiones.py mide PLACA PUERTO VERSION [--sin-grabar]
#       PLACA: rp2350 | p4.  VERSION: hoy | v7 | v6 | v5 | v4.
#       Compila los programas con el compilador de VERSION, graba su firmware (salvo --sin-grabar: mide lo que lleve
#       la placa), espera a que conteste, sube los .mod a /app, pasa cada uno DOS veces y guarda las salidas en
#       compat/informes/rendimiento/<placa>_<version>.json.
#   python -X utf8 tools/rendimiento_versiones.py compara PLACA VERSION VERSION
#       Las dos medidas, programa a programa y línea a línea (los tiempos que imprime cada programa, y el del RUN
#       entero), a compat/informes/rendimiento_<placa>_<v1>_<v2>.md.
#
# LA PLACA, antes y después (lo hace quien mide, a mano; ver FICHAS #743):
#   - antes de grabar una versión vieja: guardar el ENV, la lista de ficheros y la de packs (wire_serie.py env, list,
#     packs), y quitar el Stdlib.pack de hoy (wire_serie.py packdel Stdlib): si no, la versión vieja podría tomar la
#     stdlib de hoy; sin él, repone /lib desde su imagen;
#   - al acabar: grabar la imagen de hoy y su Stdlib.pack (wire_serie.py pack packs/Stdlib.pack), que vacía /lib.
import json, os, re, subprocess, sys, tempfile, time

AQUI = os.path.dirname(os.path.abspath(__file__))
BP = os.path.dirname(AQUI)
REPO = os.path.dirname(BP)
INF = os.path.join(REPO, "compat", "informes", "rendimiento")
ENV = dict(os.environ, MSYS_NO_PATHCONV="1")
DESDE = {"arraybench": 4, "fibobench": 4, "floatbench": 4, "intbench": 4, "sortbench": 4, "stringbench": 4,
         "sumbench": 4, "Bench": 6, "AllocBench": 6, "PrintBench": 6, "VivoLargo": 6, "BenchMem": 7}
VECES = 2
TIEMPO_RUN = 600   # s, el tope de cada RUN

def num_version(v):
    return 99 if v == "hoy" else int(v.lstrip("v"))

def dist(v):
    return os.path.join(REPO, "dist", "BasicPlus-%d.0-win" % num_version(v))

def compila(v, trabajo):
    """Los .mod de la versión v, compilados con SU compilador, a trabajo/; devuelve {programa: ruta del .mod}."""
    os.makedirs(trabajo, exist_ok=True)
    hechos = {}
    for prog, desde in DESDE.items():
        if num_version(v) < desde: continue
        if v == "hoy":
            cwd = REPO
            cmd = ["java", "-jar", os.path.join(REPO, "lexer-java", "target", "basicplus-frontend.jar")]
        else:
            cwd = dist(v)
            jar = [f for f in os.listdir(cwd) if f.startswith("BpIde-") and f.endswith(".jar")][0]
            cmd = ["java", "-cp", jar, "basicplus.frontend.Main"]
        bp = os.path.join(cwd, "samples", "benchmarks", prog + ".bp")
        r = subprocess.run(cmd + [bp, "--compile", trabajo, "--backend=mivm"], cwd=cwd, capture_output=True,
                           text=True, encoding="utf-8", errors="replace")
        m = re.search(r"^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)", open(bp, encoding="utf-8", errors="replace").read(), re.M)
        mod = os.path.join(trabajo, (m.group(1) if m else prog) + ".mod")
        if r.returncode != 0 or not os.path.exists(mod):
            sys.exit("%s no compila con %s:\n%s" % (prog, v, (r.stdout + r.stderr)[-800:]))
        hechos[prog] = mod
    return hechos

def wire(puerto, *args, timeout=120):
    r = subprocess.run([sys.executable, "-X", "utf8", os.path.join(AQUI, "wire_serie.py"), puerto] + list(args),
                       capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout, env=ENV)
    return r.stdout + r.stderr

def powershell_idf(orden):
    """Una orden con el entorno del IDF (sólo en PowerShell: desde bash se pierde). Sin las variables de MSYS: si se
    lanza desde Git Bash, idf.py las ve, dice «MSys/Mingw is no longer supported» y no graba nada (31-oct)."""
    entorno = os.path.join(AQUI, "entorno_idf.ps1")
    limpio = {k: v for k, v in os.environ.items() if not k.upper().startswith(("MSYSTEM", "MSYS", "MINGW"))}
    return subprocess.run(["powershell", "-NoProfile", "-Command", ". '%s' *>$null; %s" % (entorno, orden)],
                          capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=900, env=limpio)

def hola(puerto):
    """La línea HELLO (dice qué imagen corre), o None si la placa no contesta."""
    try:
        h = [l.strip() for l in wire(puerto, "info", timeout=30).splitlines() if l.startswith("HELLO")]
    except Exception:
        return None
    return h[0] if h else None

def graba(placa, puerto, v):
    antes = hola(puerto)
    graba_imagen(placa, puerto, v)
    despues = hola(puerto)
    if antes is not None and despues == antes:   # 31-oct: un grabado que no grabó medía la imagen de antes
        sys.exit("la placa sigue con la misma imagen tras grabar %s (%s): no se mide" % (v, despues))

def graba_imagen(placa, puerto, v):
    if placa == "rp2350":
        uf2 = (os.path.join(BP, "pico", "build", "bpvm_pico.uf2") if v == "hoy"
               else os.path.join(dist(v), "firmware", "bpvm_pico.uf2"))
        r = subprocess.run([sys.executable, os.path.join(AQUI, "flash_pico.py"), puerto, uf2], capture_output=True,
                           text=True, encoding="utf-8", errors="replace", timeout=180)
        salida = r.stdout + r.stderr
    elif v == "hoy":   # la P4: bootloader, particiones y aplicación del build de hoy
        r = powershell_idf("idf -C '%s' -p %s flash" % (os.path.join(BP, "esp32p4"), puerto))
        salida = r.stdout + r.stderr
    else:              # la P4: el _merged.bin publicado, al offset 0
        binario = os.path.join(dist(v), "firmware", "bpvm_esp32p4_merged.bin")
        r = powershell_idf("& \"$env:IDF_PYTHON_ENV_PATH\\Scripts\\python.exe\" -m esptool --chip esp32p4 -p %s "
                           "-b 460800 write_flash 0x0 '%s'" % (puerto, binario))
        salida = r.stdout + r.stderr
    if r.returncode != 0: sys.exit("no se pudo grabar %s en %s:\n%s" % (v, puerto, salida[-1500:]))
    print("grabada:", (salida.strip().splitlines() or ["?"])[-1])
    time.sleep(8)
    for _ in range(40):
        try:
            if '"boardName"' in wire(puerto, "info", timeout=20): return
        except Exception:
            pass
        time.sleep(3)
    sys.exit("la placa no contesta tras grabar " + v)

def mide(placa, puerto, v, sin_grabar):
    # los .mod, a un directorio temporal (uno por medida: dos placas pueden medir a la vez); el informe sólo guarda salidas
    trabajo = tempfile.mkdtemp(prefix="rendimiento_%s_%s_" % (placa, v))
    mods = compila(v, trabajo)
    print("compilados con %s: %d programas" % (v, len(mods)))
    if not sin_grabar: graba(placa, puerto, v)
    log = wire(puerto, "log", timeout=60)
    arranque = [l.strip() for l in log.splitlines() if "=== boot" in l or "boot: imagen" in l][-2:]
    saludo = hola(puerto)
    print("arranque:", " | ".join(arranque), "|", saludo or "")
    res = {"placa": placa, "puerto": puerto, "version": v, "fecha": time.strftime("%Y-%m-%d %H:%M"),
           "arranque": arranque, "hello": saludo, "programas": {}}
    for prog, mod in mods.items():
        out = wire(puerto, "put", mod, "/app/%s" % os.path.basename(mod), timeout=120)
        if "ok" not in out.lower(): print("  put %s: %s" % (prog, out.strip()[-200:]))
    for prog, mod in mods.items():
        pasadas = []
        for _ in range(VECES):
            out = wire(puerto, "run", "/app/%s" % os.path.basename(mod), str(TIEMPO_RUN), timeout=TIEMPO_RUN + 60)
            m = re.search(r"\[EXITED (\w+) exit=(-?\d+) (\d+) ms\]", out)
            pasadas.append({"salida": out, "estado": m.group(1) if m else None, "ms": int(m.group(3)) if m else None})
        res["programas"][prog] = pasadas
        print("  %-12s %s" % (prog, "  ".join("%s %s ms" % (p["estado"], p["ms"]) for p in pasadas)))
    os.makedirs(INF, exist_ok=True)
    json.dump(res, open(os.path.join(INF, "%s_%s.json" % (placa, v)), "w", encoding="utf-8"), indent=1,
              ensure_ascii=False)

TIEMPO = re.compile(r"(-?\d+(?:\.\d+)?)\s*(ms|s)\b")
NUM = re.compile(r"-?\d+(?:\.\d+)?")

def lineas_con_tiempo(salida):
    """[(clave, ms)]: cada línea del programa que dice un tiempo; la clave, la línea sin números (para emparejar)."""
    r = []
    for l in salida.splitlines():
        if "EXITED" in l or l.startswith("==="): continue
        t = TIEMPO.search(l)
        if t:
            ms = float(t.group(1)) * (1000.0 if t.group(2) == "s" else 1.0)
            r.append((NUM.sub("#", l.strip()), ms))
    return r

def media(xs):
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else None

def compara(placa, v1, v2):
    a = json.load(open(os.path.join(INF, "%s_%s.json" % (placa, v1)), encoding="utf-8"))
    b = json.load(open(os.path.join(INF, "%s_%s.json" % (placa, v2)), encoding="utf-8"))
    md = ["# Rendimiento en placa: %s, %s frente a %s" % (placa, v1, v2), "",
          "- %s: %s, %s" % (v1, a["fecha"], " | ".join(a["arranque"])),
          "- %s: %s, %s" % (v2, b["fecha"], " | ".join(b["arranque"])),
          "- cada programa, %d veces; los tiempos son la media; «%s → %s» y el cambio (negativo = %s más rápido)" %
          (VECES, v1, v2, v2), "",
          "| programa | lo que mide | %s | %s | cambio |" % (v1, v2), "|---|---|---|---|---|"]
    for prog in DESDE:
        if prog not in a["programas"] or prog not in b["programas"]: continue
        pa, pb = a["programas"][prog], b["programas"][prog]
        la = [lineas_con_tiempo(p["salida"]) for p in pa]
        lb = [lineas_con_tiempo(p["salida"]) for p in pb]
        claves = [c for c, _ in (la[0] if la else [])]
        for i, c in enumerate(claves):
            xa = media([l[i][1] for l in la if i < len(l)])
            xb = media([l[i][1] for l in lb if i < len(l) and l[i][0] == c])
            cambio = ("%+.0f %%" % (100.0 * (xb - xa) / xa)) if xa and xb is not None else "—"
            md.append("| %s | %s | %s | %s | %s |" % (prog, c.replace("|", "/")[:70],
                      "%.1f ms" % xa if xa is not None else "—", "%.1f ms" % xb if xb is not None else "—", cambio))
        ma, mb = media([p["ms"] for p in pa]), media([p["ms"] for p in pb])
        estados = "" if all(p["estado"] == "OK" for p in pa + pb) else " (¡%s / %s!)" % (
            [p["estado"] for p in pa], [p["estado"] for p in pb])
        cambio = ("%+.0f %%" % (100.0 * (mb - ma) / ma)) if ma and mb is not None else "—"
        md.append("| %s | **el RUN entero**%s | %s | %s | %s |" % (prog, estados,
                  "%.0f ms" % ma if ma is not None else "—", "%.0f ms" % mb if mb is not None else "—", cambio))
    salida = os.path.join(os.path.dirname(INF), "rendimiento_%s_%s_%s.md" % (placa, v1, v2))
    open(salida, "w", encoding="utf-8").write("\n".join(md) + "\n")
    print("\n".join(md))
    print("\n->", salida)

if __name__ == "__main__":
    if len(sys.argv) >= 5 and sys.argv[1] == "mide":
        mide(sys.argv[2], sys.argv[3], sys.argv[4], "--sin-grabar" in sys.argv)
    elif len(sys.argv) == 5 and sys.argv[1] == "compara":
        compara(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        sys.exit(__doc__ or "uso: mide PLACA PUERTO VERSION [--sin-grabar] | compara PLACA V1 V2")
