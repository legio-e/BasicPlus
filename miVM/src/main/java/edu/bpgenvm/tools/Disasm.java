/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.tools;

/**
 * Disassembler de ficheros .mod (formato v5).
 *
 * Soporta como extensión (#163):
 *   - Decodificación correcta de CALL_BUILTIN vía catálogo {@link Builtin}.
 *   - Anotación específica del operando IMM_U8 según opcode.
 *   - Operandos SLOT_NUMARGS_U8U8 (INVOKE_VIRTUAL) y TRY_HANDLER_I32_I16
 *     (TRY_BEGIN).
 *   - Reconstrucción de string literals en el data block (chars contiguos
 *     en slots i32).
 *   - Lectura de companion {@code .mdn} (formato MDN/1, header + tabla de
 *     símbolos AOT) y cross-ref de exports del .mod con thunks AOT.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.Builtin;
import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.bytecode.OpCode.OperandKind;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Disasm {

    public static void main(String[] args) throws IOException {
        if (args.length == 0) {
            System.err.println("Uso: java -cp target/classes edu.bpgenvm.tools.Disasm <archivo.mod> [...]");
            System.exit(1);
        }
        for (String fn : args) {
            dump(fn, System.out);
            System.out.println();
        }
    }

    public static void dump(String filename, PrintStream out) throws IOException {
        try (DataInputStream in = new DataInputStream(new FileInputStream(filename))) {
            int magic       = in.readInt();
            int dataSize    = in.readInt();
            int mainOffset  = in.readInt();
            int importsSize = in.readInt();
            int exportsSize = in.readInt();
            int codeSize    = in.readInt();
            int librarySize = in.readInt();

            byte[] libraryBuf = readN(in, librarySize);
            byte[] importsBuf = readN(in, importsSize);
            byte[] exportsBuf = readN(in, exportsSize);
            byte[] dataBuf    = readN(in, dataSize);
            byte[] codeBuf    = readN(in, codeSize);

            String library = new String(libraryBuf, StandardCharsets.UTF_8);

            out.println("================================================================");
            out.printf ("  Disassembly: %s%n", filename);
            out.println("================================================================");

            out.println("=== HEADER ===");
            String magicOk = (magic == ModFormat.MAGIC_NUMBER) ? " (MOD5, OK)" : " (NO MATCH, esperado 0x4D4F4435)";
            out.printf ("  magic         0x%08X%s%n", magic, magicOk);
            out.printf ("  dataSize      %d bytes%n", dataSize);
            out.printf ("  mainOffset    %d %s%n", mainOffset,
                    (mainOffset == -1) ? "(sin main)" : "(relativo al inicio del code)");
            out.printf ("  importsSize   %d bytes%n", importsSize);
            out.printf ("  exportsSize   %d bytes%n", exportsSize);
            out.printf ("  codeSize      %d bytes%n", codeSize);
            out.printf ("  librarySize   %d bytes%n", librarySize);
            out.printf ("  library       %s%n", library.isEmpty() ? "(none)" : "\"" + library + "\"");

            List<ImportEntry> imports = parseImports(importsBuf);
            out.println();
            out.printf ("=== IMPORTS (%d) ===%n", imports.size());
            if (imports.isEmpty()) out.println("  (ninguno)");
            for (int i = 0; i < imports.size(); i++) {
                ImportEntry e = imports.get(i);
                if (e.fromPath.isEmpty()) {
                    out.printf("  [%d] %s%n", i, e.name);
                } else {
                    out.printf("  [%d] %s   from \"%s\"%n", i, e.name, e.fromPath);
                }
            }

            Map<Integer, String> exportsByOffset = parseExports(exportsBuf);

            /* #163 — Companion .mdn: si existe junto al .mod, lo leemos para
             * cross-referenciar exports con thunks AOT. Tolerante: si falla
             * el parse, lo decimos y seguimos sin AOT info. */
            MdnInfo mdn = tryLoadMdnCompanion(filename, library);

            out.println();
            out.printf ("=== EXPORTS (%d) ===%n", exportsByOffset.size());
            if (exportsByOffset.isEmpty()) out.println("  (ninguno)");
            for (Map.Entry<Integer, String> e : exportsByOffset.entrySet()) {
                String aotTag = (mdn != null && mdn.hasSymbol(e.getValue(), library))
                        ? "  [AOT]" : "";
                out.printf("  %-25s @code +%d%s%n",
                        e.getValue(), e.getKey(), aotTag);
            }

            if (mdn != null) {
                out.println();
                out.printf ("=== COMPANION .mdn (\"%s\") ===%n", mdn.path);
                out.printf ("  magic         MDN/%d  abi=%d%n",
                            mdn.version, mdn.abiVersion);
                out.printf ("  code_size     %d bytes (Thumb-2 PIC)%n", mdn.codeSize);
                out.printf ("  symbols       %d%n", mdn.symbols.size());
                for (MdnSymbol s : mdn.symbols) {
                    out.printf("    %-32s thunk @ .mdn+%d%n", s.name, s.thunkOffset);
                }
            }

            out.println();
            out.printf ("=== DATA BLOCK (%d bytes) ===%n", dataSize);
            dumpData(dataBuf, out);

            out.println();
            out.printf ("=== CODE (%d bytes) ===%n", codeSize);
            dumpCode(codeBuf, exportsByOffset, imports, out);
        }
    }

    private static byte[] readN(DataInputStream in, int n) throws IOException {
        byte[] b = new byte[n];
        in.readFully(b);
        return b;
    }

    private static final class ImportEntry {
        final String name;
        final String fromPath;
        ImportEntry(String name, String fromPath) {
            this.name = name;
            this.fromPath = (fromPath == null) ? "" : fromPath;
        }
        @Override public String toString() {
            return fromPath.isEmpty() ? name : name + " (from \"" + fromPath + "\")";
        }
    }

    private static List<ImportEntry> parseImports(byte[] buf) throws IOException {
        List<ImportEntry> result = new ArrayList<>();
        if (buf.length == 0) return result;
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(buf));
        int n = in.readInt();
        for (int i = 0; i < n; i++) {
            String name = in.readUTF();
            String fromPath = in.readUTF();
            result.add(new ImportEntry(name, fromPath));
        }
        return result;
    }

    private static Map<Integer, String> parseExports(byte[] buf) throws IOException {
        Map<Integer, String> result = new LinkedHashMap<>();
        if (buf.length == 0) return result;
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(buf));
        int n = in.readInt();
        for (int i = 0; i < n; i++) {
            String name = in.readUTF();
            int off = in.readInt();
            result.put(off, name);
        }
        return result;
    }

    private static void dumpData(byte[] data, PrintStream out) {
        if (data.length == 0) {
            out.println("  (vacío)");
            return;
        }
        /* #163 — Detección heurística de string literals. El emisor
         * coloca cada char de un literal en un slot i32 independiente
         * (un char por palabra; ver HEAP_LAYOUT). Por consenso de
         * MivmEmitter, el patrón típico es [len:i32][ch:i32]*. Si
         * encontramos un slot con un int pequeño (1..255) seguido de
         * `n` slots cuyos valores son ASCII printable, lo señalamos
         * como string literal en un footer. No alteramos el dump
         * principal — sólo añadimos un resumen al final. */
        List<int[]> detectedStrings = new ArrayList<>();
        for (int i = 0; i + 4 <= data.length; i += 4) {
            int maybeLen = readBigEndianInt(data, i);
            if (maybeLen <= 0 || maybeLen > 256) continue;
            int charsStart = i + 4;
            int charsEnd   = charsStart + maybeLen * 4;
            if (charsEnd > data.length) continue;
            boolean allPrintable = true;
            for (int j = charsStart; j < charsEnd; j += 4) {
                int v = readBigEndianInt(data, j);
                if (v < 0x20 || v > 0x7E) { allPrintable = false; break; }
            }
            if (allPrintable && maybeLen >= 3) {
                detectedStrings.add(new int[] { i, maybeLen, charsStart, charsEnd });
            }
        }
        for (int i = 0; i < data.length; i += 4) {
            int rem = Math.min(4, data.length - i);
            StringBuilder hex = new StringBuilder();
            for (int j = 0; j < 4; j++) {
                if (j > 0) hex.append(' ');
                if (j < rem) hex.append(String.format("%02X", data[i + j] & 0xFF));
                else hex.append("  ");
            }
            String interp;
            if (rem == 4) {
                int v = readBigEndianInt(data, i);
                float fv = Float.intBitsToFloat(v);
                String charPart = (v >= 32 && v < 127)
                        ? "  char='" + (char) v + "'"
                        : "";
                String floatPart = (!Float.isNaN(fv) && !Float.isInfinite(fv) && Math.abs(fv) > 1e-6f && Math.abs(fv) < 1e9f)
                        ? String.format("  ~f32=%g", fv) : "";
                interp = String.format("int=%-11d 0x%08X%s%s", v, v, charPart, floatPart);
            } else {
                interp = "(parcial)";
            }
            out.printf("  +%04d:  %s    %s%n", i, hex, interp);
        }
        if (!detectedStrings.isEmpty()) {
            out.printf("  -- Detected string literals (heurística: [len:i32][ch:i32]*) --%n");
            for (int[] s : detectedStrings) {
                int lenAt = s[0], n = s[1], charsStart = s[2], charsEnd = s[3];
                StringBuilder text = new StringBuilder();
                for (int j = charsStart; j < charsEnd; j += 4) {
                    text.append((char) readBigEndianInt(data, j));
                }
                out.printf("  +%04d:  STRING(len=%d) \"%s\"  [chars +%04d..+%04d]%n",
                        lenAt, n, escapeForDisplay(text.toString()),
                        charsStart, charsEnd - 4);
            }
        }
    }

    private static String escapeForDisplay(String s) {
        StringBuilder sb = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '\\': sb.append("\\\\"); break;
                case '"':  sb.append("\\\""); break;
                default:   sb.append(c);
            }
        }
        return sb.toString();
    }

    private static int readBigEndianInt(byte[] b, int off) {
        return ((b[off]     & 0xFF) << 24)
             | ((b[off + 1] & 0xFF) << 16)
             | ((b[off + 2] & 0xFF) <<  8)
             |  (b[off + 3] & 0xFF);
    }

    private static void dumpCode(byte[] code, Map<Integer, String> exports,
                                 List<ImportEntry> imports, PrintStream out) {
        if (code.length == 0) {
            out.println("  (vacío)");
            return;
        }
        int pc = 0;
        while (pc < code.length) {
            String funcName = exports.get(pc);
            if (funcName != null) {
                out.printf("%n  ; --- %s() ---%n", funcName);
            }

            int instAddr = pc;
            byte rawOp = code[pc];
            pc++;

            OpCode op;
            try {
                op = OpCode.fromByte(rawOp);
            } catch (IllegalArgumentException ex) {
                out.printf("  +%04d  %02X                            ??? opcode desconocido%n",
                        instAddr, rawOp & 0xFF);
                continue;
            }

            OperandKind kind = op.operandKind;

            StringBuilder hex = new StringBuilder(String.format("%02X", rawOp & 0xFF));
            long rawOperand = 0;
            for (int j = 0; j < kind.bytes; j++) {
                if (pc + j >= code.length) {
                    hex.append(" ??");
                    continue;
                }
                byte b = code[pc + j];
                hex.append(' ').append(String.format("%02X", b & 0xFF));
                rawOperand = (rawOperand << 8) | (b & 0xFF);
            }

            String operandStr = formatOperand(op, kind, rawOperand, instAddr, exports, imports);
            pc += kind.bytes;

            String hexCol = String.format("%-15s", hex.toString());
            out.printf("  +%04d  %s %-16s %s%n", instAddr, hexCol, op.name(), operandStr);
        }
    }

    private static String formatOperand(OpCode op, OperandKind kind, long raw,
                                        int instAddr, Map<Integer, String> exports,
                                        List<ImportEntry> imports) {
        switch (kind) {
            case NONE:
                return "";
            case IMM_I32: {
                int v = (int) raw;
                return String.format("%d", v);
            }
            case IMM_F32: {
                int bits = (int) raw;
                float f = Float.intBitsToFloat(bits);
                return String.format("%g  (bits=0x%08X)", f, bits);
            }
            case IMM_U8: {
                /* El significado del byte depende del opcode. Antes la
                 * anotación era "; paramsCount" genérica → engañosa para
                 * GET_FIELD/SET_FIELD. #163 lo desglosa: */
                int v = (int) (raw & 0xFF);
                switch (op) {
                    case RET:
                        return String.format("%d  ; nArgs (palabras a popear tras RET)", v);
                    case GET_FIELD: case SET_FIELD: case SET_FIELD_OWNER:
                        return String.format("%d  ; field idx", v);
                    default:
                        return String.format("%d", v);
                }
            }
            case SOFF_I8: {
                int v = (byte) raw;          // sign-extend
                String base;
                switch (op) {
                    case GET_GLOBAL_S8: case SET_GLOBAL_S8: case LEA_GLOBAL_S8:
                        base = "CS";
                        break;
                    case GET_LOCAL_S8: case SET_LOCAL_S8: case LEA_LOCAL_S8:
                        base = "BP";
                        break;
                    default:
                        base = "?";
                }
                return String.format("%+d  ; [%s%+d]", v, base, v);
            }
            case SOFF_I16: {
                short v = (short) raw;
                String base;
                switch (op) {
                    case GET_GLOBAL: case SET_GLOBAL: case LEA_GLOBAL:
                        base = "CS";
                        break;
                    case GET_LOCAL: case SET_LOCAL: case LEA_LOCAL:
                        base = "BP";
                        break;
                    case NEW_OBJECT: case INSTANCEOF:
                        base = "class@CS";
                        break;
                    default:
                        base = "?";
                }
                return String.format("%+d  ; [%s%+d]", v, base, v);
            }
            case IDX_U16: {
                /* Antes el operand siempre se interpretaba contra `imports`
                 * — pero CALL_BUILTIN indexa el catálogo Builtin, no la
                 * import-table. #163 lo separa según opcode. */
                int idx = (int) (raw & 0xFFFF);
                if (op == OpCode.CALL_BUILTIN) {
                    String bname = builtinNameOf(idx);
                    return String.format("[%d]  ; %s", idx, bname);
                }
                /* CALL_EXT y resto que usen IDX_U16 sí apuntan a imports. */
                String name = (idx >= 0 && idx < imports.size())
                        ? imports.get(idx).toString() : "?";
                return String.format("[%d]  ; %s", idx, name);
            }
            case SIZE_U16: {
                int sz = (int) (raw & 0xFFFF);
                return String.format("%d  ; bytes de locales", sz);
            }
            case CALL_TGT_I32: {
                int target = (int) raw;
                String name = exports.get(target);
                return name != null
                        ? String.format("@%d  ; %s()", target, name)
                        : String.format("@%d", target);
            }
            case JUMP_REL_I32: {
                int rel = (int) raw;
                int absTarget = instAddr + rel;
                return String.format("%+d  ; -> @%d", rel, absTarget);
            }
            case JUMP_REL_I16: {
                int rel = (short) raw;
                int absTarget = instAddr + rel;
                return String.format("%+d  ; -> @%d", rel, absTarget);
            }
            case JUMP_REL_I8: {
                int rel = (byte) raw;
                int absTarget = instAddr + rel;
                return String.format("%+d  ; -> @%d", rel, absTarget);
            }
            case SLOT_NUMARGS_U8U8: {
                /* INVOKE_VIRTUAL — slot (u8) | numArgs (u8). */
                int slot    = (int) ((raw >> 8) & 0xFF);
                int numArgs = (int) ( raw       & 0xFF);
                return String.format("slot=%d args=%d", slot, numArgs);
            }
            case TRY_HANDLER_I32_I16: {
                /* TRY_BEGIN — handler offset (i32 rel. al TRY_BEGIN) | cs_off (i16, 0=any). */
                int rel    = (int)  ((raw >> 16) & 0xFFFFFFFFL);
                int csOff  = (short)( raw        & 0xFFFF);
                int absHnd = instAddr + rel;
                String catchPart = (csOff == 0) ? "any"
                        : String.format("class@CS%+d", csOff);
                return String.format("handler=%+d (-> @%d), catch=%s",
                        rel, absHnd, catchPart);
            }
            default:
                return "?";
        }
    }

    /* ============================================================ */
    /*  Helpers añadidos por #163                                    */
    /* ============================================================ */

    /** Nombre del builtin con id `idx`, o "?" si fuera de rango. */
    private static String builtinNameOf(int idx) {
        Builtin[] all = Builtin.values();
        if (idx < 0 || idx >= all.length) return "?";
        return all[idx].bpName;
    }

    /* ============================================================ */
    /*  Companion .mdn parser (#163)                                 */
    /*  Spec: bpgenvm-c/src/mdn_format.h. Big-endian.                */
    /*    [magic[4]="MDN\0"][version:u16][abi:u16]                   */
    /*    [code_size:u32][sym_count:u32][_reserved:u32]              */
    /*    N × { name[32 bytes nul-padded] , thunk_offset:u32 }       */
    /*    code (code_size bytes — Thumb-2 PIC)                       */
    /* ============================================================ */

    private static final int MDN_HEADER_SIZE = 20;
    private static final int MDN_NAME_MAX    = 32;
    private static final int MDN_SYM_SIZE    = MDN_NAME_MAX + 4;

    private static final class MdnSymbol {
        final String name;
        final int    thunkOffset;
        MdnSymbol(String name, int thunkOffset) {
            this.name = name;
            this.thunkOffset = thunkOffset;
        }
    }

    private static final class MdnInfo {
        final String path;
        final int version;
        final int abiVersion;
        final int codeSize;
        final List<MdnSymbol> symbols;
        MdnInfo(String path, int version, int abiVersion, int codeSize,
                List<MdnSymbol> symbols) {
            this.path = path;
            this.version = version;
            this.abiVersion = abiVersion;
            this.codeSize = codeSize;
            this.symbols = symbols;
        }
        /** ¿Existe un thunk AOT para el export `exportName` de este módulo?
         *  Los nombres en .mdn son "qualified" (Mod.func o Lib.Mod.func);
         *  los exports del .mod son "func". Componemos el qualified con
         *  el library prefix del .mod y comparamos. */
        boolean hasSymbol(String exportName, String library) {
            if (exportName == null) return false;
            String prefix1 = (library == null || library.isEmpty())
                    ? "" : library + ".";
            for (MdnSymbol s : symbols) {
                if (s.name.equals(exportName)) return true;            // raw
                if (s.name.endsWith("." + exportName)) return true;    // Mod.f / Lib.Mod.f
                if (s.name.equals(prefix1 + exportName)) return true;  // exacto con prefix
            }
            return false;
        }
    }

    /** Devuelve la información del companion .mdn (mismo basename, ext
     *  .mdn) si existe y parsea bien. Null si no existe; lanza
     *  IllegalArgumentException si existe pero el formato no cuadra
     *  (preferible a "silenciosamente sin AOT info").
     *
     *  Endian: el .mdn es LITTLE-endian (cf. {@code MdnInspect}; el
     *  comentario "big-endian" de {@code mdn_format.h} es incorrecto —
     *  el writer real usa {@code ByteBuffer.LITTLE_ENDIAN}). */
    private static MdnInfo tryLoadMdnCompanion(String modPath, String library) {
        File modFile = new File(modPath);
        String name = modFile.getName();
        if (!name.toLowerCase().endsWith(".mod")) return null;
        String base = name.substring(0, name.length() - 4);
        File mdnFile = new File(modFile.getParentFile(), base + ".mdn");
        if (!mdnFile.isFile()) return null;

        try {
            byte[] all = Files.readAllBytes(mdnFile.toPath());
            if (all.length < MDN_HEADER_SIZE) {
                throw new IOException("tamaño < header (" + all.length + ")");
            }
            ByteBuffer bb = ByteBuffer.wrap(all).order(ByteOrder.LITTLE_ENDIAN);
            if (all[0] != 'M' || all[1] != 'D' || all[2] != 'N' || all[3] != 0) {
                throw new IOException("magic MDN no coincide");
            }
            int version    = bb.getShort(4) & 0xFFFF;
            int abiVersion = bb.getShort(6) & 0xFFFF;
            int codeSize   = bb.getInt(8);
            int symCount   = bb.getInt(12);
            // bb.getInt(16) = _reserved (ignorado)
            long needed = (long) MDN_HEADER_SIZE
                        + (long) symCount * MDN_SYM_SIZE
                        + (long) codeSize;
            if (all.length < needed) {
                throw new IOException("archivo truncado: esperaba " + needed
                        + " bytes, tengo " + all.length);
            }
            List<MdnSymbol> syms = new ArrayList<>(symCount);
            for (int i = 0; i < symCount; i++) {
                int symOff = MDN_HEADER_SIZE + i * MDN_SYM_SIZE;
                int nameLen = 0;
                while (nameLen < MDN_NAME_MAX && all[symOff + nameLen] != 0) nameLen++;
                String n = new String(all, symOff, nameLen, StandardCharsets.UTF_8);
                int off = bb.getInt(symOff + MDN_NAME_MAX);
                syms.add(new MdnSymbol(n, off));
            }
            return new MdnInfo(mdnFile.getPath(), version, abiVersion,
                                codeSize, syms);
        } catch (IOException ioe) {
            throw new IllegalArgumentException(
                    "fallo leyendo companion .mdn: " + ioe.getMessage(), ioe);
        }
    }
}
