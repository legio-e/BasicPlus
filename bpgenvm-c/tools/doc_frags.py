# -*- coding: utf-8 -*-
"""doc_frags.py — compila TODOS los fragmentos de BasicPlus de la documentacion.

USO
    python bpgenvm-c/tools/doc_frags.py                    # docs/*.html, docs/en/*.html
                                                           # y los .md de usuario
    python bpgenvm-c/tools/doc_frags.py --solo docs/gui.html [--solo docs/en/gui.html]
    python bpgenvm-c/tools/doc_frags.py --work DIR         # conserva los .bp generados
    python bpgenvm-c/tools/doc_frags.py -v                 # lista tambien lo saltado
    python bpgenvm-c/tools/doc_frags.py --json informe.json

    Codigo de salida: 0 si todos los fragmentos BP compilan, 1 si alguno falla,
    2 si falta el frontend o algun fichero pedido con --solo.

POR QUE EXISTE (D1, 12-sep)
    Documentar V5 destapo ejemplos que no compilaban; documentar V6 volvio a
    destaparlos (Tabview con Gui.Component, kb.attach, el do-while del manual,
    `step -1`, el L16 de PENDIENTES...). Un fragmento que no compila es un bug
    de la documentacion, y comprobarlo a mano no se repite. Esto es la
    herramienta, no el artefacto: se corre en el checklist de PUBLICAR.md.

QUE HACE
    1. Extrae los bloques <pre>...</pre> de cada .html (quitando las etiquetas
       de resaltado y deshaciendo las entidades) y los bloques ```basic /
       ```basicplus / ```bp de los .md (un bloque ``` sin lenguaje pasa por la
       heuristica). Anota linea y encabezado (h1-h4) mas cercano.
    2. Decide si es BP por heuristica (module/function/var/print/:=/end X...
       frente a $ mvn java -jar { [ ; -> #include). Lo que no es BP (shell,
       JSON, salidas de consola, C) se salta y se cuenta.
    3. Si el fragmento es un modulo entero (empieza por `module` o `library`)
       se compila tal cual; si trae varios `module`, se parten en ficheros y
       se compilan en orden (el frontend resuelve los imports entre ellos por
       el .bp vecino). Si es un trozo suelto, se envuelve con el andamio
       MINIMO: `module Frag_<doc>_<n>` + `import Core` + un `import M` por
       cada modulo M de la stdlib (o de packs/) que el trozo nombre cualificado
       sin importarlo (`Gui.Label`, `Json.parse`) + los imports y las
       declaraciones de nivel superior (class/enum/function/const/property/
       event) al cuerpo del modulo, y las sentencias (var incluidas) dentro de
       `function main()`. Nada mas: no se inventan variables ni clases. Si el
       fragmento las necesita y el doc no las ensena, falla, y eso es lo que
       hay que ver (-v ensena que imports puso el andamio).
    4. Compila cada uno con el frontend actual en un work dir POR DOCUMENTO,
       con bpstdlib/*.mod y packs/*.pack copiados antes (la misma receta que
       compat.sh y tanda.py). Un doc = un directorio, en secuencia: asi un
       fragmento que importa el modulo de un fragmento anterior lo encuentra.
       Los documentos se compilan en paralelo (--jobs).
    5. Informe por documento (OK / FALLO / ESQUEMA / ERROR-ESPERADO) y total.

MARCAS EN EL DOCUMENTO (para lo que a proposito no compila solo)
    HTML:  <pre data-bp="skip">   no es un programa: no se compila (se cuenta)
           <pre data-bp="error">  DEBE fallar (ejemplo de error del compilador):
                                  OK si falla, FALLO si compila
           <pre data-bp="sigue">  continua el trozo suelto anterior del mismo
                                  doc (se compilan juntos): para las secciones
                                  que van construyendo un ejemplo paso a paso
    MD:    ```basic skip  /  ```basic error  /  ```basic sigue  (en el fence)
    Ademas, un fragmento con una linea que es o termina en `...` / `…` es un
    ESQUEMA: se lista, no se compila y no cuenta como fallo. Que se vea, no
    que muerda. El texto de interfaz (`bpi 7`) no es BP y se salta solo.
    Las marcas son para lo que el LECTOR ya entiende que no es un programa
    entero. Un ejemplo que deberia compilar y no compila se arregla, no se
    marca.
"""
import argparse
import html as htmlmod
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import threading
from concurrent.futures import ThreadPoolExecutor

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
JAR = os.path.join(RAIZ, "lexer-java", "target", "basicplus-frontend.jar")
STDLIB = os.path.join(RAIZ, "bpstdlib")
PACKS = os.path.join(RAIZ, "packs")

