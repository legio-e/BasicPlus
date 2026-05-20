package edu.bpgenvm.vm;

import java.util.ArrayList;
import java.util.List;

/**
 * Snapshot del estado del thread BP en el instante en que el VM llama a
 * {@link DebugHook#onLineChange(DebugContext)}. Inmutable: bp/sp/cs/pc
 * apuntan al momento de la pausa. Helpers para leer locales y reconstruir
 * la pila de frames.
 *
 * <p>El thread BP está bloqueado durante toda la llamada al hook, así que
 * todo lectura aquí es consistente.</p>
 */
public final class DebugContext {
    /** Id del ThreadContext BP (0 = main). */
    public final int tid;
    /** PC absoluto del próximo opcode a ejecutar. */
    public final int absPc;
    /** Línea origen 1-based del .bp, o -1 si el módulo no tiene .dbg. */
    public final int line;
    /** Path al .bp original (del .dbg), o null si no se conoce. */
    public final String sourceFile;
    /** Registros BP en el momento de la pausa. */
    public final int bp, sp, cs;
    /** Dirección más baja de la región de pila del thread. */
    public final int stackBase;

    private final VirtualMachine vm;

    DebugContext(VirtualMachine vm, int tid, int absPc, int line, String sourceFile,
                 int bp, int sp, int cs, int stackBase) {
        this.vm         = vm;
        this.tid        = tid;
        this.absPc      = absPc;
        this.line       = line;
        this.sourceFile = sourceFile;
        this.bp         = bp;
        this.sp         = sp;
        this.cs         = cs;
        this.stackBase  = stackBase;
    }

    /** Lee un i32 en una dirección absoluta de memoria. */
    public int readInt(int addr) { return vm.readInt32(addr); }

    /** Lee el i32 del slot local en offset {@code +/-} desde bp. */
    public int readLocal(int offset) { return vm.readInt32(bp + offset); }

    /**
     * Recorre la cadena de frames guardada en la pila (saved PC en bp-12,
     * saved BP en bp-8, saved CS en bp-4). Cada entry del resultado es
     * {@code [pc, bp]}. La primera entry corresponde al frame actual
     * (innermost); la última al frame raíz.
     */
    public List<int[]> stackFrames() {
        List<int[]> frames = new ArrayList<>();
        int curBp = bp;
        int curPc = absPc;
        int safety = 0;
        while (curBp > stackBase && safety < 256) {
            frames.add(new int[]{curPc, curBp});
            curPc = vm.readInt32(curBp - 12);
            curBp = vm.readInt32(curBp - 8);
            safety++;
        }
        frames.add(new int[]{curPc, curBp});   // frame top-level
        return frames;
    }

    /**
     * Snapshot de las properties públicas de TODOS los módulos cargados
     * (lectura directa del data block — sin ejecutar bytecode). Vacía si
     * ningún módulo trae info en su .dbg v2. Consumido por el IDE para
     * rellenar la sección "Properties módulo" del panel de variables.
     */
    public List<ModuleManager.PropertyView> moduleProperties() {
        ModuleManager mm = vm.getModuleManager();
        if (mm == null) return new ArrayList<>();
        return mm.snapshotAllProperties();
    }
}
