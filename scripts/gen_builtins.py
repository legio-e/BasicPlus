# -*- coding: utf-8 -*-
"""gen_builtins.py — H1 P2 (20-sep): LA tabla de builtins de las dos VMs se GENERA del módulo raíz.

`bpstdlib/Core.bp` declara cada primitiva de la VM como una función con cuerpo `inline
CALL_BUILTIN <id>` (la sección «LAS PRIMITIVAS DE LA VM»). De ahí salen:

  miVM/src/main/java/edu/bpgenvm/bytecode/Builtin.java   el enum (constante, nombre BP, id = orden)
  bpgenvm-c/src/builtin_ids.h                              el enum de C (BUILTIN_X = n) con la firma BP
  bpgenvm-c/src/builtin_cadenas.h                          #774: dónde están sus parámetros string en la pila

y se COMPRUEBA que los demás módulos de la stdlib (`Math.sin` → 24…) usan ids que existen.

#774 (V8, 2-nov) — UN STRING null DONDE LA PRIMITIVA PIDE UNA CADENA: «referencia nula», como un objeto null (Eduardo:
«una cadena no puede ser null»; la variable sí). De la firma de Core.bp sale dónde está cada parámetro string al llamar
al builtin (bytes bajo sp: el último argumento, arriba; un integer, un float o un boolean ocupan 4, y un long, un double
o una referencia, 8), y las dos VMs lo miran antes de entrar (Builtin.cadenas en miVM, builtin_cadenas.h en la VM-C).
Los que tratan el null como "" a propósito (NULL_ES_VACIA: concatenar y el aviso de un destructor), con su marca: el
null vale, pero no una referencia caducada.
#805 (V8, 7-nov) — Y UNA REFERENCIA A UNA CADENA QUE YA NO EXISTE: la misma tabla, «referencia a objeto eliminado
(use-after-free)» antes de entrar, en todos (Eduardo: la lectura de alto nivel llama a la de bajo nivel, y si le dice
que el string no existe, salta el error).
#807 (V8, 13-nov) — Y LOS DEMÁS PARÁMETROS REFERENCIA (los arrays, any, Object): dónde está cada uno, en otra tabla
(Builtin.referencias() en miVM, bpvm_builtin_referencias en builtin_cadenas.h), para mirar sólo que siga vivo; el null
lo trata cada builtin. Fuera, los de SIN_MIRAR_REFERENCIAS, cada uno con su porqué.
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
CC   = os.path.join(RAIZ, "bpgenvm-c", "src", "builtin_cadenas.h")

# #774 — los builtins con un parámetro string que tratan el null como "" (como print): el null, sin comprobar; #805,
# una referencia caducada, sí.
NULL_ES_VACIA = {
    "__strCopy": "la concatenación (los __strconcat del compilador): un null se concatena como \"\"",
    "__avisaDestructor": "escribe el mensaje de un destructor que lanzó (e.msg, que puede ser null)",
}
# #807 — los builtins cuyos parámetros referencia que no son string (los arrays, any, Object) NO se miran a la entrada.
# Los demás, sí: sólo que la referencia siga viva —una caducada es «referencia a objeto eliminado (use-after-free)», como
# en los opcodes—; el null lo trata cada builtin.
SIN_MIRAR_REFERENCIAS = {
    "__eventRaise": "el compilador lo apila a su manera (recv arriba y dest con 4 B, #811): su firma no dice dónde están",
    "__liberaSiSinDestructores": "la maquinaria de los destructores (Core.__destruye): trabaja con lo que se destruye",
    "__ordenDestruccion": "la maquinaria de los destructores (Core.__destruye): trabaja con lo que se destruye",
    "__preparaDestructor": "la maquinaria de los destructores (Core.__destruye): trabaja con lo que se destruye",
    "__liberaSolo": "la maquinaria de los destructores (Core.__destruye): trabaja con lo que se destruye",
    "__avisaDestructor": "la maquinaria de los destructores (Core.__destruye): trabaja con lo que se destruye",
}
# #774 — el nombre del mensaje, cuando el del builtin despista (lo llama la stdlib por otro, con el mismo string)
NOMBRE_DE_ERROR = {
    "__fstateSize": "File",       # IO.File(path, mode), antes de abrir
    "__fopen": "File",
    "__readFileI": "readFile",    # readFile(path, ignorarError)
    "__overlayLoad": "Overlay.load",
    "__overlayExe": "Overlay.tryExe",
}
TAM_4 = ("integer", "float", "boolean", "byte", "short")   # en la pila, 4 bytes
TAM_8 = ("long", "double")                                  # 8; una referencia (string, any, Object, un array), 8

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

def tamano(tipo):
    """Lo que ocupa en la pila un argumento de ese tipo, o None si no se sabe."""
    if tipo.endswith("[]") or tipo in ("string", "any", "Object"): return 8
    if tipo in TAM_4: return 4
    if tipo in TAM_8: return 8
    return None

def posiciones(nombre, params):
    """[(bytes bajo sp, tipo)] de cada parámetro al llamar al builtin, del primero al último (el último, arriba)."""
    ps = [p.strip() for p in params.split(",") if p.strip()]
    if any(":" not in p for p in ps):
        print("  ✗ %s(%s): un parámetro sin tipo" % (nombre, params)); sys.exit(1)
    tipos = [p.split(":", 1)[1].strip() for p in ps]
    tams = [tamano(t) for t in tipos]
    if None in tams:
        print("  ✗ %s(%s): un tipo sin tamaño conocido en la pila (%s)" % (nombre, params, tipos[tams.index(None)])); sys.exit(1)
    return [(sum(tams[k:]), t) for k, t in enumerate(tipos)]

def cadenas(nombre, params):
    """#774 — dónde está cada parámetro string al llamar al builtin: bytes bajo sp, del primero al último."""
    return [off for off, t in posiciones(nombre, params) if t == "string"]   # #805: también los de NULL_ES_VACIA

