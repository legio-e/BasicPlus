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

    /** Identificador de los .mod v7 (V6/N1.4 — con sección `native` embebida:
     *  el `.mdn` deja de ser un fichero aparte). */
    public static final int MAGIC_NUMBER_V7 = 0x4D4F4437; // "MOD7"

    /** Tamaño del header v7 en bytes (9 enteros: añade nativeSize). */
    public static final int HEADER_SIZE_V7 = 36;

    /** Versión lógica del formato; informativa. */
    public static final int FORMAT_VERSION = 7;

    /** True si {@code magic} es un MAGIC de .mod RECONOCIBLE (v5 o v6). Sirve
     *  para INSPECCIONAR (disasm / module info): saber qué es un fichero. NO
     *  implica que se pueda ejecutar — para eso está {@link #isAbiSupported}. */
    public static boolean isKnownMagic(int magic) {
        return magic == MAGIC_NUMBER || magic == MAGIC_NUMBER_V6
            || magic == MAGIC_NUMBER_V7;
    }

    /** Bytes de header que le corresponden a este magic. El header CRECE con la
     *  versión, así que leerlo mal desplaza TODAS las secciones. */
    public static int headerSizeDe(int magic) {
        if (magic == MAGIC_NUMBER_V7) return HEADER_SIZE_V7;
        if (magic == MAGIC_NUMBER_V6) return HEADER_SIZE_V6;
        return HEADER_SIZE;
    }

    // ============================================================
    // #284 — CONTRATO DE ABI: la versión de formato ES la declaración de ABI
    // ============================================================
    //
    // El ensanchado de refs 4→8B (H1.2a, d2dcbe9, 9-jul-2026) cambió el ABI del
    // bytecode pero NO bumpeó la versión del .mod. Consecuencia: un .mod de la
    // era 4B entra en una VM de 8B y corrompe memoria EN SILENCIO (use-after-free,
    // objetos mal creados) — nos costó días de depuración a ciegas.
    //
    //   v5 y anteriores : ABI AMBIGUO. Puede ser era 4B (corruptor) o era 8B; el
    //                     fichero no lo declara y no hay forma de distinguirlo.
    //                     ⇒ los loaders lo RECHAZAN.
    //   v6              : refs de 8 bytes GARANTIZADO — v6 nació DESPUÉS del
    //                     ensanchado, así que todo v6 sale del compilador actual.
    //
    // Si el ancho de ref vuelve a cambiar ⇒ hay que bumpear a v7. Regla: un .mod
    // que la VM no pueda garantizar debe GRITAR al cargarlo, nunca correr y
    // corromper (principio "errores que gritan", ver docs/V4_REF_AUDIT.md).

    /** Ancho de referencia (bytes) que garantiza el formato v6. */
    public static final int ABI_REF_SIZE_V6 = 8;

    /** True si un .mod con este magic tiene un ABI que esta VM puede EJECUTAR.
     *
     *  <p>v6 Y v7. Y que v6 siga valiendo NO es una concesión: el gate de `#284`
     *  vigila el <b>ABI</b> —el ancho de referencia—, y v7 no lo toca. v7 sólo
     *  AÑADE una sección (`native`) al final del header y del layout, igual que
     *  v6 añadió `interface` sobre v5. Un v6 ejecuta idéntico; lo único que no
     *  tiene es bloque nativo embebido, que es justo lo que no tenía antes.
     *
     *  <p>Por eso el salto a v7 <b>no obliga a regenerar</b> los `.mod` que ya
     *  hay. v5 sigue rechazado, y por el motivo de siempre: su ABI es ambiguo. */
    public static boolean isAbiSupported(int magic) {
        return magic == MAGIC_NUMBER_V6 || magic == MAGIC_NUMBER_V7;
    }

    /** Motivo de rechazo, claro y accionable, para un magic no ejecutable. */
    public static String abiRejectReason(int magic) {
        if (magic == MAGIC_NUMBER) {
            return "formato v5 (\"MOD5\"): es anterior al ensanchado de refs 4->8B, "
                 + "así que su ABI no se puede garantizar (si es de la era 4B, "
                 + "corrompería memoria en silencio). Recompílalo con el compilador actual.";
        }
        return String.format(
                "MAGIC 0x%08X no reconocido: no parece un .mod (se esperaba \"MOD6\" o \"MOD7\")", magic);
    }

    // ============================================================
    // [V6/N1.4] Fundir un `.mdn` DENTRO del `.mod` (sección `native`, v7)
    // ============================================================

    /**
     * Devuelve los bytes de un `.mod` con {@code blob} AÑADIDO a su sección
     * `native`. Si el módulo era v6, sale v7; si ya era v7 con contenido, el
     * blob se **añade** al final de la sección.
     *
     * <p><b>Que sea acumulativa es el diseño, no un extra.</b> La sección son N
     * blobs `.mdn` concatenados, uno por familia; llamar una vez por familia da
     * el multifamilia sin escribir nada más. Y como cada `.mdn` dice su `arch`,
     * el cargador se queda con el suyo.
     *
     * <p><b>Alineación.</b> Cada blob empieza en múltiplo de 4 — con XIP el
     * módulo se ejecuta EN SU SITIO, así que su posición en el fichero ES su
     * dirección de ejecución, y un blob RISC-V en offset impar no arranca. El
     * relleno va delante de cada blob y cuenta dentro de `nativeSize`.
     *
     * <p>Idea de Eduardo: hacerlo <b>en memoria</b>. `MdnPack` ya construía el
     * `.mdn` entero en un buffer, así que escribirlo a disco para releerlo y
     * reescribir el `.mod` eran dos vueltas que no hacían falta. El llamante
     * junta las dos cosas aquí y escribe el fichero final UNA vez.
     */
    public static byte[] conBloqueNativo(byte[] mod, byte[] blob) {
        if (mod == null || mod.length < HEADER_SIZE_V6) {
            throw new IllegalArgumentException("no es un .mod: " +
                    (mod == null ? "null" : mod.length + " bytes"));
        }
        java.nio.ByteBuffer b = java.nio.ByteBuffer.wrap(mod);   // big-endian
        int magic = b.getInt();
        if (!isAbiSupported(magic)) {
            throw new IllegalArgumentException(abiRejectReason(magic));
        }
        boolean v7 = (magic == MAGIC_NUMBER_V7);
        int dataSize   = b.getInt();
        int mainOffset = b.getInt();
        int impSize    = b.getInt();
        int expSize    = b.getInt();
        int codeSize   = b.getInt();
        int libSize    = b.getInt();
        int ifaceSize  = b.getInt();
        int natSize    = v7 ? b.getInt() : 0;

        int hdr    = v7 ? HEADER_SIZE_V7 : HEADER_SIZE_V6;
        int iniLib = hdr;
        int iniNat = iniLib + libSize + impSize + expSize + ifaceSize;
        int iniData = iniNat + natSize;

        /* El relleno se calcula sobre dónde caerá el blob en el fichero NUEVO,
         * que siempre tiene header v7 (36). Si el módulo venía en v6 el header
         * crece 4 bytes — múltiplo de 4, así que no descoloca lo de delante. */
        int finNatNuevo = HEADER_SIZE_V7 + libSize + impSize + expSize + ifaceSize + natSize;
        int pad = (4 - (finNatNuevo & 3)) & 3;

        int natSizeNuevo = natSize + pad + blob.length;
        java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream(
                mod.length + pad + blob.length + 4);
        java.nio.ByteBuffer h = java.nio.ByteBuffer.allocate(HEADER_SIZE_V7);
        h.putInt(MAGIC_NUMBER_V7);
        h.putInt(dataSize); h.putInt(mainOffset); h.putInt(impSize);
        h.putInt(expSize);  h.putInt(codeSize);   h.putInt(libSize);
        h.putInt(ifaceSize); h.putInt(natSizeNuevo);
        out.write(h.array(), 0, HEADER_SIZE_V7);

        out.write(mod, iniLib, iniNat - iniLib);        // library+imports+exports+interface
        if (natSize > 0) out.write(mod, iniNat, natSize);   // los blobs que ya había
        for (int i = 0; i < pad; i++) out.write(0);         // relleno de ESTE blob
        out.write(blob, 0, blob.length);
        out.write(mod, iniData, dataSize + codeSize);   // data + code, intactos
        return out.toByteArray();
    }

    private ModFormat() { /* no-instanciable */ }
}
