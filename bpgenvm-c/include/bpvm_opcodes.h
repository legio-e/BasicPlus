/*
 * bpvm_opcodes.h — códigos de opcode estables.
 *
 * Generado MANUALMENTE a partir de docs/OPCODES.md. Si la spec cambia,
 * actualizar también este header. Idealmente sería generado, pero la
 * lista es estática.
 */
#ifndef BPVM_OPCODES_H
#define BPVM_OPCODES_H

/* 0x00..0x0F — núcleo */
#define OP_HALT            0x00
#define OP_PUSH            0x01
#define OP_ADD             0x02
#define OP_PRINT           0x03
#define OP_GET_GLOBAL      0x04
#define OP_SET_GLOBAL      0x05
#define OP_CALL_EXT        0x06
#define OP_CALL            0x07
#define OP_RET             0x08
#define OP_GET_LOCAL       0x09
#define OP_SET_LOCAL       0x0A
#define OP_EQ              0x0B
#define OP_LT              0x0C
#define OP_JUMP            0x0D
#define OP_JUMP_IF_FALSE   0x0E
#define OP_ENTER           0x0F

/* 0x10..0x1F */
#define OP_SUB             0x10
#define OP_MUL             0x11
#define OP_DIV             0x12
#define OP_MOD             0x13
#define OP_NEG             0x14
#define OP_AND             0x15
#define OP_OR              0x16
#define OP_NOT             0x17
#define OP_GT              0x18
#define OP_GE              0x19
#define OP_LE              0x1A
#define OP_NEQ             0x1B
#define OP_LEA_GLOBAL      0x1C
#define OP_NEWARRAY        0x1D
#define OP_ALOAD           0x1E
#define OP_ASTORE          0x1F

/* 0x20..0x2F */
#define OP_ALEN            0x20
#define OP_PRINT_CHAR      0x21
#define OP_PRINT_STRING    0x22
#define OP_LEA_LOCAL       0x23
#define OP_JUMP8           0x24
#define OP_JUMP16          0x25
#define OP_JUMP_IF_FALSE8  0x26
#define OP_JUMP_IF_FALSE16 0x27
#define OP_FPUSH           0x28
#define OP_FADD            0x29
#define OP_FSUB            0x2A
#define OP_FMUL            0x2B
#define OP_FDIV            0x2C
#define OP_FMOD            0x2D
#define OP_FNEG            0x2E
#define OP_FEQ             0x2F

/* 0x30..0x3F */
#define OP_FNEQ            0x30
#define OP_FLT             0x31
#define OP_FLE             0x32
#define OP_FGT             0x33
#define OP_FGE             0x34
#define OP_FPRINT          0x35
#define OP_I2F             0x36
#define OP_F2I             0x37
#define OP_NEWARRAY_I8     0x38
#define OP_NEWARRAY_I16    0x39
#define OP_ALOAD_I8        0x3A
#define OP_ALOAD_U8        0x3B
#define OP_ALOAD_I16       0x3C
#define OP_ALOAD_U16       0x3D
#define OP_ASTORE_I8       0x3E
#define OP_ASTORE_I16      0x3F

/* 0x40..0x4F */
#define OP_GET_GLOBAL_I8   0x40
#define OP_GET_GLOBAL_U8   0x41
#define OP_GET_GLOBAL_I16  0x42
#define OP_GET_GLOBAL_U16  0x43
#define OP_SET_GLOBAL_I8   0x44
#define OP_SET_GLOBAL_I16  0x45
#define OP_BAND            0x46
#define OP_BOR             0x47
#define OP_BXOR            0x48
#define OP_BNOT            0x49
#define OP_SHL             0x4A
#define OP_SHR_S           0x4B
#define OP_SHR_U           0x4C
#define OP_I32_TO_I8       0x4D
#define OP_I32_TO_U8       0x4E
#define OP_I32_TO_I16      0x4F