def referencias(nombre, params):
    """#807 — dónde está cada parámetro referencia que no es string (un array, any, Object), salvo en los de
    SIN_MIRAR_REFERENCIAS: bytes bajo sp, del primero al último."""
    if nombre in SIN_MIRAR_REFERENCIAS: return []
    return [off for off, t in posiciones(nombre, params) if t.endswith("[]") or t in ("any", "Object")]

def nombre_de_error(nombre):
    """El del mensaje, «referencia nula: <nombre> de un string null»: el nombre BP sin los __ (o el de NOMBRE_DE_ERROR)."""
    if nombre in NOMBRE_DE_ERROR: return NOMBRE_DE_ERROR[nombre]
    return nombre[2:] if nombre.startswith("__") else nombre

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
        offs = "".join(", %d" % x for x in cadenas(nombre, params))
        o.append("    %s(\"%s\"%s)%s   // [%d] %s" % (constante(nombre), nombre, offs, coma, i, firma))
    o.append("")
    o.append("    public final String bpName;")
    o.append("    public final int id;")
    o.append("    /** #774 — dónde está cada parámetro string al llamarlo (bytes bajo sp): un null ahí es «referencia nula», salvo")
    o.append("     *  en los que lo tratan como \"\" (nuloVale); #805, y una referencia caducada, en todos, «objeto eliminado». */")
    o.append("    public final int[] cadenas;")
    o.append("")
    o.append("    Builtin(String bpName, int... cadenas) { this.bpName = bpName; this.id = ordinal(); this.cadenas = cadenas; }")
    o.append("")
    o.append("    /** #774 — el nombre del mensaje «referencia nula: <nombre> de un string null»: el BP, sin los __, salvo los que")
    o.append("     *  despistan (NOMBRE_DE_ERROR en el generador; el mismo texto que builtin_cadenas.h). */")
    o.append("    public String nombreDeError() {")
    o.append("        switch (this) {")
    for nombre, nuevo in sorted(NOMBRE_DE_ERROR.items()):
        o.append("            case %s: return \"%s\";" % (constante(nombre), nuevo))
    o.append("            default: return bpName.startsWith(\"__\") ? bpName.substring(2) : bpName;")
    o.append("        }")
    o.append("    }")
    o.append("")
    o.append("    /** #774/#805 — ¿trata el null de sus parámetros string como \"\"? (NULL_ES_VACIA en el generador; el mismo")
    o.append("     *  que la marca nulo_vale de builtin_cadenas.h). Una referencia caducada no vale en ninguno. */")
    o.append("    public boolean nuloVale() {")
    o.append("        switch (this) {")
    for nombre in sorted(NULL_ES_VACIA):
        o.append("            case %s: return true;" % constante(nombre))
    o.append("            default: return false;")
    o.append("        }")
    o.append("    }")
    o.append("")
    o.append("    /** #807 — dónde está cada parámetro referencia que no es un string (los arrays, any, Object) al llamarlo (bytes")
    o.append("     *  bajo sp): una referencia caducada ahí es «objeto eliminado», antes de entrar; el null lo trata cada builtin.")
    o.append("     *  Sin los de SIN_MIRAR_REFERENCIAS en el generador (el compilador los apila a su manera, o son de los")
    o.append("     *  destructores). La misma tabla que bpvm_builtin_referencias en builtin_cadenas.h. */")
    o.append("    public int[] referencias() { return REFERENCIAS[ordinal()]; }")
    o.append("")
    o.append("    private static final int[] NINGUNA = {};")
    o.append("    private static final int[][] REFERENCIAS = new int[values().length][];")
    o.append("    static {")
    o.append("        java.util.Arrays.fill(REFERENCIAS, NINGUNA);")
    for i, (nombre, params, ret) in enumerate(tabla):
        refs = referencias(nombre, params)
        if refs:
            o.append("        REFERENCIAS[%d] = new int[] { %s };   // %s(%s)" % (i, ", ".join(str(x) for x in refs), nombre, params))
    o.append("    }")
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

