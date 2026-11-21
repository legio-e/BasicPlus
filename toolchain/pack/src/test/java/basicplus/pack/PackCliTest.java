package basicplus.pack;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * El CLI trabaja sobre una LISTA PLANA de ficheros (no lee proyectos). El
 * ciclo build → extract reproduce los originales byte a byte: la red de
 * seguridad del formato de punta a punta.
 */
class PackCliTest {

    /** Corre el CLI tragándose la salida; devuelve el código de salida. */
    private static int run(String... args) {
        PrintStream sink = new PrintStream(new ByteArrayOutputStream());
        return PackCli.run(args, sink, sink);
    }

    @Test void buildListExtractRoundTrip(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path mod = src.resolve("App.mod");  Files.write(mod, "bytecode-de-App".getBytes(StandardCharsets.UTF_8));
        Path mdn = src.resolve("App.mdn");  Files.write(mdn, new byte[]{1, 2, 3, 4, 5});
        Path png = src.resolve("logo.png"); Files.write(png, new byte[]{(byte) 0x89, 'P', 'N', 'G', 0, 0, (byte) 0xFF});
        Path mft = src.resolve("App.mft");  Files.write(mft, "main=App\n".getBytes(StandardCharsets.UTF_8));

        Path pack = tmp.resolve("App.pack");
        assertEquals(0, run("build", "--out", pack.toString(), "--name", "App", "--version", "1.0",
                            "--date", "1700000000",
                            mod.toString(), mdn.toString(), png.toString(), mft.toString()));
        assertTrue(Files.exists(pack));
        assertEquals(0, Files.size(pack) % PackFormat.DEFAULT_BLOCK, "múltiplo del bloque");

        assertEquals(0, run("list", pack.toString()));

        Path out = tmp.resolve("out");
        assertEquals(0, run("extract", pack.toString(), out.toString()));
        assertArrayEquals(Files.readAllBytes(mod), Files.readAllBytes(out.resolve("App.mod")));
        assertArrayEquals(Files.readAllBytes(mdn), Files.readAllBytes(out.resolve("App.mdn")));
        assertArrayEquals(Files.readAllBytes(png), Files.readAllBytes(out.resolve("logo.png")));
        assertArrayEquals(Files.readAllBytes(mft), Files.readAllBytes(out.resolve("App.mft")));
    }

    @Test void buildDesdeListaDeFicheros(@TempDir Path tmp) throws Exception {
        Path a = tmp.resolve("A.mod"); Files.write(a, "aaa".getBytes(StandardCharsets.UTF_8));
        Path b = tmp.resolve("B.mod"); Files.write(b, "bbbb".getBytes(StandardCharsets.UTF_8));
        Path lst = tmp.resolve("files.txt");
        Files.write(lst, (a + System.lineSeparator() + "# un comentario" + System.lineSeparator() + b)
                .getBytes(StandardCharsets.UTF_8));

        Path pack = tmp.resolve("Lib.pack");
        assertEquals(0, run("build", "--out", pack.toString(), "--files", lst.toString()));

        PackReader.Pack p = PackReader.read(Files.readAllBytes(pack));
        assertEquals(2, p.entries.size());
        assertEquals("Lib", p.nombre, "sin --name, el default es el basename de --out");
    }

    @Test void ficheroSinExtensionEsError(@TempDir Path tmp) throws Exception {
        Path noext = tmp.resolve("README"); Files.write(noext, "x".getBytes(StandardCharsets.UTF_8));
        Path pack = tmp.resolve("P.pack");
        assertNotEquals(0, run("build", "--out", pack.toString(), noext.toString()));
    }

    @Test void faltaOutEsError(@TempDir Path tmp) throws Exception {
        Path a = tmp.resolve("A.mod"); Files.write(a, "a".getBytes(StandardCharsets.UTF_8));
        assertNotEquals(0, run("build", a.toString()));
    }

    @Test void sinArgumentosEsError() {
        assertNotEquals(0, run());
    }

    @Test void subcomandoDesconocidoEsError() {
        assertNotEquals(0, run("chorizo"));
    }
}
