/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.bytecode;

/**
 * Catálogo único de opcodes de la VM. Lo comparten el generador
 * (ModWriter), el intérprete (VirtualMachine) y el disassembler (Disasm).
 *
 * @author eortiz
 */
public enum OpCode {
    HALT          (0x00, OperandKind.NONE),
    PUSH          (0x01, OperandKind.IMM_I32),
    ADD           (0x02, OperandKind.NONE),
    PRINT         (0x03, OperandKind.NONE),
    GET_GLOBAL    (0x04, OperandKind.SOFF_I16),
    SET_GLOBAL    (0x05, OperandKind.SOFF_I16),
    CALL_EXT      (0x06, OperandKind.IDX_U16),
    CALL          (0x07, OperandKind.CALL_TGT_I32),
    RET           (0x08, OperandKind.IMM_U8),
    GET_LOCAL     (0x09, OperandKind.SOFF_I16),
    SET_LOCAL     (0x0A, OperandKind.SOFF_I16),
    EQ            (0x0B, OperandKind.NONE),
    LT            (0x0C, OperandKind.NONE),
    JUMP          (0x0D, OperandKind.JUMP_REL_I32),
    JUMP_IF_FALSE (0x0E, OperandKind.JUMP_REL_I32),
    ENTER         (0x0F, OperandKind.SIZE_U16),

    SUB           (0x10, OperandKind.NONE),
    MUL           (0x11, OperandKind.NONE),
    DIV           (0x12, OperandKind.NONE),
    MOD           (0x13, OperandKind.NONE),
    NEG           (0x14, OperandKind.NONE),

    AND           (0x15, OperandKind.NONE),
    OR            (0x16, OperandKind.NONE),
    NOT           (0x17, OperandKind.NONE),

    GT            (0x18, OperandKind.NONE),
    GE            (0x19, OperandKind.NONE),
    LE            (0x1A, OperandKind.NONE),
    NEQ           (0x1B, OperandKind.NONE),

    LEA_GLOBAL    (0x1C, OperandKind.SOFF_I16),
    NEWARRAY      (0x1D, OperandKind.NONE),
    ALOAD         (0x1E, OperandKind.NONE),
    ASTORE        (0x1F, OperandKind.NONE),
    ALEN          (0x20, OperandKind.NONE),

    PRINT_CHAR    (0x21, OperandKind.NONE),
    PRINT_STRING  (0x22, OperandKind.NONE),

    LEA_LOCAL     (0x23, OperandKind.SOFF_I16),

    JUMP8           (0x24, OperandKind.JUMP_REL_I8),
    JUMP16          (0x25, OperandKind.JUMP_REL_I16),
    JUMP_IF_FALSE8  (0x26, OperandKind.JUMP_REL_I8),
    JUMP_IF_FALSE16 (0x27, OperandKind.JUMP_REL_I16),

    FPUSH         (0x28, OperandKind.IMM_F32),
    FADD          (0x29, OperandKind.NONE),
    FSUB          (0x2A, OperandKind.NONE),
    FMUL          (0x2B, OperandKind.NONE),
    FDIV          (0x2C, OperandKind.NONE),
    FMOD          (0x2D, OperandKind.NONE),
    FNEG          (0x2E, OperandKind.NONE),
    FEQ           (0x2F, OperandKind.NONE),
    FNEQ          (0x30, OperandKind.NONE),
    FLT           (0x31, OperandKind.NONE),
    FLE           (0x32, OperandKind.NONE),
    FGT           (0x33, OperandKind.NONE),
    FGE           (0x34, OperandKind.NONE),
    FPRINT        (0x35, OperandKind.NONE),
    I2F           (0x36, OperandKind.NONE),
    F2I           (0x37, OperandKind.NONE),

    NEWARRAY_I8     (0x38, OperandKind.NONE),
    NEWARRAY_I16    (0x39, OperandKind.NONE),
    ALOAD_I8        (0x3A, OperandKind.NONE),
    ALOAD_U8        (0x3B, OperandKind.NONE),
    ALOAD_I16       (0x3C, OperandKind.NONE),
    ALOAD_U16       (0x3D, OperandKind.NONE),
    ASTORE_I8       (0x3E, OperandKind.NONE),
    ASTORE_I16      (0x3F, OperandKind.NONE),
    GET_GLOBAL_I8   (0x40, OperandKind.SOFF_I16),
    GET_GLOBAL_U8   (0x41, OperandKind.SOFF_I16),
    GET_GLOBAL_I16  (0x42, OperandKind.SOFF_I16),
    GET_GLOBAL_U16  (0x43, OperandKind.SOFF_I16),
    SET_GLOBAL_I8   (0x44, OperandKind.SOFF_I16),
    SET_GLOBAL_I16  (0x45, OperandKind.SOFF_I16),

