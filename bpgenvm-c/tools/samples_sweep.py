# -*- coding: utf-8 -*-
"""samples_sweep.py — compila TODOS los samples/**/*.bp con el frontend actual.

USO
    python bpgenvm-c/tools/samples_sweep.py               # el corpus entero
    python bpgenvm-c/tools/samples_sweep.py --solo samples/Blink.bp [--solo ...]
    python bpgenvm-c/tools/samples_sweep.py --work DIR    # conserva los .mod
    python bpgenvm-c/tools/samples_sweep.py -v            # lista tambien los OK
    python bpgenvm-c/tools/samples_sweep.py --json informe.json

    Codigo de salida: 0 si todo compila, 1 si algo falla, 2 si falta el frontend.

POR QUE EXISTE (D1, 12-sep)
    PUBLICAR.md decia «baterIa de samples: 0 no compilan», medido en V5 sobre
    la LISTA de H13 (scripts/h13-lista.sh compila una lista, no el corpus).
    El 12-sep, con el compilador congelado, 6 de 326 samples publicados no
    compilaban (#498: `import Core` obligatorio desde #458) y nadie lo habria
    visto hasta publicar. Esto compila el CORPUS: lo que hay en el disco bajo
    samples/, no una lista que alguien mantiene a mano.

QUE HACE
    1. Recorre samples/ (recursivo) y toma todos los .bp, menos:
         - out/ y target/ (salidas de build);
         - samples/errores/   -> fallan A PROPOSITO; los vigila scripts/h13-errores.sh
                                 (y en los dos sentidos: que fallen y con que mensaje);
         - samples/pendientes/ -> correctos que hoy no compilan por un bug nuestro;
                                 se COMPILAN e informan aparte (PENDIENTE), sin
                                 contar como fallo: el dia que compilen, el bug
                                 esta arreglado y vuelven a samples/;
         - samples/holes/     -> repros internos de agujeros del compilador
                                 (cascadas de errores): no son ejemplos.
       Las carpetas se explican en el LEEME de cada una; aqui solo se leen.
    2. Un work dir con bpstdlib/*.mod y packs/*.pack copiados antes: la misma
       receta que compat.sh, tanda.py y doc_frags.py. Cada .bp se compila EN SU
       SITIO (`--compile <work>`): asi un sample que importa a otro del lado
       (frompathtest -> external/helper.bp, GuiForm -> su .win) lo encuentra
       como lo encontraria el IDE, y SqlDemo/DaoDemo resuelven SQLite/Orm
       desde el pack, sin construir bpstdlib/sqlite antes.
    3. Un sample compila si el frontend devuelve 0, deja el .mod y no hay
       lineas de error: el frontend puede escribir el .mod Y dar error a la vez
       (`step -1`, 11-sep).
    4. Informe: FALLO con las primeras lineas de error, PENDIENTE, y el total.
       En paralelo (--jobs) con un work dir por trabajador, para que dos
       samples con el mismo nombre de modulo no se pisen.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
from concurrent.futures import ThreadPoolExecutor

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
JAR = os.path.join(RAIZ, "lexer-java", "target", "basicplus-frontend.jar")
STDLIB = os.path.join(RAIZ, "bpstdlib")
PACKS = os.path.join(RAIZ, "packs")
SAMPLES = os.path.join(RAIZ, "samples")

EXCLUIDAS = {          # carpeta de samples/ -> por que no se compila aqui
    "errores": "fallan a proposito (scripts/h13-errores.sh los vigila)",
    "holes": "repros internos del compilador, no ejemplos",
}
PENDIENTES = "pendientes"      # se compilan, se informan aparte, no cuentan como fallo
DIRS_BUILD = {"out", "target"}

RE_ERR = re.compile(r"^\[\d+:\d+\]\s+error|^-- Errores|^error:|^error de I/O|compilaci.n abortada")
RE_AVISO = re.compile(r"no se localiz.*se omitir")      # import que el frontend omite en silencio


def decodificar(b):
    try:
        return b.decode("utf-8")
    except UnicodeDecodeError:
        return b.decode("cp1252", "replace")


def lineas_de_error(log):
    """Las lineas de error del frontend. Tras una cabecera `-- Errores ... --`
    los mensajes van indentados debajo (el emisor no lleva [linea:col]): se
    arrastran tambien, que son los que dicen QUE paso."""
    errs, arrastrar = [], 0
    for l in log.splitlines():
        if RE_ERR.search(l):
            errs.append(l.strip())
            arrastrar = 3 if l.startswith("-- Errores") else 0
        elif arrastrar and l.startswith("  ") and l.strip():
            errs.append(l.strip())
            arrastrar -= 1
        else:
            arrastrar = 0
    return errs


def asc(s):
    return s.encode("ascii", "replace").decode("ascii")


def corpus():
    """(ruta relativa, categoria) para cada .bp bajo samples/."""
    lista = []
    for raiz, dirs, fichs in os.walk(SAMPLES):
        dirs[:] = sorted(d for d in dirs if d not in DIRS_BUILD)
        rel = os.path.relpath(raiz, SAMPLES).replace("\\", "/")
        top = rel.split("/")[0] if rel != "." else ""
        for f in sorted(fichs):
            if not f.endswith(".bp"):
                continue
            ruta = ("samples/" + (rel + "/" if rel != "." else "") + f)
            if top in EXCLUIDAS:
                cat = "EXCLUIDO"
            elif top == PENDIENTES:
                cat = "PENDIENTE"
            else:
                cat = "SAMPLE"
            lista.append((ruta, cat))
    return lista


def nombre_modulo(ruta):
    """Nombre del .mod que el frontend escribira: `library "a.b"` + `module M`
    -> a.b.M ; sin library -> M."""
    with open(os.path.join(RAIZ, ruta), encoding="utf-8", errors="replace") as fh:
        txt = fh.read()
    m = re.search(r"^\s*module\s+(\w+)", txt, re.M)
    if not m:
        return None
    lib = re.search(r"^\s*library\s+\"([^\"]+)\"", txt, re.M)
    return (lib.group(1) + "." if lib else "") + m.group(1)


def preparar_work(work):
    os.makedirs(work, exist_ok=True)
    for f in os.listdir(STDLIB):
        if f.endswith(".mod"):
            shutil.copy2(os.path.join(STDLIB, f), work)
    if os.path.isdir(PACKS):
        for f in os.listdir(PACKS):
            if f.endswith(".pack"):
                shutil.copy2(os.path.join(PACKS, f), work)


def compilar(ruta, work):
    """Devuelve (ok, rc, errores, avisos, log)."""
    modulo = nombre_modulo(ruta)
    mod = os.path.join(work, (modulo or "?") + ".mod")
    if modulo and os.path.exists(mod):
        os.remove(mod)
    try:
        r = subprocess.run(["java", "-jar", JAR, os.path.join(RAIZ, ruta), "--compile", work, "--backend=mivm"],
                           cwd=work, capture_output=True, timeout=180)
        log = decodificar(r.stdout) + decodificar(r.stderr)
        rc = r.returncode
    except subprocess.TimeoutExpired:
        log, rc = "TIMEOUT compilando", -1
    errs = lineas_de_error(log)
    avisos = [l.strip() for l in log.splitlines() if RE_AVISO.search(l)]
    if modulo is None:
        errs.insert(0, "no se encuentra `module <Nombre>` en el fichero")
    ok = rc == 0 and modulo is not None and os.path.exists(mod) and not errs
    if not ok and not errs:
        errs = ["rc=%d sin diagnostico; cola del log: %s" % (rc, log.strip()[-300:].replace("\n", " | "))]
    return ok, rc, errs[:6], avisos, log


def main():
    try:
        sys.stdout.reconfigure(errors="replace")
    except Exception:
        pass
    ap = argparse.ArgumentParser(description="compila todos los samples/**/*.bp")
    ap.add_argument("--solo", action="append", default=[], help="solo este .bp (repetible)")
    ap.add_argument("--work", help="directorio de trabajo a conservar (por defecto, temporal)")
    ap.add_argument("--jobs", type=int, default=4, help="compilaciones en paralelo (defecto 4)")
    ap.add_argument("--json", help="volcar el informe a este fichero")
    ap.add_argument("-v", "--verbose", action="store_true", help="listar tambien los OK")
    a = ap.parse_args()

    if not os.path.exists(JAR):
        print("FALTA el frontend: %s (mvn -f lexer-java/pom.xml install)" % JAR)
        return 2

    if a.solo:
        lista = [(s.replace("\\", "/"), "SAMPLE") for s in a.solo]
        for ruta, _c in lista:
            if not os.path.exists(os.path.join(RAIZ, ruta)):
                print("no existe: " + ruta)
                return 2
    else:
        lista = corpus()

    excluidos = [(r, c) for r, c in lista if c == "EXCLUIDO"]
    a_compilar = [(r, c) for r, c in lista if c != "EXCLUIDO"]

    tmp = None
    if a.work:
        base = os.path.abspath(a.work)
        os.makedirs(base, exist_ok=True)
    else:
        tmp = tempfile.mkdtemp(prefix="samples_sweep_")
        base = tmp
    jobs = max(1, a.jobs)
    works = [os.path.join(base, "w%d" % i) for i in range(jobs)]
    for w in works:
        preparar_work(w)

    print("frontend : %s" % os.path.relpath(JAR, RAIZ))
    print("stdlib   : %s (%d .mod)  packs: %s" % (
        os.path.relpath(STDLIB, RAIZ), len([f for f in os.listdir(STDLIB) if f.endswith(".mod")]),
        ", ".join(sorted(f for f in os.listdir(PACKS) if f.endswith(".pack"))) if os.path.isdir(PACKS) else "-"))
    print("work     : %s (%d en paralelo)" % (base, jobs))
    print("corpus   : %d .bp bajo samples/ - %d a compilar, %d excluidos" % (
        len(lista), len(a_compilar), len(excluidos)))
    for d, motivo in sorted(EXCLUIDAS.items()):
        n = sum(1 for r, _c in excluidos if r.startswith("samples/" + d + "/"))
        if n:
            print("           samples/%s/ (%d): %s" % (d, n, motivo))
    print()

    lock = threading.Lock()
    libres = list(works)
    res = []

    def tarea(item):
        ruta, cat = item
        with lock:
            w = libres.pop()
        try:
            ok, rc, errs, avisos, _log = compilar(ruta, w)
        finally:
            with lock:
                libres.append(w)
        r = {"ruta": ruta, "cat": cat, "ok": ok, "rc": rc, "errores": errs, "avisos": avisos}
        if cat == "PENDIENTE":
            r["estado"] = "PENDIENTE-OK" if ok else "PENDIENTE"
        else:
            r["estado"] = "OK" if ok else "FALLO"
        with lock:
            if r["estado"] != "OK" or a.verbose:
                print(asc("  %-12s %s" % (r["estado"], ruta)))
                for e in errs[:4]:
                    print(asc("      ! " + e[:200]))
                if a.verbose:
                    for e in avisos[:3]:
                        print(asc("      ? " + e[:200]))
            sys.stdout.flush()
        return r

    with ThreadPoolExecutor(max_workers=jobs) as ex:
        res = list(ex.map(tarea, a_compilar))

    fallos = [r for r in res if r["estado"] == "FALLO"]
    pend = [r for r in res if r["cat"] == "PENDIENTE"]
    okn = sum(1 for r in res if r["estado"] == "OK")
    print()
    print("=" * 72)
    print("TOTAL: %d samples compilados: %d OK, %d FALLO; pendientes/: %d (%d ya compilan); excluidos: %d" % (
        len(res) - len(pend), okn, len(fallos), len(pend),
        sum(1 for r in pend if r["ok"]), len(excluidos)))
    if fallos:
        print("FALLAN: " + ", ".join(r["ruta"] for r in fallos))
    con_aviso = [r for r in res if r["avisos"]]
    if con_aviso:
        print("AVISO: %d compilan con un import que el frontend OMITE (no existe el .bp/.mod): %s  (-v para ver cual)" % (
            len(con_aviso), ", ".join(r["ruta"] for r in con_aviso)))
    if a.json:
        with open(a.json, "w", encoding="utf-8") as fh:
            json.dump(res, fh, ensure_ascii=False, indent=1)
        print("informe: " + a.json)
    if tmp and not fallos:
        shutil.rmtree(tmp, ignore_errors=True)
    elif tmp:
        print("los .mod quedan en " + tmp + " (se conservan porque hubo fallos)")
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
