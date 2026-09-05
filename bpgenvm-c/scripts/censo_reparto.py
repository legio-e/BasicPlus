# censo_map.py — el censo por el ARTEFACTO, leyendo el .map del enlazador.
#
# Por que el .map y no las lineas de fichero: contar lineas mide lo ESCRITO;
# el .map mide lo que el enlazador METIO EN LA IMAGEN, que es lo que se pregunta
# cuando se dice "proporciones". La diferencia no es teorica — en la Nucleo,
# `gui_display_sdl.o` esta en el build y aporta 0 bytes, porque el fichero entero
# es un #ifdef que no se cumple.
#
# Y no depende de tener el toolchain de cada arquitectura: el .map ya esta hecho.
import os, re, glob, io, collections

RAIZ = "C:/lenguajes/pm/bpgenvm-c"

MAPAS = [
    ("Pico 2 (RP2350)", RAIZ + "/pico/build/bpvm_pico.elf.map",        "pico"),
    ("Nucleo U575",     RAIZ + "/stm32/Nucleo_u575b/Nucleo_u575b/Debug/Nucleo_u575b.map", "stm32"),
    ("Discovery U5G9J", RAIZ + "/stm32/Discovery_u5g9j/Discovery_u5g9j/Debug/Discovery_u5g9j.map", "stm32"),
    ("ESP32-C6",        RAIZ + "/esp32c6/build/bpvm_esp32c6.map",      "esp32"),
    ("ESP32-C3",        RAIZ + "/esp32c3/build/bpvm_esp32c3.map",      "esp32"),
    ("ESP32-P4",        RAIZ + "/esp32p4/build/bpvm_esp32p4.map",      "esp32"),
    ("ESP32-S3",        RAIZ + "/esp32/build/bpvm_esp32.map",          "esp32"),
]

# Sólo secciones de CÓDIGO: los datos (.rodata con los blobs de stdlib) falsearían
# el reparto — un blob generado de 48 KB no es código escrito por nadie.
SEC_CODIGO = re.compile(r"^\s*\.(text|iram)")
LINEA_SEC  = re.compile(r"^\s*(\.\S+)\s*$")
LINEA_DIR  = re.compile(r"^\s*(\.\S+)?\s*0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S+)\s*$")
SOLO_TAM   = re.compile(r"^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S+)\s*$")

# El clasificador: por NOMBRE DE FICHERO contra el arbol real, porque cada build
# nombra sus objetos de una forma (`src/interp.o` en CubeIDE, `interp.c.obj` en
# CMake, y en ESP-IDF todo dentro de `libmain.a(interp.c.obj)`, donde la ruta ya
# no dice de que carpeta venia). Las COLISIONES se avisan y se resuelven por
# ruta: `platform_freertos.c` existe en src/ y en pico/, y confundirlas ya me
# costo una medida falsa hoy.
def mapa_nombres():
    m, colisiones = {}, []
    def anota(d, cubo, rec=True):
        base = os.path.join(RAIZ, d)
        if not os.path.isdir(base): return
        for raiz, dirs, fs in os.walk(base):
            dirs[:] = [x for x in dirs if x not in ("build", "third_party", "Debug")]
            for f in fs:
                if not f.endswith(".c"): continue
                if f in m and m[f] != cubo:
                    colisiones.append((f, m[f], cubo))
                m.setdefault(f, cubo)
            if not rec: break
    anota("src", "comun")
    anota("esp32/common", "familia"); anota("stm32/port", "familia")
    anota("pico", "familia", rec=False); anota("pico/boards", "familia")
    for d in ("esp32/main", "esp32c3/main", "esp32c6/main", "esp32p4/main"):
        anota(d, "placa")
    return m, colisiones

NOMBRES, COLISIONES = mapa_nombres()

