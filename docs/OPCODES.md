# Catálogo de opcodes de la VM BasicPlus

Documento canónico de los opcodes del bytecode `.mod`. Cada entrada
incluye:
- Byte (`code`), mnemonic.
- Operando inmediato (si lo hay): tipo + tamaño en bytes.
- Stack effect: `( inputs -- outputs )` con la cima a la derecha.
- Semántica precisa (estado del intérprete tras ejecutar).

Convenciones:
- **i32, u32, i16, u16, i8, u8, f32**: enteros y float big-endian.
- **soff**: signed offset al `bp` (positivo = arg/var arriba; negativo
  = saved regs / args debajo).
- **rel**: relativo al **instruction address** (byte del opcode de jump,
  no al pc post-operando). Esto difiere del estilo JVM. Implementación
  de referencia: `pc = (pc_at_opcode) + rel`. Pseudo-código:
  ```
  instruction_addr = pc - 1     # ya consumimos el byte de opcode
  rel              = read_immediate(pc)
  pc               = instruction_addr + rel
  ```
  Aplica a `JUMP`, `JUMP_IF_FALSE` y sus variantes compactas
  (`JUMP8/16`, `JUMP_IF_FALSE8/16`).
- **csOff**: relativo al `cs` (= codeStart) del módulo actual.
- Stack effect notación: `( a b -- c )` significa pop b, pop a, push c.
- "tc.X" = campos del ThreadContext (ver `HEAP_LAYOUT.md` §5).

Tras ejecutar un opcode, `pc` apunta al siguiente opcode (suma de su
propio byte + tamaño del operando), salvo que el opcode altere el flujo
(JUMP, CALL, RET, THROW, INVOKE_VIRTUAL).

---

## 0x00..0x0F — núcleo

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x00 | `HALT` | — | `( -- )` | Termina la VM entera. **Solo legal en thread main**; en un worker es fatal (mensaje "HALT en thread no-main"). |
| 0x01 | `PUSH` | `i32` value | `( -- v )` | Empuja `value` al stack. |
| 0x02 | `ADD` | — | `( a b -- a+b )` | Suma entera (i32 wrap). |
| 0x03 | `PRINT` | — | `( v -- )` | Imprime int + newline al OutputSink. |
| 0x04 | `GET_GLOBAL` | `i16` soff | `( -- v )` | Push `mem[cs + soff]` como i32 (lectura de 4 bytes BE). |
| 0x05 | `SET_GLOBAL` | `i16` soff | `( v -- )` | Escribe i32 `v` en `mem[cs + soff]`. |
| 0x06 | `CALL_EXT` | `u16` idx | `( -- )` | Llama a la función importada en índice `idx` de la ext-table del módulo. Equivalente a `CALL` con target = `mem[extTableAddress + idx*4]`. Frame guarda saved pc/bp/cs como en CALL. |
| 0x07 | `CALL` | `i32` relAddr | `( -- )` | Llama a función dentro del mismo módulo. `relAddr` es relativo al `cs` (= offset CS-relative del entry-point). Push saved pc, bp, cs; bp ← sp; pc ← cs + relAddr. |
| 0x08 | `RET` | `u8` paramsCount | `( retVal -- )` | Pop `retVal` del top del stack del callee. Restaura cs/bp/pc de los slots guardados (`[bp-12]`, `[bp-8]`, `[bp-4]`). Sp ← `bp - 12 - paramsCount*4`. Push `retVal` al nuevo sp. (El callee debe asegurar que `retVal` está en el top antes del RET; típicamente con un `PUSH 0` o `GET_LOCAL +slot` que contenga el return value.) |
| 0x09 | `GET_LOCAL` | `i16` soff | `( -- v )` | Push `mem[bp + soff]` i32. |
| 0x0A | `SET_LOCAL` | `i16` soff | `( v -- )` | Escribe i32 `v` en `mem[bp + soff]`. |
| 0x0B | `EQ` | — | `( a b -- (a==b)?1:0 )` | Comparación entera. |
| 0x0C | `LT` | — | `( a b -- (a<b)?1:0 )` | Comparación entera signed. |
| 0x0D | `JUMP` | `i32` rel | `( -- )` | `pc ← pc + rel` (donde pc ya está tras el operando). |
| 0x0E | `JUMP_IF_FALSE` | `i32` rel | `( v -- )` | Si `v == 0`, `pc ← pc + rel`; sino, pc continúa. |
| 0x0F | `ENTER` | `u16` bytes | `( -- )` | Reserva `bytes` en el stack (`sp += bytes`). Lo emite el frontend al inicio de cada función para alocar locales. |

