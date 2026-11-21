# -*- coding: utf-8 -*-
"""gen_core_raiz.py — H1 P2 (20-sep), guion de UNA VEZ: escribe la sección «las primitivas de la
VM» de bpstdlib/Core.bp — las 240 intrínsecas, una por id, con su firma y su cuerpo `inline` —
a partir de lo que había: Builtin.java (ids y nombres), los 65 `addBuiltin` del semántico (las
firmas de las globales: públicas, visibles sin cualificar por ser Core la raíz), las declaraciones
`inline` de los módulos de la stdlib (las firmas de las de módulo: privadas de Core, sólo son la
tabla) y seis firmas a mano (las que sólo usa el compilador). Después de esto, Core.bp es LA
fuente y Builtin.java / builtins.c se generan de él (scripts/gen_builtins.py).

    python scripts/gen_core_raiz.py > /tmp/raiz.txt     (y se pega en Core.bp)
"""
import io, os, re, sys, glob
sys.stdout.reconfigure(encoding='utf-8')
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEM = os.path.join(R, 'toolchain/lexer-java/src/main/java/basicplus/frontend/SemanticAnalyzer.java')
EMI = os.path.join(R, 'toolchain/lexer-java/src/main/java/basicplus/frontend/MivmEmitter.java')
BJ = os.path.join(R, 'toolchain/miVM/src/main/java/edu/bpgenvm/bytecode/Builtin.java')

TIPO = {'PrimitiveType.INTEGER': 'integer', 'PrimitiveType.STRING': 'string', 'PrimitiveType.FLOAT': 'float',
        'PrimitiveType.BOOLEAN': 'boolean', 'PrimitiveType.INT8': 'int8', 'PrimitiveType.UINT8': 'byte',
        'PrimitiveType.INT16': 'int16', 'PrimitiveType.UINT16': 'word', 'PrimitiveType.LONG': 'long',
        'PrimitiveType.DOUBLE': 'double', 'AnyType.INSTANCE': 'any', 'INT_ARRAY': 'integer[]',
        'BYTE_ARRAY': 'byte[]', 'LONG_ARRAY': 'long[]', 'DOUBLE_ARRAY': 'double[]', 'STRING_ARRAY': 'string[]',
        'new ArrayType(new BpType.ClassType(objectCls))': 'Object[]', 'VoidType.INSTANCE': None}

# los que NO son CALL_BUILTIN sino un opcode (MivmEmitter.narrowCastOpFor / arrayAllocOpFor)
OPCODE = {'byte': 'I32_TO_U8', 'int8': 'I32_TO_I8', 'word': 'I32_TO_U16', 'int16': 'I32_TO_I16',
          'newByteArray': 'NEWARRAY_I8', 'newLongArray': 'NEWARRAY_I64', 'newDoubleArray': 'NEWARRAY_I64'}
ALIAS = {'newObjArray': '__newRefArray', 'growObjArray': '__growRefArray'}   # Builtin.BY_NAME.put
A_MANO = {
    '__throwRte':       ('msg: string', None),
    '__longToString':   ('v: long', 'string'),
    '__doubleToString': ('v: double', 'string'),
    '__guiChange':      ('id: integer', None),
    '__eventRaise':     ('recv: any, dest: any, nargs: integer, masks: integer', None),   # args… delante, en la pila
    '__runArg':         ('defecto: string', 'string'),
}

def globales():
    s = io.open(SEM, encoding='utf-8').read()
    seg = s[s.index('private static Scope builtinScope()'):s.index('private static void addBuiltin(')]
    out = {}
    for n, ret, pn, pt in re.findall(r'addBuiltin\(s,\s*"([^"]+)",\s*([^,]+),\s*new String\[\]\{([^}]*)\},\s*new BpType\[\]\{([^}]*)\}\)', seg):
        nombres = [x.strip().strip('"') for x in pn.split(',') if x.strip()]
        tipos = [TIPO[x.strip()] for x in re.split(r',(?![^(]*\))', pt) if x.strip()]
        params = ', '.join(a + ': ' + b for a, b in zip(nombres, tipos))
        out[n] = (params, TIPO[ret.strip()])
    out['input'] = ('', 'string'); out['parseInt'] = ('s: string', 'integer'); out['parseFloat'] = ('s: string', 'float')
    return out

