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

    /** [V7/H6, #563/#564] Un `.mod` mínimo pero con la cabecera REAL de su versión (la que
     *  lee duenosDeImports: 44 B en MOD8/MOD9) y una sección de imports de verdad. */
    static byte[] modSintetico(int version, int gen, String... imports) throws IOException {
        java.io.ByteArrayOutputStream ib = new java.io.ByteArrayOutputStream();
        java.io.DataOutputStream di = new java.io.DataOutputStream(ib);
        di.writeInt(imports.length);
        for (String s : imports) { di.writeUTF(s); di.writeUTF(""); }
        byte[] imp = ib.toByteArray();
        java.io.ByteArrayOutputStream ob = new java.io.ByteArrayOutputStream();
        java.io.DataOutputStream d = new java.io.DataOutputStream(ob);
        d.writeInt(0x4D4F4430 + version);          /* "MOD<v>" */
        d.writeInt(0); d.writeInt(0);               /* dataSize, mainOffset */
        d.writeInt(imp.length);                     /* importSize */
        d.writeInt(0); d.writeInt(0);               /* exportSize, codeSize */
        d.writeInt(0);                              /* librarySize */
        if (version >= 6) d.writeInt(0);            /* interfaceSize */
        if (version >= 7) d.writeInt(0);            /* nativeSize */
        if (version >= 8) { d.writeInt(gen); d.writeInt(0); }   /* gen, ifaceCrc */
        d.write(imp);
        return ob.toByteArray();
    }

    /** [#564] El cierre lee los imports en TODOS los formatos con cabecera conocida — MOD9
     *  incluido, que es el que escribe el compilador desde V7/H2 y el que se le escapaba. */
    @Test void duenosDeImportsEnCadaFormato() throws Exception {
        for (int v : new int[] { 6, 7, 8, 9 }) {
            java.util.List<String> d = PackStep.duenosDeImports(modSintetico(v, 7, "Core.print", "acme.Lib.__init", "Core.List#add#3"));
            assertEquals(java.util.Arrays.asList("Core", "acme.Lib"), d, "MOD" + v);
        }
        assertTrue(PackStep.duenosDeImports("no es un mod".getBytes(StandardCharsets.UTF_8)).isEmpty());
    }

    /** [#564] El cierre TRANSITIVO de un pack con módulos MOD9: App importa Lib y Lib
     *  importa Base; ni Lib ni Base están en el outDir. Tienen que entrar los dos. */
    @Test void cierreTransitivoConModulosMod9(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Path stdlib = Files.createDirectories(tmp.resolve("stdlib"));
        Files.write(outDir.resolve("App.mod"), modSintetico(9, 7, "Lib.f"));
        Files.write(stdlib.resolve("Lib.mod"), modSintetico(9, 7, "Base.g"));
        Files.write(stdlib.resolve("Base.mod"), modSintetico(9, 7));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.outDir = outDir.toString(); b.main = "App"; b.out = "pack"; b.packName = "App";
        PackStep.Cierre c = new PackStep.Cierre(java.util.Collections.emptyMap(), java.util.Collections.emptySet(),
                true, "", java.util.Collections.singletonList(stdlib));
        PackReader.Pack p = PackReader.read(Files.readAllBytes(PackStep.buildPack(b, c)));
        Set<String> got = new HashSet<>();
        for (PackEntry e : p.entries) got.add(e.tipo + ":" + e.nombre);
        assertTrue(got.contains("mod:Lib") && got.contains("mod:Base"),
                   "el cierre trae Lib (import directo) y Base (de segunda mano): " + got);
    }

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
