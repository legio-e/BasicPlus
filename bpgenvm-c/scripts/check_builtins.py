# -*- coding: utf-8 -*-
"""check_builtins.py — que las DOS tablas de builtins no diverjan en silencio.

    python bpgenvm-c/scripts/check_builtins.py        (desde la raiz del repo)

POR QUE EXISTE. El id de un builtin es el `ordinal()` del enum `Builtin` de miVM,
y la VM-C lleva los mismos numeros ESCRITOS A MANO. Lo denuncia el propio codigo,
`bpgenvm-c/src/builtins.c`:

    «Estos numeros se escriben A MANO aqui y tienen que ser el ordinal() del enum
     Builtin de miVM. Si divergen, la VM-C ejecuta OTRO builtin y el sintoma no
     apunta a esto.»

Son 226 entradas mantenidas a mano contra 232, y la unica defensa era una
convencion escrita en un comentario («anadir al final»). Esto la convierte en un
error de construccion: errores si, silenciosos no.

ES UN ANDAMIO. La reforma de verdad es de V7 (el modulo raiz: que las dos tablas
se GENEREN de un fichero unico legible; ver la ficha del modulo raiz). Cuando
llegue, este guion se tira — habra sido el andamio.
"""
import io, os, re, sys

RAIZ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
JAVA = os.path.join(RAIZ, "miVM", "src", "main", "java", "edu", "bpgenvm", "bytecode", "Builtin.java")
C    = os.path.join(RAIZ, "bpgenvm-c", "src", "builtins.c")

def lee(p):
    if not os.path.exists(p):
        sys.exit("no existe: " + p)
    return io.open(p, encoding="utf-8", errors="replace").read()

# --- Java: las constantes del enum, EN ORDEN. El id es el ordinal. -----------
def java_ordinales(txt):
    # el cuerpo del enum va de "enum Builtin {" al primer ";" a nivel de linea
    i = txt.index("enum Builtin")
    cuerpo = txt[i:]
    fin = re.search(r"^\s*;\s*$", cuerpo, re.M)
    if fin: cuerpo = cuerpo[:fin.start()]
    orden, vistos = {}, []
    for l in cuerpo.split("\n"):
        l = l.strip()
        if l.startswith("//") or l.startswith("/*") or l.startswith("*"): continue
        m = re.match(r"^([A-Z][A-Z0-9_]*)\s*\(", l)
        if m:
            n = m.group(1)
            if n in orden: sys.exit("constante repetida en Builtin.java: " + n)
            orden[n] = len(vistos); vistos.append(n)
    return orden

# --- C: los numeros escritos a mano ------------------------------------------
def c_numeros(txt):
    out = {}
    for m in re.finditer(r"\bBUILTIN_([A-Z0-9_]+)\s*=\s*(\d+)", txt):
        out[m.group(1)] = int(m.group(2))
    return out

jav = java_ordinales(lee(JAVA))
cee = c_numeros(lee(C))
print("miVM (Builtin.java): %d constantes  |  VM-C (builtins.c): %d numeros" % (len(jav), len(cee)))

errores = []
for nombre, num in sorted(cee.items(), key=lambda x: x[1]):
    if nombre not in jav:
        errores.append("  BUILTIN_%-24s = %-4d  pero miVM NO tiene esa constante" % (nombre, num))
    elif jav[nombre] != num:
        errores.append("  BUILTIN_%-24s = %-4d  pero en miVM su ordinal es %d  <-- DIVERGEN"
                       % (nombre, num, jav[nombre]))

# Lo que miVM tiene y la VM-C no implementa NO es un error: es una diferencia
# conocida (hay builtins que solo existen en el lado Java). Se informa y ya.
solo_java = [n for n in jav if n not in cee]

if errores:
    print("")
    print("DIVERGENCIAS (%d) — un .mod ejecutaria OTRA funcion y nadie lo diria:" % len(errores))
    for e in errores: print(e)
    print("")
    print("Arreglo: el id es el ordinal() del enum de miVM. Anadir SIEMPRE al final")
    print("del enum, y copiar ese mismo numero en builtins.c.")
    sys.exit(1)

print("las %d entradas de la VM-C casan con su ordinal en miVM" % len(cee))
if solo_java:
    print("(%d builtins solo en miVM, sin numero en la VM-C: %s%s)"
          % (len(solo_java), ", ".join(sorted(solo_java)[:6]),
             ", ..." if len(solo_java) > 6 else ""))
sys.exit(0)
