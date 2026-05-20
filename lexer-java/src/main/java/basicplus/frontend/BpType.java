// ============================================================
// BpType.java
// Sistema de tipos resueltos (semánticos) de BASICPLUS.
//
// 'BpType' es la representación tras el análisis semántico —
// distinta de 'Ast.TypeRef' que es la sintaxis cruda escrita
// por el usuario.
//
// Subtipos como nested static classes para mantener todo en un
// archivo (estilo Ast.java).
// ============================================================
package basicplus.frontend;

public abstract class BpType {

    public abstract String display();

    /** true si this y other denotan exactamente el mismo tipo. */
    public abstract boolean sameAs(BpType other);

    /**
     * true si una expresión de tipo 'source' puede asignarse a
     * una variable de tipo 'this' (o pasarse como argumento).
     * Cubre la jerarquía de clases (subclase asignable a base) y
     * la nullabilidad (null se acepta en referencias, no en escalares).
     */
    public abstract boolean isAssignableFrom(BpType source);

    /** integer/float/string/boolean/enum. */
    public boolean isScalar()    { return false; }
    /** clases y arrays (admiten null). */
    public boolean isReference() { return false; }
    /** integer y float. */
    public boolean isNumeric()   { return false; }
    /** enum. */
    public boolean isEnum()      { return false; }

    @Override public String toString() { return display(); }

    // ============================================================
    // Tipos primitivos
    // ============================================================
    public static final class PrimitiveType extends BpType {

        public enum Kind { INTEGER, FLOAT, STRING, BOOLEAN }

        public final Kind tag;
        private PrimitiveType(Kind k) { this.tag = k; }

        public static final PrimitiveType INTEGER = new PrimitiveType(Kind.INTEGER);
        public static final PrimitiveType FLOAT   = new PrimitiveType(Kind.FLOAT);
        public static final PrimitiveType STRING  = new PrimitiveType(Kind.STRING);
        public static final PrimitiveType BOOLEAN = new PrimitiveType(Kind.BOOLEAN);

        @Override public String  display()   { return tag.name().toLowerCase(); }
        @Override public boolean isScalar()  { return true; }
        @Override public boolean isNumeric() { return tag == Kind.INTEGER || tag == Kind.FLOAT; }

        @Override
        public boolean sameAs(BpType other) {
            return other instanceof PrimitiveType && ((PrimitiveType) other).tag == tag;
        }

