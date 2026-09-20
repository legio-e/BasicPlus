# -*- coding: utf-8 -*-
"""gen_builtins.py — H1 P2 (20-sep): LA tabla de builtins de las dos VMs se GENERA del módulo raíz.

`bpstdlib/Core.bp` declara cada primitiva de la VM como una función con cuerpo `inline
CALL_BUILTIN <id>` (la sección «LAS PRIMITIVAS DE LA VM»). De ahí salen:

  miVM/src/main/java/edu/bpgenvm/bytecode/Builtin.java   el enum (constante, nombre BP, id = orden)
  bpgenvm-c/src/builtin_ids.h                              el enum de C (BUILTIN_X = n) con la firma BP

y se COMPRUEBA que los demás módulos de la stdlib (`Math.sin` → 24…) usan ids que existen.
Reglas: el id es el número escrito; la primera declaración con ese número es la CANÓNICA (su
nombre es el nombre BP del builtin) y las siguientes con el mismo número son alias (`newObjArray`
de `__newRefArray`); la constante Java/C es el nombre BP sin `__` en MAYÚSCULAS_CON_GUIONES;
los ids tienen que ser 0..N-1 sin huecos (el enum Java usa el ordinal). Se niega a escribir si
un id existente cambia de nombre respecto a lo que hay generado (errores sí, silenciosos no),
salvo con --forzar.

    python scripts/gen_builtins.py            comprueba (sale 1 si algo no casa)
    python scripts/gen_builtins.py --escribir  genera los dos ficheros
"""
import io, os, re, sys, glob

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORE = os.path.join(RAIZ, "bpstdlib", "Core.bp")
JAVA = os.path.join(RAIZ, "miVM", "src", "main", "java", "edu", "bpgenvm", "bytecode", "Builtin.java")
CH   = os.path.join(RAIZ, "bpgenvm-c", "src", "builtin_ids.h")

RE_FUNC = re.compile(r"^\s*(public\s+)?function\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*(:\s*([^\n/]+?))?\s*(//.*)?$")
RE_INLINE = re.compile(r"^\s*inline\s+CALL_BUILTIN\s+(\d+)\s*$")

def constante(bpname):
    n = bpname.lstrip("_")
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", n).upper()

def leer(path):
    """[(id, nombre, firma, publica)] en orden de aparición, de un .bp."""
    out = []
    fn = None
    for l in io.open(path, encoding="utf-8", errors="replace").read().splitlines():
        m = RE_FUNC.match(l)
        if m: fn = (m.group(2), m.group(3).strip(), (m.group(5) or "").strip(), bool(m.group(1))); continue
        m = RE_INLINE.match(l)
        if m and fn: out.append((int(m.group(1)), fn[0], fn[1], fn[2], fn[3])); fn = None
    return out

def tabla_de_core():
    canon = {}; alias = []
    for bid, nombre, params, ret, pub in leer(CORE):
        if bid in canon: alias.append((bid, nombre))
        else: canon[bid] = (nombre, params, ret)
    n = len(canon)
    huecos = [i for i in range(n) if i not in canon]
    fuera = [i for i in canon if i >= n]
    if huecos or fuera:
        print("  ✗ los ids de Core.bp tienen que ser 0..N-1 sin huecos: faltan %s, sobran %s" % (huecos, fuera)); sys.exit(1)
    return [canon[i] for i in range(n)], alias

def generados_hoy():
    """constante → id de lo que hay generado ahora (para no mover un id en silencio)."""
    if not os.path.exists(JAVA): return {}
    s = io.open(JAVA, encoding="utf-8").read()
    cuerpo = s[s.index("public enum Builtin {"):s.index("public final String bpName")]
    cuerpo = re.sub(r"/\*.*?\*/", "", cuerpo, flags=re.S); cuerpo = re.sub(r"//[^\n]*", "", cuerpo)
    return {c: i for i, (c, b) in enumerate(re.findall(r"\b([A-Z][A-Z0-9_]*)\s*\(\s*\"([^\"]*)\"", cuerpo))}

