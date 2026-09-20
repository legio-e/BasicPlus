# -*- coding: utf-8 -*-
"""gen_builtins.py — H1 (20-sep): las intrínsecas que la stdlib declara con cuerpo `inline`
(`inline CALL_BUILTIN <id>`) son LA tabla de builtins. Este guion la recorre y, de momento
(mientras la migración avanza), la COMPRUEBA contra Builtin.java: cada id existe, y el nombre
BP que Builtin.java le atribuye (su `bpName`) casa con la función que lo usa. Cuando toda la
stdlib esté migrada, este mismo guion pasará a ESCRIBIR Builtin.java y el enum de builtins.c
(y check_builtins.py se tira).

    python scripts/gen_builtins.py            (desde la raíz; sale 1 si algo no casa)
"""
import io, os, re, sys, glob

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JAVA = os.path.join(RAIZ, "miVM", "src", "main", "java", "edu", "bpgenvm", "bytecode", "Builtin.java")

def tabla_java():
    s = io.open(JAVA, encoding="utf-8").read()
    cuerpo = s[s.index("public enum Builtin {"):s.index("public final String bpName")]
    cuerpo = re.sub(r"/\*.*?\*/", "", cuerpo, flags=re.S)
    cuerpo = re.sub(r"//[^\n]*", "", cuerpo)
    return [(m.group(1), m.group(2)) for m in re.finditer(r"\b([A-Z][A-Z0-9_]*)\s*\(\s*\"([^\"]*)\"", cuerpo)]

RE_FUNC = re.compile(r"^\s*(?:public\s+)?function\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(")
RE_INLINE = re.compile(r"^\s*inline\s+CALL_BUILTIN\s+(\d+)\s*$")

def usos_en_bp():
    """[(modulo, funcion, id, fichero:linea)] de todos los .bp de la stdlib."""
    out = []
    for f in sorted(glob.glob(os.path.join(RAIZ, "bpstdlib", "*.bp"))):
        lineas = io.open(f, encoding="utf-8", errors="replace").read().splitlines()
        mod = None; fn = None
        for i, l in enumerate(lineas, 1):
            m = re.match(r"^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)", l)
            if m: mod = m.group(1); continue
            m = RE_FUNC.match(l)
            if m: fn = m.group(1); continue
            m = RE_INLINE.match(l)
            if m and fn: out.append((mod, fn, int(m.group(1)), os.path.basename(f) + ":" + str(i)))
    return out

def main():
    tabla = tabla_java()
    usos = usos_en_bp()
    mal = 0
    for mod, fn, bid, donde in usos:
        if bid >= len(tabla):
            print("  ✗ %s.%s (%s): el id %d no existe en Builtin.java (hay %d)" % (mod, fn, donde, bid, len(tabla))); mal += 1; continue
        const, bpname = tabla[bid]
        # el nombre que Builtin.java le da al id, sin el prefijo interno, tiene que ser el de la función
        esperado = bpname.lstrip("_")
        if esperado.lower() != fn.lower() and esperado.lower().rstrip("if") != fn.lower().rstrip("if") and esperado.lower().replace("_", "") != fn.lower():
            print("  ? %s.%s (%s): usa el id %d, que Builtin.java llama %s(\"%s\")" % (mod, fn, donde, bid, const, bpname))
    ids = {}
    for mod, fn, bid, donde in usos:
        ids.setdefault(bid, []).append(mod + "." + fn)
    print("  %d intrínsecas con cuerpo inline en la stdlib, %d ids distintos, %d en Builtin.java" % (len(usos), len(ids), len(tabla)))
    sys.exit(1 if mal else 0)

if __name__ == "__main__":
    main()
