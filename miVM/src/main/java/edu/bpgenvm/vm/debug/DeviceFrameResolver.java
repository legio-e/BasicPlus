package edu.bpgenvm.vm.debug;

import edu.bpgenvm.vm.ModuleManager;

import java.util.ArrayList;
import java.util.List;

/**
 * H6.b.3 — Resuelve el frame CRUDO de una VM device-role (server wire C / Pico)
 * a símbolos, usando el {@link DbgFile} que el host tiene. Es la traducción
 * "host hace el trabajo simbólico" de la regla de oro: el device sólo da
 * pc/bp/memoria crudos; aquí se aplican los nombres, tipos y el render.
 *
 * <p>El acceso a memoria se inyecta vía {@link MemReader} — en producción son
 * los comandos {@code READ_INT}/{@code READ_STRING} del wire; en tests, un fake.
 * Así el resolver es puro y host-testeable sin sockets ni GUI.</p>
 *
 * <p>Para cada variable: valor = lectura en {@code bp + offset} (offset con
 * signo: negativo = param, positivo = local; long/double leen 2 slots
 * big-endian, idéntico a la VM). El {@code display} se renderiza por tipo, igual
 * que hace {@code DebugServer} en el caso VM-Java (string→texto, double→f64,
 * etc.), pero AQUÍ, del lado del host.</p>
 */
public final class DeviceFrameResolver {

    private DeviceFrameResolver() {}

    /** Acceso a la memoria del device (= comandos READ_INT/READ_STRING del wire). */
    public interface MemReader {
        int    readI32(int addr);     // READ_INT{addr}  → i32 big-endian del device
        String readString(int ref);   // READ_STRING{ref} → texto, o null si nula
    }

    /** Una variable resuelta: nombre + tipo + valor crudo + texto listo. */
    public static final class Local {
        public final String name, type, display;
        public final long   value;
        public final int    offset;
        Local(String name, String type, long value, int offset, String display) {
            this.name = name; this.type = type; this.value = value;
            this.offset = offset; this.display = display;
        }
    }

    /** El frame resuelto: función + línea + locales por nombre. */
    public static final class Frame {
        public final String      function;   // null si el pc no cae en ninguna función del .dbg
        public final int         line;        // -1 si no hay info
        public final List<Local> locals;
        Frame(String function, int line, List<Local> locals) {
            this.function = function; this.line = line; this.locals = locals;
        }
    }

    /**
     * Resuelve el frame en {@code relPc} (= pc_absoluto - cs, que el device
     * envía en BP_HIT) con base {@code bp}, leyendo memoria vía {@code r}.
     */
    public static Frame resolve(DbgFile dbg, int relPc, int bp, MemReader r) {
        int line = dbg.lineForRelPc(relPc);
        ModuleManager.FunctionVars fv = dbg.functionForRelPc(relPc);
        List<Local> out = new ArrayList<>();
        if (fv == null) return new Frame(null, line, out);
        for (ModuleManager.LocalVarDescriptor v : fv.vars) {
            long value;
            if (v.sizeBytes == 8) {
                // long/double i64 big-endian: high word en offset, low en offset+4
                value = ((long) r.readI32(bp + v.offset) << 32)
                      | ((long) r.readI32(bp + v.offset + 4) & 0xFFFFFFFFL);
            } else {
                value = r.readI32(bp + v.offset);
            }
            out.add(new Local(v.name, v.type, value, v.offset, render(v, value, r)));
        }
        return new Frame(fv.name, line, out);
    }

    /** Render por tipo (= DebugServer.renderLocalDisplay, pero del lado host). */
    private static String render(ModuleManager.LocalVarDescriptor v, long value, MemReader r) {
        if (v.isArray) return "array[len=" + value + "]";  // array local inline: el slot guarda la longitud
        switch (v.type) {
            case "integer": return Integer.toString((int) value);
            case "long":    return Long.toString(value);
            case "float":   return Float.toString(Float.intBitsToFloat((int) value));
            case "double":  return Double.toString(Double.longBitsToDouble(value));
            case "boolean": return (value != 0) ? "true" : "false";
            case "string": {
                if (value == 0) return "null";
                String s = r.readString((int) value);
                return (s != null) ? ("\"" + s + "\"") : ("@" + value);
            }
            case "ref":     return (value == 0) ? "null" : ("@" + value);
            default:        return Long.toString(value);   // "?" / desconocido → crudo
        }
    }
}
