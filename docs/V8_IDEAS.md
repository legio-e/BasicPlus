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
> (E3, abajo), en análisis desde el 19-oct. **H4 = el sonido en la P4** (E4, abajo): el `.wav` suena desde el 19-oct;
> parado. **H5 = los strings** (E5, abajo): sus operaciones, de la más lenta a la más rápida, y las que viven en la
> PSRAM; en análisis desde el 21-oct.

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
- **De paso, la P4:** del 14 al 19-oct llevó el cargador y la tabla de particiones de las pruebas del Bluetooth (la
  `factory` de 1,5 MB del IDF), no los de BasicPlus. **El 19-oct por la noche volvió a BasicPlus**: la imagen
  fusionada entera, ya con el IDF 6.1 (`#707`), y con su Entorno, su sistema de ficheros y sus packs donde estaban.

**Lo siguiente:** el paso 2, la conexión WiFi del sistema, con las decisiones que quedan (arriba, «Lo que hay que
decidir») y las de este paso:
- ~~el IDF 6.1 para la P4~~: **decidido**, para toda la familia ESP32 (`#707`);
- qué cuenta como actividad para apagar a los 10 minutos; ~~dónde se dice que no se apague~~: **decidido**, en el
  fichero de configuración, y con alimentación externa no se apaga;
- cerrar `#706` porque el apagado del todo lo evita, o arreglar la C6.

---

## E4 · El sonido en la P4: WAV primero, MP3 después (H4; Eduardo, 19-oct; `#708`)

> **Es el H4 de V8** (Eduardo, 19-oct, tras oírlo: *«El sonido lo ponemos como hito 4. Podemos dejar aquí lo del sonido
> de momento.»*). Parado, con el paso 1 hecho.

Eduardo, al acabar la sesión de la WiFi: *«¿Cuánto costaría reproducir un .mp3 en el P4?»*. Y tras el análisis:
*«La placa tiene una salida speaker y la placa venía con unos pequeños altavoces. Y sí, podemos empezar con .wav.»*

**Lo que hay en la placa** (el esquema de Waveshare y su ejemplo `06_I2SCodec`):
- el códec ES8311 (convierte el audio digital en señal para el altavoz, y la del micrófono en digital), el amplificador
  NS4150B y la salida del altavoz, con unos altavoces pequeños que venían con la placa; y el ES7210, para los
  micrófonos;
- el ES8311 se configura por I2C, en el bus de la táctil (SDA GPIO 7, SCL GPIO 8). Ese bus ya está declarado como
  compartido justo para esto (`#669`, Eduardo: *«hay placas donde ese I2c se utiliza también para controlar otros
  dispositivos»*);
- el I2S: MCLK GPIO 13, BCLK 12, WS 10, la salida al códec 9 y la entrada 11. El amplificador se enciende con el
  GPIO 53. Por lo que se ve, la imagen de BasicPlus no usa ninguno; se confirma en la prueba;
- el ejemplo de Waveshare reproduce PCM crudo (16 kHz) por ese camino, sin decodificar nada.

**Lo que hay en BasicPlus:** nada de sonido. `Pwm.bp` habla de tonos con PWM, y los documentos de V3 y V7 sólo dicen
que la placa trae el códec.

**Lo que costaría** (estimado el 19-oct; se mide en la prueba):

| | |
|---|---|
| flash | ~50 KB: el decodificador de MP3 (~30 KB), el I2S y el códec. Nada en una partición de 6 MB |
| RAM | ~40 KB: el decodificador, sus búferes y los del I2S. Casi todo puede ir a la PSRAM |
| CPU | menos del 10 % de un núcleo para un MP3 estéreo de 44,1 kHz: la P4 va a 360 MHz y tiene coma flotante |
| la SD | un MP3 se lee a 16–40 KB/s; la SD da unos 2 MB/s |

La mayor parte del trabajo es el camino del sonido: el códec, el I2S, una tarea que no deje de darle datos y el
módulo de BasicPlus. El MP3 es un decodificador encima. Por eso WAV primero.

**El decodificador del MP3: minimp3.** Es de dominio público, cabe en un solo fichero de C y compila en todas las
placas y en el PC. El de Espressif (`esp_audio_codec`) va más optimizado, pero es una biblioteca cerrada y sólo
funciona en sus chips. Las patentes del MP3 expiraron en 2017.

**Por pasos:**
1. **WAV, a mano en la P4, sin BasicPlus**, como el paso 1 de la WiFi: el ES8311 por I2C, el I2S, el amplificador y un
   `.wav` leído de la SD, por el altavoz. Lo que cuesta, medido. Un día, más o menos.
2. **El MP3, encima:** minimp3 entre la lectura y el I2S.
3. **BasicPlus:** de 3 a 5 días, según lo que decida Eduardo:
   - el módulo (`Audio.play`, `stop`, `volume`, `isPlaying`…) y los formatos;
   - en la imagen o en un pack, como LVGL;
   - qué hace en el PC: sonido de verdad (SDL en la VM-C; en miVM, `javax.sound` lee WAV, y el MP3 pediría un
     decodificador) o simulado, con su rastro como los periféricos;
   - el códec y los pines, en la configuración de la placa;
   - por el ahorro de energía (§E3, «Lo decidido»), el amplificador y el códec apagados cuando no suena;
   - el ES8311 en el bus de la táctil, por el árbitro de la HAL BP.

**Paso 1, hecho el 19-oct** (Eduardo: *«Continuamos, probamos un .wav»*). El firmware de prueba está en
`C:\tmp\v8sonido\p4_wav`, compilado con el IDF 6.1 y `esp_codec_dev` 1.5.10 (Apache-2.0). Ese componente salió del
almacén local que dejó EIM, sin bajar nada. En la P4 se grabó sólo la app.
- **El programa** escribe en la SD dos `.wav`: un la de 440 Hz (2 s, 44,1 kHz, mono) y el canon del ejemplo del IDF
  (10 s, 16 kHz, estéreo). Después los lee con su cabecera RIFF y los toca por el ES8311. El códec contesta en el I2C
  (0x18) y el audio está listo en 17 ms.
- **En tiempo real:** los 10 s de canon suenan en 9,99 s. Leer la SD es el 4 % del tiempo (428 ms de los 10 s), y el
  resto la CPU espera al I2S. El audio ocupa ~7 KB de RAM interna, y no crece de una vuelta a otra.
- **El amplificador sólo está encendido mientras suena:** `esp_codec_dev` lo enciende al abrir y lo apaga al cerrar,
  como pide el ahorro de energía.
- **El volumen.** La primera prueba, a 60, se oyó *«bastante bajo»*, por dos cosas:
  - el 0–100 de `esp_codec_dev` va de −50 a 0 dB, más +3,6 dB porque el amplificador va a 5 V y el códec a 3,3 V; el
    60 son −16,4 dB en el códec;
  - el canon venía grabado muy bajo: su pico, al 19 % de la escala (−14,5 dBFS).

  Con el canon normalizado (el pico a −3,6 dBFS) y tres vueltas a 70, 85 y 100, Eduardo: *«A 70 diría que un poco
  bajo, a 85 sería un volumen 'normal' y a 100 podemos decir que está un poco alto. Yo lo dejaría a 85.»* **El volumen
  por defecto, 85.**
- Cada fichero deja una línea de error del IDF, sin daño: `esp_codec_dev` apaga el canal I2S antes de
  reconfigurarlo aunque ya esté apagado.

**Lo siguiente**, a elegir por Eduardo: el MP3 encima (minimp3), o el módulo de BasicPlus con sus decisiones.

---

## E5 · Los strings: el censo de sus operaciones, el invariante y la PSRAM (H5; Eduardo, 21-oct)

> **Es el H5 de V8** (Eduardo, 21-oct): *«A ver tenemos pendiente de V7 optimizar los strings. Esto lo vamos agrupar
> en H5. Por un lado la optimización de la operación de strings. Y por otro la optimización de strings que están en
> PSRAM.»* **Sólo análisis, por ahora** (*«De momento solo analizamos»*). Y la norma, ese mismo día: *«La norma debe
> ser: 1ª analizar, 2º mejorar algoritmo, 3º mejorar implementación.»* Del GC, en concreto: *«Lo del GC y la lista de
> huecos de momento solo analiza, de momento no intentamos mejorar.»*

**Lo que pide Eduardo:**
1. *«El primer punto es que una vez creado un string es un invariante mientras exista. Así que la equivalencia entre
   strings y array de bytes tiene matices.»*
2. *«El segundo punto a analizar es si la PSRAM tiene caché y si la podemos aprovechar para trabajar con strings.»*
3. *«Necesitamos una lista de las operaciones sobre strings, así de memoria: Operaciones de creación/destrucción,
   búsqueda, copiado. Habría que censar de más lentas a más rápidas, y optimizar las más lentas.»*

### De dónde viene

- **`#554`** (V7, cerrada el 27-sep): concatenar en placa era unas 250 veces más lento que en el PC, porque
  `__strconcat` copiaba byte a byte en bytecode. Desde entonces copia en C (`__strCopy`, el camino C de `V7_IDEAS`
  §H7), y en la Metro `CatCrono` pasó de 2122 a 44 ms. Con `#587`, el `StringBuilder` va por piezas.
- **`#665`** (pasada a V8): el 12-oct, en la Metro, concatenar iba ×2,1 más lento con la VM en la PSRAM. Eduardo
  preguntó: *«¿cuál es la explicación de que el trabajo con los strings sea tan lento?»*. Quedaron tres cosas por
  decidir:
  - las líneas de diagnóstico del GC;
  - concatenar en una sola operación nativa;
  - la velocidad del intérprete en la RP2350.
- En el bloque de V8 de FICHAS: «reservar sin el cerrojo de la VM» (con dos runs, las cadenas no escalan).

### Cómo es un string hoy (leído el 21-oct en las dos VMs y en el compilador)

- **Un string es un `byte[]` con texto UTF-8**: `[etiqueta][longitud en bytes][bytes]`. En la memoria no se
  distinguen; que algo sea un string sólo lo sabe el compilador. Los índices (`strlen`, `charAt`, `charCodeAt`,
  `substring`, `indexOf`) cuentan caracteres, no bytes.
- **Un literal** vive en el bloque de datos de su módulo, `[longitud][bytes]`, **sin etiqueta**. Se copia a la RAM al
  cargar, también con XIP, y se usa con una referencia directa, sin handle.
- **No se recuerda nada**: ni cuántos caracteres tiene una cadena, ni si es toda ASCII, ni un hash.
  - `strlen`, `charAt`, `charCodeAt` y `substring` cuentan la cadena ENTERA en cada llamada, para mirar el rango, y
    luego caminan hasta el índice.
  - La VM-Java hace lo mismo con otra maquinaria: cada builtin decodifica la cadena entera a un `String` de Java.
- **Crear:** toda reserva pone el bloque a cero, y quien crea el string copia encima. Cada byte se escribe dos veces.
- **Concatenar:** el compilador sintetiza en cada módulo `__strconcat` y `__strconcatN`, con una reserva y una copia en
  C por operando. `"a" + "b"` no se junta al compilar, y `"" + n` reserva dos veces.
- **Comparar:** `==`, `!=` y los `case` de un `switch` de cadenas llaman a `__strequals`.
  - Es un bucle de BYTECODE: 17 instrucciones por byte, después de mirar `null`, la misma referencia y la longitud.
  - El `switch` prueba los casos uno a uno.
- **`StringBuilder.toString()`** reserva y copia dos veces: el `byte[]` (que ya nace a cero) y luego `fromBytes`.
- **La stdlib recorre por índice.** Llaman a `charCodeAt` en cada carácter, así que son cuadráticos:
  - el lector y el escritor de `Json` (en el lector, además, unas siete llamadas a métodos por carácter: contadas en
    el código, sin medir);
  - `Str.compare`, y con él todo `Map` con claves de cadena;
  - `Str.lastIndexOf`, `Str.parse*` y `Orm.esc`.
- **No hay hash en ninguna parte:** `Collections.Map` es una lista ordenada con búsqueda binaria, y `JsonObject` busca
  sus claves una a una.

### El censo, medido el 21-oct (`samples/benchmarks/StrCenso.bp`)

**Cómo se midió:**
- 36 operaciones, con cadenas ASCII de 16, 256 y 2048 bytes;
- cada una se repite durante un tiempo fijo, y se le descuenta el bucle vacío;
- en la VM-C y la VM-Java del PC, y en tres placas con su imagen de siempre, sin grabar nada: la Pico 2, la Metro (con
  la VM en la SRAM, `psram=0`, y en la PSRAM, la de siempre) y la P4;
- la Metro con la PSRAM, dos veces: difieren menos de un 1 %. Se quedó con `psram=1`, como estaba;
- la C5 y la S3, sin medir: no estaban conectadas.

Con 2048 bytes, de la más lenta a la más rápida (por la Pico 2):

| grupo | operación | Pico 2 | Metro, VM en SRAM | Metro, VM en PSRAM | P4 | VM-C del PC |
|---|---|---|---|---|---|---|
| patrón | recorrer con `charAt` | 581 ms | 574 ms | 623 ms | 240 ms | 1,5 ms |
| patrón | recorrer con `charCodeAt` | 535 ms | 533 ms | 556 ms | 228 ms | 1,5 ms |
| patrón | construir con `s := s + x` | 382 ms | 310 ms | 637 ms | 123 ms | 0,7 ms |
| patrón | construir con `appendStr(x)` | 162 ms | 161 ms | 190 ms | 130 ms | 0,6 ms |
| crear | `StringBuilder`: n `appendChar` + `toString` | 63,6 ms | 63,3 ms | 67,0 ms | 48,8 ms | 232 µs |
| comparar | `s == t`, iguales | 23,2 ms | 23,2 ms | 24,0 ms | 19,5 ms | 90 µs |
| crear | `split` por comas (n/8 trozos) | 9,6 ms | 7,2 ms | 7,2 ms | 3,2 ms | 20 µs |
| destruir | `gc()` | 2,4 ms | 2,0 ms | 10,4 ms | 10,1 ms | 253 µs |
| crear | `StringBuilder`: 4 `appendStr` + `toString` | 1,4 ms | 1,2 ms | 2,0 ms | 724 µs | 3,3 µs |
| crear | `replace` (a por x) | 1,2 ms | 1,2 ms | 1,6 ms | 488 µs | 11 µs |
| crear | `upper` | 1,1 ms | 1,0 ms | 1,5 ms | 356 µs | 3,6 µs |
| crear | `trim` | 952 µs | 881 µs | 1,5 ms | 351 µs | 1,5 µs |
| crear | `substring` (la mitad central) | 606 µs | 580 µs | 826 µs | 203 µs | 1,7 µs |
| buscar | `indexOf`, al final | 497 µs | 500 µs | 522 µs | 278 µs | 3,5 µs |
| copiar | `toBytes` | 460 µs | 378 µs | 800 µs | 114 µs | 1,0 µs |
| copiar | `fromBytes` | 450 µs | 377 µs | 1,1 ms | 114 µs | 0,9 µs |
| buscar | `contains`, no está | 373 µs | 373 µs | 392 µs | 216 µs | 3,5 µs |
| buscar | `indexOf`, no está | 370 µs | 370 µs | 391 µs | 205 µs | 3,4 µs |
| medir | `charAt(s, n/2)` | 285 µs | 283 µs | 331 µs | 115 µs | 0,7 µs |
| crear | `a + b + c + d` (cuatro cuartos) | 263 µs | 188 µs | 543 µs | 82 µs | 0,5 µs |
| medir | `charCodeAt(s, n/2)` | 261 µs | 261 µs | 276 µs | 107 µs | 0,7 µs |
| crear | `a + b` (dos mitades) | 243 µs | 171 µs | 755 µs | 103 µs | 0,5 µs |
| comparar | `switch` de 8 casos (acierta el último) | 212 µs | 211 µs | 223 µs | 189 µs | 0,9 µs |
| crear | cadena + entero | 199 µs | 164 µs | 445 µs | 80 µs | 0,3 µs |
| medir | `strlen(s)` | 132 µs | 132 µs | 139 µs | 65 µs | 0,2 µs |
| crear | `intToString` | 45,5 µs | 43,7 µs | 61,8 µs | 17,9 µs | 38 ns |
| crear | `floatToString` | 43,8 µs | 40,2 µs | 46,6 µs | 18,1 µs | 14 ns |
| convertir | `parseFloat` | 35,2 µs | 34,8 µs | 55,6 µs | 12,6 µs | 0,4 µs |
| comparar | `s == t`, distintas en el 1er byte | 26,9 µs | 26,6 µs | 30,2 µs | 23,6 µs | 90 ns |
| comparar | `s == t`, distinta longitud | 16,8 µs | 17,0 µs | 19,1 µs | 15,1 µs | 45 ns |
| convertir | `parseInt` | 13,9 µs | 14,7 µs | 15,5 µs | 4,0 µs | ~0 |
| buscar | `startsWith` (8 B) | 8,6 µs | 8,6 µs | 7,8 µs | 4,1 µs | ~0 |
| buscar | `endsWith` (3 B) | 7,4 µs | 7,0 µs | 6,1 µs | 3,3 µs | ~0 |
| medir | `__bytesDe(s)` (los bytes) | 1,9 µs | 1,8 µs | 1,5 µs | 1,1 µs | ~0 |
| crear | un literal | 1,0 µs | 0,7 µs | 0,2 µs | 0,9 µs | ~0 |
| copiar | asignar (no copia) | 0,9 µs | 1,6 µs | 1,5 µs | 0,9 µs | ~0 |

**Lo que dice:**
1. **Lo más lento no son operaciones sueltas, son patrones:** recorrer una cadena por índice y construirla
   concatenando. Son cuadráticos. De 256 a 2048 bytes (×8), en la Pico 2:
   - recorrer con `charCodeAt` crece ×50;
   - construir con `+` crece ×20;
   - lo lineal crece entre ×7 y ×9.
2. **Detrás vienen tres operaciones:** el `StringBuilder` carácter a carácter (64 ms), `==` entre cadenas iguales
   (23 ms: 11 µs por byte, el bucle de bytecode) y `split` (10 ms).
3. **`gc()` cuesta 2 ms con la VM en la SRAM y 10 con la PSRAM.** Pesa sobre todo cuántos huecos hay en el heap (abajo,
   «El GC y los huecos»).
4. **Un solo `charCodeAt(s, 1024)` de una cadena de 2 KB cuesta 261 µs** en la Pico 2: cuenta 2048 bytes y luego
   camina 1024.
5. **Lo que no depende del tamaño cuesta entre 1 y 27 µs:** asignar, un literal, los bytes (`__bytesDe`),
   `startsWith` y `endsWith`, `==` con distinta longitud.
6. **Con 256 bytes, el `StringBuilder` pierde:** `appendStr("x")` 256 veces tarda 20,8 ms, y concatenar 18,7 ms. Cada
   `append` son tres o cuatro llamadas a métodos BP, unos 80–110 µs por pieza en la RP2350.
7. **El orden de las placas es casi el del PC.** Las dos VMs del PC ponen arriba las mismas cinco operaciones (sin
   contar `gc()`, que en el PC inflan sus líneas de diagnóstico), en otro orden.
8. **La P4 no gana igual en todo.** En lo que es C (copiar, buscar, contar) va de 2 a 4 veces más rápida que la
   Pico 2. En lo que es bytecode y llamadas (`==`, el `switch`, los `append`), sólo un 10–30 %. Y su `gc()` es más
   lento: 10 ms frente a 2,4.

**De paso, el `switch` de ENTEROS también compara caso a caso.** Llegar al caso 37 del censo cuesta, sobre el primero,
97 µs en la Pico 2 (2,6 µs por caso) y 78 µs en la P4.

### El GC y los huecos: un coste cuadrático (`#715`, sólo análisis)

**Lo que se ve en el código** (`gc_sweep_phase`, en `heap.c`):
- cada recolección barre el heap y rehace la lista de huecos libres en orden de dirección;
- pero cada hueco lo da de alta `add_to_free_list`, que busca su sitio recorriendo la lista desde el principio;
- con k huecos son k²/2 pasos, y en la PSRAM cada paso puede ser un fallo de caché.

**Medido** con `samples/benchmarks/GcHuecos.bp`: k cadenas vivas, cada una entre dos que se tiran, y luego diez `gc()`
seguidos.

| k | Pico 2 | Metro, VM en PSRAM | P4 | VM-C del PC |
|---|---|---|---|---|
| 500 | 43 ms | 178 ms | 167 ms | — |
| 1000 | 93 ms | 514 ms | 610 ms | 16 ms |
| 2000 | 343 ms | 2972 ms | 2869 ms | 30 ms |
| 4000 | 1232 ms | 12 710 ms | **36 026 ms** | 242 ms |

- **Doblar los huecos cuadruplica el tiempo.** En la P4, de 2000 a 4000, crece ×12,6: la lista ya no cabe en la caché.
- **Con 4000 cadenas vivas, un `gc()` para la P4 3,6 s.** Probablemente es lo que más pesa en los programas reales que
  manejan muchas cadenas pequeñas, como un JSON o una lista de nombres.

Eduardo: *«de momento no intentamos mejorar»*. Para cuando toque, lo que pediría el algoritmo, sin probar: el barrido va
en orden de dirección, así que el hueco nuevo va siempre al final. Engancharlo a la cola es un paso, no k.

**La condición de Eduardo (21-oct, tarde):** *«La lista de huecos tiene que ser por dirección para así poder fusionar 2
huecos adyacentes.»* Ya lo es desde `#355` (su propuesta del 2-ago): `add_to_free_list` la mantiene de menor a mayor
dirección y fusiona el hueco nuevo con los dos vecinos. Y engancharlo a la cola la conserva tal cual:
- el barrido junta él mismo los bloques muertos seguidos y da de alta cada tramo al llegar al siguiente bloque vivo.
  Cada hueco va detrás de todos los anteriores y nunca toca al anterior: entre los dos hay un bloque vivo;
- la compactación rehace la lista igual, en orden de dirección;
- fuera del GC (soltar un `owner`, el sobrante de una reserva), el alta sigue buscando su sitio y fusionando.

El detalle, en FICHAS, `#715`.

### El perfil por fases (21-oct, `samples/benchmarks/StrFases.bp`)

Eduardo: *«Quizás faltaría un profile que nos diga cuanto tiempo está haciendo cada tarea. Mi intuición me dice que la
parte de localizar y copiar es bastante importante y tu me dices que la parte de crear […] también es costosa. Como
método, optimizar cada parte por separado, al final si optimizas la creación, la búsqueda y la copia, estas optimizando
la manipulación de bits si o si.»*

Una operación de cadena es una suma de fases: llamar (el intérprete), reservar (con su puesta a cero y el GC que le
toque), copiar y localizar (contar caracteres, buscar). Cada fase se midió SOLA, con el método del censo, contando las
colectas que caen dentro. Al final se midieron tres operaciones enteras, para ver que la suma cuadra.

Con 2048 bytes (con 256, en el mismo programa):

| fase | Pico 2 | Metro, PSRAM | P4 | PC |
|---|---|---|---|---|
| llamar: una función BP con dos cadenas | 5,9 µs | 6,5 µs | 6,1 µs | 23 ns |
| reservar 16 bytes (casi todo, el coste fijo) | 24 µs | 23 µs | 7,3 µs | 34 ns |
| reservar 2048 bytes | 140 µs (71 GC cada 1000) | 407 µs (2 GC cada 1000) | 42 µs | 343 ns |
| copiar 16 bytes (casi todo, la llamada al builtin) | 6,2 µs | 12 µs | 3,8 µs | 16 ns |
| copiar 2048 bytes (`__strCopy`: `memmove`) | 18 µs | 59 µs | 7,2 µs | 22 ns |
| localizar: contar 2048 bytes (`strlen`) | 131 µs | 147 µs | 64 µs | 180 ns |
| localizar: buscar sin hallar en 2048 (`indexOf`) | 366 µs | 396 µs | 189 µs | 3,4 µs |
| localizar: contar y saltar al último (`charCodeAt`) | 385 µs | 416 µs | 146 µs | 1,1 µs |
| destruir: `gc()` | 1,3 ms | 3,8 ms | 3,0 ms | 247 µs |
| entera: `a + b` (dos mitades) | 203 µs | 569 µs | 64 µs | 476 ns |
| entera: `fromBytes` de 2048 | 333 µs | 679 µs | 102 µs | 987 ns |
| entera: `substring` (la mitad central) | 596 µs | 786 µs | 200 µs | 1,7 µs |

**La suma cuadra.** En la Pico 2, `a + b` es llamar (6) + reservar 2 KB (140) + dos copias de 1 KB (~24) + el resto del
bytecode (~30): unos 200 µs, y se midieron 203.

