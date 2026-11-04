#!/usr/bin/env python3
"""traza_dbg.py — V8 #582: la traza de un error con sus nombres.

La placa no tiene el `.dbg`: en el `stackTrace` del EXITED da posiciones («Prueba+50 Prueba+72 Core+1200», de dentro
afuera) y el PC pone los nombres, como en el depurador. Cada posición es la instrucción SIGUIENTE a la que falló o llamó,
así que la línea es la de la posición menos uno. Lo que queda en la línea 0 (el `__init` del módulo) no es del programa.

    from traza_dbg import traduce
    traduce("Prueba+50 Prueba+72", ["build/tanda/Prueba", "../bpstdlib"])   ->  ["Prueba.bp:5", "Prueba.bp:9"]

Un módulo sin `.dbg` sale tal cual («Core+1200»): se pierde la línea, no el dato. El espejo de BpIde, TrazaError.java.
"""
import bisect
import os


def lee_dbg(ruta):
    """(nombre del fuente, posiciones ordenadas, líneas) de un .dbg; la sección `lines` son parejas «rel línea»."""
    fuente, pos, lin, en_lineas = "", [], [], False
    with open(ruta, encoding="utf-8", errors="replace") as f:
        for l in f:
            p = l.split()
            if l.startswith("source "):
                fuente = l[len("source "):].strip()
            elif p == ["lines"]:
                en_lineas = True
            elif en_lineas and len(p) == 2 and p[0].isdigit() and p[1].isdigit():
                pos.append(int(p[0]))
                lin.append(int(p[1]))
            else:
                en_lineas = False
    return os.path.basename(fuente.replace("\\", "/")), pos, lin


def linea_de(pos, lin, rel):
    """La línea de una posición: la entrada más alta que no la pasa, como DbgFile.lineForRelPc."""
    i = bisect.bisect_right(pos, rel) - 1
    return lin[i] if i >= 0 else -1


def traduce(traza, carpetas):
    """La traza del EXITED, marco a marco: «Prueba.bp:23», o «Mod+rel» si no hay .dbg de ese módulo."""
    salida, dbgs = [], {}
    for marco in (traza or "").split():
        mod, _, rel = marco.rpartition("+")
        if mod not in dbgs:
            dbgs[mod] = None
            for c in carpetas:
                r = os.path.join(c, mod + ".dbg")
                if os.path.exists(r):
                    dbgs[mod] = lee_dbg(r)
                    break
        if dbgs[mod] is None:
            salida.append(marco)
            continue
        fuente, pos, lin = dbgs[mod]
        n = linea_de(pos, lin, int(rel) - 1)
        if n > 0:
            salida.append("%s:%d" % (fuente, n))
    return salida
