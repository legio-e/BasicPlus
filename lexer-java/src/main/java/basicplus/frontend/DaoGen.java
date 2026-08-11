/*
 * DaoGen — el generador de DAOs (V5/H5).
 * ============================================================================
 *
 * Toma las entidades anotadas con `@BD{...}` y emite su DAO. El objetivo está
 * escrito a mano en `samples/Orm.bp` + `samples/Medidas.bp`: esto no inventa
 * una forma, la REPRODUCE. El criterio de que está terminado es que lo generado
 * y lo escrito a mano no se distingan.
 *
 * ─── DECISIONES QUE CONVIENE CONOCER ANTES DE TOCAR ESTO ────────────────────
 *
 * **Emite BP FUENTE, no `.mod`.** El DAO generado es la clase de la que HEREDA
 * el usuario (tercer nivel del diseño), así que tiene que poder leerla. Además
 * entra por el mismo compilador que todo lo demás —no puede generar algo que el
 * lenguaje no exprese—, tiene números de línea y sale en el depurador.
 *
 * **Trabaja sólo sobre el AST.** No hace falta el análisis semántico: el tipo
 * SINTÁCTICO de la property (`long`, `double`, `string`…) ya dice si toca
 * `getLong`, `getDouble` o `getStr`. Eso hace que generar no dependa de que
 * resuelvan los imports ni las dependencias — sólo de que el fichero parsee.
 *
 * **Un módulo de DAOs por módulo de entidades**, con un DAO por entidad
 * (criterio de Eduardo: copiar la disposición que eligió el usuario). El
 * módulo `Medidas` produce `MedidasDao` con la clase `MedidaDao` dentro.
 *
 * **La entidad y el DAO NO pueden compartir fichero**: la entidad es código del
 * usuario y el DAO se rehace. Por eso el generado es su propio módulo e importa
 * al de las entidades.
 *
 * **Van al `sourceDir`**, no a una carpeta aparte, porque `scanBpSources` no es
 * recursivo y el compilador no encontraría una subcarpeta. Se distinguen por el
 * sufijo `Dao` y por la cabecera de NO EDITAR.
 *
 * ─── EL SEGURO CONTRA EL QUE EDITA LO GENERADO ──────────────────────────────
 *
 * La cabecera lleva un hash del cuerpo. Al regenerar se recalcula sobre lo que
 * hay en disco: si no cuadra, ese fichero está tocado a mano y NO se pisa — se
 * dice y se para. Es preferible una queja a tirarle el trabajo a alguien.
 *
 * ─── LO QUE NO PUEDE EMITIR (limitaciones MEDIDAS del compilador) ───────────
 *
 *   #392  nada de métodos SOBRECARGADOS: una hija con sobrecargas, usada desde
 *         otro módulo, descoloca los slots y falla al ENLAZAR. Por eso el
 *         nombrado es `load(e)` / `loadById(id)` y no dos `load`.
 *   #388  nada de encadenado: lo que devuelve un método pierde sus miembros.
 *   #390/#391  sin `protected` ni `virtual`: lo heredable va `public`.
 *   #249  la continuación de línea con '+' sólo vale DENTRO de paréntesis.
 */
package basicplus.frontend;

