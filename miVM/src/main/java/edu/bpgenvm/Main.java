/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Entrada CLI de la VM.
 *
 *   bpgenvm <fichero.mod>            ejecuta el módulo (resuelve dependencias
 *                                    buscando ModuleX.mod en el cwd).
 *   bpgenvm -d <fichero.mod>         desensambla y vuelca el bytecode a stdout.
 *   bpgenvm --disasm <fichero.mod>   alias largo de -d.
 *   bpgenvm -h | --help              ayuda.
 *
 * El nombre de módulo se deriva del basename del fichero quitando la
 * extensión ".mod" (p.ej. "C:\demos\GCDemo.mod" → "GCDemo"). Esto coincide
 * con la convención que usan todos los Main* del proyecto al generar
 * los .mod (writeToFile("Foo.mod") + executeRootModule("Foo.mod","Foo")).
 *
 * Para ejecutar desde Maven:
 *   mvn exec:java -Dexec.mainClass="edu.bpgenvm.Main" -Dexec.args="GCDemo.mod"
 *   mvn exec:java -Dexec.mainClass="edu.bpgenvm.Main" -Dexec.args="--disasm GCDemo.mod"
 *
 * Para ejecutar el jar (tras `mvn package`):
 *   java -jar target/bpgenvm-1.0.jar GCDemo.mod
 *   java -jar target/bpgenvm-1.0.jar -d GCDemo.mod
 *
 * @author eortiz
 */
import edu.bpgenvm.config.BpProject;
import edu.bpgenvm.config.VmConfig;
import edu.bpgenvm.tools.Disasm;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.File;
import java.io.IOException;
import java.nio.file.Paths;

public class Main {

    public static void main(String[] args) throws IOException {
        boolean disasm = false;
        boolean trace = false;
        int workers = -1;       // -1 = default de la VM
        String configPath = null;
        boolean noConfig = false;
        String file = null;

        for (int i = 0; i < args.length; i++) {
            String a = args[i];
            switch (a) {
                case "-h":
                case "--help":
                    printUsage(System.out);
                    return;
                case "-d":
                case "--disasm":
                    disasm = true;
                    break;
                case "-t":
                case "--trace":
                    trace = true;
                    break;
                case "--no-config":
                    noConfig = true;
                    break;
                case "--config":
                    if (i + 1 >= args.length) {
                        System.err.println("--config requiere una ruta");
                        System.exit(2); return;
                    }
                    configPath = args[++i];
                    break;
                default:
                    if (a.startsWith("--config=")) {
                        configPath = a.substring("--config=".length());
                        break;
                    }
                    if (a.startsWith("--workers=")) {
                        try {
                            workers = Integer.parseInt(a.substring("--workers=".length()));
                        } catch (NumberFormatException nfe) {
                            System.err.println("--workers requiere un entero: " + a);
                            System.exit(2);
                            return;
                        }
                        if (workers < 1) {
                            System.err.println("--workers debe ser >= 1");
                            System.exit(2);
                            return;
                        }
                        break;
                    }
                    if (a.startsWith("-")) {
                        System.err.println("Argumento desconocido: " + a);
                        printUsage(System.err);
                        System.exit(2);
                        return;
                    }
                    if (file != null) {
                        System.err.println("Sólo se admite un fichero por invocación: ya tengo '"
                                + file + "', ahora '" + a + "'");
                        System.exit(2);
                        return;
                    }
                    file = a;
            }
        }

        if (file == null) {
            printUsage(System.err);
            System.exit(2);
            return;
        }

        if (!new File(file).isFile()) {
            System.err.println("No existe el fichero: " + file);
            System.exit(1);
            return;
        }

        if (disasm) {
            Disasm.dump(file, System.out);
            return;
        }

        // Detectar tipo de argumento: .bpproject (fichero de proyecto) o .mod.
        BpProject project = null;
        String runMod = file;
        if (file.toLowerCase().endsWith(".bpproject")) {
            try {
                project = BpProject.load(Paths.get(file));
            } catch (IOException ex) {
                System.err.println("error leyendo proyecto: " + ex.getMessage());
                System.exit(2); return;
            }
            runMod = project.mainModPath;
            System.out.println("project: " + project.sourcePath
                    + " (main=" + project.mainModPath
                    + ", modulePaths=" + project.modulePaths + ")");
        }

        // Resolver config:
        //   --no-config  → usar defaults sin buscar BpVM.cfg.
        //   --config X   → cargar X explícitamente (error si no existe).
        //   en otro caso → buscar BpVM.cfg en cwd y junto al .mod.
        VmConfig cfg;
        if (noConfig) {
            cfg = VmConfig.defaults();
        } else if (configPath != null) {
            try {
                cfg = VmConfig.load(Paths.get(configPath));
            } catch (IOException ex) {
                System.err.println("error leyendo " + configPath + ": " + ex.getMessage());
                System.exit(2); return;
            }
        } else {
            cfg = VmConfig.loadDefaultFor(Paths.get(runMod));
        }
        if (cfg.sourcePath != null) {
            System.out.println("config: " + cfg.sourcePath
                    + " (memorySize=" + cfg.memorySize
                    + ", stackBase=" + cfg.stackBase
                    + (cfg.stdlibDir == null ? "" : ", stdlibDir=" + cfg.stdlibDir)
                    + ")");
        }

        // Ejecutar el módulo
        String moduleName = stripModExtension(new File(runMod).getName());
        VirtualMachine vm = new VirtualMachine(cfg.memorySize, cfg.stackBase);
        vm.setTracing(trace);
        if (workers > 0) vm.setNumWorkers(workers);
        ModuleManager loader = new ModuleManager(vm);
        if (cfg.stdlibDir != null) loader.setStdlibDir(cfg.stdlibDir);
        if (project != null)       loader.setModulePaths(project.modulePaths());
        vm.setModuleManager(loader);
        loader.executeRootModule(runMod, moduleName);
        vm.run();
    }