def java(tabla, alias):
    o = []
    o.append("/*")
    o.append(" * GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1) — NO EDITAR A MANO.")
    o.append(" * Cada builtin es una función de Core.bp con cuerpo `inline CALL_BUILTIN <id>`; el id es el número")
    o.append(" * escrito allí y aquí es el ordinal(). Para añadir uno: declararlo en Core.bp (al final, con el")
    o.append(" * número siguiente), implementarlo en VirtualMachine.dispatchBuiltin y en builtins.c, y regenerar.")
    o.append(" */")
    o.append("package edu.bpgenvm.bytecode;")
    o.append("")
    o.append("import java.util.HashMap;")
    o.append("import java.util.Map;")
    o.append("")
    o.append("public enum Builtin {")
    for i, (nombre, params, ret) in enumerate(tabla):
        firma = "%s(%s)%s" % (nombre, params, (": " + ret) if ret else "")
        coma = "," if i < len(tabla) - 1 else ";"
        o.append("    %s(\"%s\")%s   // [%d] %s" % (constante(nombre), nombre, coma, i, firma))
    o.append("")
    o.append("    public final String bpName;")
    o.append("    public final int id;")
    o.append("")
    o.append("    Builtin(String bpName) { this.bpName = bpName; this.id = ordinal(); }")
    o.append("")
    o.append("    private static final Map<String, Builtin> BY_NAME = new HashMap<>();")
    o.append("    static {")
    o.append("        for (Builtin b : values()) BY_NAME.put(b.bpName, b);")
    for bid, nombre in alias:
        o.append("        BY_NAME.put(\"%s\", %s);   // alias en Core.bp" % (nombre, constante(tabla[bid][0])))
    o.append("    }")
    o.append("")
    o.append("    public static Builtin byName(String name) { return BY_NAME.get(name); }")
    o.append("    public static Builtin byId(int id) { return values()[id]; }")
    o.append("}")
    return "\n".join(o) + "\n"

def c_header(tabla):
    o = []
    o.append("/* builtin_ids.h — GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1).")
    o.append(" * NO EDITAR A MANO. Los mismos ids que el enum Builtin de miVM (su ordinal): las dos VMs beben")
    o.append(" * del mismo fichero. La firma BP va al lado, para leerla sin abrir Core.bp. */")
    o.append("#ifndef BPVM_BUILTIN_IDS_H")
    o.append("#define BPVM_BUILTIN_IDS_H")
    o.append("")
    o.append("enum {")
    for i, (nombre, params, ret) in enumerate(tabla):
        firma = "%s(%s)%s" % (nombre, params, (": " + ret) if ret else "")
        o.append("    BUILTIN_%-28s = %3d,   /* %s */" % (constante(nombre), i, firma))
    o.append("    BUILTIN__CUANTOS = %d" % len(tabla))
    o.append("};")
    o.append("")
    o.append("#endif")
    return "\n".join(o) + "\n"

def main():
    escribir = "--escribir" in sys.argv; forzar = "--forzar" in sys.argv
    tabla, alias = tabla_de_core()
    hoy = generados_hoy()
    mal = 0
    for i, (nombre, params, ret) in enumerate(tabla):
        c = constante(nombre)
        if c in hoy and hoy[c] != i:
            print("  ✗ %s tenía el id %d y Core.bp le da el %d: un id no se mueve (usa --forzar si es a propósito)" % (c, hoy[c], i)); mal += 1
    for f in sorted(glob.glob(os.path.join(RAIZ, "bpstdlib", "*.bp"))):
        if os.path.basename(f) == "Core.bp": continue
        for bid, nombre, params, ret, pub in leer(f):
            if bid >= len(tabla):
                print("  ✗ %s.%s usa el id %d, que Core.bp no declara (hay %d)" % (os.path.basename(f)[:-3], nombre, bid, len(tabla))); mal += 1
    print("  %d builtins en Core.bp (%d alias); %d generados hasta hoy" % (len(tabla), len(alias), len(hoy)))
    if mal and not forzar: sys.exit(1)
    if escribir:
        io.open(JAVA, "w", encoding="utf-8", newline="\n").write(java(tabla, alias))
        io.open(CH, "w", encoding="utf-8", newline="\n").write(c_header(tabla))
        print("  escritos %s y %s" % (os.path.relpath(JAVA, RAIZ), os.path.relpath(CH, RAIZ)))
    sys.exit(1 if mal else 0)

if __name__ == "__main__":
    main()
