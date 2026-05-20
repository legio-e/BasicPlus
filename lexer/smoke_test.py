#!/usr/bin/env python3
"""
Smoke test: réplica mínima de la lógica del lexer C# en Python para
verificar que samples/hello.bp se tokeniza sin errores.

NO sustituye al lexer real (que está en Lexer.cs); es solo una
validación rápida de que las reglas léxicas son consistentes con el
sample. Si Python encuentra errores aquí, lo más probable es que C#
también los encuentre.
"""

import sys, os

KEYWORDS = {
    "module","end","import","const","var","function","class","extends","enum",
    "property","get","set","endprop","endget","endset","public","final",
    "this","super","if","then","elseif","else","endif","switch","case",
    "default","endsw","while","do","endwh","loop","for","to","step","next",
    "in","break","continue","return","try","catch","finally","endtry","throw",
    "print","integer","float","string","boolean","true","false","null",
    "and","or","not","xor","mod","shl","shr","field"
}

class Lexer:
    def __init__(self, src):
        self.src = src
        self.pos = 0
        self.line = 1
        self.col = 1
        self.errors = []
        self.tokens = []

    def at_end(self):
        return self.pos >= len(self.src)

    def peek(self, off=0):
        i = self.pos + off
        return self.src[i] if i < len(self.src) else '\0'

    def advance(self):
        if self.at_end(): return
        self.pos += 1
        self.col += 1

    def consume_newline(self):
        if self.peek() == '\r' and self.peek(1) == '\n':
            self.pos += 2
        else:
            self.pos += 1
        self.line += 1
        self.col = 1

    def err(self, msg, line=None, col=None):
        self.errors.append(f"[{line or self.line}:{col or self.col}] {msg}")

    def skip_ws_comments(self):
        while not self.at_end():
            c = self.peek()
            if c in (' ', '\t'):
                self.advance()
            elif c == '/' and self.peek(1) == '/':
                while not self.at_end() and self.peek() not in ('\n', '\r'):
                    self.advance()
            elif c == '/' and self.peek(1) == '*':
                sl, sc = self.line, self.col
                self.advance(); self.advance()
                while not self.at_end():
                    if self.peek() == '*' and self.peek(1) == '/':
                        self.advance(); self.advance(); break
                    if self.peek() in ('\n', '\r'):
                        self.consume_newline()
                    else:
                        self.advance()
                else:
                    self.err("comentario de bloque sin cerrar", sl, sc)
            else:
                break

    def is_alpha(self, c): return c.isascii() and c.isalpha()
    def is_digit(self, c): return c.isascii() and c.isdigit()
    def is_alnum_us(self, c): return self.is_alpha(c) or self.is_digit(c) or c == '_'
    def is_hex(self, c): return self.is_digit(c) or c in 'abcdefABCDEF'
    def is_bin(self, c): return c in '01'

    def scan_ident(self, sl, sc):
        start = self.pos
        while not self.at_end() and self.is_alnum_us(self.peek()):
            self.advance()
        lex = self.src[start:self.pos]
        kind = "KW" if lex.lower() in KEYWORDS else "ID"
        self.tokens.append((sl, sc, kind, lex))

    def scan_number(self, sl, sc):
        start = self.pos
        if self.peek() == '0' and self.peek(1) in 'xX':
            self.advance(); self.advance()
            hs = self.pos
            while not self.at_end() and self.is_hex(self.peek()): self.advance()
            lex = self.src[start:self.pos]
            if self.pos == hs: self.err(f"hex vacío: {lex}", sl, sc)
            self.tokens.append((sl, sc, "INT", lex)); return
        if self.peek() == '0' and self.peek(1) in 'bB':
            self.advance(); self.advance()
            hs = self.pos
            while not self.at_end() and self.is_bin(self.peek()): self.advance()
            lex = self.src[start:self.pos]
            if self.pos == hs: self.err(f"bin vacío: {lex}", sl, sc)
            self.tokens.append((sl, sc, "INT", lex)); return
        while not self.at_end() and self.is_digit(self.peek()): self.advance()
        is_float = False
        if self.peek() == '.' and self.is_digit(self.peek(1)):
            is_float = True
            self.advance()
            while not self.at_end() and self.is_digit(self.peek()): self.advance()
        if self.peek() in ('e', 'E'):
            is_float = True
            self.advance()
            if self.peek() in ('+', '-'): self.advance()
            es = self.pos
            while not self.at_end() and self.is_digit(self.peek()): self.advance()
            if self.pos == es: self.err("exponente sin dígitos", sl, sc)
        lex = self.src[start:self.pos]
        self.tokens.append((sl, sc, "FLOAT" if is_float else "INT", lex))

    def scan_string(self, sl, sc):
        start = self.pos
        self.advance()
        terminated = False
        while not self.at_end():
            c = self.peek()
            if c == '"':
                self.advance(); terminated = True; break
            if c in ('\n', '\r'):
                self.err("salto de línea dentro de string"); break
            if c == '\\':
                self.advance()
                if self.at_end(): self.err("escape incompleto"); break
                esc = self.peek()
                if esc not in 'ntr\\"0':
                    self.err(f"escape desconocido \\{esc}")
                self.advance()
            else:
                self.advance()
        if not terminated: self.err("string no terminado", sl, sc)
        self.tokens.append((sl, sc, "STR", self.src[start:self.pos]))

    OPS_2 = {':=', '+=', '-=', '==', '!=', '<=', '>='}
    OPS_1 = set("+-*/|&<>()[],;:.")

    def scan_op(self, sl, sc):
        two = self.peek() + self.peek(1)
        if two in self.OPS_2:
            self.advance(); self.advance()
            self.tokens.append((sl, sc, "OP", two)); return True
        c = self.peek()
        if c in self.OPS_1:
            self.advance()
            self.tokens.append((sl, sc, "OP", c)); return True
        return False

    def tokenize(self):
        while True:
            self.skip_ws_comments()
            if self.at_end(): break
            sl, sc = self.line, self.col
            c = self.peek()
            if c in ('\r', '\n'):
                lex = "\\r\\n" if (c == '\r' and self.peek(1) == '\n') else ("\\r" if c == '\r' else "\\n")
                self.consume_newline()
                self.tokens.append((sl, sc, "NL", lex)); continue
            if self.is_alpha(c): self.scan_ident(sl, sc); continue
            if self.is_digit(c): self.scan_number(sl, sc); continue
            if c == '"': self.scan_string(sl, sc); continue
            if self.scan_op(sl, sc): continue
            self.err(f"carácter inesperado: {c!r}", sl, sc)
            self.advance()
        self.tokens.append((self.line, self.col, "EOF", ""))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "samples", "hello.bp")
    with open(path) as f: src = f.read()
    lx = Lexer(src); lx.tokenize()

    print(f"=== {path} ({len(src)} chars) ===")
    print(f"Tokens: {len(lx.tokens)}")
    print(f"Errores: {len(lx.errors)}")
    for e in lx.errors: print("  " + e)
    # Resumen por tipo
    from collections import Counter
    counts = Counter(t[2] for t in lx.tokens)
    print()
    print("Resumen por tipo:")
    for k, v in sorted(counts.items()): print(f"  {k:6} {v}")
    return 0 if not lx.errors else 1

if __name__ == "__main__":
    sys.exit(main())