# Los .md de USUARIO. FICHAS/ESTADO/*_IDEAS son diario: sus fragmentos son
# historia y no prometen nada al lector del paquete.
MD_USUARIO = ["README.md", "README.es.md",
              "docs/QUICKSTART.md", "docs/INSTALAR_FIRMWARE.md",
              "docs/PENDIENTES.md", "docs/RELEASES.md",
              "docs/en/QUICKSTART.md", "docs/en/INSTALAR_FIRMWARE.md"]

# ── extraccion ───────────────────────────────────────────────────────────────

RE_PRE = re.compile(r"<pre([^>]*)>(.*?)</pre>", re.S)
RE_H = re.compile(r"<h([1-4])[^>]*>(.*?)</h\1>", re.S)
RE_TAG = re.compile(r"<[^>]+>")
RE_FENCE = re.compile(r"^```([^\n]*)\n(.*?)^```[ \t]*$", re.S | re.M)


def limpiar_html(txt):
    txt = RE_TAG.sub("", txt)
    txt = htmlmod.unescape(txt)
    return txt.replace("\u00a0", " ")


def encabezado(heads, pos):
    sec = ""
    for p, h in heads:
        if p < pos:
            sec = h
        else:
            break
    return sec


def extraer_html(ruta, texto):
    heads = [(m.start(), re.sub(r"\s+", " ", limpiar_html(m.group(2))).strip())
             for m in RE_H.finditer(texto)]
    frags = []
    for i, m in enumerate(RE_PRE.finditer(texto), 1):
        attrs, cuerpo = m.group(1), m.group(2)
        marca = ""
        mm = re.search(r'data-bp\s*=\s*"([^"]*)"', attrs)
        if mm:
            marca = mm.group(1).strip().lower()
        frags.append({"doc": ruta, "idx": i,
                      "linea": texto.count("\n", 0, m.start()) + 1,
                      "sec": encabezado(heads, m.start()),
                      "marca": marca, "lang": "",
                      "codigo": limpiar_html(cuerpo)})
    return frags


def extraer_md(ruta, texto):
    heads = [(m.start(), m.group(2).strip()) for m in re.finditer(r"^(#{1,4})\s+(.*)$", texto, re.M)]
    frags = []
    for i, m in enumerate(RE_FENCE.finditer(texto), 1):
        info = m.group(1).strip().lower().split()
        lang = info[0] if info else ""
        marca = info[1] if len(info) > 1 else ""
        frags.append({"doc": ruta, "idx": i,
                      "linea": texto.count("\n", 0, m.start()) + 1,
                      "sec": encabezado(heads, m.start()),
                      "marca": marca, "lang": lang,
                      "codigo": m.group(2)})
    return frags


# ── heuristica BP / no-BP ────────────────────────────────────────────────────

RE_BP = re.compile(
    r"^\s*(module\s+\w+|library\s+\"|import\s+\w+|(public\s+)?(final\s+)?(native\s+|sync\s+|intrinsic\s+)?"
    r"function\s+\w+|(public\s+)?class\s+\w+|enum\s+\w+|end\s+\w+\s*$|endif\b|endwh\b|endtry\b|endsw\b|"
    r"endpar\b|endprop\b|print\b|var\s+(owner\s+)?\w+\s*:|const\s+\w+\s*:|(public\s+)?property\s+\w+\s*:|"
    r"event\s+\w+\s*\(|raise\s+\w+|throw\s+\w+|for\s+\w+\s*(:=|\s+in\s)|next\s+\w+\s*$|while\b.*\bdo\b|"
    r"if\b.*\bthen\b|try\s*$|catch\s+\w+|parallel\s*$|return\b|break\s*$|continue\s*$|"
    r"[A-Za-z_]\w*(\.\w+)*\(.*\)\s*$)")           # una llamada sola: writeFile(..), db.exec(..), Gui.run()