def clasifica(obj, familia):
    r = obj.replace("\\", "/").lower()
    # ESP-IDF mete todo en un archivo: `esp-idf/main/libmain.a(bpvm_io.c.obj)`.
    # Sin sacar el nombre de dentro del parentesis, la clasificacion daba CERO
    # para las cuatro imagenes ESP32 — y un cero se lee como "no aplica", no
    # como "el parser no sabe". Por eso el script avisa cuando un cubo sale a 0.
    m_arch = re.search(r"\(([^)]+)\)\s*$", r)
    base = os.path.basename(m_arch.group(1) if m_arch else r)
    base = re.sub(r"\.(o|obj)$", "", base)
    if not base.endswith(".c"): base += ".c"
    # 1) por ruta, cuando la ruta lo dice sin ambiguedad
    if "/bpgenvm-c/src/" in r or re.search(r"(^|/)src/[^/]+$", r):  return "comun"
    if familia == "stm32" and "/core/" in r:                        return "cubemx"
    if familia == "pico" and re.search(r"bpvm_pico\.dir/[^/]+$", r):
        # el fichero propio de la Pico gana sobre el homonimo del comun
        return NOMBRES.get(base) if base not in ("platform_freertos.c",) else "familia"
    # 2) por nombre contra el arbol
    return NOMBRES.get(base)

def censa(ruta, familia):
    tot = collections.Counter()
    detalle = collections.Counter()
    if not os.path.exists(ruta):
        return None, None
    sec_actual = None
    for l in io.open(ruta, encoding="utf-8", errors="replace"):
        m = LINEA_SEC.match(l)
        if m:
            sec_actual = m.group(1); continue
        m = LINEA_DIR.match(l)
        if m:
            if m.group(1): sec_actual = m.group(1)
            tam = int(m.group(3), 16); obj = m.group(4)
        else:
            m = SOLO_TAM.match(l)
            if not m: continue
            tam = int(m.group(2), 16); obj = m.group(3)
        if tam <= 0 or not sec_actual or not SEC_CODIGO.match(" " + sec_actual):
            continue
        c = clasifica(obj, familia)
        if c in ("comun", "familia", "placa", "cubemx"):
            tot[c] += tam
            detalle[(c, os.path.basename(obj))] += tam
    return tot, detalle

if COLISIONES:
    print("AVISO - nombres repetidos entre cubos (se resuelven por ruta):")
    for f, a1, b1 in COLISIONES: print("   %s: %s vs %s" % (f, a1, b1))
    print()
print("== CENSO POR EL ARTEFACTO: bytes de CODIGO de cada imagen, por origen ==\n")
print("%-18s %9s %9s %8s %9s  %s" % ("imagen", "comun", "familia", "placa", "total", "comun / familia / placa"))
suma = collections.Counter()
detalles = {}
for nombre, ruta, fam in MAPAS:
    tot, det = censa(ruta, fam)
    if tot is None:
        print("%-18s  (sin .map)" % nombre); continue
    n = tot["comun"] + tot["familia"] + tot["placa"]
    if n == 0:
        print("%-18s  (0 bytes: revisar el parser)" % nombre); continue
    suma.update(tot); detalles[nombre] = det
    extra = ("   + %d B de CubeMX (de ST)" % tot["cubemx"]) if tot["cubemx"] else ""
    print("%-18s %9d %9d %8d %9d   %4.1f %% / %4.1f %% / %4.1f %%%s" % (
        nombre, tot["comun"], tot["familia"], tot["placa"], n,
        100.0*tot["comun"]/n, 100.0*tot["familia"]/n, 100.0*tot["placa"]/n, extra))

n = suma["comun"] + suma["familia"] + suma["placa"]
if n:
    print("\n%-18s %9d %9d %8d %9d   %4.1f %% / %4.1f %% / %4.1f %%" % (
        "SUMA", suma["comun"], suma["familia"], suma["placa"], n,
        100.0*suma["comun"]/n, 100.0*suma["familia"]/n, 100.0*suma["placa"]/n))