**Lo que dice, fase a fase:**
1. **Crear (reservar) es lo más caro en todo lo que crea:** dos tercios de `a + b` en las tres placas. Pesan tres cosas
   distintas:
   - **un coste fijo alto:** reservar 16 bytes cuesta 24 µs en la RP2350 (unos 3600 ciclos) y 7 en la P4. Qué parte es
     el cerrojo, la lista de huecos, el handle o la puesta a cero, no se ve desde BP;
   - **con un heap pequeño, el GC:** en la Pico 2 (219 KB) hay una colecta cada 14 reservas de 2 KB, y se lleva unos
     90 de los 140 µs;
   - **con PSRAM, la memoria nueva:** en la Metro casi no hay GC (2 cada 1000), pero reservar 2 KB cuesta 407 µs.
     Poner a cero memoria recién estrenada cuesta siete veces más que copiar en memoria que ya está en la caché
     (59 µs). Es el punto 2 en números.
2. **Localizar es lo más caro en lo que busca o cuenta.** Contar va a 64 ns por byte en la RP2350; buscar, a 179 (un
   `memcmp` por posición). Leer la memoria podría ir unas diez veces más deprisa.
3. **Copiar es barato si lo hace `memmove`:** 12 µs para 2 KB en la Pico 2. Pero los bucles propios byte a byte cuestan
   unas quince veces más: es la diferencia entre `fromBytes` (333 µs) y reservar más copiar con `memmove` (unos
   160 µs).
4. **Llamar cuesta unos 6 µs por llamada BP** en las tres placas: la P4 no gana aquí (es bytecode).

**Con 256 bytes** el reparto se mueve hacia lo fijo. En la Pico 2, `a + b` son 83 µs: reservar, 37 (de ellos 24 fijos);
llamar y copiar, 14. Localizar sigue proporcional: 19 µs contar, 52 buscar.

**El método de Eduardo, con esta línea de base:** optimizar por separado crear, localizar y copiar.
- **Crear:** el coste fijo de la reserva, la puesta a cero y el GC (`#715`).
- **Localizar:** contar y buscar leyendo palabras, y recordar «es ASCII».
- **Copiar:** `memmove` (o lo nuestro, donde la libc copia byte a byte) en vez de los bucles.

Para ver DENTRO del coste fijo de la reserva (el cerrojo, la lista, el handle, el cero), hace falta medir en C, con el
contador de ciclos de cada procesador, en una imagen instrumentada: por decidir.

### Punto 1: el invariante, y los matices de «string = byte[]»

**El mecanismo.** Un `byte[]` y un string son los mismos bytes con dos contratos distintos:
- el `byte[]` es un búfer que se escribe;
- el string es un valor que no cambia mientras existe (Eduardo).

En la memoria no se distinguen: los distingue el compilador.

**Lo que el invariante permite.** Lo que vale al crear un string vale mientras exista, así que se puede:
- compartirlo sin copiar (ya se hace: asignar no copia);
- **recordar lo que se calcula una vez**: si es todo ASCII, cuántos caracteres tiene, un hash. Hoy no se recuerda
  nada. Con «es ASCII», el carácter `i` es el byte `i`, y `charAt`, `charCodeAt`, `substring` y `strlen` dejan de
  contar;
- comparar primero por referencia (ya lo hace `__strequals`);
- no copiar los literales (ya se hace: el camino C de `#554`).

**Los matices:**
1. **De string a `byte[]`, siempre se copia** (`toBytes`): el `byte[]` se puede escribir, y el string no debe cambiar.
2. **De `byte[]` a string también se copia** (`fromBytes`), porque quien tiene el `byte[]` puede seguir escribiéndolo.
   - **Salvo si el `byte[]` muere ahí mismo.** `StringBuilder.toString()` llena un `byte[]` que no ve nadie más, y
     luego lo vuelve a copiar.
   - Bastaría un traspaso interno de `Core`, sin copia. Sólo para `Core`: fuera de `Core` no hay `any`.
3. **Texto frente a bytes.** Un `byte[]` puede llevar cualquier byte; un string debería ser texto, UTF-8 bien formado.
   - Hoy `fromBytes` acepta cualquier cosa, y con texto mal formado las dos VMs no dan lo mismo (`#711`).
   - Si la conversión a string sustituyera lo mal formado por U+FFFD, como ya hace Java, todo string sería UTF-8
     válido. Entonces las operaciones por bytes (`memcmp`, `memchr`) serían exactas en las dos VMs.
   - **De dónde sale** (Eduardo: *«¿lo generamos nosotros o de dónde sale?»*): de las dos partes, medido con un
     programa en las dos VMs (el detalle, en `#711`).
     - **De fuera:** un fichero en otra codificación (un CSV de Windows en Latin-1: `caña` lleva la `ñ` en un byte,
       `F1`), el puerto serie, la red, o `fromBytes` con bytes cualesquiera.
     - **De nuestro propio código:** `appendChar` con medio par de sustitutos, o con un número fuera de Unicode. Y
       por él, `Json`, que convierte cada `\uXXXX` con un `appendChar`: un emoji escrito en JSON como `😀`
       (lo normal en las API web) sale en la VM-C partido en dos medios caracteres, y en la VM-Java, bien.
4. **Hoy el invariante no se cumple del todo:**
   - `move` (pública, de `Core`) escribe dentro de un string, y el cambio lo ven todas sus referencias (`#714`);
   - dentro de `Core`, un `any` deja que un `byte[]` pase por string sin copiarse.

   Para recordar algo de un string, como lo de arriba, el invariante tiene que ser de verdad.
5. **Lo que se recuerde en la etiqueta no llega a los literales**, porque no la tienen. Seguirían como hoy; suelen ser
   cortos.

**El ejemplo.** Una cadena de 2048 caracteres ASCII, recorrida con `charCodeAt` en la Pico 2.
- Hoy tarda 535 ms, porque cada llamada cuenta los 2048 bytes.
- Con «es ASCII» recordado quedaría el bucle: unos 13 µs por carácter (lo que cuesta con 16 bytes), unos 27 ms en
  total. Veinte veces menos, estimado y sin medir.

### Punto 2: la PSRAM y su caché

Sí: en todas las placas con PSRAM se llega a ella **a través de una caché**. La PSRAM no se lee directamente: está
mapeada en la memoria del procesador, y cada acceso pasa por la caché. Cuando la caché no tiene la línea, la trae entera
de la PSRAM; y la escribe de vuelta más tarde (escritura diferida).

| placa | PSRAM | la VM en ella | la caché que la sirve | ¿la comparte con el código? |
|---|---|---|---|---|
| **Metro** (RP2350) | 8 MB, APS6404 en QPI a 75 MHz (150 MHz / 2; su tope son 109) | con `psram=1`, 5,5 MB | la del XIP: **16 KB**, 2 vías, **líneas de 8 B**, diferida | **sí**: el intérprete se ejecuta de la flash por esa misma caché |
| **S3** | 8 MB, octal, a 80 MHz | 7,0 MB | la de datos: 32 KB, 8 vías, líneas de 32 B (se puede subir a 64 KB y 64 B) | no: el código va por su caché de instrucciones (16 KB) |
| **C5** | 8 MB, quad, a 80 MHz | 6,5 MB | **una sola caché de 32 KB** para el código y los datos, líneas de 32 B | **sí** |
| **P4** | 32 MB, hex, a 200 MHz | 23,5 MB | L1 de datos de 64 KB (2 vías, líneas de 64 B) y L2 de 128 KB (8 vías, 64 B; se puede subir a 256 o 512 KB) | en la L2 sí; en la L1, no |

Sin PSRAM, las cadenas van en la SRAM, sin esta caché delante: la Pico 2, la C3, la C6, la Nucleo y la DK2.

De dónde sale cada dato:
- los `sdkconfig` de `esp32/`, `esp32c5/` y `esp32p4/`;
- el `soc_caps.h` del IDF 6.1 (la C5 y la P4, con caché compartida para instrucciones y datos; la S3, no);
- el `pico-sdk` (`xip_cache.h`: 16 KB y líneas de 8 B) y `pico/psram.c` (el reloj de la PSRAM de la Metro);
- la hoja de datos de la P4, §4.1.3.3 (los tamaños de la L1);
- los informes del IDF 6.1 (`compat/informes/idf61_*.json`: `psramBytes`, `vmHeapBytes`).

**Lo que explica de lo medido.** Se compara la Metro consigo misma: el mismo chip, la misma imagen, la VM en la SRAM o
en la PSRAM.
- **Leer una cadena cuesta casi lo mismo:** `strlen`, `charCodeAt`, `indexOf` y `==`, entre un 3 y un 6 % más. La
  cadena que se lee ya está en la caché.
- **Crear cuesta de ×1,5 a ×4,4:** `a + b` de 2 KB pasa de 171 a 755 µs; `fromBytes`, ×2,9; construir concatenando,
  ×2,1; `gc()`, ×5,2. El mecanismo:
  - cada cadena nueva es memoria que no está en la caché: la reserva avanza por cientos de KB antes del siguiente GC
    (con 5,5 MB de heap, cada ~700 KB);
  - cada línea de 8 bytes de la cadena nueva pasa por la caché dos veces (se pone a cero y se copia encima) y acaba
    escrita en la PSRAM (si la caché lee además la línea antes de escribirla, sin comprobar);
  - y en la Metro esa caché de 16 KB es también la del código: lo que entra de la cadena echa al intérprete, que se
    vuelve a leer de la flash.
- **La P4, con la VM en la PSRAM**, va de 2 a 4 veces más rápida que la Pico 2 en lo que es C, y sólo un 10–30 % en
  lo que es bytecode. No se puede comparar consigo misma en la SRAM: su VM siempre está en la PSRAM.

**Cómo aprovecharla**, de más barato a más caro:
1. **Crear menos** (algoritmo): el `StringBuilder` sin la segunda copia, recorrer sin crear (`charCodeAt` en vez de
   `charAt`), y la lista de huecos del GC. Vale en todas las placas, y más con PSRAM.
2. **No escribir dos veces** (implementación): reservar sin poner a cero lo que se va a llenar entero.
3. **La imagen** (`#665`): en la Metro y la C5, sacar el bucle del intérprete de la caché compartida, a la SRAM.
4. **El tamaño de la caché** (configuración), midiendo antes:
   - la de datos de la S3, de 32 a 64 KB (32 KB menos de RAM interna);
   - la L2 de la P4, de 128 a 256 KB (128 KB menos).
5. **Fuera, de momento:** las cadenas jóvenes en la SRAM y las viejas en la PSRAM (un GC generacional) cambia el modelo
   de memoria.

### Los algoritmos: de quién son, y si cambian según el caso (Eduardo, 21-oct)

Eduardo: *«Ahora, los algoritmos. Son nuestros? Son estandard? Sirven para todos los casos, o en unos casos se utilizan
unos y en otros se utilizan otros? En los micros, se utilizan algoritmos de la HAL?»*

**De quién es cada uno.** En la VM-C, casi todo es nuestro: el mismo C en todas las placas. La libc pone las piezas
básicas (copiar, poner a cero, comparar bloques, convertir números). La VM-Java usa la biblioteca de Java 8.

| operación | VM-C: el algoritmo | ¿de quién? | VM-Java |
|---|---|---|---|
| `strlen`, `charAt`, `charCodeAt`, `substring` | recorrer los bytes UTF-8 desde el principio | nuestro (`utf8_cp_count`, `utf8_byte_offset`) | decodificar la cadena entera a un `String` de Java y usar sus métodos |
| `indexOf`, `contains`; el separador de `split`; `replace` | búsqueda directa: la aguja, comparada en cada posición | nuestro, con el `memcmp` de la libc | `indexOf`, búsqueda directa; `split` y `replace`, por las expresiones regulares de Java 8 |
| `startsWith`, `endsWith` | `memcmp` | la libc | los de Java |
| `==`, `!=`, los `case` | bucle de bytecode byte a byte, con atajos (null, la misma referencia, la longitud) | nuestro: lo genera el compilador | el mismo bytecode |
| concatenar | una reserva y un `memmove` por trozo | nuestro, con la libc | `System.arraycopy` |
| `upper`, `lower`, `trim` | bucles propios (las mayúsculas, sólo Latin-1) | nuestro | `trim` de Java; las mayúsculas, propias |
| `intToString` | `snprintf` | la libc | `Integer.toString` |
| `floatToString`, `doubleToString` | un formateador propio («GAP-4»), por la paridad | nuestro | el mismo, escrito en Java |
| `parseInt`, `parseFloat` | `strtol`, `strtod` | la libc | `Long.parseLong`, `Double.parseDouble`: **no dan lo mismo** (`#716`) |
| `StringBuilder`, `Str.compare`, `Map`, `Json` | escritos en BP | nuestros | los mismos |
| reservar | el primer hueco que cabe en una lista ordenada por dirección; si no hay, al final | nuestro | el mismo, en Java |
| el GC | marcar y barrer, sin compactar; la lista de huecos se rehace al barrer (`#715`) | nuestro | el mismo, en Java |

**¿Sirven para todos los casos?** Los nuestros, sí: un algoritmo por operación, el mismo para 3 bytes que para 3 MB, y
para ASCII que para lo demás. Las únicas elecciones por caso son los atajos de `==` y la concatenación de tres o más
trozos de una vez.

Las bibliotecas estándar sí eligen por caso por dentro: el `memcpy` de newlib copia por palabras si las direcciones van
alineadas y byte a byte si no.

Otros lenguajes eligen por caso justo donde a nosotros nos duele:
- **cómo se guarda una cadena:**
  - CPython guarda cada una con 1, 2 o 4 bytes por carácter, según el más ancho que tenga: el índice es directo;
  - Java (desde la 9) la guarda en Latin-1 si cabe y en UTF-16 si no;
  - Go y Rust la dejan en UTF-8 e indexan por bytes; los caracteres, con un iterador;
- **buscar:** búsqueda directa para agujas cortas, y Two-Way o Boyer-Moore para las largas (la glibc);
- **recordar:** Java y MicroPython guardan el hash en la cadena; MicroPython y Lua internan las cadenas cortas o los
  nombres, y entonces comparar es mirar si son la misma;
- **reservar:** casi todos los asignadores separan los huecos por tamaños; el nuestro tiene una sola lista.

**MicroPython, el más cercano** (Eduardo los compara en sus Picos), tomó la misma decisión que nosotros: UTF-8 y el
índice O(n), recorriendo desde el principio. Se discutió en python-dev en 2014, con Guido en contra porque mucho de
Python va por índices. Pero hace tres cosas que nosotros no:
- calcula el hash al crear la cadena y lo guarda;
- puede validar el UTF-8 al crear la cadena (una opción de compilación, de 110 a 170 bytes de código): es lo que
  propone `#711`;
- interna los nombres y los literales (sus «qstr»).