import basicplus.frontend.Ast.*;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class DaoGen {

    /** Marca de la cabecera que lleva el hash del cuerpo. */
    private static final String MARCA_HASH = "// @generado ";

    // ---- Modelo intermedio: lo que el generador necesita saber ----

    private static final class Campo {
        final String prop;      // nombre de la property en BP
        final String columna;   // nombre en la tabla
        final String tipo;      // tipo sintáctico: long | integer | double | float | string | boolean
        final boolean pk;
        Campo(String prop, String columna, String tipo, boolean pk) {
            this.prop = prop; this.columna = columna; this.tipo = tipo; this.pk = pk;
        }
    }

    private static final class Entidad {
        final String clase;
        final String tabla;
        final List<Campo> campos = new ArrayList<>();
        Entidad(String clase, String tabla) { this.clase = clase; this.tabla = tabla; }
        Campo pk() { for (Campo c : campos) if (c.pk) return c; return null; }
    }

    /** Resultado de una pasada: para que el llamante decida qué hacer. */
    public static final class Resultado {
        public final List<String> errores = new ArrayList<>();
        public final List<String> avisos  = new ArrayList<>();
        public final List<String> escritos = new ArrayList<>();   // ficheros generados
        public final List<String> sinCambios = new ArrayList<>(); // ya estaban al día
        public int entidades = 0;
        public boolean ok() { return errores.isEmpty(); }
    }

    private DaoGen() { }

    // ========================================================================
    //  PUNTO DE ENTRADA
    // ========================================================================

    /**
     * Barre el proyecto, encuentra las entidades y produce sus DAOs.
     *
     * @param escribir  true = `DAO build` (genera). false = modo INFORME, el que
     *                  llama `Build`: dice qué pasaría y no toca el disco. Es la
     *                  misma maquinaria con dos entradas, para que la detección
     *                  siga siendo automática aunque la generación sea manual.
     */
    public static Resultado generar(BpBuild proj, boolean escribir) {
        Resultado r = new Resultado();
        Path srcDir = Paths.get(proj.sourceDir);

        // Entidades agrupadas por módulo: un módulo de DAOs por módulo de
        // entidades, que es la disposición que pidió Eduardo.
        Map<String, List<Entidad>> porModulo = new LinkedHashMap<>();

        try (DirectoryStream<Path> ds = Files.newDirectoryStream(srcDir, "*.bp")) {
            for (Path f : ds) {
                // Los propios generados se saltan: si no, la segunda pasada
                // intentaría generar el DAO de un DAO.
                if (esGenerado(f)) continue;
                try {
                    String fuente = new String(Files.readAllBytes(f), StandardCharsets.UTF_8);
                    ModuleNode mod = parsear(fuente, f, r);
                    if (mod == null) continue;
                    List<Entidad> ents = entidadesDe(mod, f, r);
                    if (!ents.isEmpty()) porModulo.put(mod.name, ents);
                } catch (IOException ex) {
                    r.errores.add(f + ": no se pudo leer (" + ex.getMessage() + ")");
                }
            }
        } catch (IOException ex) {
            r.errores.add("no se pudo recorrer " + srcDir + ": " + ex.getMessage());
            return r;
        }

        if (porModulo.isEmpty()) {
            r.avisos.add("no hay ninguna entidad: ninguna clase lleva `@BD{ tabla = \"...\" }`");
            return r;
        }
        if (!r.ok()) return r;   // con entidades mal declaradas no se genera nada

        for (Map.Entry<String, List<Entidad>> e : porModulo.entrySet()) {
            r.entidades += e.getValue().size();
            emitirModulo(proj, srcDir, e.getKey(), e.getValue(), escribir, r);
        }
        return r;
    }

    // ========================================================================
    //  LEER LAS ENTIDADES DEL AST
    // ========================================================================

    private static ModuleNode parsear(String fuente, Path f, Resultado r) {
        Lexer lx = new Lexer(fuente);
        List<Token> toks = lx.tokenize();
        if (!lx.getErrors().isEmpty()) {
            // No es cosa nuestra arreglarlo: que lo diga el compilador de
            // verdad. Aquí sólo se salta el fichero.
            return null;
        }
        Parser ps = new Parser(toks);
        ModuleNode mod = ps.parseModule();
        if (mod == null || !ps.getErrors().isEmpty()) return null;
        return mod;
    }

    /** Saca las entidades de un módulo ya parseado, validando lo que el
     *  generador necesita y que el semántico no puede saber. */
    private static List<Entidad> entidadesDe(ModuleNode mod, Path f, Resultado r) {
        List<Entidad> out = new ArrayList<>();
        for (ITopLevelDecl def : mod.defs) {
            if (!(def instanceof ClassDef)) continue;
            ClassDef cd = (ClassDef) def;
            if (cd.annotation == null || !"BD".equals(cd.annotation.name)) continue;

            Ast.AnnEntry tabla = cd.annotation.get("tabla");
            if (tabla == null || !(tabla.value instanceof String)) continue;  // ya lo dijo el semántico
            Entidad ent = new Entidad(cd.name, (String) tabla.value);

            for (ITopLevelDecl m : cd.members) {
                if (!(m instanceof PropertyDef)) continue;
                PropertyDef p = (PropertyDef) m;
                if (p.annotation == null || !"BD".equals(p.annotation.name)) continue;

                String tipo = nombreDeTipo(p.type);
                if (tipo == null || mapeoLectura(tipo) == null) {
                    r.errores.add(loc(f, p) + ": el generador no sabe llevar '" + p.name.name
                            + "' a una columna: su tipo es '" + (tipo == null ? "?" : tipo)
                            + "' y sólo se mapean long, integer, double, float, string y boolean");
                    continue;
                }
                Ast.AnnEntry col = p.annotation.get("columna");
                String columna = (col != null && col.value instanceof String)
                        ? (String) col.value : p.name.name;
                ent.campos.add(new Campo(p.name.name, columna, tipo, p.annotation.get("pk") != null));
            }

            if (ent.campos.isEmpty()) {
                r.errores.add(loc(f, cd) + ": la entidad '" + cd.name
                        + "' no tiene ninguna property marcada con `@BD`: no hay nada que guardar");
                continue;
            }
            if (ent.pk() == null) continue;   // ya lo dijo el semántico
            out.add(ent);
        }
        return out;
    }

    /** El tipo TAL COMO ESTÁ ESCRITO. No hace falta resolverlo: para elegir
     *  entre `getLong` y `getStr` basta con lo que puso el usuario. */
    private static String nombreDeTipo(TypeRef t) {
        if (t instanceof SimpleTypeRef) return ((SimpleTypeRef) t).name;
        return null;   // arrays, tuplas, tipos de clase: no van a una columna
    }

    private static String mapeoLectura(String tipo) {
        switch (tipo) {
            case "long":                 return "getLong";
            case "integer": case "byte": case "word": case "int8": case "int16":
                                         return "getInt";
            case "double": case "float": return "getDouble";
            case "string":               return "getStr";
            case "boolean":              return "getInt";   // SQLite guarda los bool como enteros
            default:                     return null;
        }
    }

    /** Cómo se escribe el valor de un campo dentro del SQL. Las cadenas van
     *  entre comillas y ESCAPADAS —`Orm.esc` es el único escapado de la
     *  librería—; los números, tal cual. */
    private static String valorSql(Campo c, String obj) {
        String acc = obj + "." + c.prop;
        switch (c.tipo) {
            case "string":
                return "\"'\" + Orm.esc(" + acc + ") + \"'\"";
            case "double": case "float":
                return "Str.doubleToString(" + acc + ")";
            case "boolean":
                return "Str.longToString(" + acc + ")";   // ver nota en el .bp generado
            default:
                return "Str.longToString(" + acc + ")";
        }
    }

    private static String loc(Path f, Node n) {
        return f.getFileName() + ":" + n.line + ":" + n.column;
    }

    // ========================================================================
    //  EMITIR
    // ========================================================================

    private static void emitirModulo(BpBuild proj, Path srcDir, String modEnt,
                                     List<Entidad> ents, boolean escribir, Resultado r) {
        String modDao = modEnt + "Dao";
        Path destino = srcDir.resolve(modDao + ".bp");

        StringBuilder b = new StringBuilder();
        cuerpoModulo(b, modEnt, modDao, ents);
        String cuerpo = b.toString();
        String hash = hashCorto(cuerpo);

        // ¿Está ya al día? Entonces no se toca — así `DAO build` es idempotente
        // y no ensucia el control de versiones con cambios que no lo son.
        if (Files.isRegularFile(destino)) {
            String enDisco;
            try {
                enDisco = new String(Files.readAllBytes(destino), StandardCharsets.UTF_8);
            } catch (IOException ex) {
                r.errores.add(destino + ": no se pudo leer para comprobarlo (" + ex.getMessage() + ")");
                return;
            }
            String hashDeclarado = hashDeclaradoEn(enDisco);
            String hashReal = hashCorto(cuerpoDe(enDisco));
            if (hashDeclarado != null && !hashDeclarado.equals(hashReal)) {
                r.errores.add(destino.getFileName() + ": está EDITADO a mano (su hash no cuadra con su"
                        + " contenido) y no se pisa. Si los cambios no hacen falta, bórralo y vuelve a"
                        + " generar; si hacen falta, muévelos a una clase que herede de ésta");
                return;
            }
            if (hash.equals(hashReal)) { r.sinCambios.add(destino.getFileName().toString()); return; }
        }

        if (!escribir) {
            r.escritos.add(destino.getFileName() + " (se generaría)");
            return;
        }
        try {
            Files.write(destino, (cabecera(modEnt, hash) + cuerpo).getBytes(StandardCharsets.UTF_8));
            r.escritos.add(destino.getFileName().toString());
        } catch (IOException ex) {
            r.errores.add(destino + ": no se pudo escribir (" + ex.getMessage() + ")");
        }
    }

    private static String cabecera(String modEnt, String hash) {
        return "// ⚠️  NO EDITAR — este fichero lo genera `DAO build` a partir de las\n"
             + "//     entidades de `" + modEnt + ".bp`, y se REHACE cada vez.\n"
             + "//\n"
             + "//     Si necesitas más de lo que hay aquí, NO lo toques: crea una clase\n"
             + "//     que herede de la que quieras ampliar. Así puedes regenerar sin\n"
             + "//     perder tus cambios, que es para lo que está pensado.\n"
             + "//\n"
             + MARCA_HASH + hash + "\n\n";
    }

    private static void cuerpoModulo(StringBuilder b, String modEnt, String modDao, List<Entidad> ents) {
        b.append("module ").append(modDao).append("\n");
        b.append("  import SQLite\n");
        b.append("  import Orm\n");
        b.append("  import ").append(modEnt).append("\n");
        b.append("  import Str\n\n");
        for (Entidad e : ents) claseDao(b, modEnt, e);
        b.append("end ").append(modDao).append("\n");
    }

    private static void claseDao(StringBuilder b, String modEnt, Entidad e) {
        String tipoEnt = modEnt + "." + e.clase;
        String dao = e.clase + "Dao";
        Campo pk = e.pk();

        b.append("  // ").append(e.clase).append("  <->  tabla '").append(e.tabla).append("'\n");
        b.append("  public class ").append(dao).append(" extends Orm.Dao\n\n");

        b.append("    public function ").append(dao).append("(base: SQLite.Db)\n");
        b.append("      super(base)\n");
        b.append("    end ").append(dao).append("\n\n");

        // ── las tres del contrato ──
        b.append("    public function tabla(): string\n");
        b.append("      return \"").append(e.tabla).append("\"\n");
        b.append("    end tabla\n\n");

        b.append("    // EL ORDEN. `read` lo respeta, y por eso van pegados.\n");
        b.append("    public function columnas(): string\n");
        b.append("      return \"").append(listaColumnas(e)).append("\"\n");
        b.append("    end columnas\n\n");

        b.append("    public function read(q: SQLite.Query): Object\n");
        b.append("      var e: ").append(tipoEnt).append(" := ").append(tipoEnt).append("()\n");
        for (int i = 0; i < e.campos.size(); i++) {
            Campo c = e.campos.get(i);
            b.append("      e.").append(c.prop).append(" := q.")
             .append(mapeoLectura(c.tipo)).append("(").append(i).append(")\n");
        }
        b.append("      return e\n");
        b.append("    end read\n\n");

        // ── los verbos que conocen la entidad ──
        b.append("    public function loadById(").append(pk.prop).append(": ").append(pk.tipo)
         .append("): ").append(tipoEnt).append("\n");
        b.append("      return this.uno(\"").append(pk.columna).append(" = \" + ")
         .append(aTexto(pk.tipo, pk.prop)).append(")\n");
        b.append("    end loadById\n\n");

        b.append("    public function load(e: ").append(tipoEnt).append("): boolean\n");
        b.append("      var otro: ").append(tipoEnt).append(" := this.loadById(e.").append(pk.prop).append(")\n");
        b.append("      if otro == null then\n");
        b.append("        return false\n");
        b.append("      endif\n");
        b.append("      this.copiar(otro, e)\n");
        b.append("      return true\n");
        b.append("    end load\n\n");

        b.append("    public function refresh(e: ").append(tipoEnt).append("): boolean\n");
        b.append("      return this.load(e)\n");
        b.append("    end refresh\n\n");

        insert(b, tipoEnt, e, pk);

        b.append("    public function add(e: ").append(tipoEnt).append(")\n");
        b.append("      this.insert(e)\n");
        b.append("    end add\n\n");

        update(b, tipoEnt, e, pk);

        b.append("    // Por clave lo hereda de `Orm.Dao`. Éste además pone la pk a 0:\n");
        b.append("    // tras el borrado el objeto ya no es ninguna fila.\n");
        b.append("    public function delete(e: ").append(tipoEnt).append("): boolean\n");
        b.append("      var fue: boolean := this.deleteById(e.").append(pk.prop).append(")\n");
        b.append("      if fue then\n");
        b.append("        e.").append(pk.prop).append(" := 0\n");
        b.append("      endif\n");
        b.append("      return fue\n");
        b.append("    end delete\n\n");

        b.append("    function copiar(de: ").append(tipoEnt).append(", a: ").append(tipoEnt).append(")\n");
        for (Campo c : e.campos) b.append("      a.").append(c.prop).append(" := de.").append(c.prop).append("\n");
        b.append("    end copiar\n\n");

        b.append("  end ").append(dao).append("\n\n");
    }

    private static void insert(StringBuilder b, String tipoEnt, Entidad e, Campo pk) {
        List<Campo> sinPk = new ArrayList<>();
        for (Campo c : e.campos) if (!c.pk) sinPk.add(c);

        b.append("    // Al volver, el objeto lleva su clave puesta — la que asignó la BD.\n");
        b.append("    public function insert(e: ").append(tipoEnt).append(")\n");
        b.append("      this.db.exec(\"INSERT INTO \" + this.tabla() + \"(");
        for (int i = 0; i < sinPk.size(); i++) {
            if (i > 0) b.append(", ");
            b.append(sinPk.get(i).columna);
        }
        b.append(") VALUES(\"\n");
        for (int i = 0; i < sinPk.size(); i++) {
            b.append("            + ").append(valorSql(sinPk.get(i), "e"));
            if (i < sinPk.size() - 1) b.append(" + \", \"");
            b.append("\n");
        }
        b.append("            + \")\")\n");
        b.append("      e.").append(pk.prop).append(" := this.db.lastInsertId()\n");
        b.append("    end insert\n\n");
    }

    private static void update(StringBuilder b, String tipoEnt, Entidad e, Campo pk) {
        List<Campo> sinPk = new ArrayList<>();
        for (Campo c : e.campos) if (!c.pk) sinPk.add(c);

        b.append("    // Devuelve si tocó alguna fila: false = esa clave no estaba.\n");
        b.append("    public function update(e: ").append(tipoEnt).append("): boolean\n");
        b.append("      this.db.exec(\"UPDATE \" + this.tabla()\n");
        for (int i = 0; i < sinPk.size(); i++) {
            Campo c = sinPk.get(i);
            String sep = (i == 0) ? " SET " : ", ";
            b.append("            + \"").append(sep).append(c.columna).append(" = \" + ")
             .append(valorSql(c, "e")).append("\n");
        }
        b.append("            + \" WHERE ").append(pk.columna).append(" = \" + ")
         .append(aTexto(pk.tipo, "e." + pk.prop)).append(")\n");
        b.append("      return this.db.changes() > 0\n");
        b.append("    end update\n\n");
    }

    private static String aTexto(String tipo, String expr) {
        if ("string".equals(tipo)) return "\"'\" + Orm.esc(" + expr + ") + \"'\"";
        if ("double".equals(tipo) || "float".equals(tipo)) return "Str.doubleToString(" + expr + ")";
        return "Str.longToString(" + expr + ")";
    }

    private static String listaColumnas(Entidad e) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < e.campos.size(); i++) {
            if (i > 0) s.append(", ");
            s.append(e.campos.get(i).columna);
        }
        return s.toString();
    }

    // ========================================================================
    //  EL SEGURO DEL HASH
    // ========================================================================

    private static boolean esGenerado(Path f) {
        try {
            for (String l : Files.readAllLines(f, StandardCharsets.UTF_8)) {
                if (l.startsWith(MARCA_HASH)) return true;
                if (l.trim().startsWith("module ")) return false;   // ya pasó la cabecera
            }
        } catch (IOException ignored) { }
        return false;
    }

    private static String hashDeclaradoEn(String texto) {
        for (String l : texto.split("\r?\n")) {
            if (l.startsWith(MARCA_HASH)) return l.substring(MARCA_HASH.length()).trim();
        }
        return null;
    }

    /** El cuerpo = todo lo que hay tras la línea en blanco que cierra la
     *  cabecera. Es lo que se hashea, para que el propio hash no se hashee. */
    private static String cuerpoDe(String texto) {
        int i = texto.indexOf(MARCA_HASH);
        if (i < 0) return texto;
        int nl = texto.indexOf('\n', i);
        if (nl < 0) return "";
        String resto = texto.substring(nl + 1);
        return resto.startsWith("\n") ? resto.substring(1)
             : resto.startsWith("\r\n") ? resto.substring(2) : resto;
    }

    private static String hashCorto(String s) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            // Normalizamos los finales de línea: que un checkout con CRLF no
            // haga creer que alguien editó el fichero.
            byte[] h = md.digest(s.replace("\r\n", "\n").getBytes(StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 8; i++) sb.append(String.format("%02x", h[i]));
            return sb.toString();
        } catch (Exception ex) {
            return "0000000000000000";
        }
    }
}