## 0x10..0x1F — aritmética + arrays básicos

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x10 | `SUB` | — | `( a b -- a-b )` | |
| 0x11 | `MUL` | — | `( a b -- a*b )` | |
| 0x12 | `DIV` | — | `( a b -- a/b )` | RuntimeError si `b == 0`. |
| 0x13 | `MOD` | — | `( a b -- a%b )` | Signo del dividendo. RuntimeError si `b == 0`. |
| 0x14 | `NEG` | — | `( a -- -a )` | i32 negate. |
| 0x15 | `AND` | — | `( a b -- a&&b )` | Lógico boolean (no bitwise). |
| 0x16 | `OR`  | — | `( a b -- a||b )` | Lógico boolean. |
| 0x17 | `NOT` | — | `( a -- !a )` | Lógico boolean. |
| 0x18 | `GT`  | — | `( a b -- (a>b)?1:0 )` | |
| 0x19 | `GE`  | — | `( a b -- (a>=b)?1:0 )` | |
| 0x1A | `LE`  | — | `( a b -- (a<=b)?1:0 )` | |
| 0x1B | `NEQ` | — | `( a b -- (a!=b)?1:0 )` | |
| 0x1C | `LEA_GLOBAL` | `i16` soff | `( -- addr )` | Push `cs + soff` (dirección absoluta del global). |
| 0x1D | `NEWARRAY` | — | `( size -- ref )` | Aloca `TYPE_ARRAY_I32` con `size` elementos i32. Length escrito en `ref+0`, payload zeroed. RuntimeError si `size < 0`. Bajo `synchronized(vmLock)` desde heapAlloc hasta el push. |
| 0x1E | `ALOAD` | — | `( ref idx -- v )` | i32 lookup: `v = mem[ref + 4 + idx*4]`. RuntimeError out-of-range. |
| 0x1F | `ASTORE` | — | `( ref idx v -- )` | i32 store: `mem[ref + 4 + idx*4] = v`. RuntimeError out-of-range. |

## 0x20..0x2F — output, jumps compactos, float push

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x20 | `ALEN` | — | `( ref -- length )` | Push `mem[ref+0]`. Devuelve 0 si `ref == 0`. |
| 0x21 | `PRINT_CHAR` | — | `( ch -- )` | Escribe codepoint al output sink (sin newline). |
| 0x22 | `PRINT_STRING` | — | `( ref -- )` | Imprime string + newline. Lee length+chars del array i32. |
| 0x23 | `LEA_LOCAL` | `i16` soff | `( -- addr )` | Push `bp + soff` (dirección absoluta del local). |
| 0x24 | `JUMP8` | `i8` rel | `( -- )` | Variante compacta de JUMP. |
| 0x25 | `JUMP16` | `i16` rel | `( -- )` | Variante mediana. |
| 0x26 | `JUMP_IF_FALSE8` | `i8` rel | `( v -- )` | |
| 0x27 | `JUMP_IF_FALSE16` | `i16` rel | `( v -- )` | |
| 0x28 | `FPUSH` | `f32` v | `( -- v )` | Push float (en stack BP los floats son i32 con `Float.floatToRawIntBits`). |
| 0x29 | `FADD` | — | `( a b -- a+b )` | Float add. |
| 0x2A | `FSUB` | — | | |
| 0x2B | `FMUL` | — | | |
| 0x2C | `FDIV` | — | | RuntimeError si `b == 0.0`. |
| 0x2D | `FMOD` | — | | Math.IEEEremainder o equiv. |
| 0x2E | `FNEG` | — | `( a -- -a )` | |
| 0x2F | `FEQ` | — | `( a b -- ... )` | Float eq estricto. |

