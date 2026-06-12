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
import edu.bpgenvm.vm.debug.DebugController;
import edu.bpgenvm.vm.debug.DebugServer;
import java.io.File;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.concurrent.TimeUnit;

public class Main {

    public static void main(String[] args) throws IOException {
        // H2 (V2): forzar UTF-8 en stdout/stderr, independiente del code page
        // de consola (Windows usa cp1252/cp850 por defecto). Así los strings
        // Unicode salen correctos y byte-idénticos a la VM-C. La ruta IDE/wire
        // inyecta su propio sink (JSON ya es UTF-8), así que no le afecta.
        System.setOut(new java.io.PrintStream(
            new java.io.FileOutputStream(java.io.FileDescriptor.out), true, "UTF-8"));
        System.setErr(new java.io.PrintStream(
            new java.io.FileOutputStream(java.io.FileDescriptor.err), true, "UTF-8"));
        boolean disasm = false;
        boolean trace = false;
        int workers = -1;       // -1 = default de la VM
        String configPath = null;
        boolean noConfig = false;
        int listenPort = -1;    // -1 = no servidor de debug
        boolean waitClient = false;  // bloquear la VM hasta que conecte el IDE
        String workdir = null;       // sandbox del filesystem; null = sin sandbox
        String cliStdlibDir = null;  // override de cfg.stdlibDir si != null
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
                    if (a.startsWith("--listen=")) {
                        try {
                            listenPort = Integer.parseInt(a.substring("--listen=".length()));
                        } catch (NumberFormatException nfe) {
                            System.err.println("--listen requiere un puerto: " + a);
                            System.exit(2); return;
                        }
                        if (listenPort < 1 || listenPort > 65535) {
                            System.err.println("--listen puerto fuera de rango: " + listenPort);
                            System.exit(2); return;
                        }
                        break;
                    }
                    if ("--listen".equals(a)) {
                        if (i + 1 >= args.length) {
                            System.err.println("--listen requiere un puerto");
                            System.exit(2); return;
                        }
                        try {
                            listenPort = Integer.parseInt(args[++i]);
                        } catch (NumberFormatException nfe) {
                            System.err.println("--listen requiere un puerto entero");
                            System.exit(2); return;
                        }
                        break;
                    }
                    if ("--wait-client".equals(a)) {
                        waitClient = true;
                        break;
                    }
                    if (a.startsWith("--workdir=")) {
                        workdir = a.substring("--workdir=".length());
                        break;
                    }
                    if ("--workdir".equals(a)) {
                        if (i + 1 >= args.length) {
                            System.err.println("--workdir requiere una ruta");
                            System.exit(2); return;
                        }
                        workdir = args[++i];
                        break;
                    }
                    if (a.startsWith("--stdlibDir=")) {
                        cliStdlibDir = a.substring("--stdlibDir=".length());
                        break;
                    }
                    if ("--stdlibDir".equals(a)) {
                        if (i + 1 >= args.length) {
                            System.err.println("--stdlibDir requiere una ruta");
                            System.exit(2); return;
                        }
                        cliStdlibDir = args[++i];
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

        // A2.3 — modo daemon: si no se pasa fichero pero sí --listen, la
        // VM arranca a la espera de un runModule command. Útil para el flujo
        // IDE→VM separados: el IDE conecta, sube ficheros, y manda runModule.
        boolean daemonMode = (file == null);
        if (daemonMode && listenPort <= 0) {
            printUsage(System.err);
            System.exit(2);
            return;
        }
        if (!daemonMode && !new File(file).isFile()) {
            System.err.println("No existe el fichero: " + file);
            System.exit(1);
            return;
        }

        if (disasm) {
            if (daemonMode) {
                System.err.println("--disasm requiere un fichero");
                System.exit(2); return;
            }
            Disasm.dump(file, System.out);
            return;
        }

        // Detectar tipo de argumento: .bpproject (fichero de proyecto) o .mod.
        BpProject project = null;
        String runMod = file;   // null en modo daemon — se resuelve tras runModule.
        if (!daemonMode && file.toLowerCase().endsWith(".bpproject")) {
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
        } else if (runMod != null) {
            cfg = VmConfig.loadDefaultFor(Paths.get(runMod));
        } else {
            // Daemon mode sin .mod conocido aún: autodiscover en cwd / workdir.
            java.nio.file.Path probe = (workdir != null) ? Paths.get(workdir) : null;
            cfg = VmConfig.loadDefaultFor(probe);
        }
        // --stdlibDir CLI gana sobre cfg.stdlibDir. Útil cuando el IDE
        // arranca la VM como subproceso desde un cwd sin BpVM.cfg: el IDE
        // resuelve su propio cfg y propaga el dir explícitamente.
        if (cliStdlibDir != null && !cliStdlibDir.isEmpty()) {
            cfg.stdlibDir = cliStdlibDir;
        }
        if (cfg.sourcePath != null) {
            System.out.println("config: " + cfg.sourcePath
                    + " (memorySize=" + cfg.memorySize
                    + ", stackBase=" + cfg.stackBase
                    + (cfg.stdlibDir == null ? "" : ", stdlibDir=" + cfg.stdlibDir)
                    + ")");
        } else if (cfg.stdlibDir != null) {
            // No hay BpVM.cfg pero sí stdlibDir via CLI — log para depurar.
            System.out.println("config: defaults + --stdlibDir=" + cfg.stdlibDir);
        }

        // Crear la VM (en daemon mode el moduleName se calcula tras runModule).
        VirtualMachine vm = new VirtualMachine(cfg.memorySize, cfg.stackBase);
        vm.setTracing(trace);
        if (workers > 0) vm.setNumWorkers(workers);
        ModuleManager loader = new ModuleManager(vm);
        if (cfg.stdlibDir != null) loader.setStdlibDir(cfg.stdlibDir);
        if (project != null)       loader.setModulePaths(project.modulePaths());
        // A2.1 — workdir como sandbox. Si se pasa --workdir, todas las
        // operaciones de fichero (IO builtins) se confinan ahí.
        if (workdir != null) {
            java.nio.file.Path wd = java.nio.file.Paths.get(workdir).toAbsolutePath().normalize();
            if (!java.nio.file.Files.isDirectory(wd)) {
                System.err.println("--workdir no es un directorio: " + wd);
                System.exit(2); return;
            }
            loader.setWorkdir(wd);
            System.err.println("[main] workdir = " + wd);
        }
        vm.setModuleManager(loader);

        // A1.4.b: si se pidió --listen, levantar el servidor de debug
        // ANTES de cargar el módulo. Con --wait-client la VM bloquea hasta
        // que conecte un cliente (el IDE); sin él, ejecuta de inmediato y
        // el cliente puede conectarse en cualquier momento (modo "fire
        // and forget"). El servidor también instala el sink + listener de
        // debug en el momento del accept.
        DebugServer dbgServer = null;
        DebugController controller = null;
        if (listenPort > 0) {
            controller = new DebugController();
            vm.setDebugHook(controller.hook());
            // A1.7: cablear el listener no-pause para que ExceptionEvent
            // generado en WorkerLoop salga al cliente del debugger.
            vm.setDebugEventListener(controller::emitEvent);
            dbgServer = new DebugServer(vm, controller);
            dbgServer.start(listenPort);
            if (waitClient) {
                System.err.println("[main] esperando cliente debug en puerto " + listenPort + "...");
                try {
                    if (!dbgServer.awaitClient(30, TimeUnit.SECONDS)) {
                        System.err.println("[main] timeout esperando cliente, sigo en modo headless");
                    }
                } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                }
            }
        }

        // En modo daemon, esperar a que el cliente mande runModule. El
        // path llega RELATIVO al workdir (el cliente acaba de subir los
        // ficheros vía uploadFile y ahora pide ejecutar uno).
        if (daemonMode) {
            System.err.println("[main] modo daemon — esperando runModule...");
            try {
                String mod = dbgServer.awaitRunModule(60 * 60 * 1000L); // 1 hora
                if (mod == null || mod.isEmpty()) {
                    System.err.println("[main] sin runModule, abortando");
                    if (dbgServer != null) dbgServer.close();
                    System.exit(2); return;
                }
                if (workdir != null) {
                    runMod = Paths.get(workdir).resolve(mod).toString();
                } else {
                    runMod = mod;
                }
                System.err.println("[main] runModule: " + runMod);
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                if (dbgServer != null) dbgServer.close();
                System.exit(2); return;
            }
        }
        String moduleName = stripModExtension(new File(runMod).getName());

        int exitCode = 0;
        String exitReason = "main returned";
        try {
            loader.executeRootModule(runMod, moduleName);
            vm.run();
            if (vm.isKillRequested()) {        // P-run-stop (#257)
                exitCode = 130;                // convención 128+SIGINT
                exitReason = "killed";
            }
        } catch (Throwable t) {
            exitCode = 1;
            exitReason = t.getClass().getSimpleName() + ": " + t.getMessage();
            throw t;
        } finally {
            if (controller != null) {
                // A1.7: notificar al cliente debug que la ejecución terminó.
                // Útil para que el IDE cierre la sesión sin necesidad de
                // poll en el process.exited.
                controller.emitEvent(new edu.bpgenvm.vm.debug.ExitedEvent(exitCode, exitReason));
            }
            if (dbgServer != null) {
                // Pequeña pausa antes de cerrar el socket para que el
                // ExitedEvent termine de escribirse al wire.
                try { Thread.sleep(50); } catch (InterruptedException ignored) {}
                dbgServer.close();
            }
        }
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
        out.println("  bpgenvm --workers=N             N workers SMP en paralelo (default 1; >1 experimental: race B1)");
        out.println("  bpgenvm --config <archivo>      configuración JSON (BpVM.cfg)");
        out.println("  bpgenvm --no-config             ignora cualquier BpVM.cfg auto-descubierto");
        out.println("  bpgenvm --listen <puerto>       arranca servidor de debug TCP+JSON");
        out.println("  bpgenvm --wait-client           con --listen, bloquea hasta que conecte el IDE");
        out.println("  bpgenvm --workdir <dir>         sandbox: la VM sólo ve ficheros bajo este dir");
        out.println("  bpgenvm --stdlibDir <dir>       override de cfg.stdlibDir (gana sobre BpVM.cfg)");
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