    BAND          (0x46, OperandKind.NONE),
    BOR           (0x47, OperandKind.NONE),
    BXOR          (0x48, OperandKind.NONE),
    BNOT          (0x49, OperandKind.NONE),
    SHL           (0x4A, OperandKind.NONE),
    SHR_S         (0x4B, OperandKind.NONE),
    SHR_U         (0x4C, OperandKind.NONE),

    I32_TO_I8     (0x4D, OperandKind.NONE),
    I32_TO_U8     (0x4E, OperandKind.NONE),
    I32_TO_I16    (0x4F, OperandKind.NONE),
    I32_TO_U16    (0x50, OperandKind.NONE),

    // --- Garbage Collector ---
    //   Trigger manual del GC mark-and-sweep conservativo. Imprime stats.
    //   El allocator lo invoca automáticamente cuando una asignación falla.
    GC_COLLECT    (0x51, OperandKind.NONE),

    // --- Clases y objetos ---
    //   NEW_OBJECT      cs_off(i16): aloca objeto del tamaño que indica el class descriptor
    //                   ubicado en CS+cs_off, escribe class_ptr en la cabecera, zero-init
    //                   campos, push ref.
    //   GET_FIELD       slot(u8):    ref=pop; push *(ref + 4 + slot*4).
    //   SET_FIELD       slot(u8):    val=pop; ref=pop; *(ref+4+slot*4)=val.
    //   INVOKE_VIRTUAL  slot(u8), numArgs(u8): peek this en SP-4-numArgs*4, lee class_ptr,
    //                   salta a vtable[slot] con convención CALL estándar.
    NEW_OBJECT     (0x52, OperandKind.SOFF_I16),
    GET_FIELD      (0x53, OperandKind.IMM_U8),
    SET_FIELD      (0x54, OperandKind.IMM_U8),
    INVOKE_VIRTUAL (0x55, OperandKind.SLOT_NUMARGS_U8U8),

    // --- Variantes de print sin newline (para emitir print multi-item con separadores).
    //   PRINT/FPRINT/PRINT_STRING ya emiten newline; éstas dejan el cursor en la línea.
    PRINT_NONL     (0x56, OperandKind.NONE),  // int → stdout, sin newline
    FPRINT_NONL    (0x57, OperandKind.NONE),  // float → stdout, sin newline
    PRINT_STR_NONL (0x58, OperandKind.NONE),  // string (ref length+chars) → stdout, sin newline
    PRINT_NL       (0x59, OperandKind.NONE),  // emite un newline solo (separador final)

    // --- Stdlib built-in dispatch. Operando u16 = id del enum Builtin.
    CALL_BUILTIN   (0x5A, OperandKind.IDX_U16),

    // --- Manejo de excepciones (modelo dynamic handler / setjmp-longjmp).
    //   TRY_BEGIN <handler_rel:i32>: push del EH actual a la pila, set EH = nuevo handler.
    //   TRY_END:                     pop la pila a EH (cierre normal del try).
    //   THROW:                       pop valor; salta a EH.handler_pc restaurando SP/BP/CS
    //                                desde lo guardado en EH; pop la pila a EH (para que un
    //                                throw dentro del catch propague al exterior).
    TRY_BEGIN      (0x5B, OperandKind.TRY_HANDLER_I32_I16),
    TRY_END        (0x5C, OperandKind.NONE),
    THROW          (0x5D, OperandKind.NONE),
    // BUG-2 — TRY_BEGIN cross-module: clase esperada por offset i32 (parcheado
    //   en link-time vía ehClassFixups; alcanza descriptores de otros módulos).
    TRY_BEGIN_EXT  (0xAB, OperandKind.TRY_HANDLER_I32_I32),

    // --- Type check: pop ref; push 1 si ref es instancia de la clase indicada
    //   (su class_ptr está en la cadena de herencia hasta la esperada); 0 si no.
    //   El valor 0 (null o no-ref) siempre devuelve 0.
    INSTANCEOF     (0x5E, OperandKind.SOFF_I16),

    // --- Ownership: liberación determinista de un objeto.
    //   FREE_REF: pop ref; si la ref apunta a una instancia de clase en el heap,
    //   devuelve el bloque al free list del allocator. Si es 0 o no-objeto, no-op.
    //   Recursivo sobre los campos owner del objeto.
    FREE_REF       (0x5F, OperandKind.NONE),

