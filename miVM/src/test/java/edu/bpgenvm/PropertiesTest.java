package edu.bpgenvm;

import edu.bpgenvm.generador.ModWriter;
import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

/** Cubre MainProperties.java: properties de módulo, instancia y estática + check de privacidad. */
class PropertiesTest {

    @Test
    void modulInstanciaYEstatica() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainProperties.main(new String[0]));
        assertEquals(
            Arrays.asList(
                7,    // module.contador (escrito por main vía setContador)
                3,    // p1.x
                4,    // p1.y
                10,   // p2.x
                2,    // Punto.instancias (estática) tras crear p1 y p2
                0     // p1.revelar() → secreto, sin inicializar = 0
            ),
            r.prints,
            r.rawOutput
        );
    }

    @Test
    void accesoAPropiedadPrivadaFueraDeLaClaseLanzaEnCompileTime() {
        // Replica el escenario "bad" de MainProperties: tocar `secreto` (privada)
        // desde un main() que no está dentro de Punto. Debe atajar en ModWriter.
        assertThrows(RuntimeException.class, () -> {
            ModWriter w = new ModWriter();
            w.addModulo("BadDemo");
            w.addClass("Punto", null);
            w.declareInstanceProperty("secreto", false, false); // private
            w.endClass();
            w.addFunction("main", true);
            w.declareLocal("p");
            w.emitNewObject("Punto"); w.emitSetLocal("p");
            w.emitGetLocal("p");
            w.emitGetInstanceProperty("Punto", "secreto"); // <- debe lanzar aquí
        });
    }
}