Fuentes de lo de MicroPython:
- `py/unicode.c` y `py/objstrunicode.c`
  ([unicode.c](https://toscode.mulanos.cn/mirrors/micropython/blob/master/py/unicode.c),
  [objstrunicode.c](https://toscode.mulanos.cn/mirrors/micropython/blob/master/py/objstrunicode.c));
- el parche de la validación
  ([68c28174](https://git.flow3r.garden/card10/micropython/-/commit/68c28174d0e0ec3f6b1461aea3a0b6a1b84610bb));
- la discusión de 2014 ([python-dev](https://www.python.org/pipermail/python-dev/2014-June/134819.html)).

**En los micros, ¿algoritmos de la HAL? No.** Las HAL de los fabricantes (pico-sdk, ESP-IDF, la de ST) no traen
algoritmos de cadenas, y nuestra HAL BP es de periféricos. Las cadenas y el GC son el mismo C en todas las placas
(`bpgenvm-c/src`). Lo que cambia de una placa a otra es lo de debajo:

| | compilador | optimización de nuestro código | libc | `memcpy` y `memset` |
|---|---|---|---|---|
| PC | gcc (MinGW) | `-O2` | la de MinGW | los de MinGW |
| RP2350 (Pico 2, Metro) | arm-none-eabi gcc 15.2 | **`-Os`** (el pico-sdk, `-O3`) | newlib completa | los de newlib, por palabras (ensamblador) |
| S3 | xtensa gcc 15.2 (IDF 6.1) | `-Os` | picolibc | los dos, de la ROM del chip |
| C3, C5, C6, P4 | riscv32 gcc 15.2 (IDF 6.1) | `-Os` | picolibc | `memset` de la ROM; `memcpy` del IDF, en la RAM interna |
| STM32 (DK2, Nucleo) | arm gcc 14.3 (CubeIDE) | `-Os` | newlib-nano | **byte a byte**: `memcpy`, `memset`, `memmove` y `memcmp` |

Tres cosas que salen de aquí:
- **Un mismo `memcpy` es código distinto en cada placa**: ensamblador por palabras (RP2350), la ROM (S3), C del IDF
  (RISC-V), o un bucle de un byte (STM32, comprobado en su desensamblado: `ldrb`/`strb`). Copiar una cadena cuesta
  distinto por eso, no por nuestro código.
- **En todas las placas nuestro código va optimizado para TAMAÑO (`-Os`), y en el PC para velocidad (`-O2`).**
  - En las ESP32 está decidido en sus `sdkconfig.defaults` («Optimizar a tamaño»).
  - En la Pico es una línea del bring-up («Flags útiles para diagnóstico durante el bring-up») que tapa el `-O3` de
    Release: comprobado en `build.ninja`, `interp.c` se compila con `-O3 … -Os` y gana el último.
  - Cuánto cuesta en el intérprete, sin medir. Puede ser parte de por qué la P4, a 360 MHz, sólo gana un 10–30 % en
    lo que es bytecode.
- **Lo que viene de la libc hereda su comportamiento**, y el de C y el de Java no es el mismo. `parseInt` y
  `parseFloat` lo enseñan (`#716`). Y `strtod` es además distinto en MinGW, en newlib y en picolibc (sin comprobar).

Existen aceleradores (el DMA en todas, y las instrucciones vectoriales de la S3 y la P4), pero no se usan para
cadenas: para una copia pequeña cuesta más prepararlos que copiar.

### Lo que se puede mejorar, por la norma de Eduardo (sin empezar nada)

**2º, el algoritmo** (lo que cambia cómo crece el coste):

| | hoy | con el algoritmo | qué toca |
|---|---|---|---|
| recorrer por índice | n² | n | recordar «es ASCII» (la VM, las dos); o recorrer sin índice (el lenguaje: un `for each` de caracteres, o decodificar una vez); o por bytes, en la stdlib |
| `Str.compare` (los `Map`) y `Str.lastIndexOf` | n² (y casi n³) | n | la stdlib: por bytes, o un builtin |
| las claves de `JsonObject` | k² al construir | k log k, o k | la stdlib |
| un `switch` de cadenas (y de enteros) | un caso tras otro | uno, o log | el compilador |
| la lista de huecos del GC (`#715`) | k² | k | la VM, las dos. **Sólo análisis** |
| construir concatenando | n² | n, con un `StringBuilder` | el idioma ya existe; que gane también con piezas pequeñas es implementación |

**3º, la implementación** (lo mismo, más deprisa):
- **`==` y los `case`, en C y no en bytecode.** Unas 100 veces menos por byte (estimado). Dos maneras:
  - con un builtin nuevo, que pide imagen nueva, como `#554`;
  - sin él: «misma longitud y `startsWith`», que en la VM-C ya es `memcmp`. Sólo es exacto en las dos VMs con `#711`
    resuelto.
- **El `StringBuilder`:** `toString` sin la segunda copia, y un `append` que no cueste tres llamadas a métodos (o
  nativo).
- **`charAt`, `charCodeAt` y `substring`:** mirar el rango caminando sólo hasta el índice. Pasa de 1,5 a 1 pasada, y
  sigue siendo n².
- **Los bucles de C:**
  - crear sin poner a cero;
  - `memcpy` y `memchr` en `split`, `replace`, `upper`, `trim`, `substring`, `toBytes` y `fromBytes`;
  - `indexOf` con `memchr` para el primer byte.
- **El GC:** formatear sus líneas de diagnóstico sólo con el log encendido.
- **El AOT:** sus literales reservan en cada evaluación, y su `switch` de cadenas reserva en cada caso.

### Hallazgos de paso (con ficha)

- **`#711`:** con UTF-8 mal formado (de `fromBytes`), las dos VMs no dan lo mismo. Rompe la paridad.
- **`#712`:** `newIntArray(2^30 + 1)` desborda el cálculo de sus bytes y deja un bloque de 4 B que dice tener mil
  millones de elementos. Escribir en él pisa otros objetos, en las dos VMs. No es de cadenas.
- **`#713`:** `<`, `>`, `<=` y `>=` entre strings compilan, pero descuadran la pila y dan basura.
- **`#714`:** `move` escribe dentro de un string.
- **`#715`:** el GC rehace la lista de huecos con coste cuadrático (arriba). Sólo análisis.
- **`#716`:** `parseInt` y `parseFloat` no dan lo mismo en las dos VMs con un texto que no es un número limpio, y
  ninguna hace lo que dice `BUILTINS.md` (un error atrapable).
- **Menores, para el `D1`:**
  - `HEAP_LAYOUT.md` está desfasado en cuatro sitios: la tabla de tipos de §2.2, el bloque libre de §2.4, la
    concatenación de §6 y el barrido de §7;
  - la cabecera de `Str.bp` dice que la VM-C no tiene `indexOf`, `upper`, `lower` ni `trim`, y los tiene;
  - un comentario de `bpvm_handles.h` habla de 256 KB donde el bit es de 1 GiB;
  - sin comprobar: `heap_start` podría no ir alineado a 4.
- **La tabla de handles vive en la PSRAM** en las placas que tienen la VM allí: desde `#451` (29-ago) está dentro del
  bloque de la VM, entre el heap y las pilas. Se consulta en cada acceso a un objeto.

### Lo que hay que decidir (Eduardo)

1. **La regla del UTF-8 mal formado** (`#711`). Propuesta: todo string es UTF-8 válido; la conversión desde bytes
   sustituye lo mal formado por U+FFFD, como Java. Es lo que deja exactas las operaciones por bytes.
2. **El agujero de `move`** (`#714`): que no acepte un string como destino. Sin cerrarlo, no se puede recordar nada de
   un string.
3. **`<` y `>` entre strings** (`#713`): un error de compilación, o comparar de verdad, como `Str.compare`.
4. **Qué algoritmo va primero**, de la tabla de arriba.
5. **Recordar «es ASCII».** Toca la cabecera del heap en las dos VMs (usaría los bits libres de la etiqueta), y no llega
   a los literales.
6. **`parseInt` y `parseFloat` con texto que no es un número** (`#716`): qué deben hacer en las dos VMs.
7. **La optimización de nuestro código en placa** (`-Os` en todas): si se mide `-O2` frente a `-Os` en el intérprete.
   Es implementación, el 3er escalón de la norma.
8. **Mirar dentro de la reserva** (24 µs fijos en la RP2350): medir en C, con el contador de ciclos, en una imagen
   instrumentada (grabar y volver a la de siempre).

### Un orden posible (la norma: analizar, algoritmo, implementación)

1. **El análisis:** hecho. Este censo, con sus programas en `samples/benchmarks/` (`StrCenso` y `GcHuecos`) para medir
   otra vez antes y después.
2. **Los algoritmos, uno a uno**, con su medida en el PC (el PC filtra) y en la Pico 2, la Metro y la P4:
   - primero lo de la stdlib, que no toca la VM;
   - luego recorrer por índice y la lista del GC, que sí la tocan.
3. **La implementación:** `==` en C, el `StringBuilder`, crear sin poner a cero, los bucles de C.
4. **La PSRAM:** el tamaño de las cachés, y la imagen de la Metro (`#665`).

### Lo cuadrático: el censo y su alternativa lineal o logarítmica (Eduardo, 21-oct)

Eduardo: *«A ver si podemos reducir el tiempo a una cuarta parte está muy bien. Pero antes de entrar en eso, me interesa
mas mejorar el algoritmo si puede ser. Algoritmos cudráticos no deberian valer, al menos lineales o logarítmicos.»*

**Lo que cuesta en programas de verdad.** Medido el 21-oct en la Pico 2 y la P4, sin grabar, con un programa que dobla
el tamaño en cada paso:

| | Pico 2 | P4 | al doblar |
|---|---|---|---|
| leer un JSON de 1, 2, 4 y 8 KB (`Json.parseJson`) | 0,5 / 1,5 / 4,8 / **16,7 s** | 0,24 / 0,64 / 2,0 / **6,8 s** | ×3,4–3,5 |
| meter 250, 500, 1000 y 2000 claves en un `Map` | 1,5 s / sin memoria | 1,1 / 4,0 / 14,8 / **56 s** | ×3,6–3,8 |

Doblar el tamaño casi cuadruplica el tiempo: las dos son cuadráticas. En la Pico 2, un JSON de 8 KB tarda 17 segundos,
y la segunda vuelta del `Map` se quedó sin memoria (en su heap de 219 KB seguía vivo el último JSON). En la P4, 2000
claves tardan casi un minuto.

**El censo de lo cuadrático, o peor, en el camino de las cadenas:**

| | dónde | hoy | por qué | la alternativa | queda en | qué toca |
|---|---|---|---|---|---|---|
| 1 | recorrer una cadena por índice (`charAt`, `charCodeAt`, `substring`, `strlen` en el bucle); y por dentro, el lector de `Json`, `Str.compare`, `Str.parse*` y `Orm.esc` | n² | cada llamada cuenta desde el principio | **a)** la stdlib, por BYTES donde lo que mira es ASCII (la estructura de JSON, los dígitos, los separadores); **b)** para los programas, recordar «es ASCII» (el índice es el byte) y, para lo demás, recorrer sin índice (un `for each` de caracteres, o decodificar una vez a `integer[]`) | n | a) la stdlib; b) la VM (la etiqueta) y el lenguaje |
| 2 | `Json`: el lector (la fila 1) y las claves de `JsonObject`, en una lista que se recorre para buscar una | n² y k² | lo de la fila 1, y buscar una clave es recorrerlas todas | el lector por bytes; las claves, en una tabla hash, con el orden de inserción aparte | n y k | la stdlib |
| 3 | `Map` con claves de cadena: meter desplaza todo lo que queda detrás (cuatro llamadas a métodos por entrada), y cada comparación es `Str.compare` | construir: n²; comparar: L² | una lista ordenada; `Str.compare` va por `charCodeAt` | comparar bytes (L); y como estructura, un árbol equilibrado (log n por operación, y conserva el orden) o una tabla hash (1, sin orden) | n log n, o n | la stdlib (`Collections`, `Str`); comparar, en la biblioteca de C |
| 4 | buscar una subcadena (`indexOf`, `contains`, `split`, `replace`): la aguja, comparada en cada posición | n·m en el peor caso | búsqueda directa | Two-Way (n + m, sin memoria extra) si la aguja es larga; si es corta, el primer byte y comparar | n + m | la biblioteca de C |
| 5 | `Str.lastIndexOf` | casi n²·m | `charCodeAt` en cada paso | buscar bytes desde el final | n + m | la stdlib o la biblioteca |
| 6 | construir concatenando (`s := s + x` en un bucle) | n² | cada `+` copia todo lo anterior: el precio de la inmutabilidad | el `StringBuilder` (ya es n): que sea el camino recomendado, y que gane también con trozos pequeños (eso es implementación). Las «cuerdas» (*ropes*) lo arreglarían sin cambiar el programa, pero cambian la representación en las dos VMs y en el AOT | n | el idioma y la documentación |
| 7 | el GC: rehacer la lista de huecos (`#715`) | k² | dar de alta desde la cabeza | darla de alta en la cola | k | la VM. Eduardo: de momento, no |
| 8 | reservar: el primer hueco que cabe, recorriendo la lista | huecos, por reserva | una sola lista | listas por tamaños | 1 por reserva | la VM (la memoria) |

Lineales y mejorables, sin ser cuadráticos: un `switch` de cadenas (y de enteros) prueba los casos uno a uno; con una
tabla, sería uno o log. Y `==` ya es lineal.

**La fila 1, el mismo 21-oct por la tarde:** en la VM-C ya es n, sin tocar la stdlib. El texto ASCII va por el rápido
(«Los dos algoritmos, hechos y medidos»), y el texto con acentos por el cursor («El lento con dos contadores, hecho y
medido»). Queda n² recorrer DOS cadenas con acentos a la vez, como hace `Str.compare`.

**Una pieza que lo abre todo sin tocar la VM: leer un byte de una cadena en O(1) desde BP.** `Core` ya aplica `ALEN`
directamente a una cadena (`__bytesDe`). Una función `inline` con `ALOAD_U8` hace lo mismo para leer un byte:

```
function byteAt(s: string, i: integer): integer
  inline ALOAD_U8
end byteAt
```

**Comprobado el 21-oct en las dos VMs:**
- vale con una cadena del heap y con un literal (`"año"`: 97 195 177 111);
- fuera de rango da el mismo error en las dos (`ALOAD_U8: idx fuera de rango 4 (len=4)`);
- recorrer 4000 bytes en la Pico 2: **37 ms con `byteAt`, frente a 1986 ms con `charCodeAt`**.

Con ella, la stdlib recorre por bytes sin copiar. `toBytes` también serviría, con una copia al principio y luego O(1)
por byte.

**Un orden posible**, por lo que se gana y lo que cuesta:
1. **La stdlib por bytes** (filas 1a, 2 y 5, y `Orm.esc`): arregla el JSON medido sin tocar la VM.
2. **Las estructuras:** las claves de `JsonObject` y el `Map` (filas 2 y 3).
3. **En la biblioteca de C:** comparar y buscar (filas 3 y 4).
4. **Para los programas:** recorrer por índice (fila 1b): «es ASCII» y una forma del lenguaje. Las dos, por decidir.
5. **El GC y la reserva** (filas 7 y 8): cuando Eduardo diga.
6. **Concatenar** (fila 6): el idioma y la documentación.

### Dos algoritmos: el rápido para ASCII y el lento para lo demás (decidido por Eduardo, 21-oct)

Eduardo, ante el censo de lo cuadrático: *«No, seguimos con UTF-8. Lo que necesitamos son 2 algoritmos. Se mira si hay
algún caracter especial. Si no hay se trabaja con el algoritmo rápido ( 1 char = 1byte ), si hay caracteres especiales
se trabaja con el algoritmo lento.»*

**Lo que decide si funciona: CUÁNDO se mira.** Mirar es lineal y rápido: una pasada con punteros (Eduardo: *«para
buscar si una cadena tiene un caracter especial no puede ser n^2 tiene que ser lineal. En C con punteros es muy
rápido»*). Por ejemplo:

```c
/* ¿todo ASCII? Una pasada. De 4 en 4 bytes es un OR por palabra y una máscara al final. */
static int todo_ascii(const uint8_t* p, uint32_t n) {
    const uint8_t* fin = p + n;
    uint8_t acc = 0;
    while (p < fin) acc |= *p++;
    return (acc & 0x80u) == 0;
}
```

Lo cuadrático no es mirar, es REPETIRLO:
- **Si se mira dentro de cada `charCodeAt`,** cada llamada paga una pasada entera. Un bucle que recorre 2048 caracteres
  hace 2048 pasadas de 2048 bytes: n². Hoy, en la Pico 2, contar 2 KB en C (`strlen`) cuesta 131 µs. Una pasada que
  sólo mire bytes sería más corta, unas decenas de µs (estimado), pero repetida 2048 veces son decenas de ms, y con
  8 KB, 16 veces más.
- **Si se mira UNA vez, al crear la cadena, y se recuerda,** cada llamada pregunta por una marca: O(1), y el bucle es
  lineal. Eso lo permite el invariante de Eduardo: una cadena no cambia mientras existe, así que lo que se miró al
  crearla sigue valiendo. Y al crearla ya se copian sus bytes con punteros: mirar va en esa misma pasada
  (`*d++ = c = *s++; acc |= c;`).

**Dónde se recuerda:** un bit en la etiqueta del bloque, «todo ASCII». Los bits 21–0 están libres, el GC los respeta al
marcar y al barrer, y la compactación usa el 31 y el 23.
- **Un solo bit basta:** 1 es «todo ASCII, seguro» (el rápido); 0 es «no lo sé, o no lo es» (el lento).
- **Por defecto, 0.** Cualquier camino que no lo ponga sólo va más despacio, nunca mal.
- **Un `byte[]` nunca lo lleva:** su contenido cambia, y para ser string pasa por `fromBytes`, que copia y mira.

**Cuándo se pone: al crear, mirando mientras se copia.** Casi toda cadena nace copiando o generando sus bytes, así que
mirar si alguno vale 0x80 o más cuesta casi nada (de 4 en 4 bytes: un OR y una máscara por palabra).
- `a + b`, `substring`, `fromBytes`, `toString` del `StringBuilder`, `upper`, `lower`, `trim`, `replace`, los trozos
  de `split`, `readFile` y `readLine`: miran al copiar.
- `intToString` y los demás números: siempre ASCII.
- `__charsToString` (los `appendChar`): mira los códigos (menores de 128).

**Lo que hace cada operación con los dos algoritmos:**

| operación | rápido (todo ASCII) | lento (hay especiales) |
|---|---|---|
| `strlen` | los bytes: O(1) | contar: O(n) |
| `charAt`, `charCodeAt`(i) | el byte `i`: O(1) | caminar hasta `i`: O(i) |
| `substring` | los bytes [a, b): O(1) para situarse | caminar hasta a y b |
| `indexOf` | el resultado ya es el índice | contar hasta lo encontrado |
| `upper`, `lower` | de 4 en 4 | carácter a carácter |
| buscar, comparar, `==` | por bytes | por bytes también: el UTF-8 lo permite |

**Las consecuencias, para decidir:**
1. **El texto con acentos va por el lento.** «canción» o «año» tienen caracteres especiales: un bucle por índice sobre
   ellos sigue siendo n². Lo que lo resuelve sin tercer algoritmo:
   - la stdlib (`Json`, `Str`, `Orm`) recorre por bytes (`byteAt`), haya acentos o no: lo que mira es ASCII;
   - para los programas, una forma de recorrer sin índice (un `for each` de caracteres), que es lineal en los dos
     casos.
2. **Los literales no tienen etiqueta:** irían por el lento, que con lo cortos que suelen ser apenas se nota. El
   compilador sabe si un literal es ASCII, pero no hay dónde decirlo sin cambiar el formato.
3. **La marca tiene que ser verdad mientras exista la cadena:**
   - `move` no puede escribir en un string (`#714`): que lo rechace, o que borre la marca;
   - con `any`, dentro de `Core`, un `byte[]` no puede pasar por string sin copiarse.
4. **Las dos VMs, igual.** La VM-Java pone el mismo bit en su etiqueta: el mismo algoritmo, y la paridad lo vigila.
5. **El UTF-8 mal formado (`#711`)** es del lento, así que su regla sigue pendiente.

**Lo que da, medido con la pieza equivalente** (`byteAt`, que es lo que haría el rápido): recorrer 4000 caracteres
ASCII en la Pico 2, 37 ms frente a 1986. Unas 50 veces menos; y el bucle pasa de n² a n.

**El bit, aprobado** (Eduardo, 21-oct: *«OK, me parece bien lo del bit»*). Siguen por decidir los puntos de arriba
(los acentos por el lento, los literales, `#714`, `#711`).

**La longitud: se guarda, y no hay `\0`** (Eduardo: *«con el string también se guarda la longitud o el final lo marca
el caracter \0?»*).
- **Se guarda la longitud en BYTES**, en los 4 bytes de delante:
  - una cadena del heap es `[etiqueta][longitud][bytes]`;
  - un literal es `[longitud][bytes]`.

  El final lo marca la longitud. No hay `\0` detrás (`heap_alloc_string` copia sus bytes justos). Un `\0` dentro es un
  carácter más: comprobado el 21-oct en las dos VMs, `a\0b` tiene `strlen` 3 y se concatena y se compara bien.
- **Lo que da frente al `\0` de C:**
  - los bytes se saben sin recorrer (`__bytesDe`, O(1));
  - una cadena puede llevar cualquier byte;
  - concatenar y `substring` saben cuánto reservar antes de copiar.
- **Lo que no se guarda: la longitud en CARACTERES**, que es lo que devuelve `strlen`, y hoy se cuenta en cada llamada.
  - **Con el bit,** en una cadena ASCII los caracteres son los bytes: O(1).
  - **Una opción más, por decidir:** en la misma etiqueta quedan, además del bit, 21 bits libres. Cabe la cuenta de
    caracteres de cualquier cadena de hasta 2 097 151, contada al crearla en la misma pasada que el bit. Así `strlen` y
    el rango de `charAt`/`charCodeAt`/`substring` serían O(1) también con acentos; sólo caminar hasta el carácter `i`
    seguiría siendo O(i). Los literales, sin etiqueta, como hoy. A revisar al hacerlo: que ningún camino reescriba la
    etiqueta entera y la borre.
- **Un detalle para la biblioteca:** la longitud va en big-endian, como todo el formato del `.mod`. Leerla en estos
  procesadores, que son little-endian, pide darle la vuelta a los bytes: en la M33, una instrucción (`REV`); en las
  RISC-V sin Zbb, varias.
- **Donde la VM pasa una cadena a la libc** (un nombre de fichero, `parseInt`), la copia a un búfer de C con su `\0`.
  Ahí un `\0` de dentro sí cortaría: es la frontera con C, no la cadena.

### Los dos algoritmos, hechos y medidos (21-oct)

Eduardo: *«en todas las funciones podemos tener las 2 opciones, sin caracteres especiales métodos clásicos optimizados
para arrays de bytes, con caracteres especiales, dejamos lo que tenemos actualmente. Y con ese cambio se puede medir y
verificar a ver que tal funciona.»* Y a mitad: *«Copiar un string debería ser igual con caracteres especiales, se
puede hacer como una copia de bytes.»*

**Lo hecho, en las dos VMs** (primero la VM-Java, como manda la cascada):

- **El bit** «todo ASCII» (`0x00200000`) en la etiqueta: `TAG_ASCII_BIT` en la VM-Java y `BPVM_TAG_ASCII` en la VM-C.
  - **Lo pone quien crea la cadena**, por los bytes que escribe: `allocVmString` y `heap_alloc_string` (que cubren casi
    todo), `fromBytes`, `readLine`, `substring`, `charAt`, `upper`, `lower`, `trim`, `replace`, los trozos de `split`,
    `listDir`, las rutas y la concatenación del AOT.
  - **Concatenar** (`__strCopy`) le pone el «Y» de los trozos.
  - **Un `byte[]` nunca lo lleva.**
  - **`move` lo quita del destino**, así que la marca no puede mentir (`#714` sigue abierta: `move` todavía escribe).
  - **Un literal no tiene etiqueta:** se mira entero cuando hace falta (suele ser corto).
- **El rápido**, si la cadena lleva el bit, en `strlen`, `charAt`, `charCodeAt`, `substring`, `indexOf`, `upper` y
  `lower` (y en la VM-Java también `trim`, `startsWith`, `endsWith` y `contains`). Con caracteres especiales, el lento
  de siempre, sin tocar: lo que hace hoy cada VM, también lo que no casa entre ellas (`#711`, `#717`).
- **Copiar es una copia de bytes en los dos caminos** (la VM-C): `memcpy` en vez de los bucles byte a byte en
  `substring`, `toBytes`/`fromBytes`, `trim`, `split`, `replace` (cada trozo de una vez), las rutas,
  `heap_alloc_string` y la concatenación del AOT. En la VM-Java no: su lento pasa por un `String` de Java, que cambia el
  UTF-8 mal formado, y cambiarlo es la decisión de `#711`.
- **Lo que no cambia:** el formato del `.mod`, el compilador, la stdlib y la ABI del código nativo. Sólo la imagen.

**Verificado:**
- **`StrDosCaminos`** (nuevo, en el corpus de la paridad) pregunta cada función por los dos caminos sobre el mismo
  contenido (el lento, con el bit quitado por `move`): 140 comparaciones, 0 distintas.
  - Las 158 líneas de su salida son las mismas en la VM-Java nueva, en la VM-C nueva y en la VM-C de antes del cambio,
    que iba siempre por el lento.
  - `move` con una `ñ` dentro deja `strlen` en 3: el bit se fue.
- **Las pruebas de la mañana** (el UTF-8 mal formado, sus orígenes, el `\0`, `move`) dan lo mismo que antes del cambio.
- **`compat.sh check`** con las dos VMs nuevas: paridad 151 PASS, la cápsula de V6 59 PASS (los demás frentes, en
  FICHAS).

**Medido** (el censo y los programas de la mañana, con la imagen nueva en la Pico 2, la Metro y la P4), con 2048 bytes:

| | Pico 2 | Metro (PSRAM) | P4 |
|---|---|---|---|
| recorrer con `charCodeAt` | 535 → **20 ms** | 556 → **20 ms** | 228 → **16 ms** |
| recorrer con `charAt` | 581 → **89 ms** | 623 → **78 ms** | 240 → **29 ms** |
| un `charCodeAt(s, 1024)` | 261 → **5,8 µs** | 276 → **5,6 µs** | 107 → **3,3 µs** |
| `strlen` | 132 → **4,1 µs** | 139 → **4,0 µs** | 65 → **1,9 µs** |
| `substring` (la mitad) | 606 → **131 µs** | 826 → **275 µs** | 203 → **31 µs** |
| `trim` | 952 → **215 µs** | 1,5 ms → **518 µs** | 351 → **55 µs** |
| `upper` | 1,1 ms → **432 µs** | 1,5 ms → **885 µs** | 356 → **164 µs** |
| `toBytes` | 460 → **288 µs** | 800 → **551 µs** | 114 → **56 µs** |

Y en programas de verdad:

| | antes | ahora |
|---|---|---|
| leer un JSON de 8 KB, Pico 2 | 16,7 s | **2,5 s** |
| leer un JSON de 8 KB, P4 | 6,8 s | **1,1 s** |
| leer un JSON de 8 KB, Metro | — | 2,4 s |
| leer un JSON de 1 / 2 / 4 / 8 KB, Pico 2 | 0,5 / 1,5 / 4,8 / 16,7 s (n²) | 0,3 / 0,6 / 1,2 / 2,5 s (n) |
| recorrer 4000 caracteres, Pico 2 | 1986 ms | **38 ms** |

**Lo que no cambia, como se esperaba:**
- el `Map` (2000 claves, 56 s en la P4): su parte cuadrática es desplazar la lista al meter, en BP;
- `==` (un bucle de bytecode);
- concatenar en un bucle;
- el `StringBuilder`;
- el GC.

`recorrer con charAt` baja menos que `charCodeAt` porque cada `charAt` crea una cadena: queda el coste de reservar.

**Lo que no cuadra, sin explicar todavía:** en las RP2350 (no en la P4), unas pocas operaciones salen más lentas, y se
repite igual en dos pasadas:
- `intToString`: 45 → 68 µs;
- `floatToString`: 44 → 56 µs;
- `parseFloat`: 35 → 50 µs;
- cadena + entero: 199 → 245 µs.

El bucle vacío del censo es el mismo antes y ahora (23,3 y 23,6 µs), y `parseFloat` no pasa por nada de lo que se
cambió. Lo que tienen en común es que llaman a funciones grandes de la libc (`snprintf`, `strtod`), que se ejecutan
desde la flash por la caché de 16 KB del XIP. La hipótesis, sin demostrar, es que la imagen nueva movió el código y
ahora esas funciones y el intérprete se pisan en la caché (ya pasó con PrintBench: ±10 % de una imagen a otra). Lo
probaría un test de desplazamiento: mover el código sin cambiar nada más.

### El lento, con dos contadores: bytes y caracteres (propuesta, Eduardo 21-oct; decidida y hecha ese día, abajo)

Eduardo, antes de seguir con las placas: *«Deberíamos ver si podemos hacer mas mejoras. Por ejemplo, find o indexof.
Por si solas si hay caracteres especiales iríamos por el método lento. Ahora si después se hace un substring o algo
similar podemos optimizar si trabajamos con punteros de C o con dos contadores de bytes y de caracteres.»*

**Lo que cuesta hoy el lento**, medido en la Pico 2 con `samples/benchmarks/StrAcentos.bp` (cadenas que empiezan por
`ñ`, así que van por el lento):

| caracteres | 500 | 1000 | 2000 | 4000 |
|---|---|---|---|---|
| recorrer por índice (`charCodeAt`) | 36 ms | 134 ms | 509 ms | **1992 ms** (n²) |
| buscar y cortar (`indexOf` + `substring`), ×100 | 32 ms | 59 ms | 117 ms | 222 ms |

**Dónde se cuenta dos veces** (lo que el lento hace hoy en cada llamada):
- `indexOf` encuentra un BYTE y lo convierte a carácter, contando desde el principio;
- `substring(s, i, j)` vuelve a contar la cadena entera (para recortar el rango) y luego camina desde el principio
  hasta `i`, y otra vez desde el principio hasta `j`: tres pasadas;
- `charAt`/`charCodeAt(s, i)`: cuenta la cadena entera y luego camina hasta `i`: dos pasadas;
- en un bucle, cada vuelta empieza otra vez desde el principio: n².

**La propuesta, en dos escalones:**
1. **Dentro de cada llamada (sin guardar nada):** una sola pasada con los dos contadores a la vez. `substring` camina
   desde el principio hasta `i` (anota el byte) y sigue hasta `j` (anota el otro); si se acaba la cadena antes, eso es
   el recorte. `charAt` camina hasta `i`, y si se acaba antes, está fuera de rango. Pasa de tres o dos pasadas a una, y
   para un `i` cerca del principio casi nada. Los bucles siguen siendo n², más deprisa.
2. **Entre llamadas: un cursor por hilo con los dos contadores.**
   - **Qué guarda:** cada hilo recuerda la última cadena del lento (su handle, con la generación), el último carácter
     que buscó, el byte donde empieza y, si se contó, cuántos caracteres tiene.
   - **Cómo se usa:** la llamada siguiente sobre la misma cadena sigue desde ahí, hacia delante o hacia atrás (el UTF-8
     se puede recorrer al revés).
   - **Lo que da:**
     - recorrer `charAt(s, i)` con `i` creciendo pasa a ser lineal también con acentos (cada vuelta, un carácter);
     - `indexOf` deja el cursor en lo encontrado, y el `substring(s, i, …)` de después empieza ahí;
     - `strlen` repetido sale directo.
   - **Por qué es seguro:** lo da el invariante (una cadena no cambia mientras existe):
     - la generación del handle distingue una cadena muerta de la que ocupa su sitio;
     - la compactación no estorba, porque el cursor guarda posiciones desde el principio de la cadena y no
       direcciones;
     - `move` sube un contador de la VM (una «época») que invalida todos los cursores;
     - cada hilo tiene el suyo, sin cerrojos.

**Lo que hay que decidir:**
- **los escalones:** el 1 solo, o el 1 y el 2. El 2 es estado escondido (un cursor), aunque nunca cambia un resultado,
  sólo el tiempo;
- **dónde:** en la VM-C, que es donde importa. La VM-Java decodifica a un `String` de Java y no cuenta, así que sin
  cursor haría de oráculo: la paridad compara la VM-C con cursor contra la VM-Java sin él, con todos los patrones de
  acceso (hacia delante, hacia atrás, al azar, dos cadenas a la vez, tras un `move`). La alternativa es ponerlo también
  en la VM-Java, por simetría.

**Lo que daría, estimado y por medir:** recorrer 4000 caracteres con acentos, de 1992 ms a unos 40 (como el ASCII); un
par «buscar y cortar» de 4000, de 2,2 a cerca de 1,4 ms.

### El lento con dos contadores, hecho y medido (21-oct)

**Lo que decidió Eduardo:** *«Yo haría el 1 y el 2. Y son optimizaciones que se pueden hacer muy bien en C porque se
pueden utilizar punteros. Creo que en Java no hace falta.»* Los dos escalones, sólo en la VM-C; la VM-Java sigue
contando desde el principio y hace de oráculo.

**Lo hecho** (`bpgenvm-c/src/builtins.c`, las primitivas de `bpvm_prims.h`):
- **Escalón 1, una pasada por llamada.** El carácter y el byte donde empieza avanzan juntos:
  - `substring` camina hasta `desde` y de ahí sigue hasta `hasta`; si la cadena se acaba antes, eso es el recorte, y
    ya no se cuenta entera para recortar;
  - `charAt` y `charCodeAt` caminan hasta `i`; si se acaba antes, está fuera de rango;
  - `strlen` cuenta una vez por cadena.
- **Escalón 2, el cursor del hilo** (`bpvm_strcur_t`, en `struct bpvm_thread`): la última cadena del lento, un carácter
  y el byte donde empieza y, si se contó, cuántos caracteres tiene.
  - La llamada siguiente camina desde el punto conocido más cerca: el principio, el cursor (hacia delante o hacia
    atrás) o el final, si se sabe cuántos caracteres hay.
  - `indexOf` cuenta desde el cursor y lo deja en lo que encontró. Un `substring(s, 0, i)` justo después usa ese
    cursor para `hasta`.
- **Por qué no puede mentir:**
  - la clave es la referencia ENTERA: el handle con su generación, o la dirección de un literal;
  - guarda posiciones desde el principio de la cadena, no direcciones, así que la compactación no le afecta;
  - `move` sube una época de la VM (`vm->str_epoca`) DESPUÉS de escribir, con RELEASE, y eso invalida todos los
    cursores. El hilo la lee con ACQUIRE: con dos runs, quien ve la época nueva ve también los bytes nuevos.
- **Lo que queda como estaba:**
  - una cadena que EMPIEZA por un byte de continuación va por el lento de siempre, sin cursor: con `utf8_cp_count`
    tiene un carácter menos que caminando (`#711`);
  - las natives (el AOT) no tienen hilo del intérprete: cada llamada usa un cursor propio que sólo dura esa llamada
    (sólo el escalón 1);
  - lo que devuelve cada función, también lo que no casa entre las VMs (`#711`, `#717`).
- Ni el formato del `.mod`, ni el compilador, ni la stdlib, ni la ABI del código nativo: sólo la imagen.

**Verificado:**
- **`bpgenvm-c/samples/StrCursor.bp`**, nuevo y en el corpus de la paridad: 9872 comprobaciones contra un oráculo
  escrito en BP (decodifica el UTF-8 sobre los bytes con `byteAt`, sin pasar por el cursor) y contra la VM-Java.
  - Patrones: hacia delante, hacia atrás, a saltos, al azar, fuera de rango, desde el final, dos cadenas a la vez,
    buscar y cortar, tras un `move` que cambia dónde empiezan los caracteres, tras un `gc()`, con handles reciclados,
    con literales y con dos hilos a la vez.
  - Su salida es la misma en la VM-Java, en la VM-C con cursor y en la VM-C de antes.
- **La prueba ve los fallos.** Con cuatro sabotajes, uno cada vez:
  - sin subir la época en `move`: 3 malas;
  - la clave sin la generación: 78 malas. La primera versión de la prueba no lo veía, porque no conseguía que la
    cadena nueva ocupara el número de la muerta. La de ahora no reserva ni suelta nada entre una y otra;
  - el paso atrás sin saltar los bytes de continuación: 4204 malas;
  - el final con un carácter de menos: 457 malas en la primera sección, y la ejecución se rompe.
- **UTF-8 mal formado**, comparado con la VM-C de antes y no con la VM-Java (`#711`): 11 cadenas, cada función en
  todos los índices y todos los trozos, y 120 llamadas al azar mezcladas. Las 2388 líneas son iguales.
  - Las cadenas: que empiezan por un byte de continuación, continuaciones sueltas, cortadas al final, una cabecera sin
    continuación, bytes imposibles, caracteres de 4 bytes y otras.
- **`StrDosCaminos`:** las mismas 158 líneas.
- **`compat.sh check` VERDE:**
  - paridad 152 PASS (con `StrCursor`), en MOD9, en MOD8 y con dos workers;
  - la cápsula de V6, 59;
  - las pruebas de C, 102/102 (entre ellas `test-aotstr`, las entradas del AOT);
  - sombra, 15/15;
  - el estrés y `packs-abi`.
- **La pila:** el marco de `bpvm_call_builtin` no cambia (2128 B en ARM, con los flags del firmware). Las primitivas
  crecen; la que más, `substring`, de 56 a 112 B, más 24 de la caminata. En la Pico 2, a la pila del run le quedan
  12 296 B libres, 48 menos que con la imagen de antes.

**Medido en el PC** (`samples/benchmarks/StrCursorBench.bp pc`, la VM-C de antes contra la de ahora; una `ñ` cada
diez caracteres):

| n caracteres | 4000 | 8000 | 16 000 | 32 000 |
|---|---|---|---|---|
| recorrer hacia delante (`charCodeAt`) | 6 → 0 ms | 26 → 0 | 108 → 1 | **413 → 1** |
| recorrer hacia atrás | 8 → 0 | 28 → 1 | 108 → 1 | **418 → 2** |
| `substring(s, k, k+1)` con `strlen(s)` en la condición | 12 → 1 | 49 → 2 | 194 → 7 | 761 → 27 |
| buscar y cortar, ×100 | 2 → 1 | 2 → 2 | 5 → 3 | 9 → 6 |
| dos cadenas a la vez | 13 → 8 | 52 → 31 | 201 → 124 | 805 → **497** |

Y un programa de verdad, `Json.parseJson` con `"ñandú"` en cada objeto:

| | 8 KB | 16 KB | 32 KB | 64 KB |
|---|---|---|---|---|
| antes | 54 ms | 247 | 1164 | 4258 |
| con cursor | 6 | 34 | 354 | 971 |
| antes, sin GC (`--nogc`) | 54 | 209 | 815 | 3299 |
| con cursor, sin GC | 6 | 12 | 22 | **44** |

Sin GC es lineal; lo que crece todavía con GC es la lista de huecos (`#715`). Un `Map` con claves con `ñ` no cambia:
lo cuadrático es desplazar la lista al meter, en BP.

**Medido en las placas** (antes, con la imagen de los dos algoritmos que llevaban; después, grabadas con la nueva). Con
4000 caracteres, `StrAcentos` y los patrones de `StrCursorBench`:

| | Pico 2 | Metro (PSRAM) | P4 |
|---|---|---|---|
| recorrer hacia delante | 1992 → **44 ms** | 2103 → **46** | 835 → **34** |
| recorrer hacia atrás | 2133 → **45** | 2302 → **47** | 906 → **33** |
| `substring(s, k, k+1)` con `strlen(s)` | 4417 → 385 | 4818 → 429 | 1826 → 149 |
| buscar y cortar, ×100 | 222 → 76 | 235 → 83 | 95 → 39 |
| dos cadenas a la vez | 4238 → 1461 | 5446 → 1891 | 1795 → 543 |

Y el JSON con acentos (1, 2, 4 y 8 KB):

| | Pico 2 | Metro (PSRAM) | P4 |
|---|---|---|---|
| antes | 0,5 / 1,5 / 4,7 / **16,1 s** | 0,6 / 1,7 / 5,3 / **19,7 s** | 0,2 / 0,6 / 1,9 / **6,6 s** |
| con cursor | 0,3 / 0,6 / 1,3 / **2,7 s** | 0,3 / 0,6 / 1,2 / **2,6 s** | 0,1 / 0,3 / 0,5 / **1,1 s** |

Lineal en las tres. Recorrer 4000 caracteres con acentos cuesta ya casi lo mismo que sin ellos (44 ms frente a 38 en la
Pico 2). En buscar y cortar queda la búsqueda de bytes de `indexOf`: unos 186 ns por byte en la Pico 2, el terreno de la
biblioteca de cadenas en C. En la P4, los packs aguantaron sin regrabar: misma página, `packRamBase` `0x48001000`, Gui
y SQLite vivos. El `Map` de 500 claves no cabe en la Pico 2, con la imagen de antes ni con la nueva.

**Lo que no arregla un cursor solo:**
- **Dos cadenas a la vez.** Con un solo cursor, saltar de una a otra empieza cada vez desde el principio. Comparar dos
  cadenas con acentos carácter a carácter sigue siendo n²: va ×3 más deprisa (la pasada única), pero doblar la cadena
  sigue cuadruplicando el tiempo (en la Pico 2, 31, 106, 384 y 1461 ms).
- **Una cadena con acentos tocada en medio.** En `substring(s, k, k+1)` con `strlen(s)` en la condición, el trozo de
  una `ñ` se lleva el cursor (su `strlen`), y la vuelta siguiente vuelve a contar `s` entera. Por eso aún crece más
  que lineal (en el PC 1, 2, 7 y 27 ms).
- **Propuesta, sin hacer:** dos cursores por hilo, las dos últimas cadenas; la más vieja deja su sitio. Con eso, los dos
  casos serían lineales. Cuesta 24 bytes más por hilo y una comparación más por llamada.

### Comparar por bytes (análisis, 21-oct)

Eduardo, al ver que dos cadenas a la vez siguen siendo n²: *«La comparación se puede hacer como si fuera byte a byte en
vez de caracter a caracter?»*

**Sí, y da exactamente lo mismo.** El UTF-8 está hecho así a propósito (RFC 3629): ordenar por bytes, de 0 a 255, da el
mismo orden que por puntos de código. Y dos cadenas son iguales carácter a carácter si y sólo si lo son byte a byte.
- Si difieren en el carácter i, comparten los bytes de los i anteriores. El primer byte distinto está dentro del
  carácter i, y los bytes de cabecera y de continuación crecen con el punto de código.
- Si una es prefijo de la otra en caracteres, también lo es en bytes.
- `Str.compare` sólo devuelve -1, 0 o 1, así que por bytes devolvería los mismos valores, no sólo el mismo signo.
- Las dos VMs dan hoy el punto de código en `charCodeAt` (la VM-Java, con `codePointAt`): `Str.compare` ordena por
  punto de código en las dos.

**Comprobado en las dos VMs.** Pares de cadenas casi iguales: una cambia un carácter, se corta o crece. Llevan
caracteres de 1, 2, 3 y 4 bytes, también los de los bordes de cada longitud (U+007F/U+0080, U+07FF/U+0800, U+FFFD,
U+FFFF/U+10000 y un emoji). Cada par se compara con `Str.compare` y con un bucle por bytes (`byteAt`), en los dos
sentidos. Resultado: 6000 comparaciones, 0 distintas en la VM-C y 0 en la VM-Java, con los mismos recuentos en las
dos (1059 pares con a < b y 837 iguales).

**Dónde sirve:**
- **`Str.compare`**, el comparador por defecto del `Map`. Hoy es L² con acentos: compara `charCodeAt(a, i)` con
  `charCodeAt(b, i)`, y el cursor salta de una cadena a la otra. Por bytes, una pasada sin contar caracteres. Es el
  caso principal de «dos cadenas a la vez».
- **`Str.lastIndexOf`**: comparar por bytes y convertir a índice de carácter una sola vez al final, como ya hace
  `indexOf`.
- **`==`** ya compara bytes (un bucle de bytecode).

**Lo que no puede ir por bytes:** lo que necesita el índice o el valor de un carácter (`charAt`, `substring` por
índice, las mayúsculas fuera de ASCII).

**Dos formas de hacerlo**, con el mismo resultado:
1. En BP, en `Str.bp`, con el `byteAt` (`inline ALOAD_U8`) ya probado: sin tocar la VM, y la paridad sale por
   construcción. Es la «stdlib por bytes» de «Lo cuadrático» (filas 1 y 3).
2. En C, en la biblioteca de cadenas (un `memcmp`): más rápido con cadenas largas, pero es un builtin nuevo en las dos
   VMs.

El `Map` seguiría cuadrático al meter, por otra razón: desplaza la lista en BP (fila 3). Esto arregla la comparación,
no eso. Por decidir (Eduardo).

**Hecho el mismo día, la forma 1** (Eduardo: *«OK, la 1»*): `Str.compare` y `Str.lastIndexOf` van por bytes con un
`byteAt` privado (`inline ALOAD_U8`). `lastIndexOf` convierte el byte encontrado a índice de carácter una vez, al final,
contando los bytes que no son de continuación.
- **La interfaz de `Str` no cambia:** las dos funciones nuevas son privadas, y un programa compilado contra la `Str` de
  antes carga con la nueva, y al revés.
- **Una trampa, cazada antes de entrar:** compilar `Str.bp` SUELTO lo deja con la generación 0, y `Collections`, que se
  compiló contra la 7, ya no carga («'Str' (gen 0) es mas antiguo que el que 'Collections' necesita (gen 7)»). La
  generación es la `version` del proyecto (`Stdlib.bpbuild`). Un módulo de la stdlib se rehace con el proyecto, y de
  `out/` se copia el que cambió: los otros 29 salieron idénticos byte a byte.
- **Ninguna imagen lleva `Str` embebida** (las tres familias embeben otros 16–17 módulos): no hay que rehacer
  firmwares. En una placa, `Str` vive en `/lib`; se subió con el wire a la Pico 2, la Metro y la P4.

**Verificado:**
- **`bpgenvm-c/samples/StrPorBytes.bp`**, nuevo y en el corpus: las de antes, copiadas tal cual, contra las nuevas.
  Usa pares casi iguales con caracteres de 1 a 4 bytes, agujas sacadas de la cadena y que no están, vacías y sin valor.
  Resultado: 7948 comprobaciones, 0 malas, igual en las dos VMs.
- **El sabotaje:** con los bytes leídos con signo, 430 malas. La prueba lo ve, y corre la `Str` nueva.
- **`compat.sh check` VERDE:** paridad 153 (MOD9, MOD8 y dos workers), cápsula 59, pruebas de C 102/102, sombra
  15/15, estrés, `packs-abi` y la stdlib embebida. `packs/Stdlib.pack`, regenerado: igual a `bpstdlib/*.mod`.

**Medido** (`StrCursorBench`; `Str.compare` de dos cadenas con acentos iguales salvo el último carácter, cinco veces):

| n caracteres | PC | Pico 2 | Metro (PSRAM) | P4 |
|---|---|---|---|---|
| 500 / 4000 (placas) | — | 160 / 7348 → 44 / **348 ms** | 195 / 8480 → 46 / **360** | 76 / 2750 → 36 / **285** |
| 4000 / 32 000 (PC) | 41 / 2501 → 1 / **13 ms** | — | — | — |

De cuadrática a lineal. En placa, una comparación por bytes en BP cuesta unos 15 µs por byte (el intérprete); un
`memcmp` en C, la forma 2, iría mil veces más deprisa. El `Map` con claves con acentos apenas cambia (Pico 2, 200
claves: 1088 → 1048 ms): con claves cortas pesa desplazar la lista, abajo.

**¿Y un CRC para descartar la igualdad? (21-oct)** Eduardo: *«Podemos utilizar CRC para descartar la igualdad, o el
comparador»*, y enseguida: *«el comparador sería igual o mas rápido? Igual con utilizar el comparador nos vale»*. Nos
vale el comparador:
- **si son distintas, el comparador para en el primer byte distinto**, que suele estar al principio. Y `==`
  (`__strequals`) ni mira los bytes si las longitudes no coinciden;
- **un CRC hay que calcularlo sobre TODOS los bytes** de cada cadena, más trabajo que comparar hasta la primera
  diferencia. Sólo compensaría guardado (en la etiqueta quedan 21 bits libres), con cadenas de la misma longitud que
  difieren al final y que se comparan muchas veces. Y calcularlo al crear encarecería crear, que ya es lo que más pesa;
- **si son iguales, el CRC no ahorra nada**: dos CRC iguales no prueban la igualdad, y hay que comparar los bytes;
- **para ordenar** (el `Map`) no sirve: el CRC no conserva el orden alfabético;
- **donde un hash sí gana** es en una tabla hash, que no busca comparando sino por el hash. Es otra estructura, sin
  orden.

Lo que de verdad pesa es que el comparador corre en bytecode (unos 15 µs por byte en placa: `Str.compare` y el bucle de
`==`). Compararlo en C, con un `memcmp`, iría unas mil veces más deprisa por byte: es la forma 2 de arriba.

### Lo que sigue lento (21-oct, pregunta de Eduardo)

Eduardo: *«De todas las operaciones de strings, entiendo que ahora en el caso no haber caracteres especiales todas van
mas rápidas. Ahora, si hay caracteres especiales queda alguna función que vaya como antes de lenta?»*

**Sin caracteres especiales, casi todas, no todas:**
- **más rápidas:**
  - `strlen`, `charAt`, `charCodeAt`, `substring`, `indexOf`, `upper` y `lower`, por el camino rápido;
  - lo que copia, con `memcpy`: `substring`, `trim`, `split`, `replace`, `toBytes`/`fromBytes` y las rutas;
- **igual que antes:**
  - `==`, un bucle de bytecode que ya comparaba bytes;
  - concatenar, ya en C desde `#554`;
  - `contains`, `startsWith` y `endsWith` en la VM-C, que ya iban por bytes;
  - el `StringBuilder`;
- **más lentas, sin explicar, sólo en las RP2350:** `intToString`, `floatToString`, `parseFloat` y cadena + entero, un
  25–50 % (hipótesis: el sitio del código en la caché del XIP; pide un test de desplazamiento);
- **los literales:** no tienen etiqueta, y cada llamada los mira enteros para saber si son ASCII. Con uno corto no se
  nota. Recorrer por índice uno largo sigue siendo cuadrático: con 4000 caracteres, **798 ms en la Pico 2, frente a 39**
  con la misma cadena en el heap (en el PC no se ve: el compilador vectoriza ese bucle).

**Con caracteres especiales, lo que queda como antes:**
- **dos cadenas con acentos a la vez, por índice, en un programa:** el cursor salta de una a otra. Sigue cuadrático,
  tres veces más deprisa (Pico 2: 4238 → 1461 ms). En la stdlib ya no queda ninguno: eran `Str.compare` y
  `Str.lastIndexOf`;
- **los literales largos**, con o sin acentos: 881 ms frente a 44 en la Pico 2;
- **`upper` y `lower`:** el código de antes, carácter a carácter. Lineal, pero no más rápido;
- **las natives** (el código AOT): sólo la pasada única. Un bucle por índice en una native sigue siendo cuadrático;
- **la VM-Java** (el PC): sin cursor, por decisión;
- **lo que no depende de los acentos:** `==` en bytecode, concatenar en un bucle (n²) y el GC (`#715`). El `Map` al
  meter era el cuarto: arreglado el mismo día (abajo, «El Map»).

**Lo que lo arreglaría, por decidir:**
- para dos cadenas a la vez, dos cursores por hilo;
- para los literales, que el cursor recuerde también lo que vio de un literal (si es ASCII y cuántos caracteres
  tiene), y así no mirarlo en cada llamada. Sin tocar el formato del `.mod`;
- para las natives, el cursor del hilo.

### El Map: la búsqueda dicotómica ya está; lo cuadrático es desplazar (21-oct)

Eduardo: *«El Map no está optimizado para ser rápido. Se pensó en una lista ordenada como solución de compromiso. Aquí
podemos hacer 2 cosas: 1 Utilizar la lista como si fuera un array y hacer una búsqueda dicotómica. 2 Sustituir la
lista por un arbol binario. Yo empezaría por la 1 ya que es mas sencilla de implementar.»*

**Lo que hace hoy** (`bpstdlib/Collections.bp`):
- **buscar ya es la 1:** `bsearch` busca de forma dicotómica sobre la lista de claves usada como array, con `get(mid)`.
  Lo usan `put`, `get`, `containsKey` y `remove`: log₂ n comparaciones;
- **meter desplaza elemento a elemento en BP:** `insertAt` añade al final y baja cada elemento posterior con
  `keyList.set(i, keyList.get(i - 1))` y lo mismo en `valList`, cuatro llamadas a métodos por elemento. Con las claves
  en desorden, cada `put` desplaza de media la mitad de la lista: n²;
- **borrar también desplaza:** `List.remove` (Core), con un bucle de bytecode.

**Medido en el PC** (`put` de N claves de cinco cifras, en orden y en desorden; en orden, cada una va al final y no se
desplaza nada):

| N | 250 | 500 | 1000 | 2000 | 4000 | 8000 |
|---|---|---|---|---|---|---|
| en orden (buscar) | 2 ms | 4 | 9 | 20 | 48 | 91 |
| en desorden (buscar y desplazar) | 5 | 19 | 69 | 275 | 997 | **3970** |

Con 8000 claves, desplazar es el 97 % del tiempo.

**Dentro de la 1, lo que falta es desplazar de una vez.** `move` (un `memmove` en C, en las dos VMs) acepta arrays de
referencias, y `List.backing()` es público. `insertAt` y `remove` pueden mover el tramo con UNA llamada en vez de un
bucle de BP:
- sigue siendo O(n) por inserción, pero mover 2000 referencias son 16 KB de `memmove`;
- y todo queda dentro de `Collections`: sin tocar Core, cuya interfaz sellan todos los módulos.

**Hecho el mismo día** (la 1 de Eduardo, completa): `insertAt` añade al final y mueve el tramo `[ip, n)` un sitio a la
derecha con `move` en las dos listas; `remove` mueve el tramo de detrás un sitio a la izquierda y quita la última
entrada, que ha quedado repetida.
- **La interfaz de `Collections` no cambia.** Un primer intento con un método privado nuevo (`removeAt`) la cambiaba
  (`d96b9491` → `7145181c`): los métodos de una clase van en su interfaz aunque sean privados. Con MOD9 un programa
  compilado contra la de antes cargaba igual (enlace tardío, sólo un aviso), pero con MOD8 se rechazaría. El código
  quedó dentro de `remove`, y la interfaz es la de antes (`Stdlib.mod`, que la sella, sale idéntico).
- **Verificado:**
  - `bpgenvm-c/samples/MapDesplaza.bp`, nuevo y en el corpus: 3000 operaciones al azar (meter, actualizar, borrar,
    buscar) sobre un `Map` numérico y otro de cadenas, contra un array de referencia. Cada 50 operaciones, el `Map`
    entero en orden. Resultado: 51 222 comprobaciones, 0 malas, igual en las dos VMs y con la `Collections` de antes;
  - con un sabotaje (un elemento de menos en el `move`), la prueba revienta: el `Map` sale desordenado.
- **Medido en el PC** (`put` de N claves en desorden; en orden no cambia):

| N | 250 | 500 | 1000 | 2000 | 4000 | 8000 |
|---|---|---|---|---|---|---|
| antes | 5 ms | 19 | 69 | 257 | 996 | 3934 |
| con `move` | 2 | 4 | 9 | 18 | 40 | **100** |

Meter en desorden cuesta ya lo mismo que en orden: lo que queda es buscar (log n comparaciones, cada una un
`Str.compare` en BP). La 2 de Eduardo, el árbol, sólo ganaría en ese O(n) del `memmove`, que con miles de claves no
pesa.

**Medido en las placas** (`Collections` nueva subida a `/lib`; ninguna imagen la embebe). `put` de N claves en
desorden, antes → ahora, y entre paréntesis en orden:

| N | Pico 2 | Metro (PSRAM) | P4 |
|---|---|---|---|
| 250 | 1427 → 529 ms (542) | 1590 → 662 (612) | 1124 → 347 (346) |
| 1000 | 17 710 → **2603** (2622) | 19 215 → 3641 (2896) | 14 796 → 1703 (1690) |
| 2000 | no cabe en el heap | 72 900 → **9301** (6289) | 56 722 → **3725** (3664) |

- En la Pico 2 y la P4, meter en desorden cuesta lo que en orden.
- En la Metro se nota todavía el `memmove`: mueve dentro de la PSRAM (unos 3 s de 9,3 con 2000 claves).
- Lo que queda, unos 2,6 ms por `put` con 1000 claves en la Pico 2, son las comparaciones: unas diez por `put`, cada una
  un `Str.compare` en bytecode (unos 15 µs por byte) tras dos llamadas a métodos. Es el terreno de la forma 2 de
  «Comparar por bytes», el `memcmp` en C.

### Comparar en C: el builtin 282, y `<`/`>` entre cadenas (21-oct)

Eduardo: *«Por supuesto, hazlo en C y a ver que tal.»* Por el camino, tres preguntas suyas:
- **¿Hay ya un opcode que compare cadenas?** No. `EQ`/`NEQ` comparan números o referencias. `==` entre cadenas es
  `__strequals`, una función que el compilador escribe en bytecode en cada módulo; `Str.compare` era BP; en C sólo
  estaban `startsWith`/`endsWith` (sí o no) y la igualdad del AOT.
- **¿Y `<` y `>` para cadenas?** Compilaban, pero daban basura: `#713`. Eduardo: comparar el texto de verdad.
- **¿Opcode o builtin?** Desde BP es igual: los dos se usan con una función `inline`. Un opcode cambia el juego de
  instrucciones, y en el AOT pediría una función más en la tabla de helpers (sube la ABI del `.mdn`). Se ahorraría unos
  3 µs por comparación en la Pico 2. Eduardo: *«el built es suficiente, no hay problema»*.

**Lo hecho:**
- **el builtin 282, `__strCompare(a, b)`:** un `memcmp` por bytes en la VM-C y el mismo bucle sobre los bytes en la
  VM-Java (sin `String` de Java, que cambiaría el UTF-8 mal formado). Devuelve -1/0/1, y una cadena sin valor cuenta
  como `""`. Va en `Core` sin `public`, como otros del sistema: la interfaz de Core no cambia (`Core.mod` y
  `Stdlib.mod` salen idénticos);
- **`Str.compare`** lo llama con su propio `inline CALL_BUILTIN 282`;
- **`<`, `<=`, `>` y `>=` entre cadenas** comparan su resultado con 0 (`#713`, cerrada). En una native, el AOT llama al
  mismo builtin por el despachador común (`call_builtin`), sin tocar la ABI;
- **`==` y `!=`:** el `__strequals` del compilador conserva sus reglas (null sólo es igual a null; si las longitudes no
  coinciden, distintas sin leer bytes) y compara el contenido con el builtin, en vez de su bucle de bytecode.

**Una trampa del build:** el proyecto de la stdlib compila por fechas. Tras cambiar el COMPILADOR, `out/` daba por
buenos los `.mod` de antes (salía distinto sólo `Str.mod`). Rehecho desde cero, cambian los seis que usan `==` entre
cadenas: Gui, IO, Json, Machine, Rtc y Str. IO, Machine y Rtc van embebidos en las imágenes: hay que rehacerlas.

**Lo que obliga:** todo lo compilado con el compilador nuevo que compare cadenas con `==` llama al builtin 282. Una
placa con una imagen anterior no lo tiene, y fallaría al llegar ahí. Hay que grabar las imágenes antes de mandarles
la stdlib o programas nuevos.

**Verificado:**
- **`bpgenvm-c/samples/StrOrden.bp`**, nuevo y en el corpus: los seis operadores contra una referencia carácter a
  carácter (`charCodeAt`), sobre pares casi iguales con caracteres de 1 a 4 bytes, literales y del heap, vacías y sin
  valor. Resultado: 18 636 comprobaciones, 0 malas, igual en las dos VMs. Incluye la línea de `#713` (`a < b: true`,
  sin perder el texto de delante) y un `switch` de cadenas;
- **`make test-aotcompara`**, nuevo y en la red: seis natives con los seis operadores, 490 comprobaciones. El AOT da lo
  mismo que el intérprete, y los cuatro relacionales salen por el builtin 282.
- **`compat.sh check` VERDE:** paridad 155 (MOD9, MOD8 y dos workers), cápsula 59, pruebas de C 103/103, sombra 15/15,
  estrés, `packs-abi` y la stdlib embebida.
- **En placa:** la Pico 2, la Metro y la P4, grabadas con la imagen nueva (en la P4 sólo la app: las páginas no
  cambian, y Gui y SQLite siguen vivos). `StrOrden` da en las tres lo mismo que en el PC.

**Medido.** `==` de dos cadenas iguales (el censo, `StrCenso`; antes, el de esta mañana):

| | 16 B | 2048 B |
|---|---|---|
| Pico 2 | 201 → **28 µs** | 22 763 → **68 µs** |
| Metro (PSRAM) | 234 → **31 µs** | 23 462 → **170 µs** |
| P4 | 169 → **21 µs** | 19 370 → **31 µs** |
| PC | 0,86 → **0,11 µs** | 91 → **0,16 µs** |

Distintas en el primer byte o en la longitud, igual que antes (17–27 µs en la Pico 2): ahí nunca se entraba al bucle.

`Str.compare` de dos cadenas de 4000 caracteres con acentos, cinco veces (`StrCursorBench`): carácter a carácter →
por bytes en BP → en C:

| Pico 2 | Metro | P4 |
|---|---|---|
| 7348 → 348 → **1 ms** | 8480 → 360 → **2 ms** | 2750 → 285 → **<1 ms** |

Y el `Map`, `put` de claves en desorden (`MapOrden`): el original → con `move` → con la comparación en C:

| | Pico 2, 1000 claves | Metro, 2000 | P4, 2000 |
|---|---|---|---|
| en desorden | 17 710 → 2603 → **1370 ms** | 72 900 → 9301 → **6348** | 56 722 → 3725 → **1511** |
| en orden | 2622 → **1330** | 6289 → **2918** | 3664 → **1341** |

- En la P4, 2000 claves en desorden van ya **37 veces** más deprisa que esta mañana.
- En la Metro, lo que separa el desorden del orden es el `memmove` en la PSRAM.
- Con claves con acentos (`StrCursorBench`, 200 claves): Metro 1111 → 292 ms, P4 778 → 114.

**Un aviso de memoria que no es de este cambio.** En la Pico 2, `StrCursorBench` se quedó sin heap al llegar al `Map`
de 200 claves, que esta tarde sí cabía. Bisecado en la placa:
- con la `Json` recompilada (cuyo `__strequals` es 32 B más corto), falla; con la de antes, pasa;
- en el PC, barriendo la memoria de la VM, fallan y pasan las DOS versiones alternándose (con 440 000 B, la nueva llega
  y la vieja no; con 400 000, al revés).

Es el banco, que con tamaños de placa va al límite de los 219 KB de la Pico 2: unos bytes de más o de menos al
principio del heap cambian cómo queda fragmentado. Es terreno del GC (`#715`).

### La PSRAM, medida por piezas (análisis, 21-oct)

Eduardo: *«Miremos la PSRAM. Como siempre empezamos analizando.»* El punto 2 de arriba dio la foto (leer +3–6 %, crear
×1,5–4,4). Esto la parte en piezas, con tamaños que caben y que no caben en la caché, y mira una cosa más: si los datos
echan al intérprete de una caché compartida.

**Lo que dice el código:**
- **Cada reserva pone a cero su contenido** (`heap_reservar`: `memset` del payload), aunque quien la pide lo vaya a
  llenar entero acto seguido (una concatenación, `substring`, `fromBytes`, `charAt`…).
- **El asignador:** el primer hueco que cabe en la lista de libres y, si no, el final del heap (el bump). En la Metro
  el GC salta cada ~700 KB reservados, así que entre colecta y colecta la reserva va estrenando memoria que nunca ha
  estado en la caché.
- **Todo el código caliente de la RP2350 está en la flash** y se ejecuta por la caché del XIP: el bucle del intérprete
  (`bpvm_interp_run_quantum`) son 15,4 KB y `bpvm_call_builtin` 22,9 KB, para una caché de 16 KB que en la Metro sirve
  además la PSRAM. Sólo el arranque de la PSRAM y el cerrojo de la flash van a la RAM.
- **En las ESP32 tampoco hay nada nuestro en la IRAM.** La S3 tiene la caché de instrucciones (16 KB) separada de la de
  datos (32 KB); la P4, sus L1 separadas y una L2 de 128 KB compartida.
- **La caché del XIP es de escritura diferida** («clean» escribe lo pendiente; `xip_cache.h`), y puede fijar líneas
  (caché como SRAM).

**Medido** (`samples/benchmarks/PsramFases.bp`, nuevo): cada pieza sola, como el perfil por fases. La Metro, con la
misma imagen y `psram=0` (la VM en la SRAM, 233 KB de heap) y `psram=1` (en la PSRAM, 5,6 MB). Por operación:

| | Pico 2 | Metro SRAM | Metro PSRAM | P4 |
|---|---|---|---|---|
| reservar 2 KB (poner a cero; con GC en las SRAM) | 152 µs | 87 µs | **346 µs** | 42 µs |
| escribir 2 KB en memoria ya reservada | 20 µs | 20 µs | **65 µs** | 9 µs |
| leer 2 × 2 KB (`==`) | 66 µs | 65 µs | **145 µs** | 31 µs |
| crear 2 KB (reservar y escribir) | 182 µs | 117 µs | **494 µs** | 53 µs |
| reservar 16 KB | 879 µs (GC en la mitad) | 448 µs (ídem) | **2378 µs** | 252 µs |
| escribir 16 KB en memoria ya reservada | 102 µs | 102 µs | **3335 µs (×33)** | 34 µs |
| leer 2 × 16 KB | 339 µs | 339 µs | **2673 µs (×8)** | 102 µs |
| crear 16 KB | 1042 µs | 600 µs | **5203 µs** | 255 µs |
| crear 64 KB | no cabe | no cabe | 22,9 ms | 1,7 ms |
| interpretar (un bucle de bytecode fijo) | 807 µs | 807 µs | 828 µs (+2,6 %) | 661 µs |
| `gc()` con poco vivo | 1,3 ms | 0,6 ms | **2,2 ms** | 2,5–3,1 ms |

Y el bucle de bytecode, justo después de leer n bytes (lo que cuesta de más que solo):

| n leídos | Pico 2 | Metro SRAM | Metro PSRAM | P4 |
|---|---|---|---|---|
| 2 × 2 KB | +43 µs (+5 %) | +43 µs | **+134 µs (+16 %)** | +0 |
| 2 × 16 KB | +42 µs (+5 %) | +42 µs | **+184 µs (+22 %)** | +4 µs |
| 2 × 64 KB | — | — | +159 µs | **+591 µs (+89 %)** |

**Lo que dice, por orden de peso:**
1. **En la Metro manda la caché del XIP, no la PSRAM en sí.** Con 16 KB, líneas de 8 B y el código dentro, en cuanto
   una operación toca unos KB de datos ya va a la velocidad de la PSRAM: unos 0,1–0,2 µs por byte, de 15 a 30 veces lo
   de la SRAM (6–10 ns por byte en la Pico 2). Escribir 2 KB en memoria ya usada cuesta el triple; 16 KB, 33 veces.
2. **Escribir parece traer antes la línea.** Escribir 16 KB fuera de la caché cuesta lo que tres transferencias por
   línea: leer el origen, leer la línea de destino y escribirla de vuelta más tarde. Con unas 0,48 µs cada una a 75 MHz
   en QPI, salen ~2,9 ms; se midieron 3,3. Con dos transferencias saldrían ~2,0. Es una deducción de las cifras: la
   hoja de datos de la RP2350 no está en el repositorio.
   Por eso **crear es poner a cero y luego copiar: dos pasadas**, y en la PSRAM se pagan las dos enteras (crear 16 KB
   = reservar 2,4 ms + escribir 3,3 ms).
3. **Los datos echan al intérprete de la caché compartida.** En la Metro, tras leer unos KB, el bytecode va un 16–22 %
   más lento hasta que vuelve a traer su código de la flash. En la P4 pasa lo mismo con su L2: tras leer 128 KB, +89 %.
   Y en la Pico 2, sin PSRAM, el código del `memcmp` ya echa algo del intérprete (+5 %): el bucle del intérprete
   (15,4 KB) casi llena la caché por sí solo.
4. **La P4 va bien hasta unos 64 KB** (su L1 de datos, de 64 KB y líneas de 64 B); el problema empieza al salir de
   ella.
5. **El GC en la PSRAM** cuesta 3,6 veces lo de la SRAM en la Metro, y encima está su coste cuadrático (`#715`).

**Lo que se podría hacer**, de más barato a más caro (sólo propuesto):
1. **No poner a cero lo que se va a llenar entero** (implementación). Una reserva sin `memset` sólo para los builtins
   que escriben todo el contenido antes de soltarlo: la concatenación, `substring`, `fromBytes`, `charAt`, `upper`…
   - Quita una de las dos pasadas al crear: en la PSRAM, crear 16 KB pasaría de ~5,2 a ~3,3 ms; en la SRAM, algo menos.
   - Es seguro porque un `byte[]` no lleva referencias que el GC tenga que seguir. `newByteArray` del programa
     seguiría a cero: así lo promete el lenguaje.
2. **El bucle del intérprete en la SRAM** en la RP2350 (`#665`): 15,4 KB fuera de la flash. Dejaría la caché del XIP
   para los datos en la Metro y quitaría los fallos de código en las dos, Pico 2 incluida. Cuesta 15,4 KB de SRAM, que
   en la Pico 2 salen del heap. Habría que medirlo en una imagen de prueba; puede que acelere todo el bytecode, no
   sólo las cadenas.
3. **La caché de las ESP32,** por configuración y midiendo antes: la L2 de la P4, de 128 a 256 KB, y la de datos de la
   S3, de 32 a 64 KB con líneas de 64 B. Cada una cuesta esa SRAM interna.
4. **El GC** (`#715`): la lista de huecos en orden de dirección, enganchando a la cola, con tu condición.
5. **Fuera, de momento:** las cadenas jóvenes en la SRAM y las viejas en la PSRAM (un GC generacional). Cambia el modelo
   de memoria.

Y queda la mitad de `#665` que no está hecha: la imagen de UN núcleo con `psram=0`, para separar la imagen de la PSRAM.

**¿Una caché de código y otra de datos en la Metro? (Eduardo, 21-oct)** *«De momento nos centramos en la Metro. ¿Sería
posible tener una cache de código y otra de datos? En la Metro creo que nos sobra RAM convencional así que se puede
aprovechar para aumentar la cache.»*
- **En el hardware, no.** La RP2350 tiene UNA caché del XIP, de 16 KB, para todo lo que llega por la QMI (el código de
  la flash y los datos de la PSRAM). No se parte ni se agranda.
- **Pero el efecto, sí, y con la RAM que sobra.** La «caché de código» puede ser la propia SRAM: el código caliente
  copiado a la SRAM se ejecuta a toda velocidad sin pasar por la caché, y los 16 KB del XIP quedan sólo para los datos
  de la PSRAM. Y en la Metro sobra: con `psram=1`, los ~357 KB de SRAM que en la Pico 2 son de la VM quedan para
  `malloc` (`pico/main.c`, «En la Metro no se reserva NADA»), que apenas los usa.
- **Qué código:** el bucle del intérprete (15,4 KB), el despachador de builtins (22,9 KB), el heap y el GC, y las
  funciones de memoria de la libc (`memcpy`, `memset`, `memcmp`, `memmove`; 1,3 KB). Unos 45 KB. Todo el programa
  (282 KB, el «copy_to_ram» del pico-sdk) no cabría en la Pico 2.
- **La pega: la imagen es única para la Pico 2 y la Metro,** y dónde va cada función lo decide el enlazador, no la
  placa. Tres formas:
  - **a)** el código caliente en la SRAM en las dos placas (`__not_in_flash_func`, como ya hacen el cerrojo de la flash
    y el arranque de la PSRAM). Es lo más sencillo. Cuesta unos 45 KB de la SRAM de la VM en la Pico 2 (su heap, de 219
    a unos 175 KB), y probablemente también la acelera (ya se ve un +5 % de fallos de código sin PSRAM);
  - **b)** sólo en la Metro: el código caliente enlazado en una zona de la SRAM que en la Pico 2 es de la VM. Al
    arrancar, si hay PSRAM, se copia ahí y el intérprete se llama por un puntero. La Pico 2 conserva su heap, pero hay
    trabajo de enlazador y dos copias del código caliente en la flash;
  - **c)** una imagen propia para la Metro, con todo el código en la SRAM. Rompe la imagen única.
