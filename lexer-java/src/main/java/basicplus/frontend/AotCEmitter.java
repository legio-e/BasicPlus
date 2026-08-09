// ============================================================
// AotCEmitter.java
// Emisor AOT — toma funciones BP marcadas con `native` y genera
// C source equivalente para compilar en el firmware.
//
// Subset mínimo (H3 #157 fase 1):
//   - Tipos: integer (i32 únicamente). Float, string, arrays → TODO.
//   - Statements: IfStmt, ReturnStmt, VarDecl (con init), AssignStmt
//                 (a locales), ExprStmt (call con efectos).
//   - Expressions: IntLitExpr, IdentifierExpr (locales), BinaryExpr
//                  (+,-,*,/,mod,<,>,<=,>=,==,!=), UnaryExpr (-,not),
//                  CallExpr (a otras funciones AOT del mismo módulo
//                  por C call directa; a interpreted → fallback con
//                  vm_call_bp helper TODO).
//
// El emisor genera DOS funciones por cada native:
//   1) static int32_t aot_<Mod>_<func>(struct bpvm* vm, int32_t arg1, ...)
//   2) static void thunk_<Mod>_<func>(struct bpvm* vm, uint32_t* sp_p,
//                                     uint32_t* bp_p)
// más una función `aot_<Mod>_register` que registra todos los thunks
// en el AOT registry tras link.
//
// Convención: se asume que el firmware tiene bpvm_aot_register_by_name
// y el helper bpvm_read_i32_be / bpvm_write_i32_be (igual que el
// thunk manual de #160).
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import basicplus.frontend.BpType.*;   // #173 — PrimitiveType (anidado en BpType)

public final class AotCEmitter {

    /** Excepción específica para señalar que una construcción no se
     *  puede traducir a AOT C. El caller la captura y reporta. */
    public static final class UnsupportedAotException extends RuntimeException {
        public UnsupportedAotException(String msg) { super(msg); }
    }

    private final String moduleName;
    private final StringWriter out = new StringWriter();
    private final PrintWriter w = new PrintWriter(out);

    /** Funciones nativas vistas en este módulo — para que el emisor
     *  reconozca CallExpr internos como C call directa. */
    private final Set<String> nativeFuncNames = new HashSet<>();

    /** Tabla nombre → FuncDef para resolver firma desde CallExpr. */
    private final java.util.Map<String, Ast.FuncDef> nativeFuncDefs = new java.util.HashMap<>();

    /** #211 — Tabla nombre → FuncDef de TODAS las funciones del módulo
     *  (native y BP normales). Permite que una `native` que llama a una
     *  función BP del mismo módulo resuelva su firma y emita la llamada-
     *  puente (vm->aot_helpers->call_bp_i32) en vez de abortar. */
    private final java.util.Map<String, Ast.FuncDef> allFuncDefs = new java.util.HashMap<>();

    /** #211 — Avisos no fatales recogidos durante la emisión (p.ej. una
     *  llamada native→BP que cruza al intérprete y pierde la velocidad
     *  AOT). El caller (Main/AotMain) los imprime tras emitModule. */
    private final List<String> warnings = new ArrayList<>();
    public List<String> getWarnings() { return warnings; }

    /** #211 — nombre de la función AOT que se está emitiendo (para mensajes
     *  de aviso). Fijado al inicio de cada emitFunction. */
    private String currentFuncName = "?";

    /** Contador para nombres únicos en for-loops (avoid colisión de
     *  __end/__step entre fors anidados). */
    private int forCounter = 0;

    /** #172 — nombres definidos en el ámbito de la función actual
     *  (parámetros + VarDecl + induction de for). Cualquier
     *  IdentifierExpr cuyo name NO esté aquí se asume variable
     *  nivel-módulo y se emite como acceso directo `mem + cs + OFFSET`,
     *  donde `cs` es la CS del módulo (cacheada via helper
     *  find_module_cs) y OFFSET es el literal compile-time guardado
     *  en `moduleVarOffsets`.
     *
     *  Limpieza: se vacía al inicio de cada emitFunction. */
    private final Set<String> localNames = new HashSet<>();

    /** #172 — offsets CS-relativos de variables nivel-módulo. Asignados
     *  en {@link #precomputeModuleVarOffsets} antes de emitir cualquier
     *  función, recorriendo module.defs en orden de declaración. Convención
     *  empírica del MivmEmitter: vars y consts comparten el área negativa
     *  desde CS hacia abajo, cada uno gana el siguiente slot de 4 bytes.
     *  El offset es negativo: counter → -4, segundo decl → -8, etc.
     *
     *  Si AotCEmitter ve un IdentifierExpr no-local que NO aparece aquí,
     *  asume que es un símbolo importado / cross-module y lanza
     *  UnsupportedAotException (cross-module sigue diferido — #169 / v2). */
    private final java.util.Map<String, Integer> moduleVarOffsets =
            new java.util.LinkedHashMap<>();

    /** #172 — true si alguna función emitida hasta ahora consume globals.
     *  Cuando es true, el header del .c incluye:
     *    - static uint32_t s_module_cs = 0;
     *    - static inline uint32_t aot_<Mod>_cs(struct bpvm* vm) { ... }
     *  Y cada función AOT empieza con: `uint32_t cs = aot_<Mod>_cs(vm);`. */
    private boolean usesModuleGlobals = false;

    /** H3 #158 — si es true, NO emite aot_<Mod>_register (esa función
     *  tiene relocs externas a bpvm_aot_register_by_name + string
     *  literal, que rompen la position-independence del .o standalone).
     *  El loader del .mdn registra automáticamente desde el symtab. */
    private boolean omitRegisterFunc = false;

    /* V5/H4 - `import v1 from pack "SQLI"`: identidad del pack NATIVO del que
     * este modulo toma sus `native` externas. 0 = no habla con ninguno.
     * La marca viaja como uint32 con los 4 caracteres empaquetados, que es como
     * la publica el pack y como la espera `pack_sym`. */
    private int packMarca   = 0;
    private int packVersion = 0;
    /** Una comilla simple. Aparece mucho al emitir C (nombres byte a byte,
     *  mensajes que citan un símbolo) y escaparla cada vez se lee fatal. */
    private static final String Q = "'";

    private int indentLevel = 0;

    public AotCEmitter(String moduleName) {
        this.moduleName = moduleName;
    }

    /** #173 — info semántica para conocer el tipo de cada expresión
     *  (info.exprTypes). Necesaria para distinguir ops de string de las
     *  numéricas: `a + b` es concat si algún operando es string, y
     *  `==`/`!=` de strings comparan contenido (no la ref). Si es null
     *  (emisión sin análisis), las ops string-ambiguas caen al
     *  comportamiento numérico — por eso AotMain / Main lo inyectan
     *  siempre que pueden. Los builtins de string (charAt, substring...)
     *  y los literales NO dependen de esto (se detectan por nombre/nodo). */
    private SemanticInfo semInfo = null;
    public void setSemanticInfo(SemanticInfo info) { this.semInfo = info; }

