// ============================================================
// ProjectSaveSmoke.java — #436: guardar el proyecto NO debe perder nada.
//
// El diálogo escribe el `.bpbuild` con `BpBuild.save()`. La pregunta que hay
// que contestar antes de dejar que alguien lo use es: ¿qué se pierde en el
// viaje de ida y vuelta? Se prueba contra el proyecto MÁS COMPLICADO que hay
// (SQLite.bpbuild: dos familias, `sources` relativos, bloque `pack` con cuatro
// campos y un array `_comentario` que el IDE no conoce).
//
// Lo que se comprueba, y por qué cada cosa:
//   · las claves que el IDE NO gestiona siguen ahí (`_comentario`, `sourceDir`…)
//     — un editor que se lleva por delante lo que no entiende es una trampa;
//   · `sources` vuelve RELATIVO como estaba, no resuelto a absoluto — si no, el
//     proyecto deja de funcionar en otra máquina y sin dar ningún error;
//   · dos familias siguen escribiéndose como `targets` y no como `target` —
//     perder una familia no da error, da un pack sin ese motor;
//   · y lo escrito lo vuelve a aceptar el cargador.
//
//   mvn -f BpIde/pom.xml exec:java \
//       -Dexec.mainClass=com.mycompany.bpide.ProjectSaveSmoke
// ============================================================
package com.mycompany.bpide;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import basicplus.frontend.BpBuild;

public final class ProjectSaveSmoke {

    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    public static void main(String[] args) throws Exception {
        Path orig = (args.length > 0) ? Paths.get(args[0])
                                      : Paths.get("bpstdlib/sqlite/SQLite.bpbuild");
        if (!Files.isRegularFile(orig)) {
            System.out.println("(skip: no está " + orig.toAbsolutePath() + ")");
            return;
        }
        System.out.println("--- #436: ida y vuelta de " + orig.getFileName() + " ---");

        // Se trabaja sobre una COPIA en su sitio: `sources` son relativos al
        // proyecto y el cargador comprueba que los .bp existan de verdad.
        Path copia = orig.getParent().resolve("_smoke436.bpbuild");
        Files.copy(orig, copia, java.nio.file.StandardCopyOption.REPLACE_EXISTING);
        try {
            String antes = new String(Files.readAllBytes(copia), StandardCharsets.UTF_8);
            BpBuild p = BpBuild.load(copia);

            check(!p.aotTargets.isEmpty(), "de partida declara varias familias: " + p.aotTargets);
            check(!p.sourcesDeclarados.isEmpty(),
                  "de partida declara sources: " + p.sourcesDeclarados);

            p.save(copia);                       // guardar SIN cambiar nada
            String despues = new String(Files.readAllBytes(copia), StandardCharsets.UTF_8);

            check(despues.contains("_comentario"),
                  "conserva `_comentario`, que el IDE no gestiona");
            check(despues.contains("\"sourceDir\""), "conserva `sourceDir`");
            check(despues.contains("\"outDir\""), "conserva `outDir`");
            check(despues.contains("\"targets\"") && !despues.contains("\"target\":"),
                  "sigue escribiendo `targets` (lista) y NO `target` (singular)");
            for (String s : p.sourcesDeclarados) {
                check(despues.contains("\"" + s + "\""),
                      "`sources` vuelve tal cual: " + s);
            }
            check(!despues.contains(orig.getParent().toAbsolutePath().toString().replace('\\', '/')),
                  "no ha colado ninguna ruta ABSOLUTA del proyecto");

            BpBuild q = BpBuild.load(copia);     // y lo escrito se puede releer
            check(q.aotTargets.equals(p.aotTargets), "las familias sobreviven la vuelta");
            check(q.sourcesDeclarados.equals(p.sourcesDeclarados), "los sources sobreviven");
            check(nvl(q.packName).equals(nvl(p.packName))
                  && nvl(q.packVersion).equals(nvl(p.packVersion))
                  && nvl(q.packProvides).equals(nvl(p.packProvides)),
                  "los datos del pack sobreviven (" + q.packName + " " + q.packVersion
                  + " " + q.packProvides + ")");
            check(antes.length() > 0 && despues.length() > 0, "el fichero no ha quedado vacío");

            System.out.println("[status=" + (fallos == 0 ? "OK" : "FAIL") + "]");
        } finally {
            Files.deleteIfExists(copia);
        }
        if (fallos != 0) System.exit(1);
    }

    private static String nvl(String s) { return (s == null) ? "" : s; }
}
