"""medir_consumo.py — CUÁNTA RAM DEL SISTEMA gasta un programa BP en una placa, y cuánta se queda después (7-oct-2026).

POR QUÉ EXISTE. Eduardo, 7-oct: «Documenta el consumo de memoria. En C3 y C6 es importante ya que vamos muy justos de
RAM». Un programa BP no gasta sólo su heap de BP (que va dentro del bloque de la VM, de tamaño fijo): el núcleo pide
memoria de C a la plataforma para cargarlo —las tablas del cargador y del enlazador, una por módulo importado—, y los
periféricos, la GUI y las natives piden la suya. En la C3 y la C6, sin PSRAM, todo eso sale de la misma RAM interna
que el sistema necesita para vivir. Esta herramienta lo mide; el documento es docs/MEMORIA.md.

CÓMO MIDE. El firmware de las ESP escribe en el log, al acabar cada RUN y ANTES de liberar el programa, la línea
    mem: DRAM interna libre X B | MINIMO HISTORICO Y B (bloque mayor Z B)
(esp32/common/repl_esp32.c, esp32_run_despues; hace falta `log=1` en el ENV). Así X cuenta lo que el programa tenía
vivo al acabar, e Y el peor momento desde el arranque. Por cada SERIE: `reset` de la placa (suelta todo), y sus
programas en orden, uno por tanda (tools/tanda.py: compila, sube, usa los packs si los tiene); tras cada uno se lee el
log y se toma SU línea —la última, más nueva que la del anterior: si el anillo del log se llena, una lectura al final
de la serie perdía las primeras—. Lo que se queda tras un programa es la diferencia entre el `Nada` (un programa vacío)
de antes y el de después: lo que el fin de RUN no devolvió.

    python tools/medir_consumo.py COM3                      # todas las series
    python tools/medir_consumo.py COM3 neopixel gui         # sólo esas
    python tools/medir_consumo.py COM3 --informe ruta.md    # y el informe en markdown

Los programas, en tools/consumo/ (y los de la ronda de V7 por su nombre: GuiWinJson, NatV7…). Sólo las ESP (la línea
«mem:» es suya); ni graba ni toca el ENV: `reset` y RUN.
"""
import json
import os
import re
import subprocess
import sys
import time

AQUI = os.path.dirname(os.path.abspath(__file__))
BPGEN = os.path.dirname(AQUI)
TRABAJO = os.path.join(BPGEN, "build", "consumo")

SERIES = [
    ("neopixel", "dos tiras NeoPixel (#666: no se sueltan al acabar)",
     ["Nada", "Nada", "NeoUna", "NeoUna", "NeoTres", "Nada"]),
    ("pwm_contador", "un PWM y un contador de pulsos (#666)",
     ["Nada", "PwmDos", "PwmDos", "PulsoTres", "PulsoTres", "Nada"]),
    ("modulos_fichero", "cinco módulos más de la stdlib, y un fichero abierto (Limpia lo borra después)",
     ["Nada", "Importa5", "Nada", "FicheroAbierto", "Nada", "Limpia"]),
    ("destructor", "un objeto con destructor vivo al acabar (#668), y un fichero cerrado y uno abierto (8-oct)",
     ["Nada", "ConDestructor", "Nada", "Limpia", "FicheroCerrado", "Nada", "FicheroAbierto", "Nada", "Limpia"]),
    ("gui", "dos programas de GUI",
     ["Nada", "GuiWinJson", "Nada", "GuiArbolG2", "Nada"]),
    ("natives", "dos programas con natives (su código, en RAM ejecutable)",
     ["Nada", "NatV7", "Nada", "FloatNat", "Nada"]),
]

PAT = re.compile(r"\[\s*(\d+)\] mem: DRAM interna libre (\d+) B \| MINIMO HISTORICO (\d+) B \(bloque mayor (\d+) B\)")


def prueba(nombre, v7):
    if nombre in v7:
        return dict(v7[nombre])
    fuente = os.path.join(AQUI, "consumo", nombre + ".bp")
    if not os.path.exists(fuente):
        sys.exit("no encuentro el programa '%s' (ni en tools/consumo/ ni en la ronda de V7)" % nombre)
    return {"nombre": nombre, "fuente": fuente.replace("\\", "/"), "criterio": "no-peta"}


def wire(puerto, *args):
    r = subprocess.run([sys.executable, os.path.join(AQUI, "wire_serie.py"), puerto] + list(args),
                       capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=BPGEN)
    return r.stdout + r.stderr


def info(puerto):
    t = wire(puerto, "info")
    i, j = t.find("{"), t.rfind("}")
    try:
        return json.loads(t[i:j + 1]) if i >= 0 else None
    except ValueError:
        return None


