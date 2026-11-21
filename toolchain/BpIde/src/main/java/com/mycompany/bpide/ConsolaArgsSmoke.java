// ============================================================
// ConsolaArgsSmoke.java — #437: el troceo de argumentos de la consola.
//
// Los comandos nuevos (`copy <local> [destino]`, `get <remoto> [local]`)
// reciben DOS rutas en una línea, y en Windows las rutas con espacios son la
// norma ("C:\Program Files\..."). Un split por espacios a secas las parte por
// la mitad y el error aparece LEJOS: "no existe C:\Program". Por eso el troceo
// respeta comillas — y por eso se prueba, que es la única parte del comando con
// lógica de verdad (lo demás es llamar al Backend, que ya está probado).
//
//   mvn -f BpIde/pom.xml exec:java \
//       -Dexec.mainClass=com.mycompany.bpide.ConsolaArgsSmoke
// ============================================================
package com.mycompany.bpide;

public final class ConsolaArgsSmoke {

    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    private static void par(String linea, String esperado0, String esperado1) {
        String[] r = PicoExplorer.partirDosParaPruebas(linea);
        check(esperado0.equals(r[0]) && esperado1.equals(r[1]),
              "[" + linea + "]  ->  [" + r[0] + "] [" + r[1] + "]"
              + (esperado0.equals(r[0]) && esperado1.equals(r[1])
                 ? "" : "   ESPERADO [" + esperado0 + "] [" + esperado1 + "]"));
    }

    public static void main(String[] args) {
        System.out.println("--- #437: troceo de argumentos de la consola ---");

        // Lo corriente.
        par("Blink.mod", "Blink.mod", "");
        par("Blink.mod /lib", "Blink.mod", "/lib");
        par("  Blink.mod   /lib  ", "Blink.mod", "/lib");
        par("", "", "");

        // EL CASO que motiva las comillas: rutas de Windows con espacios.
        par("\"C:\\Program Files\\x.mod\"", "C:\\Program Files\\x.mod", "");
        par("\"C:\\Program Files\\x.mod\" /lib", "C:\\Program Files\\x.mod", "/lib");
        par("x.mod \"/mi carpeta/\"", "x.mod", "/mi carpeta/");
        par("\"C:\\a b\\x.mod\" \"/c d/\"", "C:\\a b\\x.mod", "/c d/");

        // Comilla sin cerrar: se toma el resto y NO se pierde el argumento —
        // mejor un destino raro que un fichero cortado en silencio.
        par("\"C:\\sin cerrar", "C:\\sin cerrar", "");

        System.out.println("[status=" + (fallos == 0 ? "OK" : "FAIL") + "]");
        if (fallos != 0) System.exit(1);
    }
}