## 0x30..0x3F — float comparisons + arrays narrow

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x30 | `FNEQ` | — | | |
| 0x31 | `FLT` | — | | |
| 0x32 | `FLE` | — | | |
| 0x33 | `FGT` | — | | |
| 0x34 | `FGE` | — | | |
| 0x35 | `FPRINT` | — | `( f -- )` | Imprime float + newline. |
| 0x36 | `I2F` | — | `( i -- f )` | Convierte i32 → f32 (bits → raw bits del float). |
| 0x37 | `F2I` | — | `( f -- i )` | Convierte f32 → i32 (truncar). |
| 0x38 | `NEWARRAY_I8` | — | `( size -- ref )` | Aloca `TYPE_ARRAY_I8` con `size` bytes. Igual que NEWARRAY pero 1 byte por elemento. |
| 0x39 | `NEWARRAY_I16` | — | `( size -- ref )` | `TYPE_ARRAY_I16`. 2 bytes por elemento. |
| 0x3A | `ALOAD_I8`  | — | `( ref idx -- v )` | Lee `mem[ref+4+idx]` como i8, sign-extend a i32. |
| 0x3B | `ALOAD_U8`  | — | `( ref idx -- v )` | Lee como u8, zero-extend. |
| 0x3C | `ALOAD_I16` | — | `( ref idx -- v )` | Lee 2 bytes BE, sign-extend. |
| 0x3D | `ALOAD_U16` | — | `( ref idx -- v )` | Zero-extend. |
| 0x3E | `ASTORE_I8`  | — | `( ref idx v -- )` | `mem[ref+4+idx] = v & 0xFF`. |
| 0x3F | `ASTORE_I16` | — | `( ref idx v -- )` | 2 bytes BE, `v & 0xFFFF`. |

## 0x40..0x4F — globals narrow + bitwise + casts

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x40 | `GET_GLOBAL_I8`  | `i16` soff | `( -- v )` | Lee `mem[cs+soff]` como i8 sign-extended. |
| 0x41 | `GET_GLOBAL_U8`  | `i16` soff | `( -- v )` | Zero-extend. |
| 0x42 | `GET_GLOBAL_I16` | `i16` soff | `( -- v )` | 2 bytes BE sign-extended. |
| 0x43 | `GET_GLOBAL_U16` | `i16` soff | `( -- v )` | Zero-extend. |
| 0x44 | `SET_GLOBAL_I8`  | `i16` soff | `( v -- )` | `mem[cs+soff] = v & 0xFF`. |
| 0x45 | `SET_GLOBAL_I16` | `i16` soff | `( v -- )` | 2 bytes BE. |
| 0x46 | `BAND` | — | `( a b -- a&b )` | Bitwise AND. |
| 0x47 | `BOR`  | — | `( a b -- a\|b )` | Bitwise OR. |
| 0x48 | `BXOR` | — | `( a b -- a^b )` | Bitwise XOR. |
| 0x49 | `BNOT` | — | `( a -- ~a )` | Bitwise NOT (complement). |
| 0x4A | `SHL` | — | `( a b -- a<<b )` | Shift left lógico. `b & 31`. |
| 0x4B | `SHR_S` | — | `( a b -- a>>b )` | Shift right aritmético. |
| 0x4C | `SHR_U` | — | `( a b -- a>>>b )` | Shift right lógico (zero-fill). |
| 0x4D | `I32_TO_I8`  | — | `( v -- v8s )` | Trunca a i8 con check de rango (-128..127). RuntimeError si fuera. |
| 0x4E | `I32_TO_U8`  | — | `( v -- v8u )` | 0..255. |
| 0x4F | `I32_TO_I16` | — | `( v -- v16s )` | -32768..32767. |

