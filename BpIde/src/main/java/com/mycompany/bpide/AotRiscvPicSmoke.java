// ============================================================
// AotRiscvPicSmoke.java — #440: el .mdn de RISC-V tiene que ser cargable en
// CUALQUIER dirección.
//
// El fallo que motiva esto: en el ESP32-P4 se colgaba toda `native` que tocara
// un literal de cadena. `NatMin` (sumar enteros) iba bien; `NatEsc` moría en el
// escalón 2, el primero que devuelve un literal. La causa estaba en el
// direccionamiento: sin `-mcmodel=medany`, gcc llega a sus datos con `lui`+`addi`
// metiendo la dirección de ENLACE como constante...
//
//     lui  a1,0x0  /  add a1,a1,648      ← a1 = 648, absoluto
//
// ...y como el `.mdn` se carga donde caiga, en la placa eso es un puntero
// salvaje: `string_from_cstr` se pone a buscar un `\0` por ahí. No revienta, se
// CUELGA — que es lo que se veía. ARM nunca lo sufrió porque va con `-fpic` y
// remata con `add r1, pc`.
//
// Qué prueba esto, y por qué en las DOS direcciones: la guarda de `AotBuild`
// cuenta relocalizaciones absolutas en el `.text` del `.o`. Una guarda que sólo
// se comprueba en verde no dice nada — podría estar contando siempre cero. Así
// que se compila el MISMO `.c` con y sin el flag y se exige que salgan distintos:
//
//     con  -mcmodel=medany  →  0 absolutas   (PCREL_HI20/LO12_I)
//     sin  -mcmodel=medany  →  >0 absolutas  (HI20/LO12_I)
//
// Se mira el `.o` y no el `.elf` a propósito: al enlazar, las relocalizaciones
// se consumen y las dos variantes quedan como bytes igual de plausibles.
//
//   mvn -f BpIde/pom.xml exec:java \
//       -Dexec.mainClass=com.mycompany.bpide.AotRiscvPicSmoke
//
// Si no está el toolchain de RISC-V (ESP-IDF), se salta sin fallar.
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class AotRiscvPicSmoke {

    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    /** El caso mínimo que reproduce: una `native` que devuelve un literal. */
    private static final String FUENTE =
            "module PicLit\n"
          + "\n"
          + "  native function saluda(): string\n"
          + "    return \"hola\"\n"
          + "  end saluda\n"
          + "\n"
          + "  function Main()\n"
          + "    print saluda()\n"
          + "  end Main\n"
          + "\n"
          + "end PicLit\n";

    public static void main(String[] args) throws Exception {
        System.out.println("--- #440: el .mdn de RISC-V, cargable en cualquier direccion ---");

        String gcc = AotBuild.resolveRiscvGcc(null);
        if (!hayToolchain(gcc)) {
            System.out.println("  (saltado: no se encuentra " + gcc + ")");
            return;
        }

        Path work = Files.createTempDirectory("bp440");
        Path bp   = work.resolve("PicLit.bp");
        Files.write(bp, FUENTE.getBytes("UTF-8"));

        // 1) El frontend emite el .c del AOT. Es el mismo para las dos familias:
        //    lo que cambia es CÓMO lo compila cada gcc.
        basicplus.frontend.AotMain.main(new String[]{
                bp.toString(), "--mdn", work.toString() });
        Path c = work.resolve("aot_PicLit.c");
        check(Files.exists(c), "el frontend emite " + c.getFileName());
        if (!Files.exists(c)) { System.exit(1); }

        // 2) Las dos variantes del MISMO .c.
        Path bueno = compilar(gcc, c, work.resolve("bueno.o"), true);
        Path roto  = compilar(gcc, c, work.resolve("roto.o"),  false);

        int absBueno = AotBuild.relocsAbsolutasRiscv(bueno);
        int absRoto  = AotBuild.relocsAbsolutasRiscv(roto);

        check(absBueno == 0,
              "con -mcmodel=medany: " + absBueno + " relocalizaciones absolutas (0 esperadas)");
        // EL CONTROL: sin el flag TIENEN que aparecer. Si no, la guarda no mide
        // nada y el verde de arriba es un verde falso.
        check(absRoto > 0,
              "sin  -mcmodel=medany: " + absRoto + " relocalizaciones absolutas (>0 esperadas)");

        System.out.println(fallos == 0 ? "TODO OK" : fallos + " FALLO(S)");
        System.exit(fallos == 0 ? 0 : 1);
    }

    /** Compila el `.c` con los flags reales de la familia, quitando o no el
     *  `-mcmodel=medany`. Todo lo demás se deja igual a propósito: la única
     *  variable del experimento es ese flag. */
    private static Path compilar(String gcc, Path c, Path o, boolean medany)
            throws IOException, InterruptedException {
        String bpgenvm = Paths.get("").toAbsolutePath().resolve("bpgenvm-c").toString();
        if (!Files.isDirectory(Paths.get(bpgenvm))) {
            bpgenvm = Paths.get("..").toAbsolutePath().normalize()
                           .resolve("bpgenvm-c").toString();
        }
        List<String> cmd = new ArrayList<>();
        cmd.add(gcc);
        for (String f : AotBuild.flagsRiscvParaPruebas()) {
            if (!medany && "-mcmodel=medany".equals(f)) continue;
            cmd.add(f);
        }
        cmd.add("-I" + Paths.get(bpgenvm, "include"));
        cmd.add("-I" + Paths.get(bpgenvm, "src"));
        cmd.add("-c"); cmd.add(c.toString());
        cmd.add("-o"); cmd.add(o.toString());

        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true);
        pb.redirectOutput(ProcessBuilder.Redirect.INHERIT);
        int rc = pb.start().waitFor();
        if (rc != 0 || !Files.exists(o)) {
            throw new IOException("gcc devolvio " + rc + ": " + String.join(" ", cmd));
        }
        return o;
    }

    private static boolean hayToolchain(String gcc) {
        if (gcc == null) return false;
        if (Files.exists(Paths.get(gcc))) return true;
        try {   /* puede venir pelado y resolverse por PATH */
            Process p = new ProcessBuilder(Arrays.asList(gcc, "--version"))
                    .redirectErrorStream(true).start();
            p.getInputStream().close();
            return p.waitFor() == 0;
        } catch (Exception e) {
            return false;
        }
    }
}
