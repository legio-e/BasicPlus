package edu.bpgenvm;

import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.generador.ModWriter;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * El `.mod` v7 y su sección `native` — sobre todo, que empieza ALINEADA A 4.
 *
 * <h3>Por qué este test existe</h3>
 *
 * Las secciones del `.mod` no están alineadas: se empaquetan seguidas, y `data` y
 * `code` empiezan en offsets impares tan tranquilos. Da igual para ellas, porque
 * el cargador las <b>copia</b> a {@code memory[]} en direcciones alineadas — la
 * alineación del fichero no se hereda.
 *
 * <p><b>Con {@code native} no da igual.</b> Un {@code .mod} dentro de un pack se
 * ejecuta por <b>XIP, en su sitio</b>: la posición del blob en el fichero <b>es
 * su dirección de ejecución</b>. Un blob RISC-V en offset impar no arranca.
 *
 * <p>Lo señaló Eduardo el 23-ago-2026 revisando el formato, <b>antes</b> de que
 * hubiera un solo blob emitido. Sin esa observación el fallo habría aparecido
 * mucho más tarde, sólo en placa, sólo con XIP y sólo en una arquitectura — de
 * los caros de encontrar.
 *
 * <p>El test hace falta además porque el relleno es código que <b>hoy no ejecuta
 * nadie</b>: la sección se emite vacía mientras el compilador no meta blobs. Un
 * camino que no se recorre es un camino que no está probado.
 */
class ModV7SeccionNativaTest {

    /** Módulo mínimo que se pueda escribir, con la sección `native` que se le pase. */
    private static Path escribeModulo(Path dir, String nombre, byte[] nativo) throws IOException {
        ModWriter w = new ModWriter();
        w.addModulo(nombre);
        w.setNativeBytes(nativo);
        w.addFunction("main", true);
        Path f = dir.resolve(nombre + ".mod");
        w.writeToFile(f.toString());
        return f;
    }

    /** Los 9 enteros del header v7. */
    private static int[] header(Path f) throws IOException {
        byte[] d = Files.readAllBytes(f);
        ByteBuffer b = ByteBuffer.wrap(d);   // big-endian por defecto
        int[] h = new int[9];
        for (int i = 0; i < 9; i++) h[i] = b.getInt();
        return h;
    }

    /** Offset donde empieza la sección `native` = header + todo lo que va antes. */
    private static int inicioNative(int[] h) {
        //     header  library  imports  exports  interface
        return 36 + h[6] + h[3] + h[4] + h[7];
    }

    @Test
    @DisplayName("un .mod recién escrito es v7 y su header tiene 9 enteros")
    void esV7(@TempDir Path dir) throws IOException {
        int[] h = header(escribeModulo(dir, "SinNativo", null));
        assertEquals(ModFormat.MAGIC_NUMBER_V7, h[0],
                "el compilador debe emitir siempre la versión actual");
        assertEquals(0, h[8], "sin código nativo, la sección va vacía");
    }

    @Test
    @DisplayName("la sección native empieza SIEMPRE en múltiplo de 4")
    void seccionNativaAlineada(@TempDir Path dir) throws IOException {
        // Varios tamaños de blob: lo que decide el relleno no es el tamaño del
        // blob sino dónde CAE la sección, y eso depende de lo que va delante.
        // Se prueban varios para no acertar por casualidad con uno.
        for (int n : new int[]{1, 2, 3, 4, 5, 7, 8, 13}) {
            byte[] blob = new byte[n];
            for (int i = 0; i < n; i++) blob[i] = (byte) (0xA0 + i);

            Path f = escribeModulo(dir, "ConNativo" + n, blob);
            int[] h = header(f);
            int ini = inicioNative(h);
            int pad = h[8] - n;                 // el relleno que metió el writer
            int primerBlob = ini + pad;

            // Lo que promete el formato NO es que la sección empiece alineada
            // —empieza donde caiga— sino que el PRIMER BLOB lo esté. El relleno
            // va dentro de la sección, y `nativeSize` lo incluye.
            assertEquals(0, primerBlob % 4,
                    "el primer blob empieza en " + primerBlob + ", que no es múltiplo de 4."
                    + " Con XIP eso ES su dirección de ejecución: en RISC-V no arranca."
                    + " (blob de " + n + " bytes, sección en " + ini + ", relleno " + pad + ")");
            assertEquals((4 - (ini & 3)) & 3, pad,
                    "el relleno no es el mínimo para alinear: sección en " + ini);

            // Y el fichero tiene que seguir cuadrando: el relleno va DENTRO de
            // nativeSize, así que la suma no puede descuadrar.
            long suma = 36L + h[6] + h[3] + h[4] + h[7] + h[8] + h[1] + h[5];
            assertEquals(Files.size(f), suma,
                    "header + secciones no suma el tamaño del fichero (blob de " + n + ")");

            // El relleno es 0..3: si fuera más, algo cuenta mal.
            assertTrue(h[8] >= n && h[8] <= n + 3,
                    "nativeSize=" + h[8] + " para un blob de " + n + ": el relleno debe ser 0..3");
        }
    }

