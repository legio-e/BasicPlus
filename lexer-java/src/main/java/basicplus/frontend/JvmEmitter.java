// ============================================================
// JvmEmitter.java
// Compilador de BASICPLUS a bytecode JVM (.class) usando ASM.
//
// FASE 2 — alcance:
//   Fase 1 (ya cubierto):
//     - Funciones a nivel módulo (clase del módulo, todo estático).
//     - Tipos primitivos: integer/float/string/boolean.
//     - Vars/consts de módulo, locales/parámetros.
//     - Aritmética, lógica con corto-circuito, comparaciones.
//     - if / while / do-loop / for := to step.
//     - print, return, break/continue.
//     - Inicializador de módulo + entry point Java main(String[]).
//
//   Fase 2 (este pase):
//     - Clases de usuario con campos de instancia y estáticos.
//     - Constructores (función con el mismo nombre de la clase) →
//       traducidos a <init>.
//     - Métodos de instancia y estáticos.
//     - this y super(args) / super.metodo().
//     - Herencia con extends, final.
//     - Construcción NombreClase(args) sin 'new'.
//     - Acceso a miembros (instancia y estáticos) con notación punto.
//     - null como literal de referencia.
//
//   Fuera de Fase 2 (van a Fase 3):
//     - Propiedades con get/set personalizados.
//     - Enums.
//     - switch/case.
//     - try/catch/finally + throw.
//     - Arrays y for-in.
//
// La clase del módulo se emite como "static class" (final + constructor
// private). Las clases del usuario son normales y se instancian con NEW.
// Cada clase BASICPLUS produce un archivo .class independiente.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;
import basicplus.frontend.BpType.*;
import basicplus.frontend.Symbol.*;

