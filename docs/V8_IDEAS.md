# V8 — ideas y diseño

> 🚀 **Desde el 14-oct-2026, V8 es la versión en curso** (V7 se publicó el 13-oct, tag `v7.0`). Éste es el cuaderno de
> diseño de V8, hermano de `V7_IDEAS.md`: las charlas de diseño van aquí **antes** de escribir código. El estado vive en
> `FICHAS.md` §ABIERTAS, bloque V8.
>
> **El modo de V8** (Eduardo, 13-oct): *«V8 es para probar cosas nuevas, para experimentar, sin ponernos obligaciones.
> Probaremos cosas y lo que funcione lo subiremos al final y lo que no funcione se puede aplazar para más adelante.»*
> El tema, decidido el 24-sep: la WiFi, el Bluetooth y el bajo consumo.

---

## E1 · El teclado y el ratón Bluetooth en la P4 (`#697`)

### De dónde sale

Eduardo, 13-oct: *«tengo un teclado y un ratón Bluetooth, podemos empezar por ahí a ver si conseguimos emparejarlos con
la P4 y a ver qué pasa»*. Son chinos y sin documentación. Emparejados con el PC, Windows los ve como Bluetooth LE con
HID sobre GATT (servicio `1812`):

| aparato | nombre | VID/PID | lo que declara |
|---|---|---|---|
| teclado | `Bluetooth Keyboard` | `04E8`/`7021` (el VID de Samsung, prestado) | teclado, multimedia y ratón |
| ratón | `BT5.2 Mouse` | `1235`/`AA22` | ratón, teclado, multimedia y sistema |

Que sean LE importa: **ninguna placa nuestra hace Bluetooth clásico**. La S3, la C3, la C5 y la C6 sólo hacen LE, y la
P4 ni siquiera tiene radio.

### Lo probado el 14-oct: pasos 1 y 2, hechos

Con dos firmwares de prueba del ESP-IDF, sin BasicPlus, en la P4 de Waveshare. El código está fuera del repo, en
`C:\tmp\v8bt`:
- **`p4_scan`**: el escaneo.
- **`p4_hid`**: el anfitrión HID. Conecta, empareja, se suscribe a los informes, los descodifica y reconecta por la
  dirección guardada.
- **`leer_consola.py`**: lee la consola sin reiniciar la placa.

1. **La P4 hace Bluetooth a través de su ESP32-C6.** La C6 (ESP32-C6-MINI-1) va por SDIO de 4 bits a 40 MHz. Los
   pines salen del esquema de Waveshare, que Eduardo pasó, y son los mismos que en la placa de Espressif:
   - SDIO: CLK 18, CMD 19 y D0–D3 en los 14–17;
   - el reset de la C6 (`C6_CHIP_PU`), en el GPIO 54;
   - el GPIO 6 llega al IO2 de la C6;
   - el conector P1 saca la UART0 y el IO9 (el de arranque) de la C6, por si hubiera que grabarla.

   En la P4 corre NimBLE como anfitrión (`BT_CONTROLLER_DISABLED`), unido a la C6 por `esp_hosted` 3.0.9, del registro
   de Espressif: es la única versión que admite el IDF v6.
2. **La C6 de fábrica lleva un ESP-Hosted viejo**: se presenta con versión 0.0.0, «BLE only» y «HCI over SDIO».
   - No contesta al protocolo de control (RPC V2) del anfitrión 3.x: ni a la versión ni a encender el controlador.
     El anfitrión avisa «major version mismatch — OTA coprocessor from host».
   - **Pero su controlador arranca encendido y el canal HCI funciona.** Con `bring_up_controller = false`, NimBLE se
     engancha y todo va. **Para el Bluetooth no hace falta tocar la C6.**
3. **Los dos se emparejan en Just Works**, aunque la placa diga que puede enseñar un código: el teclado no pide el
   suyo. Quedan vinculados, con las claves en la NVS. Al anunciarse en modo de emparejar, el ratón dice que es HID
   (servicio y apariencia `0x03C2`) y el teclado no, así que se reconocen por el nombre.
4. **Cambian de dirección cada vez que entran en modo de emparejar.** El ratón alterna `…:01:C5` y `…:02:CE`; el
   teclado estrena una cada vez. La dirección que vale es la del vínculo.
5. **Los informes:**
   - El ratón, por su informe 1, de 7 B: botones (`01` izquierdo, `02` derecho, `04` central), X e Y de 16 bits con
     signo (±2047), la rueda y el desplazamiento horizontal.
   - El teclado, el informe estándar de 8 B: modificadores, un byte reservado y seis teclas. Cada pulsación llega con
     su suelta y no se perdió ninguna.
   - Los dos juntos, en dos conexiones a la vez.
6. **Los parámetros de conexión.** Los aparatos piden cambiarlos: el ratón, 7,5–11,25 ms con latencia 32; el teclado,
   10 ms sin latencia.
   - La primera vez, aceptar el cambio del ratón dejó el procedimiento sin terminar, y a los 40 s justos la conexión se
     cortó: `0x222`, *LL Response Timeout*. Puede ser cosa del controlador viejo de la C6.
   - La placa ahora fija 7,5–15 ms sin latencia y rechaza los cambios, y ninguna conexión se ha cortado.
   - El precio: sin latencia, el aparato gasta más batería.
7. **Reconectar por la dirección guardada. Es idea de Eduardo**, que recordaba que emparejar *«no siempre funcionaba»* y
   que con la dirección MAC sí: *«una vez tienes la dirección MAC, las siguientes veces se puede conectar directamente
   siendo más rápido y limpio»*.
   - **Cómo:** la dirección se guarda al vincular, y la placa espera a las guardadas con la lista blanca del
     controlador (`ble_gap_connect` sin dirección), sin escanear.
   - **Resultado:** al encender el aparato o al reiniciar la placa, vuelven en ~1 s, cifrados con su clave y sin modo
     de emparejar.
   - **Hace falta:** un aparato vinculado ya no se anuncia con su nombre, sino a su anfitrión.
