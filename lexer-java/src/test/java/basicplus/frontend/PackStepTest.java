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

    @Test void outDirSinModulosEsError(@TempDir Path tmp) throws Exception {
        Path outDir = Files.createDirectories(tmp.resolve("out"));
        Files.write(outDir.resolve("App.bpi"), "solo-interfaz".getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.outDir = outDir.toString();
        b.main = "App";
        b.out = "pack";
        assertThrows(IOException.class, () -> PackStep.buildPack(b));
    }
}
