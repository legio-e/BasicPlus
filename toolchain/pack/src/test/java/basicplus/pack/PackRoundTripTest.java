package basicplus.pack;

import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * "Verificar por construcción": build → read reproduce las entradas byte a byte,
 * y build es determinista. Es el auto-test principal del núcleo (§7.7).
 */
class PackRoundTripTest {

    private static PackEntry e(String tipo, String nombre, String data) {
        return new PackEntry(tipo, nombre, data.getBytes(StandardCharsets.UTF_8));
    }

    /** Muestra con manifest (mft), dos módulos y un recurso "binario". */
    private static List<PackEntry> sample() {
        List<PackEntry> l = new ArrayList<>();
        l.add(e("mft", "manifest", "main=App\n"));
        l.add(e("mod", "App",      "<<bytecode de App>>"));
        l.add(e("mod", "Helper",   "<<bytecode de Helper, un poco mas largo>>"));
        l.add(new PackEntry("img", "logo", new byte[]{0, 1, 2, (byte) 0xFF, 'x', 'y'}));
        return l;
    }

    @Test void crc16CcittFalseValorDeComprobacion() {
        assertEquals(0x29B1, Crc16.compute("123456789".getBytes(StandardCharsets.US_ASCII)));
    }

    @Test void roundTripPreservaEntradasYCabecera() throws Exception {
        byte[] img = PackWriter.build("MiPack", "1.0.0", 1_700_000_000L, sample(), 4096);
        assertEquals(0, img.length % 4096, "el tamaño debe ser múltiplo del bloque");

        PackReader.Pack p = PackReader.read(img);
        assertEquals("MiPack", p.nombre);
        assertEquals("1.0.0", p.versionContenido);
        assertEquals(1_700_000_000L, p.fechaUnix);
        assertTrue(p.active);

        List<PackEntry> orig = sample();
        assertEquals(orig.size(), p.entries.size());
        for (int i = 0; i < orig.size(); i++) {
            assertEquals(orig.get(i).tipo,   p.entries.get(i).tipo,   "tipo[" + i + "]");
            assertEquals(orig.get(i).nombre, p.entries.get(i).nombre, "nombre[" + i + "]");
            assertArrayEquals(orig.get(i).data, p.entries.get(i).data, "data[" + i + "]");
        }
    }

    @Test void determinismoMismoInputMismosBytes() throws Exception {
        byte[] a = PackWriter.build("MiPack", "1.0.0", 1_700_000_000L, sample(), 4096);
        byte[] b = PackWriter.build("MiPack", "1.0.0", 1_700_000_000L, sample(), 4096);
        assertArrayEquals(a, b, "mismos inputs → bytes idénticos (invariante byte-idéntico)");
    }

    @Test void slackFinalEsFF() throws Exception {
        byte[] img = PackWriter.build("P", "", 0L, sample(), 4096);
        assertEquals((byte) 0xFF, img[img.length - 1], "el slack final debe quedar borrado (0xFF)");
    }

    @Test void packVacioEsValido() throws Exception {
        byte[] img = PackWriter.build("Vacio", "", 0L, new ArrayList<>(), 4096);
        PackReader.Pack p = PackReader.read(img);
        assertEquals(0, p.entries.size());
        assertEquals("Vacio", p.nombre);
    }

    @Test void crcCabDetectaCorrupcionDeCabecera() throws Exception {
        byte[] img = PackWriter.build("P", "1.0", 1L, sample(), 4096);
        img[PackFormat.OFF_NOMBRE] ^= 0x40;   // corrompe el nombre del pack
        PackException ex = assertThrows(PackException.class, () -> PackReader.read(img));
        assertTrue(ex.getMessage().contains("crc_cab"), ex.getMessage());
    }

    @Test void crcContenidoDetectaCorrupcionDeDatos() throws Exception {
        byte[] img = PackWriter.build("P", "1.0", 1L, sample(), 4096);
        img[PackFormat.HEADER_SIZE + PackFormat.ENTRY_HEADER_SIZE] ^= 0x01;   // 1er byte de datos
        PackException ex = assertThrows(PackException.class, () -> PackReader.read(img));
        assertTrue(ex.getMessage().contains("crc_contenido"), ex.getMessage());
    }

    @Test void tombstoneNoInvalidaCrcCab() throws Exception {
        // Limpiar ALIVE_BIT de flags (tombstone 1->0) NO debe romper crc_cab,
        // porque crc_cab se calcula SALTÁNDOSE el campo flags.
        byte[] img = PackWriter.build("P", "1.0", 1L, sample(), 4096);
        int flags = PackFormat.getU16(img, PackFormat.OFF_FLAGS);
        PackFormat.putU16(img, PackFormat.OFF_FLAGS, flags & ~PackFormat.ALIVE_BIT);
        PackReader.Pack p = PackReader.read(img);   // no lanza
        assertFalse(p.active, "un pack con ALIVE_BIT a 0 debe verse como tombstoned");
    }

    @Test void nombreDemasiadoLargoEsError() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 40; i++) sb.append('x');   // 40 B > 32
        List<PackEntry> l = new ArrayList<>();
        l.add(new PackEntry("mod", sb.toString(), new byte[]{1, 2, 3}));
        assertThrows(PackException.class, () -> PackWriter.build("P", "", 0L, l, 4096));
    }

    @Test void tipoNoMinusculaEsError() {
        List<PackEntry> l = new ArrayList<>();
        l.add(new PackEntry("MOD", "App", new byte[]{1}));   // mayúsculas
        assertThrows(PackException.class, () -> PackWriter.build("P", "", 0L, l, 4096));
    }

    @Test void magicInvalidoEsError() {
        byte[] junk = new byte[256];
        Arrays.fill(junk, (byte) 0xAB);
        PackException ex = assertThrows(PackException.class, () -> PackReader.read(junk));
        assertTrue(ex.getMessage().contains("magic"), ex.getMessage());
    }
}
