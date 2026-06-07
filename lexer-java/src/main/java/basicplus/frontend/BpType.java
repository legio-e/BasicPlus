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

        /**
         * L10 — los tipos enteros estrechos (INT8/UINT8/INT16/UINT16) son
         * "ciudadanos de segunda" del sistema: existen para layouts de
         * memoria compactos (arrays, globals) y para señalar API contracts,
         * pero la VM SÓLO opera con i32 en la pila. Por tanto:
         *   - Cualquier LOAD de un valor estrecho lo promociona a INTEGER.
         *   - Una asignación INTEGER → estrecho REQUIERE cast explícito
         *     `byte(x)` / `int8(x)` / `word(x)` / `int16(x)` / `short(x)`
         *     porque puede perder información.
         *   - `short` es alias gramatical de `int16` (mismo Kind).
         */
        public enum Kind {
            INTEGER, FLOAT, STRING, BOOLEAN,
            INT8, UINT8, INT16, UINT16,
            LONG,   // H1.2 (V2): entero de 64 bits (i64)
            DOUBLE  // H1.3 (V2): coma flotante de 64 bits (f64)
        }

        public final Kind tag;
        private PrimitiveType(Kind k) { this.tag = k; }

        public static final PrimitiveType INTEGER = new PrimitiveType(Kind.INTEGER);
        public static final PrimitiveType FLOAT   = new PrimitiveType(Kind.FLOAT);
        public static final PrimitiveType STRING  = new PrimitiveType(Kind.STRING);
        public static final PrimitiveType BOOLEAN = new PrimitiveType(Kind.BOOLEAN);
        public static final PrimitiveType INT8    = new PrimitiveType(Kind.INT8);
        public static final PrimitiveType UINT8   = new PrimitiveType(Kind.UINT8);   // byte
        public static final PrimitiveType INT16   = new PrimitiveType(Kind.INT16);   // short
        public static final PrimitiveType UINT16  = new PrimitiveType(Kind.UINT16);  // word
        public static final PrimitiveType LONG    = new PrimitiveType(Kind.LONG);    // i64 (H1.2)
        public static final PrimitiveType DOUBLE  = new PrimitiveType(Kind.DOUBLE);  // f64 (H1.3)

        @Override public String display() {
            switch (tag) {
                case UINT8:  return "byte";
                case UINT16: return "word";
                // int8/int16 se muestran tal cual (la versión "short" se mapea a int16 en
                // el parser para colapsar el alias; el usuario que escribió "short" verá
                // "int16" en mensajes de error, asumido como aceptable).
                default:     return tag.name().toLowerCase();
            }
        }
        @Override public boolean isScalar()  { return true; }
        @Override public boolean isNumeric() {
            return tag == Kind.INTEGER || tag == Kind.FLOAT
                || tag == Kind.LONG || tag == Kind.DOUBLE
                || isNarrowInteger();
        }

        /** True para INT8/UINT8/INT16/UINT16. Su semántica es: load auto
         *  a INTEGER; store desde INTEGER requiere cast explícito. */
        public boolean isNarrowInteger() {
            return tag == Kind.INT8 || tag == Kind.UINT8
                || tag == Kind.INT16 || tag == Kind.UINT16;
        }

        /** True para los tipos del "grupo integer-like" — i32 + estrechos. */
        public boolean isIntegerLike() {
            return tag == Kind.INTEGER || isNarrowInteger();
        }

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
            // L10 — Estrechos → INTEGER: load promociona automáticamente.
            //   var n: integer := someByte    ✓
            //   var n: integer := someInt16   ✓
            if (tag == Kind.INTEGER && source instanceof PrimitiveType
                    && ((PrimitiveType) source).isNarrowInteger())
                return true;
            // H1.2 — int/narrow → long (widening, sin pérdida).
            if (tag == Kind.LONG && source instanceof PrimitiveType
                    && ((PrimitiveType) source).isIntegerLike())
                return true;
            // H1.3 — cualquier numérico más estrecho → double (widening).
            if (tag == Kind.DOUBLE && source instanceof PrimitiveType
                    && ((PrimitiveType) source).isNumeric())
                return true;
            // H1.3 — int/narrow/long → float (widening). long→float puede perder
            // precisión pero no rango (como en Java). double→float NO (narrowing).
            if (tag == Kind.FLOAT && source instanceof PrimitiveType
                    && (((PrimitiveType) source).isIntegerLike()
                        || ((PrimitiveType) source).tag == Kind.LONG))
                return true;
            // L10 — INTEGER → estrecho: SÓLO si la fuente es a su vez
            // del mismo tipo (ya cubierto por sameAs). Cualquier otra
            // asignación necesita cast explícito y NO pasa por aquí —
            // la verifica el SemanticAnalyzer con literal-range-check.
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

        /** Rango [min,max] inclusive para tipos narrow. Indefinido para INTEGER/FLOAT. */
        public long rangeMin() {
            switch (tag) {
                case INT8:   return -128L;
                case UINT8:  return 0L;
                case INT16:  return -32768L;
                case UINT16: return 0L;
                default:     return Long.MIN_VALUE;
            }
        }
        public long rangeMax() {
            switch (tag) {
                case INT8:   return 127L;
                case UINT8:  return 255L;
                case INT16:  return 32767L;
                case UINT16: return 65535L;
                default:     return Long.MAX_VALUE;
            }
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
            if (other instanceof ClassType) return ((ClassType) other).cls == cls;
            // GAP-2 — cross-module: el otro lado puede ser una referencia de clase
            // sin resolver (leída del .bpi por nombre). Igual si el nombre coincide.
            if (other instanceof UnresolvedClassRef)
                return ((UnresolvedClassRef) other).name.equals(cls.name);
            return false;
        }

        @Override
        public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            if (source instanceof NullType)  return true;
            if (source instanceof AnyType)   return true;   // escape hatch para stdlib
            // GAP-2 — cross-module: un valor cuyo tipo es "una clase de otro
            // módulo" llega como UnresolvedClassRef (referencia por nombre del
            // .bpi, sin símbolo local). Asignable si el nombre coincide con el
            // tipo local. Cubre built-in (List/Map/SyncList/…) y clases de
            // usuario retornadas/pasadas cross-module.
            if (source instanceof UnresolvedClassRef)
                return ((UnresolvedClassRef) source).name.equals(cls.name);
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
    // Tuplas — SOLO como tipo de retorno de función (retorno múltiple).
    // No son valores de primera clase: el SemanticAnalyzer prohíbe
    // almacenarlas/pasarlas; solo se destructuran en el acto con
    // `{ a, b } := f()`. Internamente = objeto sintético con N campos.
    // ============================================================
    public static final class TupleType extends BpType {
        public final java.util.List<BpType> elements;
        public TupleType(java.util.List<BpType> elements) { this.elements = elements; }

        @Override public String display() {
            StringBuilder sb = new StringBuilder("(");
            for (int i = 0; i < elements.size(); i++) {
                if (i > 0) sb.append(", ");
                sb.append(elements.get(i).display());
            }
            return sb.append(")").toString();
        }
        @Override public boolean isReference() { return true; }  // ref a objeto sintético
        @Override public boolean sameAs(BpType other) {
            if (!(other instanceof TupleType)) return false;
            TupleType t = (TupleType) other;
            if (t.elements.size() != elements.size()) return false;
            for (int i = 0; i < elements.size(); i++)
                if (!elements.get(i).sameAs(t.elements.get(i))) return false;
            return true;
        }
        @Override public boolean isAssignableFrom(BpType source) {
            if (source instanceof ErrorType) return true;
            if (source instanceof AnyType) return true;   // H8.2: leer una tupla de una colección (any); estructural, sin check runtime (cf. L1)
            return sameAs(source);
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