- **La parte de los datos:** con el código fuera, los 16 KB del XIP son todos para la PSRAM. Una caché de datos más
  grande hecha con SRAM no se puede montar sin que la VM lo sepa (el M33 no tiene MMU que intercepte los accesos). Sí se
  podrían llevar a la SRAM datos calientes concretos que la VM alcanza por un puntero de C, como la tabla de handles.
  Las pilas de BP no: se direccionan como desplazamientos dentro del bloque de la VM.
- **Lo propuesto:** medir primero, con una imagen de prueba de la forma a), en la Metro y en la Pico 2 (`PsramFases`, el
  censo y un banco de bytecode general). Si gana mucho, elegir entre a) y b).

**¿Y DMA entre la PSRAM y la RAM? (Eduardo, 21-oct)** Sí, el hardware lo permite (leído en las cabeceras del
pico-sdk):
- **el DMA de la RP2350** (16 canales) llega a la zona de la memoria externa, así que puede mover entre la PSRAM y la
  SRAM, o dentro de la PSRAM;
- **hay un alias sin caché y sin reserva de línea** de toda esa zona (`XIP_NOCACHE_NOALLOC`, desde `0x14000000`; la
  PSRAM, en `0x15000000`). Por él, un acceso no pasa por los 16 KB de la caché ni echa nada de ella;
