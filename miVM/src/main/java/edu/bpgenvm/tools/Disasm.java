/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.tools;

/**
 * Disassembler de ficheros .mod (formato v5).
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.bytecode.OpCode.OperandKind;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
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
            out.println();
            out.printf ("=== EXPORTS (%d) ===%n", exportsByOffset.size());
            if (exportsByOffset.isEmpty()) out.println("  (ninguno)");
            for (Map.Entry<Integer, String> e : exportsByOffset.entrySet()) {
                out.printf("  %-25s @code +%d%n", e.getValue(), e.getKey());
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
                int v = (int) (raw & 0xFF);
                return String.format("%d  ; paramsCount", v);
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
                    default:
                        base = "?";
                }
                return String.format("%+d  ; [%s%+d]", v, base, v);
            }
            case IDX_U16: {
                int idx = (int) (raw & 0xFFFF);
                String name = (idx >= 0 && idx < imports.size()) ? imports.get(idx).toString() : "?";
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
            default:
                return "?";
        }
    }
}
