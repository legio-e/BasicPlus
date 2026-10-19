# V8 — ideas y diseño

> 🚀 **Desde el 14-oct-2026, V8 es la versión en curso** (V7 se publicó el 13-oct, tag `v7.0`). Éste es el cuaderno de
> diseño de V8, hermano de `V7_IDEAS.md`: las charlas de diseño van aquí **antes** de escribir código. El estado vive en
> `FICHAS.md` §ABIERTAS, bloque V8.
>
> **El modo de V8** (Eduardo, 13-oct): *«V8 es para probar cosas nuevas, para experimentar, sin ponernos obligaciones.
> Probaremos cosas y lo que funcione lo subiremos al final y lo que no funcione se puede aplazar para más adelante.»*
> El tema, decidido el 24-sep: la WiFi, el Bluetooth y el bajo consumo.
>
> **Los hitos** (Eduardo, 18-oct): `H` numerados, como en V7, con sus apartados en `FICHAS.md`, y los nombra Eduardo.
> **H1 = JC** (E2, abajo), cerrado el 18-oct. **H2 = el Bluetooth** (E1, abajo), a medias y parado. **H3 = la WiFi**
> (E3, abajo), en análisis desde el 19-oct.

---

## E1 · El teclado y el ratón Bluetooth en la P4 (`#697`)

> 🔶 **Es el H2 de V8, a medias y parado** (Eduardo, 18-oct: *«de momento no lo tocamos»*). Cuando se retome, lo
> primero es probarlo en la Pico 2, para ver qué se puede hacer común y qué difiere; luego, el módulo de BasicPlus.
> Y una regla que salió con la WiFi (Eduardo, 19-oct): el Bluetooth, como la WiFi, **apagado al arrancar** salvo que el
> Entorno lo active (*«Si no se dice nada, tanto el Bluetooth como el Wifi desactivados»*); §E3, «Lo decidido».

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
2. **La C6 de fábrica lleva un ESP-Hosted viejo**: al anfitrión se le presenta con versión 0.0.0, «BLE only» y «HCI
   over SDIO».
   - No contesta al protocolo de control (RPC V2) del anfitrión 3.x: ni a la versión ni a encender el controlador.
     El anfitrión avisa «major version mismatch — OTA coprocessor from host».
   - **Pero su controlador arranca encendido y el canal HCI funciona.** Con `bring_up_controller = false`, NimBLE se
     engancha y todo va. **Para el Bluetooth no hace falta tocar la C6.**
   - Su propio log, leído el 19-oct por su UART0 (§E3), dice más: ESP-Hosted-MCU 0.0.6 (`network_adapter`,
     `ng-v1.0.2`, IDF 5.4-dev), con **la WiFi** («WLAN over SDIO») y el Bluetooth LE por HCI («BLE only» es sin el
     clásico). Tenía WiFi; lo que no habla es el protocolo de control del anfitrión 3.x.
   - **El 19-oct se actualizó a ESP-Hosted 3.0.9** (§E3, «Paso 1»). La nueva no enciende el controlador sola: lo enciende
     el anfitrión. Con ella, `p4_hid` necesita `bring_up_controller = true`, que es lo de por defecto: basta quitar esa
     línea.
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
   **Hecho el 19-oct**, por el conector P1 (§E3, «Paso 1»); desde entonces el anfitrión tiene que encender el
   controlador (punto 2 de arriba).
8. **El kit P4 EV** lleva, presumiblemente, la misma C6. Falta probarlo.

### Lo que puede seguir (sin orden ni obligación)

- **El ratón en la GUI de la P4**: el anfitrión dentro del firmware de BasicPlus, con el puntero y el cursor. En una
  rama de experimento.
- **El mismo anfitrión en una ESP con Bluetooth propio** (la C5, que tiene pantalla), para ver si el código común
  sirve.
- **La C6 de la P4, actualizada a ESP-Hosted 3.x**: su versión, el protocolo de control y la puerta a la WiFi.
  **Hecho el 19-oct** (§E3, «Paso 1»).
- **La memoria**: cuánto ocupan NimBLE y ESP-Hosted.

---

## E2 · JC: BasicPlus con camisa de Java y C# (Eduardo, 17-oct)

> ✅ **Es el H1 de V8, cerrado el 18-oct** (Eduardo: *«creo que podemos dar este tema por validado. Más adelante en esta
> misma versión 8 podemos hacer algún ajuste adicional»*). Sus diez apartados, con sus commits, en `FICHAS.md`, bloque
> V8, «🧩 H1». Este cuaderno guarda el diseño y lo medido de cada paso, y es la materia de la documentación de JC, que
> va en la final de V8 con las dos opciones de traducción del IDE (Eduardo, 18-oct; `FICHAS`, `D1` de V8).

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

**Lo medido después** (17-oct, por la noche): AST → BP, la sintaxis entera con Eduardo, AST → JC, el parser de JC,
la comparación y el lenguaje entero salieron en un día, frente a los 9–15 días de la tabla. Eduardo: *«al final
parece que no era tan complicado como esperabas, o al menos hemos tardado mucho menos»*. Lo que lo hizo corto: el
árbol de BP es fiel a la sintaxis, y la ida y vuelta sobre todo el corpus cazó cada fallo en el mismo momento. Queda
generar código desde JC (`#700`, el compilador que lea `.jc`, la prueba por la salida) y el IDE.

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
   - **Varios nombres, cada uno con su valor, como en C#** (Eduardo, 17-oct): `int a = 1, b = 2;`. Ojo: en BP,
     `var a, b: integer := 3` da 3 a las dos; en C#, `int a, b = 3;` sólo a `b`. JC es C#.

     **Con el árbol de BP:** cada nombre con valor es su propia declaración (`VarDecl`), como los grupos de BP; sin
     ningún valor, una sola con todos los nombres.

     | JC | BP |
     |---|---|
     | `int a, b;` | `var a, b: integer` |
     | `int a = 1, b = 2;` | `var a: integer := 1, b: integer := 2` |
     | `int a, b = 3;` | `var a: integer, b: integer := 3` |

     **El `var a, b: integer := 3` de BP**, un valor para varios nombres, se escribe en JC repitiendo el valor:
     `int a = 3, b = 3;` (Eduardo; también valdría `int a = 3; int b = 3;`).
     - Es exacto: BP evalúa el valor una vez por nombre, así que `var a, b: integer := f()` también llama dos veces a
       `f`.
     - Medido el 17-oct: las dos formas dan el mismo `.mod`, byte a byte, en una local, una global y un campo de una
       clase.
     - De vuelta a BP salen dos grupos (`var a: integer := 3, b: integer := 3`). La comparación de árboles tendrá que
       contar como iguales un `var` de varios nombres con valor y uno por nombre con el mismo valor.
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
   - **El `do … loop` de BP sin condición**, que se repite hasta un `break`, no existe en C#, y JC no lo tiene
     (Eduardo, conforme). En el corpus no hay ninguno: los tres `do` que hay llevan condición. Si un programa de BP lo
     usa, el traductor lo dice, con su línea. En JC, el bucle sin fin es `while (true)`.
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

    **Decidido** (mi propuesta; Eduardo, conforme): el `for` de JC es el de BP con la ropa de C, sólo con estas formas.

    | JC | BP |
    |---|---|
    | `for (int i = a; i <= b; i++)` | `for i := a to b do` |
    | `for (int i = a; i <= b; i += 2)` | `for i := a to b step 2 do` |
    | `for (int i = a; i >= b; i--)` | `for i := a to b step -1 do` |
    | `for (int i = a; i >= b; i -= 2)` | `for i := a to b step -2 do` |

    - El paso es un número, y la condición va con él: `<=` si sube, `>=` si baja.
    - Lo demás es un error que lo dice: `i < n`, otra condición u otro incremento, o un paso en una variable. El mensaje:
      «el `for` de JC cuenta como el de BP; para lo demás, `while`».
    - `i < n`, el más típico de C, de momento no: se escribe `i <= n - 1`. Admitirlo obligaría al parser a escribir
      él el `n - 1`.
    - Lo que BP tiene y JC no, de momento: el paso en una variable. Sólo sale en tres samples de prueba (`ForStep`,
      `ContinueFor` y `errores/StepCero`). En el corpus hay 39 `for`: 21 numéricos y 18 `for … in`.
    - El `for x in lista` de BP es el `foreach` (16).
12. **`v++` y `v--`** (17-oct). Eduardo: en C# están `v++` y `v--`; en BP, lo más cercano es `v += 1` y `v -= 1`.
    *«Si hace falta se puede añadir un opCode para cada uno y 2 funciones inline para BP: inc(v) y dec(v)»*.

    **Decidido** (mi propuesta; Eduardo, conforme): en JC, `v++;` y `v--;` como sentencias, que son el `v += 1` y el `v -= 1` de BP: el mismo árbol,
    sin nada nuevo en BP ni en la VM. Valen donde vale el `+=` de BP: una variable, un campo o un elemento de un array
    (`a[i]++`).
    - Dentro de una expresión (`x = v++`, `a[i++]`), no: en BP ninguna expresión cambia una variable, y haría falta un
      nodo nuevo en el árbol. `++v`, tampoco: como sentencia es lo mismo que `v++`.
    - El opcode sólo daría velocidad, y eso lo puede hacer el compilador sin tocar el lenguaje: emitir un `INC` para el
      `v += 1`, en BP y en JC por igual. Antes habría que medir si compensa (el PC filtra las ideas de rendimiento).
      Con eso, `inc(v)` y `dec(v)` no hacen falta en BP.
    - En la cabecera del `for`, `i++` sigue siendo lo decidido: el `for` sin `step`.

13. **El `try`, el de BP con los bloques entre llaves** (17-oct). Eduardo: *«El try no cambia, todo igual que en BP
    solamente que el bloque va entre llaves.»* Varios `catch`, un `finally` opcional y el recurso, como en BP.

    **La cabecera del `catch` y el recurso**, con el tipo delante, como en las variables (mi propuesta; Eduardo,
    conforme):

    | BP | JC |
    |---|---|
    | `try` … `catch e: RuntimeError` … `finally` … `endtry` | `try { … } catch (RuntimeError e) { … } finally { … }` |
    | `catch e`, sin tipo (el parser de BP le pone `Exception`, `#459`) | `catch (Exception e) { … }` |
    | `catch`, sin variable (atrapa todo) | `catch { … }` |
    | `try var r: T := e`, el recurso (siempre `owner`) | `try (T r = e) { … }`, como en Java |

14. **El `switch`, pendiente: no entra en la primera versión de JC** (Eduardo, 17-oct: *«es un poco complejo»*). En el
    corpus hay 20 `switch`, en 8 samples; ninguno en la stdlib ni en `bpdevices`. Hasta que tenga su forma, el escritor
    de JC dirá que no lo sabe escribir.