- **y una cola de lectura en flujo** (`XIP_STREAM`, de 2 entradas) pensada para que el DMA lea en bloque.

**Lo que daría,** sólo en las operaciones de bloque (copiar y poner a cero; comparar no, que el DMA sólo mueve):
- **copiar y poner a cero cadenas grandes sin echar al intérprete de la caché** (el +16–22 % de arriba);
- **quizá más deprisa:** una escritura sin caché no tiene que traer antes la línea, así que poner a cero se ahorraría
  una de las tres transferencias. Está por medir;
- **en paralelo,** en teoría: la CPU seguiría interpretando mientras el DMA trabaja. Pero la VM suele necesitar la
  cadena en seguida; lo aprovechable sería, por ejemplo, poner a cero por delante la memoria nueva.

**Las pegas:**
- **la coherencia con la caché:** lo que el DMA (o la CPU por el alias sin caché) escribe no actualiza las líneas que la
  caché tenga de esas direcciones. Antes hay que limpiar e invalidar el rango (`xip_cache_clean_range`,
  `xip_cache_invalidate_range`); con líneas de 8 B, 16 KB son 2048 operaciones. Ahí es donde se esconden los fallos;
- **el coste de arrancar cada transferencia:** sólo compensa por encima de algunos KB;
- **la QMI es una:** mientras el DMA lee la PSRAM, la CPU que trae código de la flash espera. Con el código caliente en
  la SRAM (la forma a), no;
- **los canales se comparten** con los drivers: habría que reservar uno;
- **lo de cada instrucción no cambia:** la pila, los campos y la tabla de handles siguen pasando por la caché.

Lo más sencillo de probar ni siquiera es el DMA: es que `memcpy`/`memset` de bloques grandes de la PSRAM usen el alias
sin caché, con el mantenimiento de la caché alrededor. Se mediría junto con la forma a).

**¿Buscar en la PSRAM, o copiar primero a la SRAM y buscar allí? (Eduardo, 21-oct, un `indexOf` de más de 1–2 KB)**
- **Hoy, buscar directamente.** Medido en la Metro (el censo de la mañana, con `psram=0` y `psram=1`): `indexOf` sin
  encontrar en 2 KB, 370 µs con la VM en la SRAM y 391 en la PSRAM (+6 %). La búsqueda misma cuesta ~180 ns por byte
  (`bp_find_bytes`: un `memcmp` en cada posición), más del doble de lo que añade traer el byte de la PSRAM (~70 ns,
  medido con 16 KB en `PsramFases`).
- **Copiar primero no puede ganar así:** para copiar hay que leer cada byte de la PSRAM igual que para buscar, y luego
  la búsqueda cuesta lo mismo. Estimado para 16 KB: directo, ~4,1 ms; copiar (~1,3) y buscar en la SRAM (~2,9), ~4,2.
- **Cambiaría con una búsqueda rápida** (la biblioteca de C: buscar el primer byte de 4 en 4, unos 10 ns por byte).
  Entonces mandaría la memoria, y copiar primero sólo ganaría:
  - si la copia lee la PSRAM más deprisa que la caché, por ejemplo el DMA en ráfaga. Está por medir;
  - si se busca varias veces en la misma cadena: se copia una vez;
  - y a su favor, copiar con el DMA sin caché no echaría al intérprete de la caché.
- **Lo que gana ya, en la SRAM y en la PSRAM:** el algoritmo de búsqueda (de ~180 a unos 10–20 ns por byte).

Eduardo: *«Yo creo entonces que no lo tocaría de momento, lo podemos dejar para mas adelante. Lo que si hay que
solucionar es lo que tarda en crear el string en el Heap de PSRAM, que entiendo es otro problema.»* La búsqueda, el DMA
y el código en la SRAM, para más adelante.

**Crear en la PSRAM, desglosado** (`PsramFases`, la Metro con `psram=0` y `psram=1`):

| crear (reservar y escribir) | SRAM | PSRAM |
|---|---|---|
| 256 B | 49 µs | 117 µs (×2,4) |
| 2 KB | 117 µs | 494 µs (×4,2) |
| 16 KB | 0,6 ms | 5,2 ms (×8,7) |

Es otro problema que la búsqueda: buscar sólo LEE, y crear ESCRIBE memoria NUEVA. Con una caché de 16 KB, la memoria
nueva nunca está en ella: el bump avanza unos 700 KB entre colecta y colecta. Cada línea de 8 B que se escribe cuesta
dos transferencias: traerla y devolverla más tarde. Y el reparto cambia con el tamaño:
- **Una cadena GRANDE** (más que la caché) son unas cinco transferencias por línea. Poner a cero trae la línea y la
  devuelve; para cuando llega la copia ya la ha echado, y la copia lee el origen, la vuelve a traer y la devuelve. Es lo
  que dan las cuentas para 16 KB (~4,9 ms; se midieron 5,2).
- **Una cadena PEQUEÑA** cabe: poner a cero trae sus líneas, la copia las encuentra, y se devuelven una vez. Son dos
  transferencias por línea, más un coste fijo que en la PSRAM se dobla: reservar 256 B, 67 µs frente a 36. Ese coste es
  el asignador, la cabecera, la lista de huecos y la tabla de handles, todo en la PSRAM. Cuánto es cada cosa no se ve
  desde BP.

**Las palancas, por lo que atacan:**
1. **No poner a cero lo que se llena entero:** en una cadena grande quita dos de las cinco transferencias (~−40 %). En
   una pequeña casi nada: la copia trae las líneas igual.
2. **Escribir la cadena nueva sin pasar por la caché** (el alias sin caché): una transferencia por línea en vez de dos,
   en todos los tamaños. Exige cuidar la coherencia: antes hay que limpiar e invalidar las líneas que la caché tenga de
   ese rango. Por medir.
3. **El coste fijo de las pequeñas:** saber primero cuánto es cada pieza. Hay que medirlo dentro, en C, con el contador
   de ciclos, en una imagen instrumentada de la Metro.
4. Y lo que ya está en la lista: el GC (`#715`) y el código en la SRAM.

**Medido por dentro (21-oct, una imagen de medida de la Metro).** Eduardo: *«Si, hay que medir primero. A ver, no hay
limpiar la memoria. Lo que si es importante es que la entrada en la tabla de handlers esté correcta. Ahí si que hay que
limpiar.»* Y: *«Los objetos se crean limpios, pero eso es especificación del lenguaje, no tiene nada que ver en el
PSRAM.»*

- **La medida:** marcas en la reserva (`heap_nuevo`, pieza a pieza) y en lo que llena la cadena (`substring`, `move`,
  `STR_COPY`), en una imagen sólo para medir. No se queda en el repositorio: el parche está en el scratchpad de la sesión.
- **El reloj:** TIMER1 a 12 MHz, porque el M33 de la RP2350 no trae el contador de ciclos (`DWT_CTRL.NOCYCCNT = 1` en el
  pico-sdk). Los fallos de la caché del XIP salen de sus contadores (`CTR_ACC − CTR_HIT`).
- **El programa** (`CreaMide`, tampoco en el repositorio): crea cadenas de 16 B, 256 B, 2 KB y 16 KB de tres formas,
  `newByteArray(n)`, `substring(fuente, 0, n)` y `mitad + mitad`.
- **Lo que pesan las marcas:** hasta ~12 µs en las operaciones pequeñas. Por eso los totales son de la imagen normal (la
  del archivo de las 18:17) y de la medida sólo sale el reparto.

**Lo que cuesta crear, con la imagen normal** (la Metro con `psram=1`, µs por operación). Con un run (`vm.runs=1`) no hay
cerrojo: `vm->smp` queda a NULL.

| | 2 runs (lo de hoy) | 1 run (sin cerrojo) |
|---|---|---|
| `substring` de 16 B | 50,2 | **24,3** |
| `newByteArray(16)` | 37,2 | **18,6** |
| `a + b` de 16 B | 73,7 | **43,9** |
| `substring` de 256 B | 94,7 | 67,3 |
| `substring` de 2 KB | 451 | 407 |
| `substring` de 16 KB | 5 033 | 4 942 |
| el bucle vacío, por vuelta | 6,8 | 6,2 |

**Dónde se va** (la medida: `substring` con dos runs, µs por operación y, entre paréntesis, los fallos de la caché):

| pieza | 16 B, PSRAM | 16 B, SRAM | 2 KB, PSRAM | 2 KB, SRAM | 16 KB, PSRAM |
|---|---|---|---|---|---|
| el cerrojo de la VM, tomar y soltar | 17,1 (9) | 16,1 (7) | 62 (58) | 17,5 (10) | 142 (134) |
| poner a cero | 1,5 (1) | 0,7 (0) | 180 (281) | 8,7 (1) | 1 468 (2 404) |
| copiar | 1,7 (1) | 1,0 (0) | 95 (118) | 9,5 (1) | 2 986 (3 928) |
| resolver los handles | 6,0 (6) | 5,7 (6) | 19 (21) | 6,3 (6) | 59 (56) |
| el asignador | 4,1 (5) | 2,6 (3) | 11 (12) | 3,0 (4) | 29 (41) |
| el resto de la reserva: entrar, el GC, la tabla, la cabecera, el alta, el bit | 5,6 (3) | 4,4 (1) | 25 (23) | 4,7 (2) | 88 (101) |
| el GC, repartido | 4,1 | 3,3 | 21 | 49 | 150 |
| lo de fuera: el despacho del builtin, el bucle… | 14,4 | 13,0 | 69 | 15 | 244 |
| **en total** (con las marcas) | 54,6 | 46,8 | 484 | 114 | 5 166 |

(La SRAM de 16 KB no sale en la tabla: con 232 KB de heap colecta en una de cada dos reservas.)

**Lo que dice:**
1. **En las cadenas pequeñas no manda la PSRAM, manda el cerrojo de la VM.**
   - Con dos runs (lo de V7 en la Pico 2 y la Metro), cada reserva toma y suelta un mutex de FreeRTOS: 17 µs dentro de
     la reserva, un tercio de crear una cadena de 16 B. En la SRAM, lo mismo.
   - Medido aparte, en caliente (mil seguidos, sin fallos de caché):

     | | µs |
     |---|---|
     | tomar y soltar el mutex | 11,4 |
     | un semáforo binario | 10,5 |
     | una sección crítica de FreeRTOS, entrar y salir (el mutex hace unas tres) | 3,7 |
     | `xTaskGetCurrentTaskHandle` | 0,8 |
     | un cerrojo atómico (un CAS y un store) | 0,25 |

   - Lo caro es la sección crítica de FreeRTOS con dos núcleos. El spinlock por software del pico-sdk (`LDAEXB`/`STREXB`)
     no lo explica, y no se ha buscado más.
   - Con un run, crear una cadena pequeña cuesta la mitad. Además, el resto del camino falla menos en la caché: 17 fallos
     por operación en vez de 49, porque el código de FreeRTOS ya no echa al otro.
   - En la Pico 2 no se ha medido, pero es la misma imagen y el mismo cerrojo. En las ESP32, el FreeRTOS es el del IDF:
     sin medir.
