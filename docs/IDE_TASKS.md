# IDE — lista de tareas (en construcción, 2026-06-03)

Fase "puesta al día del IDE" (acordada: T0 → T1 → **IDE** → Debugger → resto).
Doc de trabajo: el usuario va dictando ítems; cuando la lista esté completa se
organiza/prioriza y se funde en `V2_BACKLOG.md`. Notas técnicas (fichero/coste)
añadidas a partir del inventario del 2026-06-03.

Inventario base: `BpIde/` Swing (Java 1.8, GUI builder NetBeans). Componentes:
`FrmMain.java` (2173 L, +`.form`), `BpvmClient` (1009), `PicoExplorer` (805),
`PromptDialog` (288), `BpSyntaxHighlighter` (212), backends (~650), `IdePrefs` (132).

## Ítems dictados por el usuario

- **IDE-1 — Reorganizar `FrmMain`** (clase-Dios, 2173 L generada por GUI builder).
  Separar responsabilidades: editor, controlador run/debug, paneles, estado. Es la
  mejora estructural base (hacerla ANTES de añadir features encima). Riesgo: el
  `.form` ata parte del código al GUI builder; decidir si se mantiene el builder
  o se pasa a layout a mano para poder trocear.

- ✅ **IDE-2 — Look & Feel del SO** — HECHO + CONFIRMADO 2026-06-03 ("todo OK").
  El `main()` generado FORZABA "Metal"; ahora `setLookAndFeel(getSystemLookAndFeel
  ClassName())` (Windows/GTK/Aqua) antes de crear la ventana.

- **IDE-2 (orig) — Look & Feel del SO**. Hoy usa el L&F por defecto de Java (Metal).
  Cambiar a `UIManager.setLookAndFeel(getSystemLookAndFeelClassName())` (Windows/
  GTK/Aqua según SO), fijándolo en el arranque ANTES de crear `FrmMain`. Revisar
  HiDPI de paso. Coste: bajo.

- **IDE-3 — Recientes (ficheros .bp Y proyectos)**. Hoy File = Load / Save / Exit;
  Project = New / Open / Close. Añadir DOS listas de recientes persistidas en
  `IdePrefs`: **Recent files** (submenú en File, alimentado por openFileInEditor) y
  **Recent projects** (submenú en Project, alimentado por onOpenProject/onNewProject;
  el proyecto es un `.bpbuild` → `currentProjectFile`). Dedup + más-reciente-primero
  + cap N + "Clear". (Requisito ampliado por el usuario 2026-06-03: también projects.)
  Coste: medio.
  🔬 HECHO 2026-06-03 (pendiente confirmar visual): `IdePrefs.recentFiles`/
  `recentProjects` (newline-joined, cap 8, dedup); submenú "Recent Files" en File
  (hook en openFileInEditor) y "Recent Projects" en Project (hook en loadProjectFrom);
  cada uno con "Clear". Compila OK.

