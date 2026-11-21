package basicplus.frontend;

import static org.junit.jupiter.api.Assertions.*;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/**
 * V7/H2 (#441) — las recetas del `.mdn` se leen del `.h` REAL de bpgenvm-c, y la
 * huella que el IDE estampa es el CRC32 IEEE de los bytes del literal: el mismo
 * que calcula `bpvm_crc32` en el firmware. Si esto se rompe, el firmware rechaza
 * todos los `.mdn` con «receta X, este firmware es Y» y nadie sabe por qué.
 */
class AotRecetasTest {

    private static Path ficheroReal() {
        /* lexer-java/ → ../../bpgenvm-c/include/bpvm_aot_recetas.h */
        Path p = Paths.get("..", "..", "bpgenvm-c", "include", AotRecetas.FICHERO);
        assertTrue(Files.isRegularFile(p), "el .h de las recetas tiene que estar en " + p.toAbsolutePath());
        return p;
    }

    @Test
    @DisplayName("el .h real trae las recetas de los dos destinos del IDE, por ISA/ABI y sin escapes")
    void recetasReales() throws IOException {
        AotRecetas r = AotRecetas.leer(ficheroReal());
        for (NpackReloc.Destino d : NpackReloc.DESTINOS) {
            String lit = r.literal(d.receta);
            assertNotNull(lit, "el destino " + d.nombre + " apunta a la receta " + d.receta + ", que no está en el .h");
            assertFalse(lit.contains("\\"), "sin escapes: el CRC es el de los bytes tal cual");
            String[] flags = AotRecetas.flags(lit);
            assertTrue(flags.length >= 3, "una receta son varios flags: " + lit);
            for (String f : flags) assertTrue(f.startsWith("-"), "cada trozo es un flag de gcc: '" + f + "'");
        }
        /* Las decisiones que costaron un cuelgue mudo, por escrito en la receta. */
        String rv = r.literal(NpackReloc.RISCV32_ESP_P4.receta);
        assertTrue(rv.contains("-mcmodel=medany"), "#440: sin medany los datos se direccionan en absoluto");
        assertTrue(rv.contains("-mabi=ilp32f") && rv.contains("-march=rv32imafc"), "la ABI del P4, explícita");
        String arm = r.literal(NpackReloc.ARM_CORTEX_M33.receta);
        assertTrue(arm.contains("-mfloat-abi=softfp") && arm.contains("-fpic"), "el .mdn ARM es softfp y PIC");
        /* Y la ISA sin FPU de los C3/C6 está descrita aunque el IDE aún no la produzca (#502). */
        assertNotNull(r.literal("RV32IMC_ILP32"));
    }

    @Test
    @DisplayName("la huella es CRC32 IEEE de los bytes del literal (el mismo que bpvm_crc32)")
    void huellaEsCrc32Ieee() {
        /* Vector clásico: CRC32("123456789") = 0xCBF43926. */
        assertEquals(0xCBF43926, AotRecetas.crc("123456789"));
        /* Un espacio de más cambia la huella: la receta ES el texto. */
        assertNotEquals(AotRecetas.crc("-Os -fpic"), AotRecetas.crc("-Os  -fpic"));
    }

    @Test
    @DisplayName("un literal con escape se rechaza, y un .h sin recetas también")
    void rechazos(@TempDir Path dir) throws IOException {
        Path malo = dir.resolve("a.h");
        Files.write(malo, "#define BPVM_AOT_RECETA_X \"-Os -DA=\\\"b\\\"\"\n".getBytes(StandardCharsets.UTF_8));
        IOException e = assertThrows(IOException.class, () -> AotRecetas.leer(malo));
        assertTrue(e.getMessage().contains("escape"), e.getMessage());

        Path vacio = dir.resolve("b.h");
        Files.write(vacio, "#define BPVM_AOT_HOST_RECETA \"\"\n".getBytes(StandardCharsets.UTF_8));
        assertThrows(IOException.class, () -> AotRecetas.leer(vacio));

        assertThrows(IOException.class, () -> AotRecetas.leer(dir.resolve("no-existe.h")));
    }

    @Test
    @DisplayName("nombreDe devuelve la receta de una huella, para decirla con nombre en el IDE")
    void nombreDe(@TempDir Path dir) throws IOException {
        Path h = dir.resolve("r.h");
        Files.write(h, ("#define BPVM_AOT_RECETA_UNA \"-Os -a\"\n"
                      + "  #define   BPVM_AOT_RECETA_DOS   \"-Os -b\"   /* comentario */\n"
                      + "#define BPVM_AOT_HOST_RECETA BPVM_AOT_RECETA_UNA\n").getBytes(StandardCharsets.UTF_8));
        AotRecetas r = AotRecetas.leer(h);
        assertEquals(2, r.nombres().size(), "las de HOST no son recetas: " + r.nombres());
        assertEquals("DOS", r.nombreDe(AotRecetas.crc("-Os -b")));
        assertNull(r.nombreDe(12345));
        assertEquals("-Os -a", r.exigir("UNA"));
        assertThrows(IOException.class, () -> r.exigir("TRES"));
    }
}