15. **Las asignaciones, las de C#** (17-oct): `x = e;` · `x += e;` · `x -= e;`. El desempaquetado de BP,
    `{ n, s } := par()`, es en JC el de C#: `(n, s) = par();`, con `_` para descartar.
    - Los demás compuestos de C# (`*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`) no están en BP: de momento no, y
      el error lo dice (mi propuesta; Eduardo, conforme). Para tenerlos habría que añadirlos a BP.
    - Como en BP, la asignación es una sentencia: no va dentro de una expresión (`a = b = 0`, `if ((x = f()) > 0)`).
16. **El `foreach`, el de C#** (17-oct): `foreach (var x in lista) stmt`, que es el `for x in lista do` de BP.
    - **El tipo, de momento `var`** (mi propuesta). En BP la variable toma el tipo de los elementos, y el árbol no lo
      guarda. Con un tipo escrito (`foreach (int x in lista)`), de momento, error: habría que guardarlo en el árbol y
      comprobarlo.
    - **De momento recorre un array**, como el de BP (`SemanticAnalyzer`: «se esperaba array en 'for in'»).
    - **Las listas, más adelante.** Eduardo: *«Debería servir para arrays y para listas pero si quieres las listas lo
      dejamos para más adelante.»*
      - Será un cambio de BP, no sólo de JC: el `for … in` de BP también recorrería una `Core.List`, con su `length()`
        y su `get(i)`. Es del compilador; la VM no cambia.
      - Con las listas, el tipo escrito tendrá sentido. Una `List` guarda `Object`, y `foreach (Persona p in lista)`
        convertiría cada elemento, como en C#. Para eso, el árbol tendrá que guardar el tipo, y BP necesitará una forma
        de escribirlo.
    - Sin llaves, es el `for … in` de una línea de BP; con llaves, el de varias, como en el `while`.
17. **El `print`, distinto del de BP** (17-oct). Eduardo: *«Va con paréntesis y ',' , pero la coma no añade un
    tabulador en el print final. El CR y LN no se añaden de forma automática, hay que indicarlos.»* Es decir,
    `print(a, b, "\n");` imprime las piezas una tras otra, sin nada entre ellas y sin salto al final.

    **El `print` de BP, medido el 17-oct** (una sonda en el scratchpad):
    - la coma añade un espacio, no un tabulador: `print "a", "b"` da `a b`;
    - el `;` no añade nada: `print "c"; "d"` da `cd`;
    - el salto (`\n`) lo pone siempre, también con un `;` al final: `print "e";` da `e` y el salto. En BP no hay forma
      de escribir en la consola sin saltar de línea (`IO.write` es para ficheros);
    - una coma al final de la línea sigue el `print` en la línea siguiente.

    **Lo que pide el `print` de JC:**
    - **Un `print` sin salto en BP.** La VM junta las piezas de un `print` y las saca enteras con el salto
      (`PRINT_NL` → `bpvm_linea_fin`, `#600`), para que un `print` salga entero aunque haya varios hilos. Sin el
      salto, las piezas esperan al siguiente. Lo creí cosa de la VM, con una instrucción nueva; luego se vio que basta
      con el compilador (abajo).
    - **La ida y vuelta da otro árbol.** El `print a, b` de BP es en JC `print(a, " ", b, "\n");`, y de vuelta a BP,
      `print a; " "; b; "\n";`. Imprime lo mismo, pero con otro árbol y otro `.mod`. Pasa con los 2.655 `print` del
      corpus, porque todos llevan el salto automático. En el `print`, la comparación tendría que ser por lo que
      imprime: la coma, un `" "`; el salto automático, un `"\n"`.

    **Decidido** (mi propuesta; Eduardo, conforme): el `print` sin salto también para BP. Un `;` al final no salta de
    línea, como en el BASIC clásico (en el corpus no hay ninguno; el manual no dice qué hace). Así, el `print(a, b);`
    de JC es el `print a; b;` de BP. En `FICHAS`, `#700`.

    **Y basta con el compilador** (Eduardo, 17-oct: *«esto mejor lo retocamos en el compilador de BP, pero nada más de
    momento»*). Es el único cambio de BP que entra ahora (abajo, «Los cambios de BP que salen de JC»).
    - Las dos VMs ya guardan las piezas en la línea del hilo y, si el hilo acaba sin el salto, las sacan tal cual
      (`bpvm_linea_vaciar` y `lineaVaciar`, `#600`).
    - Un `print` que acaba en `;` deja sus piezas en la línea. Salen con el siguiente salto, o al acabar el hilo: los
      bytes de la salida son los que tienen que ser.
    - Lo que no da: que un trozo salga en el momento, sin esperar al resto de la línea (un aviso antes de leer por la
      UART, un progreso con puntos). Para eso haría falta una instrucción nueva en la VM: más adelante, si hace falta.

    **Y el `println`** (Eduardo, 18-oct, al salir `#703`): *«añadimos una función println(). La función internamente
    llama a print pero añade un '\n' al final.»* `println(a, b);` es el `print a; b` de BP, con su salto: cierra la
    línea del hilo, y la línea sale en su momento y entera. `println();` es sólo el salto, el `print` vacío de BP. Un
    `"\n"` dentro de un `print` es un carácter más: no cierra la línea, y lo que haya en ella espera al siguiente
    `println` o al final del hilo. El traductor escribe `println(…)` donde BP salta y `print(…)` donde no. `println`
    es palabra reservada de JC, como `print`: se pierde ese nombre, que no usa ningún `.bp` del repositorio.
18. **La potencia, `**`** (Eduardo, 17-oct), y el `^` queda para el XOR, como en C#. Con el árbol de BP: `a ** b` es
    el `a ^ b` de BP, y `a ^ b` es el `a xor b`. La potencia, con su precedencia de BP, que es también la de Python:
    más fuerte que el menos (`-x ** 2` es `-(x ** 2)`) y asociativa por la derecha (`2 ** 3 ** 2` es `2 ** 9`).

    **La precedencia de los operadores, la de C#** (mi recomendación; Eduardo, conforme). La de BP no es la de C# en
    los de bits y en el `not` (`Parser.java`, de `parseNot` a `parsePower`):

    | expresión | en BP | en C# |
    |---|---|---|
    | `a << 1 + 2` | `(a << 1) + 2`: los desplazamientos van con `*` | `a << (1 + 2)` |
    | `a & b == c` | `(a & b) == c`: el `&` va con `*` | `a & (b == c)` |
    | `a \| b + c` | `(a \| b) + c`: el `\|` y el XOR van con `+` | `a \| (b + c)` |
    | `!a == b` | `!(a == b)`: el `not` va por debajo de la comparación | `(!a) == b` |

    La de C# es la que conoce quien viene de C# o de Java. El coste, en el traductor:
    - cada escritor pone los paréntesis que pide la precedencia de su lenguaje;
    - la comparación de árboles no cuenta los paréntesis. El emisor los atraviesa (`MivmEmitter`, `ParenExpr`), así
      que no cambian el `.mod`.

    La otra era la de BP (que es la de Go): no costaba nada, pero sorprendía justo en estos casos.
19. **Las constantes literales, las de C#** (Eduardo, 17-oct). Las declaradas ya eran las de C# (punto 5:
    `const int MAX = 10;`).
    - **Con el árbol de BP**, el cambio está en los reales. En C#, `1.5` es `double` y `1.5f`, `float`; en BP, `1.5`
      es `float` y `1.5d`, `double`. El traductor les cambia la letra, y el árbol es el mismo:

      | JC | BP |
      |---|---|
      | `1.5f` | `1.5` (`float`) |
      | `1.5` · `1.5d` | `1.5d` (`double`) |
      | `10L` · `0xFF` · `0b1010` · `1_000_000` | los mismos |

    - Como en C#, `float x = 1.5;` es un error, y en BP también lo es. Medido: `var x: float := 1.5d` da «valor de
      tipo 'double' no asignable a variable de tipo 'float'». En las placas conviene el `float`, con su `f`: la
      mayoría sólo tienen FPU de precisión simple, o ninguna.
    - **Los caracteres, `'a'` y `'@'`, son constantes enteras** (Eduardo, 17-oct: *«son (o deberían ser) integer en
      BP. En JC los admitimos pero son constantes»*). Su valor es el código del carácter, el mismo que da `charCodeAt`,
      que es el punto de código: `'a'` es 97, y `'é'`, 233. En C# serían un `char`; en JC, como en C, un entero.
      - BP no los tiene hoy: un `'` es «carácter inesperado» (`Lexer`). Añadirlos es sólo el lexer, y a BP también le
        sirven: en el corpus hay 10 comparaciones como `charCodeAt(s, 0) == 48`, que serían `== '0'`.
      - **La forma escrita, una posible mejora** (abajo). El árbol sólo guarda el valor (`IntLitExpr`), así que `'a'`
        vuelve de la traducción como `97`, igual que hoy `0xFF` vuelve como `255`. La mejora: que el literal guarde
        cómo se escribió (decimal, hexadecimal, binario o carácter), que los escritores lo respeten y que la
        comparación no lo cuente. En el código de las placas, una máscara `0xFF00` que vuelve como `65280` se lee peor.
    - Lo que C# tiene y BP no: los sufijos `u` y `m` (BP no tiene tipos sin signo de 32 y 64 bits ni `decimal`), y las
      cadenas `@"…"` y `$"…"`. En JC, de momento, error.
    - **Las secuencias de escape** de C# que BP no tiene (`\u00e9`, `\x41`, `\a`, `\'`…), en las cadenas y en los
      caracteres. BP sólo tiene `\n \t \r \\ \" \0`. Las lee el lexer de JC. Añadirlas
      también a BP, que es sólo el lexer, es una posible mejora (abajo); mientras, si un texto de JC lleva un carácter
      que BP no sabe escribir, el traductor lo dice.
20. **La función** (Eduardo, 17-oct):

    ```
    funcdecl ::= 'function' type id '(' args ')' blockstmt
    ```

    Los parámetros, con el tipo delante y el valor por defecto como en C#: `function int suma(int a, int b = 1) { … }`.

    **Los casos de BP** (`FuncDef`), con mi propuesta (Eduardo, conforme):

    | BP | JC |
    |---|---|
    | `function f()`, sin tipo | `function void f() { … }`, el `void` de C# |
    | `function Medida(v: double := 0.0d)`, el constructor | `function Medida(double v = 0.0) { … }`: sin tipo, como en C# |
    | `function ~Medida()`, el destructor | `function ~Medida() { … }` |
    | `public final native function …` | igual: los modificadores, delante de `function` |
    | `function event pulsado()` | `function event void pulsado() { … }` |
    | `function par(): (integer, string)` | `function (int, string) par() { … }` |
    | sin cuerpo: la `native` externa de un pack, la función de un `module interface` | con `;` en vez del bloque: `public native function int abrir(string n);` |
    | `function Medida.crear()`, la de la clase | `static function Medida crear() { … }`, el `static` de C# |

    Sin tipo, y con un nombre que no es el de la clase, el error lo dice: «falta el tipo; si no devuelve nada,
    `void`».

    **`static`, como en C#** (Eduardo, 17-oct), para todo lo que en BP lleva delante el nombre de su clase: funciones,
    variables, constantes y propiedades (`isStaticDecl`). En BP ese prefijo tiene que ser el de la clase en la que
    está (`SemanticAnalyzer`: «prefijo 'X.' no coincide con la clase»), así que `static` dice lo mismo: el parser de
    JC pone de prefijo el nombre de la clase, y el escritor de JC lo quita. Lo que queda para las clases: en C#, una
    constante de clase no lleva `static`, porque ya lo es.