    // --- SET_FIELD para campos owner: libera el objeto anterior antes de
    //   escribir el nuevo. Semánticamente equivale a:
    //     old = field[slot]; freeOwnedObject(old); field[slot] = val
    SET_FIELD_OWNER (0x60, OperandKind.IMM_U8),

    // ============================================================
    // Compactos para reducir tamaño del bytecode (no añaden semántica nueva).
    // ============================================================
    // --- PUSH inmediato de constantes muy frecuentes. Sin operando, 1 byte
    //     total cada uno. Reemplazan a `PUSH 0..4 / -1` que ocupan 5 bytes.
    PUSH_0         (0x61, OperandKind.NONE),
    PUSH_1         (0x62, OperandKind.NONE),
    PUSH_2         (0x63, OperandKind.NONE),
    PUSH_3         (0x64, OperandKind.NONE),
    PUSH_4         (0x65, OperandKind.NONE),
    PUSH_NEG1      (0x66, OperandKind.NONE),

    // --- Variantes con offset i8 (signed) de locales y globales. Cuando el
    //     offset cabe en -128..127 ocupan 1 byte de operando en vez de 2.
    //     Sin cambio semántico respecto a las versiones i16: el sign-extend
    //     a int produce el mismo direccionamiento.
    GET_LOCAL_S8   (0x67, OperandKind.SOFF_I8),
    SET_LOCAL_S8   (0x68, OperandKind.SOFF_I8),
    LEA_LOCAL_S8   (0x69, OperandKind.SOFF_I8),
    GET_GLOBAL_S8  (0x6A, OperandKind.SOFF_I8),
    SET_GLOBAL_S8  (0x6B, OperandKind.SOFF_I8),
    LEA_GLOBAL_S8  (0x6C, OperandKind.SOFF_I8),

    // --- Threading ---
    // THREAD_EXIT: termina el thread actual (sin tumbar la VM). Lo usa la VM
    //   internamente como sentinela en memory[0]: cuando un worker hace RET
    //   desde su run(), el saved PC apunta a 0 y aquí se lee THREAD_EXIT.
    //   HALT (0x00) sigue existiendo pero termina la VM ENTERA (sólo legal
    //   en el thread main; en un worker es un error fatal).
    THREAD_EXIT    (0x70, OperandKind.NONE),

    // ============================================================
    // H1.2 (V2) — long (i64). Tipo de 8 bytes / 2 slots. Mismo set de
    // operaciones que i32 pero con opcodes propios. El operand stack es
    // byte-addressed (sp = offset en bytes), así que un push de long es
    // escribir 8 bytes y sp += 8 — natural, sin "2 slots" a nivel VM.
    // Aritmética: pop 16 (2 longs), push 8. Comparaciones: pop 16, push 4
    // (bool i32). Conversiones I32_TO_I64 (sign-extend) / I64_TO_I32 (trunc).
    // GET/SET_LOCAL_L y GET/SET_GLOBAL_L mueven 8 bytes. NEWARRAY/ALOAD/
    // ASTORE_I64 = arrays de 8 bytes/elem. LRET = return de 8 bytes (mismo
    // teardown que RET; su operando sigue siendo el slot-count de params).
    // ============================================================
    LPUSH          (0x71, OperandKind.IMM_I64),
    LADD           (0x72, OperandKind.NONE),
    LSUB           (0x73, OperandKind.NONE),
    LMUL           (0x74, OperandKind.NONE),
    LDIV           (0x75, OperandKind.NONE),
    LMOD           (0x76, OperandKind.NONE),
    LNEG           (0x77, OperandKind.NONE),
    LBAND          (0x78, OperandKind.NONE),
    LBOR           (0x79, OperandKind.NONE),
    LBXOR          (0x7A, OperandKind.NONE),
    LBNOT          (0x7B, OperandKind.NONE),
    LSHL           (0x7C, OperandKind.NONE),
    LSHR_S         (0x7D, OperandKind.NONE),
    LSHR_U         (0x7E, OperandKind.NONE),
    LEQ            (0x7F, OperandKind.NONE),
    LNEQ           (0x80, OperandKind.NONE),
    LLT            (0x81, OperandKind.NONE),
    LLE            (0x82, OperandKind.NONE),
    LGT            (0x83, OperandKind.NONE),
    LGE            (0x84, OperandKind.NONE),
    LPRINT         (0x85, OperandKind.NONE),
    LPRINT_NONL    (0x86, OperandKind.NONE),
    I32_TO_I64     (0x87, OperandKind.NONE),
    I64_TO_I32     (0x88, OperandKind.NONE),
    GET_LOCAL_L    (0x89, OperandKind.SOFF_I16),
    SET_LOCAL_L    (0x8A, OperandKind.SOFF_I16),
    GET_GLOBAL_L   (0x8B, OperandKind.SOFF_I16),
    SET_GLOBAL_L   (0x8C, OperandKind.SOFF_I16),
    NEWARRAY_I64   (0x8D, OperandKind.NONE),
    ALOAD_I64      (0x8E, OperandKind.NONE),
    ASTORE_I64     (0x8F, OperandKind.NONE),
    LRET           (0x90, OperandKind.IMM_U8),