8. **Se duermen y vuelven solos.** Tras unos 10–12 minutos sin usarse, cada aparato cierra la conexión. El ratón lo
   hace con el motivo `0x13` (el otro extremo cerró); el teclado, con códigos que no son estándar (`0xE1`, `0x90`),
   probablemente los suyos propios al dormirse. Al moverlo o al pulsar una tecla vuelven por la dirección guardada en
   menos de un segundo, y las primeras teclas llegan incluso antes de terminar la suscripción: el teclado recuerda que
   estábamos suscritos.

### El recorrido del usuario (Eduardo, 14-oct)

Eduardo, poniéndose en el lugar del usuario: una opción del programa para configurar el Bluetooth.
- **La primera pantalla:** activar o desactivar, la lista de los aparatos ya conocidos y la opción de añadir nuevos.
- **La segunda pantalla:** el escaneo, con la lista de los encontrados. Se elige uno y se añade a la lista.

**Qué se guarda**, según Eduardo: *«Muy poco, el nombre, la mac address y el tipo»*. Con un matiz para cada cosa:
1. **El nombre**, para la lista.
2. **La dirección con su tipo**, pública o aleatoria: 6 bytes y uno de tipo (el `ble_addr_t`). Sin el tipo no se
   puede conectar.
3. **El tipo de aparato** (teclado, ratón…), para el icono y para saber adónde va lo que manda. **En el escaneo no
   siempre se sabe**: el ratón lo anuncia, pero el teclado no. El tipo seguro sale de su descriptor de informes, al
   conectarlo. En la lista de encontrados va el nombre, y el tipo se apunta al añadir.
4. **Y lo que no se ve: las claves del vínculo**, las que se acuerdan al emparejar. Un teclado o un ratón Bluetooth
   exigen conexión cifrada: sin las claves habría que ponerlo en modo de emparejar cada vez. Las guarda la pila de
   Bluetooth al emparejar. Al quitar un aparato de la lista hay que borrar también sus claves.

Quizá también la distribución de un teclado (española, americana), aunque puede ser un ajuste general.

El recorrido encaja con lo medido:
- **Añadir:** escanear (los que están en modo de emparejar se anuncian con su nombre), elegir, conectar, emparejar y
  guardar. Si algún aparato pidiera código, la pantalla lo enseña para teclearlo.
- **Al arrancar**, si está activado, esperar a los guardados por su dirección.
- **Quitar:** la entrada y sus claves.
- **Desactivar:** no arrancar la radio, lo que además ahorra consumo.

### La configuración, un programa BasicPlus (Eduardo, 14-oct)

Eduardo: *«Podemos hacer un programa BP que haga todo eso. Al terminar se puede guardar en un fichero o en el
environment. Una vez configurado no hace falta volver a tocarlo hasta que el usuario quiera.»*

Así quedan repartidos los papeles:
- **El programa BasicPlus de configuración**, con sus dos pantallas. Es un programa más: se ejecuta cuando el usuario
  quiere cambiar algo.
- **Un módulo BasicPlus para ese programa**, por diseñar: activar y desactivar, escanear (los encontrados, con nombre,
  dirección e intensidad), añadir (conecta, empareja y devuelve nombre, dirección y tipo), quitar, y la lista de los
  guardados. Y el aviso para el caso del código.
- **El firmware.** Al arrancar lee la configuración y, si está activado, espera a los aparatos guardados por su
  dirección y lleva lo que mandan al puntero y al teclado de LVGL (y quizá a eventos del programa). Funciona con
  cualquier programa, o sin ninguno, como el cable del IDE.

**¿Fichero o Entorno? Un fichero JSON en `/sys`** (decidido por Eduardo el 14-oct: *«un fichero json en /sys me
parece bien»*). Se valoró:
- **El Entorno** es donde vive la configuración de la placa: la pantalla de la P4 pasó de `/sys/board.json` al
  Entorno. El firmware lo lee al arrancar y el IDE lo enseña y lo deja editar. Pero hoy un programa BasicPlus sólo
  puede leerlo (`Machine.getEnv`): escribirlo pediría un `Machine.setEnv`, nuevo.
- **Un fichero** en `/sys`: BasicPlus ya escribe ficheros y JSON (`Json`). El firmware tendrá que leerlo al
  arrancar. Sin ningún cambio en el sistema.

El nombre del fichero y dónde van las claves (en él o en otro fichero de `/sys`) quedan por decidir.

**El programa, en un pack** (Eduardo, 14-oct): *«se lo podemos dar al usuario en un pack y el usuario puede
ejecutarlo siempre que quiera desde el IDE»*. Un pack ejecutable lleva su programa con `main`, todo lo que importa y
sus recursos (el `.win` de cada pantalla). Hay que comprobar cómo se lanza desde el IDE un pack ejecutable que ya está
grabado en la zona de packs.

### Para más adelante: ejecutar un programa BasicPlus desde otro (Eduardo, 14-oct)

Lo natural sería que una aplicación tuviera su opción «Configurar el Bluetooth», que lance el programa de
configuración y, al terminar, vuelva. Eduardo: *«En teoría hoy no es posible, nuestro SO solamente soporta la ejecución
de 1 programa BP. Sin embargo hace tiempo te comenté lo de los 'Overlays', con comandos nuevos 'LoadMod' y 'UnloadMod'
que nos permitía hacer precisamente esto.»*