    /** #173 — ¿el tipo resuelto de `e` es string? false si no hay info. */
    private boolean isStringExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
    }

    /** #173 — ¿el tipo resuelto de `e` es integer? */
    private boolean isIntExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.INTEGER;
    }

    /** #193 — ancho del elemento del array `arr` para elegir el helper AOT.
     *  Devuelve "u8"/"i8" para byte[] (uint8/int8), "i32" para el resto
     *  (incl. tipo desconocido → comportamiento previo). */
    private String arrElemKind(Ast.IExpr arr) {
        if (semInfo != null) {
            BpType t = semInfo.exprTypes.get(arr);
            if (t instanceof ArrayType) {
                BpType el = ((ArrayType) t).element;
                if (el instanceof PrimitiveType) {
                    switch (((PrimitiveType) el).tag) {
                        case UINT8: return "u8";
                        case INT8:  return "i8";
                        default: break;
                    }
                }
            }
        }
        return "i32";
    }
    /** Nombre del helper de carga de elemento según el ancho del array. */
    private String arrLoadFn(Ast.IExpr arr) {
        return "array_load_" + arrElemKind(arr);
    }
    /** Nombre del helper de store de elemento. byte[] (u8/i8) → array_store_i8
     *  (el store trunca; signed/unsigned es el mismo opcode). */
    private String arrStoreFn(Ast.IExpr arr) {
        String k = arrElemKind(arr);
        return ("u8".equals(k) || "i8".equals(k)) ? "array_store_i8" : "array_store_i32";
    }

    /** #173 — emite un operando de concat como string-handle. Si ya es
     *  string, tal cual; si es integer, lo convierte con int_to_string
     *  (cubre el patrón típico `"x = " + n`). Otros tipos mixtos
     *  (float/bool) aún sin soporte → UnsupportedAotException. */
    private void emitStringOperand(Ast.IExpr e) {
        if (isStringExpr(e)) {
            emitExpr(e);
            return;
        }
        if (isIntExpr(e)) {
            w.print("vm->aot_helpers->int_to_string(vm, ");
            emitExpr(e);
            w.print(")");
            return;
        }
        throw new UnsupportedAotException(
            "AOT: concat de string solo soporta string/integer por ahora "
            + "(float/bool → pendiente). line " + ((Ast.Node) e).line);
    }

    /** #173 — escapa los BYTES UTF-8 de un literal para un string C.
     *  Trabajamos sobre bytes (no chars) para que el length que pasamos
     *  a string_from_cstr cuadre exactamente con lo escapado. */
    private static String cEscapeBytes(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            int c = b & 0xff;
            switch (c) {
                case '"':  sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n");  break;
                case '\r': sb.append("\\r");  break;
                case '\t': sb.append("\\t");  break;
                default:
                    if (c < 0x20 || c > 0x7e) sb.append(String.format("\\x%02x", c));
                    else sb.append((char) c);
            }
        }
        return sb.toString();
    }

    /** Activa modo "para .mdn": code section 100% PIC, sin
     *  referencias a símbolos externos del runtime. */
    public void setOmitRegisterFunc(boolean omit) {
        this.omitRegisterFunc = omit;
    }

    /**
     * Emite la unidad de compilación completa para todas las funciones
     * native del módulo. Devuelve el C source listo para escribir a
     * un archivo aot_<Mod>.c.
     *
     * Lanza UnsupportedAotException si alguna función contiene
     * constructos no soportados — en ese caso el módulo entero no se
     * emite (el flag native se ignora y solo queda el bytecode normal).
     */
    public String emitModule(Ast.ModuleNode module) {
        // Pre-pass: recolectar nombres de funciones native para
        // que el emisor reconozca self-calls como C calls directas.
        List<Ast.FuncDef> nativeFuncs = new ArrayList<>();
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.FuncDef) {
                Ast.FuncDef f = (Ast.FuncDef) d;
                allFuncDefs.put(f.name.name, f);   /* #211 — TODAS, para call_bp */
                if (f.isNative && !f.isIntrinsic) {
                    nativeFuncs.add(f);
                    nativeFuncNames.add(f.name.name);
                    nativeFuncDefs.put(f.name.name, f);
                }
            }
        }
        if (nativeFuncs.isEmpty()) return "";

        /* V5/H4 - el pack del que salen las `native` sin cuerpo. */
        if (module.packNativoMarca != null) {
            String m = module.packNativoMarca;   /* el parser ya valido 4 chars */
            packMarca = ((m.charAt(0) & 0xFF) << 24) | ((m.charAt(1) & 0xFF) << 16)
                      | ((m.charAt(2) & 0xFF) <<  8) |  (m.charAt(3) & 0xFF);
            packVersion = module.packNativoVersion;
        }

        // #172 — Pre-pass: asignar offsets CS-relativos a vars/consts
        // nivel-módulo recorriendo en orden de declaración. MivmEmitter
        // sigue la misma convención (cf. ModWriter.registerSymbol):
        // primer decl encontrado → -4, segundo → -8, etc.
        precomputeModuleVarOffsets(module);

        // Header del archivo.
        emitHeader();

        // Una función + thunk por cada native.
        for (Ast.FuncDef f : nativeFuncs) {
            /* #349 — LA FUNCIÓN Y LA LÍNEA, o el aviso no sirve de nada. Los
             * throw de aquí abajo nacen en sitios que no saben en qué función
             * están (cType, readHelper...), así que el mensaje llegaba suelto:
             * "el tipo 'double' ocupa 8 bytes". En un módulo con varias native
             * eso obliga a buscarlas una a una. Se enriquece aquí, que es el
             * único punto por el que pasan todas. */
            try {
                /* V5/H4 - una `native` EXTERNA no tiene cuerpo que traducir: lo
                 * pone el pack. Lo que se emite en su lugar es el PUENTE. El
                 * thunk es el mismo de siempre: desde la pila BP, llamar a un
                 * pack o a codigo traducido se ve exactamente igual. */
                if (f.isPackExtern) emitPackExtern(f); else emitFunction(f);
                emitThunk(f);
            } catch (UnsupportedAotException ex) {
                throw new UnsupportedAotException(
                    "en la funcion native '" + f.name.name + "' (linea " + f.line + "): "
                    + ex.getMessage());
            }
        }

        // Función de registro — solo si NO estamos en modo .mdn.
        // En .mdn las relocs de bpvm_aot_register_by_name + string
        // literal contaminan el code section. El loader del firmware
        // registra automáticamente a partir del symtab.
        if (!omitRegisterFunc) {
            emitRegisterFunc(nativeFuncs);
        }
        return out.toString();
    }

    // ==================== Header ====================

    private void emitHeader() {
        w.println("/*");
        w.println(" * aot_" + moduleName + ".c — AUTOGENERADO por AotCEmitter (H3 #157).");
        w.println(" * NO EDITAR A MANO. Regenerar compilando " + moduleName + ".bp con --aot.");
        w.println(" *");
        w.println(" * Funciones BP marcadas con `function native ...` traducidas a C.");
        w.println(" * El bytecode .mod se sigue generando normalmente; el runtime");
        w.println(" * decide qué versión usar via aot_registry tras link.");
        w.println(" */");
        w.println();
        w.println("#include \"aot_registry.h\"");
        w.println("#include \"bpvm.h\"");
        w.println("#include \"bpvm_internal.h\"");
        w.println("#include \"bpvm_aot_helpers.h\"   /* H3 #158 — helpers indirect */");
        w.println();
        w.println("/* Forward decls de las funciones AOT de este módulo. */");
        for (Ast.FuncDef f : nativeFuncDefs.values()) {
            /* #349 — mismo motivo que en el bucle de emisión: un tipo de la
             * FIRMA revienta aquí, antes de llegar allí, así que sin este catch
             * el caso más común (un `double` como parámetro o retorno) era
             * justo el que salía sin nombre de función. */
            try {
                w.print("static " + cType(f.returnType) + " aot_"
                    + moduleName + "_" + f.name.name + "(struct bpvm* vm");
                for (Ast.Param p : f.params) {
                    w.print(", " + cType(p.type) + " " + p.name);
                }
                w.println(");");
            } catch (UnsupportedAotException ex) {
                throw new UnsupportedAotException(
                    "en la firma de la funcion native '" + f.name.name
                    + "' (linea " + f.line + "): " + ex.getMessage());
            }
        }
        w.println();

        /* #172 — Si el módulo declara vars/consts a nivel-módulo, emitimos
         * el cache de CS por-módulo. CS es runtime (depende de en qué
         * orden cargue el loader los módulos), así que usamos lazy init.
         * Las accesos a globales se compilan como:
         *     read_i32_be(mem + cs + OFFSET_LITERAL)
         * donde `cs` = aot_<Mod>_cs(vm) cacheado en la static. */
        if (!moduleVarOffsets.isEmpty()) {
            w.println("/* #172 — cache de CS del módulo. Lazy init en la primera");
            w.println(" * invocación de cualquier thunk AOT de este .c. No hace");
            w.println(" * falta sincronización: en runtime single-thread la");
            w.println(" * primera escritura es idempotente; en multi-thread");
            w.println(" * cualquier carrera escribiría el mismo valor (data race");
            w.println(" * benigno tipo \"singly assigned\"). */");
            w.println("static uint32_t s_aot_" + moduleName + "_cs = 0;");
            w.println();
            w.println("static inline uint32_t aot_" + moduleName + "_cs(struct bpvm* vm) {");
            w.println("    uint32_t cs = s_aot_" + moduleName + "_cs;");
            w.println("    if (!cs) {");
            w.println("        cs = vm->aot_helpers->find_module_cs(vm, \""
                    + moduleName + "\");");
            w.println("        if (!cs) {");
            w.println("            vm->aot_helpers->throw_runtime(vm,");
            w.println("                \"AOT: módulo '" + moduleName + "' no encontrado\");");
            w.println("            return 0;");
            w.println("        }");
            w.println("        s_aot_" + moduleName + "_cs = cs;");
            w.println("    }");
            w.println("    return cs;");
            w.println("}");
            w.println();
            w.println("/* Offsets CS-relativos de los module-globals (compile-time): */");
            for (java.util.Map.Entry<String, Integer> e : moduleVarOffsets.entrySet()) {
                w.println("/*   " + moduleName + "." + e.getKey() + " @ CS"
                        + (e.getValue() < 0 ? "" : "+") + e.getValue() + " */");
            }
            w.println();
        }
    }

    // ==================== Función AOT ====================

    private void emitFunction(Ast.FuncDef f) {
        currentFuncName = f.name.name;   /* #211 — para mensajes de aviso */
        String fname = "aot_" + moduleName + "_" + f.name.name;
        String cRet  = cType(f.returnType);
        w.print("static " + cRet + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) {
            w.print(", " + cType(p.type) + " " + p.name);
        }
        w.println(") {");
        w.println("    (void) vm;   /* puede no usarse si la función no toca");
        w.println("                  *  globals/arrays/builtins. */");

        /* #172 — Si el módulo tiene globals declarados, exponemos `mem`
         * y `cs` al cuerpo de la función. `cs` se resuelve UNA VEZ por
         * thunk-invocation (no por acceso) vía la helper-static
         * aot_<Mod>_cs(vm). Para funciones que no tocan globals el
         * compilador C las elimina como dead-store con -O2. */
        if (!moduleVarOffsets.isEmpty()) {
            w.println("    uint8_t* mem = vm->memory;");
            w.println("    uint32_t cs = aot_" + moduleName + "_cs(vm);");
            w.println("    (void) mem; (void) cs;");
        }

        /* #171 — Silenciar warnings de params no usados (frecuente en
         * funciones de pass-through o cuando el AOT no usa todos los
         * args). El compilador C los elimina en -O2 igualmente. */
        for (Ast.Param p : f.params) {
            w.println("    (void) " + p.name + ";");
        }

        /* #172 — Inicializar ámbito local: params + reset entre funcs. */
        localNames.clear();
        for (Ast.Param p : f.params) localNames.add(p.name);

        indentLevel = 1;
        for (Ast.IStmt s : f.body) {
            emitStmt(s);
        }
        // Fall-through default: para retornos integer 0, float 0.0f.
        // Si returnType es null (void), no se hace.
        if (f.returnType != null && !endsWithReturn(f.body)) {
            indent();
            String z = "float".equals(cRet) ? "0.0f" : "0";
            w.println("return " + z + ";   /* fall-through default */");
        }
        w.println("}");
        w.println();
    }

    // ============ V5/H4: puente a un pack nativo ============

    /**
     * Emite el cuerpo de una `native` EXTERNA: la declarada sin cuerpo en un
     * modulo con `import v<N> from pack "MARCA"`. Hace dos cosas y ya - resolver
     * el simbolo por nombre y adaptar los tipos.
     *
     * NI UN LITERAL DE CADENA AQUI DENTRO. Medido con el toolchain real: un
     * literal de C se va a `.rodata.str1.1` y en `.text` queda un `R_ARM_REL32`;
     * MdnPack empaqueta `.text` y NADA MAS, sin aplicar relocs. En placa ese
     * puntero no apuntaria a ningun sitio. Por eso el nombre del simbolo se
     * materializa BYTE A BYTE en la pila (medido: cero relocs de -O0 a -Os), y
     * el texto de los errores lo redacta la VM (helper `pack_fallo`).
     *
     * Y ojo con la FORMA: `char n[] = {ESE_ARRAY}` no vale - gcc reconoce el
     * inicializador y lo vuelve a sacar a `.rodata`. Asignacion a asignacion si.
     */
    private void emitPackExtern(Ast.FuncDef f) {
        currentFuncName = f.name.name;
        String sim   = f.name.name;
        String fname = "aot_" + moduleName + "_" + sim;
        String marca = String.format("0x%08Xu", packMarca);
        String ver   = packVersion + "u";
        int n = f.params.size();

        w.println("/* " + sim + " - EXTERNA: la trae el pack " + Q + marcaTexto() + Q
                  + " v" + packVersion + ".");
        w.println(" * No hay cuerpo en el .bp: lo pone el pack. Esto es el puente. */");
        w.print("static " + cType(f.returnType) + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) w.print(", " + cType(p.type) + " " + p.name);
        w.println(") {");
        w.println("    const struct aot_helpers_v2* H = vm->aot_helpers;");

        /* La firma REAL, la que el pack expone. */
        StringBuilder sig = new StringBuilder();
        for (int i = 0; i < n; i++) {
            if (i > 0) sig.append(", ");
            sig.append(cTypePack(f.params.get(i).type, sim));
        }
        if (n == 0) sig.append("void");
        w.println("    typedef " + cTypePack(f.returnType, sim) + " (*fn_t)(" + sig + ");");
        w.println("    fn_t fn;");

        /* Todas las DECLARACIONES juntas y antes de cualquier sentencia: el .c
         * generado se compila con toolchains distintos y no todos son C99. */
        w.println("    char nm[" + (sim.length() + 1) + "];");
        for (int i = 0; i < n; i++) {
            Ast.TypeRef t = f.params.get(i).type;
            if (esString(t))             w.println("    char b" + i + "[BPVM_AOT_PACK_STR];");
            else if (salida8(t) != null) w.println("    " + salida8(t) + " o" + i + " = 0;");
        }
        /* El nombre, byte a byte - ver la nota de arriba. */
        StringBuilder ln = new StringBuilder("   ");
        for (int i = 0; i < sim.length(); i++) {
            ln.append(" nm[").append(i).append("]=").append(Q).append(sim.charAt(i)).append(Q).append(";");
            if (ln.length() > 66) { w.println(ln); ln = new StringBuilder("   "); }
        }
        ln.append(" nm[").append(sim.length()).append("]=0;");
        w.println(ln);

        w.println("    fn = (fn_t) H->pack_sym(" + marca + ", " + ver + ", nm);");
        w.println("    if (fn == 0) H->pack_fallo(vm, " + marca + ", " + ver
                  + ", nm, 1);   /* no retorna */");
        for (int i = 0; i < n; i++) {
            if (!esString(f.params.get(i).type)) continue;
            String pn = f.params.get(i).name;
            w.println("    if (H->string_to_cstr(vm, (uint32_t) " + pn + ", b" + i
                      + ", (int32_t) sizeof b" + i + ") < 0)");
            w.println("        H->pack_fallo(vm, " + marca + ", " + ver
                      + ", nm, 2);   /* no retorna */");
        }

        StringBuilder args = new StringBuilder();
        for (int i = 0; i < n; i++) {
            Ast.TypeRef t = f.params.get(i).type;
            if (i > 0) args.append(", ");
            if (esString(t))             args.append("b").append(i);
            else if (salida8(t) != null) args.append("&o").append(i);
            else                         args.append(f.params.get(i).name);
        }
        boolean devuelveCadena = esString(f.returnType);
        boolean hayCajas = false;
        for (int i = 0; i < n; i++) if (salida8(f.params.get(i).type) != null) hayCajas = true;

        if (f.returnType == null) {
            w.println("    fn(" + args + ");");
        } else if (devuelveCadena) {
            /* El pack devuelve `const char*`; BP quiere una cadena del heap. La
             * longitud se cuenta a mano: `strlen` seria un simbolo externo, y en
             * el .mdn eso es otra reloc que no resuelve nadie. */
            w.println("    { const char* r = fn(" + args + "); int32_t k = 0;");
            w.println("      if (r == 0) return 0;");
            /* ACOTADO a proposito, y por dos motivos que coinciden:
             *  1. gcc reconoce `while (r[k]) k++;` y lo sustituye por una
             *     llamada a strlen -> simbolo externo -> reloc -> el .mdn no la
             *     resuelve. MEDIDO: era el UNICO reloc que quedaba.
             *  2. si el pack devuelve un buffer sin terminar, un bucle sin tope
             *     se sale de la memoria. Con tope, se corta. */
            w.println("      while (k < BPVM_AOT_PACK_STR && r[k]) k++;");
            w.println("      return (int32_t) H->string_from_cstr(vm, r, k); }");
        } else {
            w.println("    { " + cTypePack(f.returnType, sim) + " r = fn(" + args + ");");
        }

        /* Las cajas se vuelcan al elemento 0 de su array. CONVENIO: un
         * `long[]`/`double[]` en una externa es una caja de UN valor - la vuelta
         * que hay para 8 bytes mientras el AOT no los marshalle (#381).
         * `array_store_*` comprueba el indice, asi que un array vacio lo DICE. */
        for (int i = 0; i < n; i++) {
            String t8 = salida8(f.params.get(i).type);
            if (t8 == null) continue;
            String hp = t8.equals("int64_t") ? "array_store_i64" : "array_store_f64";
            w.println("    H->" + hp + "(vm, (uint32_t) " + f.params.get(i).name
                      + ", 0, o" + i + ");   /* caja de salida */");
        }
        if (f.returnType != null && !devuelveCadena) w.println("    return r; }");
        else if (hayCajas && f.returnType == null)   { /* nada: no se abrio bloque */ }
        w.println("}");
        w.println();
    }

    /** Los 4 caracteres de la marca, para los comentarios del .c generado. */
    private String marcaTexto() {
        return "" + (char)((packMarca >> 24) & 0xFF) + (char)((packMarca >> 16) & 0xFF)
                  + (char)((packMarca >>  8) & 0xFF) + (char)( packMarca        & 0xFF);
    }

    private boolean esString(Ast.TypeRef t) {
        return (t instanceof Ast.SimpleTypeRef)
            && "string".equals(((Ast.SimpleTypeRef) t).name);
    }

    /** `long[]` / `double[]` como CAJA DE SALIDA - el pack recibe un puntero. */
    private String salida8(Ast.TypeRef t) {
        if (!(t instanceof Ast.ArrayTypeRef)) return null;
        Ast.TypeRef e = ((Ast.ArrayTypeRef) t).element;
        if (!(e instanceof Ast.SimpleTypeRef)) return null;
        String n = ((Ast.SimpleTypeRef) e).name;
        if ("long".equals(n))   return "int64_t";
        if ("double".equals(n)) return "double";
        return null;
    }

    /**
     * El tipo tal como lo ve EL PACK, que no es el que ve BP: una cadena BP es un
     * handle al heap y el pack quiere `const char*`; un `long[]` de salida es un
     * handle y el pack quiere `int64_t*`.
     *
     * Lo que NO cruza se rechaza AQUI y CON NOMBRE. Un objeto BP no puede viajar
     * a un pack: el pack no conoce el GC ni debe, y darle un puntero al heap es
     * exactamente el use-after-free que costo media V4.
     */
    private String cTypePack(Ast.TypeRef t, String sim) {
        if (t == null) return "void";
        if (esString(t)) return "const char*";
        String s8 = salida8(t);
        if (s8 != null) return s8 + "*";
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            if ("integer".equals(n) || "int".equals(n))  return "int32_t";
            if ("boolean".equals(n) || "bool".equals(n)) return "int32_t";
            if ("float".equals(n))                       return "float";
            if ("long".equals(n) || "double".equals(n)) {
                throw new UnsupportedAotException(
                    Q + sim + Q + " usa " + Q + n + Q + " (8 bytes) y el puente a un "
                    + "pack solo pasa valores de 4. Para traerse un valor de 8 bytes "
                    + "se usa un ARRAY DE SALIDA: declara el parametro como " + Q + n
                    + "[]" + Q + " y el pack escribe en su elemento 0. (El rodeo se "
                    + "quitara cuando el AOT marshalle 8 bytes - tarea #381.)");
            }
            throw new UnsupportedAotException(
                Q + sim + Q + " usa el tipo " + Q + n + Q + ", que no puede cruzar a "
                + "un pack. Un pack no conoce el GC de BP, asi que solo cruzan "
                + "VALORES: integer, boolean, float, string, y long[]/double[] como "
                + "caja de salida. Un objeto se reparte como HANDLE (un integer que "
                + "el pack valida).");
        }
        throw new UnsupportedAotException(
            Q + sim + Q + ": ese tipo de array no cruza a un pack. Solo long[] y "
            + "double[], y solo como caja de salida de un valor de 8 bytes.");
    }

    private boolean endsWithReturn(List<Ast.IStmt> body) {
        if (body.isEmpty()) return false;
        Ast.IStmt last = body.get(body.size() - 1);
        return last instanceof Ast.ReturnStmt;
    }

    // ==================== Thunk ====================

    private void emitThunk(Ast.FuncDef f) {
        String fname = "aot_" + moduleName + "_" + f.name.name;
        String tname = "thunk_" + moduleName + "_" + f.name.name;
        // En modo .mdn el thunk debe ser visible al linker — MdnPack lo
        // busca por nombre en el symtab del .o. Además marcamos `used`
        // para que -Os con -ffunction-sections no lo deade-code-stripe
        // por no tener llamadores en este TU. En modo "linked-in" se
        // mantiene como antes: static, registrado via aot_<Mod>_register.
        if (omitRegisterFunc) {
            w.println("__attribute__((used))");
            w.println("void " + tname + "(struct bpvm* vm,");
        } else {
            w.println("static void " + tname + "(struct bpvm* vm,");
        }
        w.println("                              uint32_t* sp_p,");
        w.println("                              uint32_t* bp_p) {");
        w.println("    (void) bp_p;");
        w.println("    /* H3 #158 — helpers accedidos indirect via vm.");
        w.println("     * No referencia símbolos del runtime por nombre → el");
        w.println("     * .o resultante con -fpic es 100% relocatable. */");
        w.println("    const struct aot_helpers_v2* H = vm->aot_helpers;");
        w.println("    uint8_t* mem = vm->memory;");
        w.println("    uint32_t sp = *sp_p;");
        // Pop args en orden inverso (último pusheado, primero popeado).
        // #302 paso 2 — un arg REFERENCIA ocupa 8 bytes en la pila BP (handle):
        // se lee con read_ref (devuelve el handle empaquetado, que el cuerpo AOT
        // maneja como i32) y avanza sp -= 8. Los no-ref siguen a 4 bytes.
        int n = f.params.size();
        for (int i = n - 1; i >= 0; i--) {
            Ast.Param p = f.params.get(i);
            if (paramIsRef(f, i)) {
                w.println("    " + cType(p.type) + " a" + i
                    + " = (int32_t) H->read_ref(mem + sp - 8); sp -= 8;  /* ref: 8B */");
            } else {
                w.println("    " + cType(p.type) + " a" + i + " = H->"
                    + readHelper(p.type) + "(mem + sp - 4); sp -= 4;");
            }
        }
        // C call con args en orden original a0, a1, a2...
        // Si la función es void, no asignamos a una variable y no
        // escribimos resultado al stack — los args ya están consumidos.
        boolean isVoid = (f.returnType == null);
        String cRet = cType(f.returnType);
        StringBuilder call = new StringBuilder();
        call.append("    ");
        if (!isVoid) call.append(cRet).append(" r = ");
        call.append(fname).append("(vm");
        for (int i = 0; i < n; i++) call.append(", a").append(i);
        call.append(");");
        w.println(call);
        if (!isVoid && returnIsRef(f)) {
            /* #302 paso 2 — retorno REFERENCIA: escribir el handle de 8 bytes con
             * la gen VIVA (write_ref regen internamente) y avanzar sp += 8. */
            w.println("    H->write_ref(vm, mem + sp, (uint32_t) r); sp += 8;  /* ref: 8B */");
        } else if (!isVoid) {
            w.println("    H->" + writeHelper(f.returnType)
                + "(mem + sp, r); sp += 4;");
        } else {
            /* #177 FIX — Las funciones BP normal con OP_RET siempre push
             * un ret_val (incluso si son void). El compilador BP emite
             * OP_POP tras un statement-call para discardarlo. Para que el
             * thunk AOT void sea balance-equivalente al call BP, debe
             * push un dummy 0 que el OP_POP siguiente popee. Si no, sp
             * decrece en 4 bytes por cada call AOT void → frame corrupt
             * tras varias calls. */
            w.println("    H->write_i32_be(mem + sp, 0); sp += 4;  /* dummy ret para OP_POP del caller */");
        }
        w.println("    *sp_p = sp;");
        w.println("}");
        w.println();
    }

    // ==================== Register ====================

    private void emitRegisterFunc(List<Ast.FuncDef> nativeFuncs) {
        w.println("/* Registra todas las funciones AOT de este módulo en el AOT");
        w.println(" * registry. Llamar tras link, antes de bpvm_run. Tolerante a");
        w.println(" * símbolos ausentes (skip silente si el .mod no está cargado). */");
        w.println("void aot_" + moduleName + "_register(struct bpvm* vm) {");
        for (Ast.FuncDef f : nativeFuncs) {
            String qualified = moduleName + "." + f.name.name;
            String tname = "thunk_" + moduleName + "_" + f.name.name;
            w.println("    bpvm_aot_register_by_name(vm, \"" + qualified + "\", " + tname + ");");
        }
        w.println("}");
        w.println();
    }

    // ==================== Statements ====================

    private void emitStmt(Ast.IStmt s) {
        if (s instanceof Ast.ReturnStmt) {
            Ast.ReturnStmt r = (Ast.ReturnStmt) s;
            indent();
            if (r.value != null) {
                w.print("return ");
                emitExpr(r.value);
                w.println(";");
            } else {
                w.println("return;");
            }
            return;
        }
        if (s instanceof Ast.IfStmt) {
            emitIfStmt((Ast.IfStmt) s);
            return;
        }
        if (s instanceof Ast.VarDecl) {
            Ast.VarDecl v = (Ast.VarDecl) s;
            String cVarType = cType(v.type);
            for (Ast.DeclName dn : v.names) {
                indent();
                w.print(cVarType + " " + dn.name);
                if (v.init != null) {
                    w.print(" = ");
                    emitExpr(v.init);
                }
                w.println(";");
                localNames.add(dn.name);   /* #172 — registrar local */
            }
            return;
        }
        if (s instanceof Ast.AssignStmt) {
            Ast.AssignStmt a = (Ast.AssignStmt) s;
            /* Target IdentifierExpr: assignment a variable local/param,
             * o bien a global de módulo (#172) si no está en localNames. */
            if (a.target instanceof Ast.IdentifierExpr) {
                String tname = ((Ast.IdentifierExpr) a.target).name;
                indent();
                if (localNames.contains(tname)) {
                    /* Local — emisión directa estilo C. */
                    w.print(tname);
                    switch (a.op) {
                        case ASSIGN:       w.print(" = ");  break;
                        case PLUS_ASSIGN:  w.print(" += "); break;
                        case MINUS_ASSIGN: w.print(" -= "); break;
                    }
                    emitExpr(a.value);
                    w.println(";");
                } else {
                    /* #172 — Global de módulo: acceso directo mem+cs+OFFSET.
                     * - ASSIGN:        write(mem+cs+OFF, value).
                     * - PLUS/MINUS:    write(mem+cs+OFF, read(mem+cs+OFF) [+|-] value). */
                    Integer off = moduleVarOffsets.get(tname);
                    if (off == null) {
                        throw new UnsupportedAotException(
                            "AOT: assign a '" + tname + "' — no es local"
                            + " ni módulo-global conocido (cross-module o"
                            + " tipo no soportado). line " + a.line);
                    }
                    usesModuleGlobals = true;
                    String addr = "mem + (uint32_t)((int32_t)cs + (" + off + "))";
                    if (a.op == Ast.AssignOpKind.ASSIGN) {
                        w.print("vm->aot_helpers->write_i32_be(" + addr + ", ");
                        emitExpr(a.value);
                        w.println(");");
                    } else {
                        String binOp = (a.op == Ast.AssignOpKind.PLUS_ASSIGN) ? "+" : "-";
                        w.print("vm->aot_helpers->write_i32_be(" + addr
                              + ", vm->aot_helpers->read_i32_be(" + addr
                              + ") " + binOp + " (");
                        emitExpr(a.value);
                        w.println("));");
                    }
                }
                return;
            }
            /* Target IndexExpr (H3 #170): a[i] := v / += / -= via helper.
             * Para += / -= cargamos primero, operamos, escribimos. */
            if (a.target instanceof Ast.IndexExpr) {
                Ast.IndexExpr ix = (Ast.IndexExpr) a.target;
                indent();
                if (a.op == Ast.AssignOpKind.ASSIGN) {
                    w.print("vm->aot_helpers->" + arrStoreFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", ");
                    emitExpr(a.value);
                    w.println(");");
                } else {
                    /* a[i] += v  →  store(a, i, load(a, i) + v) */
                    String binOp = (a.op == Ast.AssignOpKind.PLUS_ASSIGN) ? "+" : "-";
                    w.print("vm->aot_helpers->" + arrStoreFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", vm->aot_helpers->" + arrLoadFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(") " + binOp + " (");
                    emitExpr(a.value);
                    w.println("));");
                }
                return;
            }
            throw new UnsupportedAotException(
                "AOT: assign target no soportado: "
                + a.target.getClass().getSimpleName() + " (line " + a.line + ")");
        }
        if (s instanceof Ast.WhileStmt) {
            Ast.WhileStmt wl = (Ast.WhileStmt) s;
            indent();
            w.print("while (");
            emitExpr(wl.condition);
            w.println(") {");
            indentLevel++;
            for (Ast.IStmt st : wl.body) emitStmt(st);
            indentLevel--;
            indent();
            w.println("}");
            return;
        }
        if (s instanceof Ast.ForStmt) {
            emitForStmt((Ast.ForStmt) s);
            return;
        }
        if (s instanceof Ast.BreakStmt) {
            indent(); w.println("break;");
            return;
        }
        if (s instanceof Ast.ContinueStmt) {
            indent(); w.println("continue;");
            return;
        }
        if (s instanceof Ast.ExprStmt) {
            /* Statement-form de una expresión (call con side-effects,
             * típicamente). Emitimos la expr seguida de ';'. */
            Ast.ExprStmt es = (Ast.ExprStmt) s;
            indent();
            emitExpr(es.expr);
            w.println(";");
            return;
        }
        if (s instanceof Ast.SwitchStmt) {
            /* BP switch → C switch. BP no tiene fall-through implícito
             * (cada case acaba al final del bloque), así que añadimos
             * `break;` automático tras cada case. */
            Ast.SwitchStmt sw = (Ast.SwitchStmt) s;
            indent();
            w.print("switch (");
            emitExpr(sw.subject);
            w.println(") {");
            indentLevel++;
            for (Ast.CaseClause cc : sw.cases) {
                for (Ast.IExpr v : cc.values) {
                    indent();
                    w.print("case ");
                    emitExpr(v);
                    w.println(":");
                }
                indentLevel++;
                for (Ast.IStmt st : cc.body) emitStmt(st);
                indent(); w.println("break;");
                indentLevel--;
            }
            if (sw.defaultBody != null) {
                indent(); w.println("default:");
                indentLevel++;
                for (Ast.IStmt st : sw.defaultBody) emitStmt(st);
                indent(); w.println("break;");
                indentLevel--;
            }
            indentLevel--;
            indent(); w.println("}");
            return;
        }
        if (s instanceof Ast.ThrowStmt) {
            emitThrowStmt((Ast.ThrowStmt) s);
            return;
        }
        if (s instanceof Ast.TryStmt) {
            /* #213 parte 2 — DIFERIDO con motivo: el código .mdn no puede
             * llamar setjmp (cero relocations externas — todo acceso al
             * runtime va via vm->aot_helpers, y un jmp_buf local del native
             * no sobrevive al longjmp del helper sin un refactor de locals
             * a context-struct + helper eh_native_try). El throw HACIA fuera
             * sí funciona (#186/#175/#213): envuelve la LLAMADA al native
             * en try/catch BP. */
            throw new UnsupportedAotException(
                "AOT: try/catch DENTRO de una función native no está soportado "
                + "(#213: .mdn no puede usar setjmp). Workaround: envuelve la "
                + "LLAMADA al native en try/catch BP — los throw del native "
                + "(RuntimeError o clases de usuario) sí se cazan ahí. line "
                + ((Ast.Node) s).line);
        }
        throw new UnsupportedAotException(
            "AOT: statement no soportado: " + s.getClass().getSimpleName()
            + " (line " + ((Ast.Node) s).line + ")");
    }

    /** #186 — `throw RuntimeError("literal")` desde native.
     *
     *  El helper throw_runtime construye el RuntimeError EN LA VM
     *  (bpvm_throw_runtime_error: localiza la clase, aloca string+objeto)
     *  y hace longjmp al boundary AOT del intérprete, que lo propaga por
     *  el eh_stack hasta el try/catch BP que envuelva la llamada (o
     *  termina el thread). El native NO construye el objeto.
     *
     *  #213 — clases de usuario: `throw MyExc(args)` tampoco construye el
     *  objeto en native. Cruza el puente call_bp_i32 a la factory
     *  cross-module `Owner.__cls_new_MyExc` (la misma que usa un módulo
     *  importador para `new` — corre el ctor REAL en el intérprete) y el
     *  ref resultante viaja por throw_ref → boundary → eh_unwind, igual
     *  que un throw BP normal. Requiere clase public (la factory solo se
     *  sintetiza para public) y ctor con firma i32 (límite v1 del puente).
     *  Lo que NO soporta esta fase: try/catch DENTRO de native (.mdn no
     *  puede llamar setjmp — cero relocations externas; necesitaría
     *  helpers eh_native_try + locals en context struct, diferido). En los
     *  casos no soportados lanzamos UnsupportedAotException y el módulo
     *  cae a bytecode interpretado (donde sí funciona). */
    private void emitThrowStmt(Ast.ThrowStmt ts) {
        /* Fast path: throw RuntimeError("literal") → throw_runtime con el
         * cstring directo (sin alocar string en el heap). */
        String lit = runtimeErrorLiteral(ts.value);
        if (lit != null) {
            byte[] bytes = lit.getBytes(java.nio.charset.StandardCharsets.UTF_8);
            indent();
            w.println("vm->aot_helpers->throw_runtime(vm, \"" + cEscapeBytes(bytes) + "\");");
            return;
        }
        /* #175 — throw RuntimeError(<expr string>) con mensaje COMPUTADO
         * (p.ej. "x = " + n). Emitimos el string-handle BP y throw_str lo lee.
         * AotCEmitter ya sabe emitir concats/ops de string (#173). */
        Ast.IExpr arg = runtimeErrorArg(ts.value);
        if (arg != null && isStringExpr(arg)) {
            indent();
            w.print("vm->aot_helpers->throw_str(vm, ");
            emitExpr(arg);
            w.println(");");
            return;
        }
        /* #213 — throw de CLASE DE USUARIO. La instancia se construye en el
         * intérprete (factory cross-module via puente); aquí solo viaja el ref. */
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(ts.value) : null;
        if (t instanceof BpType.ClassType) {
            Symbol.ClassSymbol cls = ((BpType.ClassType) t).cls;
            if (ts.value instanceof Ast.CallExpr
                    && cls.name.equals(calleeSimpleName(((Ast.CallExpr) ts.value).callee))) {
                emitThrowUserClass((Ast.CallExpr) ts.value, cls, ts.line);
                return;
            }
            /* Ref ya construida (param de tipo clase, llamada que devuelve la
             * excepción, ...) → throw_ref directo del handle i32. Si emitExpr
             * no sabe emitir la expresión, su UnsupportedAotException hace el
             * fallback a interpretado, como siempre. */
            indent();
            w.print("vm->aot_helpers->throw_ref(vm, (uint32_t) (");
            emitExpr(ts.value);
            w.println("));");
            return;
        }
        throw new UnsupportedAotException(
            "AOT: en native `throw` soporta RuntimeError(string) y clases de "
            + "usuario public que desciendan de Exception "
            + "(try/catch DENTRO de native = pendiente #175b). line "
            + ts.line);
    }

    /** #213 — `throw MyExc(args)` desde native. La construcción NO se hace en
     *  C: cruzamos al intérprete via call_bp_i32 → factory cross-module
     *  `Owner.__cls_new_MyExc(args)` (corre el ctor real), y throw_ref deja el
     *  ref en el fault slot + longjmp al boundary, que lo propaga por
     *  eh_unwind. GC-safe: el thunk corre dentro del dispatch del intérprete
     *  (sin safepoint intermedio), así que el ref no puede ser recolectado
     *  entre la factory y el unwind. Validamos contra la firma REAL de la
     *  factory = ctor PROPIO de la clase (igual que synthesizeCrossModuleFactory). */
    private void emitThrowUserClass(Ast.CallExpr c, Symbol.ClassSymbol cls, int line) {
        if (!cls.isPublic) {
            throw new UnsupportedAotException(
                "AOT: `throw " + cls.name + "(...)` en native (line " + line + ") necesita "
                + "que la clase sea public: la instancia se construye via la factory "
                + "cross-module __cls_new_" + cls.name + ", que solo se sintetiza para "
                + "clases public. Declárala `public class` o quita native.");
        }
        Symbol.FunctionSymbol ctor = cls.constructor;
        int expected = (ctor != null) ? ctor.params.size() : 0;
        if (c.args.size() != expected) {
            throw new UnsupportedAotException(
                "AOT: `throw " + cls.name + "(...)` (line " + line + "): " + c.args.size()
                + " args pero la factory __cls_new_" + cls.name + " espera " + expected
                + " (firma del ctor propio de la clase).");
        }
        if (ctor != null) {
            for (Symbol.ParamSymbol p : ctor.params) {
                if (!isBridgeI32Type(p.type)) {
                    throw new UnsupportedAotException(
                        "AOT: `throw " + cls.name + "(...)` (line " + line + "): el puente v1 "
                        + "solo soporta params i32 (integer/boolean/string/ref); el param '"
                        + p.name + "' del ctor no lo es. float/long/double → pendiente.");
                }
            }
        }
        StringBuilder qn = new StringBuilder();
        if (cls.isExternal) {
            if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty())
                qn.append(cls.externalLibrary).append('.');
            qn.append(cls.externalModule);
        } else {
            qn.append(moduleName);
        }
        qn.append(".__cls_new_").append(cls.name);

        indent();
        w.print("vm->aot_helpers->throw_ref(vm, (uint32_t) ");
        /* La factory __cls_new_* SIEMPRE devuelve la instancia (una ref) → ret_is_ref=1. */
        int ctorMask = (ctor != null) ? refMaskOfParams(ctor.params) : 0;
        emitCallBpEmission(qn.toString(), c.args, line,
            "la factory de '" + cls.name + "' (la instancia del throw se construye "
            + "en el intérprete)", ctorMask, true);
        w.println(");");
    }

    /** Devuelve el literal del mensaje si {@code value} es exactamente
     *  {@code RuntimeError("literal")}, o null si no matchea el subset v1. */
    private static String runtimeErrorLiteral(Ast.IExpr value) {
        Ast.IExpr arg = runtimeErrorArg(value);
        if (arg instanceof Ast.StringLitExpr) return ((Ast.StringLitExpr) arg).value;
        return null;
    }

    /** Devuelve el único argumento de {@code RuntimeError(arg)} si {@code value}
     *  es exactamente esa construcción, o null en otro caso. #175. */
    private static Ast.IExpr runtimeErrorArg(Ast.IExpr value) {
        if (!(value instanceof Ast.CallExpr)) return null;
        Ast.CallExpr call = (Ast.CallExpr) value;
        if (!"RuntimeError".equals(calleeSimpleName(call.callee))) return null;
        if (call.args == null || call.args.size() != 1) return null;
        return call.args.get(0);
    }

    /** Nombre simple del callee de una llamada: el identificador, o el
     *  miembro final de un acceso {@code X.RuntimeError}. */
    private static String calleeSimpleName(Ast.IExpr callee) {
        if (callee instanceof Ast.IdentifierExpr)   return ((Ast.IdentifierExpr) callee).name;
        if (callee instanceof Ast.MemberAccessExpr) return ((Ast.MemberAccessExpr) callee).member;
        return null;
    }

    /** for-loop BP: `for i = <from> to <to> [step <step>]` con semántica
     *  inclusiva. Cuando step es literal positivo emitimos un C `for`
     *  optimizable; en otro caso una variante `while` con check dinámico
     *  de dirección. */
    private void emitForStmt(Ast.ForStmt fs) {
        if (!(fs.range instanceof Ast.ForNumericRange)) {
            throw new UnsupportedAotException(
                "AOT: for-range no numérico no soportado (line " + fs.line + ")");
        }
        Ast.ForNumericRange r = (Ast.ForNumericRange) fs.range;
        int tag = ++forCounter;
        String it   = fs.iteratorName;
        String end  = "__aot_end_"  + tag;
        String step = "__aot_step_" + tag;

        // Wrap en bloque para limitar el scope del iterator + end + step.
        indent();
        w.println("{");
        indentLevel++;

        indent(); w.print("int32_t " + it + " = ");
                  emitExpr(r.from); w.println(";");
        indent(); w.print("int32_t " + end + " = ");
                  emitExpr(r.to);   w.println(";");
        if (r.step != null) {
            indent(); w.print("int32_t " + step + " = ");
                      emitExpr(r.step); w.println(";");
        } else {
            indent(); w.println("int32_t " + step + " = 1;");
        }
        /* #172 — el iterador es local del bucle; registrarlo para que
         * emitExpr(IdentifierExpr) lo trate como tal. Lo dejamos en
         * localNames durante el ciclo del for; al cerrar el for queda
         * el scope cerrado, pero como no removemos sería ambiguo si la
         * misma variable se reutilizara en otro for hermano dentro de
         * la misma función. En la práctica el frontend BP genera
         * scoping por bloque, así que el riesgo es bajo; documentamos
         * por si surge ruido futuro. */
        localNames.add(it);

        // while ((step > 0) ? (it <= end) : (it >= end)) { body; it += step; }
        indent();
        w.println("while ((" + step + " > 0) ? (" + it + " <= " + end + ") : ("
                  + it + " >= " + end + ")) {");
        indentLevel++;
        for (Ast.IStmt st : fs.body) emitStmt(st);
        indent(); w.println(it + " += " + step + ";");
        indentLevel--;
        indent(); w.println("}");

        indentLevel--;
        indent();
        w.println("}");
    }

    private void emitIfStmt(Ast.IfStmt iff) {
        indent();
        w.print("if (");
        emitExpr(iff.then_.condition);
        w.println(") {");
        indentLevel++;
        for (Ast.IStmt s : iff.then_.body) emitStmt(s);
        indentLevel--;
        for (Ast.IfClause eif : iff.elseIfs) {
            indent();
            w.print("} else if (");
            emitExpr(eif.condition);
            w.println(") {");
            indentLevel++;
            for (Ast.IStmt s : eif.body) emitStmt(s);
            indentLevel--;
        }
        if (iff.else_ != null) {
            indent();
            w.println("} else {");
            indentLevel++;
            for (Ast.IStmt s : iff.else_) emitStmt(s);
            indentLevel--;
        }
        indent();
        w.println("}");
    }

    // ==================== Expressions ====================

    private void emitExpr(Ast.IExpr e) {
        if (e instanceof Ast.IntLitExpr) {
            w.print(((Ast.IntLitExpr) e).value);
            return;
        }
        if (e instanceof Ast.FloatLitExpr) {
            /* Emitimos con sufijo 'f' para que gcc no promocione a double.
             * Java Float.toString puede dar "1.0E10" — eso es válido como
             * literal C. */
            double v = ((Ast.FloatLitExpr) e).value;
            w.print(Float.toString((float) v) + "f");
            return;
        }
        if (e instanceof Ast.BoolLitExpr) {
            /* BP boolean → C int 0/1 (matchea cType(boolean) = int32_t). */
            w.print(((Ast.BoolLitExpr) e).value ? "1" : "0");
            return;
        }
        if (e instanceof Ast.StringLitExpr) {
            /* #173 — literal string: aloca un string heap en runtime con
             * los bytes UTF-8 del literal. Devuelve el handle (i32). */
            String v = ((Ast.StringLitExpr) e).value;
            byte[] bytes = v.getBytes(java.nio.charset.StandardCharsets.UTF_8);
            w.print("vm->aot_helpers->string_from_cstr(vm, \"");
            w.print(cEscapeBytes(bytes));
            w.print("\", " + bytes.length + ")");
            return;
        }
        if (e instanceof Ast.IdentifierExpr) {
            String name = ((Ast.IdentifierExpr) e).name;
            if (localNames.contains(name)) {
                w.print(name);                          // param o local
                return;
            }
            /* #172 — Variable nivel-módulo: acceso directo `mem + cs +
             * OFFSET` con OFFSET compile-time y `cs` cacheada al inicio
             * de la función. */
            Integer off = moduleVarOffsets.get(name);
            if (off == null) {
                throw new UnsupportedAotException(
                    "AOT: identificador '" + name + "' no resuelve a local"
                    + " ni a módulo-global conocido (cross-module o tipo no"
                    + " soportado — pendiente #169/#171). line "
                    + ((Ast.Node) e).line);
            }
            usesModuleGlobals = true;
            w.print("vm->aot_helpers->read_i32_be(mem + (uint32_t)((int32_t)cs + ("
                    + off + ")))");
            return;
        }
        if (e instanceof Ast.ParenExpr) {
            /* Paréntesis explícitos en BP — los emitimos igual en C
             * por claridad y para preservar precedencia escrita. */
            w.print("(");
            emitExpr(((Ast.ParenExpr) e).inner);
            w.print(")");
            return;
        }
        if (e instanceof Ast.BinaryExpr) {
            Ast.BinaryExpr b = (Ast.BinaryExpr) e;
            boolean stringOp = isStringExpr(b.left) || isStringExpr(b.right);
            /* #173 — concat: `a + b` con algún operando string → helper.
             * Operando no-string se coacciona (integer → int_to_string;
             * otros tipos mixtos quedan sin soporte por ahora). */
            if (stringOp && "+".equals(b.op)) {
                w.print("vm->aot_helpers->string_concat(vm, ");
                emitStringOperand(b.left);
                w.print(", ");
                emitStringOperand(b.right);
                w.print(")");
                return;
            }
            /* #173 — igualdad de strings: compara CONTENIDO, no la ref.
             * `==` → string_eq; `!=` → !string_eq. */
            if (stringOp && ("==".equals(b.op) || "!=".equals(b.op))) {
                boolean neg = "!=".equals(b.op);
                if (neg) w.print("(!");
                w.print("vm->aot_helpers->string_eq(vm, ");
                emitExpr(b.left);
                w.print(", ");
                emitExpr(b.right);
                w.print(")");
                if (neg) w.print(")");
                return;
            }
            /* Numérico/lógico normal. */
            w.print("(");
            emitExpr(b.left);
            w.print(" " + cBinaryOp(b.op) + " ");
            emitExpr(b.right);
            w.print(")");
            return;
        }
        if (e instanceof Ast.UnaryExpr) {
            Ast.UnaryExpr u = (Ast.UnaryExpr) e;
            w.print("(");
            if ("-".equals(u.op))      w.print("-");
            else if ("not".equals(u.op)) w.print("!");
            else throw new UnsupportedAotException("AOT: unary '" + u.op + "' no soportado");
            emitExpr(u.operand);
            w.print(")");
            return;
        }
        if (e instanceof Ast.IndexExpr) {
            /* a[i] — lectura de array via helper. v1: solo integer[]. */
            Ast.IndexExpr ix = (Ast.IndexExpr) e;
            w.print("vm->aot_helpers->" + arrLoadFn(ix.target) + "(vm, ");
            emitExpr(ix.target);
            w.print(", ");
            emitExpr(ix.index);
            w.print(")");
            return;
        }
        if (e instanceof Ast.CallExpr) {
            Ast.CallExpr c = (Ast.CallExpr) e;

            if (c.callee instanceof Ast.MemberAccessExpr) {
                Ast.MemberAccessExpr ma = (Ast.MemberAccessExpr) c.callee;
                /* #169 — llamada cross-module `Mod.func(args)`: el símbolo
                 * resuelto del callee es una FunctionSymbol externa a nivel
                 * módulo. La emitimos por el puente call_bp con el nombre
                 * cualificado; el runtime resuelve la dirección y el CS del
                 * módulo destino. (Intrínsecos cross-module y métodos de
                 * instancia [dispatch virtual, #174] siguen pendientes.) */
                Symbol callSym = (semInfo != null) ? semInfo.exprSymbols.get(c.callee) : null;
                if (callSym instanceof Symbol.FunctionSymbol) {
                    Symbol.FunctionSymbol fs = (Symbol.FunctionSymbol) callSym;
                    if (fs.isExternal && !fs.isIntrinsic && fs.ownerClass == null) {
                        emitCrossModuleBridgeCall(fs, c);
                        return;
                    }
                    if (fs.isExternal && fs.isIntrinsic) {
                        throw new UnsupportedAotException(
                            "AOT: intrínseco cross-module '" + fs.externalQualifiedName()
                            + "' (line " + c.line + ") no soportado en native todavía.");
                    }
                    /* #174b — método PÚBLICO de instancia (virtual) sobre un
                     * receptor: despacho por vtable vía call_method_i32 con el
                     * slot que computa el frontend (ClassSymbol.slotOf, decisión
                     * B). Privado/super/estático van por el throw de abajo (no
                     * virtuales → sin asidero, pendientes). */
                    if (fs.ownerClass != null && !fs.isStatic && fs.isPublic
                            && !(ma.target instanceof Ast.SuperExpr)) {
                        emitVirtualMethodCall(fs, ma, c);
                        return;
                    }
                }
                throw new UnsupportedAotException(
                    "AOT: method call '" + ma.member + "' no soportado todavía (line "
                    + c.line + ") — solo método público (virtual); privado/super/"
                    + "estático/construcción pendientes.");
            }

            if (!(c.callee instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: call con callee no-identifier no soportado (line " + c.line + ")");
            }
            String name = ((Ast.IdentifierExpr) c.callee).name;

            /* Builtins (H3 #168) — antes que native funcs por si hubiera
             * colisión de nombres. Lista hardcoded por ahora; expansible. */
            if (emitBuiltinCall(name, c.args)) return;

            if (!nativeFuncNames.contains(name)) {
                /* #211 — ¿es una función BP del mismo módulo? Entonces NO
                 * abortamos: emitimos la llamada-puente native→BP
                 * (vm->aot_helpers->call_bp_i32), que ejecuta su cuerpo
                 * interpretado. Antes esto era un error duro. */
                Ast.FuncDef target = allFuncDefs.get(name);
                if (target != null) {
                    emitBridgeCall(name, target, c);
                    return;
                }
                throw new UnsupportedAotException(
                    "AOT: call a función desconocida '" + name + "' (line " + c.line + "). "
                    + "Debe ser native o BP del mismo módulo, o un builtin AOT-soportado "
                    + "(now, charAt, charCodeAt, substring, intToString). "
                    + "Cross-module sigue pendiente (#169).");
            }

            /* C call directo a otra función native del mismo módulo.
             * BL PC-relative dentro del mismo blob — gcc lo resuelve
             * sin relocations porque ambas son static en el mismo .o. */
            w.print("aot_" + moduleName + "_" + name + "(vm");
            for (Ast.IExpr arg : c.args) {
                w.print(", ");
                emitExpr(arg);
            }
            w.print(")");
            return;
        }
        throw new UnsupportedAotException(
            "AOT: expression no soportada: " + e.getClass().getSimpleName());
    }

    /** #211 — emite una llamada native→BP usando el puente call_bp_i32.
     *  La función BP destino corre en el INTÉRPRETE (pierde la velocidad
     *  AOT — de ahí el aviso). v1: solo firmas i32-compatibles
     *  (integer/boolean/string/array; el return también). float/long/
     *  double/void → error claro (marshalling distinto, pendiente). */
    private void emitBridgeCall(String name, Ast.FuncDef target, Ast.CallExpr c) {
        if (!isBridgeI32Type(target.returnType)) {
            throw new UnsupportedAotException(
                "AOT: call native→BP a '" + name + "' (line " + c.line + "): el puente v1 "
                + "solo soporta retorno i32 (integer/boolean/string/array); "
                + "float/long/double/void → pendiente.");
        }
        for (Ast.Param p : target.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: call native→BP a '" + name + "' (line " + c.line + "): el puente v1 "
                    + "solo soporta params i32; el param '" + p.name + "' no lo es. "
                    + "Tipos mixtos → pendiente.");
            }
        }
        if (c.args.size() != target.params.size()) {
            throw new UnsupportedAotException(
                "AOT: call native→BP a '" + name + "' (line " + c.line + "): "
                + c.args.size() + " args pero '" + name + "' espera "
                + target.params.size() + ".");
        }

        emitCallBpEmission(moduleName + "." + name, c.args, c.line,
            "la función BP interpretada '" + name + "'. Para máximo rendimiento, "
            + "declara '" + name + "' también como native",
            refMaskOfFuncDef(target), returnIsRef(target));
    }

    /** #169 — emite una llamada cross-module native→BP (`Mod.func(args)`) por
     *  el puente. La firma viene de la FunctionSymbol externa (.bpi del módulo
     *  dueño). El runtime resuelve la dirección y el CS del módulo destino. */
    private void emitCrossModuleBridgeCall(Symbol.FunctionSymbol fs, Ast.CallExpr c) {
        String qn = fs.externalQualifiedName();
        if (!isBridgeI32Type(fs.returnType)) {
            throw new UnsupportedAotException(
                "AOT: call native→BP cross-module a '" + qn + "' (line " + c.line + "): el puente "
                + "v1 solo soporta retorno i32; float/long/double/void → pendiente.");
        }
        for (Symbol.ParamSymbol p : fs.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: call native→BP cross-module a '" + qn + "' (line " + c.line + "): el puente "
                    + "v1 solo soporta params i32; tipos mixtos → pendiente.");
            }
        }
        if (c.args.size() != fs.params.size()) {
            throw new UnsupportedAotException(
                "AOT: call cross-module a '" + qn + "' (line " + c.line + "): "
                + c.args.size() + " args pero espera " + fs.params.size() + ".");
        }
        emitCallBpEmission(qn, c.args, c.line, "la función BP cross-module '" + qn + "'",
            refMaskOfParams(fs.params), isRefBp(fs.returnType));
    }

    /** #174b — emite una llamada a método PÚBLICO de instancia (virtual) desde
     *  native vía call_method_i32. El slot lo computa el frontend
     *  (ClassSymbol.slotOf) y MivmEmitter lo verifica contra ModWriter. La VM
     *  hace el paseo vtable (obj→class→vtable[slot]) según la clase REAL del
     *  receptor → polimorfismo correcto. v1: firmas i32 (params + retorno). */
    private void emitVirtualMethodCall(Symbol.FunctionSymbol fs, Ast.MemberAccessExpr ma, Ast.CallExpr c) {
        if (!isBridgeI32Type(fs.returnType)) {
            throw new UnsupportedAotException(
                "AOT: método native→BP '" + fs.name + "' (line " + c.line + "): el puente v1 "
                + "solo soporta retorno i32; float/long/double/void → pendiente.");
        }
        for (Symbol.ParamSymbol p : fs.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: método native→BP '" + fs.name + "' (line " + c.line + "): el puente v1 "
                    + "solo soporta params i32; tipos mixtos → pendiente.");
            }
        }
        int slot = (fs.ownerClass != null) ? fs.ownerClass.slotOf(fs.name) : -1;
        if (slot < 0) {
            throw new UnsupportedAotException(
                "AOT: no se pudo resolver el slot de vtable de '" + fs.name + "' (line "
                + c.line + ").");
        }
        if (c.args.size() != fs.params.size()) {
            throw new UnsupportedAotException(
                "AOT: método '" + fs.name + "' (line " + c.line + "): " + c.args.size()
                + " args pero espera " + fs.params.size() + ".");
        }
        warnings.add("la función native '" + currentFuncName + "' invoca el método público '"
            + fs.name + "' (línea " + c.line + "). Esa llamada cruza al intérprete por el "
            + "puente native→BP (dispatch virtual) y NO se acelera por AOT.");
        /* call_method_i32(vm, <this>, slot, (int32_t[]){args...}, n, ref_mask, ret_is_ref).
         * #302 paso 2 — ref_mask marca los args del USUARIO (el `this` lo añade la
         * propia call_method_i32 como bit 0); ret_is_ref, el retorno. */
        int refMask = refMaskOfParams(fs.params);
        boolean retIsRef = isRefBp(fs.returnType);
        String tail = ", " + refMask + "u, " + (retIsRef ? 1 : 0) + ")";
        w.print("vm->aot_helpers->call_method_i32(vm, ");
        emitExpr(ma.target);   // receptor (this)
        w.print(", " + slot + ", ");
        int n = c.args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0" + tail);
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(c.args.get(i));
        }
        w.print(" }, " + n + tail);
    }

    /** Emisión común del puente: aviso + call_bp_i32(vm, find_function(qn),
     *  (int32_t[]){args...}, n). El compound literal C99 vive en el bloque
     *  envolvente — válido como argumento. find_function resuelve el nombre
     *  cada vez (scan barato; cachear en static es mejora futura, pero el coste
     *  del puente domina). */
    private void emitCallBpEmission(String qualified, List<Ast.IExpr> args, int line,
                                    String targetDesc, int refMask, boolean retIsRef) {
        warnings.add("la función native '" + currentFuncName + "' llama a " + targetDesc
            + " (línea " + line + "). Esa llamada cruza al intérprete por el puente "
            + "native→BP y NO se acelera por AOT.");
        /* #302 paso 2 — ref_mask + ret_is_ref: los args-ref se ensanchan a 8 bytes
         * con regen en el puente y el retorno-ref popea 8 (ver bridge_run_bp_frame). */
        String tail = ", " + refMask + "u, " + (retIsRef ? 1 : 0) + ")";
        w.print("vm->aot_helpers->call_bp_i32(vm, vm->aot_helpers->find_function(vm, \""
            + qualified + "\"), ");
        int n = args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0" + tail);
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(args.get(i));
        }
        w.print(" }, " + n + tail);
    }

    /** #211 — ¿el tipo se representa como un i32 de 4 bytes que el puente
     *  call_bp_i32 puede marshallar tal cual? (integer/boolean/string/array
     *  → cType int32_t). float/long/double/void → no. */
    private boolean isBridgeI32Type(Ast.TypeRef t) {
        if (t == null) return false;   /* void */
        try { return "int32_t".equals(cType(t)); }
        catch (UnsupportedAotException e) { return false; }
    }

    /** #169 — variante BpType (firma de una FunctionSymbol cross-module).
     *  i32 = integer/boolean/string/narrow-int + refs (array/clase).
     *  float/long/double/void → no. */
    private boolean isBridgeI32Type(BpType t) {
        if (t == null) return false;   /* void */
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case FLOAT: case LONG: case DOUBLE: return false;
                default: return true;   /* integer/string/boolean/narrow → i32 */
            }
        }
        return true;   /* ArrayType / ClassType / ref → handle i32 de 4 bytes */
    }

    private String cBinaryOp(String bpOp) {
        switch (bpOp) {
            case "+":  return "+";
            case "-":  return "-";
            case "*":  return "*";
            case "/":  return "/";
            case "mod": case "%": return "%";
            case "==": return "==";
            case "!=": return "!=";
            case "<":  return "<";
            case ">":  return ">";
            case "<=": return "<=";
            case ">=": return ">=";
            case "and": return "&&";
            case "or":  return "||";
            case "&":  return "&";
            case "|":  return "|";
            case "xor": return "^";
            case "shl": return "<<";
            case "shr": return ">>";
            default:
                throw new UnsupportedAotException("AOT: binary op '" + bpOp + "' no soportado");
        }
    }

    /** Despacha CallExpr a un builtin si el nombre matchea uno conocido.
     *  Devuelve true si emitió código, false si no es builtin (caller
     *  prueba con native func). H3 #168.
     *
     *  NOTA sobre length(): BP usa `arr.length()` (method call), NO
     *  `len(arr)`. El AOT lo soportará cuando implementemos method
     *  dispatch en #174 — entonces detectaremos MemberAccessExpr
     *  "length" sobre target array y emitiremos array_length helper. */
    private boolean emitBuiltinCall(String name, List<Ast.IExpr> args) {
        switch (name) {
            case "now":
                if (args.size() != 0) {
                    throw new UnsupportedAotException(
                        "AOT: now() no toma argumentos");
                }
                w.print("vm->aot_helpers->now_ms(vm)");
                return true;
            /* #173 — builtins de string (funciones libres en BP). */
            case "charAt":
                requireArgc(name, args, 2);
                w.print("vm->aot_helpers->string_char_at(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(")");
                return true;
            case "charCodeAt":
                requireArgc(name, args, 2);
                w.print("vm->aot_helpers->string_char_code_at(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(")");
                return true;
            case "substring":
                requireArgc(name, args, 3);
                w.print("vm->aot_helpers->string_substring(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(", "); emitExpr(args.get(2));
                w.print(")");
                return true;
            case "intToString":
                requireArgc(name, args, 1);
                w.print("vm->aot_helpers->int_to_string(vm, ");
                emitExpr(args.get(0));
                w.print(")");
                return true;
            default:
                return false;
        }
    }

    /** #173 — valida nº de args de un builtin; lanza si no cuadra. */
    private void requireArgc(String name, List<Ast.IExpr> args, int n) {
        if (args.size() != n) {
            throw new UnsupportedAotException(
                "AOT: builtin '" + name + "' espera " + n + " args, recibió "
                + args.size());
        }
    }

    // ==================== Helpers ====================

    /** #302 paso 2 — ¿este tipo cruza la frontera AOT como REFERENCIA de 8 bytes?
     *  Fuente de verdad = MivmEmitter.occupies8Bytes sobre el BpType RESUELTO: es
     *  el mismo predicado que decide cuántos bytes empuja OP_CALL, así el thunk lee
     *  exactamente lo que el intérprete escribió. El `!is8Byte` deja fuera
     *  long/double (que el AOT rechaza antes en cType). El enum NO es ref
     *  (isScalar, isReference()=false → 4 bytes), a diferencia de una clase. */
    private static boolean isRefBp(BpType t) {
        return t != null && MivmEmitter.occupies8Bytes(t) && !MivmEmitter.is8Byte(t);
    }

    /** #302 paso 2 — ref_mask de una lista de parámetros: bit i = el param i es
     *  una referencia (8 bytes). Lo consumen call_bp_i32 / call_method_i32. */
    private static int refMaskOfParams(List<Symbol.ParamSymbol> ps) {
        int m = 0;
        for (int i = 0; i < ps.size(); i++) if (isRefBp(ps.get(i).type)) m |= (1 << i);
        return m;
    }

    /** #302 paso 2 — la FunctionSymbol de un FuncDef (tipos resueltos), o null si
     *  no hay info semántica. `declSymbols` la mapea al declarar la función. */
    private Symbol.FunctionSymbol funcSym(Ast.FuncDef f) {
        if (semInfo == null) return null;
        Symbol s = semInfo.declSymbols.get(f);
        return (s instanceof Symbol.FunctionSymbol) ? (Symbol.FunctionSymbol) s : null;
    }

    /** #302 paso 2 — ¿el parámetro i-ésimo del thunk es una ref (8 bytes)? Prefiere
     *  el BpType resuelto (distingue enum de clase); sin info semántica cae al
     *  mirror por Ast.TypeRef (string/array/clase → ref; puede errar en enum, caso
     *  que no ocurre sin semInfo en la práctica). */
    private boolean paramIsRef(Ast.FuncDef f, int i) {
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null && i < fs.params.size()) return isRefBp(fs.params.get(i).type);
        return isRefTypeRefFallback(f.params.get(i).type);
    }

    /** #302 paso 2 — ¿el retorno del thunk es una ref (8 bytes)? */
    private boolean returnIsRef(Ast.FuncDef f) {
        if (f.returnType == null) return false;   /* void */
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null) return isRefBp(fs.returnType);
        return isRefTypeRefFallback(f.returnType);
    }

    /** #302 paso 2 — ref_mask de un FuncDef (para el puente call_bp_i32 a una
     *  función BP del mismo módulo). Prefiere BpType; si no, mirror por Ast. */
    private int refMaskOfFuncDef(Ast.FuncDef f) {
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null) return refMaskOfParams(fs.params);
        int m = 0;
        for (int i = 0; i < f.params.size(); i++)
            if (isRefTypeRefFallback(f.params.get(i).type)) m |= (1 << i);
        return m;
    }

    /** Fallback sin BpType: string/array/clase-o-any (default de cType) → ref. */
    private boolean isRefTypeRefFallback(Ast.TypeRef t) {
        if (t instanceof Ast.ArrayTypeRef) return true;
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "float": case "boolean": return false;
                case "long": case "double": return false;   /* rechazados en cType */
                case "string": return true;
                default: return true;   /* clase/any (enum se maneja con BpType) */
            }
        }
        return false;
    }

    /** TypeRef BP → tipo C. null (sin tipo de retorno) → void.
     *  ArrayTypeRef se trata como handle i32 (el ref al heap donde
     *  vive el array). El acceso a elementos pasa por helpers. */
    private String cType(Ast.TypeRef t) {
        if (t == null) return "void";
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": return "int32_t";
                case "float":   return "float";
                case "boolean": return "int32_t";   /* bool como i32 0/1 */
                case "string":  return "int32_t";   /* #171: handle al heap. */
                case "long": case "double":
                    /* #349 — 8 bytes: el AOT v1 marshalla todo en slots de 4.
                     *
                     * El mensaje NO puede decir "en la signature": cType se usa
                     * también para las VARIABLES LOCALES, y decía signature en
                     * los dos casos. Con un `double` declarado dentro del cuerpo,
                     * el usuario iba a mirar la firma, no encontrar nada raro y
                     * quedarse atascado. Un recorte anunciado mal es casi tan
                     * caro como uno mudo.
                     *
                     * Y dice QUÉ HACER, que es lo que convierte un error en una
                     * decisión: o `float` (32 bits, sí soportado) si la precisión
                     * da, o quitar `native` y que esa función corra interpretada
                     * — el resto del módulo sigue compilando a nativo igual. */
                    throw new UnsupportedAotException(
                        "el tipo '" + n + "' ocupa 8 bytes y el AOT v1 sólo maneja "
                        + "valores de 4 (parámetros, retorno y variables locales). "
                        + "Opciones: usar 'float' si la precisión de 32 bits basta, "
                        + "o quitar 'native' de esta función para que corra "
                        + "interpretada (el resto del módulo sigue yendo a nativo).");
                default:
                    /* #174b — clase/enum/any: ref u valor de 4 bytes → handle i32.
                     * (El análisis semántico ya validó el tipo, así que un nombre
                     * desconocido no llega aquí.) */
                    return "int32_t";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            /* Handle al heap. Element type se traduce a través de los
             * helpers array_load_<T> / array_store_<T> en el AccessExpr. */
            return "int32_t";
        }
        throw new UnsupportedAotException(
            "AOT: TypeRef no soportado: " + t.getClass().getSimpleName());
    }

    /** Para el thunk: nombre del helper que lee un valor del tipo
     *  indicado desde el stack BP. Cada slot ocupa 4 bytes BE. */
    private String readHelper(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "boolean": case "string":
                    return "read_i32_be";
                case "float":
                    return "read_f32_be";
                case "long": case "double":
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' (8 bytes) no soportado en thunk native");
                default:
                    /* #174b — clase/enum/any: handle/valor i32. */
                    return "read_i32_be";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            /* El handle es i32. */
            return "read_i32_be";
        }
        throw new UnsupportedAotException(
            "AOT: no hay readHelper para tipo " + (t == null ? "null" : t.getClass().getSimpleName()));
    }

    /** Para el thunk: nombre del helper que escribe un valor del tipo
     *  indicado al stack BP. */
    private String writeHelper(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "boolean": case "string":
                    return "write_i32_be";
                case "float":
                    return "write_f32_be";
                case "long": case "double":
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' (8 bytes) no soportado en thunk native");
                default:
                    /* #174b — clase/enum/any: handle/valor i32. */
                    return "write_i32_be";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            return "write_i32_be";
        }
        throw new UnsupportedAotException(
            "AOT: no hay writeHelper para tipo " + (t == null ? "null" : t.getClass().getSimpleName()));
    }

    private void indent() {
        for (int i = 0; i < indentLevel; i++) w.print("    ");
    }

    /** Para test unitario o uso desde IDE. Corre análisis semántico
     *  best-effort para que las ops de string (#173) se resuelvan. */
    public static String emit(Ast.ModuleNode module) {
        AotCEmitter e = new AotCEmitter(module.name);
        try {
            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            e.setSemanticInfo(analyzer.analyze(module));
        } catch (RuntimeException ignore) {
            /* sin info → las ops string-ambiguas caen a numérico */
        }
        return e.emitModule(module);
    }

    /* ============================================================ */
    /*  #172 helpers — layout de variables nivel-módulo              */
    /* ============================================================ */

    /** Recorre `module.defs` en orden y asigna offsets CS-relativos a
     *  cada VarDecl / ConstDecl con tipo primitivo. Convención cuadrada
     *  con MivmEmitter / ModWriter.registerSymbol: el primer decl
     *  encontrado obtiene offset -4, el siguiente -8, etc.
     *
     *  v1: solo cuenta integer/float/bool (4 bytes); otros tipos (string,
     *  array, ref) no entran en este path. Limitación documentada — si
     *  un .bp más complejo tiene class descriptors o synthetics
     *  intercalados, los offsets podrían descuadrarse y la AOT cascarse
     *  con datos incoherentes. La convención está fijada en MivmEmitter:
     *  user-vars/consts van primero, __initialized y class descriptors
     *  después (verificado en GlobalsAot.mod). */
    private void precomputeModuleVarOffsets(Ast.ModuleNode module) {
        int nextOffset = -4;
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.VarDecl) {
                Ast.VarDecl vd = (Ast.VarDecl) d;
                if (!isPrimitiveTypeName(vd.type)) continue;
                for (Ast.DeclName dn : vd.names) {
                    moduleVarOffsets.put(dn.name, nextOffset);
                    nextOffset -= 4;
                }
            } else if (d instanceof Ast.ConstDecl) {
                Ast.ConstDecl cd = (Ast.ConstDecl) d;
                // L8 v2 — convención actualizada de MivmEmitter: SOLO las const
                // int/float-literal materializan data symbol (compat); bool/
                // string/long/double/negativas se inlinan en cada uso y ya NO
                // ocupan slot. Contarlas aquí descuadraría todos los offsets.
                if (cd.value instanceof Ast.IntLitExpr
                        || cd.value instanceof Ast.FloatLitExpr) {
                    moduleVarOffsets.put(cd.name.name, nextOffset);
                    nextOffset -= 4;
                }
            }
        }
    }

    /** True si el tipo BP cabe en un slot de 4 bytes (integer/float/bool).
     *  Sólo nombres simples; tipos compuestos (array, class refs) no. */
    private boolean isPrimitiveTypeName(Ast.TypeRef t) {
        if (!(t instanceof Ast.SimpleTypeRef)) return false;
        String s = ((Ast.SimpleTypeRef) t).name;
        return "integer".equals(s) || "int".equals(s)
            || "float".equals(s)
            || "boolean".equals(s) || "bool".equals(s);
    }
}
