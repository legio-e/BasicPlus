/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.bytecode;

/**
 * Constantes del formato de fichero .mod.
 *
 * Formato v6 (H6.a — módulo autodescriptivo):
 * <pre>
 *   [ MAGIC=0x4D4F4436 ("MOD6") ]   4 bytes
 *   [ dataSize                  ]   4 bytes
 *   [ mainOffset                ]   4 bytes  (offset relativo en code, o -1)
 *   [ importsSize               ]   4 bytes
 *   [ exportsSize               ]   4 bytes
 *   [ codeSize                  ]   4 bytes
 *   [ librarySize               ]   4 bytes  (0 si no hay library)
 *   [ interfaceSize             ]   4 bytes  (0 si no hay interfaz; SOLO v6)
 *   --- header v6 (32 bytes) ---
 *   [ library   ] librarySize   bytes  (UTF-8 raw, sin length prefix)
 *   [ imports   ] importsSize   bytes  (metadatos del linker)
 *   [ exports   ] exportsSize   bytes  (metadatos del linker)
 *   [ interface ] interfaceSize bytes  (texto de interfaz = el antiguo .bpi, UTF-8)
 *   [ data      ] dataSize      bytes  (constantes + globals + class descriptors)
 *   [ code      ] codeSize      bytes
 * </pre>
 *
 * v6 vs v5: añade `interfaceSize` al header (8º entero) y una sección
 * `interface` situada ENTRE `exports` y `data` (junto a los demás metadatos,
 * antes del payload de runtime). Lleva la interfaz que antes iba al `.bpi`
 * (mismo texto de directivas): el `.mod` es ahora autodescriptivo y el `.bpi`
 * deja de generarse. Las VMs NO necesitan la interfaz para ejecutar: leen
 * `interfaceSize` del header y SALTAN la sección antes de leer `data`.
 *
 * COMPAT: los loaders aceptan v5 (MAGIC "MOD5", header 28 bytes, sin sección
 * interface) y v6. El escritor emite v6 sólo cuando hay interfaz que embeber;
 * si no, emite v5 (comportamiento previo intacto).
 *
 * Formato v5 (previo):
 * <pre>
 *   [ MAGIC=0x4D4F4435 ("MOD5") ]   4 bytes
 *   ... 6 enteros más (28 bytes de header) ...
 *   [ library ][ imports ][ exports ][ data ][ code ]
 * </pre>
 *
 * Sección IMPORTS (v5+):
 * <pre>
 *   [ count           ] i32           número de imports
 *   { para cada import:
 *       [ name        ] UTF-8 (con writeUTF: u16 length + bytes)  ; "Module.func" o "lib.Module.func"
 *       [ fromPath    ] UTF-8 (con writeUTF: u16 length + bytes)  ; "" = usa convención por defecto
 *   }
 * </pre>
 *
 * El fromPath es un string libre (ruta absoluta o relativa al .mod del importer).
 * Solo lo usa el loader cuando resuelve qué fichero abrir; el linker sigue
 * trabajando con los nombres lógicos.
 *
 * v5 vs v4: cada import lleva ahora un fromPath (string vacío = convención por defecto).
 * v4 vs v3: añade `librarySize` al header y una sección `library` con el
 * nombre de la librería del módulo (UTF-8 raw, longitud dada por librarySize).
 *
 * Convención de nombre de fichero (decidida por el ModWriter):
 *   - Sin library     ⇒ "<ModuleName>.mod"
 *   - Con library "L" ⇒ "L.<ModuleName>.mod"
 *
 * @author eortiz
 */
public final class ModFormat {

    /** Identificador estable de los .mod con el formato v5 (sin interfaz). */
    public static final int MAGIC_NUMBER = 0x4D4F4435; // "MOD5"

    /** Identificador de los .mod v6 (H6.a — con sección interface embebida). */
    public static final int MAGIC_NUMBER_V6 = 0x4D4F4436; // "MOD6"

    /** Tamaño del header v5 en bytes (7 enteros de 32 bits). */
    public static final int HEADER_SIZE = 28;

    /** Tamaño del header v6 en bytes (8 enteros: añade interfaceSize). */
    public static final int HEADER_SIZE_V6 = 32;

    /** Versión lógica del formato; informativa. */
    public static final int FORMAT_VERSION = 6;

    /** True si {@code magic} es un MAGIC de .mod reconocido (v5 o v6). */
    public static boolean isKnownMagic(int magic) {
        return magic == MAGIC_NUMBER || magic == MAGIC_NUMBER_V6;
    }

    private ModFormat() { /* no-instanciable */ }
}
