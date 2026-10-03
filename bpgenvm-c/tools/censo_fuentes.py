#!/usr/bin/env python3
"""censo_fuentes.py — V7/H10 #473 (lote D, 280; 3-oct): QUÉ IMAGEN COMPILA CADA FICHERO COMÚN (src/*.c).

`docs/CENSO_FAMILIAS.md` era una foto del 17-ago escrita a mano, y la unificación de V6 cambió casi todas sus filas:
quien planificara con ella repartía mal. Aquí el reparto se LEE de lo que compiló el último build de cada imagen —no
de memoria ni del texto de un CMake con condicionales—:

  host y sim     las variables del Makefile (`make` las imprime) y la receta de `sim`;
  Pico           pico/build/build.ninja;
  S3, C3, C5, C6, P4   <placa>/build/compile_commands.json (idf.py);
  Nucleo, DK2    Debug/src/subdir.mk (lo regenera cada build de CubeIDE: es el último build).

Sin el artefacto de una imagen, su columna dice «sin build»: no se adivina. Cada columna dice de dónde sale y de qué
fecha es: un build viejo da un censo viejo.

    python tools/censo_fuentes.py          (desde bpgenvm-c; la tabla, en markdown, a stdout)
"""
import json, os, re, subprocess, sys, tempfile, time

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SRC = os.path.join(RAIZ, "src")


def comunes_de(textos):
    """Los nombres src/X.c que aparecen en los textos (rutas con / o \\)."""
    vistos = set()
    for t in textos:
        for m in re.finditer(r"(?:^|[\s\"'\\/])src[\\/]([A-Za-z0-9_]+\.c)\b", t):
            vistos.add(m.group(1))
    return vistos


def fecha(p):
    return time.strftime("%d-%m %H:%M", time.localtime(os.path.getmtime(p)))


def del_makefile():
    """host y sim: lo que imprime make (GUI=1 y LVGL=0, los de por defecto) y la receta de `sim`."""
    with tempfile.NamedTemporaryFile("w", suffix=".mk", delete=False) as f:
        f.write("imprime:\n\t@echo $(SRC)\n\t@echo SIMSD $(SIM_SD_SRC)\n")
        extra = f.name
    try:
        out = subprocess.run(["make", "-s", "-f", "Makefile", "-f", extra, "imprime"], cwd=RAIZ,
                             capture_output=True, text=True).stdout
    finally:
        os.unlink(extra)
    lineas = out.strip().splitlines()
    host = comunes_de([lineas[0]]) if lineas else set()
    sim_sd = comunes_de([l for l in lineas if l.startswith("SIMSD")])
    receta = ""
    mk = open(os.path.join(RAIZ, "Makefile"), encoding="utf-8").read().replace("\r\n", "\n")
    m = re.search(r"\nsim:[^\n]*\n(\t[^\n]*\n)+", mk)
    if m:
        receta = m.group(0)
    sim = host | sim_sd | comunes_de([receta])
    f = "Makefile (" + fecha(os.path.join(RAIZ, "Makefile")) + ")"
    return {"host": (host, f), "sim": (sim, f + " + la receta de sim")}


def de_ninja(nombre, ruta):
    p = os.path.join(RAIZ, ruta)
    if not os.path.exists(p):
        return {nombre: (None, "sin build (falta " + ruta + ")")}
    txt = open(p, encoding="utf-8", errors="replace").read()
    objs = [l for l in txt.splitlines() if l.startswith("build ") and ".obj:" in l or l.startswith("build ") and ".o:" in l]
    # build.ninja se reescribe al CONFIGURAR (cmake), no en cada build: su fecha es la de la lista de fuentes.
    return {nombre: (comunes_de(objs), ruta + " (" + fecha(p) + ", la última vez que se configuró)")}


def de_compdb(nombre, ruta):
    p = os.path.join(RAIZ, ruta)
    if not os.path.exists(p):
        return {nombre: (None, "sin build (falta " + ruta + ")")}
    db = json.load(open(p, encoding="utf-8"))
    return {nombre: (comunes_de([e.get("file", "") for e in db]), ruta + " (" + fecha(p) + ")")}


def de_subdir(nombre, ruta):
    p = os.path.join(RAIZ, ruta)
    if not os.path.exists(p):
        return {nombre: (None, "sin build (falta " + ruta + ")")}
    txt = open(p, encoding="utf-8", errors="replace").read().replace("\\\n", " ")
    m = re.search(r"C_SRCS\s*\+=([^\n]*)", txt)
    return {nombre: (comunes_de([m.group(1)]) if m else set(), ruta + " (" + fecha(p) + ")")}


def main():
    imagenes = {}
    imagenes.update(del_makefile())
    imagenes.update(de_ninja("pico", "pico/build/build.ninja"))
    for nom, d in [("s3", "esp32"), ("c3", "esp32c3"), ("c5", "esp32c5"), ("c6", "esp32c6"), ("p4", "esp32p4")]:
        imagenes.update(de_compdb(nom, d + "/build/compile_commands.json"))
    imagenes.update(de_subdir("nucleo", "stm32/Nucleo_u575b/Nucleo_u575b/Debug/src/subdir.mk"))
    imagenes.update(de_subdir("dk2", "stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/src/subdir.mk"))
    orden = ["host", "sim", "pico", "s3", "c3", "c5", "c6", "p4", "nucleo", "dk2"]

    ficheros = sorted(f for f in os.listdir(SRC) if f.endswith(".c"))
    print("# El reparto de `src/*.c` por imagen (generado por `tools/censo_fuentes.py`, %s)\n"
          % time.strftime("%d-%m-%Y %H:%M"))
    print("De dónde sale cada columna:\n")
    for i in orden:
        print("- **%s**: %s" % (i, imagenes[i][1]))
    con = [i for i in orden if imagenes[i][0] is not None]
    en_todas = [f for f in ficheros if all(f in imagenes[i][0] for i in con)]
    print("\n**%d** de %d ficheros comunes están en las %d imágenes con build. Los demás:\n"
          % (len(en_todas), len(ficheros), len(con)))
    print("| fichero | " + " | ".join(orden) + " |")
    print("|---|" + "---|" * len(orden))
    for f in ficheros:
        if f in en_todas:
            continue
        celdas = []
        for i in orden:
            s = imagenes[i][0]
            celdas.append("?" if s is None else ("x" if f in s else "–"))
        print("| `%s` | %s |" % (f, " | ".join(celdas)))
    ninguna = [f for f in ficheros if not any(imagenes[i][0] and f in imagenes[i][0] for i in orden)]
    if ninguna:
        print("\n⚠️ En ninguna imagen: " + ", ".join("`%s`" % f for f in ninguna))
    return 0


if __name__ == "__main__":
    sys.exit(main())