RE_NOBP = re.compile(
    r"(;\s*$|;\s*//|\{\s*$|^\s*\}|->|#include|#define|\bint32_t\b|\buint8_t\b|\bvoid\b|\bstatic\b|"
    r"^\s*\$ |^\s*> |^\s*PS |^\s*C:\\|^\s*(mvn|java|bash|make|cd|python|git|idf\.py|ninja|esptool|"
    r"arm-none-eabi|gh|curl|cp|mkdir|ls|dir|put|get|run|kill|reset|autorun|log|help)\b|^\s*[\[\"]|"
    r"^\s*\{(?!.*:=)|^\s*=== |^\s*\[bpvm\]|^\s*exit \d|^bpi \d)")   # `bpi 7` = texto de interfaz
RE_ASIGNA = re.compile(r":=|\w::\w")               # `:=` y `obj::metodo` no existen fuera de BP
LANGS_BP = ("basic", "basicplus", "bp")


def sin_comentario(s):
    return re.sub(r"//.*$", "", s).strip()


def es_bp(f):
    if f["marca"] == "skip":
        return False
    if f["lang"]:
        return f["lang"] in LANGS_BP
    lineas = [sin_comentario(l) for l in f["codigo"].splitlines()]
    lineas = [l for l in lineas if l]
    if not lineas or re.match(r"^bpi \d", lineas[0]):    # texto de interfaz (referencia §17.2)
        return False
    bp = sum(1 for l in lineas if RE_BP.match(l))
    bp += sum(1 for l in lineas if RE_ASIGNA.search(l))
    no = sum(1 for l in lineas if RE_NOBP.search(l) and not RE_ASIGNA.search(l))
    if RE_NOBP.search(lineas[0]) and not RE_BP.match(lineas[0]):
        no += 2
    return bp > 0 and bp > no


# ── andamio ──────────────────────────────────────────────────────────────────

RE_MODULE = re.compile(r"^(module)\s+(\w+)")
RE_LIBRARY = re.compile(r"^library\s+\"")
RE_ABRE = re.compile(r"^(public\s+|private\s+)?(final\s+)?(native\s+|sync\s+)?(function|class|enum)\b")
RE_INTRINSIC = re.compile(r"^(public\s+|private\s+)?(final\s+)?intrinsic\s+function\b")
RE_CIERRA = re.compile(r"^end(\s+[\w.]+)?\s*$")      # end Counter.bump (estatico) tambien
RE_DECL1 = re.compile(r"^(public\s+|private\s+)?(final\s+)?(const\s+\w+\s*:|property\s+\w+\s*:|event\s+\w+\s*\(|"
                      r"intrinsic\s+function\b)")
RE_IMPORT = re.compile(r"^import\s+\w")
RE_MAIN = re.compile(r"^(public\s+)?function\s+main\s*\(", re.I)
RE_ESQUEMA = re.compile(r"(^|[\s(,])(\.\.\.|…)\s*\)?\s*$")   # `...` solo o al final de la linea


def partir_modulos(codigo):
    """Un fragmento con varios `module` a columna 0 → lista de (nombre, texto)."""
    lineas = codigo.split("\n")
    trozos, actual, nombre, pendiente = [], [], None, []
    for l in lineas:
        s = sin_comentario(l)
        m = RE_MODULE.match(s)
        if m:
            if nombre is not None:
                trozos.append((nombre, "\n".join(actual)))
            nombre, actual = m.group(2), pendiente + [l]
            pendiente = []
        elif nombre is None or (RE_LIBRARY.match(s) and not actual):
            pendiente.append(l)
        elif RE_LIBRARY.match(s):
            # `library` precede al SIGUIENTE module: cierra el actual sin ella
            trozos.append((nombre, "\n".join(actual)))
            nombre, actual, pendiente = None, [], [l]
        else:
            actual.append(l)
    if nombre is not None:
        trozos.append((nombre, "\n".join(actual + pendiente)))
    return trozos