KN = {'integer','float','string','boolean','byte','int8','word','int16','long','double','any','Object'}
def any_si_ajeno(sig):
    """En la tabla de Core un tipo de OTRO módulo (Component, de Gui) es `any`: la firma real vive en su módulo."""
    return re.sub(r'(?<![A-Za-z0-9_])([A-Za-z_][A-Za-z0-9_.]*)(\[\])?(?=\s*(,|$))', lambda m: (m.group(1) if m.group(1) in KN else 'any') + (m.group(2) or ''), sig)

def de_modulos():
    out = {}
    for f in sorted(glob.glob(os.path.join(R, 'bpstdlib', '*.bp'))):
        if os.path.basename(f) == 'Core.bp': continue
        t = io.open(f, encoding='utf-8', errors='replace').read()
        for m in re.finditer(r'function\s+(\w+)\s*\(([^)]*)\)\s*(:\s*([^\n/]+?))?\s*(//[^\n]*)?\r?\n\s*inline CALL_BUILTIN (\d+)', t):
            out.setdefault(int(m.group(6)), (any_si_ajeno(m.group(2).strip()), any_si_ajeno((m.group(4) or '').strip()) or None, os.path.basename(f)[:-3] + '.' + m.group(1)))
    return out

def main():
    b = io.open(BJ, encoding='utf-8').read()
    cuerpo = b[b.index('public enum Builtin {'):b.index('public final String bpName')]
    cuerpo = re.sub(r'/\*.*?\*/', '', cuerpo, flags=re.S); cuerpo = re.sub(r'//[^\n]*', '', cuerpo)
    pares = re.findall(r'\b([A-Z][A-Z0-9_]*)\s*\(\s*"([^"]*)"', cuerpo)
    glob_ = globales(); mods = de_modulos()
    por_bpname = {}
    for n, firma in glob_.items():
        if n not in ALIAS: por_bpname[n] = (n, firma)   # la canónica primero (su nombre es el bpName)
    print('  // ------------------------------------------------------------------------------')
    print('  // LAS PRIMITIVAS DE LA VM (H1, 20-sep) — una función por builtin, en orden de id. El')
    print('  // cuerpo es su bytecode (`inline CALL_BUILTIN <id>`, o el opcode directo): Builtin.java y')
    print('  // el enum de builtins.c se GENERAN de estas líneas (scripts/gen_builtins.py), y el id es')
    print('  // el número escrito: no se mueve. Las `public` se ven sin cualificar en todo programa')
    print('  // (Core es el módulo raíz); las demás son sólo la tabla: cada módulo de la stdlib declara')
    print('  // las suyas con el mismo número (Math.sin → 24). Las `__` son del compilador.')
    print('  // ------------------------------------------------------------------------------')
    faltan = []
    for i, (const, bpname) in enumerate(pares):
        if bpname in por_bpname:
            n, (params, ret) = por_bpname[bpname]; pub = 'public '; nota = ''
            if n != bpname: nota = '   // alias: ' + n
            nombre = n
        elif i in mods:
            params, ret, quien = mods[i]; pub = ''; nombre = bpname; nota = '   // ' + quien
        elif bpname in A_MANO:
            params, ret = A_MANO[bpname]; pub = ''; nombre = bpname; nota = '   // sólo el compilador'
        else:
            faltan.append((i, const, bpname)); continue
        cuerpo_inl = OPCODE.get(nombre, 'CALL_BUILTIN ' + str(i))
        print('  %sfunction %s(%s)%s%s' % (pub, nombre, params, ': ' + ret if ret else '', nota))
        print('    inline ' + cuerpo_inl)
        print('  end ' + nombre)
        for al, canon in ALIAS.items():   # un segundo nombre del mismo id (Builtin.BY_NAME.put)
            if canon == bpname:
                params2, ret2 = glob_[al]
                print('  public function %s(%s)%s   // alias de %s' % (al, params2, ': ' + ret2 if ret2 else '', canon))
                print('    inline ' + cuerpo_inl)
                print('  end ' + al)
    # globales que no son un builtin de Builtin.java (opcodes puros)
    for n, (params, ret) in glob_.items():
        if ALIAS.get(n, n) not in dict((bp, c) for c, bp in pares):
            print('  public function %s(%s)%s   // opcode, no builtin' % (n, params, ': ' + ret if ret else ''))
            print('    inline ' + OPCODE[n])
            print('  end ' + n)
    if faltan:
        print('// SIN FIRMA:', faltan, file=sys.stderr); sys.exit(1)

if __name__ == '__main__':
    main()
