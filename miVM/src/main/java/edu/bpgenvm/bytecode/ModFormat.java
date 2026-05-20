/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.bytecode;

/**
 * Constantes del formato de fichero .mod.
 *
 * Formato actual (v5):
 * <pre>
 *   [ MAGIC=0x4D4F4435 ("MOD5") ]   4 bytes
 *   [ dataSize                  ]   4 bytes
 *   [ mainOffset                ]   4 bytes  (offset relativo en code, o -1)
 *   [ importsSize               ]   4 bytes
 *   [ exportsSize               ]   4 bytes
 *   [ codeSize                  ]   4 bytes
 *   [ librarySize               ]   4 bytes  (0 si no hay library)
 *   --- header (28 bytes) ---
 *   [ library  ] librarySize bytes  (UTF-8 raw, sin length prefix)
 *   [ imports  ] importsSize bytes  (metadatos del linker)
 *   [ exports  ] exportsSize bytes  (metadatos del linker)
 *   [ data     ] dataSize    bytes  (constantes + globals + class descriptors)
 *   [ code     ] codeSize    bytes
 * </pre>
 *
 * Sección IMPORTS (v5):
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

    /** Identificador estable de los .mod con el formato v5. */
    public static final int MAGIC_NUMBER = 0x4D4F4435; // "MOD5"

    /** Tamaño total del header binario en bytes (7 enteros de 32 bits). */
    public static final int HEADER_SIZE = 28;

    /** Versión lógica del formato; informativa. */
    public static final int FORMAT_VERSION = 5;

    private ModFormat() { /* no-instanciable */ }
}