def modulos_stdlib():
    """Nombres de modulo que el fragmento puede dar por instalados: los .mod de
    bpstdlib/ y los modulos fuente de sus subcarpetas (sqlite/: SQLite, Orm),
    que viajan en packs/*.pack."""
    nombres = set()
    for f in os.listdir(STDLIB):
        if f.endswith(".mod"):
            nombres.add(f[:-4])
    for raiz, _dirs, fichs in os.walk(STDLIB):
        if raiz == STDLIB or os.sep + "out" in raiz:
            continue
        for f in fichs:
            if f.endswith(".bp"):
                with open(os.path.join(raiz, f), encoding="utf-8", errors="replace") as fh:
                    m = re.search(r"^\s*module\s+(\w+)", fh.read(), re.M)
                if m:
                    nombres.add(m.group(1))
    return nombres


def envolver(nombre, codigo, stdlib):
    """Andamio minimo alrededor de un trozo suelto. Devuelve (texto, andamio):
    andamio es la lista de imports que se han AÑADIDO por el fragmento
    (Core siempre; y cada modulo de la stdlib que el codigo nombra
    cualificado — `Gui.Label`, `Json.parse` — sin haberlo importado)."""
    imports, decls, stmts, pend = [], [], [], []
    lineas = codigo.split("\n")
    i, n = 0, len(lineas)

    def siguiente_sig(k):
        while k < n and not sin_comentario(lineas[k]):
            k += 1
        return sin_comentario(lineas[k]) if k < n else ""

    while i < n:
        l = lineas[i]
        s = sin_comentario(l)
        i += 1
        if not s or s.startswith("@"):
            pend.append(l)
            continue
        if RE_IMPORT.match(s):
            imports += pend
            pend = []
            imports.append(l)
            continue
        if RE_ABRE.match(s) and not RE_INTRINSIC.match(s):
            # bloque class/enum/function hasta su `end` (anidamiento incluido)
            decls += pend
            pend = []
            decls.append(l)
            prof = 1
            while i < n and prof > 0:
                l2 = lineas[i]
                s2 = sin_comentario(l2)
                i += 1
                decls.append(l2)
                if RE_ABRE.match(s2) and not RE_INTRINSIC.match(s2):
                    prof += 1
                elif RE_CIERRA.match(s2):
                    prof -= 1
            continue
        if RE_DECL1.match(s):
            decls += pend
            pend = []
            decls.append(l)
            es_prop = re.match(r"^(public\s+|private\s+)?(final\s+)?property\b", s) is not None
            sig = siguiente_sig(i)
            if es_prop and (sig == "get" or sig.startswith("set(") or sig.startswith("set (")):
                # property extendida: hasta endprop
                while i < n:
                    l2 = lineas[i]
                    i += 1
                    decls.append(l2)
                    if sin_comentario(l2) == "endprop":
                        break
            continue
        stmts += pend
        pend = []
        stmts.append(l)
    stmts += pend
    hay_main = any(RE_MAIN.match(sin_comentario(l)) for l in decls)
    ya = set()
    for l in imports:
        m = re.match(r"^import\s+([\w.]+)", sin_comentario(l))
        if m:
            ya.add(m.group(1).split(".")[-1])
    cuerpo = "\n".join(sin_comentario(l) for l in decls + stmts)
    andamio = []
    if "Core" not in ya:
        andamio.append("Core")
    for mod in sorted(stdlib):
        if mod not in ya and mod != "Core" and re.search(r"(?<![\w.])" + re.escape(mod) + r"\.\w", cuerpo):
            andamio.append(mod)
    out = ["module " + nombre]
    out += ["  import " + mod for mod in andamio]
    out += ["  " + l for l in imports]
    out += ["  " + l if l.strip() else "" for l in decls]
    if not hay_main:
        out.append("  function main()")
        out += ["    " + l if l.strip() else "" for l in stmts]
        out.append("  end main")
    else:
        out += ["  " + l if l.strip() else "" for l in stmts]
    out.append("end " + nombre)
    return "\n".join(out) + "\n", andamio