import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.FieldVisitor;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class JvmEmitter implements Opcodes {

    private final ModuleNode moduleAst;
    private final SemanticInfo info;

    private Path outputDir;

    // Una "ClassEmitInfo" por cada clase JVM que generamos:
    // - una para el módulo (cls=null)
    // - una por cada class_def del usuario
    private static final class ClassEmitInfo {
        final ClassSymbol cls;        // null para la clase del módulo
        final String jvmName;
        final ClassWriter cw;
        final MethodVisitor cctor;    // <clinit>
        boolean cctorHasContent;
        ClassEmitInfo(ClassSymbol cls, String jvmName, ClassWriter cw, MethodVisitor cctor) {
            this.cls = cls; this.jvmName = jvmName; this.cw = cw; this.cctor = cctor;
        }
    }

    // Referencia a un método JVM emitido (function de BASICPLUS).
    private static final class EmitMethodRef {
        final String ownerClass;
        final String name;       // nombre JVM ('<init>' para constructor, sino el nombre BP)
        final String desc;
        final boolean isStatic;
        final boolean isConstructor;
        final MethodVisitor mv;
        final FunctionSymbol fs;
        EmitMethodRef(String ownerClass, String name, String desc, boolean isStatic,
                      boolean isConstructor, MethodVisitor mv, FunctionSymbol fs) {
            this.ownerClass = ownerClass; this.name = name; this.desc = desc;
            this.isStatic = isStatic; this.isConstructor = isConstructor;
            this.mv = mv; this.fs = fs;
        }
    }

    // Referencia a un field JVM emitido.
    private static final class EmitFieldRef {
        final String ownerClass;
        final String name;
        final String desc;
        final boolean isStatic;
        final BpType bpType;
        EmitFieldRef(String ownerClass, String name, String desc, boolean isStatic, BpType bpType) {
            this.ownerClass = ownerClass; this.name = name; this.desc = desc;
            this.isStatic = isStatic; this.bpType = bpType;
        }
    }

    private ClassEmitInfo moduleClass;
    private final Map<ClassSymbol, ClassEmitInfo> userClasses = new LinkedHashMap<>();
    private final Map<FunctionSymbol, EmitMethodRef> funcRefs   = new HashMap<>();
    private final Map<VarSymbol,      EmitFieldRef> varFields   = new HashMap<>();
    private final Map<ConstSymbol,    EmitFieldRef> constFields = new HashMap<>();

    public final List<String> errors = new ArrayList<>();

    public JvmEmitter(ModuleNode moduleAst, SemanticInfo info) {
        this.moduleAst = moduleAst;
        this.info = info;
    }

    // ============================================================
    // PUNTO DE ENTRADA
    // ============================================================
    public void emitTo(Path outputDir) throws IOException {
        this.outputDir = outputDir;

        // 1) Crear las clases JVM (módulo + clases del usuario) y declarar
        //    sus campos y métodos.
        declareModuleClass();
        for (ITopLevelDecl d : moduleAst.defs)
            if (d instanceof ClassDef) declareUserClass((ClassDef) d);

        for (ITopLevelDecl d : moduleAst.defs) declareModuleMember(d);
        for (ClassDef cd : userClassDefs())
            for (ITopLevelDecl m : cd.members) declareClassMember(cd, m);

        // 2) Emitir cuerpos
        emitModuleBodies();
        for (ClassDef cd : userClassDefs()) emitUserClassBodies(cd);

        // 3) Cerrar cctors y entry point del módulo
        closeAllCctors();
        emitModuleEntryPoint();

        // 4) Escribir todos los .class
        Files.createDirectories(outputDir);
        writeClass(moduleClass);
        for (ClassEmitInfo ci : userClasses.values()) writeClass(ci);
    }

    private List<ClassDef> userClassDefs() {
        List<ClassDef> out = new ArrayList<>();
        for (ITopLevelDecl d : moduleAst.defs)
            if (d instanceof ClassDef) out.add((ClassDef) d);
        return out;
    }

    private void writeClass(ClassEmitInfo ci) throws IOException {
        ci.cw.visitEnd();
        Path out = outputDir.resolve(ci.jvmName + ".class");
        Files.write(out, ci.cw.toByteArray());
    }

    // ============================================================
    // Declaración de clases JVM (módulo + clases del usuario)
    // ============================================================
    private void declareModuleClass() {
        String name = moduleAst.name;
        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS);
        cw.visit(V1_6,
                ACC_PUBLIC | ACC_FINAL | ACC_SUPER,
                name, null, "java/lang/Object", null);

        // Constructor PRIVADO para que la clase no se pueda instanciar
        // (semántica de "static class").
        MethodVisitor ctor = cw.visitMethod(ACC_PRIVATE, "<init>", "()V", null, null);
        ctor.visitCode();
        ctor.visitVarInsn(ALOAD, 0);
        ctor.visitMethodInsn(INVOKESPECIAL, "java/lang/Object", "<init>", "()V", false);
        ctor.visitInsn(RETURN);
        ctor.visitMaxs(0, 0);
        ctor.visitEnd();

        MethodVisitor cctor = cw.visitMethod(ACC_STATIC, "<clinit>", "()V", null, null);
        cctor.visitCode();

        moduleClass = new ClassEmitInfo(null, name, cw, cctor);
    }

    /** Genera una clase JVM para un enum BASICPLUS:
     *  campos public static final long con el valor literal cada uno. */
    private void declareEnumClass(EnumDef ed) {
        Symbol s = info.declSymbols.get(ed);
        if (!(s instanceof EnumSymbol)) return;
        EnumSymbol en = (EnumSymbol) s;

        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS);
        cw.visit(V1_6, ACC_PUBLIC | ACC_FINAL | ACC_SUPER,
                en.name, null, "java/lang/Object", null);

        // Constructor privado: enum no se instancia.
        MethodVisitor ctor = cw.visitMethod(ACC_PRIVATE, "<init>", "()V", null, null);
        ctor.visitCode();
        ctor.visitVarInsn(ALOAD, 0);
        ctor.visitMethodInsn(INVOKESPECIAL, "java/lang/Object", "<init>", "()V", false);
        ctor.visitInsn(RETURN);
        ctor.visitMaxs(0, 0);
        ctor.visitEnd();

        // Para cada miembro: public static final long X = <valor>
        for (Map.Entry<String, Long> e : en.values.entrySet()) {
            FieldVisitor fv = cw.visitField(
                    ACC_PUBLIC | ACC_STATIC | ACC_FINAL,
                    e.getKey(), "J", null, e.getValue());
            fv.visitEnd();
        }

        // Generamos un cctor vacío para uniformidad (algunos verifiers lo esperan
        // si hay fields finales; con valor inicial inline no es estrictamente
        // necesario, pero no sobra).
        MethodVisitor cctor = cw.visitMethod(ACC_STATIC, "<clinit>", "()V", null, null);
        cctor.visitCode();
        cctor.visitInsn(RETURN);
        cctor.visitMaxs(0, 0);
        cctor.visitEnd();

        cw.visitEnd();

        try {
            Files.createDirectories(outputDir);
            Files.write(outputDir.resolve(en.name + ".class"), cw.toByteArray());
        } catch (IOException ex) {
            errors.add("error escribiendo enum " + en.name + ": " + ex.getMessage());
        }
    }

    private void declareUserClass(ClassDef cd) {
        Symbol s = info.declSymbols.get(cd);
        if (!(s instanceof ClassSymbol)) return;
        ClassSymbol cls = (ClassSymbol) s;

        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS);
        String baseJvm = (cls.baseClass != null) ? jvmNameOfClass(cls.baseClass) : "java/lang/Object";
        int access = ACC_PUBLIC | ACC_SUPER;
        cw.visit(V1_6, access, cls.name, null, baseJvm, null);

        MethodVisitor cctor = cw.visitMethod(ACC_STATIC, "<clinit>", "()V", null, null);
        cctor.visitCode();

        userClasses.put(cls, new ClassEmitInfo(cls, cls.name, cw, cctor));
    }

    // ============================================================
    // Declaración de miembros (campos y métodos)
    // ============================================================
    private void declareModuleMember(ITopLevelDecl d) {
        if (d instanceof FuncDef)         declareModuleFunc((FuncDef) d);
        else if (d instanceof VarDecl)    declareModuleVar((VarDecl) d);
        else if (d instanceof ConstDecl)  declareModuleConst((ConstDecl) d);
        else if (d instanceof PropertyDef) {
            PropertyDef p = (PropertyDef) d;
            errors.add("[" + p.line + ":" + p.column + "] FASE2: propiedades a nivel módulo no soportadas");
        }
        else if (d instanceof EnumDef) {
            declareEnumClass((EnumDef) d);
        }
        // ClassDef ya se procesa en declareUserClass
    }

    private void declareModuleFunc(FuncDef f) {
        Symbol sym = info.declSymbols.get(f);
        if (!(sym instanceof FunctionSymbol)) return;
        FunctionSymbol fs = (FunctionSymbol) sym;
        String desc = methodDescriptor(fs);
        MethodVisitor mv = moduleClass.cw.visitMethod(
                ACC_PUBLIC | ACC_STATIC, fs.name, desc, null, null);
        funcRefs.put(fs, new EmitMethodRef(moduleClass.jvmName, fs.name, desc, true, false, mv, fs));
    }

    private void declareModuleVar(VarDecl v) {
        if (v.names.size() != 1) {
            errors.add("[" + v.line + ":" + v.column + "] FASE1: var con varios nombres no soportada");
            return;
        }
        DeclName n = v.names.get(0);
        Symbol s = info.declSymbols.get(v);
        if (!(s instanceof VarSymbol)) return;
        VarSymbol vs = (VarSymbol) s;
        String d = desc(vs.type);
        FieldVisitor fv = moduleClass.cw.visitField(ACC_PUBLIC | ACC_STATIC, n.name, d, null, null);
        fv.visitEnd();
        varFields.put(vs, new EmitFieldRef(moduleClass.jvmName, n.name, d, true, vs.type));
    }

    private void declareModuleConst(ConstDecl c) {
        Symbol s = info.declSymbols.get(c);
        if (!(s instanceof ConstSymbol)) return;
        ConstSymbol cs = (ConstSymbol) s;
        String d = desc(cs.type);
        FieldVisitor fv = moduleClass.cw.visitField(
                ACC_PUBLIC | ACC_STATIC | ACC_FINAL, c.name.name, d, null, null);
        fv.visitEnd();
        constFields.put(cs, new EmitFieldRef(moduleClass.jvmName, c.name.name, d, true, cs.type));
    }

    private void declareClassMember(ClassDef cd, ITopLevelDecl m) {
        Symbol cs = info.declSymbols.get(cd);
        if (!(cs instanceof ClassSymbol)) return;
        ClassSymbol cls = (ClassSymbol) cs;
        ClassEmitInfo ci = userClasses.get(cls);
        if (ci == null) return;

        if (m instanceof FuncDef)         declareClassFunc(cls, ci, (FuncDef) m);
        else if (m instanceof VarDecl)    declareClassVar(cls, ci, (VarDecl) m);
        else if (m instanceof ConstDecl)  declareClassConst(cls, ci, (ConstDecl) m);
        else if (m instanceof PropertyDef) {
            declareClassProperty(cls, ci, (PropertyDef) m);
        }
    }

    /** Para una propiedad genera: campo backing privado + getter + setter. */
    private void declareClassProperty(ClassSymbol cls, ClassEmitInfo ci, PropertyDef p) {
        Symbol s = info.declSymbols.get(p);
        if (!(s instanceof PropertySymbol)) return;
        PropertySymbol ps = (PropertySymbol) s;
        String fieldName = "_" + p.name.name;       // backing field (privado)
        String d = desc(ps.type);
        // Campo backing
        int fAccess = ACC_PRIVATE;
        if (ps.isStatic) fAccess |= ACC_STATIC;
        FieldVisitor fv = ci.cw.visitField(fAccess, fieldName, d, null, null);
        fv.visitEnd();
        propBacking.put(ps, new EmitFieldRef(ci.jvmName, fieldName, d, ps.isStatic, ps.type));

        // Getter: get<Name>(): T
        String gName = "get_" + p.name.name;
        int mAccess = ps.isPublic ? ACC_PUBLIC : ACC_PRIVATE;
        if (ps.isStatic) mAccess |= ACC_STATIC;
        if (ps.isFinal && !ps.isStatic) mAccess |= ACC_FINAL;
        MethodVisitor gmv = ci.cw.visitMethod(mAccess, gName, "()" + d, null, null);
        propGetters.put(ps, new EmitMethodRef(ci.jvmName, gName, "()" + d, ps.isStatic, false, gmv, null));

        // Setter: set<Name>(T): V
        String sName = "set_" + p.name.name;
        MethodVisitor smv = ci.cw.visitMethod(mAccess, sName, "(" + d + ")V", null, null);
        propSetters.put(ps, new EmitMethodRef(ci.jvmName, sName, "(" + d + ")V", ps.isStatic, false, smv, null));
    }

    private final Map<PropertySymbol, EmitFieldRef>  propBacking = new HashMap<>();
    private final Map<PropertySymbol, EmitMethodRef> propGetters = new HashMap<>();
    private final Map<PropertySymbol, EmitMethodRef> propSetters = new HashMap<>();

    private void declareClassFunc(ClassSymbol cls, ClassEmitInfo ci, FuncDef f) {
        Symbol s = info.declSymbols.get(f);
        if (!(s instanceof FunctionSymbol)) return;
        FunctionSymbol fs = (FunctionSymbol) s;
        boolean isCtor   = fs.isConstructor;
        boolean isStatic = fs.isStatic;
        String jvmName = isCtor ? "<init>" : fs.name;
        String desc = isCtor ? constructorDescriptor(fs) : methodDescriptor(fs);

        int access = fs.isPublic ? ACC_PUBLIC : ACC_PRIVATE;
        if (isStatic) access |= ACC_STATIC;
        if (fs.isFinal && !isCtor && !isStatic) access |= ACC_FINAL;

        MethodVisitor mv = ci.cw.visitMethod(access, jvmName, desc, null, null);
        funcRefs.put(fs, new EmitMethodRef(ci.jvmName, jvmName, desc, isStatic, isCtor, mv, fs));
    }

    private void declareClassVar(ClassSymbol cls, ClassEmitInfo ci, VarDecl v) {
        if (v.names.size() != 1) {
            errors.add("[" + v.line + ":" + v.column + "] FASE2: var con varios nombres no soportada");
            return;
        }
        DeclName n = v.names.get(0);
        Symbol s = info.declSymbols.get(v);
        if (!(s instanceof VarSymbol)) return;
        VarSymbol vs = (VarSymbol) s;
        String d = desc(vs.type);
        int access = vs.isPublic ? ACC_PUBLIC : ACC_PRIVATE;
        if (vs.isStatic) access |= ACC_STATIC;
        FieldVisitor fv = ci.cw.visitField(access, n.name, d, null, null);
        fv.visitEnd();
        varFields.put(vs, new EmitFieldRef(ci.jvmName, n.name, d, vs.isStatic, vs.type));
    }

    private void declareClassConst(ClassSymbol cls, ClassEmitInfo ci, ConstDecl c) {
        Symbol s = info.declSymbols.get(c);
        if (!(s instanceof ConstSymbol)) return;
        ConstSymbol cs = (ConstSymbol) s;
        String d = desc(cs.type);
        int access = (cs.isPublic ? ACC_PUBLIC : ACC_PRIVATE) | ACC_FINAL;
        if (cs.isStatic) access |= ACC_STATIC;
        FieldVisitor fv = ci.cw.visitField(access, c.name.name, d, null, null);
        fv.visitEnd();
        constFields.put(cs, new EmitFieldRef(ci.jvmName, c.name.name, d, cs.isStatic, cs.type));
    }

    // ============================================================
    // Cuerpos
    // ============================================================
    private void emitModuleBodies() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof FuncDef) {
                FuncDef f = (FuncDef) d;
                Symbol s = info.declSymbols.get(f);
                if (s instanceof FunctionSymbol)
                    emitFuncBody(f, (FunctionSymbol) s, null);
            } else if (d instanceof VarDecl) {
                VarDecl v = (VarDecl) d;
                if (v.init != null && v.names.size() == 1) {
                    Symbol s = info.declSymbols.get(v);
                    if (s instanceof VarSymbol && varFields.containsKey(s)) {
                        EmitFieldRef fi = varFields.get(s);
                        EmitContext ctx = new EmitContext(moduleClass.cctor, null, null, true);
                        emitExpr(moduleClass.cctor, v.init, ctx);
                        emitConvertIfNeeded(moduleClass.cctor, typeOfExpr(v.init), fi.bpType);
                        moduleClass.cctor.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
                        moduleClass.cctorHasContent = true;
                    }
                }
            } else if (d instanceof ConstDecl) {
                ConstDecl c = (ConstDecl) d;
                Symbol s = info.declSymbols.get(c);
                if (s instanceof ConstSymbol && constFields.containsKey(s)) {
                    EmitFieldRef fi = constFields.get(s);
                    EmitContext ctx = new EmitContext(moduleClass.cctor, null, null, true);
                    emitExpr(moduleClass.cctor, c.value, ctx);
                    emitConvertIfNeeded(moduleClass.cctor, typeOfExpr(c.value), fi.bpType);
                    moduleClass.cctor.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
                    moduleClass.cctorHasContent = true;
                }
            }
        }
    }

    private void emitUserClassBodies(ClassDef cd) {
        Symbol s = info.declSymbols.get(cd);
        if (!(s instanceof ClassSymbol)) return;
        ClassSymbol cls = (ClassSymbol) s;
        ClassEmitInfo ci = userClasses.get(cls);
        if (ci == null) return;

        // Si la clase NO define constructor:
        //   - Si la base tiene constructor con args, generar uno que delega
        //     a super(args) con la misma firma (herencia Python-style).
        //   - Si no, default sin args.
        boolean hasUserCtor = cls.constructor != null;
        if (!hasUserCtor) {
            FunctionSymbol baseCtor = (cls.baseClass != null) ? cls.baseClass.findConstructor() : null;
            if (baseCtor != null && !baseCtor.params.isEmpty()) {
                emitInheritedConstructor(cls, ci, baseCtor);
            } else {
                emitDefaultConstructor(cls, ci);
            }
        }

        // Recolectar inicializadores de campos de instancia (vars con init)
        // que el <init> tendrá que aplicar tras el super-call.
        List<VarDecl> instanceInits = new ArrayList<>();
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof VarDecl) {
                VarDecl v = (VarDecl) m;
                if (v.init != null && !v.names.isEmpty() && !v.names.get(0).isStatic())
                    instanceInits.add(v);
            }
        }

        // Cuerpos de propiedades (getter y setter)
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof PropertyDef) emitPropertyBodies(cls, (PropertyDef) m);
        }

        // Cuerpos de funciones (incluye constructor del usuario si existe)
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof FuncDef) {
                FuncDef f = (FuncDef) m;
                Symbol fsym = info.declSymbols.get(f);
                if (fsym instanceof FunctionSymbol) {
                    FunctionSymbol fs = (FunctionSymbol) fsym;
                    emitFuncBody(f, fs, fs.isConstructor ? instanceInits : null);
                }
            } else if (m instanceof VarDecl) {
                VarDecl v = (VarDecl) m;
                if (v.init != null && v.names.size() == 1
                        && v.names.get(0).isStatic()) {
                    // Campo estático: inicializar en el cctor.
                    Symbol vsym = info.declSymbols.get(v);
                    if (vsym instanceof VarSymbol && varFields.containsKey(vsym)) {
                        EmitFieldRef fi = varFields.get(vsym);
                        EmitContext ctx = new EmitContext(ci.cctor, cls, null, true);
                        emitExpr(ci.cctor, v.init, ctx);
                        emitConvertIfNeeded(ci.cctor, typeOfExpr(v.init), fi.bpType);
                        ci.cctor.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
                        ci.cctorHasContent = true;
                    }
                }
            } else if (m instanceof ConstDecl) {
                ConstDecl c = (ConstDecl) m;
                Symbol csym = info.declSymbols.get(c);
                if (csym instanceof ConstSymbol && constFields.containsKey(csym)) {
                    EmitFieldRef fi = constFields.get(csym);
                    EmitContext ctx = new EmitContext(ci.cctor, cls, null, true);
                    emitExpr(ci.cctor, c.value, ctx);
                    emitConvertIfNeeded(ci.cctor, typeOfExpr(c.value), fi.bpType);
                    ci.cctor.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
                    ci.cctorHasContent = true;
                }
            }
        }

        // Si hubiera un default constructor, ya lo cerramos antes del bucle
        // (lo hicimos en emitDefaultConstructor).
    }

    /** Genera el cuerpo del getter y del setter de una propiedad. */
    private void emitPropertyBodies(ClassSymbol cls, PropertyDef p) {
        Symbol s = info.declSymbols.get(p);
        if (!(s instanceof PropertySymbol)) return;
        PropertySymbol ps = (PropertySymbol) s;
        EmitFieldRef bk = propBacking.get(ps);
        EmitMethodRef gmi = propGetters.get(ps);
        EmitMethodRef smi = propSetters.get(ps);
        if (bk == null || gmi == null || smi == null) return;

        // ----- GETTER -----
        MethodVisitor gmv = gmi.mv;
        gmv.visitCode();
        EmitContext gctx = new EmitContext(gmv, cls, null, false);
        gctx.currentProperty = ps;
        if (!ps.isStatic) gctx.allocSlot(null); // 'this'

        if (p.getter != null) {
            // Getter custom: ejecutamos su cuerpo. 'field' se traduce a GETFIELD/PUTFIELD.
            for (IStmt st : p.getter.body) emitStmt(gmv, st, gctx);
            // Si el cuerpo no terminó con return, devolver default
            boolean endsR = !p.getter.body.isEmpty()
                    && p.getter.body.get(p.getter.body.size() - 1) instanceof ReturnStmt;
            if (!endsR) {
                emitDefault(gmv, ps.type);
                gmv.visitInsn(returnOpcode(ps.type));
            }
        } else {
            // Getter trivial: return field
            if (ps.isStatic) gmv.visitFieldInsn(GETSTATIC, bk.ownerClass, bk.name, bk.desc);
            else { gmv.visitVarInsn(ALOAD, 0); gmv.visitFieldInsn(GETFIELD, bk.ownerClass, bk.name, bk.desc); }
            gmv.visitInsn(returnOpcode(ps.type));
        }
        gmv.visitMaxs(0, 0);
        gmv.visitEnd();

        // ----- SETTER -----
        MethodVisitor smv = smi.mv;
        smv.visitCode();
        EmitContext sctx = new EmitContext(smv, cls, null, false);
        sctx.currentProperty = ps;
        if (!ps.isStatic) sctx.allocSlot(null); // 'this'

        // Param del setter (nombre = paramName declarado en BP)
        String paramName = (p.setter != null) ? p.setter.paramName : "value";
        ParamSymbol psParam = new ParamSymbol(paramName, p.line, p.column);
        psParam.type = ps.type;
        int paramSlot = sctx.allocSlot(ps.type);
        sctx.paramSlots.put(psParam, paramSlot);
        sctx.localNames.put(paramName, psParam);

        if (p.setter != null) {
            // Setter custom
            for (IStmt st : p.setter.body) emitStmt(smv, st, sctx);
        } else {
            // Setter trivial: field := value
            if (ps.isStatic) {
                loadLocal(smv, ps.type, paramSlot);
                smv.visitFieldInsn(PUTSTATIC, bk.ownerClass, bk.name, bk.desc);
            } else {
                smv.visitVarInsn(ALOAD, 0);
                loadLocal(smv, ps.type, paramSlot);
                smv.visitFieldInsn(PUTFIELD, bk.ownerClass, bk.name, bk.desc);
            }
        }
        smv.visitInsn(RETURN);
        smv.visitMaxs(0, 0);
        smv.visitEnd();

        sctx.localNames.remove(paramName);
    }

    /** Constructor heredado: misma firma que el del padre, solo delega.
     *  ALOAD 0; (cargar cada arg); INVOKESPECIAL <padre>.<init>(...)V; RETURN. */
    private void emitInheritedConstructor(ClassSymbol cls, ClassEmitInfo ci, FunctionSymbol baseCtor) {
        String desc = constructorDescriptor(baseCtor);
        MethodVisitor mv = ci.cw.visitMethod(ACC_PUBLIC, "<init>", desc, null, null);
        mv.visitCode();
        mv.visitVarInsn(ALOAD, 0);
        // Cargar cada parámetro respetando los slots JVM (long/double = 2)
        int slot = 1;
        for (ParamSymbol p : baseCtor.params) {
            loadLocal(mv, p.type, slot);
            slot += slotsOf(p.type);
        }
        mv.visitMethodInsn(INVOKESPECIAL, cls.baseClass.name, "<init>", desc, false);
        mv.visitInsn(RETURN);
        mv.visitMaxs(0, 0);
        mv.visitEnd();

        // Registrar este constructor sintético: lo necesitamos para que
        // emitNew(Perro, args) sepa qué firma invocar incluso si Perro no
        // tiene FunctionSymbol propia. Reutilizamos el FunctionSymbol del
        // padre como "stand-in" pero con ownerClass = cls.
        // (Truco simple: añadimos un alias en funcRefs apuntando al método
        // recién generado.)
        FunctionSymbol alias = new FunctionSymbol(cls.name, true, false, false, cls,
                baseCtor.astNode);
        alias.params.addAll(baseCtor.params);
        alias.returnType = null;
        alias.isConstructor = true;
        funcRefs.put(alias, new EmitMethodRef(cls.name, "<init>", desc, false, true, mv, alias));
        // Y que cls.constructor apunte al alias para que findConstructor lo encuentre.
        cls.constructor = alias;
    }

    /** Constructor por defecto sin args: super.<init>() y RETURN. */
    private void emitDefaultConstructor(ClassSymbol cls, ClassEmitInfo ci) {
        MethodVisitor mv = ci.cw.visitMethod(ACC_PUBLIC, "<init>", "()V", null, null);
        mv.visitCode();
        mv.visitVarInsn(ALOAD, 0);
        String baseJvm = (cls.baseClass != null) ? jvmNameOfClass(cls.baseClass) : "java/lang/Object";
        mv.visitMethodInsn(INVOKESPECIAL, baseJvm, "<init>", "()V", false);
        mv.visitInsn(RETURN);
        mv.visitMaxs(0, 0);
        mv.visitEnd();
    }

    private void closeAllCctors() {
        moduleClass.cctor.visitInsn(RETURN);
        moduleClass.cctor.visitMaxs(0, 0);
        moduleClass.cctor.visitEnd();
        for (ClassEmitInfo ci : userClasses.values()) {
            ci.cctor.visitInsn(RETURN);
            ci.cctor.visitMaxs(0, 0);
            ci.cctor.visitEnd();
        }
    }

    private void emitModuleEntryPoint() {
        MethodVisitor mv = moduleClass.cw.visitMethod(
                ACC_PUBLIC | ACC_STATIC, "main", "([Ljava/lang/String;)V", null, null);
        mv.visitCode();

        // Inicializador del módulo (si existe)
        if (info.module != null && info.module.initializer != null) {
            EmitMethodRef mi = funcRefs.get(info.module.initializer);
            if (mi != null) {
                mv.visitMethodInsn(INVOKESTATIC, mi.ownerClass, mi.name, mi.desc, false);
            }
        }
        // Main
        if (info.module != null && info.module.mainFunction != null) {
            EmitMethodRef mi = funcRefs.get(info.module.mainFunction);
            if (mi != null) {
                mv.visitMethodInsn(INVOKESTATIC, mi.ownerClass, mi.name, mi.desc, false);
            }
        } else {
            errors.add("módulo sin función Main; el ejecutable no hará nada útil");
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(0, 0);
        mv.visitEnd();
    }

    // ============================================================
    // Cuerpo de función / método / constructor
    // ============================================================
    private void emitFuncBody(FuncDef f, FunctionSymbol fs, List<VarDecl> instanceInitsForCtor) {
        EmitMethodRef mi = funcRefs.get(fs);
        if (mi == null) return;
        MethodVisitor mv = mi.mv;
        mv.visitCode();

        ClassSymbol owner = fs.ownerClass;
        boolean isStatic = mi.isStatic;
        EmitContext ctx = new EmitContext(mv, owner, fs, false);

        // Slot 0 = 'this' en métodos de instancia (incluido el constructor)
        if (!isStatic) ctx.allocSlot(/*reference*/ null); // 1 slot para 'this'

        // Parámetros
        for (ParamSymbol p : fs.params) {
            int slot = ctx.allocSlot(p.type);
            ctx.paramSlots.put(p, slot);
        }

        // Cuerpo del usuario.
        // Caso CONSTRUCTOR:
        //   - Si la primera sentencia es ExprStmt(SuperCallExpr), emitir super(args)
        //     y luego inicializadores de campos.
        //   - Si NO, emitir super.<init>() implícito (sin args), luego inicializadores.
        //   - Después emitir el resto del cuerpo (saltando el super-call si lo hubo).
        List<IStmt> body = f.body;
        int startIdx = 0;
        if (mi.isConstructor) {
            boolean userSuper = !body.isEmpty()
                    && body.get(0) instanceof ExprStmt
                    && ((ExprStmt) body.get(0)).expr instanceof SuperCallExpr;
            if (userSuper) {
                // Emite el super(args) y avanza
                ExprStmt es = (ExprStmt) body.get(0);
                emitSuperCallExpr(mv, (SuperCallExpr) es.expr, ctx);
                startIdx = 1;
            } else {
                // super.<init>() sin args
                String baseJvm = (owner != null && owner.baseClass != null)
                        ? owner.baseClass.name : "java/lang/Object";
                mv.visitVarInsn(ALOAD, 0);
                mv.visitMethodInsn(INVOKESPECIAL, baseJvm, "<init>", "()V", false);
            }
            // Inicializadores de campos de instancia
            if (instanceInitsForCtor != null) {
                for (VarDecl iv : instanceInitsForCtor) {
                    DeclName n = iv.names.get(0);
                    Symbol vsym = info.declSymbols.get(iv);
                    if (vsym instanceof VarSymbol && varFields.containsKey(vsym)) {
                        EmitFieldRef fi = varFields.get(vsym);
                        mv.visitVarInsn(ALOAD, 0);
                        emitExpr(mv, iv.init, ctx);
                        emitConvertIfNeeded(mv, typeOfExpr(iv.init), fi.bpType);
                        mv.visitFieldInsn(PUTFIELD, fi.ownerClass, fi.name, fi.desc);
                    }
                }
            }
        }

        for (int i = startIdx; i < body.size(); i++) emitStmt(mv, body.get(i), ctx);

        // Return implícito si no termina con return
        boolean endsWithReturn = !body.isEmpty()
                && body.get(body.size() - 1) instanceof ReturnStmt;
        if (!endsWithReturn) {
            BpType ret = mi.isConstructor ? VoidType.INSTANCE : fs.returnType;
            if (ret == null || ret instanceof VoidType) {
                mv.visitInsn(RETURN);
            } else {
                emitDefault(mv, ret);
                mv.visitInsn(returnOpcode(ret));
            }
        }

        mv.visitMaxs(0, 0);
        mv.visitEnd();
    }

    private void emitDefault(MethodVisitor mv, BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: mv.visitInsn(LCONST_0); return;
                case FLOAT:   mv.visitInsn(DCONST_0); return;
                case BOOLEAN: mv.visitInsn(ICONST_0); return;
                case STRING:  mv.visitLdcInsn("");    return;
            }
        }
        if (t instanceof EnumType) { mv.visitInsn(LCONST_0); return; }
        mv.visitInsn(ACONST_NULL);
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    private void emitStmt(MethodVisitor mv, IStmt s, EmitContext ctx) {
        if      (s instanceof VarDecl)      emitLocalVar(mv, (VarDecl) s, ctx);
        else if (s instanceof AssignStmt)   emitAssign(mv, (AssignStmt) s, ctx);
        else if (s instanceof IfStmt)       emitIf(mv, (IfStmt) s, ctx);
        else if (s instanceof SwitchStmt)   emitSwitch(mv, (SwitchStmt) s, ctx);
        else if (s instanceof TryStmt)      emitTry(mv, (TryStmt) s, ctx);
        else if (s instanceof ThrowStmt)    emitThrow(mv, (ThrowStmt) s, ctx);
        else if (s instanceof WhileStmt)    emitWhile(mv, (WhileStmt) s, ctx);
        else if (s instanceof DoLoopStmt)   emitDoLoop(mv, (DoLoopStmt) s, ctx);
        else if (s instanceof ForStmt)      emitFor(mv, (ForStmt) s, ctx);
        else if (s instanceof ReturnStmt)   emitReturn(mv, (ReturnStmt) s, ctx);
        else if (s instanceof PrintStmt)    emitPrint(mv, (PrintStmt) s, ctx);
        else if (s instanceof BreakStmt) {
            if (ctx.loopStack.isEmpty()) { errors.add("break fuera de bucle"); return; }
            mv.visitJumpInsn(GOTO, ctx.loopStack.peek().breakTarget);
        }
        else if (s instanceof ContinueStmt) {
            if (ctx.loopStack.isEmpty()) { errors.add("continue fuera de bucle"); return; }
            mv.visitJumpInsn(GOTO, ctx.loopStack.peek().continueTarget);
        }
        else if (s instanceof ExprStmt) {
            ExprStmt es = (ExprStmt) s;
            BpType t = typeOfExpr(es.expr);
            emitExpr(mv, es.expr, ctx);
            if (!(t instanceof VoidType)) {
                if (slotsOf(t) == 2) mv.visitInsn(POP2);
                else                  mv.visitInsn(POP);
            }
        }
    }

    private void emitLocalVar(MethodVisitor mv, VarDecl v, EmitContext ctx) {
        if (v.names.size() != 1) { errors.add("[" + v.line + ":" + v.column + "] FASE: var local con varios nombres"); return; }
        DeclName n = v.names.get(0);
        Symbol sym = info.declSymbols.get(v);
        if (!(sym instanceof VarSymbol)) return;
        VarSymbol vs = (VarSymbol) sym;
        int slot = ctx.allocSlot(vs.type);
        ctx.localSlots.put(vs, slot);
        if (v.init != null) {
            emitExpr(mv, v.init, ctx);
            emitConvertIfNeeded(mv, typeOfExpr(v.init), vs.type);
            storeLocal(mv, vs.type, slot);
        }
    }

    private void emitAssign(MethodVisitor mv, AssignStmt a, EmitContext ctx) {
        BpType lhsT = typeOfExpr(a.target);

        if (a.op == AssignOpKind.ASSIGN) {
            // Para campo de instancia: necesitamos cargar 'this' ANTES del valor.
            preparePutTarget(mv, a.target, ctx);
            emitExpr(mv, a.value, ctx);
            emitConvertIfNeeded(mv, typeOfExpr(a.value), lhsT);
            doStore(mv, a.target, ctx);
            return;
        }

        // += / -=: cargar valor actual, sumar/restar, almacenar
        // (variante simplificada: re-emite el target dos veces)
        preparePutTarget(mv, a.target, ctx);
        emitExpr(mv, a.target, ctx);
        emitExpr(mv, a.value, ctx);
        emitConvertIfNeeded(mv, typeOfExpr(a.value), lhsT);

        if (lhsT instanceof PrimitiveType
                && ((PrimitiveType) lhsT).tag == PrimitiveType.Kind.STRING
                && a.op == AssignOpKind.PLUS_ASSIGN) {
            mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "concat",
                    "(Ljava/lang/String;)Ljava/lang/String;", false);
        } else {
            int op;
            if (lhsT instanceof PrimitiveType
                    && ((PrimitiveType) lhsT).tag == PrimitiveType.Kind.FLOAT)
                op = (a.op == AssignOpKind.PLUS_ASSIGN) ? DADD : DSUB;
            else
                op = (a.op == AssignOpKind.PLUS_ASSIGN) ? LADD : LSUB;
            mv.visitInsn(op);
        }
        doStore(mv, a.target, ctx);
    }

    /**
     * Para PUTFIELD necesitamos 'this' (o el objeto) en pila ANTES del valor.
     * Para PUTSTATIC y locales no hace falta nada antes.
     */
    private void preparePutTarget(MethodVisitor mv, IExpr target, EmitContext ctx) {
        if (target instanceof FieldExpr) {
            if (ctx.currentProperty != null && !ctx.currentProperty.isStatic)
                mv.visitVarInsn(ALOAD, 0);
            return;
        }
        if (target instanceof IndexExpr) {
            IndexExpr ix = (IndexExpr) target;
            emitExpr(mv, ix.target, ctx);   // arrayref
            emitExpr(mv, ix.index, ctx);    // index (long)
            BpType idxT = typeOfExpr(ix.index);
            if (idxT instanceof PrimitiveType
                    && ((PrimitiveType) idxT).tag == PrimitiveType.Kind.INTEGER) {
                mv.visitInsn(L2I);
            }
            return;
        }
        if (target instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) target;
            // Si es acceso a estático (Class.field), no se necesita 'this'.
            // Si es acceso de instancia, hay que poner el target.
            // Resolveremos: si target.target resuelve a ClassSymbol, es estático.
            if (isStaticAccess(ma)) return;
            emitExpr(mv, ma.target, ctx);
        } else if (target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) target;
            // Si es campo de instancia de la clase actual, push 'this'.
            FieldKind fk = identKind(id, ctx);
            if (fk == FieldKind.INSTANCE_FIELD_OF_THIS) mv.visitVarInsn(ALOAD, 0);
        }
        // Locales y param: no requieren preparación (storeLocal solo necesita el valor).
        // Static field por nombre (sin prefijo): no requiere preparación.
    }

    private void doStore(MethodVisitor mv, IExpr target, EmitContext ctx) {
        if (target instanceof FieldExpr) {
            if (ctx.currentProperty == null) {
                errors.add("[" + ((FieldExpr) target).line + ":" + ((FieldExpr) target).column + "] 'field' fuera de get/set");
                mv.visitInsn(POP);
                return;
            }
            EmitFieldRef bk = propBacking.get(ctx.currentProperty);
            if (ctx.currentProperty.isStatic)
                mv.visitFieldInsn(PUTSTATIC, bk.ownerClass, bk.name, bk.desc);
            else
                mv.visitFieldInsn(PUTFIELD, bk.ownerClass, bk.name, bk.desc);
            return;
        }
        if (target instanceof IndexExpr) {
            IndexExpr ix = (IndexExpr) target;
            BpType arrT = typeOfExpr(ix.target);
            BpType elemT = (arrT instanceof ArrayType) ? ((ArrayType) arrT).element : ErrorType.INSTANCE;
            mv.visitInsn(arrayStoreOpcode(elemT));
            return;
        }
        if (target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) target;
            Symbol sym = resolveIdent(id, ctx);
            FieldKind fk = identKind(id, ctx);
            switch (fk) {
                case LOCAL: {
                    VarSymbol vs = (VarSymbol) sym;
                    storeLocal(mv, vs.type, ctx.localSlots.get(vs));
                    return;
                }
                case PARAM: {
                    ParamSymbol ps = (ParamSymbol) sym;
                    storeLocal(mv, ps.type, ctx.paramSlots.get(ps));
                    return;
                }
                case STATIC_FIELD: {
                    EmitFieldRef fi = fieldRefOf(sym);
                    mv.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
                    return;
                }
                case INSTANCE_FIELD_OF_THIS: {
                    EmitFieldRef fi = fieldRefOf(sym);
                    mv.visitFieldInsn(PUTFIELD, fi.ownerClass, fi.name, fi.desc);
                    return;
                }
                case CONSTANT:
                    errors.add("[" + id.line + ":" + id.column + "] no se puede asignar a una constante: '" + id.name + "'");
                    mv.visitInsn(POP);
                    return;
                default:
                    errors.add("[" + id.line + ":" + id.column + "] identificador no asignable: '" + id.name + "'");
                    mv.visitInsn(POP);
                    return;
            }
        }
        if (target instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) target;
            Symbol member = resolveMember(ma);
            if (member == null) {
                errors.add("[" + ma.line + ":" + ma.column + "] miembro no resuelto al asignar");
                mv.visitInsn(POP);
                return;
            }
            // Propiedad: invocar setter
            if (member instanceof PropertySymbol) {
                EmitMethodRef smi = propSetters.get(member);
                if (smi != null) {
                    if (smi.isStatic) mv.visitMethodInsn(INVOKESTATIC, smi.ownerClass, smi.name, smi.desc, false);
                    else              mv.visitMethodInsn(INVOKEVIRTUAL, smi.ownerClass, smi.name, smi.desc, false);
                    return;
                }
            }
            EmitFieldRef fi = fieldRefOf(member);
            if (fi == null) {
                errors.add("[" + ma.line + ":" + ma.column + "] '" + ma.member + "' no es un campo asignable");
                mv.visitInsn(POP);
                return;
            }
            if (isStaticAccess(ma)) mv.visitFieldInsn(PUTSTATIC, fi.ownerClass, fi.name, fi.desc);
            else                     mv.visitFieldInsn(PUTFIELD,  fi.ownerClass, fi.name, fi.desc);
            return;
        }
        errors.add("FASE2: target de asignación no soportado: " + target.getClass().getSimpleName());
        mv.visitInsn(POP);
    }

    private void emitIf(MethodVisitor mv, IfStmt iff, EmitContext ctx) {
        Label endLbl = new Label();
        Label nextLbl = new Label();
        emitExpr(mv, iff.then_.condition, ctx);
        mv.visitJumpInsn(IFEQ, nextLbl);
        for (IStmt s : iff.then_.body) emitStmt(mv, s, ctx);
        mv.visitJumpInsn(GOTO, endLbl);
        for (int i = 0; i < iff.elseIfs.size(); i++) {
            mv.visitLabel(nextLbl);
            nextLbl = new Label();
            IfClause ei = iff.elseIfs.get(i);
            emitExpr(mv, ei.condition, ctx);
            mv.visitJumpInsn(IFEQ, nextLbl);
            for (IStmt s : ei.body) emitStmt(mv, s, ctx);
            mv.visitJumpInsn(GOTO, endLbl);
        }
        mv.visitLabel(nextLbl);
        if (iff.else_ != null)
            for (IStmt s : iff.else_) emitStmt(mv, s, ctx);
        mv.visitLabel(endLbl);
    }

    /** switch sobre integer/enum (long), boolean (int), float (double) o string. */
    private void emitSwitch(MethodVisitor mv, SwitchStmt sw, EmitContext ctx) {
        BpType subjT = typeOfExpr(sw.subject);

        // Guardar el subject en un local para poder reutilizarlo.
        int slot = ctx.allocSlot(subjT);
        emitExpr(mv, sw.subject, ctx);
        storeLocal(mv, subjT, slot);

        Label endLbl = new Label();
        Label[] caseLbls = new Label[sw.cases.size()];
        for (int i = 0; i < caseLbls.length; i++) caseLbls[i] = new Label();
        Label defaultLbl = (sw.defaultBody != null) ? new Label() : endLbl;

        // Cadena de comparaciones: si subject == valueK → goto caseLblK
        for (int i = 0; i < sw.cases.size(); i++) {
            CaseClause cc = sw.cases.get(i);
            for (IExpr v : cc.values) {
                emitSwitchCompareAndJump(mv, subjT, slot, v, ctx, caseLbls[i]);
            }
        }
        // Ningún case match → goto default
        mv.visitJumpInsn(GOTO, defaultLbl);

        // Cuerpos de cada case
        for (int i = 0; i < sw.cases.size(); i++) {
            mv.visitLabel(caseLbls[i]);
            for (IStmt s : sw.cases.get(i).body) emitStmt(mv, s, ctx);
            mv.visitJumpInsn(GOTO, endLbl);
        }
        // Default
        if (sw.defaultBody != null) {
            mv.visitLabel(defaultLbl);
            for (IStmt s : sw.defaultBody) emitStmt(mv, s, ctx);
        }
        mv.visitLabel(endLbl);
    }

    /** Carga subject (del slot), evalúa value, compara, salta si igual. */
    private void emitSwitchCompareAndJump(MethodVisitor mv, BpType subjT, int slot,
                                           IExpr value, EmitContext ctx, Label target) {
        BpType valT = typeOfExpr(value);
        if (isString(subjT)) {
            // String.equals: subject.equals(value) → if true, goto target
            loadLocal(mv, subjT, slot);
            emitExpr(mv, value, ctx);
            mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "equals",
                    "(Ljava/lang/Object;)Z", false);
            mv.visitJumpInsn(IFNE, target);
            return;
        }
        // Long o enum (J): LCMP + IFEQ
        if (subjT instanceof PrimitiveType
                && ((PrimitiveType) subjT).tag == PrimitiveType.Kind.INTEGER
                || subjT instanceof EnumType) {
            loadLocal(mv, subjT, slot);
            emitExpr(mv, value, ctx);
            emitConvertIfNeeded(mv, valT, subjT);
            mv.visitInsn(LCMP);
            mv.visitJumpInsn(IFEQ, target);
            return;
        }
        // Double (float): DCMPL + IFEQ
        if (subjT instanceof PrimitiveType
                && ((PrimitiveType) subjT).tag == PrimitiveType.Kind.FLOAT) {
            loadLocal(mv, subjT, slot);
            emitExpr(mv, value, ctx);
            emitConvertIfNeeded(mv, valT, subjT);
            mv.visitInsn(DCMPL);
            mv.visitJumpInsn(IFEQ, target);
            return;
        }
        // Boolean (int): IF_ICMPEQ
        if (subjT instanceof PrimitiveType
                && ((PrimitiveType) subjT).tag == PrimitiveType.Kind.BOOLEAN) {
            loadLocal(mv, subjT, slot);
            emitExpr(mv, value, ctx);
            mv.visitJumpInsn(IF_ICMPEQ, target);
            return;
        }
        // Caso por defecto: comparar como referencia
        loadLocal(mv, subjT, slot);
        emitExpr(mv, value, ctx);
        mv.visitJumpInsn(IF_ACMPEQ, target);
    }

    /** try/catch/finally + throw.
     *  'throw expr' lanza java.lang.RuntimeException con expr.toString() como mensaje.
     *  'catch' captura java.lang.Throwable y vincula la variable a un Object.
     *
     *  Estrategia para finally completo (Fase 4):
     *    - Inline del finally en el path normal (tras body sin excepción).
     *    - Inline del finally en el path de cada catch.
     *    - Handler universal que cubre body Y catches: ejecuta finally y
     *      re-lanza la excepción (cubre el caso "excepción dentro del catch"
     *      o "excepción no atrapada por ningún catch específico").
     */
    private void emitTry(MethodVisitor mv, TryStmt tr, EmitContext ctx) {
        Label tryStart = new Label();
        Label tryEnd   = new Label();
        Label endLbl   = new Label();
        Label[] handlers = new Label[tr.catches.size()];
        for (int i = 0; i < tr.catches.size(); i++) handlers[i] = new Label();
        Label catchesEnd = new Label();
        Label universalHandler = (tr.finallyBody != null) ? new Label() : null;

        // 1) Registrar handlers de los catches (cubren solo el body).
        for (int i = 0; i < tr.catches.size(); i++) {
            mv.visitTryCatchBlock(tryStart, tryEnd, handlers[i], "java/lang/Throwable");
        }
        // 2) Si hay finally, registrar el universal que cubre body Y todos los catches.
        if (universalHandler != null) {
            mv.visitTryCatchBlock(tryStart, catchesEnd, universalHandler, null);
        }

        // 3) Body — registramos el finally en la pila para que ReturnStmt
        //    dentro del body lo ejecute antes de retornar.
        if (tr.finallyBody != null) ctx.finallyStack.push(tr.finallyBody);
        mv.visitLabel(tryStart);
        for (IStmt s : tr.body) emitStmt(mv, s, ctx);
        mv.visitLabel(tryEnd);
        if (tr.finallyBody != null) ctx.finallyStack.pop();
        // Path normal: ejecuta finally inline y salta al fin.
        if (tr.finallyBody != null)
            for (IStmt s : tr.finallyBody) emitStmt(mv, s, ctx);
        mv.visitJumpInsn(GOTO, endLbl);

        // 4) Cuerpos de los catches. Cada uno termina con finally inline + GOTO end.
        for (int i = 0; i < tr.catches.size(); i++) {
            mv.visitLabel(handlers[i]);
            CatchClause cl = tr.catches.get(i);
            if (cl.varName != null) {
                int errSlot = ctx.allocSlot(null);
                mv.visitVarInsn(ASTORE, errSlot);
                VarSymbol errSym = new VarSymbol(cl.varName, false, false, null, true,
                        cl.line, cl.column);
                errSym.type = NullType.INSTANCE;
                ctx.localSlots.put(errSym, errSlot);
                ctx.localNames.put(cl.varName, errSym);
            } else {
                mv.visitInsn(POP);
            }
            // Mientras procesamos el catch, el finally también es "pendiente"
            // para los return dentro del catch.
            if (tr.finallyBody != null) ctx.finallyStack.push(tr.finallyBody);
            for (IStmt s : cl.body) emitStmt(mv, s, ctx);
            if (tr.finallyBody != null) ctx.finallyStack.pop();
            if (cl.varName != null) ctx.localNames.remove(cl.varName);
            if (tr.finallyBody != null)
                for (IStmt s : tr.finallyBody) emitStmt(mv, s, ctx);
            mv.visitJumpInsn(GOTO, endLbl);
        }
        mv.visitLabel(catchesEnd);

        // 5) Handler universal (solo si hay finally): ejecuta finally y re-lanza.
        if (universalHandler != null) {
            mv.visitLabel(universalHandler);
            int excSlot = ctx.allocSlot(null);
            mv.visitVarInsn(ASTORE, excSlot);
            for (IStmt s : tr.finallyBody) emitStmt(mv, s, ctx);
            mv.visitVarInsn(ALOAD, excSlot);
            mv.visitInsn(ATHROW);
        }

        mv.visitLabel(endLbl);
    }

    /** throw expr — si expr es una instancia de Throwable (e.g. RuntimeError),
     *  hacemos ATHROW directo. Si es otro tipo (string típicamente), envolvemos
     *  en java.lang.RuntimeException(expr.toString()). */
    private void emitThrow(MethodVisitor mv, ThrowStmt th, EmitContext ctx) {
        BpType vt = typeOfExpr(th.value);
        boolean isThrowable = (vt instanceof ClassType)
                && isThrowableClass(((ClassType) vt).cls);
        if (isThrowable) {
            emitExpr(mv, th.value, ctx);
            mv.visitInsn(ATHROW);
            return;
        }
        mv.visitTypeInsn(NEW, "java/lang/RuntimeException");
        mv.visitInsn(DUP);
        emitExpr(mv, th.value, ctx);
        emitToStringInline(mv, vt);
        mv.visitMethodInsn(INVOKESPECIAL, "java/lang/RuntimeException", "<init>",
                "(Ljava/lang/String;)V", false);
        mv.visitInsn(ATHROW);
    }

    /** true si la clase es RuntimeError built-in o una subclase suya (transitiva). */
    private static boolean isThrowableClass(ClassSymbol cls) {
        ClassSymbol c = cls;
        while (c != null) {
            if ("RuntimeError".equals(c.name) && SemanticAnalyzer.isBuiltin(c)) return true;
            c = c.baseClass;
        }
        return false;
    }

    private void emitWhile(MethodVisitor mv, WhileStmt w, EmitContext ctx) {
        Label start = new Label(), end = new Label();
        mv.visitLabel(start);
        emitExpr(mv, w.condition, ctx);
        mv.visitJumpInsn(IFEQ, end);
        ctx.loopStack.push(new LoopFrame(start, end));
        for (IStmt s : w.body) emitStmt(mv, s, ctx);
        ctx.loopStack.pop();
        mv.visitJumpInsn(GOTO, start);
        mv.visitLabel(end);
    }

    private void emitDoLoop(MethodVisitor mv, DoLoopStmt dl, EmitContext ctx) {
        Label start = new Label(), end = new Label();
        Label cont = (dl.condition == null) ? start : new Label();
        mv.visitLabel(start);
        ctx.loopStack.push(new LoopFrame(cont, end));
        for (IStmt s : dl.body) emitStmt(mv, s, ctx);
        ctx.loopStack.pop();
        if (dl.condition == null) {
            mv.visitJumpInsn(GOTO, start);
        } else {
            mv.visitLabel(cont);
            emitExpr(mv, dl.condition, ctx);
            mv.visitJumpInsn(IFNE, start);
        }
        mv.visitLabel(end);
    }

    private void emitFor(MethodVisitor mv, ForStmt f, EmitContext ctx) {
        if (f.range instanceof ForInRange) { emitForIn(mv, f, (ForInRange) f.range, ctx); return; }
        if (!(f.range instanceof ForNumericRange)) {
            errors.add("[" + f.line + ":" + f.column + "] for con range no soportado");
            return;
        }
        ForNumericRange nr = (ForNumericRange) f.range;
        int iterSlot = ctx.allocSlot(PrimitiveType.INTEGER);
        VarSymbol iterSym = new VarSymbol(f.iteratorName, false, false, null, true, f.line, f.column);
        iterSym.type = PrimitiveType.INTEGER;
        ctx.localSlots.put(iterSym, iterSlot);
        ctx.localNames.put(f.iteratorName, iterSym);

        emitExpr(mv, nr.from, ctx);
        emitConvertIfNeeded(mv, typeOfExpr(nr.from), PrimitiveType.INTEGER);
        mv.visitVarInsn(LSTORE, iterSlot);

        int toSlot = ctx.allocSlot(PrimitiveType.INTEGER);
        emitExpr(mv, nr.to, ctx);
        emitConvertIfNeeded(mv, typeOfExpr(nr.to), PrimitiveType.INTEGER);
        mv.visitVarInsn(LSTORE, toSlot);

        int stepSlot = ctx.allocSlot(PrimitiveType.INTEGER);
        if (nr.step != null) {
            emitExpr(mv, nr.step, ctx);
            emitConvertIfNeeded(mv, typeOfExpr(nr.step), PrimitiveType.INTEGER);
        } else {
            mv.visitInsn(LCONST_1);
        }
        mv.visitVarInsn(LSTORE, stepSlot);

        Label start = new Label(), end = new Label(), cont = new Label();
        mv.visitLabel(start);
        boolean negStep = (nr.step instanceof IntLitExpr) && ((IntLitExpr) nr.step).value < 0;
        mv.visitVarInsn(LLOAD, iterSlot);
        mv.visitVarInsn(LLOAD, toSlot);
        mv.visitInsn(LCMP);
        if (negStep) mv.visitJumpInsn(IFLT, end);
        else         mv.visitJumpInsn(IFGT, end);

        ctx.loopStack.push(new LoopFrame(cont, end));
        for (IStmt s : f.body) emitStmt(mv, s, ctx);
        ctx.loopStack.pop();

        mv.visitLabel(cont);
        mv.visitVarInsn(LLOAD, iterSlot);
        mv.visitVarInsn(LLOAD, stepSlot);
        mv.visitInsn(LADD);
        mv.visitVarInsn(LSTORE, iterSlot);
        mv.visitJumpInsn(GOTO, start);
        mv.visitLabel(end);

        ctx.localNames.remove(f.iteratorName);
    }

    /** for n in arr do ...body... next n */
    private void emitForIn(MethodVisitor mv, ForStmt f, ForInRange fir, EmitContext ctx) {
        BpType collT = typeOfExpr(fir.iterable);
        if (!(collT instanceof ArrayType)) {
            errors.add("[" + f.line + ":" + f.column + "] for-in requiere array");
            return;
        }
        BpType elemT = ((ArrayType) collT).element;

        // Slots: array, idx (int), len (int), n (elemT)
        int arrSlot = ctx.allocSlot(null /*ref 1 slot*/);
        int idxSlot = ctx.allocSlot(PrimitiveType.BOOLEAN); // int (1 slot)
        int lenSlot = ctx.allocSlot(PrimitiveType.BOOLEAN);
        int nSlot   = ctx.allocSlot(elemT);

        VarSymbol nSym = new VarSymbol(f.iteratorName, false, false, null, true, f.line, f.column);
        nSym.type = elemT;
        ctx.localSlots.put(nSym, nSlot);
        ctx.localNames.put(f.iteratorName, nSym);

        // arr := <iterable>
        emitExpr(mv, fir.iterable, ctx);
        mv.visitVarInsn(ASTORE, arrSlot);
        // idx := 0
        mv.visitInsn(ICONST_0);
        mv.visitVarInsn(ISTORE, idxSlot);
        // len := arr.length
        mv.visitVarInsn(ALOAD, arrSlot);
        mv.visitInsn(ARRAYLENGTH);
        mv.visitVarInsn(ISTORE, lenSlot);

        Label start = new Label(), end = new Label(), cont = new Label();
        mv.visitLabel(start);
        // if idx >= len goto end
        mv.visitVarInsn(ILOAD, idxSlot);
        mv.visitVarInsn(ILOAD, lenSlot);
        mv.visitJumpInsn(IF_ICMPGE, end);
        // n := arr[idx]
        mv.visitVarInsn(ALOAD, arrSlot);
        mv.visitVarInsn(ILOAD, idxSlot);
        mv.visitInsn(arrayLoadOpcode(elemT));
        storeLocal(mv, elemT, nSlot);

        ctx.loopStack.push(new LoopFrame(cont, end));
        for (IStmt s : f.body) emitStmt(mv, s, ctx);
        ctx.loopStack.pop();

        mv.visitLabel(cont);
        // idx++
        mv.visitIincInsn(idxSlot, 1);
        mv.visitJumpInsn(GOTO, start);
        mv.visitLabel(end);

        ctx.localNames.remove(f.iteratorName);
    }

    private void emitArrayLit(MethodVisitor mv, ArrayLitExpr al, EmitContext ctx) {
        BpType arrT = typeOfExpr(al);
        BpType elemT = (arrT instanceof ArrayType) ? ((ArrayType) arrT).element : ErrorType.INSTANCE;
        // size
        loadInt(mv, al.elements.size());
        emitNewArray(mv, elemT);
        // Populate
        for (int i = 0; i < al.elements.size(); i++) {
            mv.visitInsn(DUP);
            loadInt(mv, i);
            emitExpr(mv, al.elements.get(i), ctx);
            emitConvertIfNeeded(mv, typeOfExpr(al.elements.get(i)), elemT);
            mv.visitInsn(arrayStoreOpcode(elemT));
        }
    }

    private void emitIndexLoad(MethodVisitor mv, IndexExpr ix, EmitContext ctx) {
        BpType arrT = typeOfExpr(ix.target);
        BpType elemT = (arrT instanceof ArrayType) ? ((ArrayType) arrT).element : ErrorType.INSTANCE;
        emitExpr(mv, ix.target, ctx);
        emitExpr(mv, ix.index, ctx);
        // El índice debe ser int en JVM; nuestro integer es long → L2I
        BpType idxT = typeOfExpr(ix.index);
        if (idxT instanceof PrimitiveType
                && ((PrimitiveType) idxT).tag == PrimitiveType.Kind.INTEGER) {
            mv.visitInsn(L2I);
        }
        mv.visitInsn(arrayLoadOpcode(elemT));
    }

    private static void loadInt(MethodVisitor mv, int v) {
        if (v >= -1 && v <= 5) {
            int op;
            switch (v) {
                case -1: op = ICONST_M1; break;
                case 0:  op = ICONST_0;  break;
                case 1:  op = ICONST_1;  break;
                case 2:  op = ICONST_2;  break;
                case 3:  op = ICONST_3;  break;
                case 4:  op = ICONST_4;  break;
                default: op = ICONST_5;  break;
            }
            mv.visitInsn(op);
        } else if (v >= Byte.MIN_VALUE && v <= Byte.MAX_VALUE) {
            mv.visitIntInsn(BIPUSH, v);
        } else if (v >= Short.MIN_VALUE && v <= Short.MAX_VALUE) {
            mv.visitIntInsn(SIPUSH, v);
        } else {
            mv.visitLdcInsn(v);
        }
    }

    private void emitNewArray(MethodVisitor mv, BpType elemT) {
        if (elemT instanceof PrimitiveType) {
            switch (((PrimitiveType) elemT).tag) {
                case INTEGER: mv.visitIntInsn(NEWARRAY, T_LONG);    return;
                case FLOAT:   mv.visitIntInsn(NEWARRAY, T_DOUBLE);  return;
                case BOOLEAN: mv.visitIntInsn(NEWARRAY, T_BOOLEAN); return;
                case STRING:  mv.visitTypeInsn(ANEWARRAY, "java/lang/String"); return;
            }
        }
        if (elemT instanceof ClassType) {
            mv.visitTypeInsn(ANEWARRAY, ((ClassType) elemT).cls.name);
            return;
        }
        if (elemT instanceof EnumType) {
            mv.visitIntInsn(NEWARRAY, T_LONG);
            return;
        }
        mv.visitTypeInsn(ANEWARRAY, "java/lang/Object");
    }

    private static int arrayLoadOpcode(BpType elemT) {
        if (elemT instanceof PrimitiveType) {
            switch (((PrimitiveType) elemT).tag) {
                case INTEGER: return LALOAD;
                case FLOAT:   return DALOAD;
                case BOOLEAN: return BALOAD;
                case STRING:  return AALOAD;
            }
        }
        if (elemT instanceof EnumType) return LALOAD;
        return AALOAD;
    }

    private static int arrayStoreOpcode(BpType elemT) {
        if (elemT instanceof PrimitiveType) {
            switch (((PrimitiveType) elemT).tag) {
                case INTEGER: return LASTORE;
                case FLOAT:   return DASTORE;
                case BOOLEAN: return BASTORE;
                case STRING:  return AASTORE;
            }
        }
        if (elemT instanceof EnumType) return LASTORE;
        return AASTORE;
    }

    private void emitReturn(MethodVisitor mv, ReturnStmt r, EmitContext ctx) {
        BpType retT = (ctx.fs != null) ? ctx.fs.returnType : null;
        if (ctx.fs != null && ctx.fs.isConstructor) {
            // Constructor: ejecutar finallys pendientes y RETURN
            runPendingFinallys(mv, ctx);
            mv.visitInsn(RETURN);
            return;
        }
        if (r.value != null) {
            emitExpr(mv, r.value, ctx);
            BpType valT = typeOfExpr(r.value);
            if (retT != null) emitConvertIfNeeded(mv, valT, retT);
            BpType actual = (retT != null) ? retT : valT;
            // Si hay finallys pendientes, guardar el valor, ejecutarlos y recargarlo
            if (!ctx.finallyStack.isEmpty()) {
                int saveSlot = ctx.allocSlot(actual);
                storeLocal(mv, actual, saveSlot);
                runPendingFinallys(mv, ctx);
                loadLocal(mv, actual, saveSlot);
            }
            mv.visitInsn(returnOpcode(actual));
        } else {
            runPendingFinallys(mv, ctx);
            if (retT == null || retT instanceof VoidType) {
                mv.visitInsn(RETURN);
            } else {
                emitDefault(mv, retT);
                mv.visitInsn(returnOpcode(retT));
            }
        }
    }

    /** Ejecuta inline todos los finally pendientes (innermost primero). */
    private void runPendingFinallys(MethodVisitor mv, EmitContext ctx) {
        // Iteración por defecto de ArrayDeque va de HEAD a TAIL — y push() añade
        // al HEAD, así que iteramos del más reciente (innermost) al más antiguo.
        for (List<IStmt> fb : ctx.finallyStack) {
            for (IStmt s : fb) emitStmt(mv, s, ctx);
        }
    }

    private void emitPrint(MethodVisitor mv, PrintStmt p, EmitContext ctx) {
        for (int i = 0; i < p.items.size(); i++) {
            PrintItem it = p.items.get(i);
            if (i > 0 && it.leadingSep == PrintSep.COMMA) {
                mv.visitFieldInsn(GETSTATIC, "java/lang/System", "out", "Ljava/io/PrintStream;");
                mv.visitLdcInsn(" ");
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "print",
                        "(Ljava/lang/String;)V", false);
            }
            if (it.expr == null) continue;
            mv.visitFieldInsn(GETSTATIC, "java/lang/System", "out", "Ljava/io/PrintStream;");
            emitExpr(mv, it.expr, ctx);
            emitToStringInline(mv, typeOfExpr(it.expr));
            mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "print",
                    "(Ljava/lang/String;)V", false);
        }
        mv.visitFieldInsn(GETSTATIC, "java/lang/System", "out", "Ljava/io/PrintStream;");
        mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "println", "()V", false);
    }

    private void emitToStringInline(MethodVisitor mv, BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case STRING: return;
                case INTEGER: mv.visitMethodInsn(INVOKESTATIC, "java/lang/Long", "toString",
                        "(J)Ljava/lang/String;", false); return;
                case FLOAT:   mv.visitMethodInsn(INVOKESTATIC, "java/lang/Double", "toString",
                        "(D)Ljava/lang/String;", false); return;
                case BOOLEAN: mv.visitMethodInsn(INVOKESTATIC, "java/lang/Boolean", "toString",
                        "(Z)Ljava/lang/String;", false); return;
            }
        }
        mv.visitMethodInsn(INVOKESTATIC, "java/lang/String", "valueOf",
                "(Ljava/lang/Object;)Ljava/lang/String;", false);
    }

    // ============================================================
    // EXPRESIONES
    // ============================================================
    private void emitExpr(MethodVisitor mv, IExpr e, EmitContext ctx) {
        if      (e instanceof IntLitExpr)    loadLong(mv, ((IntLitExpr) e).value);
        else if (e instanceof FloatLitExpr)  loadDouble(mv, ((FloatLitExpr) e).value);
        else if (e instanceof StringLitExpr) mv.visitLdcInsn(((StringLitExpr) e).value);
        else if (e instanceof BoolLitExpr)   mv.visitInsn(((BoolLitExpr) e).value ? ICONST_1 : ICONST_0);
        else if (e instanceof NullLitExpr)   mv.visitInsn(ACONST_NULL);
        else if (e instanceof ThisExpr)      mv.visitVarInsn(ALOAD, 0);
        else if (e instanceof FieldExpr)     emitFieldLoad(mv, (FieldExpr) e, ctx);
        else if (e instanceof ParenExpr)     emitExpr(mv, ((ParenExpr) e).inner, ctx);
        else if (e instanceof IdentifierExpr) emitIdentLoad(mv, (IdentifierExpr) e, ctx);
        else if (e instanceof MemberAccessExpr) emitMemberAccessLoad(mv, (MemberAccessExpr) e, ctx);
        else if (e instanceof UnaryExpr)     emitUnary(mv, (UnaryExpr) e, ctx);
        else if (e instanceof BinaryExpr)    emitBinary(mv, (BinaryExpr) e, ctx);
        else if (e instanceof CallExpr)      emitCall(mv, (CallExpr) e, ctx);
        else if (e instanceof ArrayLitExpr)  emitArrayLit(mv, (ArrayLitExpr) e, ctx);
        else if (e instanceof IndexExpr)     emitIndexLoad(mv, (IndexExpr) e, ctx);
        else if (e instanceof SuperExpr) {
            // 'super' como valor en la pila no tiene sentido; su uso útil es
            // en super.foo() o super(args), gestionados aparte. Por seguridad,
            // empujamos 'this' (ASM con V1_6 no se queja).
            mv.visitVarInsn(ALOAD, 0);
        }
        else if (e instanceof SuperCallExpr) {
            emitSuperCallExpr(mv, (SuperCallExpr) e, ctx);
        }
        else {
            errors.add("FASE2: expresión no soportada: " + e.getClass().getSimpleName());
            mv.visitInsn(ACONST_NULL);
        }
    }

    /** 'field' dentro de get/set → acceso al campo backing de la propiedad actual. */
    private void emitFieldLoad(MethodVisitor mv, FieldExpr fe, EmitContext ctx) {
        if (ctx.currentProperty == null) {
            errors.add("[" + fe.line + ":" + fe.column + "] 'field' fuera de get/set");
            mv.visitInsn(ACONST_NULL);
            return;
        }
        EmitFieldRef bk = propBacking.get(ctx.currentProperty);
        if (ctx.currentProperty.isStatic) {
            mv.visitFieldInsn(GETSTATIC, bk.ownerClass, bk.name, bk.desc);
        } else {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitFieldInsn(GETFIELD, bk.ownerClass, bk.name, bk.desc);
        }
    }

    private void emitIdentLoad(MethodVisitor mv, IdentifierExpr id, EmitContext ctx) {
        Symbol sym = resolveIdent(id, ctx);
        if (sym == null) {
            errors.add("[" + id.line + ":" + id.column + "] identificador no resuelto: '" + id.name + "'");
            mv.visitInsn(ACONST_NULL);
            return;
        }
        // Casos
        if (sym instanceof VarSymbol) {
            VarSymbol vs = (VarSymbol) sym;
            if (ctx.localSlots.containsKey(vs)) {
                loadLocal(mv, vs.type, ctx.localSlots.get(vs));
                return;
            }
            EmitFieldRef fi = varFields.get(vs);
            if (fi != null) {
                if (fi.isStatic) mv.visitFieldInsn(GETSTATIC, fi.ownerClass, fi.name, fi.desc);
                else {
                    mv.visitVarInsn(ALOAD, 0);
                    mv.visitFieldInsn(GETFIELD, fi.ownerClass, fi.name, fi.desc);
                }
                return;
            }
            errors.add("[" + id.line + ":" + id.column + "] var sin slot ni field: '" + id.name + "'");
            mv.visitInsn(ACONST_NULL);
            return;
        }
        if (sym instanceof ParamSymbol) {
            ParamSymbol ps = (ParamSymbol) sym;
            if (ctx.paramSlots.containsKey(ps)) {
                loadLocal(mv, ps.type, ctx.paramSlots.get(ps));
                return;
            }
            errors.add("[" + id.line + ":" + id.column + "] param sin slot: '" + id.name + "'");
            mv.visitInsn(ACONST_NULL);
            return;
        }
        if (sym instanceof ConstSymbol) {
            EmitFieldRef fi = constFields.get(sym);
            if (fi != null) {
                if (fi.isStatic) mv.visitFieldInsn(GETSTATIC, fi.ownerClass, fi.name, fi.desc);
                else {
                    mv.visitVarInsn(ALOAD, 0);
                    mv.visitFieldInsn(GETFIELD, fi.ownerClass, fi.name, fi.desc);
                }
                return;
            }
        }
        if (sym instanceof PropertySymbol) {
            // Acceso a propiedad sin prefijo (this.prop implícito)
            PropertySymbol ps = (PropertySymbol) sym;
            EmitMethodRef gmi = propGetters.get(ps);
            if (gmi != null) {
                if (gmi.isStatic) {
                    mv.visitMethodInsn(INVOKESTATIC, gmi.ownerClass, gmi.name, gmi.desc, false);
                } else {
                    mv.visitVarInsn(ALOAD, 0);
                    mv.visitMethodInsn(INVOKEVIRTUAL, gmi.ownerClass, gmi.name, gmi.desc, false);
                }
                return;
            }
        }
        if (sym instanceof ClassSymbol) {
            // Identificador suelto que es una clase: solo sirve como receptor
            // de . (se resuelve en MemberAccess) o de llamada (constructor en
            // emitCall). En lone form no produce valor; emitimos ACONST_NULL.
            mv.visitInsn(ACONST_NULL);
            return;
        }
        errors.add("[" + id.line + ":" + id.column + "] no sé cargar '" + id.name + "'");
        mv.visitInsn(ACONST_NULL);
    }

    private void emitMemberAccessLoad(MethodVisitor mv, MemberAccessExpr ma, EmitContext ctx) {
        // Acceso a miembro de enum: Dia.LUNES → GETSTATIC Dia.LUNES J
        if (ma.target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ma.target;
            Symbol idSym = info.exprSymbols.get(id);
            if (idSym == null) idSym = lookupTypeByName(id.name);
            if (idSym instanceof EnumSymbol) {
                EnumSymbol en = (EnumSymbol) idSym;
                if (en.values.containsKey(ma.member)) {
                    mv.visitFieldInsn(GETSTATIC, en.name, ma.member, "J");
                    return;
                }
                errors.add("[" + ma.line + ":" + ma.column + "] enum '" + en.name + "' no tiene '" + ma.member + "'");
                mv.visitInsn(LCONST_0);
                return;
            }
        }

        // Acceso estático: ClassName.field o ClassName.METHOD?
        if (ma.target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ma.target;
            Symbol idSym = info.exprSymbols.get(id);
            if (idSym == null) idSym = lookupTypeByName(id.name);
            if (idSym instanceof ClassSymbol) {
                ClassSymbol cs = (ClassSymbol) idSym;
                Symbol member = cs.lookupStatic(ma.member);
                if (member == null) {
                    errors.add("[" + ma.line + ":" + ma.column + "] estático no encontrado: '" + cs.name + "." + ma.member + "'");
                    mv.visitInsn(ACONST_NULL);
                    return;
                }
                EmitFieldRef fi = fieldRefOf(member);
                if (fi != null) {
                    mv.visitFieldInsn(GETSTATIC, fi.ownerClass, fi.name, fi.desc);
                    return;
                }
                // Propiedad estática: invocar getter
                if (member instanceof PropertySymbol) {
                    EmitMethodRef gmi = propGetters.get(member);
                    if (gmi != null) {
                        mv.visitMethodInsn(INVOKESTATIC, gmi.ownerClass, gmi.name, gmi.desc, false);
                        return;
                    }
                }
                errors.add("[" + ma.line + ":" + ma.column + "] '" + cs.name + "." + ma.member + "' no es un campo");
                mv.visitInsn(ACONST_NULL);
                return;
            }
        }
        // Acceso de instancia: emit target, luego GETFIELD o INVOKEVIRTUAL getter
        BpType tt = typeOfExpr(ma.target);
        if (tt instanceof ClassType) {
            ClassType ct = (ClassType) tt;
            Symbol member = ct.cls.lookupInstance(ma.member);
            if (member == null) {
                errors.add("[" + ma.line + ":" + ma.column + "] miembro de instancia '" + ma.member + "' no encontrado en '" + ct.cls.name + "'");
                mv.visitInsn(ACONST_NULL);
                return;
            }
            EmitFieldRef fi = fieldRefOf(member);
            if (fi != null) {
                emitExpr(mv, ma.target, ctx);
                mv.visitFieldInsn(GETFIELD, fi.ownerClass, fi.name, fi.desc);
                return;
            }
            // Propiedad de instancia: invocar getter
            if (member instanceof PropertySymbol) {
                EmitMethodRef gmi = propGetters.get(member);
                if (gmi != null) {
                    emitExpr(mv, ma.target, ctx);
                    mv.visitMethodInsn(INVOKEVIRTUAL, gmi.ownerClass, gmi.name, gmi.desc, false);
                    return;
                }
            }
            errors.add("[" + ma.line + ":" + ma.column + "] '" + ct.cls.name + "." + ma.member + "' no es un campo");
            mv.visitInsn(ACONST_NULL);
            return;
        }
        errors.add("[" + ma.line + ":" + ma.column + "] no se puede acceder a miembro de '" + tt.display() + "'");
        mv.visitInsn(ACONST_NULL);
    }

    private void emitUnary(MethodVisitor mv, UnaryExpr u, EmitContext ctx) {
        emitExpr(mv, u.operand, ctx);
        if ("-".equals(u.op)) {
            BpType t = typeOfExpr(u.operand);
            if (t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.FLOAT)
                mv.visitInsn(DNEG);
            else
                mv.visitInsn(LNEG);
        } else if ("not".equals(u.op)) {
            mv.visitInsn(ICONST_1); mv.visitInsn(IXOR);
        } else {
            errors.add("unario no soportado: '" + u.op + "'");
        }
    }

    private void emitBinary(MethodVisitor mv, BinaryExpr b, EmitContext ctx) {
        if ("and".equals(b.op)) { emitShortCircuit(mv, b, ctx, true);  return; }
        if ("or".equals(b.op))  { emitShortCircuit(mv, b, ctx, false); return; }

        BpType lt = typeOfExpr(b.left);
        BpType rt = typeOfExpr(b.right);

        // == y != especiales para referencias / null / strings
        if (("==".equals(b.op) || "!=".equals(b.op))
                && (isRefOrNullOrString(lt) || isRefOrNullOrString(rt))) {
            emitEqualityRefOrString(mv, b, ctx, lt, rt, "!=".equals(b.op));
            return;
        }

        if ("+".equals(b.op) && (isString(lt) || isString(rt))) {
            emitExpr(mv, b.left, ctx);  emitToStringInline(mv, lt);
            emitExpr(mv, b.right, ctx); emitToStringInline(mv, rt);
            mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "concat",
                    "(Ljava/lang/String;)Ljava/lang/String;", false);
            return;
        }

        BpType common = numericPromotion(lt, rt);
        emitExpr(mv, b.left, ctx);
        emitConvertIfNeeded(mv, lt, common);
        emitExpr(mv, b.right, ctx);
        emitConvertIfNeeded(mv, rt, common);
        boolean isFloat = (common instanceof PrimitiveType
                && ((PrimitiveType) common).tag == PrimitiveType.Kind.FLOAT);

        switch (b.op) {
            case "+":   mv.visitInsn(isFloat ? DADD : LADD); return;
            case "-":   mv.visitInsn(isFloat ? DSUB : LSUB); return;
            case "*":   mv.visitInsn(isFloat ? DMUL : LMUL); return;
            case "/":   mv.visitInsn(isFloat ? DDIV : LDIV); return;
            case "mod": mv.visitInsn(LREM); return;
            case "&":   mv.visitInsn(LAND); return;
            case "|":   mv.visitInsn(LOR);  return;
            case "xor": mv.visitInsn(LXOR); return;
            case "shl": mv.visitInsn(L2I); mv.visitInsn(LSHL); return;
            case "shr": mv.visitInsn(L2I); mv.visitInsn(LSHR); return;
            case "==": case "!=": case "<": case ">": case "<=": case ">=":
                emitCompareToBool(mv, b.op, isFloat); return;
            default:
                errors.add("operador no soportado: '" + b.op + "'");
        }
    }

    private void emitCompareToBool(MethodVisitor mv, String op, boolean isFloat) {
        if (isFloat) mv.visitInsn(DCMPL);
        else         mv.visitInsn(LCMP);
        int branchOp;
        switch (op) {
            case "==": branchOp = IFEQ; break;
            case "!=": branchOp = IFNE; break;
            case "<":  branchOp = IFLT; break;
            case ">":  branchOp = IFGT; break;
            case "<=": branchOp = IFLE; break;
            case ">=": branchOp = IFGE; break;
            default:   branchOp = IFEQ; break;
        }
        Label trueL = new Label(), endL = new Label();
        mv.visitJumpInsn(branchOp, trueL);
        mv.visitInsn(ICONST_0);
        mv.visitJumpInsn(GOTO, endL);
        mv.visitLabel(trueL);
        mv.visitInsn(ICONST_1);
        mv.visitLabel(endL);
    }

    /** Igualdad/desigualdad cuando alguno de los operandos es referencia/null/string.
     *  MARKER-FIX-001 — si ves esto, el archivo está sincronizado correctamente. */
    private void emitEqualityRefOrString(MethodVisitor mv, BinaryExpr b, EmitContext ctx,
                                          BpType lt, BpType rt, boolean negate) {
        // Para strings: usar String.equals (igualdad estructural).
        if (isString(lt) && isString(rt)) {
            emitExpr(mv, b.left, ctx);
            emitExpr(mv, b.right, ctx);
            mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "equals",
                    "(Ljava/lang/Object;)Z", false);
            if (negate) {
                mv.visitInsn(ICONST_1);
                mv.visitInsn(IXOR);
            }
            return;
        }
        // Refs / null: IF_ACMPEQ / IF_ACMPNE
        emitExpr(mv, b.left, ctx);
        emitExpr(mv, b.right, ctx);
        Label trueL = new Label(), endL = new Label();
        int branchOp = negate ? IF_ACMPNE : IF_ACMPEQ;
        mv.visitJumpInsn(branchOp, trueL);
        mv.visitInsn(ICONST_0);
        mv.visitJumpInsn(GOTO, endL);
        mv.visitLabel(trueL);
        mv.visitInsn(ICONST_1);
        mv.visitLabel(endL);
    }

    private static boolean isRefOrNullOrString(BpType t) {
        if (t instanceof ClassType || t instanceof NullType || t instanceof ArrayType) return true;
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
    }

    private void emitShortCircuit(MethodVisitor mv, BinaryExpr b, EmitContext ctx, boolean andSemantics) {
        Label end = new Label(), shortL = new Label();
        emitExpr(mv, b.left, ctx);
        mv.visitJumpInsn(andSemantics ? IFEQ : IFNE, shortL);
        emitExpr(mv, b.right, ctx);
        mv.visitJumpInsn(GOTO, end);
        mv.visitLabel(shortL);
        mv.visitInsn(andSemantics ? ICONST_0 : ICONST_1);
        mv.visitLabel(end);
    }

    // ============================================================
    // Llamadas: estático / método de instancia / construcción / super
    // ============================================================
    private void emitCall(MethodVisitor mv, CallExpr ce, EmitContext ctx) {
        // 1) Construcción NombreClase(args)
        if (ce.callee instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ce.callee;
            Symbol resolved = info.exprSymbols.get(id);
            if (resolved == null) resolved = lookupTypeByName(id.name);
            if (resolved instanceof ClassSymbol) {
                emitNew(mv, (ClassSymbol) resolved, ce, ctx);
                return;
            }
        }

        // 2) Llamada por nombre: function de módulo, método estático, o método
        //    de instancia de la clase actual (this implícito).
        if (ce.callee instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ce.callee;
            FunctionSymbol fn = findFunctionByName(id.name);
            if (fn != null) {
                EmitMethodRef mi = funcRefs.get(fn);
                if (mi == null) { errors.add("[" + id.line + ":" + id.column + "] sin metodo emit: " + id.name); mv.visitInsn(ACONST_NULL); return; }

                if (mi.isStatic) {
                    // Static: solo args.
                    pushArgs(mv, ce, fn, ctx);
                    mv.visitMethodInsn(INVOKESTATIC, mi.ownerClass, mi.name, mi.desc, false);
                } else {
                    // Método de instancia: this implícito.
                    mv.visitVarInsn(ALOAD, 0);
                    pushArgs(mv, ce, fn, ctx);
                    mv.visitMethodInsn(INVOKEVIRTUAL, mi.ownerClass, mi.name, mi.desc, false);
                }
                return;
            }
            // ¿Built-in stdlib?
            if (emitBuiltinCall(mv, id.name, ce, ctx)) return;
            errors.add("[" + id.line + ":" + id.column + "] función no encontrada: '" + id.name + "'");
            mv.visitInsn(ACONST_NULL);
            return;
        }

        // 3) Llamada vía MemberAccess: obj.metodo(args) o Class.metodo(args) o super.metodo(args)
        if (ce.callee instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) ce.callee;

            // super.metodo(args) — INVOKESPECIAL al método de la base
            if (ma.target instanceof SuperExpr) {
                if (ctx.ownerClass == null || ctx.ownerClass.baseClass == null) {
                    errors.add("[" + ma.line + ":" + ma.column + "] 'super' sin clase base");
                    mv.visitInsn(ACONST_NULL); return;
                }
                Symbol member = ctx.ownerClass.baseClass.lookupInstance(ma.member);
                if (!(member instanceof FunctionSymbol)) {
                    errors.add("[" + ma.line + ":" + ma.column + "] super." + ma.member + " no es método");
                    mv.visitInsn(ACONST_NULL); return;
                }
                FunctionSymbol bfn = (FunctionSymbol) member;
                EmitMethodRef bmi = funcRefs.get(bfn);
                if (bmi == null) { errors.add("super." + ma.member + " sin emit ref"); mv.visitInsn(ACONST_NULL); return; }
                mv.visitVarInsn(ALOAD, 0);
                pushArgs(mv, ce, bfn, ctx);
                mv.visitMethodInsn(INVOKESPECIAL, bmi.ownerClass, bmi.name, bmi.desc, false);
                return;
            }

            // ClassName.metodo(args) — INVOKESTATIC
            if (ma.target instanceof IdentifierExpr) {
                IdentifierExpr tid = (IdentifierExpr) ma.target;
                Symbol idSym = info.exprSymbols.get(tid);
                if (idSym == null) idSym = lookupTypeByName(tid.name);
                if (idSym instanceof ClassSymbol) {
                    ClassSymbol cs = (ClassSymbol) idSym;
                    Symbol member = cs.lookupStatic(ma.member);
                    if (!(member instanceof FunctionSymbol)) {
                        errors.add("[" + ma.line + ":" + ma.column + "] " + cs.name + "." + ma.member + " no es método estático");
                        mv.visitInsn(ACONST_NULL); return;
                    }
                    FunctionSymbol sfn = (FunctionSymbol) member;
                    EmitMethodRef smi = funcRefs.get(sfn);
                    pushArgs(mv, ce, sfn, ctx);
                    mv.visitMethodInsn(INVOKESTATIC, smi.ownerClass, smi.name, smi.desc, false);
                    return;
                }
            }

            // obj.metodo(args) — INVOKEVIRTUAL
            BpType tt = typeOfExpr(ma.target);
            if (tt instanceof ClassType) {
                ClassType ct = (ClassType) tt;
                Symbol member = ct.cls.lookupInstance(ma.member);
                if (!(member instanceof FunctionSymbol)) {
                    errors.add("[" + ma.line + ":" + ma.column + "] " + ct.cls.name + "." + ma.member + " no es método");
                    mv.visitInsn(ACONST_NULL); return;
                }
                FunctionSymbol vfn = (FunctionSymbol) member;
                EmitMethodRef vmi = funcRefs.get(vfn);
                if (vmi == null) { errors.add("método sin emit: " + ma.member); mv.visitInsn(ACONST_NULL); return; }
                emitExpr(mv, ma.target, ctx);
                pushArgs(mv, ce, vfn, ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, vmi.ownerClass, vmi.name, vmi.desc, false);
                return;
            }
            errors.add("[" + ma.line + ":" + ma.column + "] no se puede llamar a '" + ma.member + "' sobre '" + tt.display() + "'");
            mv.visitInsn(ACONST_NULL);
            return;
        }

        errors.add("FASE2: callee no soportado: " + ce.callee.getClass().getSimpleName());
        mv.visitInsn(ACONST_NULL);
    }

    /** Llamadas a la stdlib built-in: input, parseInt, parseFloat, strlen.
     *  Devuelve true si emitió algo (la llamada era built-in). */
    private boolean emitBuiltinCall(MethodVisitor mv, String name, CallExpr ce, EmitContext ctx) {
        switch (name) {
            case "input": {
                // new Scanner(System.in).nextLine()
                mv.visitTypeInsn(NEW, "java/util/Scanner");
                mv.visitInsn(DUP);
                mv.visitFieldInsn(GETSTATIC, "java/lang/System", "in", "Ljava/io/InputStream;");
                mv.visitMethodInsn(INVOKESPECIAL, "java/util/Scanner", "<init>",
                        "(Ljava/io/InputStream;)V", false);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/util/Scanner", "nextLine",
                        "()Ljava/lang/String;", false);
                return true;
            }
            case "parseInt": {
                if (ce.args.isEmpty()) { mv.visitInsn(LCONST_0); return true; }
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Long", "parseLong",
                        "(Ljava/lang/String;)J", false);
                return true;
            }
            case "parseFloat": {
                if (ce.args.isEmpty()) { mv.visitInsn(DCONST_0); return true; }
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Double", "parseDouble",
                        "(Ljava/lang/String;)D", false);
                return true;
            }
            case "strlen": {
                if (ce.args.isEmpty()) { mv.visitInsn(LCONST_0); return true; }
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "length",
                        "()I", false);
                mv.visitInsn(I2L);
                return true;
            }
            // ---- Numéricas integer ----
            case "abs": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "abs", "(J)J", false);
                return true;
            }
            case "min": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "min", "(JJ)J", false);
                return true;
            }
            case "max": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "max", "(JJ)J", false);
                return true;
            }
            // ---- Numéricas float ----
            case "sqrt": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitConvertIfNeeded(mv, typeOfExpr(ce.args.get(0)), PrimitiveType.FLOAT);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "sqrt", "(D)D", false);
                return true;
            }
            case "pow": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitConvertIfNeeded(mv, typeOfExpr(ce.args.get(0)), PrimitiveType.FLOAT);
                emitExpr(mv, ce.args.get(1), ctx);
                emitConvertIfNeeded(mv, typeOfExpr(ce.args.get(1)), PrimitiveType.FLOAT);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "pow", "(DD)D", false);
                return true;
            }
            case "random": {
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "random", "()D", false);
                return true;
            }
            // ---- Conversión float → integer ----
            case "floor": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "floor", "(D)D", false);
                mv.visitInsn(D2L);
                return true;
            }
            case "ceil": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "ceil", "(D)D", false);
                mv.visitInsn(D2L);
                return true;
            }
            case "round": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "round", "(D)J", false);
                return true;
            }
            // ---- Conversión a string ----
            case "intToString": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Long", "toString", "(J)Ljava/lang/String;", false);
                return true;
            }
            case "floatToString": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Double", "toString", "(D)Ljava/lang/String;", false);
                return true;
            }
            case "boolToString": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Boolean", "toString", "(Z)Ljava/lang/String;", false);
                return true;
            }
            // ---- Strings ----
            case "upper": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "toUpperCase", "()Ljava/lang/String;", false);
                return true;
            }
            case "lower": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "toLowerCase", "()Ljava/lang/String;", false);
                return true;
            }
            case "trim": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "trim", "()Ljava/lang/String;", false);
                return true;
            }
            case "substring": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx); mv.visitInsn(L2I);
                emitExpr(mv, ce.args.get(2), ctx); mv.visitInsn(L2I);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "substring",
                        "(II)Ljava/lang/String;", false);
                return true;
            }
            case "indexOf": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "indexOf",
                        "(Ljava/lang/String;)I", false);
                mv.visitInsn(I2L);
                return true;
            }
            case "startsWith": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "startsWith",
                        "(Ljava/lang/String;)Z", false);
                return true;
            }
            case "endsWith": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "endsWith",
                        "(Ljava/lang/String;)Z", false);
                return true;
            }
            case "contains": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "contains",
                        "(Ljava/lang/CharSequence;)Z", false);
                return true;
            }
            case "charAt": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx); mv.visitInsn(L2I);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "charAt", "(I)C", false);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/String", "valueOf",
                        "(C)Ljava/lang/String;", false);
                return true;
            }
            // ---- Strings extra ----
            case "split": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "split",
                        "(Ljava/lang/String;)[Ljava/lang/String;", false);
                return true;
            }
            case "replace": {
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                emitExpr(mv, ce.args.get(2), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "replace",
                        "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;", false);
                return true;
            }
            // ---- Math extra ----
            case "log": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "log", "(D)D", false);
                return true;
            }
            case "log10": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "log10", "(D)D", false);
                return true;
            }
            case "exp": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "exp", "(D)D", false);
                return true;
            }
            case "sin": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "sin", "(D)D", false);
                return true;
            }
            case "cos": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "cos", "(D)D", false);
                return true;
            }
            case "tan": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Math", "tan", "(D)D", false);
                return true;
            }
            case "pi": {
                mv.visitFieldInsn(GETSTATIC, "java/lang/Math", "PI", "D");
                return true;
            }
            case "e": {
                mv.visitFieldInsn(GETSTATIC, "java/lang/Math", "E", "D");
                return true;
            }
            case "randomInt": {
                mv.visitMethodInsn(INVOKESTATIC,
                        "java/util/concurrent/ThreadLocalRandom", "current",
                        "()Ljava/util/concurrent/ThreadLocalRandom;", false);
                emitExpr(mv, ce.args.get(0), ctx);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL,
                        "java/util/concurrent/ThreadLocalRandom", "nextLong", "(JJ)J", false);
                return true;
            }
            // ---- Tiempo ----
            case "now": {
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/System", "currentTimeMillis",
                        "()J", false);
                return true;
            }
            case "sleep": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESTATIC, "java/lang/Thread", "sleep",
                        "(J)V", false);
                return true;
            }
            // ---- I/O archivos ----
            case "readFile": {
                // new String(Files.readAllBytes(Paths.get(path)))
                mv.visitTypeInsn(NEW, "java/lang/String");
                mv.visitInsn(DUP);
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitInsn(ICONST_0);
                mv.visitTypeInsn(ANEWARRAY, "java/lang/String");
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Paths", "get",
                        "(Ljava/lang/String;[Ljava/lang/String;)Ljava/nio/file/Path;", false);
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Files", "readAllBytes",
                        "(Ljava/nio/file/Path;)[B", false);
                mv.visitMethodInsn(INVOKESPECIAL, "java/lang/String", "<init>",
                        "([B)V", false);
                return true;
            }
            case "writeFile": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitInsn(ICONST_0);
                mv.visitTypeInsn(ANEWARRAY, "java/lang/String");
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Paths", "get",
                        "(Ljava/lang/String;[Ljava/lang/String;)Ljava/nio/file/Path;", false);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "getBytes",
                        "()[B", false);
                mv.visitInsn(ICONST_0);
                mv.visitTypeInsn(ANEWARRAY, "java/nio/file/OpenOption");
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Files", "write",
                        "(Ljava/nio/file/Path;[B[Ljava/nio/file/OpenOption;)Ljava/nio/file/Path;", false);
                mv.visitInsn(POP);  // descartar Path devuelto
                return true;
            }
            case "appendFile": {
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitInsn(ICONST_0);
                mv.visitTypeInsn(ANEWARRAY, "java/lang/String");
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Paths", "get",
                        "(Ljava/lang/String;[Ljava/lang/String;)Ljava/nio/file/Path;", false);
                emitExpr(mv, ce.args.get(1), ctx);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/String", "getBytes",
                        "()[B", false);
                // OpenOption[]{ StandardOpenOption.APPEND, StandardOpenOption.CREATE }
                mv.visitInsn(ICONST_2);
                mv.visitTypeInsn(ANEWARRAY, "java/nio/file/OpenOption");
                mv.visitInsn(DUP);
                mv.visitInsn(ICONST_0);
                mv.visitFieldInsn(GETSTATIC, "java/nio/file/StandardOpenOption", "APPEND",
                        "Ljava/nio/file/StandardOpenOption;");
                mv.visitInsn(AASTORE);
                mv.visitInsn(DUP);
                mv.visitInsn(ICONST_1);
                mv.visitFieldInsn(GETSTATIC, "java/nio/file/StandardOpenOption", "CREATE",
                        "Ljava/nio/file/StandardOpenOption;");
                mv.visitInsn(AASTORE);
                mv.visitMethodInsn(INVOKESTATIC, "java/nio/file/Files", "write",
                        "(Ljava/nio/file/Path;[B[Ljava/nio/file/OpenOption;)Ljava/nio/file/Path;", false);
                mv.visitInsn(POP);
                return true;
            }
            case "fileExists": {
                mv.visitTypeInsn(NEW, "java/io/File");
                mv.visitInsn(DUP);
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESPECIAL, "java/io/File", "<init>",
                        "(Ljava/lang/String;)V", false);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/File", "exists", "()Z", false);
                return true;
            }
            case "listDir": {
                mv.visitTypeInsn(NEW, "java/io/File");
                mv.visitInsn(DUP);
                emitExpr(mv, ce.args.get(0), ctx);
                mv.visitMethodInsn(INVOKESPECIAL, "java/io/File", "<init>",
                        "(Ljava/lang/String;)V", false);
                mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/File", "list",
                        "()[Ljava/lang/String;", false);
                return true;
            }
        }
        return false;
    }

    private void pushArgs(MethodVisitor mv, CallExpr ce, FunctionSymbol fn, EmitContext ctx) {
        for (int i = 0; i < ce.args.size(); i++) {
            emitExpr(mv, ce.args.get(i), ctx);
            if (i < fn.params.size())
                emitConvertIfNeeded(mv, typeOfExpr(ce.args.get(i)), fn.params.get(i).type);
        }
    }

    private void emitNew(MethodVisitor mv, ClassSymbol cls, CallExpr ce, EmitContext ctx) {
        FunctionSymbol ctor = cls.findConstructor();
        String jvmName = jvmNameOfClass(cls);
        mv.visitTypeInsn(NEW, jvmName);
        mv.visitInsn(DUP);
        if (ctor == null) {
            mv.visitMethodInsn(INVOKESPECIAL, jvmName, "<init>", "()V", false);
            return;
        }
        EmitMethodRef mi = funcRefs.get(ctor);
        String desc;
        if (mi != null) {
            desc = mi.desc;
        } else {
            // Constructor built-in (ej. RuntimeError) — construimos el desc
            // a partir de los parámetros del FunctionSymbol lógico.
            StringBuilder sb = new StringBuilder("(");
            for (ParamSymbol p : ctor.params) sb.append(desc(p.type));
            sb.append(")V");
            desc = sb.toString();
        }
        pushArgs(mv, ce, ctor, ctx);
        mv.visitMethodInsn(INVOKESPECIAL, jvmName, "<init>", desc, false);
    }

    private void emitSuperCallExpr(MethodVisitor mv, SuperCallExpr sc, EmitContext ctx) {
        if (ctx.ownerClass == null || ctx.ownerClass.baseClass == null) {
            errors.add("[" + sc.line + ":" + sc.column + "] super() sin clase base");
            return;
        }
        ClassSymbol base = ctx.ownerClass.baseClass;
        FunctionSymbol baseCtor = base.findConstructor();
        mv.visitVarInsn(ALOAD, 0);
        if (baseCtor == null) {
            mv.visitMethodInsn(INVOKESPECIAL, base.name, "<init>", "()V", false);
            return;
        }
        EmitMethodRef mi = funcRefs.get(baseCtor);
        // Push args
        for (int i = 0; i < sc.args.size(); i++) {
            emitExpr(mv, sc.args.get(i), ctx);
            if (i < baseCtor.params.size())
                emitConvertIfNeeded(mv, typeOfExpr(sc.args.get(i)), baseCtor.params.get(i).type);
        }
        mv.visitMethodInsn(INVOKESPECIAL, mi.ownerClass, "<init>", mi.desc, false);
    }

    // ============================================================
    // Resolución de identificadores (clasifica el "kind")
    // ============================================================
    private enum FieldKind { LOCAL, PARAM, INSTANCE_FIELD_OF_THIS, STATIC_FIELD, CONSTANT, TYPE, NONE }

    private FieldKind identKind(IdentifierExpr id, EmitContext ctx) {
        Symbol sym = resolveIdent(id, ctx);
        if (sym instanceof VarSymbol) {
            VarSymbol vs = (VarSymbol) sym;
            if (ctx.localSlots.containsKey(vs)) return FieldKind.LOCAL;
            EmitFieldRef fi = varFields.get(vs);
            if (fi == null) return FieldKind.NONE;
            return fi.isStatic ? FieldKind.STATIC_FIELD : FieldKind.INSTANCE_FIELD_OF_THIS;
        }
        if (sym instanceof ParamSymbol) return FieldKind.PARAM;
        if (sym instanceof ConstSymbol) return FieldKind.CONSTANT;
        if (sym instanceof ClassSymbol || sym instanceof EnumSymbol) return FieldKind.TYPE;
        return FieldKind.NONE;
    }

    private Symbol resolveIdent(IdentifierExpr id, EmitContext ctx) {
        // 1) Locales registradas por nombre (for-iterator etc.)
        Symbol s = ctx.localNames.get(id.name);
        if (s != null) return s;
        // 2) Parámetros por nombre
        for (Map.Entry<ParamSymbol, Integer> e : ctx.paramSlots.entrySet())
            if (e.getKey().name.equals(id.name)) return e.getKey();
        // 3) Locales por nombre
        for (Map.Entry<VarSymbol, Integer> e : ctx.localSlots.entrySet())
            if (e.getKey().name.equals(id.name)) return e.getKey();
        // 4) Campos / propiedades de instancia de la clase actual (vía 'this').
        //    Dentro de un getter/setter, fs es null pero seguimos siendo "instancia".
        boolean inInstanceCtx = (ctx.ownerClass != null)
                && ((ctx.fs != null && !ctx.fs.isStatic)
                    || (ctx.currentProperty != null && !ctx.currentProperty.isStatic));
        if (inInstanceCtx) {
            Symbol m = ctx.ownerClass.lookupInstance(id.name);
            if (m instanceof VarSymbol || m instanceof PropertySymbol) return m;
        }
        // 5) Campos del módulo y consts
        for (Map.Entry<VarSymbol, EmitFieldRef> e : varFields.entrySet())
            if (e.getKey().name.equals(id.name) && moduleClass.jvmName.equals(e.getValue().ownerClass))
                return e.getKey();
        for (Map.Entry<ConstSymbol, EmitFieldRef> e : constFields.entrySet())
            if (e.getKey().name.equals(id.name) && moduleClass.jvmName.equals(e.getValue().ownerClass))
                return e.getKey();
        // 6) Tipos (clases) por nombre
        Symbol t = lookupTypeByName(id.name);
        if (t != null) return t;
        // 7) Pista del analizador
        return info.exprSymbols.get(id);
    }

    private Symbol lookupTypeByName(String name) {
        if (info.module != null) {
            Symbol s = info.module.members.tryLookup(name);
            if (s instanceof ClassSymbol || s instanceof EnumSymbol) return s;
        }
        return null;
    }

    private FunctionSymbol findFunctionByName(String name) {
        for (Map.Entry<FunctionSymbol, EmitMethodRef> e : funcRefs.entrySet())
            if (e.getKey().name.equals(name)) {
                FunctionSymbol fn = e.getKey();
                // Preferir función a nivel módulo o método de instancia
                // de la clase actual, según contexto. Esta heurística
                // funciona para casos típicos.
                return fn;
            }
        return null;
    }

    private boolean isStaticAccess(MemberAccessExpr ma) {
        if (ma.target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ma.target;
            Symbol s = info.exprSymbols.get(id);
            if (s == null) s = lookupTypeByName(id.name);
            return (s instanceof ClassSymbol) || (s instanceof EnumSymbol);
        }
        return false;
    }

    private Symbol resolveMember(MemberAccessExpr ma) {
        if (ma.target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ma.target;
            Symbol s = info.exprSymbols.get(id);
            if (s == null) s = lookupTypeByName(id.name);
            if (s instanceof ClassSymbol)
                return ((ClassSymbol) s).lookupStatic(ma.member);
        }
        BpType t = typeOfExpr(ma.target);
        if (t instanceof ClassType)
            return ((ClassType) t).cls.lookupInstance(ma.member);
        return null;
    }

    private EmitFieldRef fieldRefOf(Symbol s) {
        if (s instanceof VarSymbol)   return varFields.get(s);
        if (s instanceof ConstSymbol) return constFields.get(s);
        return null;
    }

    // ============================================================
    // Tipos / descriptores
    // ============================================================
    private static String desc(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: return "J";
                case FLOAT:   return "D";
                case STRING:  return "Ljava/lang/String;";
                case BOOLEAN: return "Z";
            }
        }
        if (t instanceof VoidType) return "V";
        if (t instanceof NullType) return "Ljava/lang/Object;";
        if (t instanceof ClassType) return "L" + jvmNameOfClass(((ClassType) t).cls) + ";";
        if (t instanceof EnumType)  return "J";   // enum BASICPLUS = long
        if (t instanceof ArrayType) return "[" + desc(((ArrayType) t).element);
        return "Ljava/lang/Object;";
    }

    /** Mapea built-ins a sus nombres JVM reales. */
    private static String jvmNameOfClass(ClassSymbol cls) {
        if (cls == null) return "java/lang/Object";
        if ("RuntimeError".equals(cls.name) && SemanticAnalyzer.isBuiltin(cls))
            return "java/lang/RuntimeException";
        return cls.name;
    }

    private static String methodDescriptor(FunctionSymbol fs) {
        StringBuilder sb = new StringBuilder("(");
        for (ParamSymbol p : fs.params) sb.append(desc(p.type));
        sb.append(")");
        sb.append(desc(fs.returnType == null ? VoidType.INSTANCE : fs.returnType));
        return sb.toString();
    }

    private static String constructorDescriptor(FunctionSymbol fs) {
        StringBuilder sb = new StringBuilder("(");
        for (ParamSymbol p : fs.params) sb.append(desc(p.type));
        sb.append(")V");
        return sb.toString();
    }

    private static int slotsOf(BpType t) {
        if (t instanceof PrimitiveType) {
            PrimitiveType.Kind k = ((PrimitiveType) t).tag;
            if (k == PrimitiveType.Kind.INTEGER || k == PrimitiveType.Kind.FLOAT) return 2;
        }
        if (t instanceof EnumType) return 2;   // enum BASICPLUS = long
        return 1;
    }

    private static int returnOpcode(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: return LRETURN;
                case FLOAT:   return DRETURN;
                case BOOLEAN: return IRETURN;
                case STRING:  return ARETURN;
            }
        }
        if (t instanceof EnumType) return LRETURN;     // enum = long
        if (t instanceof VoidType) return RETURN;
        return ARETURN;
    }

    private static void loadLocal(MethodVisitor mv, BpType t, int slot) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: mv.visitVarInsn(LLOAD, slot); return;
                case FLOAT:   mv.visitVarInsn(DLOAD, slot); return;
                case BOOLEAN: mv.visitVarInsn(ILOAD, slot); return;
                case STRING:  mv.visitVarInsn(ALOAD, slot); return;
            }
        }
        if (t instanceof EnumType) { mv.visitVarInsn(LLOAD, slot); return; }
        mv.visitVarInsn(ALOAD, slot);
    }

    private static void storeLocal(MethodVisitor mv, BpType t, int slot) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: mv.visitVarInsn(LSTORE, slot); return;
                case FLOAT:   mv.visitVarInsn(DSTORE, slot); return;
                case BOOLEAN: mv.visitVarInsn(ISTORE, slot); return;
                case STRING:  mv.visitVarInsn(ASTORE, slot); return;
            }
        }
        if (t instanceof EnumType) { mv.visitVarInsn(LSTORE, slot); return; }
        mv.visitVarInsn(ASTORE, slot);
    }

    private static void loadLong(MethodVisitor mv, long v) {
        if (v == 0L) mv.visitInsn(LCONST_0);
        else if (v == 1L) mv.visitInsn(LCONST_1);
        else mv.visitLdcInsn(v);
    }

    private static void loadDouble(MethodVisitor mv, double v) {
        if (v == 0.0) mv.visitInsn(DCONST_0);
        else if (v == 1.0) mv.visitInsn(DCONST_1);
        else mv.visitLdcInsn(v);
    }

    private BpType typeOfExpr(IExpr e) {
        BpType t = info.exprTypes.get(e);
        if (t != null) return t;
        if (e instanceof IntLitExpr)    return PrimitiveType.INTEGER;
        if (e instanceof FloatLitExpr)  return PrimitiveType.FLOAT;
        if (e instanceof StringLitExpr) return PrimitiveType.STRING;
        if (e instanceof BoolLitExpr)   return PrimitiveType.BOOLEAN;
        if (e instanceof NullLitExpr)   return NullType.INSTANCE;
        return ErrorType.INSTANCE;
    }

    private static boolean isString(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
    }

    private static BpType numericPromotion(BpType a, BpType b) {
        boolean af = a instanceof PrimitiveType && ((PrimitiveType) a).tag == PrimitiveType.Kind.FLOAT;
        boolean bf = b instanceof PrimitiveType && ((PrimitiveType) b).tag == PrimitiveType.Kind.FLOAT;
        if (af || bf) return PrimitiveType.FLOAT;
        return PrimitiveType.INTEGER;
    }

    private void emitConvertIfNeeded(MethodVisitor mv, BpType src, BpType dst) {
        if (src == null || dst == null) return;
        if (src.sameAs(dst)) return;
        if (dst instanceof PrimitiveType && src instanceof PrimitiveType) {
            PrimitiveType.Kind d = ((PrimitiveType) dst).tag;
            PrimitiveType.Kind s = ((PrimitiveType) src).tag;
            if (d == PrimitiveType.Kind.FLOAT && s == PrimitiveType.Kind.INTEGER)
                mv.visitInsn(L2D);
            else if (d == PrimitiveType.Kind.INTEGER && s == PrimitiveType.Kind.FLOAT)
                mv.visitInsn(D2L);
        }
    }

    // ============================================================
    // EmitContext y LoopFrame
    // ============================================================
    private static final class LoopFrame {
        final Label continueTarget;
        final Label breakTarget;
        LoopFrame(Label c, Label b) { this.continueTarget = c; this.breakTarget = b; }
    }

    private static final class EmitContext {
        final MethodVisitor method;
        final ClassSymbol ownerClass;          // null si es función a nivel módulo
        final FunctionSymbol fs;               // null para cctor / getter / setter
        final boolean isCctor;
        PropertySymbol currentProperty;        // !=null si estamos en getter/setter
        final Map<VarSymbol,   Integer> localSlots = new HashMap<>();
        final Map<ParamSymbol, Integer> paramSlots = new HashMap<>();
        final Map<String, Symbol>       localNames = new HashMap<>();
        final Deque<LoopFrame>          loopStack  = new java.util.ArrayDeque<>();
        // Finallys pendientes (innermost first via push). Cuando emitReturn
        // dispara, ejecuta cada uno antes de RETURN para no saltarlos.
        final Deque<List<IStmt>>        finallyStack = new java.util.ArrayDeque<>();
        int nextSlot = 0;

        EmitContext(MethodVisitor mv, ClassSymbol ownerClass, FunctionSymbol fs, boolean isCctor) {
            this.method = mv; this.ownerClass = ownerClass; this.fs = fs; this.isCctor = isCctor;
        }

        int allocSlot(BpType t) {
            int s = nextSlot;
            nextSlot += slotsOf(t);
            return s;
        }
    }
}
