package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H8 (28-sep) — el guardián de la miVM EMBEBIDA ({@link Version#compararVm}): avisa si la miVM construida al lado
 * es más nueva que el jar que la lleva dentro; se calla si es más vieja, si no hay miVM al lado o si el compilador
 * no va en un jar.
 */
class VersionVmTest {

    private static File fichero(Path dir, String nombre, long fecha) throws Exception {
        File f = Files.write(dir.resolve(nombre), new byte[] {1}).toFile();
        assertTrue(f.setLastModified(fecha), "no se pudo fijar la fecha de " + nombre);
        return f;
    }

    @Test
    @DisplayName("la miVM del árbol, más NUEVA que el jar que la embebe: aviso, con las dos fechas")
    void vmMasNueva(@TempDir Path tmp) throws Exception {
        File jar = fichero(tmp, "BpIde-6.0-shaded.jar", 1_000_000_000_000L);
        File vm  = fichero(tmp, "bpgenvm-1.0.jar",      1_000_000_060_000L);   // un minuto después
        String aviso = Version.compararVm(jar, vm);
        assertNotNull(aviso, "tenía que avisar");
        assertTrue(aviso.contains("BpIde-6.0-shaded.jar") && aviso.contains("bpgenvm-1.0.jar"), aviso);
    }

    @Test
    @DisplayName("callado: la miVM más vieja, sin miVM al lado, o el compilador en clases sueltas")
    void callado(@TempDir Path tmp) throws Exception {
        File jar = fichero(tmp, "basicplus-frontend.jar", 1_000_000_060_000L);
        File vm  = fichero(tmp, "bpgenvm-1.0.jar",        1_000_000_000_000L);
        assertNull(Version.compararVm(jar, vm), "la miVM es más vieja: nada que decir");
        assertNull(Version.compararVm(jar, null), "sin miVM al lado: nada");
        File clase = fichero(tmp, "Version.class", 1_000_000_000_000L);
        File vmNueva = fichero(tmp, "otra-bpgenvm.jar", 1_000_000_600_000L);
        assertNull(Version.compararVm(clase, vmNueva), "clases sueltas: la miVM no va dentro");
    }
}