        @Override
        public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            if (sameAs(source)) return true;
            // Promoción integer -> float
            if (tag == Kind.FLOAT && source instanceof PrimitiveType
                    && ((PrimitiveType) source).tag == Kind.INTEGER)
                return true;
            // L1: any → primitivo. A nivel bytecode los slots son
            // idénticos (4 bytes), así que la asignación es
            // mecánicamente correcta. Sin esto, List/SyncList con
            // primitivos obliga a envolverlos en clases wrapper. El
            // SemanticAnalyzer emitirá un warning en cada sitio para
            // que el usuario sea consciente de que el typecheck es
            // estructural, no de runtime — si el `any` en realidad
            // contenía un ref, este re-asignado a primitive va a leer
            // bytes basura.
            if (source instanceof AnyType) return true;
            return false;
        }
    }

    // ============================================================
    // Clases (referencias)
    // ============================================================
    public static final class ClassType extends BpType {
        public final Symbol.ClassSymbol cls;
        public ClassType(Symbol.ClassSymbol cls) { this.cls = cls; }

        @Override public String  display()     { return cls.name; }
        @Override public boolean isReference() { return true; }

        @Override
        public boolean sameAs(BpType other) {
            return other instanceof ClassType && ((ClassType) other).cls == cls;
        }

        @Override
        public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            if (source instanceof NullType)  return true;
            if (source instanceof AnyType)   return true;   // escape hatch para stdlib
            if (!(source instanceof ClassType)) return false;
            // Subclase asignable a base
            Symbol.ClassSymbol cur = ((ClassType) source).cls;
            while (cur != null) {
                if (cur == cls) return true;
                cur = cur.baseClass;
            }
            return false;
        }
    }

    // ============================================================
    // Enums
    // ============================================================
    public static final class EnumType extends BpType {
        public final Symbol.EnumSymbol en;
        public EnumType(Symbol.EnumSymbol en) { this.en = en; }

        @Override public String  display()  { return en.name; }
        @Override public boolean isEnum()   { return true; }
        @Override public boolean isScalar() { return true; }

        @Override
        public boolean sameAs(BpType other) {
            return other instanceof EnumType && ((EnumType) other).en == en;
        }

        @Override
        public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            return sameAs(source);
        }
    }

    // ============================================================
    // Arrays
    // ============================================================
    public static final class ArrayType extends BpType {
        public final BpType element;
        public ArrayType(BpType element) { this.element = element; }

        @Override public String  display()     { return element.display() + "[]"; }
        @Override public boolean isReference() { return true; }

        @Override
        public boolean sameAs(BpType other) {
            return other instanceof ArrayType && ((ArrayType) other).element.sameAs(element);
        }

        @Override
        public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            if (source instanceof NullType)  return true;
            if (source instanceof ArrayType) return element.sameAs(((ArrayType) source).element);
            return false;
        }
    }

    // ============================================================
    // Tipo del literal null
    // ============================================================
    public static final class NullType extends BpType {
        public static final NullType INSTANCE = new NullType();
        private NullType() { }

        @Override public String  display() { return "null"; }
        @Override public boolean sameAs(BpType other) { return other instanceof NullType; }
        @Override public boolean isAssignableFrom(BpType source) {
            return source instanceof NullType || source instanceof ErrorType;
        }
    }

    // ============================================================
    // Tipo void (return sin valor)
    // ============================================================
    public static final class VoidType extends BpType {
        public static final VoidType INSTANCE = new VoidType();
        private VoidType() { }

        @Override public String  display() { return "void"; }
        @Override public boolean sameAs(BpType other) { return other instanceof VoidType; }
        @Override public boolean isAssignableFrom(BpType source) {
            return source instanceof VoidType || source instanceof ErrorType;
        }
    }

    // ============================================================
    // Tipo de error (recuperación)
    // ============================================================
    public static final class ErrorType extends BpType {
        public static final ErrorType INSTANCE = new ErrorType();
        private ErrorType() { }

        @Override public String  display() { return "<error>"; }
        @Override public boolean sameAs(BpType other) { return true; }
        @Override public boolean isAssignableFrom(BpType source) { return true; }
    }

    // ============================================================
    // Tipo "any" — interno: NO se puede escribir en BP source. Lo usan las
    // signatures de stdlib sintetizadas (List.add(item), List.get(): any,
    // etc.) para evitar fricción de tipos: cualquier valor entra y cualquier
    // tipo target acepta un valor de tipo any. Es la mínima escapatoria al
    // type checker mientras no haya genéricos.
    // ============================================================
    public static final class AnyType extends BpType {
        public static final AnyType INSTANCE = new AnyType();
        private AnyType() { }

        @Override public String  display()     { return "any"; }
        @Override public boolean isReference() { return true; }
        @Override public boolean sameAs(BpType other) { return other instanceof AnyType; }
        @Override public boolean isAssignableFrom(BpType source) { return true; }
    }

    // ============================================================
    // Tipo "clase no resuelta" — placeholder usado por el lector de
    // .bpi cuando aparece una referencia a otra clase del mismo módulo
    // dentro de la firma de un método o de una property. En ese momento
    // ModuleInterface no tiene acceso a ClassSymbol, así que guarda el
    // nombre y deja que el consumidor (Main.java loader) reemplace por
    // un ClassType real una vez creados todos los stubs de clase.
    // Se trata como ref-type para que typecheck básico funcione.
    // ============================================================
    public static final class UnresolvedClassRef extends BpType {
        public final String name;
        public UnresolvedClassRef(String name) { this.name = name; }

        @Override public String  display()     { return name; }
        @Override public boolean isReference() { return true; }
        @Override public boolean sameAs(BpType other) {
            return other instanceof UnresolvedClassRef
                    && ((UnresolvedClassRef) other).name.equals(name);
        }
        @Override public boolean isAssignableFrom(BpType source) {
            // Lo más laxo posible — esto sólo debería existir momentáneamente
            // entre lectura del .bpi y resolución. Si llega al typecheck del
            // usuario es que algo no se resolvió y dejarlo "asignable a todo"
            // pasa el chequeo en vez de bloquear con un error confuso.
            return true;
        }
    }
}
