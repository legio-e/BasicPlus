# -*- coding: utf-8 -*-
"""migrar_intrinsic_a_inline.py — H1 (20-sep): de `public intrinsic function f(...)` a la forma
de Turbo Pascal, una función normal cuyo cuerpo es el bytecode:

    public function f(x: float): float
      inline CALL_BUILTIN 23
    end f

    python scripts/migrar_intrinsic_a_inline.py bpstdlib/Math.bp [más .bp]   (desde la raíz)

De dónde sale el número: de lo que hay hoy. `Intrinsics.java` ata "Math.f" → Builtin.CONST y
`Builtin.java` da el id de CONST (su ordinal). Es un guion de UNA VEZ: cuando la stdlib esté
migrada, los ids viven en los .bp y son Builtin.java y el enum de C los que se GENERAN de ahí
(scripts/gen_builtins.py). No toca nada que no sea una línea `intrinsic`; si una no tiene
registro en Intrinsics.java, lo dice y no la migra (errores sí, silenciosos no).
"""
import io, os, re, sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JAVA = os.path.join(RAIZ, "miVM", "src", "main", "java", "edu", "bpgenvm", "bytecode", "Builtin.java")
INTR = os.path.join(RAIZ, "lexer-java", "src", "main", "java", "basicplus", "frontend", "Intrinsics.java")

def ids_de_builtin_java():
    s = io.open(JAVA, encoding="utf-8").read()
    cuerpo = s[s.index("public enum Builtin {"):s.index("public final String bpName")]
    cuerpo = re.sub(r"/\*.*?\*/", "", cuerpo, flags=re.S)
    cuerpo = re.sub(r"//[^\n]*", "", cuerpo)
    consts = re.findall(r"\b([A-Z][A-Z0-9_]*)\s*\(\s*\"", cuerpo)
    return {c: i for i, c in enumerate(consts)}

def enlaces_de_intrinsics_java():
    """"Mod.f" → CONST. Los del bucle `for (String mod : new String[]{"Machine"})` se expanden."""
    s = io.open(INTR, encoding="utf-8").read()
    out = {}
    for m in re.finditer(r'register\("([A-Za-z_][A-Za-z0-9_.]*)",\s*w\s*->\s*emitBuiltin\(w,\s*Builtin\.([A-Z0-9_]+)\)\)', s):
        out[m.group(1)] = m.group(2)
    for m in re.finditer(r'register\(mod\s*\+\s*"\.([A-Za-z_][A-Za-z0-9_]*)",\s*w\s*->\s*emitBuiltin\(w,\s*Builtin\.([A-Z0-9_]+)\)\)', s):
        out["Machine." + m.group(1)] = m.group(2)
    return out

RE_INTR = re.compile(r"^(\s*)(public\s+)?intrinsic\s+function\s+([A-Za-z_][A-Za-z0-9_]*)\s*(\([^)]*\))\s*(:\s*[^\n/]+?)?\s*(//[^\n]*)?\r?$")

def migrar(path, ids, enlaces):
    s = io.open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in s else "\n"
    mod = re.search(r"^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)", s, re.M).group(1)
    lineas = s.split(nl)
    out, hechas, sin = [], 0, []
    for l in lineas:
        m = RE_INTR.match(l)
        if not m:
            out.append(l); continue
        sangria, pub, nombre, params, ret, coment = m.groups()
        clave = mod + "." + nombre
        const = enlaces.get(clave)
        if const is None or const not in ids:
            sin.append(clave); out.append(l); continue
        cab = sangria + (pub or "") + "function " + nombre + params + (" " + ret.strip() if ret else "")
        if coment: cab += "   " + coment.strip()
        out.append(cab)
        out.append(sangria + "  inline CALL_BUILTIN " + str(ids[const]))
        out.append(sangria + "end " + nombre)
        hechas += 1
    io.open(path, "w", encoding="utf-8", newline="").write(nl.join(out))
    return hechas, sin

def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    ids = ids_de_builtin_java()
    enlaces = enlaces_de_intrinsics_java()
    total = 0
    for p in sys.argv[1:]:
        hechas, sin = migrar(p, ids, enlaces)
        total += hechas
        print("%s: %d migradas" % (p, hechas) + ("; SIN registro (no tocadas): " + ", ".join(sin) if sin else ""))
    print("total:", total)

if __name__ == "__main__":
    main()