## 0x50..0x5F — clases, builtins, exception handling

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x50 | `I32_TO_U16` | — | `( v -- v16u )` | 0..65535. |
| 0x51 | `GC_COLLECT` | — | `( -- )` | Dispara GC manual. Imprime stats al output. |
| 0x52 | `NEW_OBJECT` | `i16` csOff | `( -- ref )` | `classPtr = cs + csOff`. Lee `num_fields` del descriptor. `ref = heapAlloc(num_fields*4, TYPE_OBJECT)`. Escribe `class_ptr` en `ref+0`. Zero-init el resto. Push `ref`. Bajo vmLock. |
| 0x53 | `GET_FIELD` | `u8` slot | `( ref -- v )` | `v = mem[ref + 4 + slot*4]`. |
| 0x54 | `SET_FIELD` | `u8` slot | `( ref v -- )` | `mem[ref + 4 + slot*4] = v`. |
| 0x55 | `INVOKE_VIRTUAL` | `u8` slot, `u8` numArgs | `( ...args this -- )` | Despacho virtual. `this = peek(sp - 4 - numArgs*4)`. RuntimeError si `this == 0`. Lee `classPtr = mem[this]`. Busca `methodOff` en `vt[slot]` del descriptor; si es -1 o `slot >= num_methods`, sube por `parent_offset` y reintenta (L2 v3 fall-back). `targetCS = getCSForDataAddr(desc)`. Push saved pc/bp/cs y salta. |
| 0x56 | `PRINT_NONL` | — | `( v -- )` | Int al output sink sin newline. |
| 0x57 | `FPRINT_NONL` | — | `( f -- )` | Float sin newline. |
| 0x58 | `PRINT_STR_NONL` | — | `( ref -- )` | String sin newline. |
| 0x59 | `PRINT_NL` | — | `( -- )` | Newline solo. |
| 0x5A | `CALL_BUILTIN` | `u16` id | `( args -- result )` | Despacha al builtin con id `id` (ver `BUILTINS.md`). Cada builtin define su stack effect específico. |
| 0x5B | `TRY_BEGIN` | `i32` handlerRel, `i16` clsOff | `( -- )` | Push entry al `handlerStack`: `{ handlerPc = pc + handlerRel, savedSp, savedBp, savedCs, expectedClass = (clsOff==0 ? 0 : cs+clsOff) }`. Si `clsOff == 0`, catch atrapa cualquier ref/excepción. |
| 0x5C | `TRY_END` | — | `( -- )` | Pop top del `handlerStack`. Cierre normal del try (sin excepción). |
| 0x5D | `THROW` | — | `( ref -- )` | Pop `ref`. Si `ref == 0` → RuntimeError "throw null". Busca un handler en `handlerStack` cuya `expectedClass` matchee (= 0 o `isDescendantOf(class(ref), expected)`). Si encuentra: restaura sp/bp/cs/pc desde la entry, pop la pila a esa entry, push `ref` al sp restaurado (el catch lo recibe en local 0). Si no encuentra: BpThreadFault con stack trace BP. |
| 0x5E | `INSTANCEOF` | `i16` csOff | `( ref -- 1|0 )` | `expected = cs + csOff`. Si `ref == 0` → push 0. Sino, sube por `parent_offset` desde `mem[ref]` (con `getCSForDataAddr` para cross-module) y push 1 si encuentra `expected`. |
| 0x5F | `FREE_REF` | — | `( ref -- )` | Liberación determinista. Si `ref` es objeto `TYPE_OBJECT`, recorre los `owner_bitmap` bits del descriptor y libera los fields owners recursivamente, luego marca el bloque como TAG_FREE y lo añade al freeListHead. NOP si `ref == 0` o tipo no liberable. |

## 0x60..0x70 — owner field + compactos + threading

| Code | Op | Operando | Stack effect | Semántica |
|---|---|---|---|---|
| 0x60 | `SET_FIELD_OWNER` | `u8` slot | `( ref v -- )` | Como SET_FIELD pero libera el ref anterior del slot (FREE_REF) antes de escribir el nuevo. Usado para fields declarados `owner`. |
| 0x61 | `PUSH_0` | — | `( -- 0 )` | |
| 0x62 | `PUSH_1` | — | `( -- 1 )` | |
| 0x63 | `PUSH_2` | — | `( -- 2 )` | |
| 0x64 | `PUSH_3` | — | `( -- 3 )` | |
| 0x65 | `PUSH_4` | — | `( -- 4 )` | |
| 0x66 | `PUSH_NEG1` | — | `( -- -1 )` | |
| 0x67 | `GET_LOCAL_S8` | `i8` soff | `( -- v )` | Variante compacta (offset i8 sign-extended) de GET_LOCAL. |
| 0x68 | `SET_LOCAL_S8` | `i8` soff | `( v -- )` | |
| 0x69 | `LEA_LOCAL_S8` | `i8` soff | `( -- addr )` | |
| 0x6A | `GET_GLOBAL_S8` | `i8` soff | `( -- v )` | i32 read; offset es i8 sign-extended. |
| 0x6B | `SET_GLOBAL_S8` | `i8` soff | `( v -- )` | |
| 0x6C | `LEA_GLOBAL_S8` | `i8` soff | `( -- addr )` | |
| 0x70 | `THREAD_EXIT` | — | `( -- )` | Termina el thread actual sin tumbar la VM. Único byte legal después de un RET de un worker: el saved-pc apunta a `memory[0]` y allí está este byte. |

---

## Detalles del calling convention

Para `CALL relAddr` y `CALL_EXT idx`:

```
preconditions:
  stack tiene los args en orden (último arg en top)
  
opcode:
  push pc                ; saved pc del caller (donde retornar)
  push bp                ; saved bp del caller
  push cs                ; saved cs del caller (importante para CALL_EXT)
  bp ← sp                ; nuevo bp = top
  pc ← cs + relAddr      ; (CALL) o targetAbs (CALL_EXT)
  cs ← targetCS          ; sólo CALL_EXT lo cambia; CALL local lo deja igual
```