    /** Los bytes de `data`+`code` de un .mod, para comprobar que no se mueven. */
    private static byte[] colaDe(byte[] mod) {
        ByteBuffer b = ByteBuffer.wrap(mod);
        b.getInt();                                   // magic
        int dataSize = b.getInt(); b.getInt();        // dataSize, mainOffset
        int imp = b.getInt(), exp = b.getInt(), code = b.getInt();
        int lib = b.getInt(), iface = b.getInt(), nat = b.getInt();
        int ini = 36 + lib + imp + exp + iface + nat;
        byte[] cola = new byte[dataSize + code];
        System.arraycopy(mod, ini, cola, 0, cola.length);
        return cola;
    }

    @Test
    @DisplayName("fundir un blob: el módulo sube a v7 y data/code no se mueven")
    void fundirUnBlob(@TempDir Path dir) throws IOException {
        byte[] original = Files.readAllBytes(escribeModulo(dir, "Fundir", null));
        byte[] colaAntes = colaDe(original);

        byte[] blob = new byte[7];
        for (int i = 0; i < 7; i++) blob[i] = (byte) (0x50 + i);
        byte[] fundido = edu.bpgenvm.bytecode.ModFormat.conBloqueNativo(original, blob);

        ByteBuffer b = ByteBuffer.wrap(fundido);
        assertEquals(ModFormat.MAGIC_NUMBER_V7, b.getInt(), "debe salir v7");
        // data y code son lo que EJECUTA la VM: si se mueven o se cortan, el
        // módulo queda roto de una forma que no se ve hasta ejecutarlo.
        org.junit.jupiter.api.Assertions.assertArrayEquals(colaAntes, colaDe(fundido),
                "data+code cambiaron al fundir el bloque nativo");
    }

    @Test
    @DisplayName("DOS familias: los dos blobs caben, y los dos quedan alineados")
    void dosFamilias(@TempDir Path dir) throws IOException {
        byte[] mod = Files.readAllBytes(escribeModulo(dir, "DosFam", null));

        byte[] arm   = {(byte) 0xA1, (byte) 0xA2, (byte) 0xA3};        // 3 bytes
        byte[] riscv = {(byte) 0xB1, (byte) 0xB2, (byte) 0xB3, (byte) 0xB4, (byte) 0xB5};

        byte[] uno = ModFormat.conBloqueNativo(mod, arm);
        byte[] dos = ModFormat.conBloqueNativo(uno, riscv);   // acumulativa

        int[] h = new int[9];
        ByteBuffer b = ByteBuffer.wrap(dos);
        for (int i = 0; i < 9; i++) h[i] = b.getInt();
        int iniSeccion = 36 + h[6] + h[3] + h[4] + h[7];

        // Los dos blobs tienen que estar, y cada uno en múltiplo de 4. Se buscan
        // por su primer byte, que es distinto a propósito.
        int pos1 = -1, pos2 = -1;
        for (int i = iniSeccion; i < iniSeccion + h[8]; i++) {
            if (dos[i] == (byte) 0xA1 && pos1 < 0) pos1 = i;
            if (dos[i] == (byte) 0xB1 && pos2 < 0) pos2 = i;
        }
        assertTrue(pos1 >= 0 && pos2 >= 0,
                "falta alguno de los dos blobs en la sección (multifamilia roto)");
        assertEquals(0, pos1 % 4, "el blob ARM empieza en " + pos1 + ", sin alinear");
        assertEquals(0, pos2 % 4, "el blob RISC-V empieza en " + pos2 + ", sin alinear");
        assertTrue(pos2 > pos1, "el segundo blob debe ir DESPUÉS del primero");
    }

    @Test
    @DisplayName("el blob se recupera intacto tras el relleno")
    void elBlobSobreviveAlRelleno(@TempDir Path dir) throws IOException {
        byte[] blob = {(byte) 0xDE, (byte) 0xAD, (byte) 0xBE, (byte) 0xEF, 0x11};
        Path f = escribeModulo(dir, "Intacto", blob);
        int[] h = header(f);
        byte[] d = Files.readAllBytes(f);

        int ini = inicioNative(h);
        int pad = h[8] - blob.length;                 // lo que se metió delante
        for (int i = 0; i < blob.length; i++) {
            assertEquals(blob[i], d[ini + pad + i],
                    "el byte " + i + " del blob no sobrevivió al relleno");
        }
    }
}