21. **Las clases, como en C#, en una segunda fase, como el `switch`** (Eduardo, 17-oct: *«las clases como en C# pero
    lo podemos dejar para una segunda fase como el 'switch'»*). Con ellas van sus propiedades, sus eventos (en BP no
    hay eventos de módulo), el constructor y el destructor, y `this`, `super` y `field`.

    **Lo que cubre la primera fase**, medido el 17-oct: de los 473 `.bp` del corpus (samples, stdlib y `bpdevices`),
    163 declaran clases y 8 usan `switch`; 168 tienen una cosa o la otra. Quedan **305 sin ninguna**, que el traductor
    podrá llevar a JC y traer de vuelta. De la stdlib, 15 de 37. Con `parallel` también en la segunda fase (31),
    quedan 302; de la stdlib, los mismos 15.
22. **Los objetos, con `new`** (Eduardo, 17-oct): `List l = new List();`. Hace falta ya en la primera fase: aunque
    no declare clases, un programa crea objetos de la stdlib.
    - **En BP, crear, llamar y convertir se escriben igual**: `List()`, `f()`, `integer(x)`, `Persona(o)`. El árbol es
      el mismo (`CallExpr`), y sólo el análisis semántico sabe qué es cada uno: si el nombre es una clase, construye;
      con un único argumento `Object` o de un antepasado, convierte (`refCasts`, `#389` y L19).
    - **De BP a JC**, el escritor necesita el análisis semántico para saber dónde va el `new`: traducirá un árbol
      analizado, con sus imports, y no sólo leído.
    - **De JC a BP**, `new X()` da el mismo nodo que el `X()` de BP. Para comprobar que el `new` va con una clase, y
      que no se crea un objeto sin él, el árbol tiene que recordar el `new`: una marca que la comparación no cuenta y
      que el análisis comprueba en JC. Es del compilador; BP no cambia.
23. **Las conversiones, como en C#** (Eduardo, 17-oct): `(int) d` es el `integer(d)` de BP, y `(Persona) o`, el
    `Persona(o)` que convierte (`refCasts`). Igual con `(byte)`, `(short)`, `(float)`, `(double)`, `(long)` y
    `(string) o`.
    - **Un paréntesis o una conversión**, con la regla de C#: es una conversión si dentro hay un tipo del lenguaje
      (`int`…), o si tras el `)` viene un nombre, un literal, un `(`, `!`, `~` o una palabra reservada. Así,
      `(a) - b` es una resta; y para convertir `-x` a una clase, `(Persona)(-x)`, como en C#.
    - **Como el `new`, el árbol recuerda la conversión**, con una marca que la comparación no cuenta. El análisis la
      comprueba en JC: `(Persona) 5` es un error, mientras que en BP `Persona(5)` construiría.
    - El escritor de JC, con el árbol analizado: si es una conversión, `(T) x`; si construye, `new T(x)`.
24. **La comprobación de tipo, `is`, como en C#** (Eduardo, 17-oct): `a is Perro` es el `a instanceof Perro` de BP
    (`InstanceOfExpr`), también con el nombre cualificado (`a is Mod.Perro`). Va con las comparaciones, en BP y en
    C#. Lo demás del `is` de C# (`a is Perro p`, `x is null`) BP no lo tiene: error, de momento; para `null`,
    `x == null`.
25. **Los `enum`, como en C#** (Eduardo, 17-oct): `public enum Color { Rojo, Verde = 5, Azul }`, con una coma al
    final si se quiere. Es el `enum … end` de BP, miembro a miembro (`EnumDef`).
    - El valor, como en BP, un entero escrito y sin signo (`Parser.parseEnumValue`). Las expresiones que admite C#
      (`-1`, `1 << 3`, `Rojo + 1`), de momento no, y el tipo de debajo (`enum Color : byte`), tampoco.
    - JC es BP: un `enum` no se convierte a entero ni al revés (`#522`), aunque en C# sí: `(int) Color.Rojo` da
      error.
26. **La función de una instrucción, `inline`** (Eduardo, 17-oct):

    ```
    'function' type id '(' args ')' 'inline' instrucción ';'
    ```

    `public function int charCodeAt(string s, int i) inline CALL_BUILTIN 48;` es la función de BP cuyo cuerpo es
    `inline CALL_BUILTIN 48` (`InlineStmt`): el mnemónico de `OPCODES.md` y sus operandos enteros.
    - Medido el 17-oct: en el corpus hay 509 funciones con `inline`, en 19 ficheros (casi todas en la stdlib), y en
      todas es la única línea del cuerpo. Esta forma las cubre todas.
    - Un `inline` en medio de otras sentencias, que BP admite, no tiene forma en JC: el traductor lo dice. En el
      corpus no hay ninguno.
27. **Las anotaciones, las de BP** (Eduardo, 17-oct: *«no recuerdo como eran en C#, pero si no las de BP ya están
    bien»*): `@BD{ tabla = "medidas", clave }`, delante de la declaración, como en BP; el mismo árbol, sin traducir
    nada.
    - En C# son atributos entre corchetes (`[Table("medidas")]`, `[Key]`), y en Java, `@Table(name = "medidas")`.
      Ninguno admite el `clave` suelto de BP.
    - La de BP no choca con nada en JC: delante de una declaración, esa llave no puede ser un bloque.
28. **`throw`, como en C#** (Eduardo, 17-oct): `throw new RuntimeError("…");` es el `throw RuntimeError("…")` de BP
    (`ThrowStmt`). El `throw;` de C#, sin nada, que relanza dentro de un `catch`, BP no lo tiene (`parseThrowStmt`
    pide una expresión): de momento, error, y se escribe `throw e;`.
29. **Las constantes, como en C#, también las locales** (Eduardo, 17-oct): `const int MAX = 10;`.
    - **El tipo, obligatorio, como en C#.** En BP se puede omitir y lo deduce del valor (`const WARN := 2`): de las 100
      constantes del corpus, 22 no lo llevan.
    - El escritor de JC les pone el tipo que deduce el compilador (el árbol analizado, como para el `new`). De vuelta
      a BP llevan el tipo: otro árbol, que la comparación tendrá que dar por igual.
    - Medido el 17-oct: con el tipo escrito o sin él, el mismo `.mod`, byte a byte (de módulo y local; `integer`,
      `float`, `string` y `long`).
30. **Los `import`, como en BP** (Eduardo, 17-oct; ya en el punto 3), con su `;`: `import Gpio;` ·
    `import X from "ruta";` · `import v1 from pack "SQLI";` · `import a.b : Impl;`. La cabecera del módulo, también
    como en BP: `library "x";` delante, y `module X implements Y`, `module interface X extends Y`.
31. **`parallel`, a la segunda fase** (Eduardo, 17-oct), con las clases y el `switch`. En el corpus hay 3. Hasta
    entonces, el escritor de JC dirá que no lo sabe escribir.
32. **Las propiedades de módulo** (las de clase van con las clases). Las 9 del corpus son de la forma corta, y van
    como la propiedad automática de C# (mi propuesta; Eduardo, conforme):

    | BP | JC |
    |---|---|
    | `public property counter: integer` | `public int counter { get; set; }` |
    | `public property abiertos: integer := 0` | `public int abiertos { get; set; } = 0;` |
    | `public sync property tick: integer` | `public sync int tick { get; set; }` |

    - Los modificadores de BP que C# no tiene (`sync`, `owner`, `final`) van delante, como palabras de JC.
    - La forma larga (`get … endget`, `set(v) … endset`) va con las clases. En C# el `set` no nombra su parámetro
      (es `value`), y eso habrá que decidirlo.
33. **Los arrays de tamaño fijo, como en C** (Eduardo, 17-oct; decidido tras el paso 2): el tamaño detrás del nombre.
    El de C es el mismo que el de BP: tamaño fijo, en el marco o dentro del objeto, y no se reasigna. El `new int[10]`
    de C# y de Java es otro, una referencia a un array del montón, como el `integer[]` de BP.

    | BP | JC |
    |---|---|
    | `var nums: integer[]` (en el montón) | `int[] nums;` |
    | `var nums: integer[10]` (fijo) | `int nums[10];` |
    | `var b: byte[4] := [1, 2, 250]` | `byte b[4] = [1, 2, 250];` |
    | `var x, y: integer[3]` | `int x[3], y[3];` |

    El valor, con los corchetes de JC (los de BP), no con las llaves de C.
34. **Las clases, como en C#** (Eduardo, 17-oct, tras el paso 3: *«antes de pasar a la generación de código quizás
    deberíamos completar lo que nos falta de JC para soportar el lenguaje completo»*). Con las cuatro decisiones de
    sus preguntas (las cuatro, mi recomendación):

    | BP | JC |
    |---|---|
    | `public class Perro extends Animal` … `end Perro` | `public class Perro : Animal { … }` |
    | `var nombre: string`, sin marca: protegido | `protected string nombre;` — JC escribe el `protected` de C# |
    | `private var x: integer` | `private int x;` |
    | `var Perro.cuantos: integer` | `protected static int cuantos;` |
    | `public const Limits.MAX: integer := 100` | `public const int MAX = 100;` (en C# ya es de la clase) |
    | el constructor, con `super(n)` o `Perro(n, 0)` de primera sentencia | `function Perro(string n) : base(n) { … }`, y `: this(n, 0)` |
    | `function ~Perro()` | `function ~Perro() { … }` |
    | `super.hablar()` | `base.hablar()`, el `base` de C# |
    | `property Valor: double` con `get`/`set(v)` | `double Valor { get { … } set(v) { … } }`; con `set(value)`, el `set { … }` de C# |
    | `event cambia(m: Medida)` · `raise cambia(this)` | `event cambia(Medida m);` · `raise cambia(this);` |
    | `b.onClick := this::pulsado` · `Thread(a::suma(7))` | `b.onClick = this::pulsado;` · `new Thread(a::suma(7))` |
    | `this`, `field`, `root class` | iguales |

    - En el corpus, el `set` se llama `v` 28 veces de 33, y `value`, 2: por eso las dos formas.
    - BP exige el `super(…)` y la delegación como primera sentencia del constructor; por eso son el `: base(…)` y
      el `: this(…)` de C#, que van en el mismo sitio.
    - `base` y `protected` son ahora palabras reservadas de JC. En el corpus, 5 ficheros llaman `base` a un parámetro
      (`LocalsDbg`, `SaltoCrudo`, `SyncListDiag`, `synclisttest` y `sqlite/Orm.bp` de la stdlib): el traductor lo
      dice, sin renombrar.
35. **El `switch`, la forma de C# con el significado de BP** (mi propuesta; Eduardo no la discutió): `switch (x) {
    case 1, 2: … default: … }`. No cae de un caso al siguiente y no hace falta `break`; si se escribe, sale del
    `switch`, como en BP.
36. **`parallel`, con la misma forma**: `parallel { case t1 = new Thread(4096): … default: … }`.

### La primera fase, en un ejemplo

Con esto, la sintaxis de la primera fase está entera (17-oct). El mismo programa en BP y en JC, escrito a mano con lo
decidido; el JC es lo que tendrá que escribir el traductor:

```
module Ejemplo
  import Collections

  public const MAX: integer := 10
  const PI := 3.5
  var cuenta: integer := 0
  public property nivel: integer := 1

  public enum Color
    Rojo
    Verde := 5
  end Color

  function par(): (integer, string)
    return (1, "uno")
  end par

  public function sumar(a: integer, b: integer := 1): integer
    return a + b
  end sumar

  public function codigo(s: string, i: integer): integer
    inline CALL_BUILTIN 48
  end codigo

  public function main()
    var n: integer
    var s: string
    var l: List := List()
    { n, s } := par()
    if n > 0 and not (s == "") then print "si"
    for i := 1 to MAX step 2 do
      cuenta += i
    next
    for x in [1, 2, 3] do print x
    do
      n -= 1
    loop n > 0
    try
      throw RuntimeError("fallo")
    catch e: RuntimeError
      print e.msg
    endtry
    print "cuenta: "; cuenta; " pi: "; PI
  end main