Layout del frame tras `CALL` + `ENTER N`:

```
              ↑ direcciones altas
  [bp + 0]    primer slot libre para el callee (return slot)
              ...
  [bp + N-4]  último slot de los locales reservados por ENTER N
  [bp + 0]                                  ← inicio de locales, sp justo tras ENTER
  ┌─────────────────────────┐
  │ saved_cs    [bp - 4]    │
  │ saved_bp    [bp - 8]    │
  │ saved_pc    [bp - 12]   │
  ├─────────────────────────┤
  │ arg_N       [bp - 16]   │
  │ ...                     │
  │ arg_0                   │  ← bp - 12 - paramsCount*4
  └─────────────────────────┘
              ↓ direcciones bajas
```

`RET k` revierte:
```
sp -= 4
returnValue = mem[sp]              ; pop del top del stack del callee
pc = mem[bp - 12]
bp_old = mem[bp - 8]
cs = mem[bp - 4]
sp = bp - 12 - k*4                 ; descarta saved + args del caller
bp = bp_old
mem[sp] = returnValue              ; push retVal en el nuevo sp del caller
sp += 4
```

El `k` (paramsCount) NO incluye el `this` implícito de un método. Para
un método de instancia con N params, el RET es `RET (N+1)` (el frontend
inserta `this` como primer param).

---

## Convención de `INVOKE_VIRTUAL` con fall-back al parent

Para `INVOKE_VIRTUAL slot, numArgs` (L2 v3):

```
this = peek(sp - 4 - numArgs*4)
if this == 0: throwBpRuntimeError("null receiver")
classPtr = mem[this]

desc = classPtr
loop:
  bitmapW   = readU16(desc + 4)
  nMethods  = readU16(desc + 2)
  vtBase    = desc + 12 + 2*bitmapW*4
  if slot < nMethods:
    methodOff = readI32(desc + vtBase offset, vtBase + slot*4)
    if methodOff != -1:
      targetCS = getCSForDataAddr(desc)
      goto found
  parentOff = readI32(desc + 8)
  if parentOff == 0:
    throwBpRuntimeError("slot N no resoluble en cadena de herencia")
  curCS = getCSForDataAddr(desc)
  desc = curCS + parentOff
  goto loop

found:
  push saved pc, bp, cs (del caller)
  bp ← sp
  pc ← targetCS + methodOff
  cs ← targetCS
```

El bucle termina porque la cadena de herencia es finita y siempre
acaba en algún descriptor sin parent (parentOff == 0).

---

## Tabla de tamaños de operando

| OperandKind | Tamaño | Uso |
|---|---|---|
| NONE | 0 | sin operando |
| IMM_I32 | 4 | PUSH, JUMP/JUMP_IF_FALSE, CALL, FPUSH (f32) |
| IMM_U8 | 1 | RET (paramsCount), GET/SET_FIELD (slot) |
| SOFF_I8 | 1 | GET/SET_LOCAL_S8, GET/SET_GLOBAL_S8 (variantes compactas) |
| SOFF_I16 | 2 | GET/SET_LOCAL, GET/SET_GLOBAL, LEA_*, NEW_OBJECT, INSTANCEOF |
| IDX_U16 | 2 | CALL_EXT, CALL_BUILTIN |
| SIZE_U16 | 2 | ENTER |
| CALL_TGT_I32 | 4 | CALL (offset CS-relative del target) |
| JUMP_REL_I32 | 4 | JUMP, JUMP_IF_FALSE |
| JUMP_REL_I16 | 2 | JUMP16, JUMP_IF_FALSE16 |
| JUMP_REL_I8 | 1 | JUMP8, JUMP_IF_FALSE8 |
| IMM_F32 | 4 | FPUSH |
| SLOT_NUMARGS_U8U8 | 2 | INVOKE_VIRTUAL (slot u8, numArgs u8) |
| TRY_HANDLER_I32_I16 | 6 | TRY_BEGIN (handlerRel i32, clsOff i16) |

Tamaño total de la instrucción = 1 (opcode) + tamaño del operando.

---

## Codes no asignados

`0x4F .. 0x50` asignados (I32_TO_I16, I32_TO_U16). 
`0x6D .. 0x6F`, `0x71 .. 0xFF` libres para futuras extensiones.

Si una implementación encuentra un opcode no listado, debe fallar con
"Opcode no implementado: 0x%02X en PC %d" — coherente con la VM Java.