La idea es suya, del 29-may: `V2_BACKLOG` §17, *«`loadModule` / `unloadModule` — overlays estilo MS-DOS»*. Una parte
del programa es residente y el resto se carga y se descarga según se necesita. Lo que se analizó entonces:
- **Cargar** es relativamente fácil: hacer en marcha lo que el cargador ya hace al arrancar.
- **Descargar** es lo difícil, por dos cosas: recuperar la memoria del módulo, y que no quede nada vivo apuntándolo
  (una función en la pila, un objeto de una clase suya, un hilo).
- **Se recomendaba el modelo (A)**: un hueco fijo para el overlay, junto a lo residente, y un overlay no llama a
  otro. Su ejemplo de uso era justo éste: *«pantalla de config, rutina de diagnóstico, asistente»*.

Desde V2 han cambiado cosas que lo acercan:
- **El enlace tardío** (V7, MOD9): los módulos se enlazan por nombre al cargar, y el cargador ya construye las tablas
  de cada módulo. Cargar uno más con el programa en marcha es ese mismo trabajo, módulo a módulo.
- **Los handles con generación** (V4): una referencia a algo que ya no existe falla en el acto en vez de corromper.
- **Los packs en sitio** (XIP): un módulo de un pack grabado corre desde la flash sin copiarse a la RAM. Cargar un
  overlay desde un pack casi no gasta RAM: sólo sus globales y sus objetos.
- **«Al acabar, todo vuelve a como estaba»** (`#668`): un overlay que se descarga puede destruir sus objetos y soltar
  sus recursos como lo hace hoy el final de un programa.
- **Las condiciones las prepara el programa** (como en `Heap.compact`): por ejemplo, descargar sólo desde el hilo que
  lo cargó y sin hilos del overlay vivos.

Es trabajo de sistema y de lenguaje, así que va para más adelante y en su propio análisis. **De momento no hace
falta** (Eduardo, 14-oct: *«lo de los overlays de momento no lo necesitamos, ejecutamos el programa desde el IDE y con
eso es suficiente»*): el programa de configuración es un programa más, que se lanza desde el IDE.

⚠️ **Una trampa, medida el 14-oct: la NVS no sobrevive a una actualización del firmware.** NimBLE guarda las claves en
la NVS (`0x9000`), y la imagen fusionada con la que se actualiza (la que `INSTALAR_FIRMWARE` dice grabar en el 0) lleva
`FF` en esa zona: la de v7.0, comprobada. Cada actualización borraría las claves, y el usuario tendría que volver a
emparejarlo todo. Para que «una vez configurado» sea verdad, **las claves tienen que vivir con el resto de la
configuración, en nuestro almacenamiento** (el Entorno o el sistema de ficheros, más allá de la aplicación, donde una
actualización no llega). NimBLE deja poner un almacén propio (`ble_hs_cfg.store_read_cb`, `store_write_cb`,
`store_delete_cb`). Y como las claves son secretas, mejor en un fichero que el IDE no enseñe que en el Entorno, que sí
lo enseña.

### Cómo entran el ratón y el teclado en LVGL: una API de entrada, genérica (Eduardo, 14-oct)

La pregunta de Eduardo: con el teclado y el ratón configurados, se arranca un programa con LVGL; ¿cómo los reconoce
LVGL? **Por el mismo camino por el que hoy entra la pantalla táctil.**

**Así entra hoy la táctil** (la P4 y la DK2):
1. **La imagen publica una API** con `bios->publica`, bajo la marca `PANT` (`include/bpvm_pant.h`). Sus funciones
   van por nombre: `init`, `size`, `flush`, `rotate` y, si hay táctil, `touch(&x, &y)`. Las opcionales son las que
   un driver no publica, y añadir una no rompe a los drivers ya grabados.
2. **El pack de la GUI la busca en su primer uso** (`bpstdlib/gui/nativo/src/pant_lvgl.c`). Si `touch` está
   publicada, crea un dispositivo de entrada de LVGL de tipo puntero cuyo lector la llama.
3. **El programa BasicPlus no hace nada**: crea su pantalla y los botones ya responden al dedo.

**El ratón y el teclado, igual:**
1. **Quien tenga ratón o teclado publica otra API**, con una marca nueva (por ejemplo `ENTR`, de «entrada»), por
   nombre:
   - `raton`: lo que se ha movido desde la última vez, los botones y la rueda;
   - `teclado`: la siguiente tecla, pulsada o soltada, ya convertida en carácter o en tecla de LVGL.
2. **El pack de la GUI, al crear la pantalla, busca también ésa.** Si está:
   - **el ratón** es un segundo puntero: su lector suma los desplazamientos, los limita al panel y da el botón
     izquierdo como «pulsado». Lleva cursor (`lv_indev_set_cursor`) con una flechita que trae el pack;
   - **el teclado** es un dispositivo de tipo teclado con un **grupo por defecto** (`lv_group_set_default`). Así, cada
     campo de texto o botón que cree después el programa entra solo en el grupo: Tab pasa al siguiente, el campo con
     el foco recibe las letras, Enter pulsa el botón con el foco, y un clic de ratón en un campo también le da el
     foco.
3. **Los programas BasicPlus no cambian**: los de hoy funcionarían con ratón y teclado.

**Genérica, independiente del Bluetooth** (Eduardo: *«tu planteamiento de hacerlo genérico, independiente del
Bluetooth es correcto»*). La API habla de ratón y de teclado, no de quién los trae. Hoy sería el anfitrión Bluetooth
de la imagen; mañana podría ser un teclado USB (la P4 y la S3 tienen USB *host*) con la misma API, y el pack no se
enteraría. Una imagen que no la publique deja el pack como hoy, y un pack viejo con una imagen nueva, también.