def esperar_placa(puerto, segs=25):
    t0 = time.time()
    while time.time() - t0 < segs:
        d = info(puerto)
        if d and "uptimeMs" in d:
            return d
        time.sleep(1)
    sys.exit("la placa de %s no contesta tras el reset" % puerto)


def correr(puerto, p, n):
    os.makedirs(TRABAJO, exist_ok=True)
    lista = os.path.join(TRABAJO, "uno_%d.json" % n)
    json.dump({"pruebas": [p]}, open(lista, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    r = subprocess.run([sys.executable, os.path.join(AQUI, "tanda.py"), "--lista", lista, "--puertos", puerto,
                        "--informe", os.path.join(TRABAJO, "uno_%d.md" % n)],
                       capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=BPGEN)
    m = re.search(r"^\s+\S+\s+\S+\s+(IDENTICO|NO-PETA|ERROR|FALLO|DIFIERE|SALTADA|NO-ARRANCA|ASERCIONES)\s*$",
                  r.stdout, re.M)
    pl = re.search(r"^placa (\S+) id=\S+ build=(.+?) imagen=(\S+)", r.stdout, re.M)   # la del saludo, no la del INFO
    return (m.group(1) if m else "?"), ("%s (imagen %s, %s)" % (pl.group(1), pl.group(3), pl.group(2)) if pl else None)


def main():
    args = [a for a in sys.argv[1:]]
    informe = None
    if "--informe" in args:
        k = args.index("--informe")
        informe = args[k + 1]
        del args[k:k + 2]
    if not args:
        sys.exit(__doc__)
    puerto, pedidas = args[0], args[1:]
    series = [s for s in SERIES if not pedidas or s[0] in pedidas]
    v7 = {p["nombre"]: p for p in json.load(open(os.path.join(AQUI, "tanda_v7.json"), encoding="utf-8"))["pruebas"]}

    d0 = info(puerto) or {}
    placa = d0.get("boardName", "?")
    resultado = {"placa": placa, "series": []}
    n = 0
    for clave, que, programas in series:
        print("== %s — %s" % (clave, que), flush=True)
        wire(puerto, "reset")
        time.sleep(3)
        esperar_placa(puerto)
        filas, previo_t, previo_libre = [], -1, None
        for nombre in programas:
            n += 1
            veredicto, sello = correr(puerto, prueba(nombre, v7), n)
            if sello and resultado["placa"] != sello:
                resultado["placa"] = placa = sello
                print("placa: " + placa, flush=True)
            mems = PAT.findall(wire(puerto, "log"))
            if not mems:
                sys.exit("el log no tiene líneas «mem:» (¿log=1 en el ENV? ¿es una ESP?)")
            t, libre, minimo, mayor = (int(x) for x in mems[-1])
            aviso = "" if t > previo_t else "  !! la última «mem:» no es de este RUN"
            delta = "" if previo_libre is None else "%+d" % (libre - previo_libre)
            print("   %-14s %-9s libre %7d B  minimo %7d B  bloque mayor %7d B  %s%s"
                  % (nombre, veredicto, libre, minimo, mayor, delta, aviso), flush=True)
            filas.append({"programa": nombre, "veredicto": veredicto, "libre": libre, "minimo": minimo,
                          "bloque_mayor": mayor, "t_ms": t, "valida": not aviso})
            previo_t, previo_libre = t, libre
        resultado["series"].append({"serie": clave, "que": que, "filas": filas})

    os.makedirs(TRABAJO, exist_ok=True)
    json.dump(resultado, open(os.path.join(TRABAJO, "consumo.json"), "w", encoding="utf-8"), ensure_ascii=False,
              indent=1)
    if informe:
        with open(informe, "w", encoding="utf-8") as f:
            f.write("# Consumo de RAM del sistema por programa — %s\n\n" % placa)
            f.write("Medido con `tools/medir_consumo.py`: la línea `mem:` del firmware tras cada RUN, antes de liberar el "
                    "programa. «Libre», lo que quedaba con el programa aún vivo; «mínimo», el peor momento desde el "
                    "reset de la serie.\n\n")
            for s in resultado["series"]:
                f.write("## %s — %s\n\n| programa | veredicto | libre | mínimo | bloque mayor |\n|---|---|---|---|---|\n"
                        % (s["serie"], s["que"]))
                for r in s["filas"]:
                    f.write("| %s | %s | %d | %d | %d |\n" % (r["programa"], r["veredicto"], r["libre"], r["minimo"],
                                                            r["bloque_mayor"]))
                f.write("\n")
        print("informe: " + informe)


if __name__ == "__main__":
    main()