- **IDE-4 — `PicoExplorer` upload recuerda el último directorio**. Al hacer
  "upload" en la ventana del dispositivo, el `JFileChooser` no recuerda la carpeta
  y hay que navegar a la de los módulos cada vez. OJO: es un chooser PROPIO de
  `PicoExplorer`, distinto del de Load (que sí lo recuerda — #119/N-ide-last-dir).
  Reusar el mismo `IdePrefs` (clave de last-dir, quizá separada "lastUploadDir").
  Coste: bajo.
  ✅ HECHO + CONFIRMADO 2026-06-03 ("todo OK"): `IdePrefs.lastUploadDir` +
  `PicoExplorer.onUpload` arranca/guarda en esa carpeta.

- ✅ **Editor → RSyntaxTextArea** (HECHO 2026-06-03, confirmado por el usuario:
  "resalta native"). Sustituido el editor casero; `BpTokenMaker` con keywords del
  Lexer + tipos V2; `RTextScrollPane` (números de línea); current-line; bracket-match.
  Commit del increment + jar reconstruido. `BpSyntaxHighlighter.java` queda borrable.

- ✅ **IDE-6 — Resaltado al día** (HECHO con el swap a RSyntaxTextArea). `native` +
  `long/double/byte/word/short/int8/int16` ahora se resaltan.

- ✅ **IDE-5 — Code folding** — HECHO + CONFIRMADO 2026-06-03 ("todo OK", incl.
  el fix de `native function`/`public ...`).
  `BpFoldParser` registrado para `text/bp`: abridores (function/class/if/while/for/
  switch/try/parallel/property/get/set/module/...) vs cerradores (end/endif/endwh/
  endsw/next/endtry/endpar/endprop/endget/endset), patrón puntero+getParent() como
  CurlyFoldParser; primera-palabra de línea, ignora `//` y blancos; descarta bloques
  de 1 línea. Build OK. **Probar**: deben salir flechas de plegado en el gutter.

- **IDE-5 (orig) — Code folding en el editor** (plegar/minimizar funciones y clases).
  El editor hoy es básico (`BpSyntaxHighlighter`, 212 L sobre un JTextComponent).
  El folding en Swing puro es trabajo; valorar `RSyntaxTextArea` (soporta folding,
  números de línea, etc. de serie) como base del editor. Decisión: ¿mejorar el
  editor actual o adoptar RSyntaxTextArea? (afecta también a IDE-6). Coste: medio-alto.

- **IDE-6 — Sincronizar el resaltado de keywords con el lexer**. `integer` sale en
  azul pero `native` no → el set de palabras de `BpSyntaxHighlighter` está
  desfasado respecto al lexer actual. Faltan al menos `native` y probablemente los
  añadidos de V2 (`long`, `double`, `byte`, `word`, `short`, `get`/`set`, etc.).
  Al implementar: **diff** del set del highlighter contra las keywords reales del
  `Lexer` y dejar una sola fuente de verdad si es posible. Coste: bajo.

## Decisiones (2026-06-03)

- **Editor → adoptar `RSyntaxTextArea`** (decisión del usuario). Resuelve de un
  plumazo **IDE-5** (code folding) e **IDE-6** (resaltado con keywords al día), y
  añade gratis números de línea, find/replace, current-line, bracket-matching.
  Sustituye el editor casero (`JTextComponent` + `BpSyntaxHighlighter`). Requiere
  un `TokenMaker` propio para BP (portar el set de keywords del `Lexer`).
- **Modo de trabajo**: incremental, "sobre la marcha" — cambios pequeños + el
  usuario prueba en el IDE. Mejoras que surjan se añaden, pero **con límite
  razonable**: es una PUESTA AL DÍA, no un rewrite.
- **Lista de captura: cerrada de momento** (6 ítems). Reabrible si surge algo.

## IDE-7 · Selección múltiple en el árbol del dispositivo — Eduardo, 4-ago (V5)

> «Permitir seleccionar varios. Borrar 1 a 1 es lento, ya que cuando termina
> tiene que refrescar el árbol y cuando hay muchos archivos se nota.»

Dictado durante las pruebas de placa de H13. **No entra en V4** (freeze): es
mejora, no bug.

**Lo que hay hoy**, y el porqué de la lentitud — el coste NO está en el borrado,
está en el refresco:

- `PicoExplorer.java:195` — `TreeSelectionModel.SINGLE_TREE_SELECTION`. Sólo se
  puede marcar un nodo, así que borrar N son N pasadas.
- `PicoExplorer.java:1353` `onDelete()` — por **cada** fichero: un diálogo de
  confirmación, un `b.del(...)` por el wire y **un `onRefresh()` entero**, que
  vuelve a listar todo el FS. Con muchos ficheros ese listado es lo que se nota,
  y se paga N veces para borrar N.

**Las tres piezas, independientes entre sí** (se puede hacer la 2 sin la 1 y ya
se gana):

1. `SINGLE_TREE_SELECTION` → `DISCONTIGUOUS_TREE_SELECTION`. Una línea.
2. `onDelete()` sobre la selección entera: **una** confirmación para los N
   («¿Borrar 7 ficheros?»), N borrados, y **un solo `onRefresh()` al final**.
   Aquí está casi toda la ganancia.
3. Repasar si el mismo patrón —refrescar dentro del bucle— aparece en las otras
   operaciones del árbol (subida de varios, sobre todo).

**Cuidado al hacerlo**: la confirmación de N ficheros tiene que **enseñar la
lista**, no sólo el número. Borrar en la placa no tiene deshacer, y un árbol con
multiselección es fácil de ampliar sin querer con shift.

## (pendiente: más ítems que el usuario siga dictando)
