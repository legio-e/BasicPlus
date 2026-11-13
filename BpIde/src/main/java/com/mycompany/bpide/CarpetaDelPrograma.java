// ============================================================
// CarpetaDelPrograma.java
// V8 #808 (13-nov) — DÓNDE CORRE UN PROGRAMA CON LA VM LOCAL: en la flash del PC, como en una placa.
//
// La flash del PC es una carpeta, ~/.basicplus/flash, la de las dos VMs por defecto (su --flash-dir): una ruta
// absoluta del programa ("/sys/x") va dentro. El programa corre en <flash>/app/<proyecto> —el nombre que le da la
// placa (FrmMain.deviceAppPrefix)—, o en <flash>/app sin proyecto, y la carpeta NO se borra al acabar: lo que el
// programa guarda con una ruta relativa sigue ahí en el siguiente Run, y se puede mirar. Antes era una carpeta
// temporal que el IDE borraba al acabar.
//
// Con proyecto, lo que sobra del Run anterior se borra, como en la placa, pero sólo lo que sube el IDE (.mod, .bpi y
// .dbg); lo que escribió el programa, nunca (Eduardo, 13-nov: «La b. Si un programa tiene que crear un fichero de
// datos y queremos ver lo que ha creado, si lo borra al terminar nunca veriamos nada»). Sin proyecto no se borra
// nada, como en la placa. Los recursos del proyecto no se copian: quien los necesite, en un pack de aplicación
// (Eduardo, el mismo día).
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Stream;

final class CarpetaDelPrograma {
    private CarpetaDelPrograma() {}

    /** La flash del PC: la que las dos VMs usan por defecto. */
    static Path flashDelPc() {
        return Paths.get(System.getProperty("user.home"), ".basicplus", "flash");
    }

    /** La carpeta del programa en esa flash, con el nombre de la placa: "/app/<proyecto>" es <flash>/app/<proyecto>,
     *  y "/app" (sin proyecto), <flash>/app. */
    static Path de(Path flash, String prefijoPlaca) {
        String p = (prefijoPlaca == null || prefijoPlaca.isEmpty()) ? "/app" : prefijoPlaca;
        while (p.startsWith("/")) p = p.substring(1);
        Path r = flash;
        for (String trozo : p.split("/")) {
            if (!trozo.isEmpty()) r = r.resolve(trozo);
        }
        return r.normalize();
    }

    /** ¿Lo sube el IDE? Es lo único que se borra si sobra. */
    static boolean loSubeElIde(String nombre) {
        String n = nombre.toLowerCase(Locale.ROOT);
        return n.endsWith(".mod") || n.endsWith(".bpi") || n.endsWith(".dbg");
    }

    /** Lo que sobra en la carpeta: de lo que sube el IDE, lo que este Run no ha subido. Sólo en la carpeta misma, que
     *  es adonde sube el IDE; las subcarpetas son del programa. Los nombres, sin mirar mayúsculas: en Windows, subir
     *  "Termostato.mod" encima de un "termostato.mod" deja el nombre viejo, y ése no sobra. */
    static List<Path> huerfanos(Path carpeta, Set<String> subidos) throws IOException {
        Set<String> bajos = new HashSet<>();
        for (String s : subidos) bajos.add(s.toLowerCase(Locale.ROOT));
        List<Path> out = new ArrayList<>();
        if (!Files.isDirectory(carpeta)) return out;
        try (Stream<Path> s = Files.list(carpeta)) {
            s.filter(Files::isRegularFile)
             .filter(p -> loSubeElIde(p.getFileName().toString()))
             .filter(p -> !bajos.contains(p.getFileName().toString().toLowerCase(Locale.ROOT)))
             .sorted()
             .forEach(out::add);
        }
        return out;
    }

    /** Borra lo que sobra (huerfanos) y lo dice. Devuelve cuántos borró. */
    static int borraLosQueSobran(Path carpeta, Set<String> subidos, Consumer<String> publish) {
        int n = 0;
        try {
            for (Path p : huerfanos(carpeta, subidos)) {
                try {
                    Files.delete(p);
                    n++;
                    publish.accept("[ide] sobraba " + p.getFileName() + ": borrado\n");
                } catch (IOException e) {
                    publish.accept("[ide] sobraba " + p.getFileName() + " y no se pudo borrar: " + e.getMessage() + "\n");
                }
            }
        } catch (IOException e) {
            publish.accept("[ide] no se pudo mirar lo que sobra en " + carpeta + ": " + e.getMessage() + "\n");
        }
        return n;
    }
}