**La distribución del teclado** (Eduardo: *«Los teclados se pueden internacionalizar, igual que en el PC. Para no
complicarnos, podemos empezar con un teclado US estándar y luego ya pasamos a teclados particulares»*):
- **Primero, el teclado US estándar.**
- Las particulares, después, con una tabla por distribución, elegida en la configuración (el JSON de `/sys`).
- La conversión de la posición de la tecla a carácter la hace quien publica la API. Así el pack recibe caracteres y
  no tiene que saber nada de distribuciones.

**Lo demás que habrá que resolver:**
- **La rueda.** LVGL 9.2 no la usa para desplazar: hay que hacerlo a mano, buscando el objeto que está bajo el cursor
  y desplazándolo.
- **Los hilos.** Los informes llegan en la tarea de NimBLE y LVGL los lee en el suyo, así que entre los dos hace falta
  una cola con cerrojo.
- **Primero en el PC**, como pide la cascada. El simulador ya registra el ratón del PC como puntero
  (`gui_display_sdl.c`). Añadiéndole el teclado del PC (`lv_sdl_keyboard_create`) y el grupo por defecto, el
  comportamiento del teclado en la GUI (foco, Tab, Enter) se prueba allí antes de tocar la placa.

### Lo que LVGL ya trae (9.2.2, la copia de `third_party/lvgl`)

**El ratón** es un dispositivo de tipo puntero (`LV_INDEV_TYPE_POINTER`), como la pantalla táctil.
- El driver le da un punto y el estado del botón. Como el ratón manda desplazamientos, se suman y se limitan al panel.
- `lv_indev_set_cursor(indev, objeto)` pone un objeto (una flecha) que sigue al puntero. En el panel no hay sistema
  operativo que la dibuje.
- Pueden convivir dos punteros: el ratón no le quita nada a la táctil.
- **La rueda no desplaza.** En 9.2, el driver del PC la trata como un *encoder*, que mueve el foco. Para desplazar con
  ella hay que hacerlo a mano: desplazar el objeto que está bajo el cursor.

**El teclado** es de tipo teclado (`LV_INDEV_TYPE_KEYPAD`).
- Manda cada tecla al widget con el foco dentro de un grupo (`lv_indev_set_group`): un campo de texto recibe los
  caracteres; Tab, las flechas, Enter, Esc y Retroceso tienen sus `LV_KEY_*`.
- **El teclado manda la posición de la tecla** (en distribución americana), no el carácter. Para uno español (la ñ,
  los símbolos) hace falta una tabla de distribución.

**Hoy**, el micro simulado registra el ratón del PC como puntero (`gui_display_sdl.c`, `lv_sdl_mouse_create()`): los
programas con GUI ya se manejan con ratón en el PC. No registra ni el teclado ni la rueda.

### Lo que hay que decidir (Eduardo)

1. **Dónde vive el anfitrión Bluetooth.** En el firmware, en C y común a la familia ESP32: la P4 llega a la radio por
   ESP-Hosted, y la S3, la C3, la C5 y la C6 tienen controlador propio. NimBLE trae su propia tarea. Hay que medir
   cuánta memoria cuesta: en la P4 sobra PSRAM, pero la C3 y la C6 van justas (`MEMORIA.md`).
2. **La GUI. Hay camino** (14-oct; arriba, «Cómo entran el ratón y el teclado en LVGL»): una API de entrada genérica,
   que el pack de la GUI busca como hoy busca la táctil. Eduardo, conforme.
3. **El programa sin GUI.** ¿Un módulo de BasicPlus que dé las teclas y el ratón como eventos? El nombre, por decidir.
4. **El emparejamiento es configuración de la placa, no del programa**, como el Entorno. **El camino lo dio Eduardo
   el 14-oct**: un programa BasicPlus de configuración, que guarda en un fichero o en el Entorno (arriba, «La
   configuración, un programa BasicPlus»), que guarda en **un fichero JSON en `/sys`** (decidido el mismo día) y se
   le da al usuario en un pack. Las claves tienen que salir de la NVS, porque una actualización del firmware la borra.
   Quedan por decidir el nombre del fichero y si las claves van en él.
5. **La distribución del teclado**: primero, el US estándar; las particulares, después, elegidas en la configuración
   (Eduardo, 14-oct).
6. **Los parámetros de conexión**: rechazarlos siempre, como hoy, o aceptar los que no llevan latencia. Hay que
   volver a mirarlo con un firmware nuevo en la C6.
7. **La C6 de la P4.** Para el Bluetooth basta la de fábrica. La WiFi de V8 necesitará el protocolo de control: habrá
   que actualizar la C6 a ESP-Hosted 3.x, por OTA desde la P4 o por su conector P1. Va en un experimento aparte.
8. **El kit P4 EV** lleva, presumiblemente, la misma C6. Falta probarlo.

### Lo que puede seguir (sin orden ni obligación)

- **El ratón en la GUI de la P4**: el anfitrión dentro del firmware de BasicPlus, con el puntero y el cursor. En una
  rama de experimento.
- **El mismo anfitrión en una ESP con Bluetooth propio** (la C5, que tiene pantalla), para ver si el código común
  sirve.
- **La C6 de la P4, actualizada a ESP-Hosted 3.x**: su versión, el protocolo de control y la puerta a la WiFi.
- **La memoria**: cuánto ocupan NimBLE y ESP-Hosted.

---

## E2 · JC: BasicPlus con camisa de Java y C# (Eduardo, 17-oct)

### La idea

