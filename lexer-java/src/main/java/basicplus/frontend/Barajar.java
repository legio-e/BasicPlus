package basicplus.frontend;

import java.util.Collections;
import java.util.Random;

/**
 * V7/H2 (#523, V1) — EL GUARDIÁN DEL COMPILADOR: {@code --barajar=<semilla>} y
 * {@code --relleno=<n>}.
 *
 * <p>El enlace tardío (MOD9) promete que un consumidor compilado contra una versión de
 * una librería corre contra OTRA versión con las ranuras en otro sitio: nada del
 * consumidor puede llevar horneado un número que dependa de la forma de una clase
 * ajena. Los censos de sitios se quedan cortos por construcción (el que se escapa es
 * el que nadie leyó); esto los caza sin leerlos: se compila la librería con las
 * ranuras propias de cada clase PERMUTADAS (y con miembros privados de relleno que
 * corren todo lo demás), y el consumidor de la semilla 0 tiene que dar el mismo
 * stdout contra cualquier semilla, en las dos VMs.
 *
 * <p>Cómo: se permuta la lista de miembros del AST de cada clase justo después de
 * parsear. Así TODO lo que numera ranuras (computeClassLayout, el emisor, la interfaz,
 * el `.slots`) ve el mismo orden sin tocar ninguno de los quince sitios que recorren
 * `members`: la semántica de una clase no depende del orden de declaración. Semilla 0
 * = identidad (los artefactos de siempre valen como «App(0)»). La permutación es
 * determinista por (semilla, nombre de la clase).
 *
 * <p>El relleno son properties PRIVADAS autoimplementadas ({@code private property
 * __relleno0Cls: integer}): cada una ocupa un campo (el backing) y dos ranuras de
 * vtable (getter y setter) — un método privado no ocupa vtable (ver `addPrivateMethod`),
 * así que no serviría. El nombre lleva la clase para que nunca coincida con uno del
 * padre (un homónimo sería un override que no desplaza).
 *
 * <p>NO es para la stdlib: `Core.Exception.msg` = campo 0 y `Object.toString` = vt[0]
 * están horneados en las VMs (R5 del diseño), y `Object`/`Thread` se sintetizan fuera
 * del AST (no se barajan). Es un instrumento de prueba de la matriz `compat/mezcla.py`.
 */
final class Barajar {
    private Barajar() {}

    /** Aplica relleno y permutación a las clases del módulo recién parseado (si los flags lo piden). */
    static void aplicar(Ast.ModuleNode m) {
        if (m == null || m.defs == null) return;
        if (Main.RELLENO <= 0 && Main.BARAJAR == 0) return;
        for (Ast.ITopLevelDecl d : m.defs) {
            if (!(d instanceof Ast.ClassDef)) continue;
            Ast.ClassDef cd = (Ast.ClassDef) d;
            if (cd.members == null) continue;
            for (int i = 0; i < Main.RELLENO; i++) cd.members.add(i, relleno(cd, i));
            if (Main.BARAJAR != 0) Collections.shuffle(cd.members, new Random(semilla(cd.name)));
        }
    }

    private static long semilla(String clase) { return Main.BARAJAR * 1000003L + clase.hashCode(); }

    private static Ast.PropertyDef relleno(Ast.ClassDef cd, int i) {
        String nombre = "__relleno" + i + cd.name;
        Ast.PropertyDef p = new Ast.PropertyDef(false, false, false, false,
                new Ast.DeclName(null, nombre, cd.line, 0), new Ast.SimpleTypeRef("integer", cd.line, 0),
                null, null, null, true, cd.line, 0);
        p.isPrivate = true;
        return p;
    }
}
