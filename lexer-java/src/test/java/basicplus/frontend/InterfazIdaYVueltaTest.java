package basicplus.frontend;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V7/H2 — la INTERFAZ embebida en el `.mod` hace la ida y vuelta byte a byte:
 * bytes del `.mod` → {@code ModuleInterface.fromBytes} → {@code toBytes} → los mismos bytes.
 *
 * <p>Es la línea base ANTES de `bpi 8` (`field`, `final`, estáticos, `ctor`×N): cualquier
 * directiva que se añada tiene que sobrevivir a leer-y-escribir, y cualquier cosa que el
 * lector tolere pero el escritor no reproduzca (o al revés) sale aquí. Se prueba sobre
 * la stdlib entera compilada (28 módulos, con clases, sobrecargas, properties, eventos,
 * constantes estáticas y layouts), no sobre un módulo de juguete.
 */
class InterfazIdaYVueltaTest {

    static Stream<Path> modulos() {
        File std = new File("../bpstdlib");
        File[] mods = std.listFiles((d, n) -> n.endsWith(".mod"));
        Assumptions.assumeTrue(mods != null && mods.length > 0, "hace falta ../bpstdlib/*.mod (compila la stdlib)");
        return Stream.of(mods).map(File::toPath).sorted();
    }

    /** La sección interface tal como la saca el compilador de un .mod (método privado de Main:
     *  se llama por reflexión para no abrir la API sólo por el test). */
    private static byte[] seccionInterface(Path mod) throws Exception {
        Method m = Main.class.getDeclaredMethod("extractInterfaceSection", Path.class);
        m.setAccessible(true);
        return (byte[]) m.invoke(null, mod);
    }

    @ParameterizedTest(name = "{0}")
    @MethodSource("modulos")
    @DisplayName("leer la interfaz de un .mod y volver a escribirla da los mismos bytes")
    void idaYVuelta(Path mod) throws Exception {
        byte[] original = seccionInterface(mod);
        assertTrue(original != null && original.length > 0, mod + ": el .mod lleva sección interface");
        ModuleInterface iface = ModuleInterface.fromBytes(original, mod.toString());
        byte[] reescrita = iface.toBytes();
        String a = new String(original, StandardCharsets.UTF_8);
        String b = new String(reescrita, StandardCharsets.UTF_8);
        assertEquals(a, b, mod.getFileName() + ": la interfaz reescrita difiere de la embebida (¿una directiva que el lector"
                + " acepta y el escritor no reproduce, o al revés?)");
    }
}
