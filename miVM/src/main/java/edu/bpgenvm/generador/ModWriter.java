/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.generador;

/**
 * Generador de bytecode .mod (formato v3) con soporte para i32, f32 y
 * tipos enteros estrechos (i8/u8/i16/u16) en memoria densa.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.OpCode;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ModWriter {

    private final ByteArrayOutputStream importStream = new ByteArrayOutputStream();
    private final ByteArrayOutputStream exportStream = new ByteArrayOutputStream();
    private final ByteArrayOutputStream codeStream   = new ByteArrayOutputStream();

    private final DataOutputStream importOut = new DataOutputStream(importStream);
    private final DataOutputStream exportOut = new DataOutputStream(exportStream);
    private final DataOutputStream codeOut   = new DataOutputStream(codeStream);

    /** Cada external es (nombreLógico, fromPath). fromPath="" ⇒ usa convención por defecto. */
    private static class ExternalRef {
        final String name;
        final String fromPath;
        ExternalRef(String name, String fromPath) {
            this.name = name;
            this.fromPath = (fromPath == null) ? "" : fromPath;
        }
    }
    private final List<ExternalRef> externalFunctions = new ArrayList<>();
    private final Map<String, Integer> functionAddresses = new HashMap<>();
    private final List<String> exportFunctions = new ArrayList<>();

    /** B3 v2 — Data symbols exportados con nombre. Hoy se usa SÓLO para
     *  exponer el class_ptr de `RuntimeError` (que el emisor sintetiza en
     *  cada módulo) al runtime, para que builtins nativos puedan
     *  instanciar la clase sin tener que parsear el data block.
     *
     *  Cada entrada se serializa como pareja (name, csOffset) AL FINAL de
     *  la sección exports — el loader detecta la subsección por bytes
     *  remanentes del exportsSize cabecera y mantiene compat con .mods
     *  viejos (que no la tendrán). */
    private final List<String> exportDataSymbols = new ArrayList<>();

    private final List<CallFixup> callFixups = new ArrayList<>();

    private final List<String> currentParams = new ArrayList<>();
    private final List<Integer> currentParamSizes = new ArrayList<>();  // H1.2: ancho por param (4/8)

    private static class LocalSlot {
        String name;
        int offsetBytes;
        int sizeBytes;
        boolean isArray;
        int arrayLength;
    }
    private final List<LocalSlot> currentLocalSlots = new ArrayList<>();
    private int currentLocalsBytes = 0;

    /**
     * H6.a.1 — descriptor de una variable (param o local) de una función, para
     * el `.dbg`. {@code offset} es el desplazamiento (con signo) desde {@code bp}:
     * negativo = param (debajo de bp/saved-regs), positivo = local. El host suma
     * {@code bp + offset} para leer el slot y mostrar la variable por nombre. NO
     * altera el {@code .mod}.
     */
    public static final class VarDebugInfo {
        public final String name;
        public final int offset;      // con signo, relativo a bp
        public final int sizeBytes;   // 4 (i32/ref) · 8 (long/double) · 4+len*4 (array)
        public final boolean isArray;
        public VarDebugInfo(String name, int offset, int sizeBytes, boolean isArray) {
            this.name = name; this.offset = offset; this.sizeBytes = sizeBytes; this.isArray = isArray;
        }
    }

    // --- Soporte de clases ---
    private static class FieldInfo {
        String name;
        boolean isRef;
        boolean isOwner;   // true ⇒ al liberar la instancia, liberar también este campo
        int slot;          // BUG-6: índice de slot (4 bytes/slot) dentro de la instancia
        boolean is8;       // BUG-6: campo de 8 bytes (long/double) → ocupa 2 slots
    }
    private static class MethodInfo {
        String simpleName;       // "hablar"
        String qualifiedName;    // "Animal.hablar" — clave en functionAddresses
        int slot;                // posición en la vtable
    }
    private static class ClassInfo {
        String name;
        String parent;           // null si no hereda; nombre LOCAL si parent del mismo módulo.
        final List<FieldInfo> fields = new ArrayList<>();
        final List<MethodInfo> methods = new ArrayList<>(); // ordenadas por slot
        // L2 v3 — herencia cross-module. Si parent vive en otro módulo, el
        // ModWriter local no tiene su descriptor; se cargan placeholders (campos
        // dummy + slots vacíos) sólo para preservar el slot/field numbering y
        // el bitmap del GC. El parentOff en el descriptor se escribe a 0 al
        // emitir y se PARCHA en runtime vía la tabla classFixups (resuelto por
        // ModuleManager.linkAll cuando ya están todos los módulos cargados).
        boolean externalParent;
        String externalParentQualified; // p.ej. "L2Lib.Counter", lookup en globalSymbolTable
    }
    private final Map<String, ClassInfo> classes = new HashMap<>();
    private ClassInfo currentClass = null;

    /** L2 v3 — fixup que el loader aplica para parchear parentOff de una clase
     *  cuyo padre vive cross-module. childClassName es el data symbol del child
     *  (registrado en este .mod); parentQualified es el nombre cualificado del
     *  parent data symbol exportado por su módulo dueño (e.g. "L2Lib.Counter"). */
    private static class ClassFixup {
        String childClassName;
        String parentQualified;
    }
    private final List<ClassFixup> classFixups = new ArrayList<>();

    // --- Soporte de properties ---
    private enum PropertyScope { MODULE, INSTANCE, STATIC }

    private static class PropertyInfo {
        String simpleName;              // p.ej. "x"
        String classOwner;              // null para MODULE; nombre de la clase para INSTANCE/STATIC
        PropertyScope scope;
        boolean isPublic;               // false ⇒ privada (no en exports + check de clase)
        String getterFunctionName;      // p.ej. "getX" o "Punto.getX"
        String setterFunctionName;
    }
    private final Map<String, PropertyInfo> moduleProperties   = new HashMap<>(); // key = simpleName
    private final Map<String, Map<String, PropertyInfo>> instanceProperties = new HashMap<>(); // [Clase][simpleName]
    private final Map<String, Map<String, PropertyInfo>> staticProperties   = new HashMap<>(); // [Clase][simpleName]

    private static final int UNDECLARED = -1;
    private Map<Integer, Integer> currentLabelMap = new HashMap<>();
    private int nextLabelId = 0;
    private final Map<String, Map<Integer, Integer>> functionLabelMaps = new HashMap<>();

    private final List<byte[]> symbolBytes = new ArrayList<>();
    private final Map<String, Integer> dataSymbolOffset = new HashMap<>();
    private final java.util.Set<String> longGlobals = new java.util.HashSet<>();  // H1.2: globals long (8 bytes)
    private int currentNegativeOffset = 0;

    private int currentBytecodeSize = 0;
    private String lastFunctionName = null;

    private final Map<String, Integer> functionEnterOperandPos = new HashMap<>();
    private final Map<String, Integer> functionLocalsBytesMap = new HashMap<>();

    // H6.a.1 (.dbg v3) — info de depuración de variables por función:
    // params + locales user-visible (los sintéticos `__*` se filtran). El .mod
    // NO cambia; esto se vuelca al .dbg separado para que el host resuelva
    // locales por nombre. LinkedHashMap conserva el orden de declaración de
    // funciones para un .dbg estable y diffeable.
    private final Map<String, List<VarDebugInfo>> functionVarsDbg = new java.util.LinkedHashMap<>();

    private final List<JumpFixup> pendingJumps = new ArrayList<>();

    public static class CallFixup {
        String targetFunction;
        int bytecodeIndex;
        public CallFixup(String targetFunction, int bytecodeIndex) {
            this.targetFunction = targetFunction;
            this.bytecodeIndex  = bytecodeIndex;
        }
    }

    private static class JumpFixup {
        String functionName;
        int labelId;
        int instructionAddress;
        int operandIndex;
        JumpFixup(String functionName, int labelId, int instructionAddress, int operandIndex) {
            this.functionName = functionName;
            this.labelId = labelId;
            this.instructionAddress = instructionAddress;
            this.operandIndex = operandIndex;
        }
    }

    // --- Ciclo de vida del módulo ---

    /** Nombre lógico del módulo actual (e.g., "Fase1"). Útil para construir
     *  el nombre del fichero ".mod" con o sin prefix de library. */
    private String currentModuleName = "";
    /** Library del módulo actual. Vacía si no se ha llamado setLibrary. */
    private String currentLibrary = "";
    /**
     * Nombre de la función que la VM debe ejecutar al cargar este módulo
     * como root. Si es null, se usa "main" por compatibilidad legacy.
     * El nombre apunta a una función ya registrada en functionAddresses.
     */
    private String mainEntryFunction = null;

    /**
     * Offset relativo al inicio de la sección de código del próximo byte
     * que se va a emitir. Lo usa el emisor para construir el mapeo
     * (relPc → línea origen) que va al fichero .dbg.
     */
    public int getCurrentBytecodeOffset() {
        return currentBytecodeSize;
    }

    public void addModulo(String name) {
        importStream.reset(); exportStream.reset(); codeStream.reset();
        externalFunctions.clear(); functionAddresses.clear(); exportFunctions.clear();
        symbolBytes.clear();
        dataSymbolOffset.clear();
        currentNegativeOffset = 0;
        currentBytecodeSize = 0;
        lastFunctionName = null;
        callFixups.clear();
        functionEnterOperandPos.clear();
        functionLocalsBytesMap.clear();
        functionVarsDbg.clear();
        functionLabelMaps.clear();
        pendingJumps.clear();
        classes.clear();
        currentClass = null;
        moduleProperties.clear();
        instanceProperties.clear();
        staticProperties.clear();
        clearFunctionScope();
        currentModuleName = name;
        currentLibrary = "";
        mainEntryFunction = null;
    }

    /** Asocia el módulo actual con una library (etiqueta string). El nombre
     *  del fichero ".mod" pasa a ser "library.module.mod" (o "module.mod" si
     *  library está vacío). Debe llamarse después de addModulo y antes de
     *  writeToFile. */
    public void setLibrary(String library) {
        this.currentLibrary = (library == null) ? "" : library;
    }

    /**
     * Establece qué función registrada (por nombre) sirve como entry-point
     * cuando este módulo es el root. mainOffset en el header apuntará a
     * esa función. Debe llamarse antes de writeToFile y la función ya tiene
     * que haber sido emitida (addFunction).
     */
    public void setMainEntry(String funcName) {
        if (funcName != null && !functionAddresses.containsKey(funcName)) {
            throw new RuntimeException("setMainEntry: función no registrada: " + funcName);
        }
        this.mainEntryFunction = funcName;
    }

    /** Devuelve el nombre canónico del fichero ".mod" según library + module. */
    public String getCanonicalFilename() {
        if (currentLibrary == null || currentLibrary.isEmpty()) {
            return currentModuleName + ".mod";
        }
        return currentLibrary + "." + currentModuleName + ".mod";
    }

    private void clearFunctionScope() {
        currentLabelMap = new HashMap<>();
        nextLabelId = 0;
        currentParams.clear();
        currentParamSizes.clear();
        currentLocalSlots.clear();
        currentLocalsBytes = 0;
    }

    /**
     * H6.a.1 — captura el mapa var→offset de la función {@code funcName} (la que
     * tiene el scope cargado AHORA) para el `.dbg`. Debe llamarse ANTES de
     * {@link #clearFunctionScope()}. Filtra los temporales sintéticos del
     * compilador (prefijo {@code __}) para no ensuciar el panel de variables;
     * conserva params (incluido {@code this}), locales y los iteradores de bucle
     * (que usan el nombre de origen del usuario).
     */
    private void captureFunctionVarsDbg(String funcName) {
        if (funcName == null) return;
        List<VarDebugInfo> vars = new ArrayList<>();
        for (int i = 0; i < currentParams.size(); i++) {
            String pn = currentParams.get(i);
            if (pn.startsWith("__")) continue;
            vars.add(new VarDebugInfo(pn, paramOffset(i), currentParamSizes.get(i), false));
        }
        for (LocalSlot s : currentLocalSlots) {
            if (s.name.startsWith("__")) continue;
            vars.add(new VarDebugInfo(s.name, s.offsetBytes, s.sizeBytes, s.isArray));
        }
        if (!vars.isEmpty()) functionVarsDbg.put(funcName, vars);
    }

    /** H6.a.1 — vars (params+locales) por función para el `.dbg`. Solo lectura. */
    public Map<String, List<VarDebugInfo>> getFunctionVarsDebug() { return functionVarsDbg; }

    /** H6.a.1 — relPc de inicio (OP_ENTER) de la función, o null si no existe. */
    public Integer getFunctionStartRel(String name) { return functionAddresses.get(name); }

    public void addFunction(String name, boolean isPublic) {
        if (lastFunctionName != null) {
            functionLocalsBytesMap.put(lastFunctionName, currentLocalsBytes);
            functionLabelMaps.put(lastFunctionName, currentLabelMap);
            captureFunctionVarsDbg(lastFunctionName);
        }

        functionAddresses.put(name, currentBytecodeSize);
        if (isPublic) exportFunctions.add(name);

        lastFunctionName = name;
        clearFunctionScope();

        try {
            codeOut.writeByte(OpCode.ENTER.code);
            functionEnterOperandPos.put(name, currentBytecodeSize + 1);
            codeOut.writeShort((short) 0);
            currentBytecodeSize += 3;
        } catch (IOException e) {
            throw new RuntimeException("Error emitiendo prólogo OP_ENTER: " + e.getMessage(), e);
        }
    }

    // --- Data block ---

    /** B3 v2 — registra `name` (un símbolo de datos previamente registrado)
     *  para ser exportado en el .mod. El loader lo cargará en su tabla de
     *  símbolos globales con la dirección absoluta del descriptor. */
    public void exportDataSymbol(String name) {
        if (!dataSymbolOffset.containsKey(name)) {
            throw new RuntimeException("exportDataSymbol: símbolo '" + name + "' no registrado");
        }
        exportDataSymbols.add(name);
    }

    private int registerSymbol(String name, byte[] bytes) {
        requireUnique(name);
        int size = bytes.length;
        int csOffset = currentNegativeOffset - size;
        if (csOffset < Short.MIN_VALUE) {
            throw new RuntimeException("Offset CS fuera de rango short al declarar '"
                    + name + "': " + csOffset + " (data block excede 32 KB)");
        }
        symbolBytes.add(bytes);
        dataSymbolOffset.put(name, csOffset);
        currentNegativeOffset = csOffset;
        return csOffset;
    }

    private void requireUnique(String name) {
        if (dataSymbolOffset.containsKey(name)) {
            throw new RuntimeException("Símbolo de datos ya existe: " + name);
        }
    }

    public int addConstantInt(String name, int value) {
        byte[] b = new byte[4];
        writeIntInBuf(b, 0, value);
        return registerSymbol(name, b);
    }

    public void declareGlobal(String name) {
        if (dataSymbolOffset.containsKey(name)) return;
        addConstantInt(name, 0);
    }

    // H1.2 (V2): global long = 8 bytes (zero-init). emitGet/SetGlobal detectan
    // que es long y emiten GET_GLOBAL_L/SET_GLOBAL_L.
    public void declareGlobalLong(String name) {
        if (dataSymbolOffset.containsKey(name)) return;
        registerSymbol(name, new byte[8]);
        longGlobals.add(name);
    }

    /**
     * Devuelve el offset (relativo al CS del módulo) de un símbolo del data
     * block ya registrado. Sirve para emitir info de debug (.dbg) que el
     * runtime usa para leer globals por nombre sin ejecutar bytecode.
     * Devuelve null si el símbolo no existe.
     */
    public Integer getDataSymbolOffset(String name) {
        return dataSymbolOffset.get(name);
    }

    public int addConstantArray(String name, int[] values) {
        byte[] b = new byte[4 + values.length * 4];
        writeIntInBuf(b, 0, values.length);
        for (int i = 0; i < values.length; i++) writeIntInBuf(b, 4 + i * 4, values[i]);
        return registerSymbol(name, b);
    }

    public int addGlobalArray(String name, int length) {
        if (length < 0) throw new RuntimeException("Tamaño negativo en addGlobalArray(" + name + "): " + length);
        byte[] b = new byte[4 + length * 4];
        writeIntInBuf(b, 0, length);
        return registerSymbol(name, b);
    }

    public int addConstantString(String name, String value) {
        // H2 (V2): strings son byte[] UTF-8. El literal en el data block es
        // [u32 byte_len][bytes UTF-8]. length = nº de bytes (no codepoints).
        byte[] utf8 = value.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] b = new byte[4 + utf8.length];
        writeIntInBuf(b, 0, utf8.length);
        System.arraycopy(utf8, 0, b, 4, utf8.length);
        return registerSymbol(name, b);
    }

    public int addConstantFloat(String name, float value) {
        byte[] b = new byte[4];
        writeIntInBuf(b, 0, Float.floatToRawIntBits(value));
        return registerSymbol(name, b);
    }

    public int addConstantArrayFloat(String name, float[] values) {
        byte[] b = new byte[4 + values.length * 4];
        writeIntInBuf(b, 0, values.length);
        for (int i = 0; i < values.length; i++) {
            writeIntInBuf(b, 4 + i * 4, Float.floatToRawIntBits(values[i]));
        }
        return registerSymbol(name, b);
    }

    // --- Tipos enteros estrechos en data block ---

    public int addConstantInt8(String name, int value) {
        byte[] b = new byte[]{ (byte) (value & 0xFF) };
        return registerSymbol(name, b);
    }

    public int addConstantInt16(String name, int value) {
        byte[] b = new byte[]{
            (byte) ((value >> 8) & 0xFF),
            (byte) ( value       & 0xFF)
        };
        return registerSymbol(name, b);
    }

    public int addConstantArrayInt8(String name, byte[] values) {
        byte[] b = new byte[4 + values.length];
        writeIntInBuf(b, 0, values.length);
        System.arraycopy(values, 0, b, 4, values.length);
        return registerSymbol(name, b);
    }

    public int addConstantArrayInt16(String name, short[] values) {
        byte[] b = new byte[4 + values.length * 2];
        writeIntInBuf(b, 0, values.length);
        for (int i = 0; i < values.length; i++) {
            short v = values[i];
            b[4 + i * 2]     = (byte) ((v >> 8) & 0xFF);
            b[4 + i * 2 + 1] = (byte) ( v       & 0xFF);
        }
        return registerSymbol(name, b);
    }

    public int addGlobalArrayInt8(String name, int length) {
        if (length < 0) throw new RuntimeException("Tamaño negativo en addGlobalArrayInt8(" + name + "): " + length);
        byte[] b = new byte[4 + length];
        writeIntInBuf(b, 0, length);
        return registerSymbol(name, b);
    }

    public int addGlobalArrayInt16(String name, int length) {
        if (length < 0) throw new RuntimeException("Tamaño negativo en addGlobalArrayInt16(" + name + "): " + length);
        byte[] b = new byte[4 + length * 2];
        writeIntInBuf(b, 0, length);
        return registerSymbol(name, b);
    }

    private static void writeIntInBuf(byte[] buf, int off, int v) {
        buf[off]     = (byte) ((v >> 24) & 0xFF);
        buf[off + 1] = (byte) ((v >> 16) & 0xFF);
        buf[off + 2] = (byte) ((v >>  8) & 0xFF);
        buf[off + 3] = (byte) ( v        & 0xFF);
    }

    // --- Params y locales ---

    public void declareParam(String paramName) { declareParam(paramName, 4); }

    // H1.2 (V2): param con ancho explícito (long = 8 bytes / 2 slots).
    public void declareParam(String paramName, int sizeBytes) {
        if (!currentParams.contains(paramName)) {
            currentParams.add(paramName);
            currentParamSizes.add(sizeBytes);
        }
    }

    // H1.2 (V2): offset (negativo, desde bp) del param `index`, width-aware:
    //   offset = -12 - (suma de anchos de params index..fin). Para params de
    //   4 bytes coincide con la fórmula previa -12 - 4*(n-index).
    private int paramOffset(int index) {
        int off = -12;
        for (int j = index; j < currentParamSizes.size(); j++) off -= currentParamSizes.get(j);
        return off;
    }

    // Nº de slots de 4 bytes que ocupan los params (long = 2 slots). Para
    // params i32/ref coincide con currentParams.size() → backward-compatible
    // con el operando que RET emitía antes.
    private int paramSlots() {
        int slots = 0;
        for (int sz : currentParamSizes) slots += sz / 4;
        return slots;
    }

    public void declareLocal(String localName) {
        if (findLocalSlot(localName) != null) return;
        LocalSlot s = new LocalSlot();
        s.name = localName;
        s.offsetBytes = currentLocalsBytes;
        s.sizeBytes = 4;
        s.isArray = false;
        currentLocalSlots.add(s);
        currentLocalsBytes += 4;
    }

    // H1.2 (V2): local long = 8 bytes (2 slots). El frame (ENTER) crece solo
    // porque se calcula desde currentLocalsBytes. emitGet/SetLocal detectan
    // sizeBytes==8 y emiten GET_LOCAL_L/SET_LOCAL_L.
    public void declareLocalLong(String localName) {
        if (findLocalSlot(localName) != null) return;
        LocalSlot s = new LocalSlot();
        s.name = localName;
        s.offsetBytes = currentLocalsBytes;
        s.sizeBytes = 8;
        s.isArray = false;
        currentLocalSlots.add(s);
        currentLocalsBytes += 8;
    }

    public void declareLocalArray(String localName, int length) {
        if (length < 0) throw new RuntimeException("Tamaño negativo en declareLocalArray(" + localName + "): " + length);
        if (findLocalSlot(localName) != null) throw new RuntimeException("Local ya declarado: " + localName);

        LocalSlot s = new LocalSlot();
        s.name = localName;
        s.offsetBytes = currentLocalsBytes;
        s.sizeBytes = 4 + length * 4;
        s.isArray = true;
        s.arrayLength = length;
        currentLocalSlots.add(s);
        currentLocalsBytes += s.sizeBytes;

        try {
            emitPushInt(length);
            emitLocalAccessOptimal(s.offsetBytes, OpCode.SET_LOCAL, OpCode.SET_LOCAL_S8);
        } catch (IOException e) {
            throw new RuntimeException("Error inicializando array local " + localName + ": " + e.getMessage(), e);
        }
    }

    private LocalSlot findLocalSlot(String name) {
        for (LocalSlot s : currentLocalSlots) if (s.name.equals(name)) return s;
        return null;
    }

    // --- Clases ---

    /**
     * Abre una nueva clase. Si parentName != null, hereda en orden los campos y
     * la vtable del padre; los métodos del padre quedan registrados con su
     * qualifiedName original (puede ser overrideado luego con addMethod).
     */
    public void addClass(String name, String parentName) {
        addClass(name, parentName, null);
    }

    /**
     * L2 v3 — Variante con padre EXTERNO (otro módulo). El ModWriter local no
     * tiene el ClassInfo del parent; recibe sólo el layout binario mínimo
     * (numFields, numMethods, field+owner bitmaps) para preservar el slot/field
     * numbering y el bitmap del GC. Los slots heredados quedan como placeholders
     * sin qualifiedName: en endClass se serializan como vt[slot] = -1, y la VM
     * sube por la cadena via parentOff (resuelto en linkAll).
     *
     * Si {@code externalParent} != null: parentName se trata como qualifiedName
     * (e.g. "L2Lib.Counter") y se registra un classFixup.
     *
     * Si {@code externalParent} == null y parentName != null: parent local
     * (como en la variante de dos args).
     */
    public void addClass(String name, String parentName, ExternalParentLayout externalParent) {
        if (currentClass != null) {
            throw new RuntimeException("addClass dentro de otra clase abierta: cierra primero con endClass()");
        }
        if (classes.containsKey(name)) {
            throw new RuntimeException("Clase ya declarada: " + name);
        }
        ClassInfo c = new ClassInfo();
        c.name = name;
        if (externalParent != null) {
            // Parent vive en otro módulo: pre-popular placeholders para el slot
            // numbering, registrar fixup, y NO copiar nombres (no los conocemos
            // todos del .bpi v6 inicial; v7 podría exponerlos para override).
            c.externalParent = true;
            c.externalParentQualified = parentName;   // qualifiedName
            c.parent = null;                          // sin entrada local en `classes`
            // BUG-6: numFields = nº de SLOTS de 4 bytes del parent (un campo de
            // 8 bytes cuenta como 2). Cada placeholder = 1 slot; un campo de 8
            // bytes del parent se ve aquí como 2 slots no-ref consecutivos. El
            // child nunca los lee (son __inh), sólo preservan el numbering/bitmap.
            for (int i = 0; i < externalParent.numFields; i++) {
                FieldInfo f = new FieldInfo();
                f.name = "__inh" + i;                 // placeholder; no usable por GET_FIELD del child
                f.slot = i;
                f.is8 = false;
                int word = i >>> 5;
                int mask = 1 << (i & 31);
                f.isRef   = (word < externalParent.fieldBitmap.length)
                            && (externalParent.fieldBitmap[word] & mask) != 0;
                f.isOwner = (word < externalParent.ownerBitmap.length)
                            && (externalParent.ownerBitmap[word] & mask) != 0;
                c.fields.add(f);
            }
            for (int i = 0; i < externalParent.numMethods; i++) {
                MethodInfo m = new MethodInfo();
                m.simpleName = "__inh" + i;
                m.qualifiedName = null;               // placeholder: vt[slot] = -1
                m.slot = i;
                c.methods.add(m);
            }
            ClassFixup fx = new ClassFixup();
            fx.childClassName = name;
            fx.parentQualified = parentName;
            classFixups.add(fx);
        } else if (parentName != null) {
            ClassInfo p = classes.get(parentName);
            if (p == null) {
                throw new RuntimeException("Clase padre no declarada: " + parentName + " (en addClass " + name + ")");
            }
            c.parent = parentName;
            for (FieldInfo pf : p.fields) {
                FieldInfo f = new FieldInfo();
                f.name = pf.name;
                f.isRef = pf.isRef;
                f.isOwner = pf.isOwner;
                f.slot = pf.slot;     // BUG-6: preservar slot/ancho heredados
                f.is8 = pf.is8;
                c.fields.add(f);
            }
            for (MethodInfo pm : p.methods) {
                MethodInfo m = new MethodInfo();
                m.simpleName = pm.simpleName;
                m.qualifiedName = pm.qualifiedName;
                m.slot = pm.slot;
                c.methods.add(m);
            }
        }
        classes.put(name, c);
        currentClass = c;
    }

    /** Layout binario mínimo de un parent cross-module, derivado del .bpi del
     *  módulo dueño. fieldBitmap y ownerBitmap son arrays de int (cada int =
     *  32 slots; mismo formato que el descriptor binario). */
    public static final class ExternalParentLayout {
        public final int numFields;
        public final int numMethods;
        public final int[] fieldBitmap;
        public final int[] ownerBitmap;
        public ExternalParentLayout(int numFields, int numMethods,
                                    int[] fieldBitmap, int[] ownerBitmap) {
            this.numFields = numFields;
            this.numMethods = numMethods;
            this.fieldBitmap = fieldBitmap != null ? fieldBitmap : new int[0];
            this.ownerBitmap = ownerBitmap != null ? ownerBitmap : new int[0];
        }
    }

    /** Declara un campo en la clase abierta.
     *  @param isRef    true ⇒ el GC traza el slot.
     *  @param isOwner  true ⇒ al liberar la instancia, este campo se libera recursivamente.
     *  @param is8      true ⇒ campo de 8 bytes (long/double): ocupa 2 slots de 4 bytes.
     */
    public void declareField(String fieldName, boolean isRef, boolean isOwner, boolean is8) {
        if (currentClass == null) {
            throw new RuntimeException("declareField fuera de una clase (¿llamada antes de addClass?)");
        }
        for (FieldInfo f : currentClass.fields) {
            if (f.name.equals(fieldName)) {
                throw new RuntimeException("Campo ya declarado en " + currentClass.name + ": " + fieldName);
            }
        }
        FieldInfo f = new FieldInfo();
        f.name = fieldName;
        f.isRef = isRef;
        f.isOwner = isOwner;
        f.is8 = is8;
        f.slot = nextFieldSlot(currentClass);   // BUG-6: slot = suma de anchos previos
        currentClass.fields.add(f);
    }

    /** Atajo: campo de 4 bytes con owner. */
    public void declareField(String fieldName, boolean isRef, boolean isOwner) {
        declareField(fieldName, isRef, isOwner, false);
    }

    /** Atajo compatible con la API anterior (sin owner). */
    public void declareField(String fieldName, boolean isRef) {
        declareField(fieldName, isRef, false, false);
    }

    /** BUG-6: índice del siguiente slot libre = slot del último campo + su ancho
     *  (1 slot si 4 bytes, 2 slots si 8 bytes). 0 si no hay campos. */
    private static int nextFieldSlot(ClassInfo c) {
        if (c.fields.isEmpty()) return 0;
        FieldInfo last = c.fields.get(c.fields.size() - 1);
        return last.slot + (last.is8 ? 2 : 1);
    }

    /**
     * Modifica el flag `isOwner` de un campo ya declarado en la clase actual.
     * Útil para clases derivadas que quieran promover un campo heredado a
     * "owner" (ej. OwnerList hereda `items` de List y lo promueve a owner
     * para que la cascada de FREE_REF libere también el array y su contenido).
     */
    public void setFieldOwner(String fieldName, boolean isOwner) {
        if (currentClass == null) {
            throw new RuntimeException("setFieldOwner fuera de una clase abierta");
        }
        for (FieldInfo f : currentClass.fields) {
            if (f.name.equals(fieldName)) { f.isOwner = isOwner; return; }
        }
        throw new RuntimeException("Campo no encontrado en " + currentClass.name + ": " + fieldName);
    }

    /**
     * Abre un nuevo método de la clase actual. Internamente registra una función
     * con nombre "Clase.metodo" (no exportada al exports table) y declara
     * automáticamente "this" como primer parámetro. Si la clase hereda un
     * método con el mismo simpleName, sobreescribe su slot (override); en otro
     * caso, asigna un slot nuevo al final de la vtable.
     *
     * El cuerpo del método se emite con las APIs normales y debe terminar con
     * emitRet(). Al llamar a addMethod / addFunction / endClass posteriormente,
     * el cierre de la función previa se hace automáticamente.
     */
    public void addMethod(String simpleName) {
        if (currentClass == null) {
            throw new RuntimeException("addMethod fuera de una clase (¿llamada antes de addClass?)");
        }
        String qualifiedName = currentClass.name + "." + simpleName;

        int slot = -1;
        for (MethodInfo m : currentClass.methods) {
            if (m.simpleName.equals(simpleName)) {
                slot = m.slot;
                m.qualifiedName = qualifiedName;
                break;
            }
        }
        if (slot == -1) {
            MethodInfo m = new MethodInfo();
            m.simpleName = simpleName;
            m.qualifiedName = qualifiedName;
            m.slot = currentClass.methods.size();
            currentClass.methods.add(m);
        }

        // Los métodos no se exportan al exports table (acceso intra-módulo vía vtable).
        addFunction(qualifiedName, false);
        declareParam("this");
    }

    /**
     * Abre un método PRIVADO de la clase actual. A diferencia de addMethod, NO
     * lo añade a la vtable: un método privado no se puede sobreescribir ni se
     * llama cross-module, así que se despacha por CALL directo ("Clase.metodo")
     * — como un super() o un método estático. Así la vtable contiene solo
     * métodos públicos y su orden coincide con el .bpi (evita el desfase de
     * slots cross-module, BUG-4). Registra la función llamable + "this".
     */
    public void addPrivateMethod(String simpleName) {
        if (currentClass == null) {
            throw new RuntimeException("addPrivateMethod fuera de una clase (¿antes de addClass?)");
        }
        String qualifiedName = currentClass.name + "." + simpleName;
        addFunction(qualifiedName, false);   // llamable por CALL, sin slot de vtable
        declareParam("this");
    }

    /**
     * Cierra la clase actual: serializa el class descriptor (num_fields,
     * num_methods, bitmap_words, field_bitmap, vtable) y lo registra como
     * símbolo del data block. Tras esto la clase puede usarse en emitNewObject.
     *
     * Layout (concuerda con lo que espera VirtualMachine):
     *   [+0]   num_fields    (u16)
     *   [+2]   num_methods   (u16)
     *   [+4]   bitmap_words  (u16)   = ceil(num_fields/32)
     *   [+6]   _pad          (u16)
     *   [+8]   parent_offset (i32)   CS-relative al descriptor del padre (0 = sin padre)
     *   [+12]  field_bitmap  (bw*4 bytes)   bit k=1 ⇒ field[k] es ref (GC trace)
     *   [+12 + bw*4]  owner_bitmap (bw*4 bytes)   bit k=1 ⇒ field[k] es owner (FREE recursivo)
     *   [+12 + 2*bw*4]  vtable (num_methods * 4 bytes; offsets relativos a code)
     */
    public void endClass() {
        if (currentClass == null) {
            throw new RuntimeException("endClass sin addClass previo");
        }
        ClassInfo c = currentClass;
        // BUG-6: num_fields es el nº de SLOTS de 4 bytes de la instancia. Un campo
        // de 8 bytes (long/double) ocupa 2 slots, así que num_slots = suma de
        // anchos, NO el nº de campos. NEW_OBJECT aloca num_fields*4 bytes.
        int numFields  = nextFieldSlot(c);   // = nº total de slots de 4 bytes
        int numMethods = c.methods.size();
        int bitmapWords = (numFields + 31) >>> 5;
        int descSize = 12 + 2 * bitmapWords * 4 + numMethods * 4;

        byte[] desc = new byte[descSize];
        // Cabecera
        desc[0] = (byte) ((numFields >> 8)  & 0xFF);
        desc[1] = (byte) ( numFields        & 0xFF);
        desc[2] = (byte) ((numMethods >> 8) & 0xFF);
        desc[3] = (byte) ( numMethods       & 0xFF);
        desc[4] = (byte) ((bitmapWords >> 8) & 0xFF);
        desc[5] = (byte) ( bitmapWords       & 0xFF);
        desc[6] = 0;
        desc[7] = 0;

        // Parent offset (CS-relative). 0 = sin padre.
        // Si parent es cross-module, parentOffset se deja a 0 aquí y se PARCHA
        // en runtime por el loader (ver classFixups + serializeClassFixups).
        int parentOffset = 0;
        if (c.parent != null) {
            Integer parentCsOff = dataSymbolOffset.get(c.parent);
            if (parentCsOff == null) {
                throw new RuntimeException("Clase padre '" + c.parent + "' no fue registrada antes que la hija '" + c.name + "'");
            }
            parentOffset = parentCsOff;
        }
        writeIntInBuf(desc, 8, parentOffset);

        // Field bitmap (offset 12). Detecta refs para el GC.
        int[] fieldBitmap = new int[bitmapWords];
        // Owner bitmap (offset 12 + bw*4). Detecta owners para FREE_REF recursivo.
        int[] ownerBitmap = new int[bitmapWords];
        // BUG-6: el bit del bitmap se indexa por SLOT (no por índice de campo).
        // Un campo de 8 bytes ocupa 2 slots; al ser primitivo (long/double) no
        // es ref ni owner, así que ambos slots quedan a 0 — correcto para el GC.
        for (FieldInfo fi : c.fields) {
            int s = fi.slot;
            if (fi.isRef)   fieldBitmap[s >>> 5] |= (1 << (s & 31));
            if (fi.isOwner) ownerBitmap[s >>> 5] |= (1 << (s & 31));
        }
        int fieldBitmapBase = 12;
        int ownerBitmapBase = 12 + bitmapWords * 4;
        for (int wIdx = 0; wIdx < bitmapWords; wIdx++) {
            writeIntInBuf(desc, fieldBitmapBase + wIdx * 4, fieldBitmap[wIdx]);
            writeIntInBuf(desc, ownerBitmapBase + wIdx * 4, ownerBitmap[wIdx]);
        }

        // Vtable: offsets relativos al code start del módulo. Para slots
        // heredados de un parent cross-module (qualifiedName == null) se
        // escribe -1: la VM detecta el sentinel en INVOKE_VIRTUAL y sube
        // por la cadena de herencia via parentOff (resuelto por linkAll).
        int vtBase = 12 + 2 * bitmapWords * 4;
        for (MethodInfo m : c.methods) {
            if (m.qualifiedName == null) {
                // Placeholder de slot heredado cross-module.
                writeIntInBuf(desc, vtBase + m.slot * 4, -1);
                continue;
            }
            Integer relAddr = functionAddresses.get(m.qualifiedName);
            if (relAddr == null) {
                throw new RuntimeException("Método sin cuerpo: " + m.qualifiedName
                        + " (¿addMethod sin emitir cuerpo + emitRet?)");
            }
            writeIntInBuf(desc, vtBase + m.slot * 4, relAddr);
        }

        registerSymbol(c.name, desc);
        // Guardamos el layout binario (numFields/numMethods + bitmaps) por
        // si el frontend lo necesita para emitir el ClassSig al .bpi.
        // Se consulta vía getClassLayout(name) después de endClass.
        ClassLayoutInfo info = new ClassLayoutInfo();
        info.numFields = numFields;
        info.numMethods = numMethods;
        info.fieldBitmap = fieldBitmap.clone();
        info.ownerBitmap = ownerBitmap.clone();
        classLayouts.put(c.name, info);
        currentClass = null;
    }

    /** Layout binario público (numFields/numMethods + bitmaps) consultable
     *  tras endClass. La info viaja al .bpi para que importadores con
     *  extends cross-module puedan reservar el slot/field count correcto. */
    public static final class ClassLayoutInfo {
        public int numFields;
        public int numMethods;
        public int[] fieldBitmap;
        public int[] ownerBitmap;
    }
    private final Map<String, ClassLayoutInfo> classLayouts = new HashMap<>();
    public ClassLayoutInfo getClassLayout(String className) {
        return classLayouts.get(className);
    }

    // --- Emisores para objetos ---

    public void emitNewObject(String className) throws IOException {
        Integer csOff = dataSymbolOffset.get(className);
        if (csOff == null) {
            throw new RuntimeException("Clase no declarada (¿endClass antes de emitNewObject?): " + className);
        }
        codeOut.writeByte(OpCode.NEW_OBJECT.code);
        codeOut.writeShort(csOff.shortValue());
        currentBytecodeSize += 3;
    }

    public void emitGetField(String className, String fieldName) throws IOException {
        FieldInfo f = resolveFieldInfo(className, fieldName);
        if (f.slot > 0xFF) throw new RuntimeException("Field slot u8 desbordado: " + f.slot + " en " + className);
        // BUG-6: campo de 8 bytes (long/double) → opcode ancho que lee 8 bytes.
        codeOut.writeByte(f.is8 ? OpCode.GET_FIELD_LONG.code : OpCode.GET_FIELD.code);
        codeOut.writeByte((byte) f.slot);
        currentBytecodeSize += 2;
    }

    public void emitSetField(String className, String fieldName) throws IOException {
        FieldInfo f = resolveFieldInfo(className, fieldName);
        if (f.slot > 0xFF) throw new RuntimeException("Field slot u8 desbordado: " + f.slot + " en " + className);
        codeOut.writeByte(f.is8 ? OpCode.SET_FIELD_LONG.code : OpCode.SET_FIELD.code);
        codeOut.writeByte((byte) f.slot);
        currentBytecodeSize += 2;
    }

    /**
     * Variante para campos owner: libera el valor anterior del campo antes de
     * escribir el nuevo. Stack: pop val, pop ref; field[slot] = val (con FREE
     * del valor que tenía).
     */
    public void emitSetFieldOwner(String className, String fieldName) throws IOException {
        int slot = resolveFieldSlot(className, fieldName);
        if (slot > 0xFF) throw new RuntimeException("Field slot u8 desbordado: " + slot + " en " + className);
        codeOut.writeByte(OpCode.SET_FIELD_OWNER.code);
        codeOut.writeByte((byte) slot);
        currentBytecodeSize += 2;
    }

    public void emitInvokeVirtual(String className, String methodSimpleName, int numArgs) throws IOException {
        int slot = resolveMethodSlot(className, methodSimpleName);
        emitInvokeVirtualBySlot(slot, numArgs);
    }

    /**
     * Variante para llamadas cross-module: el caller ya conoce el slot del
     * vtable (porque se lo dio el .bpi del módulo dueño), y la clase NO está
     * registrada en este ModWriter (su descriptor vive en otro .mod).
     *
     * El bytecode generado es idéntico a {@link #emitInvokeVirtual}; sólo
     * cambia que el slot lo provee el caller en vez de mirarse en classes.
     */
    public void emitInvokeVirtualBySlot(int slot, int numArgs) throws IOException {
        if (slot < 0 || slot > 0xFF) throw new RuntimeException("Method slot u8 desbordado: " + slot);
        if (numArgs < 0 || numArgs > 0xFF) throw new RuntimeException("numArgs u8 desbordado: " + numArgs);
        codeOut.writeByte(OpCode.INVOKE_VIRTUAL.code);
        codeOut.writeByte((byte) slot);
        codeOut.writeByte((byte) numArgs);
        currentBytecodeSize += 3;
    }

    private FieldInfo resolveFieldInfo(String className, String fieldName) {
        ClassInfo c = classes.get(className);
        if (c == null) throw new RuntimeException("Clase no declarada: " + className);
        for (FieldInfo f : c.fields) {
            if (f.name.equals(fieldName)) return f;
        }
        throw new RuntimeException("Campo no encontrado en " + className + ": " + fieldName);
    }

    private int resolveFieldSlot(String className, String fieldName) {
        // BUG-6: el slot ya no es el índice en la lista, sino el offset de slot
        // (4 bytes/slot) acumulado, que es lo que el opcode lleva como inmediato.
        return resolveFieldInfo(className, fieldName).slot;
    }

    private int resolveMethodSlot(String className, String methodSimpleName) {
        ClassInfo c = classes.get(className);
        if (c == null) throw new RuntimeException("Clase no declarada: " + className);
        for (MethodInfo m : c.methods) {
            if (m.simpleName.equals(methodSimpleName)) return m.slot;
        }
        throw new RuntimeException("Método no encontrado en " + className + ": " + methodSimpleName);
    }

    /** #174b — slot de vtable de un método, o -1 si la clase/método no existe.
     *  Versión no-lanzante para el cross-check del frontend: AotCEmitter computa
     *  el slot por su cuenta (ClassSymbol.slotOf) y MivmEmitter lo verifica contra
     *  esta, que es la autoridad del layout del .mod. */
    public int methodSlotOrMinus1(String className, String methodSimpleName) {
        ClassInfo c = classes.get(className);
        if (c == null) return -1;
        for (MethodInfo m : c.methods) {
            if (m.simpleName.equals(methodSimpleName)) return m.slot;
        }
        return -1;
    }

    // --- Emisiones de instrucciones simples ---

    public void emit(OpCode op) throws IOException {
        codeOut.writeByte(op.code);
        currentBytecodeSize += 1;
    }

    public void emitInt(int value) throws IOException {
        codeOut.writeInt(value);
        currentBytecodeSize += 4;
    }

    public void emitShort(short value) throws IOException {
        codeOut.writeShort(value);
        currentBytecodeSize += 2;
    }

    // H1.2 (V2): inmediato de 64 bits big-endian para LPUSH (operand IMM_I64).
    public void emitLong(long value) throws IOException {
        codeOut.writeLong(value);
        currentBytecodeSize += 8;
    }

    public void emitPushFloat(float v) throws IOException {
        codeOut.writeByte(OpCode.FPUSH.code);
        codeOut.writeInt(Float.floatToRawIntBits(v));
        currentBytecodeSize += 5;
    }

    // --- Llamadas a funciones ---

    public void emitCall(String functionName) throws IOException {
        codeOut.writeByte(OpCode.CALL.code);
        callFixups.add(new CallFixup(functionName, currentBytecodeSize + 1));
        codeOut.writeInt(0);
        currentBytecodeSize += 5;
    }

    /**
     * Versión single-arg: no especifica fromPath, el loader usará la convención
     * por defecto (library.module.mod en el mismo directorio).
     */
    public void emitCallExt(String moduleAndFunc) throws IOException {
        emitCallExt(moduleAndFunc, "");
    }

    /**
     * Variante con fromPath explícito. fromPath es una ruta (absoluta o relativa
     * al directorio del importer) hacia el .mod que provee el símbolo. Sólo la
     * usa el loader; el linker resuelve por nombre lógico.
     *
     * Si el mismo moduleAndFunc se referencia varias veces, debe llevar siempre
     * el mismo fromPath (lo enforzamos para no acabar con metadatos
     * inconsistentes).
     */
    public void emitCallExt(String moduleAndFunc, String fromPath) throws IOException {
        int idx = -1;
        for (int i = 0; i < externalFunctions.size(); i++) {
            if (externalFunctions.get(i).name.equals(moduleAndFunc)) { idx = i; break; }
        }
        if (idx < 0) {
            externalFunctions.add(new ExternalRef(moduleAndFunc, fromPath));
            idx = externalFunctions.size() - 1;
        } else {
            String prev = externalFunctions.get(idx).fromPath;
            String now  = (fromPath == null) ? "" : fromPath;
            if (!prev.equals(now)) {
                throw new RuntimeException("Inconsistencia de fromPath para import '"
                        + moduleAndFunc + "': '" + prev + "' vs '" + now + "'");
            }
        }
        codeOut.writeByte(OpCode.CALL_EXT.code);
        codeOut.writeShort((short) idx);
        currentBytecodeSize += 3;
    }

    // --- Acceso a params y locales escalares ---

    /**
     * Emite un acceso BP-relativo (GET_LOCAL / SET_LOCAL / LEA_LOCAL) eligiendo
     * la variante compacta S8 (operando de 1 byte) si el offset cabe en
     * -128..127, o la variante I16 (2 bytes) en otro caso. Ahorra 1 byte por
     * uso para los offsets habituales (params típicos = -16..-20, locales
     * típicos = 0..+64).
     */
    /**
     * Activa las variantes compactas de opcodes (PUSH_0..4, *_S8). Reducen
     * el tamaño del .mod en ~14% para programas típicos, pero AÑADEN cases
     * al switch del intérprete que la JIT del HotSpot no compila tan bien
     * como antes (medido: fibo(30) 1.22s → 2.08s, +70% latencia).
     *
     * La causa probable: el tableswitch generado por C2 crece (109 entradas
     * vs 97) y el branch-target-buffer del CPU acumula más mispredictions
     * en el dispatch indirecto. Para programas pequeños CPU-light el
     * trade-off es aceptable; para hot loops (fibo, parsers) NO compensa.
     *
     * Hasta haber un compilador AOT / mejor profile-driven dispatch, lo
     * dejamos OFF por defecto. La infraestructura está completa (opcodes,
     * cases en run(), Disasm, helpers) y se puede activar aquí.
     */
    private static final boolean USE_COMPACT_OPS = false;
    private void emitLocalAccessOptimal(int offset, OpCode i16op, OpCode i8op) throws IOException {
        if (USE_COMPACT_OPS && offset >= -128 && offset <= 127) {
            codeOut.writeByte(i8op.code);
            codeOut.writeByte((byte) offset);
            currentBytecodeSize += 2;
        } else {
            codeOut.writeByte(i16op.code);
            codeOut.writeShort((short) offset);
            currentBytecodeSize += 3;
        }
    }

    public void emitGetParam(String paramName) throws IOException {
        int index = currentParams.indexOf(paramName);
        if (index == -1) throw new RuntimeException("Parámetro no declarado: " + paramName);
        int offset = paramOffset(index);
        if (currentParamSizes.get(index) == 8) {   // H1.2: param long → GET_LOCAL_L
            codeOut.writeByte(OpCode.GET_LOCAL_L.code);
            codeOut.writeShort((short) offset);
            currentBytecodeSize += 3;
            return;
        }
        emitLocalAccessOptimal(offset, OpCode.GET_LOCAL, OpCode.GET_LOCAL_S8);
    }

    public void emitSetParam(String paramName) throws IOException {
        int index = currentParams.indexOf(paramName);
        if (index == -1) throw new RuntimeException("Parámetro no declarado: " + paramName);
        int offset = paramOffset(index);
        if (currentParamSizes.get(index) == 8) {   // H1.2: param long → SET_LOCAL_L
            codeOut.writeByte(OpCode.SET_LOCAL_L.code);
            codeOut.writeShort((short) offset);
            currentBytecodeSize += 3;
            return;
        }
        emitLocalAccessOptimal(offset, OpCode.SET_LOCAL, OpCode.SET_LOCAL_S8);
    }

    public void emitGetLocal(String localName) throws IOException {
        LocalSlot s = findLocalSlot(localName);
        if (s == null) throw new RuntimeException("Variable local no declarada: " + localName);
        if (s.isArray) throw new RuntimeException("La local '" + localName + "' es un array; usa emitLeaLocal.");
        if (s.sizeBytes == 8) {   // H1.2: long local → GET_LOCAL_L (8 bytes)
            codeOut.writeByte(OpCode.GET_LOCAL_L.code);
            codeOut.writeShort((short) s.offsetBytes);
            currentBytecodeSize += 3;
            return;
        }
        emitLocalAccessOptimal(s.offsetBytes, OpCode.GET_LOCAL, OpCode.GET_LOCAL_S8);
    }

    public void emitSetLocal(String localName) throws IOException {
        LocalSlot s = findLocalSlot(localName);
        if (s == null) throw new RuntimeException("Variable local no declarada: " + localName);
        if (s.isArray) throw new RuntimeException("La local '" + localName + "' es un array; no se puede asignar por valor.");
        if (s.sizeBytes == 8) {   // H1.2: long local → SET_LOCAL_L (8 bytes)
            codeOut.writeByte(OpCode.SET_LOCAL_L.code);
            codeOut.writeShort((short) s.offsetBytes);
            currentBytecodeSize += 3;
            return;
        }
        emitLocalAccessOptimal(s.offsetBytes, OpCode.SET_LOCAL, OpCode.SET_LOCAL_S8);
    }

    public void emitLeaLocal(String localName) throws IOException {
        LocalSlot s = findLocalSlot(localName);
        if (s == null) throw new RuntimeException("Variable local no declarada: " + localName);
        emitLocalAccessOptimal(s.offsetBytes, OpCode.LEA_LOCAL, OpCode.LEA_LOCAL_S8);
    }

    public void emitRet() throws IOException {
        codeOut.writeByte(OpCode.RET.code);
        codeOut.writeByte((byte) paramSlots());   // H1.2: slot-count (long=2), no nº params
        currentBytecodeSize += 2;
    }

    // H1.2 (V2): RET para función que devuelve long (return value de 8 bytes).
    // Mismo teardown que RET; el operando sigue siendo el slot-count de params.
    public void emitLRet() throws IOException {
        codeOut.writeByte(OpCode.LRET.code);
        codeOut.writeByte((byte) paramSlots());
        currentBytecodeSize += 2;
    }

    // --- Acceso a símbolos del data block (i32) ---

    public void emitGetGlobal(String name) throws IOException {
        if (longGlobals.contains(name)) { emitGlobalAccess(name, OpCode.GET_GLOBAL_L); return; }  // H1.2
        emitGlobalAccessOptimal(name, OpCode.GET_GLOBAL, OpCode.GET_GLOBAL_S8);
    }
    public void emitSetGlobal(String name) throws IOException {
        if (longGlobals.contains(name)) { emitGlobalAccess(name, OpCode.SET_GLOBAL_L); return; }  // H1.2
        emitGlobalAccessOptimal(name, OpCode.SET_GLOBAL, OpCode.SET_GLOBAL_S8);
    }
    public void emitLeaGlobal(String name) throws IOException {
        emitGlobalAccessOptimal(name, OpCode.LEA_GLOBAL, OpCode.LEA_GLOBAL_S8);
    }

    /**
     * Como emitGlobalAccess, pero elige la variante S8 (operando de 1 byte)
     * cuando el offset cabe en -128..127. Sólo para los opcodes GET/SET/LEA
     * wide (i32); las variantes narrow (I8/U8/I16/U16) tienen semántica de
     * tamaño de lectura distinta y conservan operando i16.
     */
    private void emitGlobalAccessOptimal(String name, OpCode i16op, OpCode i8op) throws IOException {
        Integer off = dataSymbolOffset.get(name);
        if (off == null) throw new RuntimeException("Símbolo de datos no declarado (forward ref no permitida): " + name);
        int offset = off;
        if (USE_COMPACT_OPS && offset >= -128 && offset <= 127) {
            codeOut.writeByte(i8op.code);
            codeOut.writeByte((byte) offset);
            currentBytecodeSize += 2;
        } else {
            codeOut.writeByte(i16op.code);
            codeOut.writeShort((short) offset);
            currentBytecodeSize += 3;
        }
    }

    /**
     * Emite PUSH de un entero eligiendo la variante más compacta:
     *   - PUSH_0..4 / PUSH_NEG1 (1 byte) para constantes muy comunes.
     *   - PUSH + i32 (5 bytes) en cualquier otro caso.
     */
    public void emitPushInt(int value) throws IOException {
        OpCode shortOp = null;
        if (USE_COMPACT_OPS) switch (value) {
            case  0: shortOp = OpCode.PUSH_0;    break;
            case  1: shortOp = OpCode.PUSH_1;    break;
            case  2: shortOp = OpCode.PUSH_2;    break;
            case  3: shortOp = OpCode.PUSH_3;    break;
            case  4: shortOp = OpCode.PUSH_4;    break;
            case -1: shortOp = OpCode.PUSH_NEG1; break;
        }
        if (shortOp != null) {
            codeOut.writeByte(shortOp.code);
            currentBytecodeSize += 1;
        } else {
            codeOut.writeByte(OpCode.PUSH.code);
            codeOut.writeInt(value);
            currentBytecodeSize += 5;
        }
    }

    // --- Acceso a símbolos del data block (tipos estrechos) ---

    public void emitGetGlobalI8(String name)  throws IOException { emitGlobalAccess(name, OpCode.GET_GLOBAL_I8);  }
    public void emitGetGlobalU8(String name)  throws IOException { emitGlobalAccess(name, OpCode.GET_GLOBAL_U8);  }
    public void emitGetGlobalI16(String name) throws IOException { emitGlobalAccess(name, OpCode.GET_GLOBAL_I16); }
    public void emitGetGlobalU16(String name) throws IOException { emitGlobalAccess(name, OpCode.GET_GLOBAL_U16); }
    public void emitSetGlobalI8(String name)  throws IOException { emitGlobalAccess(name, OpCode.SET_GLOBAL_I8);  }
    public void emitSetGlobalI16(String name) throws IOException { emitGlobalAccess(name, OpCode.SET_GLOBAL_I16); }

    private void emitGlobalAccess(String name, OpCode op) throws IOException {
        Integer off = dataSymbolOffset.get(name);
        if (off == null) throw new RuntimeException("Símbolo de datos no declarado (forward ref no permitida): " + name);
        codeOut.writeByte(op.code);
        codeOut.writeShort(off.shortValue());
        currentBytecodeSize += 3;
    }

    // --- Etiquetas ---

    public int newLabel() {
        int id = nextLabelId++;
        currentLabelMap.put(id, UNDECLARED);
        return id;
    }

    public void freeLabel(int labelId) {
        currentLabelMap.remove(labelId);
    }

    public void declareLabel(int labelId) {
        Integer existing = currentLabelMap.get(labelId);
        if (existing == null) {
            throw new RuntimeException("Etiqueta no creada con newLabel: " + labelId);
        }
        if (existing != UNDECLARED) {
            throw new RuntimeException("Etiqueta " + labelId + " ya fue declarada en offset " + existing);
        }
        currentLabelMap.put(labelId, currentBytecodeSize);
    }

    public void emitJump(int labelId) throws IOException {
        emitJumpGeneric(labelId, true);
    }

    /**
     * Emite TRY_BEGIN handler_rel(i32) + expected_class_cs_off(i16).
     * Si expectedClassName == null o no resuelve, se emite 0 (catch genérico).
     */
    public void emitTryBegin(int handlerLabel, String expectedClassName) throws IOException {
        Integer addr = currentLabelMap.get(handlerLabel);
        if (addr == null) throw new RuntimeException("Etiqueta no existe: " + handlerLabel);
        int instrAddr = currentBytecodeSize;
        codeOut.writeByte(OpCode.TRY_BEGIN.code);
        if (addr != UNDECLARED) {
            codeOut.writeInt(addr - instrAddr);
        } else {
            pendingJumps.add(new JumpFixup(lastFunctionName, handlerLabel, instrAddr, currentBytecodeSize + 1));
            codeOut.writeInt(0);
        }
        short expectedOff = 0;
        if (expectedClassName != null) {
            Integer csOff = dataSymbolOffset.get(expectedClassName);
            if (csOff == null) {
                throw new RuntimeException("Clase '" + expectedClassName + "' no declarada para TRY_BEGIN");
            }
            expectedOff = csOff.shortValue();
        }
        codeOut.writeShort(expectedOff);
        currentBytecodeSize += 7;
    }

    /** Versión retro-compat: TRY_BEGIN genérico (atrapa cualquier excepción). */
    public void emitTryBegin(int handlerLabel) throws IOException {
        emitTryBegin(handlerLabel, null);
    }

    public void emitInstanceOf(String className) throws IOException {
        Integer csOff = dataSymbolOffset.get(className);
        if (csOff == null) throw new RuntimeException("Clase '" + className + "' no declarada para INSTANCEOF");
        codeOut.writeByte(OpCode.INSTANCEOF.code);
        codeOut.writeShort(csOff.shortValue());
        currentBytecodeSize += 3;
    }

    public void emitTryEnd() throws IOException {
        codeOut.writeByte(OpCode.TRY_END.code);
        currentBytecodeSize += 1;
    }

    public void emitThrow() throws IOException {
        codeOut.writeByte(OpCode.THROW.code);
        currentBytecodeSize += 1;
    }

    /** FREE_REF: pop ref, libera el objeto al que apunta (no-op si null o no-objeto). */
    public void emitFreeRef() throws IOException {
        codeOut.writeByte(OpCode.FREE_REF.code);
        currentBytecodeSize += 1;
    }

    public void emitJumpIfFalse(int labelId) throws IOException {
        emitJumpGeneric(labelId, false);
    }

    private void emitJumpGeneric(int labelId, boolean unconditional) throws IOException {
        Integer addr = currentLabelMap.get(labelId);
        if (addr == null) {
            throw new RuntimeException("Etiqueta no existe: " + labelId);
        }

        int instrAddr = currentBytecodeSize;

        if (addr != UNDECLARED) {
            int offset = addr - instrAddr;
            if (offset >= -128 && offset <= 127) {
                codeOut.writeByte((unconditional ? OpCode.JUMP8 : OpCode.JUMP_IF_FALSE8).code);
                codeOut.writeByte((byte) offset);
                currentBytecodeSize += 2;
            } else if (offset >= Short.MIN_VALUE && offset <= Short.MAX_VALUE) {
                codeOut.writeByte((unconditional ? OpCode.JUMP16 : OpCode.JUMP_IF_FALSE16).code);
                codeOut.writeShort((short) offset);
                currentBytecodeSize += 3;
            } else {
                codeOut.writeByte((unconditional ? OpCode.JUMP : OpCode.JUMP_IF_FALSE).code);
                codeOut.writeInt(offset);
                currentBytecodeSize += 5;
            }
        } else {
            codeOut.writeByte((unconditional ? OpCode.JUMP : OpCode.JUMP_IF_FALSE).code);
            pendingJumps.add(new JumpFixup(lastFunctionName, labelId, instrAddr, currentBytecodeSize + 1));
            codeOut.writeInt(0);
            currentBytecodeSize += 5;
        }
    }

    // --- Properties ---
    //
    // Una property se descompone en: (1) variable de respaldo, (2) getter, (3) setter.
    // El runtime no entiende properties; ModWriter genera el código de los accessors
    // y mapea los emitGet/SetProperty al opcode adecuado según el scope.

    private static String capitalize(String s) {
        if (s.isEmpty()) return s;
        return Character.toUpperCase(s.charAt(0)) + s.substring(1);
    }

    /**
     * Property a nivel de módulo. Crea un global "name" + dos funciones públicas
     * o privadas "getName"/"setName". Si isPublic=true, las funciones van al
     * exports table y se pueden importar desde otro módulo con CALL_EXT.
     */
    public void declareModuleProperty(String name, boolean isRef, boolean isPublic) throws IOException {
        if (currentClass != null) {
            throw new RuntimeException("declareModuleProperty no puede usarse dentro de una clase abierta");
        }
        if (moduleProperties.containsKey(name)) {
            throw new RuntimeException("Property de módulo ya declarada: " + name);
        }
        // Backing global (int de 4 bytes, inicializado a 0)
        declareGlobal(name);

        String getName = "get" + capitalize(name);
        String setName = "set" + capitalize(name);

        // --- Getter ---
        addFunction(getName, isPublic);
        emitGetGlobal(name);
        emitRet();

        // --- Setter ---
        addFunction(setName, isPublic);
        declareParam("val");
        emitGetParam("val");
        emitSetGlobal(name);
        emitPushInt(0); // dummy return
        emitRet();

        PropertyInfo p = new PropertyInfo();
        p.simpleName = name;
        p.scope = PropertyScope.MODULE;
        p.isPublic = isPublic;
        p.getterFunctionName = getName;
        p.setterFunctionName = setName;
        moduleProperties.put(name, p);
    }

    public void emitGetModuleProperty(String name) throws IOException {
        PropertyInfo p = moduleProperties.get(name);
        if (p == null) throw new RuntimeException("Property de módulo no declarada: " + name);
        emitCall(p.getterFunctionName);
    }

    public void emitSetModuleProperty(String name) throws IOException {
        PropertyInfo p = moduleProperties.get(name);
        if (p == null) throw new RuntimeException("Property de módulo no declarada: " + name);
        // Convención: caller deja `val` en la pila antes; tras el setter queda el dummy 0,
        // que es responsabilidad del caller descartar (SET_LOCAL a un dummy).
        emitCall(p.setterFunctionName);
    }

    /**
     * Property de instancia (vive en la vtable). Añade un field al objeto +
     * dos métodos virtuales getName/setName auto-generados. isPublic afecta
     * sólo al check de visibilidad en compile-time (instance properties nunca
     * van al exports table).
     */
    public void declareInstanceProperty(String name, boolean isRef, boolean isPublic) throws IOException {
        if (currentClass == null) {
            throw new RuntimeException("declareInstanceProperty fuera de una clase abierta");
        }
        Map<String, PropertyInfo> classProps = instanceProperties.computeIfAbsent(currentClass.name, k -> new HashMap<>());
        if (classProps.containsKey(name)) {
            throw new RuntimeException("Property de instancia ya declarada en " + currentClass.name + ": " + name);
        }

        // 1) Backing field en el objeto
        declareField(name, isRef);
        String className = currentClass.name;

        // 2) Getter virtual
        String getSimple = "get" + capitalize(name);
        addMethod(getSimple);
        // currentParams = ["this"] (auto-declarado por addMethod)
        emitGetParam("this");
        emitGetField(className, name);
        emitRet();

        // 3) Setter virtual
        String setSimple = "set" + capitalize(name);
        addMethod(setSimple);
        declareParam("val");
        // SET_FIELD pop'ea (val, then ref); push in that order
        emitGetParam("this");
        emitGetParam("val");
        emitSetField(className, name);
        emitPushInt(0);
        emitRet();

        PropertyInfo p = new PropertyInfo();
        p.simpleName = name;
        p.classOwner = className;
        p.scope = PropertyScope.INSTANCE;
        p.isPublic = isPublic;
        p.getterFunctionName = className + "." + getSimple;
        p.setterFunctionName = className + "." + setSimple;
        classProps.put(name, p);
    }

    public void emitGetInstanceProperty(String className, String propName) throws IOException {
        PropertyInfo p = lookupInstanceProperty(className, propName);
        checkPrivateAccess(p);
        // Convención: caller ya ha puesto `this` en la pila.
        String simpleGet = "get" + capitalize(propName);
        emitInvokeVirtual(className, simpleGet, 0);
    }

    public void emitSetInstanceProperty(String className, String propName) throws IOException {
        PropertyInfo p = lookupInstanceProperty(className, propName);
        checkPrivateAccess(p);
        // Convención: caller ya ha puesto `this` y `val` (en ese orden).
        String simpleSet = "set" + capitalize(propName);
        emitInvokeVirtual(className, simpleSet, 1);
    }

    private PropertyInfo lookupInstanceProperty(String className, String propName) {
        // Busca en la clase y sus padres
        ClassInfo c = classes.get(className);
        if (c == null) throw new RuntimeException("Clase no declarada: " + className);
        while (c != null) {
            Map<String, PropertyInfo> classProps = instanceProperties.get(c.name);
            if (classProps != null) {
                PropertyInfo p = classProps.get(propName);
                if (p != null) return p;
            }
            c = (c.parent != null) ? classes.get(c.parent) : null;
        }
        throw new RuntimeException("Property de instancia no encontrada en " + className + ": " + propName);
    }

    /**
     * Property estática de clase. Crea un global cualificado "ClassName.name" +
     * dos funciones (sin `this`) "ClassName.getName"/"ClassName.setName". Si
     * isPublic=true, las funciones van al exports table.
     */
    public void declareStaticProperty(String className, String name, boolean isRef, boolean isPublic) throws IOException {
        if (currentClass != null) {
            throw new RuntimeException("declareStaticProperty no puede usarse dentro de una clase abierta (llámala después de endClass)");
        }
        if (!classes.containsKey(className)) {
            throw new RuntimeException("Clase no declarada: " + className);
        }
        Map<String, PropertyInfo> classProps = staticProperties.computeIfAbsent(className, k -> new HashMap<>());
        if (classProps.containsKey(name)) {
            throw new RuntimeException("Property estática ya declarada en " + className + ": " + name);
        }

        String qualifiedGlobalName = className + "." + name;
        declareGlobal(qualifiedGlobalName);

        String getName = className + ".get" + capitalize(name);
        String setName = className + ".set" + capitalize(name);

        // Getter
        addFunction(getName, isPublic);
        emitGetGlobal(qualifiedGlobalName);
        emitRet();

        // Setter
        addFunction(setName, isPublic);
        declareParam("val");
        emitGetParam("val");
        emitSetGlobal(qualifiedGlobalName);
        emitPushInt(0);
        emitRet();

        PropertyInfo p = new PropertyInfo();
        p.simpleName = name;
        p.classOwner = className;
        p.scope = PropertyScope.STATIC;
        p.isPublic = isPublic;
        p.getterFunctionName = getName;
        p.setterFunctionName = setName;
        classProps.put(name, p);
    }

    public void emitGetStaticProperty(String className, String propName) throws IOException {
        PropertyInfo p = lookupStaticProperty(className, propName);
        checkPrivateAccess(p);
        emitCall(p.getterFunctionName);
    }

    public void emitSetStaticProperty(String className, String propName) throws IOException {
        PropertyInfo p = lookupStaticProperty(className, propName);
        checkPrivateAccess(p);
        emitCall(p.setterFunctionName);
    }

    private PropertyInfo lookupStaticProperty(String className, String propName) {
        Map<String, PropertyInfo> classProps = staticProperties.get(className);
        if (classProps == null) {
            throw new RuntimeException("La clase " + className + " no tiene properties estáticas declaradas");
        }
        PropertyInfo p = classProps.get(propName);
        if (p == null) throw new RuntimeException("Property estática no encontrada en " + className + ": " + propName);
        return p;
    }

    /**
     * Compile-time check de visibilidad. Una property privada de la clase X
     * solo puede emitirse desde un método de X (currentClass.name == X).
     * Las properties de módulo privadas se enforzan automáticamente por no
     * estar en exports (cross-module no puede importarlas).
     */
    private void checkPrivateAccess(PropertyInfo p) {
        if (p.isPublic) return;
        if (p.classOwner == null) return; // module-private: ya enforced por no-exportar
        // Para instance/static private: el emit debe ocurrir dentro de un método
        // de la misma clase (o de una subclase, para simplicidad lo permitimos).
        if (currentClass == null) {
            throw new RuntimeException("Acceso a property privada '" + p.simpleName
                    + "' de " + p.classOwner + " fuera del contexto de la clase");
        }
        // Permitir la propia clase o cualquiera de sus ancestros (Java-like)
        String cur = currentClass.name;
        while (cur != null) {
            if (cur.equals(p.classOwner)) return;
            ClassInfo info = classes.get(cur);
            cur = (info != null) ? info.parent : null;
        }
        throw new RuntimeException("Acceso a property privada '" + p.simpleName
                + "' de " + p.classOwner + " desde la clase " + currentClass.name);
    }

    // --- Escritura final ---

    public void writeToFile(String filename) throws IOException {
        if (lastFunctionName != null) {
            functionLocalsBytesMap.put(lastFunctionName, currentLocalsBytes);
            functionLabelMaps.put(lastFunctionName, currentLabelMap);
            captureFunctionVarsDbg(lastFunctionName);
        }

        importOut.writeInt(externalFunctions.size());
        for (ExternalRef ext : externalFunctions) {
            importOut.writeUTF(ext.name);
            importOut.writeUTF(ext.fromPath);
        }
        importOut.flush();

        byte[] rawCode = codeStream.toByteArray();

        for (Map.Entry<String, Integer> entry : functionEnterOperandPos.entrySet()) {
            int idx = entry.getValue();
            int localsBytes = functionLocalsBytesMap.getOrDefault(entry.getKey(), 0);
            if (localsBytes < 0 || localsBytes > 0xFFFF) {
                throw new RuntimeException("Tamaño de locales fuera de rango para "
                        + entry.getKey() + ": " + localsBytes);
            }
            rawCode[idx]     = (byte) ((localsBytes >> 8) & 0xFF);
            rawCode[idx + 1] = (byte) (localsBytes & 0xFF);
        }

        for (CallFixup fixup : callFixups) {
            Integer relativeAddr = functionAddresses.get(fixup.targetFunction);
            if (relativeAddr == null) throw new RuntimeException("Función no encontrada: " + fixup.targetFunction);
            int idx = fixup.bytecodeIndex;
            rawCode[idx]     = (byte) ((relativeAddr >> 24) & 0xFF);
            rawCode[idx + 1] = (byte) ((relativeAddr >> 16) & 0xFF);
            rawCode[idx + 2] = (byte) ((relativeAddr >> 8)  & 0xFF);
            rawCode[idx + 3] = (byte) ( relativeAddr        & 0xFF);
        }

        for (JumpFixup fixup : pendingJumps) {
            Map<Integer, Integer> labelMap = functionLabelMaps.get(fixup.functionName);
            if (labelMap == null) {
                throw new RuntimeException("No se encontró mapa de etiquetas para función " + fixup.functionName);
            }
            Integer targetAddr = labelMap.get(fixup.labelId);
            if (targetAddr == null) {
                throw new RuntimeException("Etiqueta " + fixup.labelId + " no existe en función " + fixup.functionName);
            }
            if (targetAddr == UNDECLARED) {
                throw new RuntimeException("Etiqueta " + fixup.labelId + " creada con newLabel pero nunca "
                        + "fijada con declareLabel (función " + fixup.functionName + ")");
            }
            int relativeOffset = targetAddr - fixup.instructionAddress;
            int idx = fixup.operandIndex;
            rawCode[idx]     = (byte) ((relativeOffset >> 24) & 0xFF);
            rawCode[idx + 1] = (byte) ((relativeOffset >> 16) & 0xFF);
            rawCode[idx + 2] = (byte) ((relativeOffset >> 8)  & 0xFF);
            rawCode[idx + 3] = (byte) ( relativeOffset        & 0xFF);
        }

        exportOut.writeInt(exportFunctions.size());
        for (String func : exportFunctions) {
            exportOut.writeUTF(func);
            exportOut.writeInt(functionAddresses.get(func));
        }
        // B3 v2 — subsección de data symbols exportados. Si no hay
        // ninguno, NO escribimos nada (compat con loaders viejos que no
        // esperan más bytes); si hay alguno, escribimos int count +
        // entries. Loaders nuevos detectan la subsección por bytes
        // remanentes de exportsSize.
        // L2 v3 — si hay classFixups, la subsección de data exports tiene
        // que existir aunque sea vacía, para que el loader sepa dónde
        // empieza la subsección de fixups (la detecta por bytes remanentes).
        boolean writeDataExports = !exportDataSymbols.isEmpty() || !classFixups.isEmpty();
        if (writeDataExports) {
            exportOut.writeInt(exportDataSymbols.size());
            for (String name : exportDataSymbols) {
                exportOut.writeUTF(name);
                exportOut.writeInt(dataSymbolOffset.get(name));
            }
        }
        // L2 v3 — subsección de class fixups: parejas (childClassName,
        // parentQualifiedName). El loader resuelve parentQualifiedName en
        // globalSymbolTable y parcha parentOff del descriptor del child.
        if (!classFixups.isEmpty()) {
            exportOut.writeInt(classFixups.size());
            for (ClassFixup fx : classFixups) {
                exportOut.writeUTF(fx.childClassName);
                exportOut.writeInt(dataSymbolOffset.get(fx.childClassName));
                exportOut.writeUTF(fx.parentQualified);
            }
        }
        exportOut.flush();

        int dataSize = -currentNegativeOffset;
        byte[] dataBuf = new byte[dataSize];
        int pos = 0;
        for (int i = symbolBytes.size() - 1; i >= 0; i--) {
            byte[] b = symbolBytes.get(i);
            System.arraycopy(b, 0, dataBuf, pos, b.length);
            pos += b.length;
        }

        // Resolución del entry-point:
        //   - si se llamó setMainEntry, esa función es la entrada.
        //   - si no, fallback a la función "main" (legacy).
        String entryName = (mainEntryFunction != null) ? mainEntryFunction : "main";
        Integer mainRelative = functionAddresses.get(entryName);
        int mainOffset = (mainRelative != null) ? mainRelative : -1;

        byte[] libraryBytes = (currentLibrary == null)
                ? new byte[0]
                : currentLibrary.getBytes(java.nio.charset.StandardCharsets.UTF_8);

        try (DataOutputStream out = new DataOutputStream(new FileOutputStream(filename))) {
            out.writeInt(ModFormat.MAGIC_NUMBER);
            out.writeInt(dataSize);
            out.writeInt(mainOffset);
            out.writeInt(importStream.size());
            out.writeInt(exportStream.size());
            out.writeInt(rawCode.length);
            out.writeInt(libraryBytes.length);

            out.write(libraryBytes);
            out.write(importStream.toByteArray());
            out.write(exportStream.toByteArray());
            out.write(dataBuf);
            out.write(rawCode);
        }
    }
}
