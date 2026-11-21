#!/usr/bin/env python3
# enlaces_docs.py — V8 (20-nov-2026): los enlaces internos de la documentación, para el pre-flight de cada versión
# (PUBLICAR.md, la fila de doc_frags). En V7 se comprobaron con un script que no se guardó; éste se queda.
#
# Mira docs/*.html, docs/en/*.html, los .md de docs/ y docs/en/ y los dos README (no FICHAS, ESTADO ni los HECHO_V*,
# que son registro): cada href, y cada enlace Markdown de un .md, que sea relativo tiene que apuntar a un fichero que
# existe y, si lleva #ancla, a un id/name que exista en el destino (en un .md, vale también el ancla que GitHub saca
# de un título). Sólo lee. Sale con 1 si hay alguno roto.
#
#   python -I -X utf8 scripts/enlaces_docs.py
import glob
import os
import re
import sys
import unicodedata

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # V8.1: en scripts/ (antes, en bpgenvm-c/tools/)
FUENTES = (glob.glob(os.path.join(RAIZ, "docs", "*.html")) + glob.glob(os.path.join(RAIZ, "docs", "en", "*.html"))
           + glob.glob(os.path.join(RAIZ, "docs", "*.md")) + glob.glob(os.path.join(RAIZ, "docs", "en", "*.md"))
           + [os.path.join(RAIZ, "README.md"), os.path.join(RAIZ, "README.es.md")])
FUENTES = [f for f in FUENTES if "HECHO_V" not in f and not f.endswith(("FICHAS.md", "ESTADO.md"))]

RE_HREF = re.compile(r'href="([^"]+)"')
RE_MD = re.compile(r'\]\(([^)\s]+)\)')
RE_ID = re.compile(r'(?:id|name)="([^"]+)"')


def slug_github(titulo):
    t = titulo.strip().lower()
    t = re.sub(r"[`*_~]", "", t)
    t = "".join(c for c in t if unicodedata.category(c)[0] in "LN" or c in " -")
    return t.replace(" ", "-")


_cache = {}


def anclas(ruta):
    if ruta not in _cache:
        texto = open(ruta, encoding="utf-8", errors="replace").read()
        s = set(RE_ID.findall(texto))
        if ruta.endswith(".md"):
            for t in re.findall(r"^#{1,6}\s+(.+?)\s*$", texto, re.M):
                s.add(slug_github(t))
        _cache[ruta] = s
    return _cache[ruta]


def main():
    rotos, total = [], 0
    for f in sorted(FUENTES):
        texto = open(f, encoding="utf-8", errors="replace").read()
        enlaces = RE_HREF.findall(texto) + (RE_MD.findall(texto) if f.endswith(".md") else [])
        for h in enlaces:
            if re.match(r"^(https?:|mailto:|data:|javascript:)", h):
                continue
            total += 1
            camino, _, ancla = h.partition("#")
            destino = os.path.normpath(os.path.join(os.path.dirname(f), camino)) if camino else f
            if camino and not os.path.exists(destino):
                rotos.append((f, h, "no existe el fichero"))
                continue
            if ancla and os.path.isfile(destino) and destino.endswith((".html", ".md")):
                if ancla not in anclas(destino):
                    rotos.append((f, h, "no existe el ancla"))
    print("enlaces internos: %d, en %d documentos; rotos: %d" % (total, len(FUENTES), len(rotos)))
    for f, h, por in rotos:
        print("  %s -> %s  (%s)" % (os.path.relpath(f, RAIZ), h, por))
    return 1 if rotos else 0


if __name__ == "__main__":
    sys.exit(main())