def c_cadenas(tabla):
    filas = []          # (nombre de error, offsets, id, firma)
    indice = []         # por id: su fila (0, ninguna)
    for i, (nombre, params, ret) in enumerate(tabla):
        offs = cadenas(nombre, params)
        if not offs: indice.append(0); continue
        if len(offs) > 3:
            print("  ✗ %s: más de 3 parámetros string (la tabla de C tiene sitio para 3)" % nombre); sys.exit(1)
        if max(offs) > 255:
            print("  ✗ %s: un parámetro string a más de 255 bytes de sp (la tabla de C usa un byte)" % nombre); sys.exit(1)
        filas.append((nombre_de_error(nombre), offs, i, "%s(%s)%s" % (nombre, params, (": " + ret) if ret else ""),
                      1 if nombre in NULL_ES_VACIA else 0))
        indice.append(len(filas))
    if len(filas) > 255:
        print("  ✗ %d builtins con parámetros string: la tabla de C indexa con un byte" % len(filas)); sys.exit(1)
    o = []
    o.append("/* builtin_cadenas.h — GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1).")
    o.append(" * NO EDITAR A MANO. Sólo lo incluye builtins.c.")
    o.append(" *")
    o.append(" * #774 (V8, 2-nov) — LOS PARÁMETROS string DE CADA BUILTIN: dónde está su referencia al llamarlo (bytes bajo sp;")
    o.append(" * el último argumento, arriba). Un null ahí es «referencia nula: <nombre> de un string null», antes de entrar")
    o.append(" * (bpvm_call_builtin); lo mismo que Builtin.cadenas en miVM. Los que tratan el null como \"\" (concatenar y el")
    o.append(" * aviso de un destructor: NULL_ES_VACIA en el generador), con su marca: nulo_vale.")
    o.append(" * #805 (V8, 7-nov) — y una referencia a una cadena que ya no existe, en todos: «referencia a objeto eliminado")
    o.append(" * (use-after-free)», también antes de entrar. */")
    o.append("#ifndef BPVM_BUILTIN_CADENAS_H")
    o.append("#define BPVM_BUILTIN_CADENAS_H")
    o.append("")
    o.append("#include <stdint.h>")
    o.append("#include \"builtin_ids.h\"")
    o.append("")
    o.append("typedef struct {")
    o.append("    const char* nombre;   /* el del mensaje: el nombre BP, sin los __ */")
    o.append("    uint8_t     off[3];   /* bytes bajo sp; 0, no hay más */")
    o.append("    uint8_t     nulo_vale;   /* 1: el null es \"\" (NULL_ES_VACIA); una referencia caducada no vale nunca */")
    o.append("} bpvm_builtin_cadenas_t;")
    o.append("")
    o.append("static const bpvm_builtin_cadenas_t bpvm_builtin_cadenas[%d] = {" % (len(filas) + 1))
    o.append("    { \"\", { 0, 0, 0 }, 0 },")
    for nombre, offs, i, firma, nulo in filas:
        o3 = (offs + [0, 0, 0])[:3]
        o.append("    { %-22s { %3d, %3d, %3d }, %d },   /* [%d] %s */" % ("\"%s\"," % nombre, o3[0], o3[1], o3[2], nulo, i, firma))
    o.append("};")
    o.append("")
    o.append("/* La fila de arriba de cada builtin, por su id (0: ningún parámetro string que mirar). */")
    o.append("static const uint8_t bpvm_builtin_cadena_de[BUILTIN__CUANTOS] = {")
    for k in range(0, len(indice), 16):
        o.append("    " + " ".join("%2d," % x for x in indice[k:k + 16]) + "   /* %d.. */" % k)
    o.append("};")
    o.append("")
    # #807 — los demás parámetros referencia
    rfilas = []; rindice = []
    for i, (nombre, params, ret) in enumerate(tabla):
        offs = referencias(nombre, params)
        if not offs: rindice.append(0); continue
        if len(offs) > 3:
            print("  ✗ %s: más de 3 parámetros referencia (la tabla de C tiene sitio para 3)" % nombre); sys.exit(1)
        if max(offs) > 255:
            print("  ✗ %s: un parámetro referencia a más de 255 bytes de sp (la tabla de C usa un byte)" % nombre); sys.exit(1)
        rfilas.append((offs, i, "%s(%s)%s" % (nombre, params, (": " + ret) if ret else "")))
        rindice.append(len(rfilas))
    if len(rfilas) > 255:
        print("  ✗ %d builtins con parámetros referencia: la tabla de C indexa con un byte" % len(rfilas)); sys.exit(1)
    o.append("/* #807 (V8, 13-nov) — LOS DEMÁS PARÁMETROS REFERENCIA DE CADA BUILTIN (los arrays, any, Object): dónde están al")
    o.append(" * llamarlo (bytes bajo sp). Una referencia caducada ahí es «referencia a objeto eliminado (use-after-free)», antes")
    o.append(" * de entrar (bpvm_call_builtin); el null lo trata cada builtin. Sin los de SIN_MIRAR_REFERENCIAS en el generador")
    o.append(" * (el compilador los apila a su manera, o son de los destructores). Lo mismo que Builtin.referencias() en miVM. */")
    o.append("typedef struct {")
    o.append("    uint8_t off[3];   /* bytes bajo sp; 0, no hay más */")
    o.append("} bpvm_builtin_referencias_t;")
    o.append("")
    o.append("static const bpvm_builtin_referencias_t bpvm_builtin_referencias[%d] = {" % (len(rfilas) + 1))
    o.append("    { { 0, 0, 0 } },")
    for offs, i, firma in rfilas:
        o3 = (offs + [0, 0, 0])[:3]
        o.append("    { { %3d, %3d, %3d } },   /* [%d] %s */" % (o3[0], o3[1], o3[2], i, firma))
    o.append("};")
    o.append("")
    o.append("/* La fila de arriba de cada builtin, por su id (0: ningún parámetro referencia que mirar). */")
    o.append("static const uint8_t bpvm_builtin_referencia_de[BUILTIN__CUANTOS] = {")
    for k in range(0, len(rindice), 16):
        o.append("    " + " ".join("%2d," % x for x in rindice[k:k + 16]) + "   /* %d.. */" % k)
    o.append("};")
    o.append("")
    o.append("#endif")
    return "\n".join(o) + "\n"