end Ejemplo
```

```
module Ejemplo
{
   import Collections;

   public const int MAX = 10;
   const float PI = 3.5f;
   int cuenta = 0;
   public int nivel { get; set; } = 1;

   public enum Color { Rojo, Verde = 5 }

   function (int, string) par()
   {
      return (1, "uno");
   }

   public function int sumar(int a, int b = 1)
   {
      return a + b;
   }

   public function int codigo(string s, int i) inline CALL_BUILTIN 48;

   public function void main()
   {
      int n;
      string s;
      List l = new List();
      (n, s) = par();
      if (n > 0 && !(s == "")) println("si");
      for (int i = 1; i <= MAX; i += 2)
      {
         cuenta += i;
      }
      foreach (var x in [1, 2, 3]) println(x);
      do
      {
         n -= 1;
      } while (n > 0);
      try
      {
         throw new RuntimeError("fallo");
      }
      catch (RuntimeError e)
      {
         println(e.msg);
      }
      println("cuenta: ", cuenta, " pi: ", PI);
   }
}
```

Lo que el ejemplo enseña de la traducción:
- el `PI` sin tipo de BP sale en JC con el que se deduce (`float`), y su `3.5` con la `f`;
- el salto que BP pone solo sale en JC escrito, `"\n"`;
- y el `List()` que construye sale con su `new`. Las dos últimas cosas el escritor de JC las sabe por el árbol
  analizado, no por el texto.

### Los cambios de BP que salen de JC: posibles mejoras, todavía no

Eduardo, 17-oct: *«De momento todos los cambios de BP los dejamos como posibles mejoras pero prefiero no
implementarlos todavía para no mezclar demasiados cambios.»* La primera versión de JC es sólo frontend, sobre el BP de
hoy, con una excepción: el `print` sin salto (`#700`), que se retoca en el compilador de BP. Lo que haría falta en BP,
y lo que hace JC mientras:

| el cambio en BP | sale de | mientras, en JC |
|---|---|---|
| `#699`, el `continue` de un `do … loop`: un fallo | 9 | el `continue` de su `do … while` es el de BP: vuelve arriba sin mirar la condición |
| `#700`, el `print` sin salto: **éste sí, en el compilador** (Eduardo); hecho el 18-oct | 17 | el `print(a, b);` es el `print a; b;` de BP |
| los caracteres, `'a'` | 19 | los lee el lexer de JC; en BP se escriben como su número |
| las secuencias de escape de C# | 19 | las lee el lexer de JC; lo que BP no sabe escribir, el traductor lo dice |
| que un literal guarde cómo se escribió | 19 | `0xFF` y `'a'` vuelven como `255` y `97` |
| el `foreach` sobre listas | 16 | sólo arrays |
| los demás compuestos (`*=`, `/=`…) | 15 | no están |

**El `print`, la excepción.** Mi propuesta para mientras era que un `print` de JC acabara con su salto escrito, y un
error si no. Eduardo: *«No, esto mejor lo retocamos en el compilador de BP, pero nada más de momento.»* Así que `#700`
se hace en el compilador de BP, sin tocar la VM (punto 17), y los demás cambios, todavía no.

### Lo que choca entre BP y JC, para más adelante

- ~~**El desempaquetado y el bloque.**~~ En BP, una sentencia que empieza por `{` es el desempaquetado; en JC, `{` abre
  un bloque. Resuelto con las asignaciones (15): en JC, `(n, s) = par();`.
- **Los nombres que en el otro lenguaje son palabras reservadas.** Las de BP no distinguen mayúsculas, así que un
  nombre de JC como `Next`, `Step` o `End` es en BP una palabra reservada, y un programa de JC que lo use no se puede
  escribir en BP tal cual. Al revés, `int` y `bool` son nombres válidos en BP que en JC no lo serán. Mi propuesta: que
  el traductor lo diga, con el nombre y la línea, sin renombrar nada por su cuenta.
- **Un `if`, `while` o `for` de una línea al final de un bloque de su clase** (`#701`, un fallo del parser de BP): BP
  no lo lee, porque toma el `endif`, `endwh` o `next` de la línea siguiente por suyo. De JC a BP saldría esa forma
  (`for (…) { for (…) x; }`). Importa en el paso 3.

### Paso 2, AST → JC: hecho el 17-oct

Eduardo: *«OK, adelante.»* Con la sintaxis de la primera fase decidida, el escritor de JC.

**Lo hecho:**
- **`EscritorJc.java`** (721 líneas). Escribe JC desde el árbol ANALIZADO: es lo que sabe dónde va el `new`, qué
  llamada es una conversión `(T) x` y qué tipo lleva una constante que en BP no lo escribe. Pone los paréntesis que
  pide la precedencia de C# y deja los del programador. Lo que la primera fase deja fuera lo dice con su motivo y su
  línea.
- **`Main.analizarParaTraducir`**: el análisis con los mismos pasos que `compileFull` antes de emitir (las interfaces
  de los imports, los implícitos y el análisis), sin emitir ni sacar ruido por la salida.
- **`--escribir-jc`**, con un fichero (el JC por la salida) o con una carpeta (las cuentas).
- **`EscritorJcTest`**, 7 pruebas:
  - la primera lee de este documento el ejemplo de la primera fase y exige ese JC carácter a carácter, porque se
    escribió a mano antes que el escritor. Con la sangría saboteada fallan ella y la del `if`;
  - y en pequeño, la precedencia, crear, llamar y convertir, las formas del `for` y del `if`, el `print` y lo que no
    se escribe.

  El frontend, 294 pruebas sin fallos.

**El barrido** (`--escribir-jc` sobre los 473 `.bp`):

| | samples | stdlib | `bpdevices` |
|---|---|---|---|
| escritos | 268 | 15 | 0 |
| no soportados | 130 | 22 | 5 |
| no se analizan | 33 | 0 | 0 |

- Los no soportados, por su motivo: 148 por clases, 4 `switch`, 2 `parallel`, 2 `for` con el paso en una variable
  (`ForStep` y `ContinueFor`, que lo prueban) y una propiedad con `get` y `set` escritos.
- Los que no se analizan: 26 están rotos a propósito (`errores/` y `holes/`), y 7 piden el pack de SQLite. Esos 7
  tampoco compilan solos con el compilador: el análisis para traducir se comporta como él.

**Lo que no está decidido y el escritor ha hecho a su manera:**
- **Los arrays de tamaño fijo** (`var nums: integer[10]`), 22 en el corpus, en 4 ficheros. El escritor los puso
  primero como `int[10] nums;`; Eduardo preguntó cómo era en C, y quedaron como en C: `int nums[10];` (punto 33).
- Las líneas en blanco: juntas las declaraciones de una línea (constantes, variables y propiedades cortas), y
  separado lo demás, como en el ejemplo.

**Lo siguiente, el paso 3:** el parser de JC, con la ida y vuelta: el árbol de BP, escrito en JC y leído otra vez,
tiene que ser el mismo (sin contar los paréntesis ni las marcas del `new` y de las conversiones); y al final, el
`.mod`.

### Paso 3, JC → AST: hecho el 17-oct

Eduardo: *«Probemos el paso 3. En principio el escáner y el parser deberían ser similares a los de BP con las
diferencias de cada lenguaje, pero puede servir de punto de partida.»* Así se hizo: el de BP, de modelo, y el árbol,
el suyo, nodo a nodo.

**Lo hecho:**
- **`JcLexer.java`** (288 líneas): las palabras reservadas distinguen mayúsculas; los literales de C# (`1.5` double,
  `1.5f` float, `'a'` un entero); los escapes de C#; y los comentarios `//` y `/* … */`.
- **`JcParser.java`** (1.125 líneas): de JC al MISMO árbol que el parser de BP. Fabrica lo mismo que él en sus tres
  sitios: el `import Core` implícito al final, el cuerpo que lanza de una `native` externa de un pack, y el
  `ReleaseOwnerStmt` que cierra el `finally` de un recurso. Lo que la primera fase deja fuera es un error con su
  motivo.
- **Las marcas `conNew` y `esConversion`** en `CallExpr`: las pone el parser de JC; el de BP las deja a false.
- **`ComparaAst`, el modo JC**: no cuenta lo acordado (los paréntesis, la mayúscula de `MOD`, el `var` de varios
  nombres con valor, el tipo de una constante que BP no escribe, las marcas), y compara el `print` por lo que imprime.
- **`--comprobar-jc`**: el árbol del original, recién leído, frente al que sale de escribirlo en JC y leerlo con
  `JcParser`.

**Dos cosas que salieron al hacerlo:**
- **El análisis cambia el árbol**: añade a cada llamada, al final, los valores por defecto de los parámetros que
  faltan (H8.1, `checkArgs`). El escritor de JC escribía también esos. Ahora se cuentan antes del análisis los
  argumentos de cada llamada (`EscritorJc.contarArgumentos`) y se escriben sólo los del programador.
- **BP admite variables locales de tipo tupla** (`var t: (string, integer) := pair()`, en `T3`), y no sólo tuplas
  como tipo de retorno. El parser de JC las lee: `(string, int) t = pair();`.

**El barrido** (`--comprobar-jc`): **283 de 283 EQUIVALENTE**, todos los que se escriben (268 samples y 15 de la
stdlib), sin un DISTINTO.
- La prueba de que mira: con `<<` y `>>` cambiados en el parser de JC, 7 DISTINTO, los 7 samples escritos que usan
  desplazamientos, cada uno con su línea.
- `JcParserTest`, 5 pruebas: la que tiene que fallar (un `-` por un `+`), la de lo que no cuenta, la ida y vuelta del
  ejemplo, los nodos que fabrica el parser y los errores con nombre. El frontend, 300 pruebas sin fallos.

