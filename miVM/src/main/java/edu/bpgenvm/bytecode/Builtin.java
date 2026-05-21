/*
 * Catálogo de funciones built-in invocables por bytecode con CALL_BUILTIN id.
 * El ID es el `ordinal()` del enum, así que el ORDEN ES ESTABLE: añadir nuevos
 * built-ins al final.
 */
package edu.bpgenvm.bytecode;

import java.util.HashMap;
import java.util.Map;

public enum Builtin {
    // Strings utilitarios
    STRLEN("strlen"),
    PARSE_INT("parseInt"),
    PARSE_FLOAT("parseFloat"),
    INT_TO_STRING("intToString"),
    FLOAT_TO_STRING("floatToString"),
    BOOL_TO_STRING("boolToString"),
    UPPER("upper"),
    LOWER("lower"),
    TRIM("trim"),
    SUBSTRING("substring"),
    INDEX_OF("indexOf"),
    STARTS_WITH("startsWith"),
    ENDS_WITH("endsWith"),
    CONTAINS("contains"),
    CHAR_AT("charAt"),
    REPLACE("replace"),

    // Numéricas enteras
    ABS("abs"),
    MIN("min"),
    MAX("max"),

    // Numéricas float
    SQRT("sqrt"),
    POW("pow"),
    LOG("log"),
    LOG10("log10"),
    EXP("exp"),
    SIN("sin"),
    COS("cos"),
    TAN("tan"),
    RANDOM("random"),
    RANDOM_INT("randomInt"),

    // Constantes
    PI("pi"),
    E("e"),

    // Conversión float→int
    FLOOR("floor"),
    CEIL("ceil"),
    ROUND("round"),

    // Tiempo
    NOW("now"),
    SLEEP("sleep"),

    // Strings que devuelven arrays
    SPLIT("split"),

    // I/O
    INPUT("input"),
    READ_FILE("readFile"),
    WRITE_FILE("writeFile"),
    APPEND_FILE("appendFile"),
    FILE_EXISTS("fileExists"),
    LIST_DIR("listDir"),

    // Debug
    GC("gc"),

    // ---- Soporte para clases stdlib (List, StringBuilder). Los nombres
    //      empiezan por __ para señalar uso interno (aunque cualquier código
    //      BP puede llamarlos también). ----
    NEW_REF_ARRAY("__newRefArray"),      // (cap)        → array TYPE_ARRAY_REF (slots a 0)
    GROW_REF_ARRAY("__growRefArray"),    // (old, newCap)→ array TYPE_ARRAY_REF copiado
    GROW_INT_ARRAY("__growIntArray"),    // (old, newCap)→ array TYPE_ARRAY_I32 copiado
    CHARS_TO_STRING("__charsToString"),  // (chars, len) → string heap-allocado
    CHAR_CODE_AT("charCodeAt"),          // (s, i)       → integer (code point del char i)

    // ---- Threading ----
    THREAD_START("__threadStart"),       // (threadRef)  → void; spawnea + arranca run() del thread
    THREAD_JOIN("__threadJoin"),         // (threadRef)  → void; bloquea hasta que termine
    YIELD("yield"),                      // ()           → void; cede CPU al scheduler

    // ---- Sync ----
    MUTEX_CREATE("__mutexCreate"),       // ()           → integer (id de mutex en VM)
    MUTEX_LOCK("__mutexLock"),           // (mutexRef)   → void; bloquea si está tomado
    MUTEX_UNLOCK("__mutexUnlock"),       // (mutexRef)   → void; libera; despierta primer waiter

    // ---- Arrays ----
    MOVE("move"),                        // (src,dst,srcStart,dstStart,count) → void
                                         //   copia `count` elementos. Soporta overlapping
                                         //   en el mismo array. RuntimeError si:
                                         //   - src/dst null o no son arrays;
                                         //   - tipos de array distintos (i8/i16/i32/ref);
                                         //   - count negativo o rango fuera de los arrays.

    // ---- Math (intrínsecos accesibles vía `import Math`) ----
    SIGN_I("__sign_i"),                  // (i: int)   → int   (-1/0/1)
    SIGN_F("__sign_f"),                  // (f: float) → int   (-1/0/1)
    ASIN("__asin"),                      // (f: float) → float
    ACOS("__acos"),                      // (f: float) → float
    ATAN("__atan"),                      // (f: float) → float
    ATAN2("__atan2"),                    // (y: float, x: float) → float
    FACTORIAL_I("__factorial_i"),        // (n: int)   → int    (n>=0 y resultado i32)
    GAMMA_F("__gamma_f"),                // (x: float) → float  (Lanczos)

    // ---- IO (intrínsecos accesibles vía `import IO`) ----
    PATH_JOIN("__pathJoin"),             // (a, b)        → string
    PATH_PARENT("__pathParent"),         // (p: string)   → string (puede ser "")
    PATH_BASENAME("__pathBasename"),     // (p: string)   → string
    PATH_EXTENSION("__pathExtension"),   // (p: string)   → string (sin punto, "" si no hay)
    PATH_ABSOLUTE("__pathAbsolute"),     // (p: string)   → string
    MKDIR("__mkdir"),                    // (p: string)   → void  (crea dirs intermedios)
    RMDIR("__rmdir"),                    // (p: string)   → void  (sólo si vacío)
    REMOVE_FILE("__removeFile"),         // (p: string)   → void
    RENAME("__rename"),                  // (from, to)    → void
    COPY_FILE("__copyFile"),             // (from, to)    → void
    FILE_SIZE("__fileSize"),             // (p: string)   → int   (bytes)
    IS_DIRECTORY("__isDirectory"),       // (p: string)   → boolean
    LAST_MODIFIED("__lastModified"),     // (p: string)   → int   (epoch ms truncado i32)

    // ---- N20 — UI via IDE conectado ----
    PROMPT("__prompt");                  // (spec: string) → string
                                         //   Envía formRequest al IDE, bloquea el
                                         //   thread BP hasta respuesta. Devuelve
                                         //   el JSON con los valores. Si no hay
                                         //   IDE, RuntimeError BP atrapable.

    public final String bpName;
    public final int id;

    Builtin(String bpName) { this.bpName = bpName; this.id = ordinal(); }

    private static final Map<String, Builtin> BY_NAME = new HashMap<>();
    static { for (Builtin b : values()) BY_NAME.put(b.bpName, b); }

    public static Builtin byName(String name) { return BY_NAME.get(name); }
    public static Builtin byId(int id) { return values()[id]; }
}
