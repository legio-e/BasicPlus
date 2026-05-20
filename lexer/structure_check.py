#!/usr/bin/env python3
"""Verifica el balance estructural de un programa BASICPLUS."""

import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from smoke_test import Lexer

OPEN_CLOSE = {
    "module":   ("end", "module"),
    "class":    ("end", "class"),
    "function": ("end", "function"),
    "enum":     ("end", "enum"),
    "if":       ("endif", "if"),
    "while":    ("endwh", "while"),
    "switch":   ("endsw", "switch"),
    "try":      ("endtry", "try"),
    "property": ("endprop", "property"),
    "get":      ("endget", "get"),
    "set":      ("endset", "set"),
    "do":       ("loop", "do"),
    "for":      ("next", "for"),
}
CLOSERS = {v[0] for v in OPEN_CLOSE.values()}
ALLOWED_INNER = {
    "if":     {"elseif", "else"},
    "switch": {"case", "default"},
    "try":    {"catch", "finally"},
}
NORMALIZE = {"while_body": "while", "for_body": "for"}


def check(path):
    with open(path) as f:
        src = f.read()
    lx = Lexer(src); lx.tokenize()
    errors = ["LEX " + e for e in lx.errors]
    stack = []
    paren = 0; bracket = 0
    for sl, sc, kind, lex in lx.tokens:
        if kind == "OP":
            if   lex == "(": paren += 1
            elif lex == ")":
                if paren <= 0: errors.append(f"[{sl}:{sc}] ')' sin '(' previa")
                else: paren -= 1
            elif lex == "[": bracket += 1
            elif lex == "]":
                if bracket <= 0: errors.append(f"[{sl}:{sc}] ']' sin '[' previa")
                else: bracket -= 1
            continue
        if kind != "KW": continue
        kw = lex.lower()
        if kw == "do":
            if stack and stack[-1][0] in ("while", "for"):
                top = stack[-1]
                stack[-1] = ("while_body" if top[0]=="while" else "for_body", top[1], top[2])
            else:
                stack.append(("do", sl, sc))
            continue
        if kw in OPEN_CLOSE:
            stack.append((kw, sl, sc)); continue
        if kw == "end":
            target = None
            for b in reversed(stack):
                kn = NORMALIZE.get(b[0], b[0])
                if OPEN_CLOSE.get(kn, (None,))[0] == "end":
                    target = b; break
            if target is None: errors.append(f"[{sl}:{sc}] 'end' sin bloque")
            else: stack.remove(target)
            continue
        if kw in CLOSERS:
            if not stack:
                errors.append(f"[{sl}:{sc}] '{kw}' sin bloque abierto"); continue
            tk = NORMALIZE.get(stack[-1][0], stack[-1][0])
            exp = OPEN_CLOSE.get(tk, (None,))[0]
            if exp != kw:
                errors.append(f"[{sl}:{sc}] '{kw}' no cierra '{tk}' (esperaba '{exp}')")
            else:
                stack.pop()
            continue
        if kw in {"elseif","else","case","default","catch","finally"}:
            if not stack:
                errors.append(f"[{sl}:{sc}] '{kw}' fuera de cualquier bloque"); continue
            par = NORMALIZE.get(stack[-1][0], stack[-1][0])
            ok = [p for p,a in ALLOWED_INNER.items() if kw in a]
            if par not in ok:
                errors.append(f"[{sl}:{sc}] '{kw}' dentro de '{par}' (esperaba {ok})")
    if paren != 0:   errors.append(f"paréntesis desbalanceados: {paren}")
    if bracket != 0: errors.append(f"corchetes desbalanceados: {bracket}")
    for kw, sl, sc in stack:
        errors.append(f"[{sl}:{sc}] bloque '{NORMALIZE.get(kw,kw)}' sin cerrar")
    return errors


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "samples", "hello.bp")
    errs = check(path)
    print(f"=== {path} ===")
    if not errs:
        print("OK — estructura balanceada")
        return 0
    print(f"PROBLEMAS ({len(errs)}):")
    for e in errs: print("  " + e)
    return 1


if __name__ == "__main__":
    sys.exit(main())
