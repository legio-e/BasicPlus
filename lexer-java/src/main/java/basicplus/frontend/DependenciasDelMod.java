package basicplus.frontend;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * V7/H8 #538 (28-sep) — las dependencias de un programa, para SUBIRLAS a una placa: las que CARGA la VM, leídas de
 * los {@code .mod} compilados —los dueños de sus imports ({@link PackStep#duenosDeImports}), transitivos— y no de
 * las líneas {@code import} del fuente.
 *
 * <p>El IDE las sacaba con una regex sobre los {@code .bp} ({@code FrmMain.parseImports}) y se le escapaban el
 * {@code import Core} implícito (lo pone el parser: un programa que no lo escribe no listaba {@code Core}), un módulo
 * de biblioteca ({@code import com.acme.lib.LibMarcaBase} capturaba {@code com}) y la dependencia de la que sólo hay
 * {@code .mod}, que cortaba la cadena. En la placa no mordía (la imagen repone {@code /lib}); con el
 * {@code Stdlib.pack} cargado {@code /lib} está vacío, y en el micro simulado un pack rancio hacía morir al programa
 * al enlazar. Lo que se lee aquí es lo que lee la VM al cargar (las entradas de import del {@code .mod}), así que no
 * puede faltar nada que ella vaya a pedir. Qué se sube de verdad lo decide después la placa ({@code STAT}, #466).
 */
public final class DependenciasDelMod {

    private DependenciasDelMod() {}

    /**
     * Los {@code .mod} de los que depende {@code principal}, transitivos y en orden de descubrimiento, sin el propio
     * principal. Cada uno se busca en {@code dondeBuscar}, en ese orden; uno que vive en {@code stdlibDir} (allí está
     * su {@code .bp} o su {@code .mod}) se busca allí PRIMERO, para no subir un {@code X.mod} rancio de otro sitio (el
     * {@code out/} de un proyecto viejo: la regla que el IDE aprendió en junio). Uno que no está en ningún sitio no se
     * lista: la placa lo tendrá (embebido, en {@code /lib} o en un pack) y, si no, la VM lo dice al cargar.
     */
    public static List<Path> paraSubir(Path principal, List<Path> dondeBuscar, Path stdlibDir) throws IOException {
        Path yo = principal.toAbsolutePath().normalize();
        List<Path> resultado = new ArrayList<>();
        Set<String> vistos = new LinkedHashSet<>();
        ArrayDeque<String> cola = new ArrayDeque<>(PackStep.duenosDeImports(Files.readAllBytes(principal)));
        while (!cola.isEmpty()) {
            String nombre = cola.poll();
            if (!vistos.add(nombre)) continue;                     // ya visto: y así no hay ciclos
            Path mod = localizar(nombre, dondeBuscar, stdlibDir);
            if (mod == null || mod.toAbsolutePath().normalize().equals(yo)) continue;
            resultado.add(mod);
            cola.addAll(PackStep.duenosDeImports(Files.readAllBytes(mod)));
        }
        return resultado;
    }

    private static Path localizar(String nombre, List<Path> dondeBuscar, Path stdlibDir) {
        List<Path> orden = new ArrayList<>();
        if (stdlibDir != null && (Files.isRegularFile(stdlibDir.resolve(nombre + ".bp"))
                || Files.isRegularFile(stdlibDir.resolve(nombre + ".mod")))) {
            orden.add(stdlibDir);
        }
        orden.addAll(dondeBuscar);
        for (Path d : orden) {
            if (d == null) continue;
            Path c = d.resolve(nombre + ".mod");
            if (Files.isRegularFile(c)) return c;
        }
        return null;
    }
}