    // ============================================================
    // H1.3 (V2) — double (f64). 8 bytes; REUSA el almacenamiento de 8 bytes
    // de long (GET/SET_LOCAL_L, GET/SET_GLOBAL_L, LRET, NEWARRAY_I64/ALOAD_I64/
    // ASTORE_I64 — opacos a 8 bytes). Solo añade aritmética f64 + la matriz
    // de conversiones numéricas (i32/i64/f32/f64).
    // ============================================================
    DPUSH          (0x91, OperandKind.IMM_I64),  // 8 bytes = bits raw del f64
    DADD           (0x92, OperandKind.NONE),
    DSUB           (0x93, OperandKind.NONE),
    DMUL           (0x94, OperandKind.NONE),
    DDIV           (0x95, OperandKind.NONE),
    DMOD           (0x96, OperandKind.NONE),
    DNEG           (0x97, OperandKind.NONE),
    DEQ            (0x98, OperandKind.NONE),
    DNEQ           (0x99, OperandKind.NONE),
    DLT            (0x9A, OperandKind.NONE),
    DLE            (0x9B, OperandKind.NONE),
    DGT            (0x9C, OperandKind.NONE),
    DGE            (0x9D, OperandKind.NONE),
    DPRINT         (0x9E, OperandKind.NONE),
    DPRINT_NONL    (0x9F, OperandKind.NONE),
    I2D            (0xA0, OperandKind.NONE),   // i32 → f64
    D2I            (0xA1, OperandKind.NONE),   // f64 → i32 (trunc)
    L2D            (0xA2, OperandKind.NONE),   // i64 → f64
    D2L            (0xA3, OperandKind.NONE),   // f64 → i64 (trunc)
    F2D            (0xA4, OperandKind.NONE),   // f32 → f64
    D2F            (0xA5, OperandKind.NONE),   // f64 → f32
    L2F            (0xA6, OperandKind.NONE),   // i64 → f32
    F2L            (0xA7, OperandKind.NONE),   // f32 → i64 (trunc)
    // H5/BUG-6 — campos de instancia de 8 bytes (long/double). slot(u8) =
    // índice de slot (4 bytes/slot) del campo; r/w 8 bytes en ref+4+slot*4.
    GET_FIELD_LONG (0xA8, OperandKind.IMM_U8), // ref=pop; push *(ref+4+slot*4) 8 bytes
    SET_FIELD_LONG (0xA9, OperandKind.IMM_U8); // val(8)=pop; ref=pop; *(ref+4+slot*4)=val

    /** Byte estable que va a parar al fichero .mod. */
    public final byte code;

    /** Descripción del operando que acompaña a este opcode (puede ser NONE). */
    public final OperandKind operandKind;

    OpCode(int code, OperandKind operandKind) {
        this.code = (byte) code;
        this.operandKind = operandKind;
    }

    public enum OperandKind {
        NONE          (0),
        IMM_I32       (4),
        IMM_I64       (8),
        IMM_U8        (1),
        SOFF_I8       (1),   // offset signed de 1 byte para variantes compactas
        SOFF_I16      (2),
        IDX_U16       (2),
        SIZE_U16      (2),
        CALL_TGT_I32  (4),
        JUMP_REL_I32  (4),
        JUMP_REL_I16  (2),
        JUMP_REL_I8   (1),
        IMM_F32       (4),
        SLOT_NUMARGS_U8U8 (2),
        // i32 (handler offset relativo al PC del TRY_BEGIN) + i16 (cs_off de clase, 0=any)
        TRY_HANDLER_I32_I16 (6),
        // BUG-2 — i32 (handler offset) + i32 (cs_off de clase cross-module)
        TRY_HANDLER_I32_I32 (8);

        public final int bytes;
        OperandKind(int bytes) { this.bytes = bytes; }
    }

    private static final OpCode[] BY_CODE = new OpCode[256];
    static {
        for (OpCode op : values()) {
            BY_CODE[op.code & 0xFF] = op;
        }
    }

    public static OpCode fromByte(byte b) {
        OpCode op = BY_CODE[b & 0xFF];
        if (op == null) {
            throw new IllegalArgumentException(
                    String.format("Opcode desconocido: 0x%02X", b & 0xFF));
        }
        return op;
    }
}
