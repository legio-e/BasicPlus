# censo.py — proporciones de codigo COMUN / de FAMILIA / de PLACA en bpgenvm-c.
#
# Cuenta LINEAS DE CODIGO (sin comentarios ni lineas en blanco), porque este
# proyecto comenta muchisimo a proposito y contar lineas brutas mediria la
# documentacion, no el codigo. Se informan las dos cifras de todos modos.
#
# Las tres trampas que el traspaso dejo anotadas y que aqui se respetan:
#   1. Los GENERADOS (los blobs de stdlib: *_mods.c/h) no son codigo escrito.
#   2. Los Core/ de los proyectos CubeIDE los genera ST: se cuentan APARTE.
#   3. "Comun" no es lo mismo que "comun EN USO": un fichero de src/ que solo
#      compila una familia no es comun de verdad. Eso lo mide censo_uso.py.
import io, os, re, json, sys

RAIZ = "C:/lenguajes/pm/bpgenvm-c"

GENERADOS = ("esp32_mods.c", "esp32_mods.h", "stm32_mods.c", "stm32_mods.h",
             "pico_mods.c", "pico_mods.h")

def lineas_codigo(p):
    try:
        s = io.open(p, encoding="utf-8", errors="replace").read()
    except Exception:
        return 0, 0
    brutas = s.count("\n") + (0 if s.endswith("\n") else 1)
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)      # bloque
    s = re.sub(r"//[^\n]*", "", s)                    # linea
    codigo = sum(1 for l in s.split("\n") if l.strip())
    return codigo, brutas

def recorre(d, recursivo=True, salta=()):
    out = []
    if not os.path.isdir(d):
        return out
    for raiz, dirs, ficheros in os.walk(d):
        dirs[:] = [x for x in dirs if x not in ("build", "third_party", ".settings", "Debug")]
        for f in ficheros:
            if not (f.endswith(".c") or f.endswith(".h")):
                continue
            out.append(os.path.join(raiz, f).replace("\\", "/"))
        if not recursivo:
            break
    return out

CUBOS = {
    "comun": [("src", True), ("include", True)],
    "familia": [("esp32/common", True), ("pico", False), ("pico/boards", True),
                ("stm32/port", True)],
    "placa": [("esp32/main", True), ("esp32c3/main", True), ("esp32c6/main", True),
              ("esp32p4/main", True)],
}
# Los Core/ de CubeMX, aparte
CUBEMX = ["stm32/Nucleo_u575b/Nucleo_u575b/Core", "stm32/Discovery_u5g9j/Discovery_u5g9j/Core"]

res = {}
detalle = {}
for cubo, dirs in CUBOS.items():
    cod = bru = 0
    gen_cod = gen_bru = 0
    fich = []
    for d, rec in dirs:
        for p in recorre(os.path.join(RAIZ, d), rec):
            nom = os.path.basename(p)
            c, b = lineas_codigo(p)
            if nom in GENERADOS:
                gen_cod += c; gen_bru += b
            else:
                cod += c; bru += b
                fich.append((p[len(RAIZ) + 1:], c))
    res[cubo] = {"codigo": cod, "brutas": bru, "generado_codigo": gen_cod, "ficheros": len(fich)}
    detalle[cubo] = sorted(fich, key=lambda x: -x[1])

cod = bru = 0
for d in CUBEMX:
    for p in recorre(os.path.join(RAIZ, d)):
        c, b = lineas_codigo(p)
        cod += c; bru += b
res["cubemx"] = {"codigo": cod, "brutas": bru, "ficheros": 0}

total = sum(res[k]["codigo"] for k in ("comun", "familia", "placa"))
print("== CENSO DE CODIGO (lineas de codigo, sin comentarios ni blancos) ==\n")
print("%-28s %9s %9s %8s" % ("cubo", "codigo", "brutas", "% del nuestro"))
for k in ("comun", "familia", "placa"):
    r = res[k]
    print("%-28s %9d %9d %7.1f %%" % (k, r["codigo"], r["brutas"], 100.0 * r["codigo"] / total))
print("%-28s %9d %9d %8s" % ("TOTAL nuestro", total, sum(res[k]["brutas"] for k in ("comun","familia","placa")), "100 %"))
print()
print("%-28s %9d %9d   (ST, no nuestro)" % ("CubeMX Core/ (generado ST)", res["cubemx"]["codigo"], res["cubemx"]["brutas"]))
gen = sum(res[k]["generado_codigo"] for k in ("comun", "familia", "placa"))
print("%-28s %9d             (blobs de stdlib, generados)" % ("*_mods.c/h (generado)", gen))
print()
print("== los cinco ficheros mas grandes de cada cubo ==")
for k in ("comun", "familia", "placa"):
    print("\n-- %s --" % k)
    for p, c in detalle[k][:5]:
        print("   %6d  %s" % (c, p))
json.dump({"res": res, "detalle": {k: detalle[k] for k in detalle}}, io.open(
    "C:/Users/Eduardo/AppData/Local/Temp/claude/C--lenguajes-pm-miVM/92dd594f-bfc5-43df-be19-03d8a397bfe5/scratchpad/censo.json", "w", encoding="utf-8"))