def preparar(f, slug, stdlib, anterior):
    """Devuelve (piezas, andamio, codigo): piezas = lista de (nombreModulo,
    textoBP), o None si es un ESQUEMA. `anterior` es el codigo del fragmento
    BP suelto previo del mismo doc, para la marca `sigue`."""
    codigo = textwrap.dedent(f["codigo"]).strip("\n")
    if f["marca"] == "sigue" and anterior:
        codigo = anterior.rstrip("\n") + "\n\n" + codigo
    if any(RE_ESQUEMA.search(sin_comentario(l)) for l in codigo.split("\n")):
        return None, [], codigo
    sig = [sin_comentario(l) for l in codigo.split("\n") if sin_comentario(l)]
    if sig and (RE_MODULE.match(sig[0]) or RE_LIBRARY.match(sig[0])):
        trozos = partir_modulos(codigo)
        if trozos:
            return [(n, t.strip("\n") + "\n") for n, t in trozos], [], codigo
    nombre = "Frag_%s_%d" % (slug, f["idx"])
    texto, andamio = envolver(nombre, codigo, stdlib)
    return [(nombre, texto)], andamio, codigo


# ── compilacion ──────────────────────────────────────────────────────────────

RE_ERR = re.compile(r"^\[\d+:\d+\]\s+error|^-- Errores|^error:|^error de I/O|compilaci.n abortada|no se localiz.*se omitir")


def lineas_de_error(log):
    """Las lineas de error del frontend. Tras una cabecera `-- Errores ... --`
    los mensajes van indentados debajo (el emisor no lleva [linea:col]): se
    arrastran tambien, que son los que dicen QUE paso. Un import que el
    frontend OMITE («no se localizo ... se omitira») cuenta como error: en un
    ejemplo de la documentacion, un modulo que no existe es un ejemplo roto."""
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


def decodificar(b):
    """El frontend escribe en la codificacion de la consola de Windows (cp1252)
    salvo que se le diga otra cosa; se prueba utf-8 y se cae a cp1252."""
    try:
        return b.decode("utf-8")
    except UnicodeDecodeError:
        return b.decode("cp1252", "replace")


def asc(s):
    """Consola en ASCII: lo que no cabe sale como '?'."""
    return s.encode("ascii", "replace").decode("ascii")