Eduardo, 17-oct: *«Ya hablamos de incorporar un 2º lenguaje de sintaxis tipo Java o C#. Pero en realidad es nuestro
Basic Plus con una camisa diferente. Así que el AST es el mismo pero necesitamos un scanner y un parser nuevos. Para
empezar el nombre: JC (abreviación de Java y C#). Ahora desarrollamos un traductor de BP->AST->JC y luego de
JC->AST->BP. No necesitamos nada más.»*

Y sobre el coste: *«Yo me centraría en el coste de los cambios del compilador, los cambios del IDE deberían ser
pequeños y los de la VM deberían ser casi nulos (al menos inicialmente).»*

La idea venía del 9-oct, sin versión entonces: facilitar la entrada a quien viene de C, C# y Java. El argumento de
Eduardo para el coste: con el mismo backend, dos lenguajes no cuestan el doble, porque el coste del proyecto está en lo
embebido (el sistema y la VM), no en el frontend.

### Las piezas: qué hay y qué es nuevo

| pieza | estado | para qué |
|---|---|---|
| BP → AST (`Lexer`, `Parser`: 699 + 2.022 líneas) | existe | |
| AST → `.mod` (el semántico y los emisores, el grueso de las 34.511 líneas del frontend) | existe | |
| **AST → JC**: escribir JC desde el árbol | **nuevo** | el traductor BP → JC |
| **JC → AST**: el scanner y el parser de JC | **nuevo** | el compilador de JC y el traductor JC → BP |
| **AST → BP**: escribir BP desde el árbol | **nuevo** | el traductor JC → BP |

El tercero no estaba en la cuenta de Eduardo: para ir de JC a BP hace falta escribir BP desde el árbol, y hoy no hay
nada que lo haga. `AstPrinter` (407 líneas) vuelca el árbol para depurar, no escribe BP; pero recorre todos los nodos y
sirve de esqueleto. Los dos escritores (a JC y a BP) comparten el recorrido y sólo cambia el «dialecto».

### Lo medido el 17-oct

1. **El AST conserva la forma escrita**; el parser no «desazucara». El operador compuesto (`AssignOpKind`), el `step`
   del `for`, los paréntesis (`ParenExpr`), las propiedades, los eventos, el `parallel`, el `print` con sus separadores
   y las anotaciones tienen su nodo. El parser fabrica algo sólo en cuatro sitios, y los cuatro se reconocen:
   - los grupos de un `var` (`var i: integer, r: double`) salen en un `VarDecl` cada uno, y se escribirían en líneas
     separadas, que es equivalente;
   - el cuerpo de las `native` externas de un pack: llevan `isPackExtern` y se escribe sólo la firma;
   - el `finally` de `try var`: la variable va en `recurso`, y se vuelve a escribir `try var` sin el `finally`;
   - el `if` de una línea lleva su marca.
2. **Los comentarios, el lexer los tira** (`//` y `/* */`). Para traducir hay que guardarlos con su posición (en los
   dos lexers) y que los escritores los vuelvan a poner junto al nodo que les sigue. Es lo más delicado del traductor.
   Las líneas en blanco, igual, por la posición.
3. **Los nombres pasan tal cual.** BP no distingue mayúsculas en las palabras clave, pero sí en los nombres, como
   Java y C#.
4. **El `.mod` no depende del texto**: las líneas y la ruta del fuente van al `.dbg`. **Con una excepción**: las clases
   que genera cada `parallel` se llaman `__Par_<línea>_<columna>_<i>` (`SemanticAnalyzer.java:2400`). Hay que
   numerarlas en orden, como ya se hace con `__Thr<n>`. Se gana algo aparte: hoy, añadir un comentario encima de un
   `parallel` cambia el `.mod`.
5. **Los mensajes del semántico son casi todos neutros.** Sólo unos 8 citan sintaxis de BP (`'var owner'`, `'tipo[]'`,
   `'endtry'`).
6. **Los fuentes de los `import` se buscan como `.bp`** (`locateBpSource` y compañía: unos diez sitios en `Main` y
   `BpBuild`). Si también se busca `.jc` y el parser se elige por la extensión, un proyecto puede mezclar módulos BP y
   JC: todo acaba en el mismo AST.

### La red de seguridad: el mismo `.mod`, byte a byte

El traductor da la prueba más fuerte con lo que ya hay: los 399 samples, la stdlib y el corpus de paridad.
1. Compilar el BP da un `.mod` (A); traducirlo a JC y compilar el JC da otro (B). **A y B, idénticos byte a byte.**
2. **Puntos fijos**: BP → JC → BP tiene que dar el BP escrito desde su propio árbol, y JC → BP → JC igual.
3. **Los errores de JC**, con sus pruebas propias.

Si el `.mod` sale idéntico, las dos VMs no se enteran, y ya están probadas con ese `.mod`. Por eso **la VM no cambia**:
el `.dbg` citará el `.jc` y nada más.

### La sintaxis de JC, por decidir (Eduardo)

Una primera tabla, con C# de base (tiene propiedades, eventos, `~Clase()`, `using`, tuplas y `foreach`, como BP) y lo
que se decide marcado con ❓:

| BP | JC (propuesta) |
|---|---|
| `module X` … `end X` | `module X { … }` |
| `import Gui` · `import X from "ruta"` | `import Gui;` · `import X from "ruta";` |
| `var x: integer := 5` | `int x = 5;` ❓ los nombres de los tipos (`int` o `integer`, `bool` o `boolean`) |
| `var owner p: Persona := Persona("a")` | `owner Persona p = Persona("a");` ❓ ¿se admite o se exige `new`? |
| `public function f(a: integer): string` … `end f` | `public string f(int a) { … }` |
| `if c then` … `elseif` … `else` … `endif` | `if (c) { … } else if (c2) { … } else { … }` |
| `while c do` … `endwh` | `while (c) { … }` |
| `do` … `loop c` (se repite mientras `c`) | `do { … } while (c);` |
| `do` … `loop` (sin condición: hasta un `break`) | ❓ necesita su forma: con `while (true)` el árbol ya no sería el mismo y el `.mod` cambiaría |
| `for i := 1 to 10 step 2 do` … `next` | ❓ el `for` de C (sólo la forma que se traduce: `for (int i = 1; i <= 10; i += 2)`) o un rango |
| `for x in lista do` … `next` | `foreach (var x in lista)` ❓ o el de Java, `for (T x : lista)` |
| `switch x` / `case 1, 2` | `switch (x) { case 1, 2: … }` ❓ sin caer al siguiente ni `break` (BP no cae) |
| `try` … `catch e: RuntimeError` … `finally` … `endtry` | `try { … } catch (RuntimeError e) { … } finally { … }` |
| `try var r: T := expr` | `using (T r = expr) { … }` ❓ o el de Java, `try (T r = expr) { … }` |
| `property Nombre: string` con `get`/`set` | `public string Nombre { get { … } set { … } }` |
| eventos y `raise onClick(x)` | `event` y `raise onClick(x);` |
| `function ~Persona()` | `~Persona() { … }` |
| `native function f(…)` · `intrinsic` · `inline` | `native int f(…) { … }` (Java ya tiene `native`); `intrinsic` e `inline`, iguales |
| `parallel` | `parallel { … }` |
| `@BD{ tabla = "x" }` | `@BD(tabla = "x")`, como las anotaciones de Java |
| `print a; b, c` | ❓ el `print` de BP, con sus separadores, no tiene equivalente en C |
| `:=` · `and or not` · `mod` · `shl shr` · `xor` | `=` · `&& \|\| !` · `%` · `<< >>` · `^` |
| `a ^ b` (potencia) | ❓ en C `^` es el XOR: la potencia necesita otra forma |
| `Clase(x)` (conversión) | ❓ el `(Clase) x` de C, o el mismo `Clase(x)` |
| `x: (integer, string)` · `var (a, b) := f()` | `(int, string) x` · `var (a, b) = f();`, como las tuplas de C# |
| fin de sentencia: el salto de línea · bloques con `end` | `;` · llaves |

### Lo que echarán de menos (y no es «sólo frontend»)

Quien venga de Java o C# echará en falta los genéricos, las lambdas, las interfaces de clase, la interpolación de
cadenas, la sobrecarga de operadores y los `static`. JC no los tendrá, porque es BP: añadirlos ya no sería sólo
frontend. El parser de JC los reconocerá y lo dirá con nombre («JC no tiene genéricos…»), en vez de un error de
sintaxis sin más.

### El coste

| pieza | dónde | tamaño (estimado) | trabajo (estimado) |
|---|---|---|---|
| La sintaxis de JC: la tabla, ejemplos escritos a mano y las decisiones ❓ | `V8_IDEAS` | — | 1–2 días, con Eduardo |
| El scanner de JC | frontend | ~700 líneas (el de BP, 699) | 0,5–1 día |
| **El parser de JC**, al mismo AST, con sus mensajes de error | frontend | ~2.000–2.500 (el de BP, 2.022) | 3–5 días |
| Los dos escritores, AST → JC y AST → BP (un recorrido y dos dialectos), con los comentarios y las líneas en blanco | frontend | ~1.200 | 2–3 días |
| La fontanería: el parser según la extensión (unos diez sitios), las órdenes de traducir, los nombres de `parallel` | frontend | ~200 | 0,5–1 día |
| Las pruebas: el `.mod` idéntico en todo el corpus, los puntos fijos y los errores de JC | frontend | ~600 | 1–2 días |
| Los ~8 mensajes del semántico que citan BP | frontend | — | 0,5 días, o más adelante |
| **Compilador** | | **~4.700–5.200 líneas (un 14–15 % más de frontend)** | **~9–15 días** |
| El IDE: `.jc` en unos 13 sitios, el coloreado (RSyntaxTextArea trae el de C# y el de Java; o uno propio como `BpTokenMaker`, de 204 líneas), el plegado por llaves (RSyntaxTextArea lo trae), «Traducir a JC / a BP» en el menú y el lenguaje al crear un proyecto | BpIde | ~400–500 | 1–2 días |
| La VM | — | 0 | 0 |

**Tres semanas como mucho, y el grueso es el parser de JC.** Sobre el proyecto entero —frontend, VMs, firmwares e
IDE— es un aumento pequeño, lo que decía Eduardo: el coste está en lo embebido. Lo que no entra en la cuenta es la
documentación de JC: una página con la tabla y ejemplos, que se pueden sacar del manual con el propio traductor.

**El orden que propuse** (el 17-oct por la mañana): la sintaxis, AST → JC, el parser de JC, AST → BP y el IDE.

### El método (Eduardo, 17-oct)

Eduardo cambia el orden: *«Podemos empezar haciendo un traductor AST->BP. Como tenemos el original del BP podemos
comparar y verificar si el generado es igual que el original.»* Y sobre qué es «igual»: *«no creo que necesitemos
que sea idéntico, nos vale con que sea equivalente (en el equivalente no hace falta respetar todos los saltos de línea
y los comentarios si se mantienen mejor pero si no lo podemos considerar también como válido)»*. Después, *«el AST
-> JC debería seguir el mismo patrón. Y por último un JC -> AST»*. Y por partes: *«no hace falta traducir todo el
lenguaje de golpe, se puede desarrollar de forma incremental tal como ya hicimos cuando programamos el compilador de
BP»*.

El orden queda así:
1. **AST → BP**, comprobado contra el original.
2. **AST → JC**, con el mismo patrón. Antes, la sintaxis de JC: la tabla, con sus ❓.
3. **JC → AST.**
4. **El IDE.**

Cómo se concreta el primer paso:
- **`EscritorBp`**, una clase nueva del compilador que escribe BP desde el árbol, con el formato de siempre (sangría y
  un `end` por bloque). Al principio, sin comentarios. Su esqueleto, el recorrido de `AstPrinter`.
- **«Equivalente», medido de dos formas:**
  - **El árbol**, en cada incremento: el original da el árbol A, se escribe BP', y BP' da el árbol B. A y B, iguales
    sin contar líneas ni columnas (se compara justo tras el parser, antes de que el semántico rellene nada). Si no lo
    son, dice el primer nodo distinto y su línea en el original. Y escribir otra vez BP' tiene que dar BP': el escritor,
    estable.
  - **El `.mod`**, al final, como prueba definitiva: BP' compilado da el mismo `.mod` byte a byte. Antes hay que
    numerar en orden las clases de `parallel`.
- **Por incrementos:**
  - Lo que el escritor aún no sabe escribir lo dice («no soportado: `ForStmt`»), nunca en silencio.
  - Un barrido sobre los 399 samples y la stdlib da tres cuentas: **equivalentes**, **no soportados** (por tipo de
    nodo, que dice qué toca después) y **distintos** (fallos del escritor).
  - Cada incremento sube la primera; se acaba cuando están todos.
- **Lo que ya se sabe que hay que cuidar:**
  - el `import Core` implícito (`implicito`) no se escribe;
  - los cuatro sitios donde el parser fabrica: los grupos de `var`, `isPackExtern`, el `finally` de `try var` y el `if`
    de una línea.
- **Dónde:**
  - dos opciones del compilador, `--escribir-bp` (saca el BP reescrito) y `--comprobar-bp` (la ida y vuelta, con el
    resultado);
  - un script de barrido como `samples_sweep.py`.

  Todo añadido: compilar BP no cambia.
- **El primer incremento:** lo de los samples más sencillos (módulo, `import`, funciones, `var`/`const`, asignaciones,
  expresiones, llamadas, `print`, `if`, `while`, `for`, `return`). Después, lo que diga el barrido.

### Paso 1, AST → BP: hecho el 17-oct, en un solo incremento

Al leer el parser entero para el primer incremento se vio que el árbol está tan pegado a la sintaxis que escribirlo
todo costaba casi lo mismo que una parte. Así que el primer incremento cubre el lenguaje entero, y lo que no supiera
escribir lo habría dicho con nombre.

**Lo hecho:**
- **`EscritorBp.java`** (625 líneas): escribe BP desde el árbol. Reconoce los cuatro sitios donde fabrica el parser,
  no escribe el `import Core` implícito y reproduce los literales con su tipo (`L`, `d`, real con punto).
- **`ComparaAst.java`** (82): compara dos árboles campo a campo, por reflexión y sin las posiciones. Si son distintos,
  dice la ruta del primer campo distinto y su línea en el original.
- **En `Main`**, las dos opciones: `--escribir-bp` y `--comprobar-bp`. La segunda, con una carpeta, la recorre y da
  las cuentas.
- **`EscritorBpTest.java`**, 6 pruebas:
  - la que **tiene que fallar**: un `+` por un `-` sale distinto, en su línea. Sin ella, el barrido verde podría ser
    una comparación que dice siempre «iguales»;
  - la que **no tiene que fallar**: las líneas en blanco y los comentarios no cuentan;
  - y casi todo el lenguaje, los literales, el `import Core` implícito, la `native` externa de un pack y un `module
    interface`.

**El barrido** (`--comprobar-bp`): **469 de 469 equivalentes**, y estables (reescribir lo reescrito da el mismo
texto), sin un solo distinto ni «no soportado».
- `samples`: 427 de 431. Los otros 4 tienen el original roto a propósito (`errores/AnnMal`, `errores/BadSyntax` y dos
  de `holes/`).
- La stdlib, 37 de 37.
- `bpdevices`, 5 de 5.

Las pruebas del frontend, **287 sin un fallo**.

**Lo que no se ha hecho, a propósito:**
- **Los comentarios**: se pierden, y Eduardo dijo que no hacen falta.
- **La prueba del `.mod`**: compilar el BP reescrito y comparar el `.mod` byte a byte. Es más fuerte que la del árbol,
  pero antes hay que numerar en orden las clases de `parallel`, que es un cambio en el compilador de BP.

### La sintaxis de JC, según la decide Eduardo

Lo que va quedando decidido, en el orden en que lo decide. La tabla de arriba, «La sintaxis de JC, por decidir», era
mi propuesta: manda lo de aquí.

1. **Las palabras clave distinguen mayúsculas**, al revés que en BP (17-oct). Los nombres, como en BP: también las
   distinguen.
2. **La estructura general, la de BP** (17-oct):

   ```
   module id
   {
      def 1
      def 2
      ...
      def n
   }
   ```

   `def` es cada definición: constantes, variables, funciones, clases, enums… La llave, en su propia línea, como en el
   ejemplo de Eduardo.
3. **Los `import`, dentro del módulo, como en BP** (17-oct). Eduardo: *«De momento intentamos que sea lo más
   parecido a BP para simplificar el trabajo, sin descartar que en el futuro hagamos pequeños cambios.»*

   ```
   module Blink
   {
      import Gpio;
      ...
   }
   ```

4. **Las sentencias y las declaraciones acaban en `;`**, como en Java y C# (17-oct). Los saltos de línea no cuentan:
   una sentencia puede ocupar varias líneas.
5. **Las variables y las constantes, con el tipo delante**, como en Java y C# (17-oct): `int x = 5;` ·
   `const int MAX = 10;` · `owner Persona p = Persona("a");`.
6. **Los tipos básicos, los de C#, también los estrechos** (17-oct). Un nombre por tipo:

   | BP | JC |
   |---|---|
   | `integer` | `int` |
   | `boolean` | `bool` |
   | `long` · `float` · `double` · `string` · `byte` | los mismos |
   | `int8` | `sbyte` |
   | `word` | `ushort` |
   | `int16` (o `short`) | `short` |

   Al preguntarlo escribí que `word`, `int8` e `int16` no tenían equivalente en C# y se quedaban con su nombre de BP.
   No era así: C# los tiene, y Eduardo elige también para ellos los de C#.
7. **El bloque** (17-oct): `blockstmt ::= '{' { stmt } '}'`.
8. **El `if`, el de C, Java y C#** (17-oct):

   ```
   ifstmt ::= 'if' '(' condicion ')' stmt [ 'else' stmt ]
   ```

   Siempre admite el `else`. En JC no cambia nada que el `if` ocupe una línea o varias; en BP sí, porque el de una
   línea no lo admite. Como en esos lenguajes, el `else` va con el `if` más cercano.

   **Con el árbol de BP** (lo que harán el escritor y el parser de JC):
   - `if (c) s`, sin `else` y sin llaves, es el `if` de una línea de BP (`isSingleLine`). Lo demás es la forma de
     varias líneas: `if (c) x = 1; else x = 2;` vuelve a BP con su `endif`, y el árbol es el mismo.
   - `else if` es el `elseif` de BP: un `else` seguido de un `if` entra en la cadena (`elseIfs`). El `if` que en BP va
     dentro de un `else` sin ser un `elseif` se escribe en JC con llaves: `else { if (…) … }`.
   - El escritor de JC pone las llaves siempre, salvo en el `if` de una línea.
9. **El `while` y el `do … while`, los de C#** (17-oct):

   ```
   whilestmt   ::= 'while' '(' condicion ')' stmt
   dowhilestmt ::= 'do' stmt 'while' '(' condicion ')' ';'
   ```

   **Con el árbol de BP:**
   - `while (c) s`, sin llaves, es el `while` de una línea de BP; con llaves, el de varias líneas, como en el `if`.
   - `do { … } while (c);` es el `do … loop c` de BP, que también se repite mientras `c` se cumpla. El `do` de BP no
     tiene forma de una línea, así que el escritor de JC le pone siempre las llaves.
   - ❓ **El `do … loop` de BP sin condición**, que se repite hasta un `break`, no existe en C#. En el corpus no hay
     ninguno: los tres `do` que hay llevan condición. Mi recomendación: que JC no lo tenga, y que el traductor lo diga,
     con su línea, si un programa de BP lo usa. En JC, el bucle sin fin es `while (true)`.
   - **El `continue` dentro de un `do`:** en C# va a la condición, y en el C del AOT también; en el intérprete de BP
     vuelve arriba sin mirarla. Es un fallo de BP, `#699`. Cuando se arregle, el `do … while` de JC será el de C#.
10. **`break`, `continue` y `return`, sin cambios, como en C#** (17-oct): `break;` · `continue;` · `return;` ·
    `return x;` · `return (a, b);`, la tupla como en C#.
11. **El `for`, el de C, de momento con una asignación, una condición y un incremento** (17-oct):
    `for (asignación; condición; incremento)`.

    **El `for` de BP es un contador** (`emitFor`, `MivmEmitter.java:3609`):
    - declara él su variable, siempre `integer`;
    - el límite es inclusivo y se evalúa una vez, antes de empezar;
    - el paso decide si sube o baja: si es una constante, al compilar; si es una expresión, se evalúa una vez y su signo
      decide al ejecutar;
    - y el `continue` va al paso (`#548`).

    El de C, en cambio, evalúa la condición y el incremento en cada vuelta, y pueden ser cualquier cosa.

    ❓ **Mi propuesta:** que el `for` de JC sea el de BP con la ropa de C, sólo con estas formas.

    | JC | BP |
    |---|---|
    | `for (int i = a; i <= b; i++)` | `for i := a to b do` |
    | `for (int i = a; i <= b; i += 2)` | `for i := a to b step 2 do` |
    | `for (int i = a; i >= b; i--)` | `for i := a to b step -1 do` |
    | `for (int i = a; i >= b; i -= 2)` | `for i := a to b step -2 do` |

    - El paso es un número, y la condición va con él: `<=` si sube, `>=` si baja.
    - Lo demás es un error que lo dice: `i < n`, otra condición u otro incremento, o un paso en una variable. El mensaje:
      «el `for` de JC cuenta como el de BP; para lo demás, `while`».
    - Lo que BP tiene y JC no, de momento: el paso en una variable. Sólo sale en tres samples de prueba (`ForStep`,
      `ContinueFor` y `errores/StepCero`). En el corpus hay 39 `for`: 21 numéricos y 18 `for … in`.
    - El `for x in lista` de BP necesitará su forma; la de C# es `foreach (var x in lista)`.

### Lo que choca entre BP y JC, para más adelante

- **El desempaquetado y el bloque.** En BP, una sentencia que empieza por `{` es el desempaquetado
  (`{ n, s } := par()`); en JC, `{` abre un bloque. La forma de C# es `(n, s) = par();`. Se decide con las
  asignaciones.
- **Los nombres que en el otro lenguaje son palabras reservadas.** Las de BP no distinguen mayúsculas, así que un
  nombre de JC como `Next`, `Step` o `End` es en BP una palabra reservada, y un programa de JC que lo use no se puede
  escribir en BP tal cual. Al revés, `int` y `bool` son nombres válidos en BP que en JC no lo serán. Mi propuesta: que
  el traductor lo diga, con el nombre y la línea, sin renombrar nada por su cuenta.