**Lo que no está hecho:**
- El análisis no comprueba todavía las marcas: que el `new` vaya con una clase, y la conversión, con una conversión.
- El compilador no lee `.jc`: falta que elija el parser por la extensión.
- La prueba por lo que imprime, compilando BP y JC y comparando la salida en las dos VMs, pide antes `#700`. Hoy el
  `print` de JC compilado saltaría de línea dos veces, porque BP aún salta con el `;` al final. Y el `print` da otro
  `.mod`, así que la comparación del `print` es por la salida.

**Lo siguiente:** `#700` en el compilador de BP, que el compilador lea `.jc`, y la prueba por la salida.

### El lenguaje entero: las clases, el `switch` y `parallel`, hechos el 17-oct

Eduardo: completar JC antes de generar código. La sintaxis, en los puntos 34 a 36; el escritor y el parser, ampliados.

**El barrido** (`--comprobar-jc`, los 473 `.bp`):

| | samples | stdlib | `bpdevices` |
|---|---|---|---|
| EQUIVALENTE | 392 | 36 | 5 |
| no soportados | 6 | 1 | 0 |
| no compilan | 33 | 0 | 0 |

- **433 de los 440 que compilan hacen la ida y vuelta.** Los 7 que no, por lo acordado: 2 `for` con el paso en una
  variable (`ForStep` y `ContinueFor`, que lo prueban) y 5 con un nombre `base`.
- La prueba de que mira también aquí: con el nombre del `set(v)` ignorado en el parser de JC, 8 DISTINTO, con su
  línea.
- Una prueba con casi todo lo de las clases (`EscritorJcTest.clasesSwitchYParallel`). El frontend, 301 pruebas sin
  fallos.

**Al escribir esa prueba salió un fallo de BP, `#702`:** un `raise` dentro del `set` de una propiedad no compila. El
análisis lo da por bueno, y el emisor dice «Campo no encontrado en Animal: cambia$dest». Es de BP, no de JC, y no se
toca (los cambios de BP, más adelante).

**Lo siguiente:** generar código desde JC. Es decir: `#700` en el compilador de BP, que el compilador lea `.jc`, la
prueba por la salida en las dos VMs, y que el análisis compruebe las marcas.

### Generar código desde JC: el 18-oct

Eduardo, 18-oct: *«continuamos JC»*. Por este orden: `#700`, que el compilador lea `.jc`, la prueba por la salida y
que el análisis compruebe las marcas.

**`#700`, el `print` sin salto, en el compilador de BP** (`152fcd9b`). Sólo el emisor y el AOT. El corpus da los mismos
`.mod` byte a byte, y `PrintSinSalto` lo prueba en las dos VMs.

**El compilador lee `.jc`.** El árbol sale de `JcParser`, y lo demás es lo de un `.bp`:
- `parseAndPrint` elige el parser por la extensión. Un error de JC sale como los de BP (`[l:c] error léxico: …` o
  `error sintáctico: …`), y el build falla;
- un proyecto encuentra su main en `<main>.jc` si no hay `<main>.bp`; con los dos, lo dice y no compila. `sources`
  admite `.jc`;
- el AOT (`AotMain`) lee `.jc`, y `--escribir-bp` traduce un `.jc` a BP;
- el `.dbg` apunta al `.jc`, con sus líneas: el depurador no necesita nada.

Lo que falta: que un módulo importe a otro escrito en `.jc`. Los imports buscan el `.bp` junto al importador.

**El análisis comprueba las marcas** (`SemanticAnalyzer.revisaJc`). JC escribe lo que BP deduce de los tipos, y el
análisis, que sabe qué es cada llamada, mira que lo escrito case, con la misma regla con la que lo escribe
`EscritorJc`:
- una construcción lleva `new`; una conversión se escribe `(T) x`; una llamada, ninguna de las dos;
- la delegación en otro constructor (`#511`) es el `: this(…)` de la cabecera, sin `new`. Un `new MiClase(…)` como
  primera sentencia de su constructor es un error, porque BP lo tomaría por delegar;
- los mensajes dicen cómo se escribe en JC: «`'integer(x)' es una conversión: en JC se escribe '(int) x'`». Los demás
  mensajes del análisis siguen con los nombres de BP.

**La prueba por la salida, `compat.sh check-jc`.** Cada sample del corpus de la paridad, además, se escribe en JC, se
compila desde el `.jc` y se ejecuta en las dos VMs: tiene que imprimir lo mismo que el de BP y acabar con el mismo
código. El `.mod` no es el mismo, porque el `print` de JC es otro árbol. Da 130 PASS, 17 FAIL y 3 SKIP (lo acordado).

**Los 17 son `#703`:** en JC el salto es un trozo de cadena, `"\n"`, y la VM sólo cierra la línea del hilo con el salto
del `print` de BP. Lo que imprime un programa JC sale al acabar el hilo: medido, un bucle que imprime cada 300 ms saca
sus cinco líneas juntas al final. Con un prototipo en el emisor (el último `"\n"` literal es el salto), `check-jc` da
147 PASS. Las tres maneras de arreglarlo y lo que cuesta cada una, en la ficha. Eduardo eligió otra: el `println`
(punto 17), el `print` de BP con su salto. Con él, `check-jc` da 147 PASS, sin tocar la VM ni el emisor.

**Las otras redes:**
- el corpus BP, compilado otra vez: los mismos 441 `.mod`. Salió `#704`, que no es de JC: el orden de las clases de
  tupla depende de los `hashCode` de identidad de Java;
- el corpus compilado desde JC: compila todo lo que se escribe. Salió que el cuerpo que fabrica el parser de JC para
  las `native` de un pack no llevaba su `new`, y quedó arreglado;
- la ida y vuelta de `bpgenvm-c/samples`, que faltaba: 254 EQUIVALENTE y 7 no soportados (6 con `base`, 1 con `int`).

### El IDE para JC: lo que toca (el mapa del 18-oct)

- **El compilador corre dentro del IDE** (llamadas directas a `basicplus.frontend.*`), y el IDE se construye contra el
  jar del frontend que hay en `~/.m2`. Para que entre JC: `mvn install` en `lexer-java` y `mvn package` en `BpIde`,
  con el IDE cerrado.
- **Unos quince sitios dan por hecho `.bp`**, casi todos en `FrmMain`:
  - el filtro de abrir y el de guardar; «Guardar como» añade `.bp`;
  - el árbol del proyecto, y «Nuevo proyecto», que siempre hace BP;
  - en modo proyecto, Depurar, Ejecutar en ventana y Depurar en la placa compilan `<main>.bp` a mano. Ejecutar y
    Ejecutar en la placa ya van por `buildProject`, que encuentra el `.jc`;
  - `ProjectDialog` («Añadir .bp…») y `AotBuild.listBpFiles`.
- **El coloreado y el plegado:**
  - hoy hay un solo estilo, `text/bp`, para todo editor: `BpTokenMaker` propio y `BpFoldParser`, que pliega por palabras
    (`end`, `endif`…);
  - para JC haría falta un `JcTokenMaker`, con la lista de palabras de `JcLexer`, `/* … */` entre líneas, `'c'` y los
    escapes; el plegado, el `CurlyFoldParser` que trae RSyntaxTextArea; y el estilo, por la extensión del fichero.
- **El depurador:** los puntos de parada van por el nombre del fichero y la línea, y el `.dbg` dice el `.jc`. Nada que
  tocar.
- **Los errores:** el IDE los lee con una expresión que ya casa con los de JC.
- **Traducir:**
  - no hay menú de herramientas, y el de Editar está vacío;
  - hoy la traducción no se puede llamar desde el IDE: `--escribir-jc` y `--escribir-bp` acaban en `System.exit` y
    escriben por el `stdout` crudo. Hace falta una entrada pública que devuelva el texto, o el error, sin salir;
  - por decidir con Eduardo: qué hace «traducir», si un buffer nuevo sin guardar o un fichero al lado. Y un proyecto
    con `X.bp` y `X.jc` a la vez no compila.

**Hecho el 18-oct.** Eduardo: la traducción, en una pestaña nueva sin guardar.
- Los `.jc` se abren, se guardan y salen en el árbol del proyecto. Al guardar sin extensión, el IDE pone la del
  lenguaje del editor.
- El coloreado (`JcTokenMaker`) y el plegado por llaves (`CurlyFoldParser`) van por la extensión del fichero.
- Las órdenes de ejecutar y depurar compilan el `<main>.jc`; con los dos, lo dicen antes de tocar nada.
- Edit → «Traducir a JC» y «Traducir a BP», con `Traductor`, la entrada pública del frontend.
- El depurador, sin tocar.
- Medido sin la ventana: lo nuevo está dentro del jar del IDE, y una sonda tokeniza JC con el `JcTokenMaker`. La prueba
  con la ventana, de Eduardo.

---

## E3 · La WiFi (H3; Eduardo, 19-oct)

> El análisis, antes de programar. Lo que hay salió de tres mapas de sólo lectura del 19-oct (`Net` en las dos VMs, el
> firmware de cada familia y las convenciones de BP), con lo que dicen comprobado donde importa.

### Lo que pide Eduardo

Las funciones (19-oct): *«Encender el wifi, Hacer una lista de redes wifi disponibles. Poder escoger una y conectarse.
Poder proporcionar la clave. Disponer de la conexión TCP/IP. Cerrar la conexión wifi. Apagar el wifi.»* Y además:
- **al conectarse, dos maneras** (*«IP dinámica e IP Fija. Cada una con sus propias opciones»*):
  - la dinámica (DHCP): el router da la IP, la máscara, la puerta de enlace y los DNS. Lo propio es el nombre con que la
    placa se presenta en la red, y que el programa pueda leer lo que le han dado;
  - la fija: la IP, la máscara, la puerta de enlace y los DNS, uno o dos. Sin DHCP no llegan DNS solos: si no se dan,
    no se resuelven nombres;
- **fuera, de momento:** la WiFi punto a punto (*«la conexión wifi se hace entre 2 dispositivos sin Access Point»*; el
  ESP-IDF no tiene el modo ad hoc, IBSS) y ESP-NOW;
- **el uso más inmediato:** *«conectar con el IDE a través de la wifi»*.

### Lo que hay (medido el 19-oct)

**`Net`, el cliente TCP** (`#241`, de V2):
- `tcpConnect`, `tcpSend`, `tcpRecv` y `tcpClose` (los builtins 131 a 134) y la clase `Net.Tcp`. En el PC, en las dos
  VMs: miVM con `java.net.Socket`, y la VM-C con la fachada `bpvm_net.h` y `net_host.c` (Winsock2 o POSIX, 16
  conexiones). En las placas no hay quien la atienda: «Net: sin red en esta plataforma».
- **Bloquea el intérprete entero**: mientras conecta o espera datos, se paran todos los hilos BP del run (en miVM, del
  worker). Es lo que ya decía el bloque V8: los builtins que esperan tendrían que ceder el hilo BP, como `sleep`.