def main():
    escribir = "--escribir" in sys.argv; forzar = "--forzar" in sys.argv
    tabla, alias = tabla_de_core()
    hoy = generados_hoy()
    mal = 0
    canonicos = set(nombre for nombre, params, ret in tabla)
    for k in sorted(set(NULL_ES_VACIA) | set(NOMBRE_DE_ERROR) | set(SIN_MIRAR_REFERENCIAS)):
        if k not in canonicos:
            print("  ✗ %s (en NULL_ES_VACIA, NOMBRE_DE_ERROR o SIN_MIRAR_REFERENCIAS) no es un builtin de Core.bp" % k); mal += 1
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
    generados = [(JAVA, java(tabla, alias)), (CH, c_header(tabla)), (CC, c_cadenas(tabla))]
    if not escribir:
        # #774 — y que lo generado esté al día: tres ficheros salen de Core.bp, y uno sin regenerar no avisa solo.
        for path, texto in generados:
            hay = io.open(path, encoding="utf-8").read().replace("\r\n", "\n") if os.path.exists(path) else None
            if hay != texto:
                print("  ✗ %s no casa con Core.bp: python scripts/gen_builtins.py --escribir" % os.path.relpath(path, RAIZ)); mal += 1
    if escribir:
        for path, texto in generados:
            io.open(path, "w", encoding="utf-8", newline="\n").write(texto)
        print("  escritos %s" % ", ".join(os.path.relpath(p, RAIZ) for p, t in generados))
    sys.exit(1 if mal else 0)

if __name__ == "__main__":
    main()