def escribir(work, nombre, texto):
    with open(os.path.join(work, nombre + ".bp"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(texto)


def compilar(work, nombre):
    mod = os.path.join(work, nombre + ".mod")
    if os.path.exists(mod):
        os.remove(mod)
    try:
        r = subprocess.run(["java", "-jar", JAR, nombre + ".bp", "--compile", ".", "--backend=mivm"],
                           cwd=work, capture_output=True, timeout=120)
        log = decodificar(r.stdout) + decodificar(r.stderr)
        rc = r.returncode
    except subprocess.TimeoutExpired:
        log, rc = "TIMEOUT compilando", -1
    errs = lineas_de_error(log)
    # el frontend puede escribir el .mod Y devolver error (step -1, 11-sep):
    # manda el codigo de salida y que no haya lineas de error, no el fichero.
    ok = rc == 0 and os.path.exists(mod) and not errs
    if not ok and not errs:
        errs = ["rc=%d sin diagnostico; cola del log: %s" % (rc, log.strip()[-300:].replace("\n", " | "))]
    return ok, rc, errs[:8], log


def procesar_doc(ruta, base_work, slug, stdlib):
    abs_ruta = os.path.join(RAIZ, ruta)
    with open(abs_ruta, encoding="utf-8") as fh:
        texto = fh.read()
    frags = extraer_md(ruta, texto) if ruta.lower().endswith(".md") else extraer_html(ruta, texto)
    work = os.path.join(base_work, slug)
    os.makedirs(work, exist_ok=True)
    for f in os.listdir(STDLIB):
        if f.endswith(".mod"):
            shutil.copy2(os.path.join(STDLIB, f), work)
    if os.path.isdir(PACKS):
        for f in os.listdir(PACKS):
            if f.endswith(".pack"):
                shutil.copy2(os.path.join(PACKS, f), work)
    res = []
    anterior = ""
    for f in frags:
        r = {"doc": ruta, "idx": f["idx"], "linea": f["linea"], "sec": f["sec"], "marca": f["marca"],
             "primera": next((l.strip() for l in f["codigo"].splitlines() if l.strip()), "")[:70]}
        if not es_bp(f):
            r["estado"] = "SKIP-MARCA" if f["marca"] == "skip" else "NO-BP"
            res.append(r)
            continue
        piezas, andamio, codigo = preparar(f, slug, stdlib, anterior)
        if piezas is None:
            r["estado"] = "ESQUEMA"
            res.append(r)
            continue
        if andamio:
            anterior = codigo          # solo los sueltos encadenan con `sigue`
            r["andamio"] = andamio
        r["modulos"] = [n for n, _ in piezas]
        ok_todos, errs_todos, rc_ult = True, [], 0
        for n, t in piezas:          # todos los .bp antes de compilar el primero:
            escribir(work, n, t)     # un modulo puede importar al que viene detras
        for n, _t in piezas:
            ok, rc, errs, _log = compilar(work, n)
            rc_ult = rc
            if not ok:
                ok_todos = False
                errs_todos += [n + ": " + e for e in errs] or [n + ": rc=%d sin diagnostico" % rc]
        if f["marca"] == "error":
            r["estado"] = "ERROR-ESPERADO" if not ok_todos else "FALLO"
            if ok_todos:
                r["errores"] = ["compila, y el doc dice que debe fallar (data-bp=error)"]
            else:
                r["errores"] = errs_todos[:3]
        else:
            r["estado"] = "OK" if ok_todos else "FALLO"
            r["errores"] = errs_todos
            r["rc"] = rc_ult
        res.append(r)
    return res


# ── informe ──────────────────────────────────────────────────────────────────

LOCK = threading.Lock()


def imprimir_doc(ruta, res, verbose):
    lineas = [ruta]
    cnt = {}
    for r in res:
        cnt[r["estado"]] = cnt.get(r["estado"], 0) + 1
        mostrar = r["estado"] in ("FALLO", "ESQUEMA", "ERROR-ESPERADO") or verbose
        if not mostrar:
            continue
        sec = (r["sec"][:48] + "..") if len(r["sec"]) > 50 else r["sec"]
        lineas.append("  %-14s L%-5d [%s] %s" % (r["estado"], r["linea"], sec, r["primera"]))
        if verbose and r.get("andamio"):
            lineas.append("      + andamio: import " + ", ".join(r["andamio"]))
        for e in r.get("errores", [])[:4]:
            lineas.append("      ! " + e[:200])
    partes = ["%d %s" % (cnt[k], k) for k in ("OK", "FALLO", "ERROR-ESPERADO", "ESQUEMA", "NO-BP", "SKIP-MARCA")
              if k in cnt]
    lineas.append("  => " + ", ".join(partes) if partes else "  => sin bloques de codigo")
    with LOCK:
        print(asc("\n".join(lineas)))
        sys.stdout.flush()


def main():
    try:
        sys.stdout.reconfigure(errors="replace")
    except Exception:
        pass
    ap = argparse.ArgumentParser(description="compila los fragmentos BP de la documentacion")
    ap.add_argument("--solo", action="append", default=[], help="solo este documento (repetible)")
    ap.add_argument("--work", help="directorio de trabajo a conservar (por defecto, temporal)")
    ap.add_argument("--jobs", type=int, default=4, help="documentos en paralelo (defecto 4)")
    ap.add_argument("--json", help="volcar el informe a este fichero")
    ap.add_argument("-v", "--verbose", action="store_true", help="listar tambien los OK y lo saltado")
    a = ap.parse_args()

    if not os.path.exists(JAR):
        print("FALTA el frontend: %s (mvn -f lexer-java/pom.xml install)" % JAR)
        return 2
    if a.solo:
        docs = [d.replace("\\", "/") for d in a.solo]
        for d in docs:
            if not os.path.exists(os.path.join(RAIZ, d)):
                print("no existe: " + d)
                return 2
    else:
        docs = sorted("docs/" + f for f in os.listdir(os.path.join(RAIZ, "docs")) if f.endswith(".html"))
        docs += sorted("docs/en/" + f for f in os.listdir(os.path.join(RAIZ, "docs", "en")) if f.endswith(".html"))
        docs += [m for m in MD_USUARIO if os.path.exists(os.path.join(RAIZ, m))]

    tmp = None
    if a.work:
        base = os.path.abspath(a.work)
        os.makedirs(base, exist_ok=True)
    else:
        tmp = tempfile.mkdtemp(prefix="doc_frags_")
        base = tmp
    print("frontend : %s" % os.path.relpath(JAR, RAIZ))
    print("stdlib   : %s (%d .mod)  packs: %s" % (
        os.path.relpath(STDLIB, RAIZ), len([f for f in os.listdir(STDLIB) if f.endswith(".mod")]),
        ", ".join(sorted(f for f in os.listdir(PACKS) if f.endswith(".pack"))) if os.path.isdir(PACKS) else "-"))
    print("work     : %s" % base)
    print("docs     : %d" % len(docs))
    print()

    def slug_de(ruta):
        r = ruta if not os.path.isabs(ruta) else os.path.basename(ruta)
        r = r.replace("docs/", "").replace(".html", "").replace(".md", "")
        return re.sub(r"[^A-Za-z0-9]+", "_", r).strip("_")

    todos = {}
    stdlib = modulos_stdlib()

    def tarea(ruta):
        res = procesar_doc(ruta, base, slug_de(ruta), stdlib)
        imprimir_doc(ruta, res, a.verbose)
        return ruta, res

    with ThreadPoolExecutor(max_workers=max(1, a.jobs)) as ex:
        for ruta, res in ex.map(tarea, docs):
            todos[ruta] = res

    plano = [r for ruta in docs for r in todos[ruta]]
    n = {}
    for r in plano:
        n[r["estado"]] = n.get(r["estado"], 0) + 1
    fallos = [r for r in plano if r["estado"] == "FALLO"]
    print()
    print("=" * 72)
    print("TOTAL: %d bloques en %d docs - BP compilados %d: %d OK, %d FALLO, %d error-esperado; "
          "%d esquema (con ...), %d no-BP saltados, %d marcados skip" % (
              len(plano), len(docs), n.get("OK", 0) + n.get("FALLO", 0) + n.get("ERROR-ESPERADO", 0),
              n.get("OK", 0), n.get("FALLO", 0), n.get("ERROR-ESPERADO", 0),
              n.get("ESQUEMA", 0), n.get("NO-BP", 0), n.get("SKIP-MARCA", 0)))
    if fallos:
        por_doc = {}
        for r in fallos:
            por_doc[r["doc"]] = por_doc.get(r["doc"], 0) + 1
        print("FALLAN: " + ", ".join("%s (%d)" % kv for kv in sorted(por_doc.items())))
    if a.json:
        with open(a.json, "w", encoding="utf-8") as fh:
            json.dump(plano, fh, ensure_ascii=False, indent=1)
        print("informe: " + a.json)
    if tmp and not fallos:
        shutil.rmtree(tmp, ignore_errors=True)
    elif tmp:
        print("los .bp generados quedan en " + tmp + " (se conservan porque hubo fallos)")
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