- No hay: servidor (`listen`/`accept`), UDP, una consulta de DNS aparte, ni la IP propia.
- **Lo que salió de paso, `#705`:** `tcpClose` no deja en la pila el valor que el compilador tira tras cada llamada
  sin resultado. Dos `Net.tcpClose(0)` sueltos cambian las dos variables locales de antes, de 7 y 9 a 32 y 264, en las
  dos VMs igual. Y además: los números de conexión no son los mismos en las dos VMs, la VM-C admite 16 y miVM sin
  límite, y nadie cierra las conexiones al acabar el programa.

**Las placas** (el ESP-IDF es el 6.0.1; la WiFi y lwIP están habilitados en su configuración, pero no se enlazan:
nadie los pide):

| placa | la radio | RAM libre en reposo | la partición de la aplicación |
|---|---|---|---|
| S3 | WiFi 2,4 GHz y BLE | holgada: la VM está en la PSRAM y quedan 297 KB internos | 1 MB, ~409 KB libres |
| C3 | WiFi 2,4 GHz y BLE | **108 KB, el bloque mayor de 86 KB: la más justa** | 1 MB, ~409 KB libres |
| C5 | WiFi 6 a 2,4 y 5 GHz, BLE | holgada: PSRAM, y 248 KB internos | 1 MB, ~345 KB libres |
| C6 | WiFi 6 a 2,4 GHz, BLE | **80 KB**, con el bloque del pack de la GUI (136 KB) en la interna | 1,5 MB, **~877 KB libres** |
| P4 | ninguna: va por su C6, con ESP-Hosted | holgada | 6 MB |
| Pico 2 W | el CYW43439 | justa: sin PSRAM, y 24 KB de montón del FreeRTOS | — |
| Metro, Nucleo, DK2 | ninguna | — | — |

- **Lo que suele costar** (la cifra general del IDF, sin medir aquí): 50 a 70 KB de RAM interna y varios cientos de KB
  de flash.
  - En la S3, la C3 y la C5 probablemente no cabe en la partición de 1 MB. Agrandarla mueve el Entorno y el sistema de
    ficheros, que se pierden, una vez.
  - En la C3 no cabe sin recortar la VM o las opciones que dejan código de la WiFi en la IRAM. En la C6, sin el bloque
    del pack de la GUI, o con uno menor.
- **La NVS se borra en cada actualización**: la imagen fusionada pisa de 0x9000 a 0xFFFF. La calibración de la radio se
  rehace sola; la red y su clave no pueden guardarse ahí.
- **La P4**: la C6 de fábrica lleva la WiFi (ESP-Hosted-MCU 0.0.6, «WLAN over SDIO»), pero no contestó al protocolo
  de control del anfitrión 3.x (ni a la versión ni a encender el controlador, 14-oct), y la WiFi del anfitrión va por
  ese protocolo: hay que actualizarla a ESP-Hosted 3.x. Y la SD y la C6 se disputan el único controlador SDMMC del
  IDF 6: hace falta un dueño. (Corregido el 19-oct: puse aquí que la de fábrica sólo hacía Bluetooth; su log dice que
  no. La actualización, hecha ese día: abajo, «Paso 1».)
- **La Pico 2 W**: según `H14_TEST_PLAN.md` (junio), la Pico 2 de pruebas de Eduardo es una W (su LED cuelga del
  CYW43439). Por confirmar. El SDK trae el driver y lwIP, pero esos submódulos están vacíos en el repositorio.
- **Lo que no hay en BP:** una capacidad «tiene WiFi» en `Machine`, ni una forma de que la plataforma avise a un
  programa (los eventos sólo los dispara `raise`; la GUI lo resuelve con un hilo BP que pregunta).

**Lo que ya existe y sirve:**
- **El cable por TCP.** El IDE habla con la VM Java por TCP con el mismo protocolo (`BpvmClient.connectRemote`). Y la
  P4 tiene un servidor del cable por TCP, `esp32p4/main/wire_v1_tcp.c`, para su Ethernet, que está apagado por defecto.
- **`net_host.c`** es casi lo que hace falta encima de lwIP, que da la misma API de sockets.
- **El diseño de mayo y junio** (`WIFI_TCP_REFLECTION.md` y `HECHO_V2.md` §H11, para la Pico 2 W):
  - la red en `/sys/wifi.json` (`{ssid, password, country}`), escrita desde el IDE por el puerto serie;
  - conectarse al arrancar sin bloquear la comunicación con el IDE;
  - el mismo protocolo del cable por TCP, como otra vía, no en lugar del cable;
  - el descubrimiento por difusión UDP primero, mDNS si hiciera falta;
  - la hora por NTP;
  - el orden: la WiFi al arrancar, `Net` sobre lwIP, y el cable por WiFi.

### La conexión WiFi y el TCP/IP, separados (Eduardo, 19-oct)

Eduardo: *«el wifi es una capa de transporte del stack TCP/IP, creo que deberíamos separar lo que es la conexión wifi
del resto de los protocolos TCP/IP. Así limitamos el alcance de cada cosa.»* Es también como lo parte el ESP-IDF: el
driver de la WiFi (`esp_wifi`) por un lado, y la interfaz de red (`esp_netif`) con lwIP por el otro. Y ya estaba
apuntado en V3 (`#145`): «desacoplar TCP/IP de WiFi».

**La conexión WiFi: el enlace, y nada más.**
- encender y apagar la radio;
- buscar redes: el nombre, la señal, el canal y la seguridad;
- conectarse a una con su clave, y desconectarse;
- el estado del enlace (apagada, desconectada, conectando, conectada, o el fallo: la red no aparece, la clave no vale)
  y la señal de la red a la que está conectada;
- el país, que fija los canales que se pueden usar.

**El TCP/IP: lo que va encima, sea cual sea el enlace.**
- la configuración IP de la interfaz: dinámica (DHCP, con el nombre de la placa) o fija (IP, máscara, puerta de enlace
  y DNS), y leer la que tiene;
- resolver nombres (DNS);
- TCP: el cliente, que ya existe (`Net.Tcp`), y lo que venga después;
- el IDE por la red: el cable por TCP.

**Entre las dos pasa una sola cosa:** el enlace avisa de que está arriba o abajo. Con el enlace arriba, el TCP/IP pide
su IP por DHCP o pone la fija, y la red está lista cuando tiene IP.

**Lo que da separarlas:**
- el mismo TCP/IP sirve para otro enlace: la Ethernet de la P4, que ya está en su build (`esp_eth`), u otro un día;
- en el PC, la WiFi es simulada y el TCP/IP es el del sistema operativo, cada uno con lo suyo y sin mezclarse;
- el IDE por la red no depende de la WiFi, sino del TCP/IP: por la WiFi, o por la Ethernet de la P4;
- cada parte se mide y se prueba por separado.

### Lo decidido (Eduardo, 19-oct)

- **La placa: la P4 de Waveshare primero** (la de la C6 que ya conocemos del Bluetooth); la C3, la C5 y la C6,
  después. Para la P4, el primer paso técnico es que hable por WiFi a través de su C6: actualizarla a ESP-Hosted 3.x
  (la de fábrica lleva la WiFi, pero no habla el protocolo de control del anfitrión 3.x; hecho el 19-oct) y resolver
  que la SD y la C6 comparten el controlador SDMMC.
- **Los ajustes de la conexión, en un fichero JSON en `/sys`.** Un programa BP puede escribirlo (el Entorno no: no hay
  `Machine.setEnv`), y el IDE no lo enseña como enseña el Entorno, que importa por la clave.
- **El Entorno, sólo para activar al arrancar:** *«En el environment quizás únicamente si queremos activar/desactivar al
  arrancar. Si no se dice nada, tanto el Bluetooth como el Wifi desactivados.»* Una clave para la WiFi y otra para el
  Bluetooth, apagadas si no están.
- **Cómo se configura:**
  - en cualquier micro, preconfigurada: el fichero se escribe desde el IDE (*«En un micro la podemos pre-configurar»*);
  - en la P4, que tiene pantalla, con un programa BP interactivo: buscar, escoger, teclear la clave y elegir la IP. Es
    la idea del programa de configuración del Bluetooth (§E1), y pide pronto el módulo `Wifi` y la configuración IP de
    `Net`.
- **El controlador SDMMC es del sistema** (decidido después de la prueba del paso 1): *«El controlador del SDMMC es
  del SO, no del BasicPlus. Si la wifi o el bluetooth están apagados el lector de SD debe estar encendido.»* Lo crea
  el arranque, y la SD y la C6 usan cada una su ranura. Comprobado en la P4 el mismo día (abajo, «Paso 1»).
- **Manda el ahorro de energía** (el mismo día): *«Aquí lo que prima (o debería primar) es el ahorro de energía. Así
  que el estado normal tanto del wifi como el bluetooth es apagado, y se enciende cuando se necesita, y si hay 10 min
  de inactividad se apaga salvo que se indique lo contrario.»* En la P4, apagado del todo es la WiFi desinicializada,
  el enlace con la C6 cerrado y la C6 en reset; comprobado (abajo, «Paso 1»). Por concretar en el paso 2: qué cuenta
  como actividad (un programa que la usa, una conexión abierta, el IDE conectado).
- **El apagado por inactividad se configura en el fichero de configuración**, y depende de la alimentación (Eduardo,
  el mismo día): *«El apagado es configurable via fichero de configuración. Si el SO puede detectar si tenemos
  alimentación externa, entonces solamente se apaga al cabo de un tiempo, con alimentación externa no hace falta que
  se apague.»* Es decir: a pilas, se apaga a los 10 minutos sin actividad; con alimentación externa, no. Cómo sabe
  cada placa de dónde come está por ver: no todas pueden saberlo.
- **El IDF 6.1, para toda la familia ESP32** (el mismo día): *«Actualizamos al 6.1 para toda la familia ESP32.»* La
  S3, la C3, la C5, la C6 y la P4, con una sola versión del IDF. Es `#707`.
- **La frontera:** *«la conexión wifi llegamos hasta que se establece la conexión, el resto es ya del TCP/IP»*.
- **El TCP/IP no se empieza de cero:** *«la conexión TCP/IP ya estaba, la probamos en su día con el P4 Kit»*, por la
  Ethernet. Antes de diseñar esa parte, se repasa lo que se hizo entonces.

### El TCP/IP que ya hubo: la Ethernet de la P4 Kit (V3, junio)

Repasado el 19-oct (Eduardo: *«quizás convendría repasar lo que se hizo en su día»*), por la historia del repositorio:
- **Lo que se hizo, del 22 al 30 de junio** (V3, la fase de la P4):
  - la Ethernet de la P4 Kit (el EMAC interno y su PHY, el IP101), con IP fija, 192.168.2.2, y un cable cruzado al PC
    (192.168.2.1) (`bdbf81ec`);
  - un log por TCP: la P4 se conecta al PC, al 5555, y allí lo lee `pc_logserver.py`. Fue la red de seguridad para
    poner en marcha el cable por UART;
  - **el servidor del cable por TCP**, `wire_v1_tcp.c`, en el 3333, con el mismo REPL que el puerto serie
    (`1b6b7f0a`, el diseño en `V3_IDEAS.md`).
