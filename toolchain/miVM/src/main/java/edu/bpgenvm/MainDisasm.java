/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Lanzador del disassembler para usar desde NetBeans (Run File). Localiza
 * todos los *.mod del directorio actual y los vuelca por orden alfabético
 * a stdout vía {@link edu.bpgenvm.tools.Disasm}.
 *
 * Si se pasan args concretos, vuelca solo esos.
 *
 * @author eortiz
 */
import edu.bpgenvm.tools.Disasm;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;

public class MainDisasm {

    public static void main(String[] args) throws IOException {
        String[] targets;
        if (args.length > 0) {
            targets = args;
        } else {
            File dir = new File(".");
            File[] mods = dir.listFiles((d, n) -> n.endsWith(".mod"));
            if (mods == null || mods.length == 0) {
                System.out.println("No hay ficheros .mod en " + dir.getAbsolutePath());
                System.out.println("Ejecuta primero Main / MainOps / MainData / MainArrays para generarlos.");
                return;
            }
            Arrays.sort(mods);
            targets = new String[mods.length];
            for (int i = 0; i < mods.length; i++) targets[i] = mods[i].getPath();
        }

        for (String t : targets) {
            Disasm.dump(t, System.out);
            System.out.println();
        }
    }
}