    private static String stripModExtension(String fname) {
        int dot = fname.lastIndexOf('.');
        if (dot >= 0 && fname.substring(dot).equalsIgnoreCase(".mod")) {
            return fname.substring(0, dot);
        }
        return fname;
    }

    private static void printUsage(java.io.PrintStream out) {
        out.println("Uso:");
        out.println("  bpgenvm <fichero.mod>           ejecuta el módulo");
        out.println("  bpgenvm <fichero.bpproject>     ejecuta el proyecto (resuelve main)");
        out.println("  bpgenvm -d <fichero.mod>        desensambla y vuelca a stdout");
        out.println("  bpgenvm --disasm <fichero.mod>  alias largo de -d");
        out.println("  bpgenvm -t | --trace            trace per-instrucción (default off)");
        out.println("  bpgenvm --workers=N             N workers Java en paralelo (default 2)");
        out.println("  bpgenvm --config <archivo>      configuración JSON (BpVM.cfg)");
        out.println("  bpgenvm --no-config             ignora cualquier BpVM.cfg auto-descubierto");
        out.println("  bpgenvm -h | --help             muestra esta ayuda");
        out.println();
        out.println("BpVM.cfg (JSON, todos los campos opcionales):");
        out.println("  {");
        out.println("    \"memorySize\": 524288,            // bytes totales");
        out.println("    \"stackBase\":  262144,            // dónde empiezan los stacks");
        out.println("    \"stdlibDir\":  \"/ruta/a/stdlib\"   // dir donde buscar .mod stdlib");
        out.println("  }");
        out.println("Si no se pasa --config se busca BpVM.cfg en el cwd y al lado del .mod.");
        out.println();
        out.println(".bpproject (JSON):");
        out.println("  {");
        out.println("    \"main\":        \"out/MyApp.mod\",       // .mod del módulo de arranque");
        out.println("    \"modulePaths\": [\"out\", \"../shared\"]   // dirs donde buscar imports");
        out.println("  }");
        out.println("Resolución de imports: as-given → modulePaths → stdlibDir → cwd.");
    }
}