- **Lo que funcionó:** el IDE se conectaba con «VM Java (TCP v1)», escribiendo `192.168.2.2:3333`, y ejecutaba
  programas: el «Hola mundo», el INFO y las demos de la GUI. **No se probó:** depurar por TCP (el depurador de las ESP
  llegó en julio, ya por UART), el DHCP (por el cable cruzado no había quien diera IP), ni `Net` para los programas, que
  nunca se conectó a lwIP.
- **Por qué quedó apagado:** la P4 de Waveshare no tiene Ethernet, y con el cable por UART una sola imagen sirve a las
  dos P4 (`82e7c659`, 30-jun); el log por la red hacía esperar 5 s en cada arranque (`65ef07b0`, agosto).
- **Cómo está hoy:** fuera de la imagen por defecto. Las piezas del IDF siguen en el build y en el IDF 6.0.1 existe
  todo lo que usa; se compiló por última vez el 1-oct, y no hay grabación por TCP desde el 30 de junio. La orden de
  compilación que documenta, `idf.py -DBPVM_P4_WIRE=tcp build`, ya no funciona: se para en un `#error`.
- **Lo que sirve tal cual para la WiFi:** `wire_v1_tcp.c` no depende del enlace (escucha en cualquier interfaz y usa
  sólo sockets); la puesta en marcha de `esp_netif` y de los eventos, la IP fija, y el aviso de «enlace arriba», que es
  justo lo que la WiFi le pasará al TCP/IP. Lo único de la Ethernet es el EMAC, su PHY y sus pines: el enlace.
- **Lo que habría que arreglar para la WiFi:**
  - con TCP, el arranque espera al enlace para siempre;
  - un solo cliente, y si su conexión muere (la WiFi que se cae), el servidor no vuelve a aceptar al IDE hasta
    reiniciar la placa;
  - la dirección y los puertos, escritos en el código;
  - cada imagen lleva el cable por UART o por TCP, nunca los dos; V8 lo quiere como otra vía, no en lugar del cable;
  - no hay autenticación (el protocolo dice «un cliente a la vez, sin autenticar»);
  - `wire_v1_tcp.c` vive en la carpeta de la P4, no en el común de las ESP;
  - en el IDE, la placa por la red usa el backend de la VM Java: no sincroniza la hora, no da el log ni la memoria del
    INFO, y la dirección se escribe a mano.

### Lo que hay que decidir (Eduardo)

**De la conexión WiFi:**
1. ~~Dónde guarda la placa su red~~: **decidido**, un JSON en `/sys` (arriba). Lo que lleva: la red, la clave y el
   país. Por concretar al diseñarlo: el nombre del fichero, si la IP de la interfaz va en él (un apartado `ip`) o en el
   suyo, y los nombres de las dos claves del Entorno.
2. ~~Cómo se configura la primera vez~~: **decidido** (arriba): preconfigurada desde el IDE, y en la P4 con un programa.
3. **La conexión WiFi del sistema y la de un programa.** La placa tiene una radio. Propuesta, con lo decidido:
   - con la WiFi activada en el Entorno, es del sistema: se conecta al arrancar con lo del fichero, y un programa la
     consulta (el estado, la señal) pero no la apaga ni la cambia; si lo intenta, una excepción lo dice;
   - sin activar (lo de siempre), es del programa: la enciende, se conecta y la apaga, y al acabar el programa se apaga
     sola (`#668`). El programa de configuración de la P4 es uno de éstos.
4. **El módulo de BP de la WiFi.** Un esbozo para hablarlo, con los nombres en el estilo de la stdlib:

   ```
   import Wifi
   var radio: Wifi.Radio := Wifi.Radio()
   radio.on()                                         // encender
   var redes: Wifi.Network[] := radio.scan(5000)       // la lista: nombre, señal (dBm), canal, seguridad
   if radio.connect("MiRed", "la clave", 15000) then  // escoger una, con su clave
     print "conectada a", radio.ssid, "con", radio.rssi, "dBm"
     radio.disconnect()                               // cerrar la conexión
   endif
   radio.off()                                        // apagar; y al destruirse, también
   ```

   - Buscar y conectar tardan segundos. Para no parar los demás hilos BP, el módulo arranca la operación, pregunta y
     espera con `sleep`, como la GUI con `__guiRunOnce`: sin tocar la VM.
   - Perder la red: de momento, preguntar (`isConnected()`); un evento, más adelante.
5. **Qué hace en el PC.** Propuesta: una WiFi simulada, la misma en las dos VMs, con su rastro `[wifi] …` como los
   periféricos simulados: unas redes de mentira fijas (una abierta y una con clave) y conectarse a ellas con su clave.
6. ~~La placa de referencia~~: **decidido**, la P4 de Waveshare (arriba). Lo que se midió de las demás sirve para
   después: la C6 tiene sitio en su partición (877 KB libres); la C3 y la C5, no; y la C3 y la C6 van justas de RAM.

**Del TCP/IP:**
7. **La configuración IP**, en `Net`, que ya es el módulo del TCP/IP. Un esbozo:

   ```
   import Net
   Net.setDhcp("sensor-cocina")                       // IP dinámica (la de siempre): el nombre en la red
   // o IP fija: Net.setStaticIp("192.168.1.50", "255.255.255.0", "192.168.1.1", "192.168.1.1", "")
   if Net.waitReady(10000) then                       // la red, lista cuando hay IP
     print "IP", Net.ip(), "puerta", Net.gateway(), "DNS", Net.dns()
     // … Net.Tcp, como hoy
   endif
   ```

   - En la placa, la guardada en el sistema (en el JSON de `/sys`: en el mismo fichero o en el suyo, por concretar) se
     aplica sola al arrancar.
   - En la P4 Kit la IP era fija y estaba escrita en el código; el DHCP no se llegó a probar. Con la WiFi de casa, lo
     normal será el DHCP.
   - En el PC el TCP/IP es el del sistema operativo, y un programa no puede cambiarlo. Lo que digan `ip()` y los demás
     en el PC está por decidir: la IP de verdad, o unos valores simulados que den lo mismo en las dos VMs.
8. **Cómo encuentra el IDE a la placa.** Con DHCP, la IP la da el router y puede cambiar. Propuesta, por pasos: la
   placa dice su IP por el puerto serie (el INFO, que hoy no la lleva) y en su pantalla si la tiene, y el IDE la
   recuerda; después, el anuncio por difusión UDP; mDNS sólo si hace falta.
9. ~~Quién puede conectarse por la red~~: **decidido de momento** (Eduardo, 19-oct): *«¿Podemos aceptar conexiones
   entrantes de cualquier ordenador? Eso es más un tema TCP/IP pero de momento sí. Posteriormente hay que estudiar
   cómo hacerlo seguro.»* La placa atiende a cualquier ordenador de la red, sin clave, como el protocolo hoy. Queda
   apuntado para más adelante: cualquiera de la red puede mandar y ejecutar programas en la placa, y lo que va por el
   cable TCP viaja en claro. Una clave del IDE guardada en la placa sería un primer paso.

**Y el alcance:** ¿un hito para cada parte, o las dos en el H3?

### Un orden posible, por pasos que se comprueban

1. **La WiFi de la P4, a mano.** Actualizar su C6 a ESP-Hosted 3.x, por OTA desde la P4 o por el conector P1, con el
   OK de Eduardo para grabarla. Y un firmware de prueba en la P4, sin la VM, como el paso 1 del Bluetooth: encender,
   buscar, conectarse, desconectar y apagar, con la SD montada a la vez por el controlador compartido. Lo que cuesta
   de verdad, en flash y en RAM. **Hecho el 19-oct** (abajo, «Paso 1»): las tres vueltas sin un fallo, con la SD a
   la vez, apagando con `esp_wifi_stop` por `#706`.
2. **La conexión WiFi.** Primero la del sistema: la fachada (`bpvm_wifi.h`, sólo el enlace), el backend del IDF (en la
   P4, por ESP-Hosted), el JSON de `/sys` y la clave del Entorno para activarla al arrancar. Luego la de los programas:
   el módulo `Wifi`, su versión simulada en las dos VMs, con la paridad, y el programa de configuración de la P4.
3. **El TCP/IP**, desde lo que se hizo con la Ethernet de la P4 Kit (arriba). Primero el del sistema:
   - la IP dinámica o fija, desde el JSON de `/sys`;
   - el cable por TCP **a la vez** que por UART;
   - que el servidor vuelva a aceptar al IDE si la conexión muere, y que el arranque no espere al enlace;
   - `wire_v1_tcp.c` al común de las ESP, para la C3, la C5 y la C6;
   - en el IDE, la placa por la red como un backend de placa (la hora, el log, la memoria del INFO), no el de la VM
     Java.

   Luego el de los programas: `Net` sobre lwIP, con `#705` y el cierre de las conexiones al acabar el programa.
4. **Las demás placas:** la C3, la C5 y la C6.

**Fuera**, salvo que Eduardo diga otra cosa: el servidor para los programas (`listen`/`accept`), UDP, TLS, la web de
configuración, la placa como punto de acceso y la hora por NTP (pequeña: lwIP la trae).

### Paso 1: la WiFi de la P4, a mano (desde el 19-oct)

Como el Bluetooth (§E1), con firmwares de prueba del ESP-IDF, sin BasicPlus. El código está fuera del repo, en
`C:\tmp\v8wifi`.

**La C6, actualizada a ESP-Hosted 3.0.9** (19-oct, con el OK de Eduardo: *«puedes actualizar el C6 de la P4»*).
- **Por el conector P1:** la UART0 de la C6 va a un adaptador USB-serie (un FTDI FT232R, en COM18) y su IO9, a masa
  con un puente. Con el RESET de la placa, la ROM de la C6 entra en el modo de grabación. Hicieron falta unos intentos
  con el cableado: primero no llegaba nada por COM18, y luego arrancaba de la flash, hasta que el IO9 quedó a masa.
- **La copia de fábrica, antes de tocar nada:** la flash entera, 4 MB, leída con su comprobación, en
  `C:\tmp\v8wifi\c6_fabrica_4MB.bin` (SHA-256 `b47bbb8e…`). Lleva el cargador, la tabla de particiones (dos `ota` de
  1,5 MB) y la app de fábrica en `ota_0`. Con ella la C6 puede volver a como venía.
- **Lo grabado:** el ejemplo de coprocesador de `esp_hosted` 3.0.9 con la WiFi y el Bluetooth
  (`eh_cp_bt_wifi_hosted_hci_mcu`). Se compiló con el IDF 6.0.1 en `C:\tmp\v8wifi\c6_cp`, con la misma copia de
  `esp_hosted` que el anfitrión de la P4.
  - El SDIO va en el modo STREAM, el mismo de la de fábrica. El de agregación pide parchear el IDF compartido, y el de
    paquetes se para en un `#error`.
  - La tabla de particiones es la del ejemplo para 4 MB: dos `ota` de 1792 KB, para poder actualizarla después desde
    la P4. La app ocupa 1374 KB.
  - Se borró la flash entera, para no dejar la NVS de la de fábrica, y después se comprobaron los cuatro trozos contra
    el build.