/* 0x50..0x5F */
#define OP_I32_TO_U16      0x50
#define OP_GC_COLLECT      0x51
#define OP_NEW_OBJECT      0x52
#define OP_GET_FIELD       0x53
#define OP_SET_FIELD       0x54
#define OP_INVOKE_VIRTUAL  0x55
#define OP_PRINT_NONL      0x56
#define OP_FPRINT_NONL     0x57
#define OP_PRINT_STR_NONL  0x58
#define OP_PRINT_NL        0x59
#define OP_CALL_BUILTIN    0x5A
#define OP_TRY_BEGIN       0x5B
#define OP_TRY_END         0x5C
#define OP_THROW           0x5D
#define OP_INSTANCEOF      0x5E
#define OP_FREE_REF        0x5F

/* 0x60..0x70 */
#define OP_SET_FIELD_OWNER 0x60
#define OP_PUSH_0          0x61
#define OP_PUSH_1          0x62
#define OP_PUSH_2          0x63
#define OP_PUSH_3          0x64
#define OP_PUSH_4          0x65
#define OP_PUSH_NEG1       0x66
#define OP_GET_LOCAL_S8    0x67
#define OP_SET_LOCAL_S8    0x68
#define OP_LEA_LOCAL_S8    0x69
#define OP_GET_GLOBAL_S8   0x6A
#define OP_SET_GLOBAL_S8   0x6B
#define OP_LEA_GLOBAL_S8   0x6C
#define OP_THREAD_EXIT     0x70

/* 0x71..0x90 — H1.2 (V2): long (i64). 8 bytes / 2 slots. */
#define OP_LPUSH           0x71
#define OP_LADD            0x72
#define OP_LSUB            0x73
#define OP_LMUL            0x74
#define OP_LDIV            0x75
#define OP_LMOD            0x76
#define OP_LNEG            0x77
#define OP_LBAND           0x78
#define OP_LBOR            0x79
#define OP_LBXOR           0x7A
#define OP_LBNOT           0x7B
#define OP_LSHL            0x7C
#define OP_LSHR_S          0x7D
#define OP_LSHR_U          0x7E
#define OP_LEQ             0x7F
#define OP_LNEQ            0x80
#define OP_LLT             0x81
#define OP_LLE             0x82
#define OP_LGT             0x83
#define OP_LGE             0x84
#define OP_LPRINT          0x85
#define OP_LPRINT_NONL     0x86
#define OP_I32_TO_I64      0x87
#define OP_I64_TO_I32      0x88
#define OP_GET_LOCAL_L     0x89
#define OP_SET_LOCAL_L     0x8A
#define OP_GET_GLOBAL_L    0x8B
#define OP_SET_GLOBAL_L    0x8C
#define OP_NEWARRAY_I64    0x8D
#define OP_ALOAD_I64       0x8E
#define OP_ASTORE_I64      0x8F
#define OP_LRET            0x90

/* 0x91..0xA7 — H1.3 (V2): double (f64). Aritmética + conversiones; el
 * almacenamiento de 8 bytes se reusa de long. */
#define OP_DPUSH           0x91
#define OP_DADD            0x92
#define OP_DSUB            0x93
#define OP_DMUL            0x94
#define OP_DDIV            0x95
#define OP_DMOD            0x96
#define OP_DNEG            0x97
#define OP_DEQ             0x98
#define OP_DNEQ            0x99
#define OP_DLT             0x9A
#define OP_DLE             0x9B
#define OP_DGT             0x9C
#define OP_DGE             0x9D
#define OP_DPRINT          0x9E
#define OP_DPRINT_NONL     0x9F
#define OP_I2D             0xA0
#define OP_D2I             0xA1
#define OP_L2D             0xA2
#define OP_D2L             0xA3
#define OP_F2D             0xA4
#define OP_D2F             0xA5
#define OP_L2F             0xA6
#define OP_F2L             0xA7

#endif /* BPVM_OPCODES_H */