2. **En las grandes manda la memoria de la PSRAM.** Poner a cero y copiar son el 86 % de crear 16 KB. Cada fallo de la
   caché cuesta 0,6–0,76 µs: con la caché llena de líneas escritas, traer una es devolver otra.
3. **Los datos echan el código, y lo pagan las piezas que no tocan datos.** En 2 KB, el cerrojo pasa de 17 a 62 µs,
   resolver los handles de 6 a 19 y lo de fuera de 15 a 69. Es lo del código en la SRAM (arriba, «¿Una caché de código y
   otra de datos?»).
4. **No poner a cero sólo quita la pasada entera cuando la cadena no cabe en la caché:** en 16 KB, 1,5 de 5,2 ms
   (−28 %). En las que caben, la copia trae las líneas que antes traía poner a cero: en 2 KB se ahorraría un 10 %
   (estimado); en 256 B o menos, casi nada.
5. **El GC, repartido,** son 4–22 µs por cadena en la PSRAM (`#715`).
6. **Y de paso:** con un run, el bucle vacío va un 9 % más rápido (6,2 frente a 6,8 µs por vuelta), y ese bucle no reserva
   nada. Con un run no arranca la comm task: puede ser ella en el otro núcleo, que comparte la caché. Sin comprobar.

**Las palancas, por lo medido:**
1. **El cerrojo de la VM — NO SE TOCA** (Eduardo, 21-oct: *«No, no tocamos el cerrojo. No quiero meterme en lios con la
   memoria»*). Lo que se midió: la mitad de crear una cadena pequeña, en la PSRAM y en la SRAM. Lo paga toda reserva,
   objetos y arrays incluidos, no sólo las cadenas.
   - La forma conocida: un camino rápido atómico (un CAS si está libre, ~0,25 µs) y el semáforo de FreeRTOS sólo para
     esperar.
   - Es el corazón del SMP de V7: el GC con el mundo parado espera con ese cerrojo y con sus condiciones. Antes, el
     análisis.
2. **No poner a cero las cadenas que se llenan enteras:** −28 % en 16 KB y poco en las pequeñas. La entrada de la tabla de
   handles sigue completa (Eduardo).
3. **El código caliente en la SRAM:** lo que los datos echan de la caché. En 2 KB son ~140 µs de 484 (las piezas que no
   tocan datos, la PSRAM frente a la SRAM).
4. **Escribir sin pasar por la caché:** por medir. Ataca las grandes.
5. **El GC** (`#715`).

**Y para `#665`** (la imagen de dos núcleos, más lenta: `fib(26)` +35 % en la Metro, y ese programa no reserva). En esa
imagen, cada sección crítica de FreeRTOS cuesta 3,7 µs; en una de un núcleo, es enmascarar las interrupciones. Si el
intérprete pasa por alguna en cada cuanto o en cada llamada, explicaría parte de lo que mide esa ficha. Es una pista, no
está comprobada.

**¿Por qué reservar más tarda más? (Eduardo, 21-oct)** *«Lo que no entiendo es por que reservar mas memoria tarda mas
que reservar menos, debería tardar lo mismo.»* Reservar, lo que es reservar, sí tarda lo mismo: buscar el hueco, escribir
la cabecera y dar de alta el handle. Lo que crece con el tamaño es lo que va pegado a la reserva. `newByteArray(n)`, con
la imagen de medida, en µs por reserva:

| | 16 B | 256 B | 2 KB | 16 KB |
|---|---|---|---|---|
| reservar (el hueco, la cabecera y el handle), SRAM | 2,8 | 3,0 | 3,4 | 5,3 |
| reservar, PSRAM | 5,0 | 7,2 | 19 | 72 |
| poner a cero, SRAM | 0,7 | 1,6 | 8,8 | 72 |
| poner a cero, PSRAM | 1,7 | 24 | 212 | 1 932 |
| el GC repartido, PSRAM | 4,2 | 8,3 | 22 | 160 |

1. **Poner a cero.** El lenguaje promete que un array nace limpio, y la VM escribe un cero en cada byte: es proporcional
   al tamaño. En la SRAM, ~4,4 ns por byte. En la PSRAM, ~120 ns, porque cada línea de 8 B hay que traerla a la caché
   antes de escribirla y devolverla después.
2. **El GC.** Salta por volumen: cada ~700 KB reservados en la PSRAM (1/8 del heap) y cada ~29 KB en la SRAM de la Metro.
   Reservando de 16 KB en 16 KB salta mil veces antes que de 16 B en 16 B, y a cada reserva le toca más.
3. **En la PSRAM, la caché.** Poner a cero 16 KB pasa 16 KB de datos por la caché de 16 KB, que es también la del código.
   Al acabar, el código y los datos del asignador (la lista de huecos, la tabla de handles) ya no están en ella, y la
   propia reserva va más lenta: de 5 a 72 µs, con 62 fallos de caché. En la SRAM no pasa: de 2,8 a 5,3 µs.

De todo esto, lo único que el lenguaje no exige es poner a cero las cadenas que la VM llena enteras nada más reservarlas
(la palanca 2).

**El GC, aparte; y la lista larga de huecos (Eduardo, 21-oct).** *«Pero entonces estamos mezclando cosas. En el tiempo de
reservar estamos metiendo tiempo del GC y eso es otra cosa. Ahora lo que si veo lento es en el caso de tener una lista
larga de huecos, tener que buscar uno libre y cuanto mas grande sea el bloque que necesitamos más dífil es encontrar uno
del tamaño suficiente.»*
- **El GC es otra cosa, sí.** En la tabla de arriba va en su fila, y «reservar» no lo lleva: las reservas que colectaron
  se apartaron. Pero las cifras de «reservar» que salieron antes desde BP (`PsramFases`) sí lo llevaban dentro.
- **La lista larga, también sí, y medida** (`samples/benchmarks/HuecosReserva.bp`). La reserva recorre la lista desde la
  cabeza y se queda con el primer hueco que le vale. Si no le vale ninguno, la recorre entera para nada y toma del final;
  y la siguiente, otra vez. Con 4000 huecos, en la Metro con PSRAM: 46 µs la reserva que cabe y 2,8 ms la que no cabe
  (0,7 µs por hueco, un fallo de caché cada uno). En la Pico 2, 0,18 µs por hueco.
- **Y un caso que no se ve a simple vista, la astilla.** Un hueco que dejaría 4 u 8 B de sobra se salta. Así, una reserva
  más PEQUEÑA que todos los huecos también puede recorrer la lista entera.
- **En programas de verdad** hay de decenas a 1100 huecos, y la astilla salta de 2 a 16 por reserva.
- **El detalle y lo que se podría hacer:** en `#718` (FICHAS). Sólo análisis.

**Una caché de los huecos más recientes (la idea de Eduardo, 21-oct; el detalle, en `#718`).** *«al final es donde mas
se crean y se destruyen objetos. Podemos una pequeña cache de los huecos mas recientes y buscar primero en la cache […]
16 entradas como mucho.»*
- **Medido en programas de verdad, la intuición es buena:** el hueco que sirve está casi siempre entre los 16 más altos
  (en cinco de seis casos, el 95–100 % de las reservas que salen de la lista). Hoy cada reserva recorre de 5 a 35
  huecos, de 1 a 6 µs.
- **Pero en nuestro heap los huecos nacen todos en el barrido,** así que «los más recientes» son los de dirección más
  alta. Mirarlos primero es servir desde arriba: lo que había antes de `#355`, que se cambió a propósito (los objetos
  se quedaban arriba, la cola no se liberaba nunca y el primer hueco por dirección fragmenta menos).
- **La misma caché, por clases de tamaño** (16 entradas, cada una con el primer hueco de su clase), encuentra el MISMO
  hueco que hoy, más deprisa: el sitio no cambia y `#355` se conserva. Quedan del 29 al 76 % de los pasos. El cuidado
  está en mantenerla: cada cambio de la lista la toca.

**El código caliente en la SRAM, medido en la Metro (22-oct).** Eduardo: *«El punto 1 solo para la metro. Para la Pico
ya veremos, 45K de RAM en la pico son muchos. De momento haces las pruebas en la metro.»*

- **Cómo:** dos imágenes de prueba, sólo para medir, grabadas sólo en la Metro (`psram=1`, dos runs) y retiradas
  después. Ninguna cambia el código: sólo cambia el script de enlace (una copia fuera del repositorio), que saca de la
  flash ficheros enteros y los pone en `.data`, que se copia a la RAM al arrancar. Es lo que el SDK ya hace con la
  coma flotante.
  - **A**, la VM caliente: el intérprete, los builtins, el heap, los handles, el planificador y `memcpy`/`memset`/
    `memmove`/`memcmp`. 57,6 KB de código, y 11,8 KB de constantes de esos ficheros (casi todo textos de error, que no
    son calientes y podrían quedarse en la flash). 70 KB de RAM en total.
  - **B**, la A y además el camino del cerrojo en FreeRTOS (`queue.c`, `tasks.c`, `list.c`, el port y
    `platform_freertos.c`): 83 KB de RAM.
- **Un hallazgo de paso:** `memcpy`, `memset`, `memmove` y `memcmp` YA debían ir en la RAM, por el script de enlace del
  SDK, pero no van. Su patrón (`*libc.a:*lib_a-mem*.o`) no casa con la newlib de nuestro compilador, que los trae en
  `libg.a(libc_a-mem*.o)`. Están en la flash desde siempre.
- **Lo mismo que antes:** `StrOrden` (18 636 comprobaciones) da en las dos imágenes lo mismo que el PC.

| | normal | A | B |
|---|---|---|---|
| el censo de cadenas, las 36 operaciones, n = 16 / 256 / 2048 | 100 % | −24 / −23 / −24 % | **−30 / −28 / −30 %** |
| `substring` de 16 B | 50,4 µs | 32,1 (−36 %) | **30,4 (−40 %)** |
| `a + b` de 16 B | 74,0 µs | 49,9 (−33 %) | 47,7 (−36 %) |
| `substring` de 2 KB | 452 µs | 371 (−18 %) | 309 (−32 %) |
| crear 16 KB (`PsramFases`) | 5 236 µs | 4 796 (−8 %) | 3 971 (−24 %) |
| `gc()` con poco vivo | 2 161 µs | 1 786 (−17 %) | 1 436 (−34 %) |
| reservar 32 B que caben (`HuecosReserva`) | 46 µs | 30 (−35 %) | 28 (−39 %) |
| reservar sin caber, con 4000 huecos | 2 836 µs | 2 766 (−2 %) | 2 643 (−7 %) |
| bytecode sin cadenas: `fib(22)` / métodos / un array | 462 / 362 / 293 ms | −1 / −6 / −5 % | 0 / −6 / −6 % |

**Lo que dice:**
1. **Las cadenas y el GC ganan un 25–40 %.** El bytecode solo apenas cambia: cuando corre solo, el bucle del
   intérprete ya cabe en la caché. Lo que se pierde es cuando compite con más código (los builtins, la reserva,
   FreeRTOS) y con los datos de la PSRAM, que es lo que hacen los programas de verdad.
2. **FreeRTOS en la SRAM suma bastante más de lo que esperaba,** sobre todo en lo grande (crear 16 KB, de −8 a −24 %).
   La explicación probable, sin comprobar: el otro núcleo (la tarea ociosa, el tic, la comm task) ejecuta FreeRTOS desde
   la flash y compite por la misma caché del XIP. Es la misma pista de `#665`.
3. **Lo que cuesta, en RAM:** de 70 a 83 KB (unos 12 menos si las constantes se quedan en la flash). En la Metro sobra:
   con `psram=1` la SRAM está casi entera para `malloc`. En la Pico 2 saldría de su heap de 219 KB.

**Cómo hacerlo sólo en la Metro, con la imagen única de la RP2350** (decisión de Eduardo del 6-jun):
- **una imagen propia de la Metro:** es lo más sencillo, pero rompe la imagen única;
- **la misma imagen con el código caliente dos veces,** una en la flash (para la Pico 2) y otra enlazada para la SRAM,
  que se copia al arrancar sólo si hay PSRAM. Conserva la imagen única y la SRAM de la Pico 2, pero es delicada: esos
  ficheros tienen variables `static` propias, y compilarlos dos veces las duplicaría. Antes habría que sacarlas;
- **el código en la SRAM en las dos placas:** es lo más sencillo con una imagen, pero es lo que cuesta en la Pico 2;
- **y dos cosas más baratas por medir:** mover sólo las funciones más calientes en vez de ficheros enteros (quizá 25–40
  KB con casi toda la ganancia), y fijar líneas de la caché del XIP (sin gastar SRAM, sólo si hay PSRAM).

Por decidir (Eduardo).

**HECHO, lo primero: `memcpy`, `memset`, `memmove`, `memcmp` y `memchr` a la RAM** (22-oct; Eduardo: *«Empecemos por
las funciones memcpy/memset/memmove»*):
- **el cambio:** el patrón del script de enlace (`pico/bp_memmap.ld`) casa ahora con nuestra newlib
  (`libg.a:libc_a-mem*.o`, y `libc.a:` por si otro compilador enlaza ésa). Es lo que el SDK ya pretendía. Van también
  `memcmp` y `memchr`, como en el patrón del SDK. Son 920 B de RAM, en las dos placas (es la imagen única);
- **medido en la Metro con PSRAM,** la misma tanda: `==` entre cadenas de 2 KB (un `memcmp`), −39 %; `a + b`, −8 a
  −12 %; escribir con `move`, −7 a −10 %; el censo de cadenas, −1 a −5 %; el bytecode, igual. Casi toda la ganancia de
  la imagen A venía del intérprete, los builtins y el heap, no de estas funciones;
- **comprobado:** `StrOrden` igual que el PC; y escribir en la flash (el FS, que es el caso para el que el SDK las quería
  en la RAM): un fichero de 7 KB escrito y leído de vuelta, igual;
- **lo de FreeRTOS** (la pregunta de Eduardo): son 12,6 KB de RAM. En la imagen B, 70 KB son de la VM y 12,6 del camino
  del cerrojo de FreeRTOS.

**Lo demás, para más adelante (`#719`).** Eduardo: *«Lo que necesitamos es una división quirurgica, lo mas utilizado y
crítico de la FreeRTOS y de la VM moverlo a RAM y el resto dejarlo en flash. Pero eso es muy complicado. Lo dejamos para
mas adelante.»* Lo medido, y lo que pide hacerlo bien, en la ficha.

**Lo probado, sólo en la Metro, con la imagen única (22-oct).** Eduardo: *«Pero lo probado en la Metro lo podemos hacer
en la Metro. Mantenemos la imagen única así que tenemos que hacer que con PSRAM se aproveche la RAM adicional pero en la
Pico (el RP2350 sin PSRAM) sigue funcionando como antes.»*

- **La dificultad:** el código se enlaza para una dirección. El que corre desde la SRAM no puede correr desde la flash,
  así que el código caliente tiene que ir DOS veces en la imagen.
- **Las dos formas:**
  1. dos firmwares completos en el mismo `.uf2`: P, el de hoy (el código en la flash), y M, la imagen B (la VM caliente
     y el cerrojo de FreeRTOS en la SRAM). Arranca P y, si el ENV dice `psram=1`, pasa a M;
  2. un firmware con el código de la VM dos veces. Pide sacar antes todas sus variables `static`, que se duplicarían, y
     FreeRTOS no podría entrar.

  Eduardo: *«La opción 1 está bien. Y si, una prueba pequeña está bien.»*
- **La prueba,** sólo en la Metro y retirada después (el parche, en el scratchpad de la sesión):
  - P es la imagen de hoy con un bloque al principio de `main()`, antes del USB. Si el arranque viene de un RESET del
    wire y el ENV tiene `prueba.m`, arma el perro (5 s), apaga las interrupciones y pasa a M;
  - M es la imagen B enlazada y guardada en `0x10100000`, a 1 MB de la P. Sus límites de la zona 2 son los de P (el ENV
    y el log son los mismos), y se presenta como `bpvm-pico-m`;
  - las dos van en un solo `.uf2`.
- **Pasar con el ROM (`rom_chain_image`) NO funcionó.** El ROM arranca la imagen «enrollada» a `0x10000000`: el SDK no
  pone `ROLLING_WINDOW_DELTA` en su IMAGE_DEF, y M, enlazada en `0x10100000`, salta a otra parte. Ni con un IMAGE_DEF
  propio con el delta llegó M a su primera instrucción del arranque del SDK (marcas en los registros del perro, que
  sobreviven al reinicio). No se siguió por ahí.
- **El salto directo SÍ funciona.** P pone la tabla de vectores y la pila de M y salta a su `crt0`; la flash queda como
  la dejó el arranque normal, sin traducción. En M, todo bien:
  - el HELLO, el INFO, el ENV, el log, el FS (escribir y leer) y los dos packs (la stdlib y SQLite);
  - `StrOrden` y `HilosMuchos`, igual que el PC; `SqlDemo` (el pack nativo de SQLite), entero;
  - el rendimiento, el de la imagen B: el censo de cadenas −25 a −29 %, `substring` de 16 B −39 % y el GC −35 a −38 %,
    frente a la imagen de hoy;
  - un RESET desde M vuelve a pasar por P y otra vez a M.
- **La red funcionó:** cuando M no arrancaba (las pruebas con el ROM), el perro devolvió la placa a P sola, y P no
  volvió a intentarlo. No hizo falta tocar la placa.
- **Lo que falta para hacerlo de verdad:**
  - el build: los dos enlaces y juntarlos en un `.uf2`. Los bloques de flash van en una sola serie, y el bloque del
    erratum E10 (el de `0x10FFFF00`) es una descarga aparte, sin renumerar: si no, el ROM no da la grabación por
    terminada;
  - la región de P, hasta `0x10100000`, para que crecer sea un error de enlace y no pise a M;
  - el disparo, por `psram=1`, con la red del perro (si M no llega, P no vuelve a intentarlo);
  - en M: la causa del reset que tenía P (hoy dice «watchdog» por el perro de P), el CRC de la imagen sobre lo suyo, y
    que el HELLO o el INFO digan qué imagen corre;
  - comprobar en la Pico 2 que P va exactamente como hoy;
  - y el `.uf2` es casi el doble: grabarlo tarda el doble.

**HECHO de verdad: la imagen con P y M (22-oct; Eduardo: *«si»*).**
- **El build** (`pico/CMakeLists.txt`, «Los firmwares»): de los mismos fuentes, dos firmwares con la configuración de
  siempre (una función de CMake para los dos):
  - `bpvm_pico_p` es P. Sus ficheros se llaman como siempre (`bpvm_pico.elf`, `.elf.map`, `.dis`), y su zona 3 acaba
    en `0x10100000`: si P crece hasta M, el enlace falla;
  - `bpvm_pico_m` es M (`bpvm_pico_m.elf`): de `0x10100000` a las particiones, con la VM caliente y el cerrojo de
    FreeRTOS en la SRAM (la imagen B);
  - el script de enlace de cada uno sale de la plantilla `pico/bp_memmap.ld`;
  - el objetivo `bpvm_pico` es LA IMAGEN, `bpvm_pico.uf2` con los dos. `ninja bpvm_pico` la sigue haciendo, como dicen
    los documentos desde V4.
- **Los junta `pico/scripts/juntar_uf2.py`,** y antes comprueba tres cosas:
  - que sean PAREJA: un número de la configuración del build, en la firma de cada uno (`.bp_firma`, a 4 KB del
    principio);
  - que lo que sobrevive a un reinicio en la RAM (el log, las migas, la marca del fallo fatal, las marcas del arranque)
    esté en el mismo sitio en los dos. Está antes de `.data`, así que sale igual, pero un cambio podría moverlo;
  - que no se pisen.
- **El salto** (`main.c`, `pico_pasar_a_m`), al empezar `main()`, si se cumple todo esto:
  - `psram=1` y una RP2350B: la misma condición con que se activa la PSRAM;
  - sin `imagen=P` en el ENV (la vuelta atrás sin grabar, como `vm.cores=1`);
  - una M que sea su pareja, con una tabla de vectores que valga.

  Antes de saltar, P arma el perro (15 s) y deja una marca en el `scratch[0]` del perro y la causa del reset en el
  `scratch[1]`. M borra la marca al llegar al REPL. Si no llega, el perro reinicia la placa, y P, al ver la marca, se
  queda esa vez; el reinicio siguiente lo vuelve a intentar.
- **En M:**
  - la causa del reset es la de P;
  - el CRC de la imagen (`imageCrc`) es el de su código;
  - el INFO dice `imagen` (`P` o `M`), y el log, «boot: imagen M …» o «boot: imagen P …; no paso a M: [el motivo]».
- **`tools/tanda.py`** compara el CRC de la placa con el `.elf` del firmware que corre.
- **Probado en la Metro:**
  - arranca en M, con el CRC del M compilado en el PC;
  - la causa sale bien: «power-on/run» tras grabar, y «reinicio pedido» tras un RESET (en la prueba decía «watchdog»);
  - el log de RAM y las marcas del arranque sobreviven al paso por P;
  - `SqlDemo`, `ParaleloPico` (dos runs), `HilosMuchos`, `StrOrden`, `JsonAcentosPlaca` y `MetroPsram`, bien;
  - la red: con una copia del `.uf2` con la primera instrucción de M cambiada por un bucle, a los 15 s el perro
    reinicia la placa y P se queda («M no llego al REPL la ultima vez»). Un RESET lo vuelve a intentar y vuelve a caer
    en P, con el log de RAM entero;
  - la pareja: un P nuevo con la M de otra compilación en la flash se queda en P, y el log dice las dos parejas.
- **M frente a P, en la Metro, misma placa y misma imagen** (P con `imagen=P`):

  | | P | M | |
  |---|---:|---:|---:|
  | el censo de cadenas, la suma de las 36 operaciones, n=16 / 256 / 2048 | 16,9 / 80,6 / 1011 ms | 11,7 / 60,8 / 739 ms | −31 / −25 / −27 % |
  | `substring` / `a + b` de 16 B (µs) | 46,7 / 66,8 | 30,6 / 48,2 | −34 / −28 % |
  | `a + b` de 2 KB / de 16 KB (µs) | 500 / 5248 | 328 / 4017 | −34 / −23 % |
  | crear una cadena de 256 B / de 64 KB (µs) | 105 / 22 470 | 66 / 19 672 | −37 / −12 % |
  | el GC (destruir, µs) | 2177 | 1455 | −33 % |
  | reservar con huecos: la que cabe / la que recorre 4000 (µs) | 45 / 2821 | 26 / 2646 | −42 / −6 % |
  | bytecode sin cadenas: `fib(22)` / métodos / arrays (ms) | 459 / 360 / 297 | 461 / 342 / 275 | 0 / −5 / −7 % |

  Lo mismo que la imagen B. Y P, lo mismo que la imagen de esta mañana (±2 %).
- **En la Pico 2:**
  - se queda en P (no tiene `psram`), con el CRC de P;
  - diez programas dan lo mismo que con su imagen del 21-oct, `MapOrden` y `JsonAcentosPlaca` incluidos (los dos se
    quedan sin memoria en el mismo sitio);
  - el heap de la VM, 928 B menos: 920 son de `mem*` en la RAM y 8 de la nota del salto.
- **Y un hallazgo, de esta mañana y no de P/M: `mem*` en la RAM es un trueque en P (`#720`).** En la Pico 2 (con la
  imagen del 21-oct, y partido con una imagen sin `mem*` en la RAM):
  - crear mejora: `a + b` −10/−13 %, `cadena + entero` −15 %, `s := s + x` −12/−13 %;
  - buscar empeora: `indexOf`/`contains` +12/+17 %, `replace` +10 %, `parseFloat` +10 %; «buscar y cortar» de
    `StrAcentos`, de 75 a 85 ms;
  - el censo entero, −4/−5 %. En la Metro con P, el mismo trueque; en M, todo mejora.
  
  La causa: cada llamada desde la flash a la SRAM pasa por un *veneer* (`ldr pc, [pc]`), y `indexOf` hace muchas
  comparaciones cortas. Por decidir: dejar `mem*` en la RAM sólo en M, que es donde están también quienes las llaman.