- **Arranca:** «ESP-Hosted Firmware version :: EH-3.0.9», con la WiFi y el Bluetooth LE por SDIO. El anfitrión de la
  P4, que todavía es el `p4_hid` del Bluetooth, se entiende con ella: negocian el protocolo de control (RPC V2).
- **Lo que cambia:**
  - **El controlador Bluetooth ya no se enciende solo**, como en la de fábrica: lo enciende el anfitrión cuando lo pide.
    La WiFi no la encendía sola ninguna de las dos. Encaja con lo decidido: sin decir nada, apagados. Para el H2,
    `p4_hid` necesita `bring_up_controller = true` (§E1, punto 2).
  - Las siguientes actualizaciones de la C6 pueden ir por OTA desde la P4: ya habla el protocolo y tiene las dos
    particiones. Está sin probar.
- **Para el usuario, apuntado:** cada P4 de Waveshare necesitará esta actualización de su C6 para la WiFi. Aquí se ha
  hecho por el P1, con un adaptador USB-serie y un puente. No se ha probado si una C6 de fábrica acepta una OTA de un
  anfitrión 3.x; con la copia de fábrica se puede probar. Cómo se le da al usuario, por decidir.

**El firmware de prueba en la P4** (19-oct, con el OK de Eduardo para grabarlo: sólo la app, en `0x10000`).
- **Con el IDF 6.1**, que Eduardo instaló ese día con el instalador de Espressif (EIM) junto al 6.0.1. El 6.0.1 sigue
  siendo el de los firmwares de BasicPlus. El 6.1 trae dentro la WiFi remota de la P4 (`esp_wifi/remote`); con el
  6.0.1 hacía falta bajar el componente `esp_wifi_remote`.
  - El entorno se carga con un script propio con las variables del de EIM, sin su `eim select v6.1` final, que
    cambiaría el IDF elegido para los IDE.
  - Las herramientas del 6.1 son las mismas del 6.0.1 (gcc `esp-15.2.0`, cmake 4.0.3); lo nuevo es su Python.
- **El programa**, en `C:\tmp\v8wifi\p4_wifi`:
  - primero la C6 (`esp_hosted` 3.0.9, la misma copia que la C6), luego la SD;
  - después, tres vueltas de encender, buscar, conectarse con la clave, esperar la IP, desconectarse y apagar;
  - entre paso y paso escribe y relee 256 KB en la SD y anota la memoria libre;
  - la red y su clave, en un fichero aparte del proyecto que escribió Eduardo; no se copia a ningún sitio.
- **Lo medido, en las tres vueltas sin un fallo:**

  | paso | tiempo |
  |---|---|
  | el enlace con la C6, con su reinicio por el GPIO 54 | 1,8 s |
  | iniciar (una vez: `esp_wifi_init` y el modo) | 153 ms |
  | encender (`esp_wifi_start`) | 81 ms la primera vez; 21 ms las siguientes |
  | buscar | 5 o 6 redes en 2,5 s |
  | conectarse y tener IP por DHCP (WPA2, canal 1, −59 a −67 dBm) | 3,7 a 3,9 s |
  | desconectarse | 37 ms |
  | apagar (`esp_wifi_stop`) | 4 ms |

  - **La SD, en la otra ranura del mismo controlador**, montada en 42–81 ms. Sus 13 comprobaciones (antes de la WiFi,
    con ella encendida, conectada y apagada) dieron los mismos datos y los mismos tiempos: 256 KB escritos en
    ~150 ms y leídos en 124 ms, a 20 MHz y un bit.
  - **La RAM interna de la P4 casi no se mueve:** 0,3 KB menos con la WiFi encendida, 6,5 KB de pico, y vuelve a lo
    de antes en cada vuelta. La PSRAM no cambia. El trabajo de la radio lo hace la C6. Lo que se reserva antes, medido
    en la tercera prueba: ~34 KB al abrir el enlace con la C6 y ~4,6 KB la interfaz de red.
  - **La flash:** la parte de red (lwIP, la interfaz de red y el anfitrión de `esp_hosted` con su protocolo) ocupa unos
    252 KB y 5,4 KB de RAM fija. La app entera, 639 KB.
  - Tras desconectarse, el IDF guarda la IP 120 s y luego avisa de que se perdió (`LOST_IP`).
- **Lo que sale para el diseño:**
  - ⚠️ **`esp_hosted` 3.0.9, en la C6, no deja volver a iniciar la WiFi tras desinicializarla** (`#706`). La primera
    prueba apagaba con `esp_wifi_deinit`, y en la segunda vuelta todo contestaba `ESP_ERR_WIFI_NOT_INIT`. La C6
    guarda una bandera de «ya iniciada» que su desinicialización no baja. Apagar con `esp_wifi_stop` apaga la radio y
    no tiene el problema. Para decidir: en BasicPlus, apagar = parar la radio; o arreglar esa línea en el firmware de
    la C6 que se le dé al usuario.
  - **El controlador SDMMC, un solo dueño.** `esp_hosted` lo crea al conectar con la C6 y, si su `sdmmc_host_init()`
    falla, no conecta. Al soltarlo, sólo suelta su ranura. Hoy nuestro driver de la SD de la P4 (`blk_sdmmc_p4.c`)
    también lo crea. En las dos primeras pruebas iba primero la C6 y la SD se montaba sin crearlo.

    **Eduardo lo decidió ese mismo día:** el controlador es del sistema, y la SD funciona con la WiFi y el Bluetooth
    apagados. **La tercera prueba lo hace así**, y salen las tres vueltas sin un fallo:
    1. El arranque crea el controlador.
    2. La SD se monta y se prueba con la C6 sin arrancar.
    3. Se abre el enlace con la C6. `esp_hosted` ya no arranca solo antes de `app_main`
       (`ESP_HOSTED_AUTO_CALL_INIT_BEFORE_APP_MAIN=n`), y su `sdmmc_host_init()` encuentra el controlador hecho. Lo
       hace un envoltorio del enlazador, `-Wl,--wrap=sdmmc_host_init`, sin tocar el código de `esp_hosted`; el ELF
       confirma que su llamada va al envoltorio.

    La SD, 14 de 14 iguales: antes de la C6, con ella y en cada fase de la WiFi. **Abrir el enlace con la C6 cuesta
    ~34 KB de RAM interna** de la P4, y el mayor bloque libre baja 24 KB. Con la WiFi y el Bluetooth apagados no se
    gastan.

    **El envoltorio sobra con el IDF 6.1** (la quinta prueba, sin él). El IDF cambió entre las dos versiones:
    - en el **6.0.1**, el de BasicPlus, `sdmmc_host_init()` crea otro controlador en cada llamada, y el segundo falla
      (lo que se midió en `#566`); y `sdmmc_host_deinit_slot()`, al soltar la última ranura, borra el controlador y
      deja su asa colgando;
    - en el **6.1**, `sdmmc_host_init()` devuelve el que haya («already created, returning existing handle»), y
      `sdmmc_host_deinit_slot()` suelta la ranura y borra el controlador sólo si no queda otra, y lo anota.

    Así que con el 6.1 la regla sale sola: **cada uno pide el controlador con `sdmmc_host_init()` y suelta sólo su
    ranura; nadie suelta el controlador entero.** La quinta prueba lo pasa todo con los valores por defecto del IDF:
    - la SD y `esp_hosted` reciben el controlador que hay;
    - con la SD desmontada, al apagar la WiFi el IDF borra el controlador; `esp_hosted` lo vuelve a crear en la vuelta
      siguiente, y la SD, al montarse otra vez;
    - tres vueltas sin un fallo.

    Queda una línea de error del IDF, sin daño: al cerrar el enlace con la SD montada, sale «host controller with slot
    registered». Es su intento de borrar el controlador, que no se borra porque la SD lo usa.

    Para BasicPlus queda:
    - el controlador lo pide cada uno, y el driver de la SD inicia y suelta sólo su ranura. Hoy, en sus salidas de
      error, `blk_sdmmc_p4.c` suelta el controlador entero (`#566`), y eso cortaría el enlace con la C6;
    - **la WiFi de la P4 pide el IDF 6.1:** trae la WiFi remota dentro y comparte el controlador sin más. Con el 6.0.1
      harían falta el componente `esp_wifi_remote` y un envoltorio que además vigilara cuándo borra el IDF el
      controlador. Es una decisión de Eduardo: pasar el firmware de la P4 al 6.1, o todas las ESP.
  - **Apagado del todo, por el ahorro de energía** (Eduardo, arriba, «Lo decidido»): la cuarta y la quinta prueba
    encienden y apagan del todo en cada vuelta:
    - encender: abrir el enlace con la C6, que arranca de cero, y arrancar la WiFi, en 2,1 s; el enlace solo, 1,9 s,
      que son sobre todo los 1,5 s que espera `esp_hosted` a que la C6 arranque;
    - apagar: parar y desinicializar la WiFi, cerrar el enlace (`esp_hosted_deinit`) y dejar la C6 en reset (su
      `CHIP_PU`, el GPIO 54, a bajo), en 125 ms;
    - la RAM interna vuelve a su sitio en cada vuelta: ~35 KB al encender, devueltos al apagar. Sólo se quedan 344 B
      la primera vez, y no crecen.

    Tres vueltas sin un fallo, en cada prueba. Como la C6 arranca de cero cada vez, **`#706` no aparece**: su bandera
    está baja. Sólo saldría si la WiFi se desinicializa con la C6 encendida por el Bluetooth; ahí basta con parar la
    radio.
  - **Un fallo del enlace con la C6 reinicia la P4** con un `abort()`. Lo hace `esp_hosted` por defecto
    (`TRANSPORT_RESTART_ON_FAILURE`). En la prueba se apagó y se escucha su evento; en BasicPlus tendrá que ir igual:
    que la WiFi se caiga no tumba la placa.
  - Al arrancar, `esp_hosted` sólo registra los manejadores de eventos de la WiFi: no enciende la radio. Encaja con
    «sin decir nada, apagada».
- **De paso, la P4:** desde el 14-oct lleva el cargador y la tabla de particiones de las pruebas del Bluetooth (la
  `factory` de 1,5 MB del IDF), no los de BasicPlus. Para volver a BasicPlus hay que grabar la imagen fusionada entera.
  El Entorno y el sistema de ficheros deberían seguir donde estaban: ninguna prueba llega a esas direcciones.

**Lo siguiente:** el paso 2, la conexión WiFi del sistema, con las decisiones que quedan (arriba, «Lo que hay que
decidir») y las de este paso:
- ~~el IDF 6.1 para la P4~~: **decidido**, para toda la familia ESP32 (`#707`);
- qué cuenta como actividad para apagar a los 10 minutos; ~~dónde se dice que no se apague~~: **decidido**, en el
  fichero de configuración, y con alimentación externa no se apaga;
- cerrar `#706` porque el apagado del todo lo evita, o arreglar la C6.
