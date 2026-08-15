package basicplus.frontend;

import basicplus.pack.PackEntry;
import basicplus.pack.PackReader;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.*;

/**
 * El paso de pack del build: junta .mod/.mdn del outDir + resources + manifest,
 * y NUNCA el .bpi/.slots. Verifica leyendo el pack producido.
 */
class PackStepTest {

    @Test void empaquetaOutDirMasResourcesMasManifest(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Files.write(outDir.resolve("App.mod"),    "app-bytecode".getBytes(StandardCharsets.UTF_8));
        Files.write(outDir.resolve("Helper.mod"), "helper-bytecode".getBytes(StandardCharsets.UTF_8));
        Files.write(outDir.resolve("App.mdn"),    new byte[]{1, 2, 3, 4});
        Files.write(outDir.resolve("App.bpi"),    "NO-DEBE-IR".getBytes(StandardCharsets.UTF_8));   // interfaz
        Files.write(outDir.resolve("App.slots"),  "NO-DEBE-IR".getBytes(StandardCharsets.UTF_8));   // debug
        Path resDir = Files.createDirectories(tmp.resolve("resources"));
        Files.write(resDir.resolve("logo.png"), new byte[]{(byte) 0x89, 'P', 'N', 'G'});

        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.outDir = outDir.toString();
        b.main = "App";
        b.out = "pack";
        // V5/H8 — el nombre del pack ya NO sale de `main`: es suyo (`pack.name`
        // o el fichero de proyecto). Un BpBuild hecho a mano no tiene fichero,
        // así que lo dice. Ver el comentario largo en PackStep.buildPack.
        b.packName = "App";

        Path pack = PackStep.buildPack(b);
        assertTrue(Files.exists(pack));
        assertEquals("App.pack", pack.getFileName().toString());

        PackReader.Pack p = PackReader.read(Files.readAllBytes(pack));
        Set<String> got = new HashSet<>();
        for (PackEntry e : p.entries) got.add(e.tipo + ":" + e.nombre);

        assertTrue(got.contains("mod:App"),      got.toString());
        assertTrue(got.contains("mod:Helper"),   got.toString());
        assertTrue(got.contains("mdn:App"),      got.toString());
        assertTrue(got.contains("png:logo"),     got.toString());
        assertTrue(got.contains("mft:manifest"), got.toString());
        assertEquals(5, p.entries.size(), "sólo mod/mdn + resource + manifest (ni .bpi ni .slots)");

        for (PackEntry e : p.entries)
            if ("mft".equals(e.tipo))
                assertEquals("main=App\n", new String(e.data, StandardCharsets.UTF_8));
    }

    /**
     * #365 — UN MÓDULO CON `library` PUEDE ARRANCAR UN PACK.
     *
     * <p>Antes no podía, y el motivo era éste: el `.mod` de un módulo con
     * `library` se llama `com.example.Demo.mod`, o sea que su entrada en el pack
     * es `com.example.Demo`; pero el manifest escribía `main=<proj.main>`, y
     * `proj.main` nombra el FICHERO FUENTE (`Demo.bp`). Quien arranca busca la
     * entrada LITERAL, así que no la encontraba — y poner el nombre cualificado
     * en `main` tampoco valía, porque entonces no encontraba el fuente.
     *
     * <p>El arreglo es que el manifest lleve el nombre CANÓNICO, que lo compone
     * el compilador y llega en el `Cierre`. Lo que se comprueba aquí es
     * exactamente eso: que el manifest dice `com.example.Demo` (y no `Demo`), que
     * es la entrada que existe de verdad dentro del pack.
     */
    @Test void moduloConLibraryPuedeSerElMain(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Files.write(outDir.resolve("com.example.Demo.mod"), "demo".getBytes(StandardCharsets.UTF_8));

        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.outDir = outDir.toString();
        b.main = "Demo";              // el FUENTE se llama Demo.bp
        b.out = "pack";
        b.packName = "Demo";

        PackStep.Cierre c = new PackStep.Cierre(
                java.util.Collections.emptyMap(), java.util.Collections.emptySet(),
                /*ejecutable*/ true, /*mainEntry*/ "com.example.Demo");

        PackReader.Pack p = PackReader.read(Files.readAllBytes(PackStep.buildPack(b, c)));
        Set<String> got = new HashSet<>();
        for (PackEntry e : p.entries) got.add(e.tipo + ":" + e.nombre);
        assertTrue(got.contains("mod:com.example.Demo"), got.toString());

        for (PackEntry e : p.entries)
            if ("mft".equals(e.tipo))
                assertEquals("main=com.example.Demo\n", new String(e.data, StandardCharsets.UTF_8),
                        "el manifest tiene que nombrar la ENTRADA, no el fichero fuente");
    }

    /**
     * #365, la trampa de al lado: un módulo llamado `Npk` (o `Mdn`) DENTRO de una
     * librería produce `com.example.Npk.mod`, y la regla de doble extensión
     * —la que existe para `sqlite.npk.RISCV`— miraba el penúltimo componente, veía
     * `npk` y renombraba la entrada a `com.example.mod` con tipo `npk`. Muda, y
     * dentro de un pack ya grabado.
     */
    @Test void nombreCualificadoQueTerminaEnUnTipoNoSeMalinterpreta(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Files.write(outDir.resolve("com.example.Npk.mod"), "demo".getBytes(StandardCharsets.UTF_8));
        /* Y el de verdad, para que quede claro que la doble extensión sigue viva:
         * éste SÍ tiene que salir como tipo `npk` con el destino en el nombre. */
        Files.write(outDir.resolve("motor.npk.RISCV"), new byte[]{9});

        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.outDir = outDir.toString();
        b.main = "Npk";
        b.out = "pack";
        b.packName = "Demo";

        PackStep.Cierre c = new PackStep.Cierre(
                java.util.Collections.emptyMap(), java.util.Collections.emptySet(),
                true, "com.example.Npk");

        PackReader.Pack p = PackReader.read(Files.readAllBytes(PackStep.buildPack(b, c)));
        Set<String> got = new HashSet<>();
        for (PackEntry e : p.entries) got.add(e.tipo + ":" + e.nombre);
        assertTrue(got.contains("mod:com.example.Npk"), got.toString());
        assertFalse(got.contains("npk:com.example.mod"), "la doble extensión se comió el nombre: " + got);
        assertTrue(got.contains("npk:motor.RISCV"), "…y la doble extensión de verdad sigue funcionando: " + got);
    }

    @Test void outDirSinModulosEsError(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Files.write(outDir.resolve("App.bpi"), "solo-interfaz".getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.outDir = outDir.toString();
        b.main = "App";
        b.out = "pack";
        // V5/H8 — el nombre del pack ya NO sale de `main`: es suyo (`pack.name`
        // o el fichero de proyecto). Un BpBuild hecho a mano no tiene fichero,
        // así que lo dice. Ver el comentario largo en PackStep.buildPack.
        b.packName = "App";
        assertThrows(IOException.class, () -> PackStep.buildPack(b));
    }
}