**`mem*` sólo en M, y la Pico 2 frente a la Metro (22-oct, noche).** Eduardo: *«SI, las mem* sólo en la Metro. Me
gustaría comparar la velocidad Pico vs Metro. Alguna mejoras se compensan con el hecho que la PSRAM va mas lenta, así
seguirán dando velocidades similares o eso es lo que creo, pero las pruebas dirán la verdad.»*
- **`mem*`, sólo en M** (`#720`, cerrada): salen de la parte común de la plantilla (`pico/bp_memmap.ld`) y van en la
  lista de M (`pico/CMakeLists.txt`). P vuelve a lo del 21-oct: «buscar y cortar» de `StrAcentos`, 76 ms en la Pico 2
  (85 con `mem*` en la RAM), y el heap de la VM, 219 016 B (8 menos que el 21-oct: la nota del salto). M no cambia.
- **Cómo se midió:** la misma imagen en las dos placas, los mismos módulos y las dos a 150 MHz. Nueve pruebas: los
  cinco bancos (`BytecodeMide`, `CreaMide`, `PsramFases`, `StrCenso` y `HuecosReserva`) y cuatro programas
  (`StrAcentos`, `MapOrden`, `JsonAcentosPlaca` y `ParaleloPico`, con dos runs). Cuatro configuraciones:
  - la **Pico 2**: P, con el heap en la SRAM (219 KB);
  - la **Metro sin PSRAM** (`psram=0`): P, con el heap en la SRAM. Es el CONTROL: va igual que la Pico 2 (±3 %; el
    bytecode, un 2–3 % más lento en la Metro). Las diferencias de abajo son de la PSRAM y de M, no de la placa;
  - la **Metro con P y PSRAM** (`imagen=P`): como en V7;
  - la **Metro con M**: la de hoy.
- **Un cuidado:** la Pico 2 tenía `log=1` en el ENV, y eso hace el GC 2,2 veces más lento (`#721`). Para comparar
  placas y no configuraciones, se midió sin él; luego se le volvió a poner.

  | | Pico 2 | Metro, P y PSRAM (V7) | Metro, M | M frente a la Pico 2 |
  |---|---:|---:|---:|---:|
  | bytecode sin cadenas: métodos / arrays (ms) | 355 / 283 | 399 / 294 | 342 / 275 | −4 / −3 % |
  | crear 16 B: `substring` / `a + b` (µs) | 40,1 / 65,6 | 47,0 / 74,1 | 30,7 / 48,2 | −23 / −27 % |
  | crear 256 B: `substring` / `a + b` (µs) | 47,5 / 75,5 | 92,4 / 126,4 | 63,7 / 82,1 | +34 / +9 % |
  | crear 2 KB / 16 KB: `a + b` (µs) | 139 / 579 | 539 / 5355 | 328 / 4008 | ×2,4 / ×6,9 |
  | leer / escribir 16 KB (µs) | 339 / 103 | 2656 / 3335 | 2037 / 2610 | ×6 / ×25 |
  | reservar, la que cabe (µs) | 36 | 45 | 26 | −28 % |
  | una recolección (`destruir`, µs) | 579 | 2190 | 1464 | ×2,5 |
  | el censo de cadenas, n=16 / 256 / 2048 (ms) | 7,8 / 65,8 / 689 | 17,2 / 83,6 / 1054 | 11,7 / 60,4 / 739 | +50 / −8 / +7 % |
  | `Json.parseJson`, 8 KB (ms) | 2369 | 2469 | 1521 | −36 % |
  | `Map`, 1000 claves desordenadas / 2000 en orden (ms) | 1400 / 2928 | 2165 / 3069 | 1049 / 1646 | −25 / −44 % |
  | `Map`, 2000 claves desordenadas (ms) | 3105 | 6431 | 3396 | +9 % |
  | `StrAcentos`, buscar y cortar (ms) | 76 | 85 | 72 | −5 % |
  | dos runs a la vez: cálculo / cadenas (ms) | 1288 / 1643 | 1872 / 3181 | 1423 / 1522 | +10 / −7 % |

- **Lo que dicen:**
  - **la hipótesis de Eduardo se cumple con datos pequeños y en los programas de tamaño normal.** El bytecode va
    igual; crear cadenas pequeñas y reservar, un 23–28 % más deprisa en M; el JSON de 8 KB y el `Map` de hasta 1000
    claves, un 25–44 % más deprisa. Lo que gana M (el intérprete, los builtins y el cerrojo en la SRAM) pesa más que
    lo que pierde la PSRAM;
  - **no se cumple con datos medianos o grandes.** Desde unos 256 B, crear sale más caro en la Metro: ×2,4 con 2 KB,
    ×7 con 16 KB. Y leer o escribir 16 KB de golpe, ×6 y ×25: la caché del XIP es de 16 KB, y fuera de ella la PSRAM es
    mucho más lenta que la SRAM. El `Map` de 2000 claves desordenadas ya va un 9 % más lento;
  - **el GC:** cada recolección cuesta 2,5 veces más en M (más heap y más lento), aunque hay menos. En el censo con 16 B,
    casi todo lo que pierde M es `gc()` explícito (6,6 ms por llamada frente a 0,87 en la Pico 2);
  - **antes de M** (la Metro con P y PSRAM, como en V7), la Metro iba más lenta que la Pico 2 en casi todo. M recupera
    lo pequeño y lo normal;
  - **lo que da la Metro a cambio es sitio:** el `Map` de 2000 claves desordenadas, el JSON de 16 KB y los 4000 huecos
    de `HuecosReserva` no caben en la Pico 2.
- **Dos hallazgos de paso, con ficha:**
  - **`#721`:** con `log=1`, el GC tarda 2,2 veces más (en la Pico 2, `destruir` 1296 frente a 579 µs; crear 16 KB, 826
    frente a 388). Cada recolección escribe dos o más líneas `[gc]`, que con el log encendido se formatean otra vez y se
    añaden; con el log lleno, cada línea desplaza el búfer entero (por el código, sin medir qué parte pesa más);
  - **`#722`:** en la Pico 2, que `MapOrden` quepa con 2000 claves depende de lo que se ejecutó antes. Tras un
    reinicio, solo, no cabe (4 de 4, con y sin log); tras `HuecosReserva` y `StrAcentos`, cabe (2 de 2); en la tanda
    entera, una vez sí y otra no. Va justo al límite. Sin analizar.

### `#711`: el UTF-8 mal formado se corrige al nacer la cadena (la regla de Eduardo, 22-oct; análisis)

Eduardo: *«un string es un invariante, así que una vez formado no se puede cambiar. ¿Donde se pueden hacer cambios?
Bien en el proceso de copiar un contante del programa a la RAM del Heap ahí se puede corregir. Y el otro punto, cuando
se genere un string como resultado de sumar 2 o de extraer de otro (un substring) se puede aprovechar para arreglar. En
resumen, se puede modificar en el proceso de creación del string pero no después.»*

**Dónde nace una cadena** (leído en la VM-C el 22-oct: los sitios que hoy le ponen el bit «todo ASCII», que son los
mismos que la crean). Cuatro grupos:

1. **Las constantes del programa.** No se copian al heap. El cargador copia la sección de datos del `.mod` a la RAM tal
   cual, sin saber qué es cada cosa (`MOD_FORMAT.md` §5: «bytes opacos al loader»), y el programa usa cada literal en
   su sitio (`LEA_GLOBAL`). Corregir ahí no se puede: el cargador no sabe cuáles son cadenas, y un U+FFFD ocupa 3
   bytes donde el malo ocupaba 1, así que movería todo lo que va detrás. **Pero su creación es la del compilador:** las
   escribe con `getBytes(UTF_8)` de Java (`ModWriter.addConstantString`; las natives, igual), que nunca da UTF-8 mal
   formado (un sustituto suelto sale `?`). Nacen bien.
2. **Las que salen de otras cadenas:** `a + b` (en el intérprete y en las natives), `substring`, `charAt`, `upper` y
   `lower`, `trim`, `replace`, `split` y las de `Path`. Si lo que entra es válido, lo que sale también: el UTF-8 se
   corta por caracteres y se junta sin romperse, y `substring` corta por caracteres. **Con entradas válidas no hace
   falta mirar nada**, y siguen igual de rápidas. Mirar aquí sería una pasada más en cada suma, para nada.
3. **Las que salen de bytes que no eran una cadena.** Por aquí entra el UTF-8 mal formado, y aquí va la corrección:
   - `fromBytes` (de `byte[]` a cadena): por aquí pasan el puerto serie, la red y los ficheros binarios;
   - `readFile` y `TextFile.readLine`;
   - los nombres de `listDir`, `Machine.getEnv`, el texto de la GUI (`getText` y las celdas de la tabla), los
     argumentos del RUN, las rutas de la app y la información de los packs;
   - las que crean las natives y los packs, como SQLite (`h_string_from_cstr`, en los helpers del AOT);
   - los números pasados a texto (`intToString`, `floatToString`…) también nacen aquí, pero son siempre ASCII.
4. **Las que salen de un punto de código:** `appendChar` y `__charsToString`. Un punto de código que no es un carácter
   (un sustituto, de D800 a DFFF, o uno por encima de 10FFFF) da hoy bytes mal formados en la VM-C; en la VM-Java, `?`
   o el hilo muere. Y `Json` convierte cada `\uXXXX` con un `appendChar` sin juntar los pares (`😀`, el
   emoji): eso se arregla en `Json`, juntándolos antes.

**Cómo se corrige.** Cada trozo mal formado pasa a U+FFFD (`EF BF BD`), como hace Java. Medido con Java 8 (un programa
en el scratchpad): un byte que no puede empezar un carácter, uno; una secuencia cortada, uno por todo el trozo (`E2 82`
y una `x` → U+FFFD y la `x`). Java 8 sólo se aparta del estándar de Unicode (las «subpartes máximas») en un caso: un
sustituto escrito en tres bytes (`ED A0 80`), donde Java pone uno y el estándar, tres. Hoy la VM-Java no corrige al
crear (su `fromBytes` copia los bytes tal cual, como la VM-C): los interpreta después, con el decodificador de Java. Si
las dos VMs corrigen al nacer, cada una con su rutina, las dos pueden seguir el estándar. Corregir puede alargar la
cadena (de 1 byte a 3), así que hay que medir antes de reservar.

**Lo que cuesta.** La mayoría de las cadenas son ASCII: la pasada que ya les pone el bit lo decide, y no hay nada más.
Las otras, sólo en el grupo 3, una pasada para validar. El grupo 2, nada. De paso, `readFile` en la VM-C no pone hoy el
bit (sus cadenas van siempre por el algoritmo lento): con la misma pasada, lo pondría.

**Lo que trae la regla:**
- **`#714` es su otra mitad:** `move` puede escribir dentro de una cadena ya formada, el «después» que la regla prohíbe.
  Sin cerrarlo, un programa puede estropear una cadena válida;
- **en la VM-Java cambian los mismos sitios:** `fromBytes` y `readLine`, que hoy copian tal cual; `readFile`, que hoy
  pasa por el decodificador de Java; y `appendChar`;
- **con todas las cadenas válidas, `#711` se cierra:** las dos VMs cuentan igual. Y el caso especial del cursor (las
  cadenas que empiezan por un byte de continuación) se puede quitar.

**Por decidir (Eduardo):**
1. la regla, tal como queda: se corrige en los grupos 3 y 4; las constantes, el compilador; el grupo 2, nada;
2. la sustitución: el estándar de Unicode, o exactamente lo que hace Java 8 (sólo cambia el sustituto en tres bytes);
3. `#714` a la vez.

Después, por la norma: el algoritmo (una rutina de «validar y corregir», probada contra un oráculo en las dos VMs) y
la implementación.

**La segunda vuelta de Eduardo (22-oct):** *«Que un string es un invariante no se toca. Para trabajar con strings
variables tenemos stringbuilder. Ahora, donde arreglar string mal formados? Si la fuente nos garantiza que el string
está bien formado no hace falta verificar, es cierto pero es una relación de confianza, eso hay que verificarlo con
algún test, por si acaso. OK, la lectura de fuentes externas: aquí estamos poniendo la lectura de un fichero o de un
stream como si fuera un solo paso pero en realidad son 2, uno que lee los bytes y un segundo que transforma los bytes a
caracteres según la codificación, y ahí es donde se puede detectar el problema. Bajo mi punto de vista debería
producirse una excepción de codificación pero si ignoramos el error lo que hay que hacer es utilizar un caracter
especial que señale el caracter erróneo.»*

Queda decidido:
- **el invariante no se toca;** para el texto que cambia, `StringBuilder`. `#714` (`move` dentro de una cadena) queda
  decidido en el qué; falta el cómo;
- **las fuentes de confianza no se verifican en marcha, pero su confianza se prueba con tests:** las constantes del
  compilador, las operaciones que salen de cadenas válidas y lo que da el propio sistema;
- **leer de fuera son dos pasos:** leer los bytes, y descodificarlos según la codificación. El error se ve al
  descodificar: por defecto, una excepción de codificación; si se decide ignorar el error, cada trozo malo se cambia
  por un carácter que lo señale (U+FFFD, «�»).

**Lo que dice el código de esto** (22-oct):
- **los dos pasos ya están en la API:** `File.read`, `readFileBytes`, `Uart.read` y `Net.recv` dan bytes, y `fromBytes`
  es el descodificador (así lo dice `IO.bp` de `TextFile`: «`toBytes`/`fromBytes` son el codificador y el
  descodificador»). Lo que hace los dos a la vez lo tendría que hacer con el mismo descodificador: `readFile`,
  `TextFile.readLine` y `readStr`, `Net.recvStr`, y el texto de las natives y los packs que viene de fuera (SQLite);
- **lo que da el propio sistema es de confianza, con su test:** los nombres de `listDir` (littlefs los escribe desde
  cadenas de BP; FatFs da los de la SD en UTF-8), el ENV (lo escribe el IDE) y el texto de la GUI (LVGL trabaja en
  UTF-8; nuestro búfer de 1 KB puede cortar un carácter: eso es un fallo nuestro, y se arregla cortando por caracteres);
- **el `toString` de `StringBuilder` acaba en `fromBytes`** (BP no tiene otra forma de pasar de `byte[]` a cadena). Sus
  trozos ya son cadenas válidas: si `fromBytes` valida, ahí validaría para nada. Le haría falta una conversión de
  confianza propia, interna;
- **`appendChar` con un punto de código que no es un carácter** (un sustituto, o uno por encima de 10FFFF): por la misma
  regla, una excepción. Hoy la VM-C deja bytes malos y la VM-Java pone `?` o mata el hilo;
- **un stream llega a trozos, y un trozo puede cortar un carácter por la mitad.** `Net.recvStr` descodifica cada trozo
  que llega: con la regla, un texto bien formado partido entre dos paquetes daría una excepción falsa (o un «�» falso,
  si se ignora el error). Hay que descodificar unidades completas (una línea, un mensaje) o con un descodificador que
  guarde el trozo pendiente para el siguiente. `readLine` no tiene el problema: un `\n` nunca cae dentro de un carácter.
  En el corpus, `FileCursor` lee 64 bytes y los pasa a texto: el mismo caso, si el fichero no es ASCII;
- **`Uart.read` llena un `integer[]`:** un programa que arma el texto con un `appendChar` por byte trata cada byte como
  un carácter (`ñ` sale `Ã±`). No es un error que se pueda ver: lo dirá el manual (los bytes, por el descodificador);
- **cambia lo que hace hoy un programa:** leer un fichero en Latin-1 con `readFile` da hoy, sin avisar, bytes malos (en
  la VM-C) o «�» (en la VM-Java); con la regla, una excepción en las dos. Si algún día hace falta leer Latin-1, el
  descodificador es el sitio para una segunda codificación;
- **en el corpus**, `fromBytes` recibe casi siempre texto válido (ida y vuelta con `toBytes`): no se rompe nada más.

**Por decidir (Eduardo):**
1. la excepción: un `RuntimeError` con su mensaje, como los demás builtins, o una clase propia (por ejemplo
   `EncodingError`) que se pueda atrapar aparte (cambia la interfaz de `Core`, que con esto se rehace de todos modos);
2. dónde se dice «ignora el error»: en el paso de descodificar (`fromBytes` con un segundo parámetro, por ejemplo), y
   lo que hace los dos pasos a la vez, siempre con excepción. Quien quiera ignorar, hace los dos pasos;
3. el stream: `Net.recvStr` guarda el trozo pendiente en la conexión, o se queda sólo `recv` y el programa descodifica
   mensajes completos;
4. cuántos «�» por trozo malo, al ignorar el error: el estándar de Unicode o lo que hace Java 8.

**Decidido el punto 2 (Eduardo, 22-oct):** *«Creo que deberiamos tener 2 métodos de lectura, uno que produzca la
excepción si el caracter no es válido y otro que no produzca la excepción y cambie el caracter. Quizás sería mejor
fusionar los dos métodos en uno, con un parámetro opcional que indique el coportamiento algo como readstring(len:int,
ignorarerror:boolean=false)»*. Un método, con un parámetro opcional al final; por defecto, la excepción.
- **BasicPlus ya lo admite:** `nombre: tipo := literal`, al final de la lista (H8.1; en JC, `= literal`). El compilador
  rellena en cada llamada el que falte.
- **Lo que llevaría el parámetro:**

  | hoy | con el parámetro |
  |---|---|
  | `fromBytes(b)`, el builtin 120 | `fromBytes(b, ignorarError := false)` |
  | `readFile(path)`, el 38 | `readFile(path, ignorarError := false)` |
  | `TextFile.readLine()`, por `__freadLine` | `readLine(ignorarError := false)` |
  | `TextFile.readStr(n)`, en BP, acaba en `fromBytes` | `readStr(n, ignorarError := false)` |
  | `Net.recvStr(max, timeoutMs)`, en BP, acaba en `fromBytes` | `recvStr(max, timeoutMs, ignorarError := false)` |

- **Un número de builtin nuevo para cada uno de los tres builtins** (`fromBytes`, `readFile` y `__freadLine`; el último
  hoy es el 282). Un builtin se declara en `Core` como `inline CALL_BUILTIN N`, pegado en cada llamada: un `.mod` ya
  compilado mete UN argumento y llama al 120. Si el 120 sacara dos, desordenaría la pila sin avisar. Los números de
  hoy se quedan, con un argumento, para lo ya compilado (y con la regla nueva: excepción).
- **No llevan el parámetro:** `appendChar` con algo que no es un carácter es un fallo del programa, no un texto de fuera:
  siempre excepción. Y el `toString` de `StringBuilder` no valida (sus trozos ya son válidos): una conversión interna.
- **El parámetro no arregla el stream:** en `recvStr`, un carácter partido entre dos paquetes daría con `true` un «�»
  falso, en medio de un texto bueno. Lo del trozo pendiente sigue haciendo falta.

Quedan por decidir: la clase de la excepción (punto 1), el stream (3), cuántos «�» (4) y el nombre del parámetro
(`ignorarError`, como lo escribió Eduardo, o `ignoreErrors`, como el resto de la API, que está en inglés; no se escribe
en las llamadas).

**El stream (punto 3), la idea de Eduardo (22-oct):** *«Los streams los podemos hacer siempre de bytes (creo que en Java
son así). Pero luego se encapsulan en un Reader. Así que separamos la fuente de los bytes que puede ser de múltiples
sitios, del resto que ya codifican a diferentes objetos (strings, object, bytes, imagen, etc). Entonces el problema no
es la lectura de los bytes, el problema es de codificación y ahí si le faltan bytes lo que toca sería esperar a que
continue la lectura.»*

Es lo de Java (`InputStream` da bytes; `InputStreamReader` y `BufferedReader`, encima, los descodifican). Lo que pide en
BasicPlus:
- **las fuentes de bytes, por un lado:** el fichero, la conexión TCP, un puerto serie, un `byte[]` en memoria. Hoy
  `IO.File` ya lee bytes (`read(buf)`); la conexión da `recv(max, timeoutMs)`, un `byte[]` nuevo cada vez; y `Uart` son
  funciones con el número de bus, sin objeto, que llenan un `integer[]`;
- **los descodificadores, por otro,** encima de cualquier fuente: el de texto (UTF-8) y, después, otros (objetos,
  imágenes…). Al de texto, si le faltan bytes para acabar un carácter, los pide a la fuente: espera. Sólo al acabarse la
  fuente (el fin del fichero, la conexión cerrada) un carácter a medias es un error: la excepción, o «�» con el
  parámetro. Y si la fuente tiene plazo (el puerto serie, la red) y vence a mitad de un carácter, los bytes se quedan en
  el descodificador para la lectura siguiente: no es un error;
- **`fromBytes(b)` no espera:** el `byte[]` está entero, no hay más que leer. Un carácter cortado al final es un error;
- **BasicPlus no tiene interfaces ni clases abstractas** (están en las ideas de la gramática), pero sus métodos son todos
  virtuales: una clase base con `read(buf)`, que cada fuente sobrescribe, hace el mismo papel sin tocar el lenguaje;
- **lo que hay hoy mezcla las dos cosas:** `TextFile` hereda de `File` y descodifica dentro; `Net.recvStr` descodifica
  cada paquete por su cuenta (por eso parte caracteres). `TextFile` se puede quedar como está para los programas de hoy
  (un `File` con un descodificador dentro);
- **la velocidad:** lo que se hace byte a byte (buscar el `\n`, validar el UTF-8, cortar en el último carácter entero)
  tiene que ir en C. Un descodificador en BP iría unas quince veces más lento en placa: por eso `readLine` ya es nativo
  (`FileBench`, 21-sep). Un builtin que descodifique un trozo de un `byte[]` y diga cuántos bytes usó, dejando el
  carácter incompleto: es una pieza natural de la biblioteca de cadenas en C (decidida el 21-oct, abajo).

Por decidir, al diseñarlo: los nombres (los de Java, `InputStream` y `Reader`, u otros), el objeto del puerto serie, qué
devuelve `readLine` cuando vence el plazo, y si `TextFile` y `Net.recvStr` se quedan como atajos.

### `#714` y `#711`: el algoritmo (23-oct; lo decidido el 22, en concreto)

Eduardo, 23-oct: *«Continuamos con #711 y #714»*. Lo leído en el código para escribirlo:
- **la etiqueta** (la palabra de cabecera de cada bloque): el bit 31 es la marca del GC, el 30 «libre», del 29 al 24 el
  tipo, el 23 y el 22 sólo existen mientras se compacta o se suelta un owner, y el 21 es «todo ASCII» (que
  `HEAP_LAYOUT.md` §2.1 aún no cuenta). **Del 20 al 0, libres** en las dos VMs. Al reservar, la etiqueta se escribe
  entera, sólo con el tipo: un hueco reutilizado no hereda nada. La compactación conserva la etiqueta;
- **la suma de cadenas, en el bytecode,** no pasa por ningún nacimiento: los ayudantes `__strconcat` y `__strconcatN`
  que el compilador genera en cada módulo reservan un `byte[]` con `NEWARRAY_I8`, copian las partes con `__strCopy` (que
  ya calcula el bit ASCII) y lo devuelven como cadena;
- **`StringBuilder.toString`** junta las partes con `__strCopy` en un `byte[]` suyo y acaba en `fromBytes`, que copia;
- **`__strCopy` es pública**, con un `byte[]` de destino: también la usa un programa sobre sus propios arrays;
- **la VM-C marca el nacimiento en `bpvm_str_nace`** (14 builtins, `heap_alloc_string` y las natives) y la VM-Java en
  `strFijaAscii` (43 llamadas), que `move` también usa para BORRAR el bit;
- **`readFile` en la VM-C no pasa por `bpvm_str_nace`**;
- **la excepción de un builtin:** la VM-C busca la clase por su nombre (`Core.RuntimeError`, `bpvm_link_lookup`).

**`#714`, el bit de solo lectura:**
1. **El bit:** el 20, `0x00100000` (`BPVM_TAG_SOLO_LECTURA`; en la VM-Java, `TAG_SOLO_LECTURA_BIT`). En
   `HEAP_LAYOUT.md` §2.1, junto con el 21.
2. **Lo pone el nacimiento de cada cadena:**
   - la función de nacer, que pone el bit ASCII por los bytes y, ahora, también el de solo lectura. En la VM-Java,
     `strFijaAscii` se parte en dos: nacer (los dos bits) y tocar sólo el ASCII;
   - `readFile` en la VM-C, que hoy no nace: pasa a nacer (y gana el bit ASCII, de paso);
   - **la suma:** un builtin interno nuevo, `__strNace`, al final de los ayudantes que genera el compilador. Marca el
     bloque en su sitio, sin copiar. Pide recompilar: los ayudantes van dentro de cada `.mod`. Un `.mod` de antes sigue
     funcionando, con sus sumas sin marcar, como hoy;
   - **`StringBuilder.toString`:** `__strNace(buf)` en vez de `fromBytes(buf)`. El `byte[]` es suyo y muere ahí: se
     marca en su sitio, con una copia menos y sin validar (sus partes son válidas: `#711`, la confianza). `__strNace`
     va sin `public`: un programa que guardara el `byte[]` podría seguir escribiéndole después;
   - **`__strCopy` no lo pone:** es pública y su destino es un `byte[]` del programa.
3. **Lo mira `move`**, en el destino: `move: dst es de solo lectura`, el mismo texto en las dos VMs. No lo miran las
   escrituras `b[i] := x` ni los builtins que reciben un `byte[]`: un programa no puede llevar una cadena ahí (sólo
   `Core` declara `any`, y una cadena no es un `Object`). Las constantes no están en el heap, y `move` ya las rechaza.
4. **El test, la relación de confianza:** un programa del corpus que intenta `move` sobre cadenas de cada origen (una
   constante, `fromBytes`, la suma, `substring`, `charAt`, `upper` y `lower`, `trim`, `replace`, `split`,
   `intToString`, `StringBuilder.toString`, `readFile`, `readLine`, `listDir` y `Json`) y espera la excepción, con la
   cadena intacta; y que sobre un `byte[]` sigue funcionando, también tras compactar. En las dos VMs, con la paridad.
5. **Después, aparte:** quitar de `move` lo que hace hoy por si escribía en una cadena (borrar el bit ASCII del destino
   y subir la época de los cursores).

**`#711`, el descodificador:**
1. **Una rutina, «validar y corregir», en las dos VMs**, por la tabla del estándar de Unicode (qué bytes puede llevar
   cada posición de un carácter; fuera quedan los sobrelargos, los sustitutos y lo que pasa de 10FFFF):
   - primero el atajo: si todos los bytes son ASCII (la pasada que ya hace el bit), vale;
   - si no, valida desde el primer byte no ASCII. Si todo vale, nace igual que hoy;
   - si hay un trozo malo y no se ignora el error: `EncodingError`, con dónde (`fromBytes: UTF-8 mal formado en el
     byte 5`);
   - si se ignora: cada trozo malo, un «�». El trozo es lo más largo que podía ser el principio de un carácter, o un
     byte que no puede empezar ninguno. Se mide antes de reservar: 1 byte malo pasa a 3.
2. **La API:** la tabla de «Decidido el punto 2», con `ignorarError: boolean := false`. Tres builtins nuevos (los
   números del 283 en adelante): `fromBytes`, `readFile` y `__freadLine` con el indicador. Los de hoy se quedan, con
   un argumento, para lo ya compilado, y con la regla nueva (la excepción).
3. **`EncodingError extends RuntimeError`, en `Core`.** La VM la busca por su nombre, como hoy busca `RuntimeError`, y
   si no está (un `Core` de antes) lanza `RuntimeError`. Quien ya atrapa `RuntimeError` la atrapa también.
4. **`appendChar` y `__charsToString`** con algo que no es un carácter: `EncodingError`.
5. **Los tests de la confianza:** la rutina contra un oráculo en Java (todas las secuencias de 1 a 4 bytes, y al
   azar), en las dos VMs; el compilador con cadenas al azar, medios emojis incluidos; y las operaciones entre cadenas
   válidas, en el corpus.

**Los streams, por diseñar aparte** (con las preguntas de «El stream»): la clase base `InputStream` con `read(buf)` (los
bytes leídos; 0 si vence el plazo; −1 al acabarse); las fuentes (`File`, que ya lee bytes, la conexión TCP, un objeto del
puerto serie y un `byte[]` en memoria para las pruebas); `TextReader(fuente)` con `readLine` y `readStr`, que guarda los
bytes pendientes; y en C, dos builtins sobre un trozo de `byte[]` (dónde acaba el último carácter entero, y
descodificarlo con la misma rutina). Ojo con el plazo: con un solo run, una espera que bloquea para también los demás
hilos BP, y eso depende de «los builtins que esperan, que cedan el hilo BP» (la lista de V8).

**El orden propuesto:** primero `#714` (el bit) y el descodificador con su API y `EncodingError`, que están decididos y
son concretos; después el diseño de los streams, para revisarlo. Cada paso por la cascada: la VM-Java, la VM-C con la
paridad, y las placas en un lote.

**`#714` HECHO (23-oct; Eduardo: *«OK, #714»*).** Como estaba escrito, con un cambio:
- **el bit 20**, puesto al nacer: en la VM-C, `bpvm_str_nace` y `bpvm_str_nace_ascii` (antes `bpvm_str_fija_ascii`:
  sólo la usaban los nacimientos); en la VM-Java, `strNace` (antes `strFijaAscii`). Y `readFile` en la VM-C, que no
  nacía. El censo de las dos VMs, completo: toda reserva de bytes que devuelve una cadena pasa por ahí;
- **la suma:** el builtin 283 `__strNace` al final de los `__strconcat` y `__strconcatN` del compilador: 3 bytes por
  ayudante (17 módulos de la stdlib cambian sólo en eso);
- **el cambio:** el `StringBuilder` NO lo usa. Una clase sólo ve lo público de su módulo (la regla del lenguaje), y
  `__strNace` no puede ser pública: un programa que guardara el `byte[]` le seguiría escribiendo. Su `toString` sigue
  con `fromBytes`, que copia y marca. La copia de menos queda para otro día;
- **`move`** lo mira tras ver que los dos son arrays: `move: dst es de solo lectura`, en las dos VMs;
- **las pruebas:** `bpgenvm-c/samples/MoveSoloLectura.bp` (nueva, en el corpus): una constante y cadenas de 23 orígenes
  del heap, intactas, y `move` sobre un `byte[]` y desde una cadena, también tras compactar. Con un sabotaje (el bit
  quitado de `bpvm_str_nace` en la VM-C), las 18 que nacen por ahí dicen «ESCRIBIO» (las 5 sumas, marcadas por el 283,
  no): la prueba lo ve. Un programa que llama a `__strNace` no compila;
- **`StrDosCaminos`**, adaptada: forzaba el camino lento con `move` dentro de una cadena, justo lo que ya no se puede.
  Ahora la cadena lenta es la misma con una `ñ` al final; 136 comparaciones, 0 distintas;
- **la red VERDE,** con el host en su sabor (`make` y `make sim`; con `LVGL=1` se salta la paridad y las pruebas de C):
  paridad 156 en MOD9, en MOD8 y con dos workers; cápsula V6, 59; pruebas de C, 103/103; sombra, 15/15;
- **en placa:** la Metro (M) y la Pico 2 (P), con la imagen nueva (su CRC, el del `.elf`): `MoveSoloLectura` (27 líneas)
  y `StrDosCaminos` (153), igual que el PC. Ojo al capturar: `python -I` no hace caso de `PYTHONIOENCODING`, y la
  salida, redirigida, sale en cp1252 (las líneas con acentos «distintas»); con `-I -X utf8`, en UTF-8. Las ESP32, la
  STM32 y la P4, en el próximo lote: lo compilado ahora llama al 283 en cada suma, y una imagen anterior no lo tiene.

### La biblioteca de cadenas en C (decidida por Eduardo, 21-oct; la forma, por decidir)

**Lo que decidió Eduardo:** *«Creo que deberíamos tener una librería en C para manejar nuestros strings. La librería
debería estar optimizada a nivel de algoritmo y de codificación y verificada. Esto se puede hacer a parte de todo el
resto del código y una vez esté terminada se puede incorporar al resto del proyecto. A nivel de rendimiento podemos
comparar nuestro código con una librería C standar. Si son similares podemos dar a nuestra librería por válida, si es
mucho peor habría que buscar soluciones.»*

Recoge lo que es C de los pasos 2 y 3 de arriba. Siguen fuera de ella lo que es BP (`Json` y `Str` recorriendo por
bytes, el `StringBuilder`) y el GC (`#715`).

**Su pregunta:** *«ARM y RISC-V necesitan librerías diferentes o la misma librería puede servir para los dos?»*

**La respuesta: la misma.** Una biblioteca en C portable sirve para ARM (la Pico 2, la Metro, las STM32), para RISC-V
(la C3, la C5, la C6, la P4), para la S3 (que es Xtensa, una tercera arquitectura) y para el PC. Es lo que ya pasa con
la VM: el mismo `bpgenvm-c/src` en todas. Funciona porque todos estos procesadores son de 32 bits y little-endian, GCC
compila bien el mismo C para cada uno, y los algoritmos rápidos de cadenas (leer de 4 en 4 bytes y operar con enteros)
sólo usan operaciones que tienen todos.

Lo que SÍ cambia, y hay que respetar al escribirla:
1. **Leer 4 bytes de una dirección no alineada.**
   - La M33 de ARM lo hace, más despacio.
   - Las RISC-V de Espressif lo llevan mal: su ROM trae un `memcpy` que va lento con datos desalineados, y el IDF lo
     cambia por otro (`LIBC_OPTIMIZED_MISALIGNED_ACCESS`).
   - La S3 (Xtensa) no lo tolera.

   La biblioteca alinea primero y lee palabras enteras alineadas: vale para todas.
2. **Las instrucciones especiales:** el SIMD de 32 bits de la M33 (su extensión DSP) y las vectoriales de la S3 y la P4;
   la C3, la C5 y la C6 no tienen. No hacen falta. Si una medida enseña que alguna gana mucho, sería una variante
   opcional, con la versión en C de respaldo y las mismas pruebas. Cada variante pide probarse en su hardware, así que
   se empieza por una sola versión.
3. **La libc de debajo** es distinta en cada familia (newlib, picolibc, la ROM, newlib-nano byte a byte). Para lo
   caliente (copiar, comparar, buscar), la biblioteca puede llevar lo suyo y no depender de ella. Lo decide la medida.

**Una forma posible, para hablarla:**
- **La frontera: la biblioteca no reserva memoria.** Trabaja sobre bytes que le dan (un puntero y una longitud); quien
  crea la cadena (la VM, el AOT) reserva y le pide que la llene. Así se prueba sola, sin la VM, y la usan igual el
  intérprete y el AOT.
- **Lo que tendría,** por el censo:
  - el UTF-8: contar caracteres, el byte donde empieza el carácter `i`, decodificar, validar (con la regla de `#711`)
    y «es todo ASCII»;
  - buscar: hacia delante, hacia atrás (`lastIndexOf`) y contar apariciones (para `split` y `replace`);
  - comparar: igualdad y orden (el orden de los bytes en UTF-8 es el de los caracteres; sirve para `Str.compare`, y
    para `#713` si se decide comparar);
  - transformar: mayúsculas y minúsculas, los bordes de `trim`, `replace` y `split` sobre un búfer;
  - los números: leer enteros y reales con reglas propias (`#716`), y escribirlos (el formateador de `double` ya es
    nuestro);
  - copiar y rellenar, si la medida dice que la libc de alguna placa no basta (la STM32).
- **Los algoritmos, por caso** (lo que Eduardo preguntaba antes):
  - contar y saltar caracteres, de 4 en 4 bytes, con un atajo cuando los 4 son ASCII;
  - buscar: una aguja de 1 byte, de 4 en 4; las cortas, el primer byte y comparar; las largas, Horspool o Two-Way (a
    partir de qué longitud, medido);
  - las mayúsculas, de 4 en 4 cuando son ASCII.
- **La codificación:**
  - lecturas alineadas, nunca más allá del final;
  - sin depender del orden de los bytes salvo donde se diga (todas son little-endian: comprobado al compilar);
  - y quizá compilada con `-O2` aunque el resto vaya con `-Os` (medido).
- **La verificación:**
  - **El oráculo:** cada función, contra una versión simple y evidente (los bucles de hoy).
  - **Exhaustivo en lo corto:** todas las cadenas de hasta 3 bytes, 16,7 millones.
  - **Los bordes:** longitudes de 0 a 64, con las 4 alineaciones.
  - **El catálogo de UTF-8 mal formado:** el «UTF-8 decoder stress test» de Markus Kuhn, que es el de referencia.
  - **Millones de pruebas al azar** contra el oráculo.
  - **Primero en el PC, y luego en cada arquitectura.** QEMU no está instalado: sin él, en las placas (la Pico 2 para
    ARM, la P4 o la C5 para RISC-V, la S3 para Xtensa), o descargándolo.
- **La medida y el criterio de Eduardo:** cada función contra su pareja de la libc donde la hay (`memchr`, `memcmp`,
  `memmem`, `memcpy`), en bytes por ciclo, en el PC y en cada arquitectura.
  - En ARM, contra la newlib completa de la RP2350: la STM32 tiene la misma M33 y su newlib-nano copia byte a byte.
  - Lo que no tiene pareja (contar UTF-8, las mayúsculas), contra lo que cuesta leer la memoria una vez (el ritmo de
    `memchr`).
  - «Similar», por fijar (¿±25 %?).
- **Dónde vive:** aparte, en su carpeta (`bpgenvm-c/bpstr/`, por ejemplo), con su `Makefile`, sus pruebas y su banco.
  Al acabar, se da de alta en los cinco builds y las primitivas de la VM la llaman. La VM-Java no la usa (es C), pero
  tiene que dar lo mismo: lo vigila la paridad.

**Los bytes: algoritmos estándar sobre UTF-8** (Eduardo, 21-oct: *«Recuerdo del ensamblador del x86 que se podía
buscar un caracter en una cadena con una sola instrucción. En ARM o en RISC-V no habrá exactamente lo mismo pero si que
con unas pocas líneas puede haber algo parecido. Lo que quiero decir es que aunque estemos con UTF-8 i podemos
aprovechar algoritmos mas o menos estandard para bytes.»*)

**Sí: el UTF-8 se diseñó para eso.**
- **Un carácter ASCII nunca aparece dentro de otro:** todos los bytes de un carácter largo valen 0x80 o más. Así que
  buscar `,`, `"`, `:` o el fin de línea es buscar un byte (`memchr`).
- **Una aguja bien formada sólo casa en el principio de un carácter.** El UTF-8 se resincroniza solo, así que buscar
  una subcadena es buscar bytes (`memmem`). `bp_find_bytes` ya se apoya en esto.
- **Comparar bytes da el mismo orden que comparar caracteres** (`memcmp`).
- **Contar caracteres es contar los bytes que no son de continuación** (los que no empiezan por `10`).

Sólo hace falta decodificar para el valor de `charCodeAt`, las mayúsculas fuera de ASCII y validar. Y aun ahí hay atajo
cuando los 4 bytes de una palabra son ASCII.

**La instrucción del x86** (`REPNE SCASB`) existe, pero en los procesadores de hoy va despacio. Las bibliotecas rápidas
del x86 usan en su lugar SIMD: comparan 16 o 32 bytes por paso.

**Lo que tiene cada uno de los nuestros:**
- **Todos (ARM, RISC-V, Xtensa), en C normal:** el truco clásico de 4 en 4. Se hace un XOR de la palabra con el byte
  buscado repetido (`c × 0x01010101`), se resta `0x01010101` y se hace AND con la negación de la palabra y con
  `0x80808080`. Da distinto de cero si algún byte casa. Son unas 5 instrucciones por cada 4 bytes, frente a 4 o 5 por
  byte en un bucle simple, y valen en cualquier procesador de 32 bits.
- **La M33 de ARM** (la Pico 2, la Metro, las STM32): su extensión DSP compara 4 bytes por pareja de instrucciones
  (`UADD8` y `SEL`), y `CLZ` con `REV` dice cuál casó. Es lo que usa el `memchr` optimizado de ARM. El de la newlib de
  la RP2350 parece ese: ocupa 148 bytes (sin comprobar su código). Hoy no lo usamos: `indexOf` llama a `memcmp` en cada
  posición.
- **Las RISC-V de Espressif:** la C3 es `rv32imc`, la C5 y la C6 `rv32imac`, y la P4 `rv32imafc` (leído en sus
  builds). Ninguna tiene la extensión Zbb, que trae `orc.b`, una instrucción hecha justo para esto. Así que el truco en
  C. La P4 tiene además un SIMD propio de 128 bits (16 bytes por instrucción), sólo en ensamblador suyo: una variante,
  si algún día se mide que compensa.
- **La S3 (Xtensa):** el truco en C, con lecturas alineadas.

**Lo que daría, estimado y por medir en la biblioteca.** Hoy, en la RP2350, contar va a 64 ns por byte (unos 10
ciclos) y buscar a 179. De 4 en 4 serían 1,5–2 ciclos por byte, 10–13 ns: unas 5 veces más rápido contar y unas 15
buscar. El tope es la memoria: con la PSRAM de la Metro, lo que no está en la caché llega a unos 50 ns por byte.

**Los algoritmos estándar** que se pueden aprovechar:
- `memchr` de 4 en 4;
- para buscar subcadenas, el primer byte y comparar si la aguja es corta, y Two-Way o Horspool si es larga;
- las técnicas conocidas para contar y validar UTF-8 de palabra en palabra.

Hay implementaciones públicas que se pueden leer o reutilizar, mirando su licencia: la de musl es MIT y la de newlib,
BSD.

**Lo que hay que decidir antes de empezar:**
1. la regla del UTF-8 mal formado (`#711`), porque la biblioteca la fija;
2. lo que entra (la lista de arriba) y lo que se deja para después;
3. qué cuenta como «similar» frente a la libc;
4. dónde se prueba en ARM, RISC-V y Xtensa: en las placas, o con QEMU (descargarlo).

---

## E6 · Los conjuntos, como en Pascal (la idea de Eduardo, 23-oct; sin empezar)

Eduardo, 23-oct: *«Como idea, que te parece añadir sets? De momento limitado a 31 elementos y representados por un
uint32. Los sets son similares a los de Pascal con operaciones de pertenencia, intersección, unión, etc. En principio es
todo a nivel de compilador, a nivel de VM creo que con las operaciones lógicas y de manipulación de bits bastaría.»* Y,
al contestarle: *«OK, terminamos el #714 primero»* (va después de los pendientes).

Lo visto ese día, para el análisis:
- **encaja en un micro:** las máscaras de bits están por todas partes (registros, pines, eventos, banderas), y hoy se
  escriben a mano con enteros y `&`/`|`;
- **la VM ya tiene lo que hace falta:** `&`, `|`, `xor`, `shl` y `shr`. Un conjunto es un entero, sin heap; cada
  operación, una o dos instrucciones. Contar sus elementos (`card`) sería un builtin pequeño o una vuelta de bits;
- **las palabras:** `in` ya es reservada (el `for … in`) y `set` también (las propiedades); `of` no, pero puede valer
  sólo dentro de un tipo, tras `set`, sin reservarla;
- **un ejemplo:** `var c: set of Color := [ROJO, AZUL]`; `VERDE in c`; `c + d` (unión), `c * d` (intersección), `c - d`
  (diferencia), `c <= d` (subconjunto); `for x in c … next`;
- **por decidir:** qué elementos (los `enum`, con valores de 0 a 31: un `enum` puede llevar valores explícitos, como
  `AZUL := 10`, y el valor es el bit); 31 o 32 elementos (con 32 bits caben 32: el bit 31 sólo vuelve negativo el
  entero, y las operaciones de conjunto no lo miran como número); cómo se imprime (un `enum` se imprime como su número:
  los nombres no existen al ejecutar);
- **lo que cuesta:** el compilador (el tipo, los literales, los operadores, `in` como expresión, el `for`); la interfaz
  que va dentro del `.mod` (un conjunto en una firma pública); JC y sus dos traducciones; el AOT (en C es inmediato); el
  depurador, el resaltado del IDE y la documentación.

**La declaración, la idea de Eduardo (23-oct):** *«Como se declaran? Similar a los enun, como set ya está cogido
podemos utilizar Set32 o algo parecido. Un ejemplo: Set32 controlBits = { bitOn, bitBlink, bitRed } . Una declaración de
variable: var control: controlBits = { bitOn, bitRed }»*. Lo que se le propuso, en la forma de BasicPlus: `set` en vez
de `Set32` (ya está reservada, así que no gasta ninguna: hoy sólo aparece dentro de una propiedad, `set(valor) …
endset`), un elemento por línea como el `enum`, y `:=`:

```
public set ControlBits
  bitOn
  bitBlink
  bitRed := 0b1000_0000
end ControlBits

var control: ControlBits := { bitOn, bitRed }
```

Las llaves están libres en las expresiones (una tupla se construye con paréntesis, `return (11, 12)`; las llaves sólo
van a la izquierda, `{ a, b } := f()`). En JC: `set ControlBits { BitOn, BitBlink, BitRed = 0b1000_0000 }` y
`ControlBits control = { BitOn, BitRed };`.

**Y lo que precisó Eduardo (23-oct):** *«Los elementos de los sets son constantes de tipo integer así que si se pueden
declarar de forma explícita pero tienen que coincidir con bits así que siempre han de ser potencias de 2: 1,2,4,8,16,
etc […] En los conjuntos podemos tener 0 bits, 1 bits o n bits activados que es lo que señala que el set contiene uno o
varios elementos. Añadir un elemento al conjunto se puede representar con una suma pero se realiza con un 'or' y quitar
un elemento se puede representar con una resta pero se tiene que hacer con un 'and' del complemento.»* Con eso:
- **el valor de un elemento es su MÁSCARA**, no el número del bit: una potencia de 2. Sin valor explícito, 1, 2, 4, 8…
  (como el `enum`, doblando). Con valor, el compilador comprueba que sea potencia de 2 y que no se repita. BasicPlus ya
  admite `0b1000_0000` y `0x80`. Así `integer(control)` es el valor del registro, sin desplazar nada;
- **31 elementos:** de 2^0 a 2^30 (`0x4000_0000`), todos enteros positivos (2^31 sería el bit de signo);
- **`+` se escribe, `|` se hace; `-` se escribe, `& ~` se hace.** Con una suma de verdad, añadir uno que ya está
  arrastraría al bit siguiente (`bitOn + bitOn` daría `bitBlink`); con una resta, quitar uno que no está tomaría
  prestado. La intersección `*` es `&`; `e in s`, `(s & e) != 0`; el subconjunto `a <= b`, `(a & ~b) == 0`; la igualdad
  compara los enteros; el vacío `{}` es 0;
- **un elemento ya es un conjunto de uno:** `control + bitBlink` es lo mismo que `control + { bitBlink }`, y un literal
  `{ bitOn, bitRed }` lo calcula el compilador;
- **el tipo los protege:** por valor son enteros, pero su tipo es el del conjunto (`ControlBits`): `control + 3`, o
  mezclar dos conjuntos distintos, es un error de compilación. Con un entero, las conversiones explícitas
  (`integer(control)`, `ControlBits(registro)`).

**Decidido (Eduardo, 23-oct: *«1 ControlBits.bitOn»*):** fuera de las llaves, el elemento lleva el nombre de su
conjunto, `ControlBits.bitOn in control`, como el `enum` (`Dia.MIERCOLES`). Dentro de las llaves, el nombre solo, como
en su ejemplo: el conjunto ya se sabe por el tipo (`var control: ControlBits := { bitOn, bitRed }`).

**Lo que quedaba: un bit no declarado al convertir un entero (`ControlBits(registro)`), y cómo se imprime.** Se le
propuso dejar el bit (como Pascal y C#), para que un registro leído, cambiado y escrito de vuelta no perdiera sus bits
reservados. **Decidido** (Eduardo, 23-oct): *«un conjunto tiene un tamaño. Lo representamos con un entero de 32 bits
pero el conjunto de 4 elementos tiene un tamaño de 4. Bien, si en una operación intentamos añadir un bit de fuera de
rango, lo correcto sería una excepción pero a mi si se ignora el bit fuera de rango y no se almacena también me vale.»*
Con eso:
- **el tamaño de un conjunto es el de sus elementos:** sus bits son los de sus elementos declarados, una máscara que
  calcula el compilador (en `ControlBits`, `0x83`). Con valores explícitos puede tener huecos: `0x04` también está
  fuera;
- **un bit de fuera es una excepción** (lo correcto, según Eduardo; ignorarlo también le valía). Sólo puede entrar por
  un sitio, la conversión desde un entero: las operaciones entre dos conjuntos del mismo tipo (`|`, `& ~`, `&`) no
  pueden sacar un bit de fuera, y el tipo no deja sumarle un entero. Se mira ahí y sólo ahí, con un `&` y una
  comparación; con una constante (`ControlBits(0x100)`), el error lo da el compilador;
- **el entero y el conjunto son siempre lo mismo:** la igualdad compara los enteros, contar los elementos es contar los
  bits a 1, y recorrerlo es recorrer esos bits;
- **un registro con más bits** se enmascara a la vista, `ControlBits(registro & 0x83)`, y al escribirlo de vuelta se le
  juntan los suyos: `registro := (registro & ~0x83) | integer(control)`. Por decidir en el análisis: cómo se nombra el
  conjunto entero, para no escribir `0x83` a mano;
- **un complemento**, si lo hay, dentro del tamaño (`0x83 & ~control`), nunca `~control`.

**Y lo demás, con su «OK»:** se imprime el número, como el `enum` (en hexadecimal, `Str.toHex(integer(control))`); los
nombres, si algún día se quieren, para los dos tipos a la vez. Y se declaran con `set`, que ya es reservada.

Lo siguiente, cuando Eduardo diga: el análisis completo, con la sintaxis de BP y la de JC.
