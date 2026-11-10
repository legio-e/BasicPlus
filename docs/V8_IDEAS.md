# V8 — ideas y diseño

> 🚀 **Desde el 14-oct-2026, V8 es la versión en curso** (V7 se publicó el 13-oct, tag `v7.0`). Éste es el cuaderno de
> diseño de V8, hermano de `V7_IDEAS.md`: las charlas de diseño van aquí **antes** de escribir código. El estado vive en
> `FICHAS.md` §ABIERTAS, bloque V8.
>
> **El modo de V8** (Eduardo, 13-oct): *«V8 es para probar cosas nuevas, para experimentar, sin ponernos obligaciones.
> Probaremos cosas y lo que funcione lo subiremos al final y lo que no funcione se puede aplazar para más adelante.»*
> El tema, decidido el 24-sep: la WiFi, el Bluetooth y el bajo consumo. El bajo consumo pasó a V9 el 6-nov (Eduardo: *«Es demasiado complejo y depende mucho del hardware»*).
>
> **Los hitos** (Eduardo, 18-oct): `H` numerados, como en V7, con sus apartados en `FICHAS.md`, y los nombra Eduardo.
> **H1 = JC** (E2, abajo), cerrado el 18-oct. **H2 = el Bluetooth** (E1, abajo), a medias y parado. **H3 = la WiFi**
> (E3, abajo), cerrado el 7-nov. **H4 = el sonido en la P4** (E4, abajo): el `.wav` suena desde el 19-oct;
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

### Lo que ve el programa: el diseño del 8-nov (Eduardo)

Al retomar H2 (8-nov, con la S3, la C5 y la P4 conectadas). Eduardo: *«Funcionalidades, encender y apagar, scan,
conectar y desconectar. Ahora una vez tenemos un ratón o un teclado conectados hay que engancharlos al resto del
sistema»*; y el criterio: *«No hay prisa, tardamos lo que sea necesario. Podemos hacer mas o menos pero lo que hagamos
ha ser funcional, algo que sea útil al usuario.»*

**El Bluetooth, `Air.Bluetooth`** (la clase que ya existe, vacía, con la forma de `Air.Wifi`):

```basic
import Air
var bt: Air.Bluetooth := Air.Bluetooth()
bt.turnOn()
var vistos: List := bt.scan(10000)        // 10 s: los aparatos en modo de emparejar (nombre, dirección, intensidad)
var d: Air.BtDevice := vistos.get(0)
bt.connect(d)                             // la primera vez empareja y lo recuerda; después, por su dirección
bt.disconnect(d)                          // cierra esa conexión; el Bluetooth sigue, con los demás
bt.forget(d)                              // lo olvida, con sus claves
var guardados: List := bt.listDevices()   // los recordados, cada uno con si está conectado
bt.turnOff()
```

- **`disconnect`** (Eduardo: *«desconectar me refiero a cerrar la conexión con el dispositivo, pero no cerramos el
  bluetooth porque podemos tener mas dispositivos conectados»*): el aparato sigue recordado, pero no vuelve solo hasta
  otro `connect` o el siguiente arranque.
- **Reconectar, por la dirección guardada**, sin volver a emparejar (*«Ya probamos que al scanear y seleccionar
  guardabamos la MAC address del dispositivo, así que cuando tuviésemos que volver a conectar la utilizamos en vez de
  volver a emparejar»*; lo probado el 14-oct). `forget` es lo único que obliga a emparejar otra vez (*«1) OK»*).
- **Lo recordado**, en un JSON de `/sys`, con las claves fuera de la NVS (14-oct). Al arrancar, si el Bluetooth está
  activado, el firmware espera a los recordados, con cualquier programa o sin ninguno.
- Los nombres siguen la convención (métodos con verbo): `listDevices()`, no `devices()`.

**El teclado y el ratón, enganchados al resto del sistema**, de dos formas:

1. **En un programa con GUI, sin escribir nada** (14-oct, la API de entrada genérica): el ratón mueve una flecha por
   la pantalla y hace clic; el teclado escribe en el campo con el foco, Tab pasa al siguiente y Enter pulsa el botón.
   Los programas de hoy funcionarían tal cual.
2. **Con una clase para cada uno, en su módulo**. Eduardo: *«Cada uno con un módulo propio»*, y *«Los módulos en
   plural la clase en singular»*, como `Overlays` con `Overlay`:

```basic
import Keyboards
import Mice
var tk: Keyboards.Keyboard := Keyboards.Keyboard()
tk.onKey := this::tecla
var ra: Mice.Mouse := Mice.Mouse()
ra.onClick := this::clic

function event tecla(t: Keyboards.Keyboard, c: string, codigo: integer)
  print "tecla: " + c                   // "a", "Ñ"…; las especiales (Enter, flechas), por su código
end tecla

function event clic(r: Mice.Mouse, boton: integer, x: integer, y: integer)
  print "clic en " + x + ", " + y
end clic
```

   - **Eventos y preguntar directamente, las dos cosas** (*«2) OK»*): además de `onKey`, `tk.hasKey()` y
     `tk.readKey()`, como el `INKEY$` de los BASIC; y del ratón, su posición y sus botones.
   - **Las distribuciones de cada país** (Eduardo: *«En el caso del teclado habrá que diseñar teclados por paises así
     que no será únicamente la clase Keyboard»*): van en `Keyboards`, con la US estándar primero (14-oct) y las demás
     después; la de cada placa se elige en la configuración.
   - **El ratón va con una pantalla** (Eduardo: *«Si no hay pantalla un ratón no tiene mucho sentido. Pero el S3 que
     ahora no tiene pantalla el usuario podría conectarle una por SPI. Hay pantallas sin táctil así que un ratón le
     proporciona una forma de interactuar»*): su posición son los puntos de esa pantalla, la de la placa o una por SPI.
     Sin pantalla, se puede conectar, pero no hace nada.
   - **Independientes del Bluetooth**: mañana puede ser un teclado USB (la P4 y la S3 tienen USB *host*) con las mismas
     clases.

**Primero en el PC, en las dos VMs** (la cascada): allí el teclado y el ratón son los del PC, y el Bluetooth tiene
aparatos simulados, como el I2C y el SPI. Después, la S3, la C5 y la P4.

**En el PC, lo decidido** (Eduardo, 8-nov: *«1) si 2) nada 3) solo con el teclado conectado»*). Lo que había: la GUI
de la VM-Java es Swing (sus campos y botones ya responden al teclado y al ratón del PC); la de la VM-C y el micro
simulado, LVGL en una ventana de SDL, con el ratón registrado y del teclado sólo F12 (la captura). Sin GUI, la consola:
`input()` lee una línea, sólo en la VM-Java (26-ago: es para depurar), y tecla a tecla la VM-Java no puede sin una
librería.
1. **El teclado y el ratón del PC son los de la ventana de la GUI**, en las dos VMs: la de Swing y la de SDL, a la que
   hay que añadirle el teclado. Así se prueba en el PC la GUI con teclado (el foco, Tab, Enter).
2. **Sin GUI, en el PC no hay teclado**, como una placa sin teclado conectado. (Lo descartado: una ventanita para
   las teclas, o la consola del IDE mandándolas a la VM, que también valdría de teclado remoto para la placa.)
3. **Las teclas del PC, sólo con el teclado simulado conectado**: `scan()` encuentra «Teclado simulado» y «Ratón
   simulado», y `connect()` los recuerda en el `/sys` del PC. Sin conectar, el programa no recibe teclas, como en la
   placa: un programa que se olvida de conectar falla en el PC igual que fallaría allí. Recordado, la siguiente
   ejecución ya lo tiene, como la placa al arrancar.
4. **El ratón del PC sigue haciendo de táctil en la GUI**, siempre, como hoy: es la única forma de usarla en el PC.
   Como ratón (la clase `Mice.Mouse`, sus eventos, su posición y la flecha), sólo con el ratón simulado conectado. Como
   en una placa con táctil y un ratón Bluetooth: la táctil siempre; el ratón, cuando está conectado (Eduardo: *«OK»*).

**En dos niveles** (Eduardo, 8-nov: *«A ver, en dos niveles. Un teclado a nivel de SO. Un teclado a nivel de programa
( un objeto ) . El teclado a nivel de SO es único, con su buffer. El de programa puede ser mas de uno, y se relaciona con
el del SO»*):
- **El teclado del SO**: uno solo, en la VM y en el firmware, con su buffer. A él llegan las teclas de todos los
  teclados conectados (uno Bluetooth, mañana uno USB; en el PC, la ventana de la GUI con el teclado simulado
  conectado). Existe aunque el programa no cree ningún objeto. El ratón, igual: uno del SO, con su posición y sus
  botones, que junta todos los ratones conectados.
- **El del programa**: un objeto `Keyboards.Keyboard`, uno o varios. La relación (*«Si»*): **los eventos, para todos**
  (cada tecla que entra en el buffer del SO se avisa a todos los objetos con `onKey`), y **preguntar lee el buffer del
  SO** (`hasKey()` mira si hay una; `readKey()` la saca: con dos objetos, el primero que lee se la lleva, como dos
  lectores de la misma consola).

```basic
var a: Keyboards.Keyboard := Keyboards.Keyboard()
var b: Keyboards.Keyboard := Keyboards.Keyboard()
a.onKey := this::teclaA
b.onKey := this::teclaB
// llega una «x» al buffer del SO → teclaA y teclaB la reciben, las dos
// y si en vez de eventos se pregunta: a.readKey() da «x», y después b.hasKey() da false
```

**Cómo llegan los eventos: los entrega la VM.** Hoy un `raise` encola el evento y el manejador corre después, en el
hilo que lo hizo; lo único que nace fuera de BP, el clic de la GUI, lo apunta la VM y lo reparte la bomba de la GUI
(`Gui.run()`), en BP. El teclado necesita eventos también sin GUI. De las tres formas (que los entregue la VM; una
bomba en BP, un hilo que pregunta cada pocos milisegundos y mantiene vivo el programa; o eventos sólo en la GUI), la
primera: cada objeto se apunta en la VM al crearse, y cuando llega una tecla la VM encola, en el hilo que lo creó, una
llamada a un método suyo (`__despacha`) que hace el `raise onKey` en BP. Usa la cola de eventos que ya existe y la
búsqueda por nombre de la GUI; la lista de los apuntados no mantiene vivos los objetos.

**Por detallar, al llegar a cada parte:**
- el evento de una tecla: si avisa también al soltarla, y los códigos de las especiales;
- los eventos del ratón (moverse, la rueda) y su forma de preguntar;
- la regla de las radios (§E3) con un teclado o un ratón conectados: qué cuenta como actividad para apagar a los
  10 minutos;
- el nombre del JSON de `/sys` y si las claves van en él o aparte (un fichero que el IDE no enseñe);
- **el paso 4, el programa de configuración** (Eduardo: *«El punto 4 habrá que detallarlo»*): sus pantallas y cómo
  se lanza.

### El paso 1, hecho (8-nov): el Bluetooth simulado del PC, en las dos VMs

El orden de trabajo (Eduardo: *«OK»*): 1) el Bluetooth simulado del PC; 2) el teclado del SO y `Keyboards.Keyboard`;
3) el ratón; 4) la GUI; 5) las placas (NimBLE en la S3, la C5 y la P4); 6) el programa de configuración.

- **Lo común, en la HAL BP** (`bpvm_bt.c`, como `bpvm_wifi.c`): el estado (0 apagado, 1 encendiéndose, 2 encendido,
  3 buscando), lo que encontró la última búsqueda (hasta 16, sin los conectados), lo conectado (hasta 8) y, al
  encender, los recordados, que se conectan por su dirección. La radio, una tabla por nombre (`'BT  '`); la del PC,
  `bt_host.c`, y en la VM-Java, `Bluetooth.java`, las dos con los mismos dos aparatos: «Teclado simulado»
  (`02:00:00:00:00:01`, que en la búsqueda dice `unknown`, como uno de verdad, y al conectarlo, `keyboard`) y «Ratón
  simulado» (`02:00:00:00:00:02`). Los builtins, del 309 al 324 (`BUILTINS.md`).
- **El fichero, `/sys/bluetooth.json`**: lo escribe la clase, en BP y con `Json`; la VM sólo lo lee, al encender y al
  arrancar. Los aparatos van en un objeto con la dirección de clave, no en una lista: `json_min`, el lector de la VM-C,
  no lee listas.
  ```
  { "autoStart": true,
    "devices": { "02:00:00:00:00:01": { "name": "Teclado simulado", "addressType": "random", "kind": "keyboard" } } }
  ```
- **Lo que añadí a la lista de Eduardo**: `isOn()`; si arranca encendido, `isAutoStart()` y `setAutoStart(b)` (sin
  ellos, nadie podía activarlo); `turnOn()` da `true` cuando está listo, como `Air.Wifi`, y espera hasta 5 s; `connect`
  de uno que no está da `false`, y con el Bluetooth apagado, un `RuntimeError`, como `scan`.
- **Lo que decidió Eduardo al verlo** (8-nov):
  1. **El nombre, el de la WiFi** (*«como en la wifi»*): `"autoStart"`, como en `/sys/wifi.json`; lo había llamado
     `"enabled"`, con `isEnabled()` y `setEnabled(b)`.
  2. **Al encender, los recordados vuelven solos, sin error** (*«La verdad es que no lo se, de momento que se conecten
     solos al activar el Bluetooth ( al menos que lo intenten pero si no están o no conectan que no se produzca un
     error )»*): `turnOn()`, y el arranque con `"autoStart"`, intentan conectar todos los recordados, también uno que se
     desconectó con `disconnect`; el que no está o no conecta se queda sin conectar, y nada más. Era lo que ya hacía;
     ahora `AirBluetooth` lo prueba, con un recordado que no existe.
  3. **El micro simulado del IDE, con el Bluetooth simulado** (*«ponlo»*): `bpvm_sim.c` registra la misma radio que la
     VM-C del PC, y al arrancar mira su `/sys/bluetooth.json`, el de su littlefs.
  4. **`#810`, cuando sea** (*«cuando sea ahora no es importante»*).
- **El fin de RUN**: si el programa lo tocó, vuelve a lo que diga el fichero, como la WiFi.
- **La memoria, pensando en la C3 y la C6** (`MEMORIA.md`: van justas): `bpvm_bt.c` entra en todas las imágenes, y en
  una placa sin radio gasta 156 B de RAM (medido en la Pico: 96 de datos y 60 de bss; la WiFi, 444) y 3 KB de flash. Las
  listas (~1,4 KB) se reservan al encender la primera vez, y el búfer del fichero (1,5 KB), sólo mientras se lee, con
  `bpvm_malloc_os`, como el sonido.
- **Hizo falta `#808`**: el `/sys` del PC iba a la raíz del disco (`C:\sys`). Ahora las dos VMs del PC tienen una
  carpeta que hace de flash (`~/.basicplus/flash`, o `--flash-dir=`), y la red le da a cada ejecución una nueva.
- **La prueba**, `bpgenvm-c/samples/AirBluetooth.bp`, en la red: las dos VMs dicen lo mismo. Y a mano, el arranque:
  con el fichero en `"autoStart"`, la VM arranca encendida y con los recordados conectados.

### El paso 2: el teclado del SO y `Keyboards.Keyboard` (8-nov, en análisis)

**Lo que hay hoy** (el mapa del 8-nov): un evento (`raise`) se encola y lo ejecuta el hilo que lo lanzó, cuando el
planificador le da turno; nada de fuera de BP mete eventos todavía. El clic de la GUI lo apunta la ventana y lo recoge
un hilo BP, el de `Gui.run()`, que pregunta cada 10 ms. Lo único que la VM ya mete por su cuenta entre turno y turno son
los destructores (`#668`). En la VM-C, con un run, la cola de eventos no tiene cerrojo: lo que venga de otro hilo
necesita su propia entrada, con el suyo.

**Cómo, lo propuesto** («los entrega la VM», lo decidido el 8-nov):
- el **teclado del SO**, un búfer en la VM (y en el firmware), con su cerrojo: las teclas llegan desde el hilo de la
  ventana (en la placa, desde el Bluetooth);
- cada **`Keyboards.Keyboard()` se apunta en la VM** al crearse, sin que eso lo mantenga vivo;
- **entre turno y turno**, donde hoy se reparten los eventos y los destructores, la VM pasa cada tecla nueva a todos los
  teclados apuntados y vivos: encola, en el hilo que creó cada uno, su `__despacha`, que hace el `raise onKey`;
- **`hasKey()` y `readKey()` leen el búfer del SO** (el primero que lee se la lleva);
- **en el PC**, las teclas de la ventana entran sólo con «Teclado simulado» conectado; y **para la red**, sin
  ventana, un builtin que mete teclas por programa, como `Gui.__guiClick`.

**Lo decidido** (Eduardo, 8-nov):
1. **Los códigos, los del estándar HID** (*«si»*): los que manda un teclado Bluetooth o USB, los mismos de SDL, con
   nombre en un enum, `Keyboards.Key` (`ENTER`, `ESCAPE`, `UP`, `F1`…).
2. **El evento, al apretar** (*«De momento al apretar»*). **Más adelante, el tiempo** (*«Mas adelante igual nos
   interesa el tiempo porque una tecla de encendido pulsada brevemente es para encender pero una tecla de encendido
   pulsada varios segundos es para apagar»*): saber cuánto ha estado pulsada, es decir, también cuándo se suelta.

3. **Una tecla que se mantiene no repite** (*«OK»*): un aviso por pulsación, igual en el PC y en la placa (en el PC se
   ignoran las repeticiones del sistema); así, cuando se mida el tiempo, no habrá repeticiones de por medio.
4. **`readKey()` devuelve un int32** (Eduardo: *«Devuelve un int32, la parte baja el código de la tecla y la parte alta
   los bits de estado ( shift, alt, ctrl, alt gr, Fn, Windows, etc.»*), en vez del objeto por tecla que propuse.

5. **El carácter** (*«3) y 4) si»*): en el PC, el que da el sistema (la distribución del teclado del PC); en las
   placas, las tablas de BasicPlus, la US primero.
6. **`#811`, ahora**, antes de seguir con el paso 2 (el mismo *«si»*).

7. **El evento, el int32 y el carácter** (Eduardo: *«OK a los 3»*): `onKey(t: Keyboards.Keyboard, k: integer, c:
   string)`; el carácter, `""` en una especial. Quien pregunta: `readKey()` da el int32 y `getChar()` el carácter de la
   última leída. La VM guarda las dos cosas juntas en el búfer de cada tecla: el int32 no puede llevar el carácter, y en
   el PC sólo lo sabe el sistema.
8. **Los bits de estado, el byte de modificadores del estándar HID** (el que manda el teclado), en la parte alta: 16
   Ctrl izquierdo, 17 Shift izquierdo, 18 Alt, 19 Windows izquierdo, 20 Ctrl derecho, 21 Shift derecho, 22 AltGr (el
   Alt derecho), 23 Windows derecho; y 24 Bloq Mayús, 25 Bloq Num, 26 Bloq Despl y 27 Fn. Con funciones para no mirar
   bits: `Keyboards.getCode(k)`, `isShift(k)`, `isCtrl(k)`, `isAlt(k)`, `isAltGr(k)`, `isWin(k)` e `isFn(k)`.
9. **Comparar con una tecla, `Keyboards.isKey(k, Keyboards.Key.ENTER)`**: un enum no se compara con un entero.
10. **El hilo de los eventos** (Eduardo: *«El teclado (el objeto) sería crearlo en el mismo hilo que el gui. Ahora el
    teclado puede tener un evento así que el evento lo captura quién lo instancie»*). Llevar la GUI al hilo del main,
    no (*«Pues no. El LVGL tiene la manía de acaparlo todo y si dejas el LVGL en el hilo principal ya casi no puedes
    hacer nada mas»*). Lo que se queda (*«OK, si, mucho mejor»*):
    - **en un programa con GUI, el teclado lo crea la propia GUI, en su hilo**: lo instancia y captura sus eventos, y se
      los pasa a LVGL (en la VM-Java, a Swing): el campo con el foco recibe los caracteres, Enter pulsa el botón, Tab
      pasa al siguiente. Y si el programa quiere las teclas tal cual, la pantalla tiene su evento,
      `scr.onKey := pant::tecla`, que la GUI lanza en su hilo, como un clic. Es el paso 4;
    - **sin GUI**, el programa crea su `Keyboards.Keyboard` y sus eventos van al hilo que lo creó, que tiene que estar
      corriendo (un bucle con `sleep`, por ejemplo); `hasKey()` y `readKey()`, desde cualquier hilo. Es el paso 2.

**El plan para programarlo** (8-nov, al parar para el descanso; lo interno es mío, lo que ve el programa está decidido):
- **El teclado del SO, en la HAL BP** (`bpvm_teclado.c`; en la VM-Java, `Teclado.java`), con su cerrojo: el búfer que
  lee `readKey()` (32 teclas) y, aparte, las que faltan por avisar (16). Cada tecla, el int32 y su carácter como código
  Unicode (0 en una especial): así la VM no reserva cadenas; las hace `Keyboards`, en BP, con `Str.fromCharCode`.
- **Los teclados del programa:** `Keyboards.Keyboard()` llama a `__kbOpen(this)`: la VM apunta la referencia entera
  (con su generación, que dice si sigue vivo: no lo mantiene vivo), el hilo que lo crea y la ranura de su `__despacha`,
  por nombre (lo que ya hace `__guiSlotOf`). `close()` y el destructor, `__kbClose(this)`.
- **El reparto, entre turno y turno**, donde hoy la VM saca un evento por turno: por cada tecla por avisar y cada
  teclado vivo, un evento de la cola de siempre para su hilo (`__despacha(k, cp)`, que hace el `raise onKey`). Si la
  cola (16) no tiene sitio para todos, la tecla espera al turno siguiente; si el hilo ya no existe, el teclado se borra.
- **Los builtins, del 325 al 331:** `__kbOpen`, `__kbClose`, `__kbHasKey`, `__kbReadKey` (el int32; su carácter se
  queda en el hilo), `__kbReadChar` (el código del carácter de la última leída por ese hilo), `__kbInject(k, cp)` (para
  las pruebas: entra como una tecla de verdad) y `__kbIsConnected`.
- **En el PC, las teclas de la ventana**, sólo con un teclado conectado (`bpvm_bt`: uno de clase `keyboard`): en la
  VM-C y el micro simulado, los `SDL_KEYDOWN` (el `scancode` de SDL ya es el código HID; sin las repeticiones) y su
  `SDL_TEXTINPUT` (el carácter); en la VM-Java, un `KeyEventDispatcher` de Swing (`KEY_PRESSED`, con una tabla de los
  `VK_` a los códigos HID, y su `KEY_TYPED`), con los lados de los modificadores por su `getKeyLocation`. Una tecla que
  da carácter espera a su texto, y si no llega, sale sin él en la siguiente pulsación o al soltarla; con Ctrl, Alt o
  Windows (no AltGr), sale en seguida. El AltGr de Windows trae un Ctrl de más, que se quita.
- **El módulo `Keyboards`:** el enum `Key` (los códigos HID), `getCode`, `isShift`, `isCtrl`, `isAlt`, `isAltGr`,
  `isWin`, `isFn`, `isCapsLock`, `isKey`, y la clase `Keyboard` (`onKey`, `hasKey`, `readKey`, `getChar`,
  `isConnected` y `close`). En el pack de la stdlib, no en las imágenes.
- **La prueba de la red**, sin ventana: un programa que mete teclas con `__kbInject`, con dos teclados (el evento a
  los dos; `readKey` se la lleva el primero), y espera sus eventos con un bucle de `sleep` (se reparte uno por turno).
- **Por mirar al programar:** que el reparto no cambie lo que ya hacen las dos VMs cuando no hay teclados (la red); y
  la pila de un `__despacha` en un hilo pequeño.

### El paso 2, hecho (9-nov): el teclado del SO y `Keyboards.Keyboard`, en las dos VMs

Lo del plan, y como salió:
- **`bpvm_teclado.c` y `Teclado.java`**, con el reparto en el planificador de las dos VMs (antes de sacar el evento del
  turno) y la limpieza al acabar el programa. La ranura de `__despacha`, por su nombre: la búsqueda de `__guiSlotOf`
  (la de MOD9 y la de los marcadores de antes) sale de la GUI a una función común (`slot_por_nombre`), porque el
  teclado también va en las placas sin pantalla; en la VM-Java ya lo estaba (`slotOfMarker`).
- **El módulo `Keyboards`** (en el pack de la stdlib, el 34): el enum `Key`, las funciones de los bits e `isKey`, y la
  clase `Keyboard`. Dos detalles del lenguaje: la tecla Fin se llama `END_KEY`, porque las palabras reservadas no
  distinguen mayúsculas y `END` es `end`; y los alias de `__kbOpen` y `__kbClose` llevan `Object`, porque `any` no sale
  en la interfaz de un módulo que no sea `Core`.
- **Los modificadores solos no son teclas** (pulsar Shift no avisa): un teclado HID los manda como bits, no como
  teclas, y así el PC hace lo mismo que la placa.
- **En el PC, las teclas de la ventana**: en la VM-C y el micro simulado, SDL (su `scancode` ya es el código HID);
  en la VM-Java, Swing (`TeclasSwing`: la tabla de los `VK_`, el lado de cada modificador y el teclado numérico como lo
  manda un teclado HID). Sólo con un teclado conectado (`bpvm_bt_hay_teclado`).
- **La memoria:** en una placa sin teclado, 12 B de RAM fija (medido en la Pico) y 1,5 KB de flash; el estado
  (~0,7 KB), la primera vez que se usa.
- **Las pruebas:** `Teclado.bp` en la red, sin ventana (con `__kbInject`): las dos VMs dicen lo mismo. Y a mano, el
  camino de las ventanas, con «Hola 1» y Enter, con y sin «Teclado simulado»: en la VM-C, con dos órdenes nuevas del
  guion de la ventana SDL (`teclea <texto>` y `tecla <código>`, de `BPVM_GUI_SCRIPT`); en la VM-Java, un arnés que le
  da el foco a la ventana de Swing y le mete los eventos en su cola. Las dos dan lo mismo: los siete códigos HID, sus
  caracteres y el Shift de la H; sin el teclado conectado, ninguna.

```basic
function event tecla(t: Keyboards.Keyboard, k: integer, c: string)
  if Keyboards.isKey(k, Keyboards.Key.ENTER) then
    print "Enter"
  elseif Keyboards.isCtrl(k) and Keyboards.isKey(k, Keyboards.Key.C) then
    print "Ctrl+C"
  elseif c != "" then
    print "tecla: " + c
  endif
end tecla
```

### El paso 3: el ratón, `Mice.Mouse` (9-nov)

**Lo que ve el programa** (lo propuse el 9-nov, y Eduardo, a los cinco puntos: *«OK»*):

```basic
import Mice
var ra: Mice.Mouse := Mice.Mouse()
ra.onPress := pant::pulsado
ra.onMove := pant::movido
ra.onWheel := pant::rueda

function event pulsado(r: Mice.Mouse, b: Mice.Button, x: integer, y: integer)
  if b == Mice.Button.LEFT then
    print "clic en " + x + ", " + y
  endif
end pulsado

function event movido(r: Mice.Mouse, x: integer, y: integer)
  if r.isPressed(Mice.Button.LEFT) then
    print "arrastrando por " + x + ", " + y
  endif
end movido

function event rueda(r: Mice.Mouse, d: integer)   // +1 hacia arriba, -1 hacia abajo, por cada muesca
```

Preguntando, desde cualquier hilo, el ratón del SO tal como está: `getX()`, `getY()` e `isPressed(b)`; y además
`isConnected()` y `close()`.

1. **Tres eventos**: al apretar un botón (`onPress`), al moverse (`onMove`) y la rueda (`onWheel`). Al soltar, nada,
   como en el teclado: más adelante, con el tiempo de las teclas. Para arrastrar, `onMove` con `isPressed`.
2. **`onPress`, y no el `onClick` del ejemplo del 8-nov**: el `onClick` de un `Gui.Button` salta al soltar.
3. **Los botones, el enum `Mice.Button`**: `LEFT`, `RIGHT`, `MIDDLE`, `BACK` y `FORWARD`, con los valores de los bits del
   informe de un ratón HID (1, 2, 4, 8 y 16). El botón va solo, así que se compara directamente
   (`b == Mice.Button.LEFT`); la tecla no, porque lleva pegados los modificadores.
4. **Al moverse, sólo la última posición**: un ratón Bluetooth manda unos 100 informes por segundo. Si entre dos turnos
   pasa por (10,10), (12,11) y (15,13), el programa recibe un solo `onMove`, con (15,13). Los botones y la rueda no se
   pierden: van en orden, como las teclas.
5. **La rueda horizontal, de momento no.**

**Lo que ya estaba decidido** (8-nov): el ratón va con una pantalla (sin pantalla se conecta, pero no hace nada); uno del
SO, que junta todos los ratones conectados, y los del programa; en el PC, el de la ventana de la GUI, sólo con «Ratón
simulado» conectado, y sigue haciendo de táctil en la GUI. Sin GUI, en el PC no hay ratón.

**El plan para programarlo** (lo interno es mío):
- **El ratón del SO, en la HAL BP** (`bpvm_raton.c`; en la VM-Java, `Raton.java`), con su cerrojo: la posición y los
  botones tal como están (lo que dan `getX`, `getY` e `isPressed`) y los avisos de los botones y de la rueda que faltan
  por repartir (16), cada uno con el punto donde pasó. Moverse no deja aviso: cada ratón del programa recuerda la última
  posición que se le avisó, y si la del SO es otra, le toca un `onMove`.
- **Los ratones del programa** (8 a la vez), como los teclados: `__msOpen(this)` apunta la referencia entera, el hilo que
  lo crea y la ranura de su `__despacha`.
- **El reparto**, después del de las teclas y con sus reglas (uno en la cola por ratón; con la cola llena, en el turno
  siguiente; quien se queda atrás pierde los avisos más viejos). Con un aviso pendiente, si el ratón del programa no
  está en su punto, primero un `onMove` hasta allí (así el programa ve llegar el ratón adonde se aprieta), y después el
  aviso; sin avisos, un `onMove` a la posición del SO, si ha cambiado. `__despacha(tipo, dato, x, y)` hace el `raise`.
- **Los builtins, del 332 al 338**: `__msOpen`, `__msClose`, `__msGetX`, `__msGetY`, `__msGetButtons`,
  `__msInject(x, y, botones, rueda)` (para las pruebas: como un informe, con el punto en vez del desplazamiento) y
  `__msIsConnected`.
- **En el PC**: en la VM-C y el micro simulado, los eventos de ratón de SDL (moverse, los botones y la rueda), con el punto
  pasado a la pantalla tal como se ve: la ventana no gira con la pantalla, y se hace la cuenta que hace LVGL con la
  táctil. En la VM-Java, un vigilante de los eventos de ratón de AWT sobre la ventana de Swing. Arrastrando fuera de la
  ventana, el punto se queda en el borde. El guion de la ventana SDL, con `mueve`, `arrastra` y `rueda`.
- **El módulo `Mice`**, en el pack de la stdlib, no en las imágenes.
- **La prueba de la red**, `Raton.bp`, sin ventana y con `__msInject`: los eventos, su orden, el punto de un clic aunque
  el ratón ya se haya ido, la rueda, dos ratones y `close()`. Lo que depende del reparto (cuántos `onMove` llegan) no
  se imprime: sólo que llega la última posición, y en orden.
- **Un límite, a la vista**: `isPressed` dice cómo está el botón ahora. Si el programa va lento, el `onMove` del final
  de un arrastre puede llegarle con el botón ya suelto. Lo resuelve el evento al soltar, cuando llegue.
- **Para la placa (el paso 5)**: el ratón manda desplazamientos, que se suman y se limitan a la pantalla; y la posición
  de partida, el centro.

### El paso 3, hecho (9-nov): el ratón del SO y `Mice.Mouse`, en las dos VMs

Lo del plan, y como salió:
- **`bpvm_raton.c` y `Raton.java`**, con el reparto en el planificador de las dos VMs, después del de las teclas, y la
  limpieza al acabar el programa: fuera sus ratones y los avisos; la posición y los botones se quedan, porque son del
  ratón.
- **El módulo `Mice`** (en el pack de la stdlib, el 35): el enum `Button` y la clase `Mouse`. Del bit del informe al
  enum, en BP y con un `if`: un entero no se convierte en un enum.
- **En el PC**: en la VM-C y el micro simulado, los eventos de ratón de SDL, con el punto pasado a la pantalla tal como se
  ve (girada, con la cuenta de LVGL); en la VM-Java, un vigía de los eventos de ratón de AWT sobre la ventana
  (`RatonSwing`). Sólo con un ratón conectado (`bpvm_bt_hay_raton`). Lo que SDL fabrica de una pantalla táctil no cuenta
  como ratón.
- **La memoria:** en una placa sin ratón, 12 B de RAM fija (medido en la Pico) y 1,2 KB de flash; el estado (528 B), la
  primera vez que se usa.
- **Las pruebas:** `Raton.bp` en la red, sin ventana (con `__msInject`): las dos VMs dicen lo mismo. Y a mano, el camino
  de las ventanas, con un guion (moverse, apretar, arrastrar, soltar, el botón derecho y la rueda), con y sin «Ratón
  simulado» y con la pantalla girada: en la VM-C, con cuatro órdenes nuevas del guion de la ventana SDL (`mueve`, `pulsa`,
  `suelta` y `rueda`); en la VM-Java, un arnés que mete los eventos en la cola de AWT. Las dos dan lo mismo, y sin el
  ratón conectado, nada. Girada, la VM-C da los puntos tal como se ven; la VM-Java no gira el dibujo, cambia el aspecto
  de la ventana, así que sus puntos ya son los de la pantalla.

### El paso 4: la GUI (10-nov)

**Lo que tiene que pasar:** en un programa con GUI, sin escribir nada, el teclado escribe en el campo con el foco, Tab pasa
al control siguiente y Enter pulsa el botón; y el ratón mueve una flecha y hace clic.

**Lo decidido** (lo propuse el 10-nov, y Eduardo, a los cinco puntos: *«OK, adelante.»*):
1. **LVGL lee el teclado y el ratón del SO él mismo, en el hilo de la GUI, como hoy la táctil**, sin un
   `Keyboards.Keyboard` ni un `Mice.Mouse` de BP en medio. El 8-nov se dijo que la GUI crearía los suyos y le pasaría sus
   eventos a LVGL; con el ratón no puede ser: un clic es apretar y soltar, LVGL necesita saber cuándo se suelta, y el ratón
   del programa no avisa al soltar. LVGL pregunta, como a la táctil: dónde está y si está apretado. Lo que se ve es lo
   acordado (todo, en el hilo de la GUI; los programas no cambian), y una tecla no gasta sitio en la cola de eventos.
2. **`scr.onKey(s, k, c)`, para las teclas tal cual**: todas, también las que usa la GUI (Tab, Enter, lo que se escribe en
   un campo), en el hilo de la GUI, como un clic, con el int32 y el carácter de `Keyboards`.
   ```basic
   var scr: Gui.Screen := Gui.Screen()
   scr.onKey := pant::tecla

   function event tecla(s: Gui.Screen, k: integer, c: string)
     if Keyboards.isKey(k, Keyboards.Key.F1) then
       ayuda()
     endif
   end tecla
   ```
3. **En el PC, con «Ratón simulado» conectado, el ratón del PC deja de hacer de táctil y hace de ratón**: en la ventana, la
   flecha de la GUI (la de la placa) y no la del sistema. Si no, cada clic contaría dos veces: el mismo ratón movería dos
   punteros. Sin él, táctil, como hoy. En la placa son dos aparatos y van los dos. Cambia, sólo en el PC, lo del 8-nov
   («el ratón del PC sigue haciendo de táctil, siempre»).
4. **En la VM-Java, sin «Teclado simulado», la ventana de Swing no acepta el teclado del PC**, como la placa y la VM-C. Con
   él, como hoy, y además Enter pulsa el botón con el foco, como en LVGL (en Swing, de serie, es la barra espaciadora).
5. **La rueda desplaza lo que haya bajo la flecha** (una lista, un panel que no cabe), en LVGL; Swing ya lo hace.

**Las teclas en LVGL:** Tab y Mayús+Tab, el control siguiente y el anterior; Enter pulsa el botón con el foco; en un
campo, las letras, Retroceso, Supr, las flechas, Inicio y Fin; Esc; en un deslizador, las flechas cambian el valor. Un
clic en un campo también le da el foco.

**El plan para programarlo** (lo interno es mío):
- **Sin tocar `bpvm_gui.h`**: el pack de la GUI de las placas se enlaza por nombre con sus funciones, y una que le
  falte deja la GUI sin funcionar hasta rehacer el pack (`gui_pack.c`, «al pack 'LVGL' le falta…»). Lo nuevo va en el
  teclado y el ratón del SO y en un builtin propio.
- **Los lectores de C** del teclado y del ratón del SO: quien lee sin un objeto de BP (LVGL), cada uno con su número del
  siguiente aviso que le toca, como los objetos apuntados. El de la pantalla, aparte: la pantalla de `scr.onKey` la
  apunta `Gui.Screen()` (y `load()`) con un builtin nuevo, el 339 (`__guiKeyTarget`): la última creada o cargada; y
  `__guiRunOnce`, en el hilo de la GUI, le entrega cada tecla con `Gui.__guiDispatchKey`, como un clic. Al acabar el
  programa, fuera la pantalla; los lectores siguen (en el micro simulado, LVGL vive de un RUN al siguiente) y empiezan de
  cero.
- **LVGL, en el PC** (la VM-C y el micro simulado; `gui_entrada.c`): un grupo por defecto (cada control que se cree entra
  solo) con un teclado de LVGL que lee su lector (las teclas HID a las de LVGL, y los caracteres); un segundo puntero
  para el ratón, con la flecha (`lv_indev_set_cursor`), que no pierde un clic corto (un botón apretado entre dos lecturas
  cuenta una vez); y la rueda, al objeto que se desplaza bajo la flecha. Con «Ratón simulado» conectado, la táctil (el
  ratón de SDL) se apaga y el cursor del sistema se esconde en la ventana.
- **Swing**: sin teclado conectado, las teclas de la ventana no llegan a sus controles; con él, Enter en un botón (y en
  una casilla) lo pulsa, y Tab sale de un campo de texto, como en LVGL.
- **La prueba de la red**, sin ventana: `scr.onKey` con `Keyboards.__kbInject`, en las dos VMs. Lo de LVGL (el foco, lo
  que se escribe, la flecha, el clic una sola vez, la rueda), a mano, con el guion de la ventana SDL; y lo de Swing, con
  un arnés.
- **Para la placa (el paso 5)**: lo mismo, por el pack de la GUI, con una API de entrada publicada por la imagen (la
  `ENTR` del 14-oct).

### El paso 4, hecho (10-nov): la GUI con el teclado y el ratón, en el PC

Lo del plan, y como salió:
- **Los lectores de C** (`bpvm_teclado_lector_*`, `bpvm_raton_lector_*`) y **la pantalla de `scr.onKey`**
  (`bpvm_teclado_pantalla`; en la VM-Java, `Teclado.pantalla`), con el builtin 339. El despachador,
  `Gui.__guiDispatchKey`, se busca por nombre sólo cuando hay una tecla: no va en `bpvm_t`, porque un campo nuevo movería
  lo que leen los `.mdn`.
- **LVGL** (`gui_entrada.c`, en la ventana de SDL de la VM-C y del micro simulado): el grupo del foco se crea cuando hay
  un teclado conectado, con los controles que ya haya. Si existiera siempre, LVGL le daría el foco al primer control al
  crearlo, y un campo de texto enseñaría su cursor desde el arranque sin teclado. El ratón, un segundo puntero con su
  flecha, que no pierde un clic corto; y la rueda, al objeto que se desplaza bajo la flecha.
- **Dos cosas de LVGL que costaron:** los manejadores de SDL de LVGL cogen el primer dispositivo de cada tipo de su
  lista, que empieza por el más nuevo, y dan por hecho que es el suyo: con nuestro teclado el más nuevo, el de las teclas
  reventaba (medido con `gdb`). Así que los nuestros se crean antes que los de SDL, con un teclado de SDL apagado, y se
  comprueba (si no, lo dice). Y la resolución «física» de LVGL 9.2 también se da girada: para deshacer el giro del puntero
  hay que tomar la del panel sin girar (con 90°, los clics caían fuera del botón).
- **La captura PNG de la ventana**, con las capas de encima: dibujaba la pantalla, no la flecha (ni un `Msgbox`).
- **Swing:** además de lo previsto, el foco, como en LVGL: en el orden en que se crearon los controles (Swing lo da por la
  posición: el campo, la lista de su misma fila y después los botones), y al abrir la ventana, en el primero.
- **Las pruebas:** `GuiTeclas.bp` en la red, sin ventana: `scr.onKey`, en las dos VMs, lo mismo. Y a mano, el guion
  «Hola», Tab, Enter, Tab, Enter en las dos ventanas: con «Teclado simulado», «Hola» en el campo, un clic en A y uno en B,
  y las 8 teclas en `scr.onKey`; sin él, nada. El del ratón, en la de la VM-C: con «Ratón simulado», un clic por botón (no
  dos), la flecha y la lista desplazada con la rueda; sin él, los clics como táctil. Y con la pantalla girada, los clics en
  su botón, cinco de cinco.
- **Salió `#813`**: la ventana de la VM-C girada se dibuja mal (lo vio Eduardo), desde V7. No es del paso 4.

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

**Los genéricos de lo que ya hay** (31-oct, V8/H7.2, `#749`; Eduardo: *«nuestro List es en realidad List<Object>»*):
`List<Object>` y `Collections.Map<integer, Object>` o `<string, Object>`, en JC y en BP. Los argumentos no llegan al
bytecode: son la `List` y el `Map` de siempre, con la llave del `Map` comprobada al compilar y su fábrica casada con
ella (`numericMap()` para la integer, `Map()` para la string; en JC, `new Collections.Map<int, Object>()` es el
`numericMap()`). Con ellos, el `foreach` sobre una `List` (el elemento, un `Object`), y `#748`: una cadena guardada en
un `Object` se imprime como texto. Otros argumentos (`List<Persona>`), más adelante (Eduardo: *«de momento no, más
adelante»*).

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
     volvía arriba sin mirarla. Era un fallo de BP, `#699`, arreglado el 24-oct: el `do … while` de JC es ya el de C#.
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
      **Hechos el 31-oct, en JC y en BP** (V8/H7.1, `#747`; Eduardo: *«los compuestos me valen»*). En BP: `*=`, `/=`,
      `mod=`, `&=`, `|=`, `xor=`, `shl=` y `shr=`. Con la regla de C# (*«Los tipos como en C#»*): `x op= e` opera
      como `x op e`, y si el resultado no se guarda solo en el tipo de `x`, vale sólo si `e` se guardaría sola en `x`
      (y entonces se recorta al tamaño de `x`, dando la vuelta: un byte que vale 255, más 1, es 0). `x` se calcula una
      vez (`#746`).
    - Como en BP, la asignación es una sentencia: no va dentro de una expresión (`a = b = 0`, `if ((x = f()) > 0)`).
    - **El ternario, `c ? a : b`** (31-oct, V8/H7.1, `#747`): el `iif(c, a, b)` de BP (Eduardo: *«El ternario BP como
      una función inline»*), con la precedencia de C# (debajo de `||`, asociativo por la derecha). Sólo se calcula la
      rama que toca. Su tipo, el de las dos ramas: el mismo; o números, y gana el más ancho; o `null` y una referencia;
      o una clase y su hija, y gana el padre. Un `var owner` local no puede ser una rama, y no es una sentencia.
      `??` y `?.`, todavía no: el error lo dice.
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
    - **Y los arrays con su tamaño** (H8, 5-nov): `new A[n]`, el `A[n]` de BP, con la misma idea: el árbol es el de indexar y lo
      distingue el análisis (§E9, «El tema 3»).
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
    - **`(T) x` es siempre una conversión** (`#797`, 5-nov; Eduardo: *«en JC los constructores van con new Caja(x)»*).
      La regla de BP, «si un constructor acepta `x`, gana el constructor», se debe a que `Caja(x)` dice las dos cosas;
      en JC hay dos formas, y no hace falta. Lo que ya es un `Caja`, o una hija suya, se queda como está, como en Java.
      Lo que no es de la familia sigue siendo un error. BP no tiene cómo escribir esa conversión: el traductor de JC a
      BP lo dice (`EscritorBp.sinFormaEnBp`), porque el árbol es el mismo y saldría un `Caja(x)` que construye.
24. **La comprobación de tipo, `is`, como en C#** (Eduardo, 17-oct): `a is Perro` es el `a instanceof Perro` de BP
    (`InstanceOfExpr`), también con el nombre cualificado (`a is Mod.Perro`). Va con las comparaciones, en BP y en
    C#. Lo demás del `is` de C# (`a is Perro p`, `x is null`) BP no lo tiene: error, de momento; para `null`,
    `x == null`.
25. **Los `enum`, como en C#** (Eduardo, 17-oct): `public enum Color { Rojo, Verde = 5, Azul }`, con una coma al
    final si se quiere. Es el `enum … end` de BP, miembro a miembro (`EnumDef`).
    - El valor, como en BP, un entero escrito y sin signo (`Parser.parseEnumValue`). Las expresiones que admite C#
      (`-1`, `1 << 3`, `Rojo + 1`), de momento no, y el tipo de debajo (`enum Color : byte`), tampoco.
    - JC es BP: un `enum` no se convierte a entero ni al revés (`#522`), aunque en C# sí: `(int) Color.Rojo` da
      error. **Desde el 31-oct, a entero sí** (`#754`; Eduardo: *«la conversión de Enum a entero ha de ser válida
      siempre que se haga con la sintaxis correcta»*): `(int) Color.Rojo` y `(int) c`, como en C#, y `integer(c)` en
      BP. De un entero a un enum, sigue sin valer.
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
| `#699`, el `continue` de un `do … loop`: un fallo; **hecho el 24-oct** | 9 | desde entonces, el de C#: el `continue` de su `do … while` va a la condición |
| `#700`, el `print` sin salto: **éste sí, en el compilador** (Eduardo); hecho el 18-oct | 17 | el `print(a, b);` es el `print a; b;` de BP |
| los caracteres, `'a'` | 19 | los lee el lexer de JC; en BP se escriben como su número |
| las secuencias de escape de C# | 19 | las lee el lexer de JC; lo que BP no sabe escribir, el traductor lo dice |
| que un literal guarde cómo se escribió | 19 | `0xFF` y `'a'` vuelven como `255` y `97` |
| el `foreach` sobre listas | 16 | sólo arrays; **hecho el 31-oct** en JC y en BP, sobre una `List` (V8/H7.2, `#749`) |
| los demás compuestos (`*=`, `/=`…) | 15 | no estaban; **hechos el 31-oct** en JC y en BP (V8/H7.1, `#747`) |

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
- **Un `if`, `while` o `for` de una línea al final de un bloque de su clase** (`#701`, un fallo del parser de BP,
  arreglado el 24-oct): BP no lo leía, porque tomaba el `endif`, `endwh` o `next` de la línea siguiente por suyo. De
  JC a BP sale esa forma (`for (…) { for (…) x; }`), y desde `#701` BP la lee.

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

**Al escribir esa prueba salió un fallo de BP, `#702`:** un `raise` dentro del `set` de una propiedad no compilaba. El
análisis lo daba por bueno, y el emisor decía «Campo no encontrado en Animal: cambia$dest». Era de BP, no de JC;
arreglado el 24-oct, también para JC (el mismo emisor).

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
  dos VMs igual (**arreglado el 23-oct**: `tcpClose` empuja su 0 en las dos; `TcpCloseSuelto` en el corpus). Y además,
  para el H3: los números de conexión no son los mismos en las dos VMs, la VM-C admite 16 y miVM sin límite, y nadie
  cierra las conexiones al acabar el programa.

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

> **6-nov:** decididos el 3 y el 8, el fichero, el Entorno y qué cuenta como actividad: abajo, «Lo decidido el 6-nov». El 4, el 5 y
> el 7 (el módulo `Wifi`, el PC simulado y la IP desde BP) se quedan para cuando se llegue a lo de los programas.

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

   Luego el de los programas: `Net` sobre lwIP, con el cierre de las conexiones al acabar el programa, y los mismos
   números de conexión y el mismo tope en las dos VMs (lo que quedó de `#705`, arreglada el 23-oct).
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
- ~~qué cuenta como actividad para apagar a los 10 minutos~~: **decidido el 6-nov** (abajo); ~~dónde se dice que no se apague~~:
  **decidido**, en el fichero de configuración, y con alimentación externa no se apaga;
- ~~cerrar `#706` porque el apagado del todo lo evita, o arreglar la C6~~: **cerrada el 4-nov**, por el diseño.

### Lo decidido el 6-nov

Eduardo: *«1 al 7 OK»*, sobre la propuesta de ese día, y tres matices suyos, que mandan sobre ella: *«Encontrar lo puede hacer cuando en
el IDE le digamos que busque un dispositivo en la red local, si no se conecta con la IP que le indiquemos. La wifi se activa de forma
opcional cuando en el environment pongamos wifi=1 o en fichero de configuración. Luego desde BP con una librería la podemos activar y
desactivar a voluntad.»*

1. **El orden:** primero el IDE por WiFi; después, lo de los programas (el módulo `Wifi` y `Net` en placa).
2. **Dos ficheros**, por la separación del enlace y el TCP/IP. `/sys/wifi.json`: `ssid`, `password`, `country`, `autoStart` y `autoOff`.
   `/sys/net.json`: `hostname` para el DHCP, o `ip`, `mask`, `gateway` y `dns` para la IP fija. La clave va en claro: cualquier programa de la
   placa, o quien tenga el cable, puede leerla (*«la seguridad, de momento no»*).
3. **Encenderla al arrancar es opcional:** `wifi=1` en el Entorno, o `"autoStart": true` en el fichero. Sin ninguna de las dos, apagada. `bt=1`,
   para el Bluetooth, más adelante.
4. **Un programa la enciende y la apaga a voluntad**, con la librería, aunque la haya encendido el arranque. Esto sustituye a lo que propuse en
   el punto 4 (que el programa no pudiera apagar la del sistema, con una excepción).
5. **La actividad:** hay actividad mientras haya una conexión TCP abierta, del programa o del IDE, y durante `autoOff` segundos desde que se
   cerró la última, o desde una búsqueda o una conexión. Por defecto, 600; con 0, no se apaga. La S3 y la P4 no saben si van a pilas, y manda el
   fichero. Con `wifi=1` y 600, una placa que se apaga sola sólo vuelve con un reinicio: para trabajar con el IDE, `"autoOff": 0`.
6. **La S3 pasa de 1 a 2 MB de aplicación.** Medido hoy, compilando sin grabar: la estación WiFi del IDF 6.1 ocupa 586 592 bytes más que su hola
   mundo, y nuestro firmware, 674 016: ~1,2 MB. En RAM interna, ~60 KB fijos más lo que pida al encenderse, sin medir, de los 166 KB libres. El
   Entorno y el sistema de ficheros se mueven, y se pierden una vez. La P4 primero, y la S3 en cuanto funcione: el código es el mismo.
7. **Cómo encuentra el IDE a la placa:** el `INFO` por el cable dice la IP, y el IDE la recuerda; con la IP que le indiquemos, o con «buscar en la
   red local», que pregunta el IDE y contestan las placas. La placa no se anuncia sola: no gasta nada mientras nadie pregunta.

### El algoritmo de los pasos 2 y 3 (6-nov; para su OK)

**Lo común y lo de cada placa**, como el sonido (`bpvm_sonido.c`). En la HAL BP, `src/bpvm_wifi.c`: los ficheros, los estados, los reintentos y el
apagado. Debajo, la parte de la familia, `esp32/common/wifi_esp32.c`: encender, apagar, conectarse, desconectarse y buscar, con `esp_wifi_*`. Esas funciones
son las mismas en las dos placas: en la S3 van a su radio, y en la P4 el IDF 6.1 las lleva a la C6. Lo único propio de la P4 es abrir y cerrar el enlace con la
C6 (`esp_hosted_init`, `esp_hosted_deinit` y su reset, el GPIO 54).

**A. El enlace del sistema.**
1. Un gancho nuevo de `bpvm_boot2`, `red`, tras el cable y antes del autorun.
2. Lee `wifi` del Entorno y `/sys/wifi.json`. Si el Entorno dice `wifi=0`, manda él. Si no toca encenderla, no se inicializa nada y no cuesta RAM. En
   la P4, la C6 se queda en reset desde el arranque: hoy nadie la sujeta (lo que gasta, sin medir).
3. Si toca, una tarea corta enciende y se conecta, sin hacer esperar al cable ni al autorun: en la P4, encender son 2,1 s.
4. Los estados: `off`, `starting`, `connecting`, `connected`, `noNetwork` y `badKey`. Si se pierde la red, reintenta a los 1, 2, 4… hasta 60 s. Con la clave
   mala, a los tres intentos se para y lo dice en el log: seguir no arregla nada y gasta.
5. Apagar del todo: `esp_wifi_stop` y `esp_wifi_deinit`, y en la P4 además cerrar el enlace y dejar la C6 en reset. Un fallo del enlace con la C6 no tumba la
   placa (`TRANSPORT_RESTART_ON_FAILURE=n`): se queda en `off` y lo dice el log.
6. El apagado por inactividad: una mirada cada 10 s con la regla de arriba.
7. El `INFO` dice `wifi`, `ssid`, `rssi` e `ip`.

**B. La IP.** Con el enlace arriba, `/sys/net.json`: sin fichero, DHCP con el nombre de la placa; con `ip`, la fija. Lo que hay que decidir de la IP desde BP, al llegar a `Net`.

**C. El cable por TCP, a la vez que el de siempre.**
1. `wire_v1_tcp.c` pasa al común de las ESP, sin el `accept` dentro de la lectura, y `esp32/common/wire_cables.c` pone las funciones `wire_v1_*` encima de los dos cables: el
   local (UART o USB-Serial-JTAG) y el de la red. El REPL no cambia.
2. **Habla uno a la vez: el último que ha mandado un mensaje válido.** Las respuestas y la salida del programa van a ése. Es lo que hace falta para que el IDE se enganche por WiFi a un
   programa que ya corre, o a un autorun, y lo pare. Sólo cambia entre mensajes, y un byte que no empieza un mensaje (`{`) no cuenta.
3. Sin nada que hacer, espera en el cable activo hasta 20 ms y mira el otro sin esperar: el activo no pierde tiempo. Durante un RUN, el sondeo mira los dos.
4. Un cliente nuevo sustituye al anterior, que con la WiFi suele ser uno muerto. Y para no quedarse colgado con uno muerto: `SO_KEEPALIVE`, y un envío atascado más de 5 s cierra el
   cliente. Al caer el cable de la red, el activo vuelve a ser el local.
5. El servidor abre el 3333 al tener IP y cierra al perderla. Una conexión abierta cuenta como actividad.

**D. El IDE.**
1. `wire_serie.py` por TCP antes que el IDE: la herramienta primero, y con ella se prueba todo lo de C.
2. En el IDE, «placa por la red», como la del puerto serie pero con `connectRemote`: la hora, el log y la memoria del `INFO`, que hoy no da el de la VM Java. Se escribe la IP, o se pulsa
   «buscar en la red».
3. Buscar: el IDE manda una difusión UDP al 3333, y cada placa con red contesta con su nombre, su micro, su IP y su puerto. Lo contesta el sondeo del cable, sin una tarea más.
4. Un diálogo para escribir `/sys/wifi.json` por el cable: la preconfiguración desde el IDE que se decidió el 19-oct.

**E. La S3:** su tabla de particiones y la misma imagen de código. **Después, lo de los programas:** el módulo `Wifi`, en Java, en C y en placa, con su PC simulado, y `Net` sobre lwIP.

**Cómo se comprueba, en cada paso y en placa:** A, el `INFO` con la IP, un `ping` desde el PC, la SD a la vez, la clave mala y el apagado con un `autoOff` corto. C, por `wire_serie`:
`INFO`, `LS`, subir y bajar ficheros, un `RUN` con su salida, `KILL`, los dos cables a la vez, y cortar la red a media sesión; y lo que tarda subir 1 MB. El ahorro de energía de la radio
(`esp_wifi_set_ps`) mete latencia: se mide ahí.

**Lo que queda por ver:** las SDIO de la C6 son de compilación: las de la P4 de Waveshare y las de la P4 Kit se comparten con `CONFIG_ESP_HOSTED_P4_DEV_BOARD_FUNC_BOARD`, sin probar en la Kit.

### La WiFi en un pack, y el punto C (6-nov)

Eduardo, sobre el algoritmo: *«OK pero el punto C de momento no, lo normal es que las placas no tengan conexión Ethernet. Un tema, ¿has pensado meter la wifi en un pack binario? Lo digo porque si
queremos tener una sóla imagen en la Pico2, hay una que tiene wifi pero la mayoría no lo tiene. En la P4 no todas las placas llevan un C6.»*

**El punto C no era Ethernet:** lo llamé «cable», que es como llamamos al canal del IDE. Es el IDE por WiFi, y se queda (*«1) si»*).

**El pack, medido** (lo que pide de fuera lo enlazado de verdad, por el `.map` y `nm`; la BIOS de los packs tiene hoy 47 ranuras):

| Placas | ¿Todas llevan radio? | Lo que pediría un pack | Decidido |
|---|---|---|---|
| S3, C3, C5, C6 | sí, en el chip | 367 funciones, 101 de ellas de la ROM o del enlazador; 46 KB que tienen que ir en RAM interna; binarios cerrados de Espressif (`net80211`, `pp`, `core`, `phy`) | en la imagen |
| P4 | no: depende de la C6 | 125 funciones (FreeRTOS, el driver SDMMC, libc…), todo fuentes. Ahorraría 252 KB de una partición de 6 MB y 5,4 KB de RAM | en la imagen |
| Pico 2 | no: sólo la W | sin medir (`cyw43-driver` y `lwip` están sin bajar). Sus dos firmwares van en huecos de 1 MB, y sin PSRAM: pagarían todas | un pack, opcional, al llegar ahí |

Eduardo: *«OK que en las ESP vaya integrado y en la Pico 2 sea opcional»*. **Para que no haya que tocar nada ese día, la radio es una tabla con nombre, `'WIFI'`, como `'PANT'` con las pantallas**: hoy la
publica la imagen, y mañana un pack; lo común la busca y no sabe quién la da (`bpvm_wifi.h`). Lo que queda para más adelante: en la C3 y la C6, la WiFi costará unos 60 KB de RAM fijos aunque no se use.

### Los pasos A y B, hechos en la P4 (6-nov)

**Lo común, probado en el PC antes que en la placa** (`make test-wifi`, 115 comprobaciones). `src/bpvm_wifi.c`: el enlace, con la máquina de estados aparte, sin hilos ni reloj, para probar los reintentos y el
apagado con un reloj de mentira; y una tarea que nace al primer encender. `src/bpvm_red.c`: la IP, separada del enlace, con el backend de `bpvm_net.h` (la misma capa que `Net`, que sigue sin sockets en las
placas). `esp32/common/wifi_esp32.c` publica `'WIFI'` y `net_esp32.c` es el backend; `esp32p4/main/radio_p4.c`, lo único de la P4: el enlace con la C6.

**Cambios sobre el algoritmo, por lo que salió al escribirlo:**
- **`radio=c6` en el Entorno.** No todas las P4 llevan C6, y en una que no la lleve el GPIO 54 y los de la SDIO son del usuario: sin esa línea no se toca ningún pin y la placa no tiene WiFi. Con ella, esos siete
  pines son del sistema, como los del sonido (`BPVM_PIN_SIS_RADIO`).
- **La «mirada cada 10 s» del apagado no hace falta:** la máquina sabe a qué hora toca apagar, y la tarea duerme hasta entonces.
- **A los tres rechazos de la clave, la radio se apaga del todo**, y el estado sigue siendo `badKey`. Y si la radio no contesta en 30 s, el intento se da por perdido.
- El búfer del `INFO`, de 1024 a 1280: la P4 ya ocupaba 911, y un campo que no cabe pierde el `INFO` entero.

**Medido en la P4** (con la red de casa; el 6-nov):

| | |
|---|---|
| Encender (el enlace con la C6, 1,93 s) | 2,17 s |
| Conectada, desde el arranque | 5,5 s; la IP por DHCP, a los 6,5 s |
| La clave mala (motivo 15) | tres intentos, y apagada a los 22 s |
| Una red que no existe (motivo 201) | reintentos a 1, 2, 4 y 8 s; con `autoOff` 40, apagada a los 40,5 s |
| `autoOff` 30 | apagada a los 30,0 s de conectarse |
| IP fija, `hostname`, y un `net.json` mal escrito (se dice, y va por DHCP) | bien; el `ping` contesta |
| `wifi=0` con `"autoStart": true` | apagada |
| `SdCard`, con la WiFi conectada | «los seis» en 2,3 s |
| `ping` desde el PC | 6 de 6, de 48 a 303 ms, media 179: el ahorro de energía de la radio |
| La imagen | de 950 000 a 1 251 328 B |
| RAM interna libre, en un RUN | 283 KB sin encenderla nunca; 230 KB conectada; 264 KB tras apagarse |

Los 19 KB que no vuelven son lwIP, el bucle de eventos y la tarea de la WiFi, que se quedan para la vez siguiente. La imagen creció 5 páginas: la zona de packs se movió y `Gui` y `SQLite` dicen
«realojado para OTRA dirección» hasta que se regraben.

### Los pasos C y D, hechos en la P4 (6-nov): el IDE por WiFi

**Lo común, probado en el PC antes que en la placa** (`make test-cables`, 48 comprobaciones, con dos cables de mentira). `src/wire_cables.c` pone las funciones `wire_v1_*` encima de los dos cables, y un cable
sólo mueve bytes: lee, escribe y, si quiere, mira lo suyo en un sondeo. La línea, el bulk, sus plazos y el cerrojo, que estaban copiados en cada cable, quedan en un sitio. `esp32/common/wire_v1.c` (la UART) se queda con 40 líneas;
`esp32/common/wire_red.c` es el cable de la red. La S3 y la P4 ya van por aquí; las de USB-Serial-JTAG (C3, C5, C6), cuando tengan WiFi. El `wire_v1_tcp.c` de la Ethernet de la P4 Kit (`BPVM_P4_WIRE=tcp`) se queda como estaba.

**Cambios sobre el algoritmo:**
- **Lo que no empieza por `{` se tira nada más llegar**, en cualquier cable: esperar a un `\n` por un byte de ruido haría esperar 3 s al otro.
- **Una espera de 20 ms sólo con un cliente de verdad en el otro cable.** Sin él, la de siempre (100 ms), y el sondeo mira si hay cliente nuevo.
- **Al cambiar de cable, o al irse el cliente, lo que se le mandaba al de antes se corta** (`bpvm_repl_cliente_ido`, lo que ya hace un `HELLO`).
- **El cable de la red junta lo que le llega y lo manda al acabar el mensaje:** con `TCP_NODELAY`, una línea y su `\n` serían dos paquetes.
- **El ahorro de energía de la radio no hace falta tocarlo:** en reposo, el `ping` tarda 179 ms de media, pero en una sesión la radio no llega a dormirse, y un `PING` del wire tarda 6 ms.

**`wire_serie.py` por la red:** donde va el puerto, la dirección de la placa; y `wire_serie.py buscar`. **En el IDE:** el backend «Placa (red)», con la IP o «Buscar en la red» (`RedBackend`, que hereda de `SerialBackend`: la hora, el log y
la memoria son los de la placa; y `BuscarPlacas`), y el botón «WiFi», que escribe `/sys/wifi.json` (`WifiDialog`). La IP que dice el `INFO` por el cable se recuerda. **Buscar pregunta varias veces:** por la WiFi una difusión no lleva
acuse, y se pierde (se perdió la primera, medido).

**Medido en la P4:**

| | Por la WiFi | Por la UART |
|---|---|---|
| `PING` del wire | 6 ms de mediana, 21 de máximo | |
| `LIST`, 65 ficheros | 355 ms | |
| Subir 1 MB | 16,1 s (64 KB/s) | ~95 s (11 KB/s, el límite de los 115 200 baudios) |
| Bajar 1 MB | 1,5 s (662 KB/s) | ~95 s |
| Grabar `Gui` + `SQLite` + `Stdlib` (1,5 MB, sellándolos) | 14 s | más de 2 min |
| Un programa imprimiendo | ~2 000 mensajes `OUTPUT` por segundo | |

Subir va más despacio que bajar porque es de 512 bytes en 512, con su acuse cada cuatro: lo que anuncia la placa, pensado para el anillo de la UART.

**Comprobado en la placa** (lo de cada línea, con su prueba): un programa lanzado por el cable, entrar por la red y que la salida cambie de cable, y pararlo con `KILL`; los dos cables por turnos, 10 de 10, cada uno con lo suyo; el cliente
se va con el programa en marcha y la salida vuelve al cable; un cliente nuevo sustituye al anterior; un cliente que no lee se da por perdido a los 13 s (se llenan los búferes y corren los 5 s), y el programa sigue; una conexión de 40 s aguanta la WiFi con
`autoOff` 20, y al cerrarla se apaga a los 20 s; `SqlDemo` y `SdCard` por la red; y con las clases del IDE, sin la ventana: buscar, conectar, `list`, `mem`, `log`, `put`, `get`, `run` y el Stop.

**Lo que falta por probar:** que se vaya la red de verdad con un cliente conectado (el `SO_KEEPALIVE`; se probó el cliente que se va, no el que desaparece), y la ventana del IDE, que sólo la ha visto el compilador.

### El paso E, la S3 (6-nov)

El mismo código que en la P4, sin el enlace con la C6: `wifi_esp32_publicar(NULL)`, y `CHIP_TIENE_WIFI` en su `chip_cfg.h` (la C3, la C5 y la C6 no lo definen, y su imagen no enlaza la radio). Su aplicación pasa a 2 MB: la
tabla de particiones nueva pide borrar la flash entera, y se pierden el Entorno, el FS y los packs de la placa, **una vez** (esto, a las notas de V8: una S3 que se actualice tiene que aprovisionarse otra vez). Sin NVS: la radio guarda su
configuración en RAM y se calibra al encenderse.

| | La S3 | |
|---|---|---|
| Encender | 96 ms | la radio está en el chip: no hay enlace que abrir |
| Conectada, desde el arranque | 0,38 s; la IP por DHCP, a los 1,75 s | |
| La imagen | 1 239 712 B | de 680 976: +559 KB |
| RAM fija | +50 KB de datos y +26 KB de código en IRAM | más que los 60 KB que se estimaron |
| `PING` del wire | 4 ms de mediana | |
| Subir 1 MB | 8,1 s (126 KB/s) | el doble que la P4: su FS va más deprisa |
| Bajar 1 MB | 1,6 s (660 KB/s) | |

**La RAM interna fue lo que se movió.** Con los búferes de la WiFi y de lwIP en la RAM interna, la libre mínima pasó de 166 KB a 58 al conectarse, y a **24 KB** tras subir y bajar ficheros: de ahí salen las pilas de las tareas. Con
`CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP` (los búferes, en la PSRAM) se queda en **57 KB** con la misma carga, y a la misma velocidad.

La batería del cable, la misma que en la P4 y en verde, incluida la salida al cambiar de cable sin perder ni repetir línea: por el cable llega hasta la N y por la red sigue la N+1.

### La C3 y la C6, sin PSRAM (6-nov; para decidir)

Eduardo: *«Esos consumos de RAM en la C3 y en la C6 nos vamos a quedar sin RAM.»* Tal como está, no les afecta: sus imágenes no llevan `CHIP_TIENE_WIFI` y no enlazan la radio. Pero la solución de la S3, los búferes en la PSRAM, no les sirve.

Medido compilando, sin grabar (`C:\tmp\v8wifi\medida_c3c6`): la RAM fija de la estación del IDF 6.1 sobre su hola mundo. Esa RAM se paga **aunque la WiFi no se encienda nunca**, por estar enlazada. «Con ahorro» es el código de la WiFi fuera de
la IRAM, menos búferes, sin WPA3, sin AMPDU (más lenta) y sin IPv6.

| | Libre hoy, en reposo | RAM fija | Con ahorro | Le quedaría con ahorro, apagada | Y flash |
|---|---|---|---|---|---|
| C3 | 108 KB | +54 KB | **+33 KB** | 75 KB | +505 KB: no cabe en su partición de 1 MB |
| C6, con la GUI | 80 KB | +88 KB | **+60 KB** | 20 KB: la GUI pide 45 en su peor momento | +588 KB |
| C6, sin la GUI | 216 KB | | | 156 KB | |

Encendida pide además lo suyo: en la S3, unos 30 KB al conectarse y otros 30 con tráfico, sin medir con los búferes recortados. O sea: **en la C6 con pantalla no cabe; en la C3 y en la C6 sin pantalla cabría, pagando siempre esa RAM**, aunque nadie use la WiFi. La
C5 tiene PSRAM y no tiene este problema: sólo su partición, como la S3.

**Lo que no se puede:** que lo pague sólo quien la use dentro de una imagen: la RAM fija es del enlazado, y un pack no vale (el censo de arriba). Lo que queda es otra imagen.

**Decidido** (Eduardo): *«C3 y C6 sin wifi. De las que tenemos quedaría la C5 que como tiene PSRAM no habría problema.»*

### Lo de los programas: el módulo `Air` y `Net` en las placas (6-nov)

Eduardo: *«Desde un programa de momento con encender y apagar es suficiente. La configuración la hacemos con un fichero con el mismo sistema para todas las placas. El resto ya es cuestión de abrir sockets y en teoría eso ya lo tenemos.»* Y
*«Haz un módulo inalambrico (el nombre que sea 'Air' por ejemplo). Dentro 2 clases, 1 para la Wifi y otra para el Bluetooth.»* Con eso se cierran las decisiones 4, 5 y 7 de arriba, y el esbozo de `Wifi.Radio` se queda en mucho menos.

```
import Air
import Net

var wifi: Air.Wifi := Air.Wifi()
if wifi.turnOn() then                 // enciende y espera a tener IP
  var t: Net.Tcp := Net.Tcp()
  ...
endif
wifi.turnOff()
```

- **`Air.Wifi`:** `turnOn()`, que espera a tener IP hasta 20 s, en BP y con `sleep`, sin parar los demás hilos; `turnOff()`; `isConnected()`, lo único añadido a encender y apagar, porque un programa largo necesita saber si ha perdido la red; e `isAvailable()`, como en `Sound`.
- **`Air.Bluetooth`:** la misma forma, sin firmware debajo: no está disponible hasta que se retome H2.
- **Al acabar el programa, la WiFi vuelve a lo que diga la configuración**, si la ha tocado: encendida con `wifi=1` o `"autoStart"`, apagada si no. Así un programa que la apaga no deja al IDE fuera, y uno que la enciende no la deja gastando.
- **En el PC, `turnOn()` da siempre `true`**, en las dos VMs: `wifi_host.c` es la radio más pequeña, y lo que corre encima es lo de la placa.
- **`Net` en las placas:** «en teoría ya lo tenemos» era verdad en el PC; en las placas no había backend. `esp32/common/net_esp32.c`: `connect` con DNS y plazo, `send`, `recv` con plazo y `close`, ocho conexiones a la vez. Una abierta cuenta como actividad, y lo que un programa deje abierto se cierra al acabar.

**Comprobado en la P4 y en la S3:** `AirWifi`, lo mismo que en el PC; un programa que pregunta al router y a `example.com` (el DNS, y `HTTP/1.1 200 OK`), en 120 ms, y un puerto cerrado da `false`; apagarla y encenderla desde un programa (6,4 s en la P4 y 1,3 en la S3); y la regla del final, en los dos sentidos. Salió un fallo mío: `isConnected()` daba `true` justo después de `turnOff()`,
porque la IP se da de baja unos milisegundos después que el estado; la IP sólo cuenta con el enlace arriba.

**La C5, preparada:** su partición a 2 MB, los búferes en la PSRAM y el cable USB como cable, igual que la UART (y con él, la C3 y la C6, sin WiFi). Su imagen, 1,46 MB. Se grabó esa misma tarde: lo siguiente.

### La C5 y la ventana del IDE (6-nov, por la tarde)

Eduardo: *«OK te he conectado la Pico2 con wifi y la C5. Cuando quieras miramos el IDE»*.

**La ventana, vista por Eduardo** en la P4, en la S3 y en la C5: «Placa (red)», «Buscar en la red» con las placas encendidas, Connect y el árbol de ficheros, Run con su salida (`AirNet`, hasta el `HTTP/1.1 200 OK`), parar un programa (`exit 130`, sin perder la conexión) y el diálogo del botón WiFi, que trae lo que ya tiene la placa, con la clave oculta. Bien a la primera, y sin un error en el IDE.

**La C5.** Borrada entera y grabada con la imagen de `be08fed7`. Se repusieron su Entorno (menos `part.fs.size`, que en esta placa no se aplica: el FS son los 16 MB de arriba, y `bpdata` se queda entera para los packs, ahora 14.240 KB), `Gui` y `DriverSpi`, sellados otra vez, `Stdlib` y sus cinco programas de `/app`. La copia de antes, en `C:\tmp\v8wifi\c5_antes`. **El cable USB nuevo** (`wire_v1_usbjtag.c` bajo `wire_cables.c`), que no había corrido en ninguna placa, bien a la primera; la C3 y la C6 lo llevan compilado, sin probar.

- Encendida en 0,7 s, enlace a los 1,3 y IP a los 2,5.
- Por la red: PING, 7 ms; subir 1 MB, 18,5 s; bajarlo, 5,1; 560 líneas de salida por segundo. Más lenta que la S3 (8 s y 1,6), con una señal floja, de -67 a -73 dBm; sin mirar si es por eso.
- Pasan todas las pruebas del cable de la red y de `Air`: entrar por la red con un programa en marcha, turnos entre los dos cables, un cliente que se va, uno que deja de leer, y la regla del final en los dos sentidos.
- La RAM interna: 144.923 B libres antes de encender, y un mínimo de 50.068 tras subir y bajar ficheros, con los búferes en la PSRAM.

**La pantalla, la SD y la WiFi a la vez.** `C5SdGui`, lanzado desde el IDE por la red: `RESULT: OK`, 121 ficheros escritos y releídos, ninguno distinto, y la etiqueta contando sin manchas (Eduardo). La pantalla sola, calcada a la del 24-sep: 188 cambios y 30 ms. Con la tarjeta, no: la etiqueta, de 176 cambios a 41, y el peor hueco, de 63 ms a unos 155; y la tarjeta, de 30 ficheros a 124. Partido cambiando una sola cosa cada vez:

| | cambios de la etiqueta | peor hueco | ficheros |
|---|---|---|---|
| WiFi encendida, tres pasadas | 41, 41, 41 | 156, 159, 155 ms | 124 |
| WiFi apagada, tres pasadas | 41, 41, 41 | 189, 154, 154 ms | 124 |
| `quantum=256` | 99 | 87 ms | 99 |
| `quantum=128` | 100 | 55 ms | 99 |
| en la flash, sin el bus de la pantalla, tres pasadas | 45, 46, 45 | 218, 234, 204 ms | de 136 a 139 |
| en la flash, con `quantum=128`, dos pasadas | 86, 126 | 177, 182 ms | 87, 125 |

**No es la WiFi, ni el bus que comparten la pantalla y la SD** (las dos en el SPI 2; Eduardo: *«En la C5 la SD y la pantalla comparten el bus SPI (creo)»*): **es el turno de los hilos, contado en opcodes** (`#801`).

### La Pico 2 W, aplazada (7-nov)

Eduardo: *«Nada, nos saltamos la wifi de la Pico 2. Nos quedamos con los de ESP32 de momento. Ya veremos mas adelante, en otras versiones.»* No se programó nada ni se bajó ninguna librería. Lo leído antes de decidirlo, para cuando se retome:

- **El esquema** (Raspberry Pi Pico 2 W, Rev 2; `files.waveshare.com/wiki/Raspberry%20Pi%20Pico%202%20W/Pico-2-w-schematic.pdf`): RP2350A, 4 MB de flash, sin PSRAM, y la CYW43439, con WiFi de 2,4 GHz y Bluetooth en el mismo chip.
- **Cuatro pines, los que en la Pico 2 hacen otra cosa:** GPIO 23, `WL_ON` (allí, el modo de la fuente); 24, `WL_D`, los datos en los dos sentidos y el aviso de la radio (allí, detectar el USB); 25, `WL_CS` (allí, el LED); y 29, `WL_CLK` (allí, el ADC 3, `VSYS`). El LED, el modo de la fuente y la detección del USB pasan a pines de la radio: en la W, el LED sólo va con la radio encendida.
- **Una imagen única:** propuse `radio=cyw43` en el Entorno, como `radio=c6` en la P4, con esos cuatro pines del sistema, y sin el LED del firmware.
- **El pack:** el bus va por PIO y DMA, con una interrupción, y la BIOS de los packs no da eso, ni tareas, ni semáforos. O crece la BIOS, o el transporte se queda en la imagen.
- **El coste:** el firmware de la radio, 225 KB (`CYW43_WIFI_FW_LEN` 224.190 y `CYW43_CLM_LEN` 984), que se manda al chip en cada encendido. Con el driver y lwIP, unos 330 KB de flash y unos 50 KB de RAM: estimado, sin medir.
- **La licencia de `cyw43-driver`:** `LICENSE`, sólo uso no comercial; `LICENSE.RP`, usarlo y redistribuirlo con chips de Raspberry Pi, conservando su aviso. Esa cubriría un pack para la RP2350.
- **La placa de Eduardo** lleva un firmware del 16-jul, sin packs. `cyw43-driver` y `lwip` siguen sin bajar en `pico-sdk`.

### Cuando la red se va (7-nov; `#803`)

Lo que quedaba de H3 era que se fuera la red con un cliente conectado. Apagar la WiFi del router no se puede (Eduardo: *«nos quedamos sin conexión»*), así que se parte en dos casos:

1. **La placa desaparece, y hay que ver qué hace el IDE.** Con `AirApaga` desde el IDE, que apaga la radio de golpe. El IDE se quedaba colgado: `#803`, arreglado, con un latido en el IDE y el `PING` contestado en marcha en la VM-C. Medido en la P4: lo dice a los 11 s, porque la WiFi ya ha vuelto y contesta que esa conexión no existe; con la red caída del todo, a los 15.
2. **El cliente desaparece, y hay que ver qué hace la placa.** Es el `SO_KEEPALIVE`. La S3 se conectó a la P4 como cliente, con `Net`, y apagó su radio sin cerrar: la P4 la dio por ida a los 25,6 s (10 + 3 × 5) y volvió a hablar por la UART. Reiniciarla no servía: a los 2 s vuelve con la misma IP y contesta que esa conexión no existe, que no es lo que se quería probar.

Lo único sin probar en placa, ni con estos dos casos, es que desaparezca el router de verdad y la placa se reconecte sola: eso lo cubre `make test-wifi`, en el PC.

### Cerrado (7-nov)

Eduardo: *«Cierra H3»*.

- **Hecho, en la P4, la S3 y la C5:** la placa se conecta sola con `/sys/wifi.json` y coge su IP, por DHCP o fija; el IDE por WiFi, con «Placa (red)», «Buscar en la red» y el botón WiFi; los dos cables a la vez; `Air.Wifi` para encender y apagar desde un programa; y `Net` para abrir sockets, con DNS.
- **Sin WiFi:** la C3 y la C6 (sin PSRAM) y las STM32. **La Pico 2 W, aplazada** (arriba).
- **Fuera, como estaba decidido:** el servidor para los programas (`listen`/`accept`), UDP, TLS, la web de configuración, la placa como punto de acceso y la hora por NTP. Y la lista de redes de un programa: Eduardo, 6-nov: *«Desde un programa de momento con encender y apagar es suficiente»*.
- **Para V9 o más adelante** (Eduardo): un módulo de radio externo, para ampliar la WiFi y el Bluetooth, y un micro STM32 con WiFi.
- **Queda suelta `#798`**, del log, y sin probar en placa que se caiga el router de verdad.

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

**27-oct: el módulo de BasicPlus, decidido** (Eduardo: *«Me gustaría retomar lo del sonido […] Lo que faltaría sería
integrarlo con la VM. Yo creo que la reproducción de sonidos debería ser siempre en paralelo al resto. Por ejemplo se
puede utilizar para dar un aviso pero el programa debería seguir. A parte de eso no creo que haga falta nada mas con
respecto al sonido.»*). Así queda:
- **El módulo `Sound`:** `play(ruta)`, `stop()`, `isPlaying()`, `setVolume(v)`/`getVolume()` e `isAvailable()`.
  ```
  Sound.play("/sd/aviso.wav")   // vuelve en seguida: suena mientras el programa sigue
  ```
- **Siempre en paralelo:** `play` abre el fichero, lee la cabecera del WAV y vuelve; lo toca una tarea propia, que lee
  el fichero a trozos (el sistema de ficheros ya tiene su cerrojo para varias tareas) y se lo da al códec. La VM no
  espera nunca. **Un sonido a la vez:** un `play` mientras suena otro corta el anterior.
- **Los errores:** lo que se sabe al llamar (no existe, no es un WAV, un formato que no se admite) es una excepción en
  el momento, como `readFile`; lo de después (la SD falla a media canción) para el sonido y queda en el log.
- **La HAL BP da el error de una placa sin sonido** (Eduardo: *«Si tenemos una interfaz HAL BP común, cuando se
  reproduce un sonido llama a la interfaz. Si la HAL del SO no lo soporta tendría que ser la HAL BP la que indicara el
  error.»*): lo común (el WAV, la tarea, las órdenes) en la HAL BP, y debajo, la parte de cada familia si la tiene; si
  no la hay, o el ENV no tiene la línea `audio`, o el códec no contesta, la HAL BP da el motivo, el mismo texto en
  todas las placas (como `bpvm_sd_placa_sin_sd` con la SD). `isAvailable()` le hace la misma pregunta sin tocar nada.
- **Los formatos:** WAV PCM de 16 bits (y de 8), mono o estéreo, a la frecuencia del fichero. El MP3, fuera.
- **El ahorro de energía:** el códec y el amplificador, encendidos sólo mientras suena. **El volumen:** de 0 a 100, 85
  por defecto.
- **En la P4:** en la imagen, no en un pack; la línea `audio` del ENV dice el códec y los pines; el I2C del códec es el
  del táctil, por el árbitro, como el táctil. `esp_codec_dev` 1.5.10 (sólo depende del IDF), declarada como los
  componentes de la pantalla: del registro, con la versión fija (Eduardo: *«si tienes permiso»*).
- **En el PC suena** (Eduardo: *«Que en el PC suene, si»*): la VM-C con el audio de Windows (winmm) y la VM-Java con
  `javax.sound`, que vienen con el sistema. Las pruebas de la red, siempre en silencio.
- **Por la cascada:** primero el PC (el módulo, las dos VMs, la tarea y las pruebas); después la P4 (el ES8311, la línea
  `audio`, grabarla y escucharlo).

**27-oct, el PC: HECHO** (Eduardo: *«OK, adelante»*):
- **El módulo `Sound`** (`bpstdlib/Sound.bp`, en el `Stdlib.pack`): `play`, `stop`, `isPlaying`, `setVolume`/`getVolume`,
  `isAvailable` y, a petición de Eduardo mientras se hacía (*«Haría falta una función para consultar el progreso»*),
  **`getPosition` y `getLength`**: bytes del sonido del último `play`, sin la cabecera, así que el tanto por ciento da
  justo de 0 a 100 (en `long`: un WAV de más de 21 MB por 100 no cabe en un `integer`). Los builtins 287–294, sin
  `public` en `Core` (su interfaz no cambia: `Core.mod` sale idéntico y la stdlib embebida de las imágenes, también).
- **Lo común, en la HAL BP** (`bpvm_sonido.c`): la cabecera del WAV (RIFF, `fmt `, `data`, los trozos que no son esos
  y el relleno de los impares), la tarea —creada al primer `play`: en una placa sin sonido no nace— que lee a trozos
  de unos 50 ms con `bpvm_fs_read_at` (por ruta y desplazamiento: no tiene el fichero abierto), la generación (uno a la
  vez), el progreso y el fin de RUN (para, el progreso a 0, el volumen a 85). Una placa sin la parte del sonido da el
  error con su motivo, el mismo texto en todas. En las seis listas de las placas (el STM32, solo).
- **El PC suena** (`sonido_host.c`): winmm en la VM-C, cuatro búferes en cola; `javax.sound` en la VM-Java
  (`Sonido.java`, el espejo: la misma cabecera, los mismos textos). Con `BPVM_SONIDO=mudo`, en silencio y al mismo
  ritmo (contra el reloj, no a base de dormir trozos): **la red corre así** (`compat.sh` lo exporta).
- **El volumen del PC, 20 dB por debajo del de la P4** (Eduardo: *«el volumen en el PC y en la P4 no se parecen. Si
  reproduces un sonido en el PC al 85% vas a dejar sordo a más de uno»*): la misma escala de 0 a 100 (de -50 a 0 dB),
  y la cintura del PC la desplaza. Elegido de oído, como el 85 de la P4: el canon a 85 con 40, 30 y 20 dB menos, dos
  vueltas, y Eduardo: *«la 3»*.
- **Un fallo mío, cazado al probar el camino de verdad:** `play`, `stop` y `setVolume` no dejaban el 0 que el
  compilador descarta tras una llamada (todos los builtins sin valor lo dejan), así que cada llamada descuadraba la
  pila 4 bytes: una variable local salía con basura (un 260 donde se puso un 9) y un contador no avanzaba. Arreglado en
  las dos VMs, y la prueba lo vigila ahora (con el 0 quitado de `stop`, «FALLO 8 … [364], se esperaba [1234]»).
- **La pila del despachador de builtins, medida** (`-fstack-usage` con la orden de la Pico): el `case` de `play` la
  subía de 2128 a 2232 B —lo pagaría cada builtin—; con un solo búfer para la ruta resuelta y, después, el texto del
  error, vuelve a 2128.
- **Las pruebas:** `SonidoParalelo` en el corpus (31 comprobaciones escritas a mano: vuelve en seguida, acaba solo con
  la posición igual a la longitud, `stop`, un `play` que corta al anterior, el progreso a medias, el volumen, un trozo
  LIST, un fichero más corto que su cabecera, los errores con sus textos y la pila); el camino de verdad del PC (winmm y
  `javax.sound`) a volumen 0, en las dos VMs; la Pico, la P4 y la Nucleo compilan con el fichero nuevo. La red, VERDE en 13:11: paridad 167 en MOD9, en MOD8 y con dos workers (ya con `SonidoParalelo`); cápsula V6, 59; norma 9/9; sim 12/12; pruebas de C, 107/107; sombra 15/15; el estrés, `packs-abi` y la stdlib embebida.
- **Lo siguiente, la P4:** su cintura (el ES8311 por I2S con `esp_codec_dev` 1.5.10 del registro, el I2C del táctil por
  el árbitro, el amplificador en el GPIO 53) y la línea `audio` del ENV; grabarla y escucharlo.

**27-oct (noche), la P4: HECHO** (Eduardo, tras oírla: *«Si se oye bien, mas fuerte que en el PC»*):
- **La línea `audio` del ENV, la lee la HAL BP** (`bpvm_sonido_linea_parse`, para cualquier placa con códec):
  `audio=es8311,i2c:0,mclk:13,bclk:12,ws:10,dout:9,pa:53` —el códec lo primero; etiquetas `clave:valor` en cualquier
  orden, como la `sd`; `dir` (0x18), `pa`, `paalto`, y `sda`/`scl` si la placa no ha declarado el bus—. Con ella, el
  tercer lector de líneas del ENV, los dos ayudantes que estaban duplicados en la `sd` de SPI y la de SDIO pasan a
  `bpvm_env_linea.h` (lo pedía su propio comentario: «si aparece un tercer parseo, ése es el momento»).
- **La cintura de la P4** (`esp32p4/main/sonido_p4.c`): el arranque del firmware de prueba del 19-oct. Al arrancar,
  tras la pantalla: lee la línea, se engancha al I2C del táctil como el firmware (por el árbitro, el bus del IDF que
  abrió la cintura), mira que el ES8311 conteste (0x18) y reserva sus pines, «del sonido» (`BPVM_PIN_SIS_SONIDO`: un
  programa no los puede pedir). El I2S y el códec, en el primer `play`; el códec y el amplificador, encendidos sólo
  mientras suena. Sin la línea, o si el códec no contesta, la HAL BP da el motivo. La imagen, +53 KB.
- **En la placa:** el log, «sonido: ES8311 en el I2C 0 (0x18); I2S mclk 13, bclk 12, ws 10, dout 9; amplificador en
  el GPIO 53»; el canon de la SD, en paralelo (`play` vuelve en 8 ms; el progreso, del 9 al 100 %, un 10 % por
  segundo) y el tono de aviso detrás; Eduardo lo oye bien. `SonidoParalelo` en la P4, IDÉNTICO al PC (31/31). La P4
  lleva ya el `Stdlib.pack` con `Sound`.
- **Las pruebas:** `make test-sonido` (35, en la red): la línea `audio`, y la tarea con una cintura que apunta cada
  muestra (las de 8 bits a 16 con signo, las de 16 tal cual, sin el medio marco del final) y cómo se cierra (drenando
  al acabar; en seco al parar o al cambiar de sonido); `test-sdio` y `test-sd`, verdes tras sacar los ayudantes; la
  Pico y la Nucleo compilan. La red, VERDE en 13:45: paridad 167 en MOD9, en MOD8 y con dos workers; cápsula V6, 59; norma 9/9; sim 12/12; pruebas de C, 108/108 (`test-sonido`, 35/35); sombra 15/15; el estrés, `packs-abi` y la stdlib embebida.
- **Para el próximo lote:** el `Stdlib.pack` con `Sound` en todas las placas (en las que no tienen sonido,
  `isAvailable()` da false y `play`, el motivo).
- **Y la C5:** la tercera parte del H4, parada hasta que haya un altavoz que entre en su conector (abajo, «27-oct,
  la C5»).

**27-oct, la C5: la tercera parte del H4, analizada y parada** (Eduardo: *«Me parece que C5 también soporta
sonido»*; y tras el análisis: *«Si, haremos el sonido de la C5 como parte de H4 (digamos la tercera parte que falta). Ahora mismo el conector del altavoz es diferente del de la P4 así que no me entra por lo que no lo podemos probar. Queda pendiente para mas adelante hasta que encuentre una solución.»*):
- **La placa** (la Waveshare ESP32-C5-Touch-LCD-2.8: su wiki, su ejemplo `07_audio` y su esquema): el mismo ES8311 y
  el mismo NS4150B que la P4, un conector de altavoz MX1.25 de dos pines y un micrófono. El ES8311 está en el I2C 0
  (los GPIO 0 y 1, en la 0x18), con el táctil, el expansor, el RTC, la IMU y el SHTC3: el árbitro ya lo cubre.
- **Lo que cambia respecto a la P4:**
  - **sin MCLK:** el códec saca el reloj del BCLK (`use_mclk = false` en el ejemplo de Waveshare); en la línea
    `audio`, sin `mclk`;
  - **el I2S:** el BCLK en el 24 y el WS en el 25; la salida al códec, el 26 o el 27 —la wiki pone `I2S_ASDOUT` en
    el 26 e `I2S_DSDIN` en el 27; el código de Waveshare, la salida en el 26 («MCU to codec DAC»); el texto del
    esquema no dice qué GPIO va a cada red—. Va en la línea del ENV: se decide en la placa, sin recompilar;
  - **el amplificador no está en un GPIO:** es el bit 3 (EXIO3) del expansor CH32V003 (I2C 0x24), el mismo chip que
    hace el reset del panel (EXIO1) y el del táctil (EXIO0), y la luz (su registro 0x05).
- **El problema, el expansor:** su salida (el registro 0x03) se escribe entera, los 8 bits, y no se puede leer (el
  0x04 es la entrada; los drivers de Waveshare y de ESPHome guardan una copia en memoria). Hoy la escribe sólo el
  driver de la pantalla (el pack `DriverC5`), con su copia. Con otra copia en el sonido se pisarían: al tocar un
  sonido, la pantalla y el táctil se quedarían en reset; al abrir la pantalla mientras suena, se apagaría el
  amplificador con `isPlaying()` diciendo que suena —un fallo silencioso—.
- **Lo propuesto** (a la pregunta *«¿Hacemos la C5 dentro del H4 antes de cerrarlo, con esta propuesta?»*, el «Si»
  de arriba): **el expansor, con un solo dueño, la HAL BP**, como el árbitro del bus. Guarda los 8 bits y ofrece
  «pon el bit N a 0 o a 1»; el sonido lo usa, y el pack de la pantalla también, en vez de escribir el expansor, por
  una entrada nueva al final de la BIOS (la versión 9: los packs hechos con la 8 siguen valiendo con la imagen nueva,
  y el `DriverC5` nuevo en una imagen vieja lo dice). La cintura de la P4 pasa a la familia (`esp32/common`, de
  donde la C5 toma su código), con el `mclk` opcional y el amplificador por un GPIO o por el expansor. En la C5:
  `audio=es8311,i2c:0,sda:0,scl:1,bclk:24,ws:25,dout:26,pa:exio3`. De paso, el EXIO6 y el EXIO7, que Waveshare
  deja libres en el conector, se podrían ofrecer sin pisar la pantalla.
- **El camino sin tocar la BIOS** —la imagen saca la pantalla y el táctil del reset al arrancar, y el pack deja de
  escribir el expansor— no se propuso: un pack nuevo en una imagen vieja dejaría la pantalla negra sin decir por qué.
- **Lo que cuesta:** un día, más o menos. La imagen de la C5 ocupa 702 KB de su partición de 1 MB (el 68 %), y el
  sonido le suma unos 50 KB.
- **Parada** hasta que Eduardo tenga un altavoz que entre en el conector de la C5.

---

## E5 · Los strings: el censo de sus operaciones, el invariante y la PSRAM (H5; Eduardo, 21-oct)

> **Es el H5 de V8** (Eduardo, 21-oct): *«A ver tenemos pendiente de V7 optimizar los strings. Esto lo vamos agrupar
> en H5. Por un lado la optimización de la operación de strings. Y por otro la optimización de strings que están en
> PSRAM.»* **Sólo análisis, por ahora** (*«De momento solo analizamos»*). Y la norma, ese mismo día: *«La norma debe
> ser: 1ª analizar, 2º mejorar algoritmo, 3º mejorar implementación.»* Del GC, en concreto: *«Lo del GC y la lista de
> huecos de momento solo analiza, de momento no intentamos mejorar.»*
>
> **El H5, CERRADO el 27-oct** (Eduardo: *«Si, cierra H5 y lo que falte como pendiente»*). Lo hecho, abajo y en la
> fila del H5 de `FICHAS.md`; lo que quedaba, en fichas sueltas: `#715`, `#718`, `#719`, `#721`, `#722`, `#723` y
> `#730`–`#739`.

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
       por él, `Json`, que convierte cada `\uXXXX` con un `appendChar`: un emoji escrito en JSON como `\uD83D\uDE00`
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
    tiene un carácter menos que caminando (`#711`). **Quitado el 2-nov (`#739`):** en su lugar, el aviso donde nace
    toda cadena;
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
  con la misma cadena en el heap (en el PC no se ve: el compilador vectoriza ese bucle). **(2-nov, `#732`: el cursor
  de un literal lo apunta.)**

**Con caracteres especiales, lo que queda como antes:**
- **dos cadenas con acentos a la vez, por índice, en un programa:** el cursor salta de una a otra. Sigue cuadrático,
  tres veces más deprisa (Pico 2: 4238 → 1461 ms). En la stdlib ya no queda ninguno: eran `Str.compare` y
  `Str.lastIndexOf`. **(2-nov, `#731`: dos cursores por hilo; en el PC, con 32 000 caracteres, de 492 ms a 3.)**;
- **los literales largos**, con o sin acentos: 881 ms frente a 44 en la Pico 2;
- **`upper` y `lower`:** el código de antes, carácter a carácter. Lineal, pero no más rápido;
- **las natives** (el código AOT): sólo la pasada única. Un bucle por índice en una native sigue siendo cuadrático.
  **(2-nov, `#733`: los cursores del hilo; en el PC, de 245 ms a 1.)**;
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
   o el hilo muere. Y `Json` convierte cada `\uXXXX` con un `appendChar` sin juntar los pares (`\uD83D\uDE00`, el
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

### `#711`: el plan, comprobado en el código (23-oct)

Eduardo, 23-oct: *«Seguimos con los pendientes, lo del conjunto son ideas. El siguiente es #711»*. Se mantiene lo escrito
arriba («`#711`, el descodificador»): la rutina por el estándar con el atajo ASCII, `EncodingError extends RuntimeError`,
`ignorarError: boolean := false`, un número nuevo para cada builtin con el indicador (desde el 284: el 283 es
`__strNace`), los de hoy con la regla nueva, `appendChar` y los tests. Al mirarlo en el código salieron seis cosas:

1. **Un parámetro nuevo en un MÉTODO rompe lo ya compilado.** La clave de un método lleva su firma (`MOD_FORMAT.md`
   §13.1, R4: `readLine()s`), y el valor por defecto lo pone quien llama (`checkArgs`). Con `readLine(ignorarError :=
   false)` la clave pasa a ser `readLine(z)s`, y un `.mod` ya compilado que llama a `readLine()` deja de enlazar: «'App'
   necesita 'IO.TextFile.readLine()s' … recompila» (E3; con nombre, no en silencio). Les pasa a `TextFile.readLine`,
   `TextFile.readStr` y `Tcp.recvStr`. A los builtins no: `fromBytes` y `readFile` son `inline` (cada llamada lleva su
   `CALL_BUILTIN`, no hay nada que enlazar), y el compilador rellena el valor por defecto también en una `inline`.
   **Propuesta:** en los tres métodos, dos declaraciones con el mismo nombre (una sobrecarga), `readLine()` y
   `readLine(ignorarError: boolean)`. Quien programa escribe lo mismo (`f.readLine()`, `f.readLine(true)`), y lo
   compilado sigue enlazando. En los builtins, el parámetro opcional, como se decidió.
2. **`recvStr` daría un error falso.** Descodifica cada paquete por su cuenta, y un carácter puede llegar partido entre
   dos («El stream», arriba). Hoy pasa en silencio (bytes rotos en la VM-C, «�» en la VM-Java); con la regla sería una
   excepción con un texto bueno. **Propuesta:** lo mínimo de la regla de Eduardo para los streams, sólo en `recvStr`:
   los bytes de un carácter a medias al final (tres como mucho) se quedan en la conexión para la llamada siguiente. Si
   vence el plazo, esperan; si el otro lado cierra, el error de siempre («Net.recv: conexión cerrada por el peer»). Lo
   demás de los streams, por diseñar, como estaba.
3. **`Json` se rompería con un emoji escrito con `\u`.** `"\uD83D\uDE00"` son un par de sustitutos, y hoy van en dos
   `appendChar` (en la VM-C salen bytes malos). Con la regla, `EncodingError` con un JSON bueno. Hay que juntarlos en
   `Json` (ya dicho en el grupo 4): el par es un carácter; uno suelto, un error de JSON con su posición, como los demás.
4. **Las natives:** el AOT crea con `string_from_cstr` los literales de una native y otros textos del compilador (de
   confianza). Validar ahí costaría en cada literal. El texto que un pack trae de fuera (el de SQLite) va aparte, en su
   pack.
5. **Las fuentes de confianza, con su test** (como se decidió): los nombres de `listDir`, el ENV, los argumentos del RUN,
   las rutas, la información de los packs y el texto de la GUI. En la GUI hay un fallo nuestro conocido (el búfer de
   1 KB puede cortar un carácter): se corta por caracteres. La prueba de cada una: `fromBytes(toBytes(s))` no lanza.
6. **`StringBuilder.toString`** acaba en `fromBytes` (no puede llamar a `__strNace`, `#714`): validará sus piezas, que
   ya son válidas. Con acentos, una pasada más, lineal.

Lo que cambia para un programa de hoy: leer con `readFile` un fichero que no es UTF-8 daba bytes rotos en la VM-C y
«�» en la VM-Java (su propio descodificador); ahora, `EncodingError` en las dos, o «�» con `true`. Lo ya compilado
llama a los números de antes, que también siguen la regla.

**Las piezas, por la cascada** (la VM-Java; la VM-C, con la paridad; las placas, en un lote):
- **`Core`:** `EncodingError`; `fromBytes` y `readFile` con el parámetro (284 y 285); `__freadLine` con el indicador
  (286).
- **las dos VMs:** la rutina; los seis builtins (120, 38 y 249 con la regla; 284–286 con el indicador); `__charsToString`
  (47) con algo que no es un carácter, `EncodingError`; la excepción buscada por su nombre, y `RuntimeError` si no está.
- **la stdlib:** `IO` (`readLine` y `readStr`, con su sobrecarga), `Net` (`recvStr`: sobrecarga y bytes pendientes) y
  `Json` (los pares).
- **las pruebas:** la rutina contra un oráculo escrito aparte (el algoritmo del estándar, tal cual), exhaustiva de 1 a
  3 bytes y al azar con 4; un sample del corpus con lo mal formado por cada entrada, con y sin el indicador; las fuentes
  de confianza.

Esperando el OK de Eduardo a las propuestas 1 y 2.

**`#711` HECHO (23-oct; Eduardo: *«OK, 1 OK»*).** Como estaba en el plan, con tres cosas más:
- **la rutina,** la misma en las dos VMs: `edu.bpgenvm.vm.Utf8` en miVM y `utf8_paso`, `utf8_primer_malo`,
  `utf8_largo_corregido` y `utf8_corrige` en `bpvm_internal.h` (junto a `utf8_encode`). El atajo ASCII primero; un trozo
  malo, un «�». Cada una contra un oráculo escrito de otra forma (la tabla 3-7 del estándar, fila a fila): todas las
  secuencias de 1 a 3 bytes (16,8 millones) y un millón al azar de 4 a 12 (`Utf8Test`, `make test-utf8`, menos de un
  segundo). Con un sabotaje en cada una (dejar pasar los sustitutos; los sobrelargos de tres bytes), la prueba falla;
- **`EncodingError extends RuntimeError`** en `Core`, buscada por su nombre en las dos VMs (`throwBpError`,
  `bpvm_throw_error_clase`), y `RuntimeError` si el `Core` no la tiene. El texto: `fromBytes: UTF-8 mal formado en el
  byte 1 (0xF1)`; con el byte, que dice qué era (un `0xF1` es una `ñ` de Latin-1);
- **los builtins:** los de un argumento (`fromBytes` 120, `readFile` 38, `__freadLine` 249) con la regla nueva, y la
  sobrecarga con el indicador (284, 285 y 286). Sobrecargas también en los builtins, no sólo en los métodos:
  `gen_builtins.py` nombra un builtin por su primera declaración y no deja renombrar uno que existe, así que el 120 no
  podía pasar a otro nombre para dejarle `fromBytes` al nuevo. Se escribe igual. `__charsToString` (47): un código que
  no es un carácter, `EncodingError` («appendChar: el código 55296 no es un carácter»);
- **`readLine` deja leída la línea mala,** como en Java: si no, un bucle con `try` que avisa y sigue repetiría la misma
  línea sin fin;
- **`IO`:** `readLine()` y `readLine(ignorarError)`, `readStr(n)` y `readStr(n, ignorarError)` (ésta antes: los
  métodos se resuelven en orden). **`Net`:** `recvStr` con su sobrecarga y los bytes de un carácter a medias en la
  conexión (`pendiente`); `make test-net` manda una `ñ` partida en dos envíos al eco y sale entera (`partida 2: [ñb]`),
  igual en la VM-Java. **`Json`:** junta los pares `\uD83D\uDE00`; uno suelto, «JSON: sustituto UTF-16 sin su pareja
  en pos N»;
- **la GUI:** `read_bp_string` (18 sitios en la VM-C, entre ellos el búfer de 1 KB de `setText`) recorta por
  caracteres. `Utf8Gui`, nuevo y en el corpus: 1022 `a` y una `ñ`; con el recorte de antes, la `ñ` salía partida
  (sabotaje: «FALLO … en el byte 1022 (0xC3)»);
- **las pruebas del corpus:** `Utf8Descodifica` (73 comprobaciones contra lo que dice el estándar, escrito a mano: cada
  entrada con y sin el indicador, la tabla 3-8 del estándar, los sobrelargos, los sustitutos, `readFile`, `readLine`,
  `readStr`, `appendChar`, `Json` y dos fuentes de confianza, `listDir` y `pathAbsolute`);
- **nada más cambia:** la salida de los 156 samples del corpus, guardada antes y comparada después, idéntica
  (`PARIDAD_GUARDA=<dir> compat.sh check-paridad`, nuevo: la paridad sola no ve que las dos VMs cambien igual);
- **la red VERDE:** paridad 158 en MOD9, en MOD8 y con dos workers; cápsula V6, 59; norma 9/9; sim 12/12; pruebas de
  C, 104/104 (con `test-utf8`; `test-net` se colgó contra un servidor de eco que dejé vivo a mano, y repetido solo,
  pasa); sombra 15/15; el estrés, `packs-abi` y la stdlib embebida;
- **en placa:** la Metro (M) y la Pico 2 (P), grabadas con la imagen nueva (su CRC, el de su `.elf`): `Utf8Descodifica`
  (73, 0 malas; con un `Json.mod` nuevo en `/app`, que tapa al viejo de `/lib`), `MoveSoloLectura` y `StrDosCaminos`,
  igual que el PC. Las ESP32, la STM32 y la P4, en el próximo lote.

Lo que queda fuera, cada cosa en su ficha: el texto de los packs (SQLite: `string_from_cstr` no descodifica, y el
pegamento del AOT corta en 512 bytes, a veces a medio carácter, y sin avisar), y los streams (por diseñar). El caso
especial del cursor (cadenas que empiezan por una continuación) se queda mientras quede una puerta. **(2-nov, `#739`:
quitado. La última puerta conocida era la VM-C de PC en Windows, que recibía el argumento y los nombres de fichero en
la página 1252 —`#763`, cerrada—; en su lugar, el aviso donde nace toda cadena.)**

**`#724` HECHO (27-oct; Eduardo: *«OK, sigue con la propuesta»*).** El texto de un pack, con la misma regla: el puente
lo copia ENTERO a un `byte[]` (sin el corte de 512; un tope de 1 MB que no corta, avisa) y, si la native devuelve
`string`, lo pasa por `fromBytes`; una native de pack puede devolver ahora `byte[]`, los bytes tal cual, para que la
librería elija el modo (SQLite: `getStr(i)` y `getStr(i, ignorarError)`). Todo en el compilador, con helpers que ya
había: la ABI del `.mdn` no cambia. El detalle, en la ficha. Con ella se cierra la última puerta conocida por la que
una cadena podía nacer rota; si eso basta para quitar el caso especial del cursor, está por mirar.

### La biblioteca de cadenas en C (decidida por Eduardo, 21-oct; hecha y dentro de la VM el 3-nov: `#730`, cerrada)

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

### `#730`: el diseño concreto (3-nov, para el OK de Eduardo)

Lo decidido el 3-nov (*«OK, adelante ( 1,2 y 3 si)»*): la forma de arriba; «similar», no apartarse más de un ±25 % de
la libc de cada familia; y vive en `bpgenvm-c/bpstr/`. Falta lo concreto: qué funciones, sus pruebas y la medida.

**Dónde está hoy el C de las cadenas:** en `include/bpvm_internal.h` (los de UTF-8, en línea) y en `src/builtins.c`
(los builtins), en bucles de un byte por paso. Lo que cuestan, medido en la S3 el 3-nov (`StrCenso`, 2 KB, con la caché
nueva de `#737`):
- **buscar:** `indexOf` o `contains` sin encontrar, 630 µs, unos 50 ciclos por byte: `bp_find_bytes` llama a `memcmp`
  en cada posición. `replace`, 2,3 ms; `split`, 5,9 ms (buscar y crear los trozos);
- **`fromBytes`**, 630 µs: validar con la regla de `#711`, copiar y mirar si es ASCII, tres pasadas de byte en byte;
- **`upper`**, 370 µs; **`s == t`** con dos iguales, 100 µs (el `memcmp` de la libc de la S3);
- y al nacer CADA cadena, una pasada entera para su bit ASCII (`bpvm_bytes_ascii`).

Lo más lento del censo —los patrones, construir con `s + x` o con el `StringBuilder`— son bucles de BP y la reserva: la
biblioteca no lo toca.

**Las funciones.** Todas sobre un puntero y una longitud: no reservan memoria y no conocen la VM.

| Grupo | Función | Lo que sustituye hoy | Cómo |
|---|---|---|---|
| UTF-8 | `bpstr_es_ascii(p, n)` | `bpvm_bytes_ascii`, al nacer cada cadena | el OR de palabras de 4 bytes |
| | `bpstr_cuenta(p, n)` | `utf8_cp_count` (11 sitios: `strlen`, el cursor, `indexOf`, `replace`…) | n menos los bytes de continuación, de 4 en 4 |
| | `bpstr_avanza`, `bpstr_retrocede` | los pasos del cursor (`strcur_byte_de`) y `utf8_byte_offset` | de 4 en 4 mientras quepan en lo que falta |
| | `bpstr_primer_malo`, `bpstr_largo_corregido`, `bpstr_corrige` | los de `#711` (`fromBytes`, `readFile`, `readLine`) | el atajo ASCII de 4 en 4; lo demás, la regla de `#711` tal cual |
| | `bpstr_decodifica`, `bpstr_codifica` | `utf8_decode`, `utf8_encode` | igual: son de un carácter |
| Buscar | `bpstr_busca_byte(p, n, c)` | — | el truco de 4 en 4 (lo de `memchr`) |
| | `bpstr_busca(p, n, q, qn, desde)` | `bp_find_bytes` (`indexOf`, `contains`, `split`, `replace`) | aguja de 1 byte: la de arriba; corta: su primer byte de 4 en 4 y comparar; larga: Two-Way, lineal siempre (nada cuadrático). El corte entre corta y larga, medido |
| | `bpstr_busca_atras` | `Str.lastIndexOf` (en BP, por bytes) | lo mismo, hacia atrás |
| | `bpstr_cuenta_apariciones` | la primera pasada de `replace` | con `bpstr_busca` |
| Comparar | `bpstr_compara(a, na, b, nb)`, `bpstr_igual` | `memcmp` (`Str.compare`, `<`, `==`) | de 4 en 4, si la libc de alguna familia va peor |
| Transformar | `bpstr_mayusculas`, `bpstr_minusculas` | `upper`, `lower` | lo ASCII de 4 en 4; lo demás, Latin-1 como hoy |
| | `bpstr_recorta(p, n, &a, &b)` | `trim` | igual: sólo mira los bordes |
| Copiar | `bpstr_copia`, `bpstr_rellena` | `memcpy`, `memset` | sólo si la libc de una familia no basta (la de la STM32 copia byte a byte) |

**Fuera, de momento: los números** (`parseInt`, `parseFloat`, `intToString`, `floatToString`). Ya son nuestros (`#716`,
`#734`) y en el censo cuestan poco (7–34 µs). Si se quieren dentro por tenerlo todo junto, se mueven tal cual.

**El truco de 4 en 4, con un ejemplo.** «año» son los bytes `61 C3 B1 6F`: tres caracteres, porque `B1` (`10110001`) es
de continuación. Hoy se miran los cuatro, uno a uno, con una comparación y un salto por byte. De 4 en 4 se lee la
palabra entera y se marcan sus bytes de continuación con dos máscaras (el bit 7 encendido y el 6 apagado): sale uno,
así que son 4 − 1 = 3 caracteres, en unas ocho operaciones para los cuatro bytes. Buscar una coma es parecido: con un
XOR contra `2C 2C 2C 2C`, el byte que casa queda a cero, y otra máscara dice si en la palabra hay alguno a cero.

**La codificación:**
- C99, con `uint8_t` siempre: el `char` tiene signo en unas arquitecturas y en otras no;
- lecturas de 4 bytes alineadas, nunca más allá del final: la cabeza y la cola, byte a byte;
- little-endian, comprobado al compilar; sin `malloc` y sin instrucciones especiales.

**Las pruebas** (`bpstr/prueba.c`; en el PC, `make prueba`):
1. **el oráculo:** los bucles de hoy, copiados tal cual;
2. **exhaustivo:** todas las cadenas de 0 a 3 bytes (16,8 millones) en cada función de UTF-8 y de transformar; y el
   validador, además, todas las de 4 bytes (4 300 millones, unos minutos en el PC);
3. **buscar:** con un alfabeto pequeño (`a`, `b`, `80`, `C3`), todos los pajares de hasta 8 bytes con todas las agujas de
   hasta 4;
4. **los bordes:** las longitudes de 0 a 64 en las cuatro alineaciones, con la cadena pegada al final de una página
   protegida: leer un byte de más revienta la prueba;
5. **al azar:** millones, con una semilla fija (la misma serie en el PC y en las placas): casi todo ASCII, con algún
   carácter largo y algún trozo malo;
6. **el catálogo de Markus Kuhn**, el de referencia del UTF-8 mal formado (un fichero de texto que habría que descargar).

En las placas, las mismas pruebas, más cortas: exhaustivo hasta 2 bytes, los bordes y 100 000 al azar.

**La medida** (`bpstr/banco.c`):
- **qué:** cada función contra su pareja de la libc (`memchr`, `memcmp`, `memmem` donde la hay, `memcpy`, `memset`) y
  contra el bucle de hoy, en ns por byte;
- **con qué tamaños:** 16, 256, 2048 y 16 384 bytes;
- **dónde:** en el PC y en una placa de cada familia (la Pico 2, la C6, la S3 y la Nucleo). En la S3 también con los
  datos en la PSRAM, que es donde viven allí las cadenas;
- **el criterio:** ±25 % de la pareja de la libc. Lo que no tiene pareja, más deprisa que hoy y cerca del ritmo de
  `memchr` (leer la memoria una vez).

En las placas, una imagen de prueba de cada familia (como las de la caché, fuera del repositorio) que al arrancar pasa
las pruebas y el banco, y lo deja en el log.

**Los pasos:**
1. **en el PC:** la biblioteca, sus pruebas y su banco, en `bpgenvm-c/bpstr/` con su `Makefile`;
2. **en las cuatro placas:** las pruebas y la tabla del banco. Lo que no llegue al ±25 %, otra vuelta;
3. **incorporarla:** la VM llama a la biblioteca en vez de a sus bucles (sólo `builtins.c` y `bpvm_internal.h`); de
   alta en los cinco builds; la red, que vigila que no cambie nada de lo que se ve (la VM-Java no la usa); y el censo
   antes y después, en el PC y en las placas;
4. **cerrar:** FICHAS y ESTADO.

**Por decidir (Eduardo):**
- (a) la lista: ¿entra todo lo de la tabla, y los números fuera?;
- (b) Two-Way para las agujas largas: lineal siempre, unas 150 líneas, con el de musl (licencia MIT) de referencia;
- (c) descargar el fichero de Kuhn;
- (d) ¿`-O2` para la biblioteca aunque el resto vaya con `-Os`? Lo diría la medida del paso 2.

**Decidido** (Eduardo, 3-nov: *«OK a) b) y c) yo diría que si también. Y d) también»*): la lista, con los números
fuera; Two-Way; el fichero de Kuhn, descargado (`third_party/utf8-kuhn/`, CC BY 4.0); y `-O2`.

**Y las natives** (Eduardo: *«en AOC cuando utilizas strings, lo haces a través de opCodes? Supongo que ahí también
ganaríamos»*). No van por opcodes: una native es C y llama a C. Las más usadas (`len`, `charCodeAt`, `charAt`,
`substring`, la concatenación, `==`, `intToString`) tienen su helper tipado, que llama a la misma primitiva que el
intérprete (`bpvm_prims.h`: una implementación, dos entradas). Las demás (`indexOf`, `contains`, `upper`, `replace`,
`split`…) van por el despachador genérico: los argumentos a la pila del hilo y el mismo builtin del intérprete. Así que lo
que gane la biblioteca lo ganan las natives a la vez, sin tocar el AOT, y en proporción más: sin el intérprete alrededor,
casi todo el tiempo de un `indexOf` en un bucle es la búsqueda misma. (La S3 no tiene natives: el AOT de Xtensa, `#488`,
está fuera del plan.)

### `#730`, el paso 1: la biblioteca en el PC (3-nov)

En `bpgenvm-c/bpstr/` (su `LEEME.md` dice qué es cada fichero): `bpstr.h` y `bpstr.c`, la biblioteca; `oraculo.h`,
los bucles de la VM de antes copiados tal cual; `prueba.c`; `banco.c`; y su `Makefile`.

**Las pruebas, en verde:** 1 761 millones de comprobaciones contra el oráculo en cada una de las tres formas de buscar
(el tope de hoy, todo por Two-Way, todo por la aguja corta), 21 s cada una en el PC. Y para saber que la prueba ve los
fallos, veinte sabotajes, uno a uno, en una copia: 17 salen en rojo y 2 revientan contra la página protegida (leer más
allá del final; escribir más allá de la salida). El que queda en verde no cambia el resultado: `retrocede` no baja del
byte 0, y contar ese byte de más o de menos no mueve dónde se para. Y la prueba larga (`make prueba-larga`): el
validador de `#711` con las 4 300 millones de cadenas de 4 bytes, 17 180 millones de comprobaciones, en verde.

**El banco en el PC** (64 bits, GCC 16 con `-O2`), en ns por byte con 2 KB:

| Función | bpstr | hoy | libc del PC |
|---|---|---|---|
| `avanza` hasta el final | 0,21 | 0,69 (×3,3) | — |
| `retrocede` hasta el principio | 0,18 | 0,48 (×2,7) | — |
| `primer_malo` (validar) | 0,43 | 1,21 (×2,8) | — |
| `busca_byte` (no está) | 0,10 | 0,36 (×3,6) | 0,022 (`memchr`) |
| `busca`, aguja de 3 B | 0,21 | 0,42 (×2,0) | — |
| `busca`, aguja de 32 B (Two-Way) | 0,16 | 0,56 (×3,4) | — |
| `mayusculas` (ASCII) | 0,19 | 0,38 (×2,0) | — |
| `es_ascii` | 0,054 | 0,010 (×0,2) | — |
| `cuenta` (con acentos) | 0,14 | 0,09 (×0,7) | — |
| `compara` (iguales) | 0,051 | — | 0,024 (`memcmp`) |
| `copia` | 0,019 | — | 0,009 (`memcpy`) |

Lo que pierde, en el PC, tiene una causa: **SSE**. GCC vectoriza solo los bucles más sencillos (`es_ascii` y `cuenta`
de hoy, 16 bytes por instrucción), y la libc del PC es de 16 en 16. En las placas no hay nada de eso —ni la M33 (sin
Helium), ni las RISC-V de Espressif (sin la extensión V), ni la S3 para GCC—: la medida que decide es la del paso 2.
Para el PC, si hace falta, la biblioteca puede usar allí la libc y los bucles sencillos (lo que le va mejor a cada
familia, como estaba previsto: «lo decide la medida»).

### `#730`, el paso 2: en las placas (3-nov)

Una imagen de prueba por familia, fuera del repositorio, que al arrancar pasa las pruebas en su versión corta y el banco
(`bpstr/placa.c`; el arnés y lo que costó, en el `LEEME` de `bpstr`). En la Pico 2 y la Metro (ARM, RP2350), la P4
(RISC-V) y la S3 (Xtensa); Eduardo: *«yo conectaría también la P4. Así podemos probar con ARM, RISC-V y Extensa»*.

**Las pruebas, VERDE en las cuatro:** 12 911 571 comprobaciones cada una (el exhaustivo hasta 2 bytes y una de cada
4099 de 3, buscar, Two-Way, comparar, copiar y rellenar, 3000 al azar y el catálogo de Kuhn; sin los bordes, que piden
páginas protegidas). En la Pico 2, medio minuto.

**El banco, con 2 KB**, en ns por byte. Entre paréntesis, lo que se aparta del tiempo de la libc de la familia (con −,
bpstr tarda menos) y lo que gana frente al bucle de hoy (×):

| | Pico 2 / Metro (ARM, newlib) | P4 (RISC-V, picolibc) | S3 (Xtensa, picolibc) |
|---|---|---|---|
| `es_ascii` | 7,3 (×4,6) | 2,3 (×4,8) | 5,7 (×7,8) |
| contar caracteres | 15,4 (×3,5) | 6,4 (×3,0) | 12,9 (×2,9) |
| avanzar / retroceder | 32 / 30 (×2,8 / ×3,0) | 13 / 13 (×2,5 / ×2,1) | 28 / 30 (×3,7 / ×3,4) |
| validar (`#711`) | 57 (×1,4) | 25 (×1,1) | 63 (×1,2) |
| buscar un byte (`memchr`) | 16,1 (−26 %; ×3,7) | 6,3 (−55 %; ×2,6) | 11,9 (−73 %; ×4,2) |
| buscar 3 B (`memmem`) | 37 (−10 %; ×1,6) | 14 (−19 %; ×1,3) | 34 (−42 %; ×1,5) |
| buscar 32 B, Two-Way (`memmem`) | 13,6 (**+188 %**; ×19) | 5,9 (−66 %; ×16) | 14,0 (−76 %; ×23) |
| comparar (`memcmp`) | 11,1 (−40 %) | 3,9 (−21 %) | 8,8 (−49 %) |
| mayúsculas, ASCII / con acentos | ×2,8 / ×1,7 | ×2,3 / ×1,6 | ×1,5 / ×1,7 |
| copiar (`memcpy`) | 6,1 (**+53 %**) | 2,0 (+12 %) | 4,2 (+2 %) |
| rellenar (`memset`) | 3,6 (−10 %) | 1,1 (−2 %) | 2,2 (+2 %) |

Lo que dice:
- **frente a hoy, bpstr gana en todas,** en las tres arquitecturas: de ×1,1 (validar, con un acento cada diez letras,
  que rompe a menudo el paso de 4 en 4) a ×23 (buscar una aguja larga; con 16 KB, hasta ×38);
- **frente a su pareja de la libc** (`memchr`, `memmem`, `memcmp`, `memcpy` y `memset`: seis medidas; el criterio,
  ±25 %): en la P4 y la S3, dentro o por delante en las seis —su libc, picolibc, es lenta en lo básico: su `memmem`,
  por ejemplo, tarda lo mismo con una aguja de 3 B que con una de 32, la cuenta del bucle sencillo—. En la RP2350, en
  cuatro; en dos gana newlib: `memcpy` (la suya, en ensamblador de ARM, un 53 % más deprisa) y `memmem` con una aguja
  larga (2,9 veces; abajo, por qué);
- **lo que no tiene pareja, frente al ritmo de leer la memoria una vez** (un `memchr` que no encuentra): `es_ascii` y
  contar, por delante; validar, por detrás (de 1,4 veces aquel tiempo en la S3 a 2,6 en la RP2350), por los acentos;
- **la Metro da lo mismo que la Pico 2** (el mismo chip, al mismo reloj; los datos del banco, en la SRAM): a menos de
  un 2 %, salvo Two-Way, un 5 % más lento (14,3; +200 %), que no es la placa sino la aguja: sale del texto en la
  posición `maxn/3`, y la Metro midió con 16 KB y la Pico 2 con 8 (con la de la Pico 2 se salta de media un 5 % más).
  En la S3 y la P4, con los datos en la PSRAM, lo mismo que en la RAM interna, también con 16 KB: caben en la caché.

**Por qué gana el `memmem` de newlib** (de Arm, 2018, licencia BSD-3; la cadena de ARM lleva newlib 4.5): para saltar,
el nuestro (el Two-Way de musl) mira UN byte, el que queda bajo el final de la aguja, y si está en la aguja sólo puede
saltar hasta alinearlo con su última aparición. El de newlib mira una PAREJA (los dos últimos bytes, mezclados en una
tabla de 256 entradas de un byte), y su bucle de saltar no hace nada más. El texto del banco tiene 28 símbolos (las
letras, el espacio y la coma); la aguja de la Metro, `orivkjwnf go, ltnsnrqmelfodywgh@`, 21 de ellos, pero sólo 31 de
las 784 parejas. Por ejemplo, si bajo el final de la aguja quedan `ae`: la `e` está en la aguja (en la posición 22), y
el nuestro salta 9; la pareja `ae` no está, y el de newlib salta 31. De media, con esa aguja y ese texto, el nuestro
salta 17,6 bytes y el de newlib 29,2 (contado sobre los mismos datos): ×1,7 menos pasos; lo demás, hasta el ×2,9, es
lo que cuesta cada paso, y el suyo es más corto. Lo usa con agujas de 3 a 256 bytes, y
Two-Way por encima: con las menores, su peor caso no tiene la garantía de Two-Way (lo acota que la aguja no pase
de 256).

**Por decidir (Eduardo), antes del paso 3:**
- **copiar y rellenar:** la VM ya usa `memcpy` y `memset`, y en la RP2350 y las ESP la libc va bien (en la RP2350, la
  `memcpy` de newlib gana a la nuestra; en las ESP, a la par): allí, propuesta, la VM sigue con las de la libc. La STM32
  es otra cosa: lleva newlib-nano (`--specs=nano.specs`), cuyas `memcpy`, `memset`, `memchr` y `memcmp` son de un byte
  por vuelta (de 16 a 32 bytes de código cada una, en el mapa de la Nucleo). Allí es donde `bpstr_copia` y
  `bpstr_rellena` tienen sentido, y se mide cuando se conecte;
- **una aguja larga:** (a) usar en la RP2350 el `memmem` de newlib (dos códigos según la familia, y con agujas de hasta
  256 B, sin la garantía de Two-Way), o (b) llevar a nuestro Two-Way la idea de las parejas: sólo cambia cómo salta (la
  tabla, por parejas y no por bytes, y un bucle de saltar tan corto como el suyo; una colisión de la tabla sólo acorta
  el salto) y conserva su garantía, y lo ganaría en las tres familias. Propuesta: (b), con las pruebas de hoy y medido como hoy (en el PC, además, contra una copia del `memmem` de
  newlib, que es C portable);
- **y de paso, corregir:** sin nada malo tarda más que validar y copiar por separado (en la RP2350, 85 ns/B frente a
  57 + 4). Hecho así —validar, y el tramo bueno con `memcpy`—, de un 17 a un 28 % menos, según la suma de lo medido.

### `#730`, la mezcla: lo de Arm que gana, dentro de bpstr (3-nov, tarde)

Eduardo, al ver el paso 2: *«lo que hay que hacer es un mix, mezclar nuestras librerías y las de ARM. En nuestras
librerías, copiamos las funciones de ARM que sean mejores»*; *«La librería mix debería dar resultados mejores o
iguales pero nunca peores, al menos en teoría»*; y *«Puedes insertar ensamblador en C?»* (sí: GCC lo deja, también
una función entera fuera de toda función).

**Hecho** (`bpstr/`, su `LEEME`):
- **buscar por parejas:** el `memmem` de newlib (Arm, 2018, BSD-3), para las agujas de 3 a 256 bytes, igual en las tres
  arquitecturas; Two-Way, por encima.
- **copiar:** la `memcpy` de Arm para Cortex-M, en ensamblador dentro del C, sólo en las ARM de Thumb-2. Comparada con
  la de la `libg.a` de la cadena de ARM, es la misma byte a byte (las direcciones, la codificación, los saltos; 236
  bytes). En las demás, la `memcpy` de su libc, *inline*.
- **las pruebas:**
  - una cuarta forma de compilarlas, con todo por parejas;
  - agujas de 3 a 300 bytes en tres alfabetos, uno de ellos para que las parejas choquen en la tabla;
  - copiar y rellenar, hasta 300 bytes.

  VERDE en el PC (1 761 722 528 comprobaciones en cada forma) y en las cuatro placas (13 628 658 cada una). Once
  sabotajes del camino por parejas, todos pillados.

**«Nunca peor»: dónde se cumple** (ns por byte con 2 KB; en la RP2350, la libc es la newlib de Arm):
- **copiar:** en la RP2350, lo mismo que newlib (0 %; un 34 % menos que nuestro bucle de antes); en las ESP, por
  construcción, su `memcpy`.
- **buscar con 24 a 256 bytes:** las parejas tardan de 0,89 a 1,01 veces lo que la `memmem` de newlib en la RP2350, y
  de 0,19 a 0,51 veces lo que la de picolibc en la P4 y la S3. Nuestra copia va algo más deprisa que la de newlib con
  las agujas cortas (con 3 bytes, 32 frente a 42).

**Dónde no, todavía:**
- **las agujas cortas, de 2 a 16 bytes:** qué camino gana depende del texto, no sólo del largo. Por ejemplo, en la
  Pico 2, con una aguja de 3 bytes que no está:

  | la aguja empieza por | la corta | parejas | newlib |
  |---|---|---|---|
  | un espacio (uno de cada siete bytes del texto) | 103 | 32 | 42 |
  | un byte que no está en el texto | 16 | 32 | 42 |

  Los dos caminos fallan por motivos distintos:
  - la corta se para en cada aparición del primer byte y vuelve a buscarlo de 4 en 4 desde una dirección sin alinear:
    unos 100 ciclos por parada;
  - con una aguja de 3 bytes, las parejas no saltan más de 2.

  Con el tope de hoy (la corta hasta 16), la mezcla sale hasta 15 veces peor que newlib (con el espacio y 16 bytes, en
  la RP2350). Con las parejas desde 3 bytes, también sale peor en algún caso:
  - 2,5 veces en la RP2350 (2 bytes que empiezan por el espacio: la corta, frente al bucle de dos bytes de newlib);
  - 2,6 en la P4 y 4,6 en la S3 (3 bytes que empiezan por un byte que no está: las parejas, frente a la corta).
- **las agujas de más de 256 bytes:** el Two-Way de newlib (otro, el de gnulib) va un 20 % más deprisa que el nuestro
  en la RP2350; en la P4, con este texto, hasta el bucle sencillo de picolibc nos gana, de un 11 a un 45 %.

**Lo que propongo (por decidir, Eduardo):**
1. **que la corta pare barato:** que mire las apariciones dentro de la palabra de 4 bytes que ya tiene, sin volver a
   llamar a `busca_byte`. Es el mismo algoritmo. La cuenta, sin medir: de unos 100 ciclos por parada a unos 15, y con
   el espacio, en la RP2350, de 103 ns/B a unos 30, cerca de las parejas (32);
2. **elegir el tope por la medida, en cada familia,** con tu criterio: el que deje el peor caso lo más pequeño posible. Lo
   que quede de peor (en el cruce de los dos caminos pierde siempre uno, por poco), dicho con su número;
3. **por encima de 256 bytes,** copiar el Two-Way de newlib si gana en las tres arquitecturas.

Del todo «nunca peor», en teoría, sólo se llega con una regla que mire el texto: que la corta cuente sus paradas y, si
para demasiado, siga por parejas. Es un mecanismo con estado escondido, y no lo propongo salvo que lo quieras.

### `#730`, los puntos 1, 2 y 3 (3-nov, noche)

Eduardo: *«OK 1,2 y 3»*. Medido en tres vueltas más por las cuatro placas, y lo que queda, en `bpstr/`.

**1. La corta para barato.** Busca el primer byte de 8 en 8, como `busca_byte`. Cuando alguno de los 8 lo es:
- saca de las mismas dos palabras cuáles son, con una máscara exacta;
- los mira en orden con cuatro preguntas fijas por palabra, el segundo byte de la aguja primero;
- y sigue sin volver a empezar la búsqueda.

El bucle de pasar de largo va aparte y con la forma que mejor traduce cada compilador (lo comprobé mirando lo que
genera), y `busca_byte` lleva la misma:
- con un puntero, porque con el índice GCC guarda un registro en la pila en cada vuelta en la M33;
- con las vueltas contadas antes, porque así en la S3 usa el `loop` del Xtensa, que repite el bucle sin comparar ni
  saltar. En una vuelta intermedia, sin él, `busca_byte` iba en la S3 un 13 % más lenta que en el paso 2.

En ns por byte con 2 KB y una aguja de 3 bytes, antes y ahora:

| la aguja empieza por | Pico 2 | P4 | S3 |
|---|---|---|---|
| un espacio | 103 → 48 | 38 → 19 | 96 → 49 |
| una letra | 38 → 25 | 14,5 → 10,4 | 33 → 23 |
| un byte que no está | 16,3 → 13,8 | 6,4 → 5,7 | 12,1 → 10,6 |
| (y `busca_byte`) | 16,1 → 13,5 | 6,3 → 5,3 | 11,9 → 10,4 |

**3. El Two-Way de newlib.** Gana al de musl en las tres arquitecturas y con todos los largos. Con 300 bytes: 34
frente a 46 en la Pico 2, 15 frente a 21 en la P4, 37 frente a 52 en la S3. Con 600, de un 25 a un 28 % menos en las
tres. Se queda el de newlib, y el de musl sale de la biblioteca. Al adaptarlo, su tabla de saltos queda en bytes, como
la que teníamos. El atajo de la aguja periódica sigue valiendo con los saltos recortados a 255; el porqué está en
`bpstr.c`, y una prueba con agujas de periodo de más de 255 bytes lo comprueba.

**2. Los topes, por arquitectura** (`bpstr.h`). Se eligen con tu criterio: el reparto que deja el peor caso, frente a
lo mejor de antes y la libc, lo más pequeño posible; y, a igualdad, la mejor media. Hay cuatro tramos según el largo de
la aguja: la corta, Two-Way, parejas hasta 256 y Two-Way por encima.

| | la corta hasta | Two-Way hasta | parejas, hasta |
|---|---|---|---|
| Pico 2 y Metro (y la STM32, sin medir) | 3 | 6 | 256 |
| P4 | 4 | 12 | 256 |
| S3 | 12 | (ninguno en medio) | 256 |
| el PC | 6 | (ninguno en medio) | 256 |

Qué camino gana con una aguja corta depende de lo frecuente que sea su primer byte en el texto:
- con uno que no está, la corta;
- con el espacio, Two-Way;
- en la S3, su libc es tan lenta que la corta no pierde con ninguna de hasta 12 bytes.

**Cómo queda «nunca peor»**, la mezcla frente a lo mejor de antes y la libc:

| | hasta 256 bytes | más de 256 | frente a antes |
|---|---|---|---|
| P4 | nunca peor (como mucho ×0,91) | hasta ×2,3, frente al bucle sencillo de picolibc | nunca peor: gana de ×1,10 a ×15 (mediana, ×2,5) |
| S3 | nunca peor (como mucho ×0,87) | hasta ×1,7, ídem | nunca peor: de ×1,14 a ×12 (×1,9) |
| Pico 2 | ×1,12–1,23, en seis casos de 2 a 4 bytes | nunca peor (×0,88–0,91 de newlib) | de ×0,82 a ×22 (×2,7) |
| Metro | ×1,11–1,15, en seis casos de 2 a 4 bytes (y ×1,03 en tres de 128) | nunca peor | de ×0,87 a ×20 (×2,8) |

Lo que queda, con su porqué:
- **las agujas de 2 a 4 bytes en las ARM.**
  - Con 2 y 3 bytes que empiezan por un espacio, la corta tarda 45–48 ns/B, y la `memmem` de newlib 40–42, con su bucle
    de dos bytes. Pero ese bucle tarda lo mismo con cualquier texto, y con un primer byte que no está la corta va a 13,8:
    ninguno de los dos gana siempre.
  - Con 4 bytes que empiezan por un byte que no está, Two-Way tarda 19,9 y la corta de antes 16,3 (×1,23). Con el
    espacio es al revés: la corta, 48; Two-Way, 20.
- **las agujas de más de 256 bytes en las ESP.** Con este texto, el bucle sencillo de picolibc (el primer byte, y el
  resto) va más deprisa que Two-Way. Pero su peor caso es cuadrático. Por ejemplo, con una aguja de 600 bytes
  `aaa…ab` en un texto de `a`, compara unos 600 bytes en cada posición, y Two-Way unos pocos. Como Arm, que sólo usa sus
  parejas hasta 256 por esa garantía, me quedo con Two-Way.

**Las pruebas:** VERDE en el PC (las cuatro formas de buscar, y también con los topes de cada familia) y en las cuatro
placas, cada una con los suyos (13 630 906 comprobaciones). Los sabotajes:
- ocho en la corta y en `busca_byte`, todos pillados;
- ocho en el Two-Way de newlib, siete en rojo. El que sale en verde es quitar una guarda que, razonado, no hacía falta;
  la quité.

### `#730`, el paso 3: bpstr dentro de la VM (3-nov, noche)

Eduardo: *«ok, paso 3»*.

**Cómo entra** (el detalle, en el `LEEME` de bpstr):
- la VM la compila por `src/bpstr_vm.c`, una línea que incluye `../bpstr/bpstr.c`. La STM32 la recoge sola, por su
  carpeta enlazada `src`; la dan de alta, con `-O2`, el `Makefile` del host, la Pico y las cinco ESP. En la imagen M de
  la Metro va en la SRAM, con el resto de la VM caliente;
- las funciones de cadenas de `bpvm_internal.h` (`utf8_cp_count`, `utf8_primer_malo`…) siguen con su nombre y la
  llaman: el intérprete, `heap.c` y los helpers del AOT ganan a la vez. Sus declaraciones van en ese `.h` sin incluir
  `bpstr.h`, porque las natives del AOT lo incluyen y una instalación no trae `bpstr/`;
- en `builtins.c`, lo que tenía su propio bucle: buscar (y `replace`, que ya no compara en cada posición), el cursor de
  las cadenas, `upper` y `lower`, `trim` y comparar. `startsWith` y `endsWith` se quedan con la `memcmp`: con trozos de
  3 u 8 bytes, `bpstr_igual` iba de un 5 a un 29 % peor (medido en el censo);
- los créditos (`docs/creditos.html`, también en inglés): newlib, con los avisos enteros que pide la BSD-3 de Arm para
  el binario.

**Las pruebas:**
- la red, VERDE, dos veces (la segunda, tras volver a la `memcmp`): la paridad 191 de 191, con dos workers 191, MOD8
  188 y sus tres SKIP de siempre, las de C 111 de 111;
- `test_utf8` prueba ahora bpstr contra la tabla del estándar de Unicode;
- compilan las siete familias y las dos STM32;
- en las placas, los packs aguantan. En la P4, el `nm` dice que la app crece 4,7 KB y no cambia de página; en las
  cuatro, el mismo `packRamBase`; `SqlDemo` corre en la Metro y en la P4, y la Gui de la P4 pinta.

**El censo de cadenas (`StrCenso`)**, antes y después, en las cuatro placas. Lo de después, dos vueltas, y entre ellas
el ruido es pequeño (mediana, del 0,6 al 1,9 %). Con 2 KB, la mejor de las dos entre la de antes:

| | Pico 2 | Metro | P4 | S3 |
|---|---|---|---|---|
| `indexOf` / `contains`, no está | ×0,19 / ×0,18 | ×0,17 | ×0,13 | ×0,11 |
| `replace` | ×0,43 | ×0,48 | ×0,26 | ×0,15 |
| `upper` | ×0,64 | ×0,72 | ×0,61 | ×0,47 |
| `fromBytes` | ×0,66 | ×0,64 | ×1,24 | ×0,42 |
| `==`, iguales | ×0,74 | ×0,76 | ×0,93 | ×0,81 |
| `strlen` (no toca bpstr) | **×2,05** | ×0,98 | ×0,93 | ×1,06 |
| `intToString` (no la toca) | **×1,78** | ×0,97 | ×1,05 | ×1,00 |
| `toBytes` (no la toca) | ×0,94 | ×1,00 | **×1,50** | ×0,98 |

**Lo que sale peor es, casi todo, lo que bpstr no toca:** una imagen nueva cambia de sitio el código de lo demás.
- **En la Pico 2 (la imagen P, todo desde la flash por la caché del XIP), es la lotería de `#734`.** Salen 38 casos
  peores, de `strlen` (×2,05) a `parseFloat` (×1,11). Lo comprobé con otra imagen, la misma con bpstr en la SRAM, que
  deja la flash casi como estaba: los tiempos se reparten otra vez, de otra manera. `parseFloat`, que no cambia ni una
  instrucción, pasa a ×0,67; `intToString` sigue a ×1,6. En la Metro, con lo caliente en la SRAM (la imagen M), lo que
  no toca bpstr se queda en ×1,00. Lo de fondo es `#719` (congelada): lo caliente de P, a la SRAM.
- **En la P4 y la S3, algo menos, por la caché de la flash y de la PSRAM:** `toBytes` (una `memcpy`) ×1,50 en la P4;
  `startsWith` y `endsWith` con 16 bytes, que vuelven a ser la `memcmp` de antes, ×1,13–1,18; en la S3, `parseFloat` y
  `gc()`, ×1,09–1,18. Lo de bpstr que sale peor es `fromBytes` con 2 KB en la P4 (×1,24); en las otras tres gana
  (×0,42–0,66).
- **En la Metro, uno:** `StringBuilder` con 4 `appendStr` y 256 bytes, ×1,46, cuando con 16 y con 2048 bytes da igual
  o mejor. Su heap está en la PSRAM, que comparte la caché del XIP.

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

## E7 · Las tuplas: lo que hay, cómo lo hace C# y lo que podría venir (Eduardo, 24-oct; la idea, sin empezar)

Al arreglar `#704`, Eduardo: *«Analicemos un poco todo el tema de las tuplas. No se podemos verlas en otros lenguajes Java
C# y Python a ver como lo hacen. En cierto modo en Java me recuerdan a las clases sin nombre»*; y después: *«Quizás lo que
mas nos interesa es como lo hace C# ya que JC debería hacerlo similar»*.

**Lo que hay** (24-oct):
- una clase oculta por forma, `__Tuple_ii` (sus campos, `_0`, `_1`…; raíz: no hereda de `Object`), que escribe cada
  módulo que la usa, igual en todos porque su disposición sale de la forma. La VM no sabe de tuplas: es un objeto más. El
  tipo es estructural: dos `(integer, string)` son el mismo, vengan de donde vengan;
- se crea sólo en `return (a, b)`; se lee sólo desempaquetando, `{ q, r } := f()`, sobre lvalues que ya existen; se
  guarda en una variable y se pasa como parámetro; no se compara, no se imprime y no entra en una colección (no es
  `Object`);
- en JC: `(int, string)` como tipo, `return (a, b);`, `(string, int) t = par();` y `(q, r) = par();`. El `var (q, r) =
  f();` de C# no (la tabla de §E2 lo daba por hecho, y no lo está);
- en la stdlib, cinco funciones: `Str.parseInt` y compañía, con `(boolean, valor)`, y `Time.civilDesdeDias`.

**Cómo lo hace C#:** `(int, string)` es `ValueTuple<int, string>`, una struct genérica de la biblioteca base, la misma
para todos los ensamblados. El literal vale en cualquier sitio. Los nombres de los elementos, `(int cociente, int resto)`,
sólo existen al compilar (en las firmas públicas viajan como metadato) y no cuentan para el tipo. Se lee `t.Item1` o
`t.cociente`; `var (q, r) = f();` declara al desempaquetar; igualdad por valor, `ToString`, en colecciones y como clave.
Y es un valor: asignar copia, y por eso deja cambiar un elemento. Su tipo anónimo, `new { Q = 3, R = 2 }`, es lo que más
se parece a las clases ocultas de BP —una clase por forma—, pero no sale de su ensamblado; en Java, una clase anónima es
una por sitio, no por forma; y en Python la tupla es un tipo del lenguaje.

**Decidido** (Eduardo, 24-oct): *«Lo que tenemos de tuplas es bastante bueno así que lo mantenemos y arreglamos lo que sea
necesario. Y si, en un futuro podemos extender su uso a otros sitios de lenguaje en vez de dejarlo sólo en el return de
las funciones. Un sitio donde se me ocurre sería poderlas utilizar en la declaraciones de Map o de Diccionarios
reemplazando a value.»*

**Lo que podría venir**, de más barato a más caro (BP primero y JC detrás, cuando se decida):
- el literal fuera del `return`, declarar al desempaquetar (en JC, `var (q, r) = f();`), leer un elemento sin
  desempaquetar, la igualdad y el imprimir: sólo el compilador, porque la tupla sigue siendo la misma clase oculta;
- los nombres de los elementos, como en C#: dentro del módulo, sólo el compilador; para que crucen de módulo, la
  interfaz del `.mod` tiene que llevarlos;
- **una tupla como el valor de un `Map` o de un diccionario** (la idea de Eduardo): hoy una tupla no entra en una
  colección porque no es `Object`; haría falta que la clase oculta heredara de `Object`, que vive en `Core`, o que la
  colección guardara lo que no lo es. Es lo más delicado, y lo que más uso abre;
- y, se haga lo que se haga, **de sólo lectura** (mi recomendación): en C# la tupla se copia; en BP es un objeto que se
  comparte, y si un elemento se pudiera cambiar, `t2 := t` y cambiar `t2` cambiaría también `t`.

## E8 · Los streams: el modelo (Eduardo, 24-oct; `#723`, por decidir los detalles; a V9 el 7-nov, como tema importante, con los dispositivos)

Viene de la idea del 22-oct (§E5, «El stream»: los streams, siempre de bytes, y encima un Reader que espera el resto de
un carácter). El 24-oct, Eduardo, el modelo entero:

> *«File, Uart, Socket y otros son objetos. Dentro tenemos (o podemos tener) InputStream y OutputStream (como su nombre
> indica, uno para leer y otro para escribir) siempre bytes (1 a 1 o en forma de arrays). Luego tenemos Reader y Writer
> que son objetos y en el constructor le pasamos un Stream (un InputStream para el Reader y un OutputStream para el
> writer). Pueden ser de diferentes tipos pero nosotros de momento soportamos texto, un TextReader y un TextWriter. Aquí
> el reader y el writer efectua una codificación de byte -> UTF-8 y al revés, de UTF-8 a bytes. No descartamos que en el
> futuro se puedan utilizar otras codificaciones.»*
>
> *«Una cosa mas, los Streams pueden tener un buffer con un tamaño por defecto y que el usuario puede fijar. De esta
> forma podemos optimizar las operaciones de lectura y escritura. También un método flush() para vaciar el buffer. Lo que
> aporta los streams es separar la lectur/escritura de la codificación. También hace mas homogeneo el acceso a todo tipo
> de canales de comunicación y al mismo tiempo divide el trabajo simplificando la codificación de cada canal. En esto
> seguimos el sistema de Java, un poco mas sencillo.»*

En BP se escribiría así:

```
var r: IO.TextReader := IO.TextReader(conexion.input())    // bytes → texto (UTF-8)
var linea: string := r.readLine()
var w: IO.TextWriter := IO.TextWriter(conexion.output())   // texto → bytes
w.writeLine("hola")
w.flush()
```

**Lo que aporta** (Eduardo): separar la lectura y la escritura de la codificación; el mismo acceso a todos los canales;
y que cada canal sólo tenga que mover bytes. Es el sistema de Java, más sencillo: el buffer va dentro del stream (en
Java es otra clase, `BufferedInputStream`).

**Cómo encaja con lo de hoy:** los tres canales ya tienen sus bytes, cada uno a su manera. `IO.File` tiene `readByte`,
`read(buf)` y `write`, y el texto va en `TextFile`, que hereda de `File` en vez de ponerse encima. `Net.Tcp` tiene `recv`
y `send` de bytes, y su `recvStr` guarda entre llamadas los bytes de un carácter partido (`#711`): lo que hará el
`TextReader` para cualquier canal. `Uart.Port` lee y escribe con `integer[]`, no con `byte[]`: le falta un stream de
bytes de verdad.

**Por decidir**, con lo que propongo (Claude, 24-oct):
1. **La forma de `InputStream` y `OutputStream`.** BP no tiene interfaces (`#460`): clases base cuyos métodos redefine
   cada canal (la herencia ya cruza módulos). `read(): integer` (un byte, o -1 al final), `read(buf: byte[]): integer`
   (cuántos), `write(b)`, `write(buf)`, `flush()` y `close()`.
2. **Cómo salen del canal:** `f.input()` y `f.output()`, por getters. Un fichero abierto sólo para leer no tiene
   `output()`.
3. **El buffer** (Eduardo): un tamaño por defecto y el que fije el usuario. Al leer, el stream llena su buffer del canal
   por bloques y `read` sirve de él; al escribir, lo vacía al llenarse, con `flush()` y al cerrar. Por decidir: el tamaño
   por defecto (en la C3 y la C6 la RAM va justa: `docs/MEMORIA.md`) y dónde se fija (un parámetro al pedir el stream,
   o una propiedad antes de la primera lectura).
4. **La espera.** El socket y la UART esperan; el fichero no. El plazo, una propiedad del stream; y `read` distingue
   «nada todavía» (0) de «se acabó» (-1).
5. **El carácter partido:** el `TextReader` guarda los bytes que faltan hasta la lectura siguiente, con cualquier canal.
   Sólo al acabarse el canal es un error (§E5).
6. **El UTF-8 mal formado**, lo de `#711`: la excepción (`EncodingError`) por defecto, y una sobrecarga del constructor
   para cambiar lo malo por «�» (sobrecarga y no valor por defecto: la clave de enlace).
7. **Otras codificaciones:** el hueco queda; se decide cuando lleguen (un parámetro del constructor, o una clase por
   codificación). De momento, sólo UTF-8.
8. **Cerrar:** cerrar el reader o el writer cierra su stream, como en Java (el writer, con su `flush`), y el destructor
   lo suelta (`#667`).
9. **Lo que ya existe** (`TextFile`, `Tcp.recvStr`, `Uart.Port.read`) se queda como está, para los programas de hoy; los
   streams, al lado, y más adelante lo de hoy se puede rehacer encima.
10. **Dónde viven:** en `IO` (`InputStream`, `OutputStream`, `Reader`, `Writer`, `TextReader`, `TextWriter`); `Net` y
    `Uart` importan `IO`.

Y la velocidad (§E5): lo que se hace byte a byte —llenar el buffer, buscar el salto de línea, validar el UTF-8, cortar en
el último carácter entero— va en C, con la rutina de `#711`, que ya está en las dos VMs.

## E9 · C# en JC: el traductor (H6; Eduardo, 31-oct; `#745`)

Eduardo (31-oct): *«Un hito nuevo, traductor C# -> JC. Que indique si funciona, o si falla y porqué.»* Cómo encaja con
H7 y H8, y las cifras de partida, en `FICHAS.md` («🧩 H6, H7 y H8» y `#745`). Aquí, el algoritmo, propuesto el 31-oct
para el OK de Eduardo (*«OK, adelante con H6»*). Nada programado.

### Lo que entra y lo que sale

Entra una prueba de C# (un `.cs` con su `Main`). Sale uno de estos veredictos:
- ✅ **funciona**, de dos clases: **se comprueba sola** (su `Main` devuelve un entero, y devuelve 0) o **sólo corre**
  (su `Main` no devuelve nada, y acaba sin excepción). En las dos, las dos VMs con la misma salida y el mismo código de
  salida. De las 404 candidatas de `#745`, 160 son de la primera clase y 244 de la segunda: en éstas, lo que se prueba es
  que JC acepte el programa y lo ejecute;
- ⛔ **no se traduce**: cada construcción que JC no tiene, con su línea y su mejora de H7 (o «fuera»). Todas, no sólo la
  primera: es el censo de lo que pide cada prueba, medido traduciendo;
- ❌ **falla**, y por qué: no compila en JC (el error del compilador: casi siempre, una regla del traductor que falta o
  está mal); su `Main` devuelve otra cosa (cuál); salta una excepción (cuál); no acaba en el tiempo máximo; o las dos
  VMs no coinciden;
- y aparte, **no válida**: no compila en .NET 10, o pide otros ficheros (`#745`: quedan 2322 de 2742). No cuenta.

### El algoritmo, en seis pasos

1. **Leer, con Roslyn** (el del SDK de .NET 10 del PC, como el censo de `#745`): el árbol de la prueba y sus tipos. Si
   no compila en .NET, «no válida».
2. **Recorrer el árbol entero, una vez.** Cada nodo, una de las tres reglas de «🧩 H6, H7 y H8»: **se copia**, **se
   reescribe** o **no se traduce**. Un nodo que el traductor no conoce es «no se traduce», con su nombre: nada se salta
   en silencio.
3. **Escribir un `.jc`**, un módulo con todo: `module CsTest6` para `test-6.cs`. Las clases anidadas y las de los
   `namespace`, sacadas al módulo (`A.B` se llama `A_B`); si dos se llamaran igual, o si la de dentro usa lo privado de
   la de fuera, «no se traduce». Y al final, el `main` de JC (el paso 6).
4. **Compilar** con el compilador de JC (`basicplus-frontend.jar`).
5. **Ejecutar en las dos VMs**, con un tiempo máximo (10 s, cambiable), y comparar la salida, byte a byte, y el código
   de salida.
6. **El veredicto**, de lo anterior. El `Main` de la prueba se queda como está, un método `static` de su clase, y el
   traductor añade el `main` de JC, que lo llama:

   ```
   public function void main()
   {
      int r = X.Main();
      if (r != 0)
         throw new RuntimeError("Main devuelve " + r);
   }
   ```

   Así, la traducción es un programa de JC normal: acaba con 0 si la prueba pasa y con 1 si no (`#481`), y el mensaje
   dice por qué. Si el `Main` no devuelve nada, el `main` sólo lo llama. (La VM no da el valor del `main` como código de
   salida: su tabla es fija, 0, 1, 2, 3, 4 y 130.)

### Las dos reglas de fondo

1. **Se reescribe sólo lo que JC escribe de otra forma.** Lo que JC decide distinto de C# a propósito no se tapa: «no se
   traduce», con su porqué. Un `enum` se copia como `enum` de JC (sus valores, los que da Roslyn), y convertirlo a un
   entero (`(int) Color.Rojo`), que JC no deja (`#522`), es «no se traduce» y una entrada de H7; no unas constantes
   enteras escritas por el traductor. Así una prueba prueba JC, y no al traductor. (De las 404 candidatas, 40 declaran
   un `enum` y 5 lo convierten.)
2. **Una reescritura no cambia lo que se calcula ni su orden**: no evalúa dos veces una expresión que llame a algo, ni
   cambia el orden en que se evalúan. Si hiciera falta, «no se traduce». Así, los argumentos con nombre se ponen en el
   orden de los parámetros sólo si no llaman a nada; y `s.Substring(a, n)` es `substring(s, a, a + n)` sólo si `a` no
   llama a nada.

Lo que imprime sí puede diferir de .NET, y no se tapa: un `enum` sale con su número, un `bool` como `true`, un `double`
con el formato de BP. En una prueba que se comprueba sola eso no cambia el veredicto, salvo que compare el texto; y
entonces falla, con su motivo.

### Las reglas, por construcción

Los tipos que da Roslyn dicen dónde va cada reescritura: qué `+` une cadenas, qué es un carácter, qué nombre es un
miembro de la clase, dónde hay una caja.

| C# | JC | regla |
|---|---|---|
| las clases, la herencia, los métodos, las propiedades, `if`, `while`, `do`, `try`, `throw new`, `is`, `(T) x`, el ternario, los compuestos, `x++;` | lo mismo | se copia |
| `switch`: `case 1: case 2:` y un `default` en medio | `case 1, 2:`, y el `default` al final (en C# un caso no cae en el siguiente, así que el orden no cambia nada) | se reescribe |
| `using`, `namespace`, `virtual`, `override`, `sealed`, `readonly`, `internal`, los atributos | nada | se quita |
| un miembro estático | `Clase.` delante, en cada uso | se reescribe |
| un miembro de la propia clase sin `this.` (método, campo o propiedad) | con `this.` delante, siempre | se reescribe |
| `public int x;` (un campo público) | `public int x { get; set; }` (BP no los deja) | se reescribe |
| `Console.WriteLine(x)` · `Console.Write(x)` · `Console.WriteLine("{0}-{1}", a, b)` | `println(x)` · `print(x)` · `println(a, "-", b)` | se reescribe |
| `char` | `int` (los caracteres de JC son enteros); en una cadena, `Str.fromCharCode(c)` | se reescribe |
| `int?` y los demás `T?` (2-nov) | la caja de BP de su `T` (`Integer`, `Long`, `Float`, `Double`, `Boolean`), o `null`; sus operaciones, con las funciones `Nullable_*` de `CsSystem` | se reescribe |
| `uint` (2-nov) | `long`, con su valor; `& 4294967295L` donde C# da la vuelta (`+`, `-`, `*`, `<<`, `~`), y la cuenta de un desplazamiento, con sus 5 bits | se reescribe |
| `s[i]` · `s.Length` | `charCodeAt(s, i)` · `strlen(s)` | se reescribe |
| `object o = 5;` · `(int) o` | `Object o = new Integer(5);` · `((Integer) o).value()` (las cajas de BP son a mano) | se reescribe |
| `new int[n]` · `{ 1, 2 }` | `newIntArray(n)` · `[1, 2]` (para `string`, `float`, `bool` o una clase, BP no tiene con qué: «no se traduce», y una entrada de H7) | se reescribe |
| `for (int i = 0; i < n; i++)`, con `n` fijo | `for (int i = 0; i <= n - 1; i++)` | se reescribe |
| los demás `for` | `while`, con el incremento también delante de cada `continue` | se reescribe |
| `var x = e;` | con su tipo | se reescribe |
| `=> e`, el cuerpo de un método o de una propiedad | `{ return e; }` | se reescribe |
| `abstract` (un método sin cuerpo) | el método lanza (nunca se llama: C# no deja crear la clase) | se reescribe |
| una excepción de .NET (`ArgumentException`…) | una clase del módulo que hereda de `Exception` | se reescribe |
| `throw;` | `throw e;`, con el nombre de su `catch` | se reescribe |
| y lo demás de la lista de `#745`: las clases `partial`, `params`, los métodos de extensión, los inicializadores de objetos, `using`… | con la misma regla | se reescribe |
| los genéricos (salvo `List<Object>` y el `Map`), `struct`, `ref`/`out`, los delegados, `as`, `??`, los indexadores, los operadores propios, `int[,]`, `++` dentro de una expresión… | — | no se traduce: le falta a JC (H7) |
| las lambdas, `yield`, LINQ, `async`, la reflexión, `ulong`, `decimal`, `goto`… (ya no: la herencia múltiple al estilo Java, con H7.3, `#756`; ni `int?` ni `uint`, el 2-nov; ni casi todo `ulong`, el 5-nov) | — | no se traduce: fuera |

Un nombre de C# que en JC es palabra reservada (`print`, `function`, `module`, `field`…) se renombra con un `_` detrás,
en todas partes, y el informe lo dice.

**La biblioteca de .NET**, con una tabla: cada miembro que el traductor sabe pasar a la de BP (`Console.WriteLine`,
`string.Length`, `ArrayList`, `StringBuilder`, `Math.Abs`…). Lo que no está en la tabla es «no se traduce», con su
nombre: así el informe de un bloque de H8 dice qué de la biblioteca se pide más.

### Un ejemplo: `test-6.cs`

```csharp
using System;

class X {
	public static int Main ()
	{
		Console.WriteLine ("From 0 to 9");
		int i;
		for (i = 0; i < 10; i++)
			Console.WriteLine (i);
		return 0;
	}
}
```

Lo que escribiría el traductor (escrito a mano con estas reglas; el compilador de JC lo compila):

```
module CsTest6
{
   class X
   {
      public static function int Main()
      {
         println("From 0 to 9");
         int i;
         i = 0;
         while (i < 10)
         {
            println(i);
            i++;
         }
         return 0;
      }
   }

   public function void main()
   {
      int r = X.Main();
      if (r != 0)
         throw new RuntimeError("Main devuelve " + r);
   }
}
```

El `for` usa una variable de fuera, y el de JC declara la suya: va como `while`.

### Lo que no hace

- **No ejecuta el original en .NET.** Sería ejecutar programas bajados, y no hace falta: las pruebas de Mono se
  comprueban solas, y las que no devuelven nada son «sólo corre».
- **No compara lo que imprime con .NET**, por lo mismo: compara las dos VMs.
- **No arregla JC:** lo que falta va a H7, y lo que falla, a H8, con su motivo.

### Dónde vive y cómo se usa

- **El traductor, en `compat/csharp/`**: un programa de C# sobre el Roslyn del SDK instalado (sin bajar nada de NuGet),
  y sus casos de prueba: programas pequeños de C#, escritos por nosotros, uno por regla, con el JC que tienen que dar.
  Las pruebas de Mono, y lo que sale de ellas, fuera del repositorio (`C:\lenguajes\pruebas-csharp\`).
- **Una prueba:** `cs2jc test-6.cs` escribe `CsTest6.jc`, lo compila, lo ejecuta y da el veredicto.
- **Un bloque:** `cs2jc --lote carpeta` lo mismo con todas, y un informe: cuántas de cada veredicto; de las «no se
  traduce», qué piden, contado; de las que fallan, por qué.
- **Sólo se ejecuta lo que escribe el traductor:** las construcciones de sus reglas y la biblioteca de su tabla, sin
  ficheros, red ni nada de fuera del programa (los `import` de JC los pone él).
- **Por incrementos:** primero el esqueleto (leer, el módulo, el `main`, compilar, ejecutar y el veredicto) con lo que se
  copia, `Console`, `static` y `this.`; después las demás reglas, una a una, cada una con su caso; por último el lote y
  su informe. Las pruebas de Mono, en H8.

### Decidido (Eduardo, 31-oct: *«OK a todos»*)

1. El algoritmo: los seis pasos, las dos reglas de fondo y la tabla.
2. Dónde vive: `compat/csharp/` (el traductor y sus casos), y lo de Mono fuera.
3. Las interfaces simples (`#745`): una interfaz que sólo implementan clases sin otra base, como una clase base cuyos
   métodos lanzan, como un `abstract` (en las candidatas, 28 pruebas). La herencia múltiple de verdad sigue fuera. BP no
   tiene interfaces (`#460`), y los streams de E8 ya van así.
4. No ejecutar el original en .NET (arriba).
5. Ejecutar en las dos VMs del PC las traducciones de las pruebas de Mono: sí.

### El esqueleto, hecho (31-oct)

**Dónde y cómo.** `compat/csharp/cs2jc/`, un programa de C# para .NET 10 sobre el Roslyn del SDK instalado (lo busca
por la versión del SDK: `$(NetCoreRoot)sdk\$(NETCoreSdkVersion)\Roslyn\bincore`; no baja nada). Se construye con
`dotnet build compat/csharp/cs2jc -c Release`. Tres formas de usarlo:
- `cs2jc prueba.cs`: el `.jc`, compilarlo, ejecutarlo en las dos VMs y el veredicto;
- `cs2jc --lote carpeta` (o una lista): todas, en paralelo, con su informe y un `informe.jsonl`;
- `cs2jc --casos`: los casos del traductor, `compat/csharp/casos/`. Cada `X.cs` tiene que dar su `X.jc`, y ese JC tiene
  que funcionar; o, si es de rechazos, sus `X.rechazos`, línea a línea.

La salida (el `.jc`, el `.mod` y la stdlib al lado, como en `compat.sh`) va por defecto a `%TEMP%\cs2jc`; la de las
pruebas de Mono, a `C:\lenguajes\pruebas-csharp\salida\`, fuera del repositorio.

**Lo que traduce ya:** las clases, la herencia, los métodos (los virtuales, sin `virtual` ni `override`), los
constructores con `: base(…)` y `: this(…)`, los destructores, las propiedades (las automáticas, y `{ get; }` como
`{ get; set; }`), los campos (los públicos, como propiedades, también los `static`), las constantes, `static` con su
clase delante, `this.` delante de lo de la propia clase, las clases anidadas y los `namespace` sacados al módulo, las
`partial` juntas, `if`, `while`, `do`, el `for` de JC, el `foreach` sobre un array, `try` con `throw;`, las excepciones
propias, el ternario, los compuestos, `~x` como `(x ^ -1)` y `Console.WriteLine` y `Write`, con su formato si es un
literal y usa cada argumento una vez y en orden.

**Las reglas que salieron al pasar Mono**, todas exactas:
- la clase padre, delante de la hija: JC lo pide («padre local … no es una clase abierta declarada antes»), y a C# le da
  igual el orden;
- una clase que se llama como una de Core (`List`, `Thread`…) lleva un `_` detrás, como un nombre reservado: JC no
  deja redeclararlas;
- el valor por defecto de un parámetro, el que calcula C#: JC sólo admite un literal, y C# cualquier constante
  (`int a = Diez * 2 + 1` es `int a = 21`);
- `throw;` relanza la excepción que llegó aunque el `catch` haya cambiado su variable (`test-630`): entonces la que
  llegó va en una variable del traductor, y la del programa es una local con su valor;
- un bloque suelto, que JC no tiene, como `if (true) { … }`, con el mismo ámbito.

**Un quinto motivo, nuevo:** «un fallo de JC», con su ficha. La traducción es buena y JC la haría mal: `#744`
(`base.prop`), `#751` (construir una clase declarada más abajo) y `#753` (`super()` hacia `Object`). Antes salían como
«❌ falla» con un mensaje que no decía la causa («desbordamiento de pila», o un error del emisor sin texto). Así,
«❌ falla» queda para lo que hay que mirar. La regla se quita al cerrar su ficha.

**Lo que JC no tiene y salió al compilar el JC** (el censo no lo veía): `&`, `|` y `^` entre `bool`; el «Color Color»
de C#; `null` en una cadena; una constante que cabe, en un argumento o un `return` estrecho; y los `_` de un número como
en C#. Con sus pruebas contadas, en `FICHAS.md`, «🧩 H6, H7 y H8», «Lo que salió al traducir».

**Los casos:** `Esqueleto` y `Reescrituras` funcionan en las dos VMs (sus `Main` devuelven 0); `Rechazos` y `RechazosJc`
dan sus rechazos, uno de cada motivo.

**Las cifras** (el lote entero, unos 20 s con 10 hilos): de los 2742 ficheros de `mcs/tests`, 2322 válidos, los mismos
que en el censo de `#745`. **172 funcionan** (47 se comprueban solas y 125 sólo corren). **2 fallan**, por diferencias
de JC: `test-29` (las sobrecargas entre una clase y su base) y `test-231` (`#752`). **2148 no se traducen todavía.** Lo
que más piden: los genéricos (824), los delegados (419), las interfaces simples (352), `struct` (249), las lambdas
(186, fuera), `enum` (149), `int?` (119, fuera), las cajas (113), crear arrays (105), `Main` con argumentos (105) y
`#751` (104).

**Lo siguiente:** las reglas que faltan, por lo que rinden, cada una con su caso: las interfaces simples y `abstract`,
`enum`, las cajas, los arrays, `Main` con argumentos, `params`, los demás `for`, el `switch`, los inicializadores,
`char`, los métodos de extensión, `using`, los argumentos con nombre; y la biblioteca.

### La primera tanda de reglas (31-oct)

Eduardo: *«Ya aunque solo hubiésemos llegado hasta aquí ha sido útil porque hemos detectado bugs del compilador. A
partir de aquí vamos ampliando poco a poco. Adelante con las reglas que faltan.»* Por tandas, cada regla con su caso.

- **Las interfaces simples** (lo decidido): una interfaz es una clase cuyos miembros lanzan, y la clase que la
  implementa sin otra base hereda de ella. Una interfaz que hereda de otra, una cadena de clases. La implementación
  explícita (`int IForma.Area()`) es el método con su nombre, salvo que la clase tenga otro miembro que se llame igual.
  `IDisposable` no cuenta: basta con su `Dispose`. Dos interfaces, o una y una clase, son herencia múltiple: fuera. Las
  de .NET, por su nombre (las genéricas, de H7).
- **`abstract`:** el método, o los accesores de la propiedad, lanzan («abstracto: Clase.metodo»). Nunca se llaman: C#
  no deja crear ni la clase abstracta ni la interfaz.
- **Los enums:** el de JC, con todos sus valores escritos. `==` y `!=`, tal cual; `(int) e`, tal cual (`#754`, hecha el
  mismo día: Eduardo, *«la conversión de Enum a entero ha de ser válida siempre que se haga con la sintaxis correcta»*);
  `a < b`, con `(int)` delante de los dos. Lo que da un enum desde un entero (`(Color) 2`, `a | b`, `c++`): fuera,
  `#522`. Un enum como valor por defecto de un parámetro, no: JC sólo admite un literal (H7).
- **`Main(string[] args)`:** se llama con un array vacío, `string[] argumentos = [];`, como cuando la prueba corre sola.
- **La conversión hacia arriba** (`(I) y`, `(Animal) perro`): JC no la escribe (sólo baja), y se quita: el objeto es el
  mismo y los métodos, virtuales. Salvo una cadena como `object`: en C#, `(object) s == (object) t` compara referencias.
- **`==` entre referencias de dos tipos distintos** (una clase y su hija, `Object`, una interfaz): en BP, del mismo tipo
  o con `null`. Le falta a JC (H7).

El caso, `Interfaces`: corre en las dos VMs. **Mono:** 219 funcionan (55 se comprueban solas), 3 fallan y 2100 no se
traducen todavía. Las tres que fallan son diferencias de JC: `test-126` (el `ToString()` por defecto: en C# el nombre de
la clase, en BP `object@…`), `test-29` y `test-231` (`#752`).

### Las tandas segunda a cuarta (31-oct)

- **Las cajas** (en BP, a mano): `object o = 5;` es `Object o = new Integer(5);`, y `(int) o`, `((Integer) o).value()`;
  `o is int`, `o is Integer`. Las de BP: `int`, `long`, `double`, `float` y `bool`; las demás (un `byte`, un enum, un
  `char`), todavía no.
- **Crear arrays:** con su tamaño, `new int[n]`, con las funciones de Core (`newIntArray`, `newByteArray`,
  `newLongArray`, `newDoubleArray` y `newObjArray`); de otro tipo (`string`, una clase…), JC no tiene con qué (H7). Con
  valores, `{ 1, 2 }`, `new[] { … }`: `[1, 2]`, el de JC, que toma el tipo de sus valores; un número de otro tipo, con su
  conversión (`[(long) 1, (long) 2]`); una clase hija, o valores mezclados, JC no los escribe (H7). `a.Length` es
  `len(a)`, y `s.Length`, `strlen(s)`.
- **`params`:** el parámetro, un array; y la llamada con los argumentos sueltos, con el array escrito, `f([1, 2, 3])`.
- **Los demás `for`:** como `while`, dentro de un `if (true)` que da a la variable el ámbito del `for`, y con los
  incrementos también delante de cada `continue` suyo. Y `i < 10`, con un límite constante, el de JC: `i <= 9`.
- **El `switch`:** el de JC (`case 1, 2:`, sin caer de un caso al siguiente, que C# tampoco hace), con el `default` al
  final; el valor de un `case`, el que calcula C# (`case Tope + 2:` es `case 5:`).
- **Un inicializador de objeto**, sólo en una declaración (`T x = new T() { A = 1 };` es `T x = new T();` y
  `x.A = 1;`): en otro sitio, el valor nuevo se vería antes de tiempo.
- **Los métodos de extensión:** la llamada estática que son, `x.Doble(a)` es `Ext.Doble(x, a)`.
- **`using`:** `R r = …; try { … } finally { if (r != null) r.Dispose(); }`, lo que hace C#.
- **`lock`:** el bloque (un programa de un hilo); **`unchecked`**, quitado; **`default`**, su valor, escrito; las
  cadenas **`$"…"`**, como la suma (con un formato, `{x:F2}`, H7).
- **Los argumentos con nombre:** en el orden de los parámetros; si así cambia el orden en que C# los calcula, sólo
  cuando ninguno calcula nada; y uno que se salta, con su valor por defecto escrito.
- **Y de paso:** varias constantes en una declaración, una por línea (JC no las junta); `(object) null` y
  `(X[]) null`, `null` a secas, salvo como argumento de un método con sobrecargas, donde el tipo elige cuál y JC no lo
  escribe (H7).

Los casos, `CajasYArrays`, `BuclesYSwitch` y `Varios`, corren en las dos VMs (con los de antes, 8). **Mono: 273
funcionan** (83 se comprueban solas, 190 sólo corren), **6 fallan** y 2043 no se traducen todavía. Las que fallan:
`test-126`, `test-29` y `test-231`, como antes; `test-907`, que C# da por asignada una variable tras un `switch` sobre una
constante (sabe qué caso corre) y JC no; y `gen-check` y `gen-cast-test`, que destaparon `#755`: en Windows, un `\n`
dentro de lo que se imprime sale `LF` en la VM-Java y `CRLF` en la VM-C. Y tres carencias más de JC (H7): `null` como
valor por defecto de un parámetro, un `null` con su tipo para elegir una sobrecarga, y un array de valores que sólo son
`null`.

Con `#755` hecha (el mismo día: `eol()`, y en el traductor `Environment.NewLine` es `eol()`), las dos coinciden:
**275 funcionan** y **4 fallan** (`test-126`, `test-29`, `test-231` y `test-907`).

### Cuántas más se pueden ganar, por escalones (1-nov, tras H7.3)

Eduardo: *«Y podemos aumentar el número de samples a probar? […] Me refiero a los que migramos de C#»*. Con el informe
del lote (las 290 de después de H7.3), por escalones de coste; una prueba cuenta cuando todo lo que pide está hecho:

| escalón | qué | dónde | pruebas más | en total |
|---|---|---|---|---|
| hoy | | | | 290 |
| 1 | la biblioteca de .NET y lo que el traductor aún no escribe (`char`, inicializadores…) | el traductor | +83 | 373 |
| 2 | `#751`: construir una clase declarada más abajo | el compilador (un fallo, también de BP) | +19 | 392 |
| 3 | lo pequeño de JC: `as`, `??` y `?.`, los formatos, `null` por defecto… | el compilador (S) | +32 | 424 |
| 4 | lo mediano: `++` en una expresión, `new`, indexadores, operadores, `int[,]` | el compilador (M) | +124 | 548 |
| 5 | los genéricos propios | L | +470 | 1018 |
| 6 | los delegados, sin lambdas | L | +169 | 1187 |
| 7 | `struct` | L (la VM) | +146 | 1333 |
| 8 | `ref` / `out` | L | +45 | 1378 |

El techo: 871 de las 2028 que no se traducen piden algo que queda fuera (las lambdas, la reflexión, `int?`, `async`,
`yield`, LINQ, `goto`…). Son candidatas, no aprobadas: cada una tiene que dar lo mismo en las dos VMs.

### El escalón 1, analizado y decidido (1-nov)

De sus 83 pruebas, **14 no se pueden llevar a BP**: la implementación explícita de verdad (una clase con `I.M()` y otro
`M()` propio: es de JC, que tiene un método por firma; pasa a «le falta a JC»), la E/S, la red y `ComponentModel` de
.NET, `IFormattable` (los formatos con cultura) y la caja de un sin signo. **Las otras 69**, en 24 trabajos; los que más
dan: los nombres que chocan (+11), las excepciones de .NET (+10), `Math`, `NaN`, `MaxValue` y `Convert` (+5), `char`
(+5), `String` y `Char` (+4), `Console.Out` (+3), los argumentos con nombre, los inicializadores dentro de una expresión
y los métodos `partial` sin cuerpo (+3 cada uno), y dieciséis de +1 o +2.

**Decidido** (Eduardo, 1-nov: *«podemos crear unas librerias CSharp para pasar las pruebas, que contengan las funciones
que necesitan las pruebas. Pero esas librerías en principio a nosotros para trabajar con los micros no nos interesan ya
que trabajaremos con las nuestras. Lo mismo sirve para las excepciones. En JC, char es un entero.»*; a la propuesta,
*«OK. Si, los nombres que chocan se antepone el namespace.»*):
- **Las librerías CSharp**, en `compat/csharp/lib/`, escritas en JC: en el repositorio, con el traductor (son parte de
  la herramienta), y fuera de `bpstdlib`, de las imágenes y del `Stdlib.pack`. Llevan lo de .NET que piden las pruebas,
  **con sus nombres de .NET** (`ArrayList.Add`, `Math.Acos`, `ArgumentException`…), hecho sobre lo de BP; un módulo por
  espacio de nombres de .NET, con `Cs` delante para no chocar con los de BP: `CsSystem`, `CsCollections`, `CsText`. El
  traductor cambia el tipo de .NET por el de la librería y pone el `import`; reescribe sólo lo que JC no escribe igual.
  Las compila al empezar un lote, junto a la stdlib, en su carpeta de trabajo. Lo que no esté en ellas sigue saliendo
  como «la biblioteca: X».
- **Las excepciones**, en `CsSystem`, con la jerarquía de .NET; `System.Exception` es nuestra `Exception`, y
  `System.SystemException`, nuestra `RuntimeError` —lo que lanza la VM—, así que un `catch (SystemException)` atrapa
  también un `null` o un índice fuera, como en .NET; sus hijas heredan de `RuntimeError`.
- **`char`, un entero**, con los literales de carácter que JC ya tiene (17-oct); `Str.fromCharCode` y `charCodeAt`
  donde se ve como carácter (imprimir, concatenar, `s[i]`, `ToString()`).
- **Los nombres que chocan**: el namespace delante (`N1_X`, `N2_X`), sólo cuando chocan.

El orden: la librería con las excepciones; los nombres; `Math` y `Convert`; lo demás. Cada trabajo, con su caso en
`compat/csharp/casos/` y el lote.

### El escalón 1, por trabajos (1-nov)

- **La librería y las excepciones.** `compat/csharp/lib/CsSystem.jc`: `ApplicationException`, `ArgumentException` (con
  `ParamName`), `ArgumentNullException`, `ArgumentOutOfRangeException`, `InvalidOperationException`,
  `NotImplementedException`, `NotSupportedException` y `FormatException`, con la jerarquía y los mensajes de .NET; las
  que piden las pruebas. El traductor la compila al empezar (el `Ejecutor`, junto a la stdlib) y escribe
  `import CsSystem;`; `SystemException` es `RuntimeError`, y una clase del programa que hereda de una de ellas sin
  constructor llama sola al suyo sin argumentos (comprobado en las dos VMs). Su caso, `Excepciones`. De paso, un fallo
  del traductor que salió con ellas: el valor por defecto de un método `partial` es el de su declaración sin cuerpo
  (`test-partial-25`; caso `Parciales`). **Mono: 299 funcionan** y 5 fallan: a las 4 de antes se suma `test-901`, la
  diferencia de JC de `test-907` (C# da por asignada una variable tras un `if (true)`; JC no).
- **Los nombres que chocan.** Si dos tipos se llamarían igual, el de fuera de cada uno lleva su namespace delante, y
  sus anidadas también: `N1.X` es `N1_X`, y `N2.Sub.X.Dentro`, `N2_Sub_X_Dentro`; uno del espacio global, como estaba.
  Si aún chocan (`C` y `C<T>` en el mismo namespace), no se traducen. Caso `Nombres`. **Mono: 310** (+11, lo previsto).
- **Las constantes y `Math`.** Una constante de .NET se escribe con su valor, que Roslyn sabe (`int.MaxValue`,
  `long.MinValue`, `Math.PI`, `PI` con `using static`; `double.NaN` y los infinitos, `(0.0 / 0.0)` y `(1.0 / 0.0)`;
  `string.Empty`, `""`); una de un tipo estrecho, con su tipo: `((ushort) 65535)` (a secas, un `int`, y `return 65535;`
  en una función `ushort` no compila en JC: `test-701`). Y en `CsSystem`, los métodos estáticos como funciones del
  módulo con el nombre de su tipo delante (`Math.Abs` es `CsSystem.Math_Abs`; una clase `Math` o `Double` chocaría con
  el módulo `Math` y las cajas de Core): `Abs`, `Max`, `Min`, `Floor`, `Ceiling`, `Round` (a medias al par, como .NET),
  `Acos` (el de BP, de `float`), `double.IsNaN`, `float.IsNaN`, e `int.Parse` y `long.Parse` con su `FormatException`
  (sobre `Str.parseInt`, que devuelve `(err, valor)`). Caso `Numeros`. **Mono: 312** (+2: casi todas las que los
  pedían chocan con otra cosa —los sin signo, que quedan fuera, `ToString`, `char`, las cadenas—).
- **Las cadenas.** Lo que Core ya tiene, directo, con el objeto delante (`s.StartsWith(p)` es `startsWith(s, p)`;
  también `EndsWith`, `Contains`, `IndexOf`, `Replace`, `ToUpper`, `ToLower` y `Trim`); en `CsSystem`, `Substring`
  (en .NET, el segundo es la longitud; el `substring` de Core pide el final) y `string.Join`. `string.Format`, la suma
  de sus piezas (`("" + a + "-" + b)`, con el troceado del formato de `Console.WriteLine`); el `ToString` de un entero,
  `("" + (n))`; el de un `bool`, `True` y `False`, como .NET; `Equals` de dos cadenas, `==`; `Console.Out.WriteLine`,
  `println`; `new object()`, `new Object()`. De paso, `o == "x"` con `o` un `object` (identidad, en C#) se rechaza
  como la `==` entre referencias de tipos distintos (H7): JC no la compila (`test-111`). Caso `Cadenas`. **Mono: 316**.
- **`char`, un entero** (Eduardo: *«En JC, char es un entero»*): el tipo, `int`; el literal, el de carácter de JC
  (`'a'`; una comilla, la barra o uno de control, su número); las cuentas, las de un número. Como texto, su carácter,
  `Str.fromCharCode(c)`: al sumarlo a una cadena, en `Write`, en un formato, en `$"…"` y en `ToString()`. `s[i]`,
  `charCodeAt(s, i)`; `(char) n`, los 16 bits de abajo (`& 65535`), y una constante, su valor; el `switch`, `default`,
  `new char[n]` (`newIntArray`) y un valor por defecto, con su código. En `CsSystem`, `char.IsDigit`, `IsLetter`,
  `IsLetterOrDigit`, `IsWhiteSpace`, `IsUpper`, `IsLower`, `ToUpper` y `ToLower`, de ASCII. De paso: el tamaño `long`
  de un array, a `(int)`, como C# (`test-138`). Caso `Caracteres` (y en `Rechazos`, la línea de `char`, otro rechazo).
  **Mono: 320**, y una que falla de más con su porqué, `test-673`: una cadena con un carácter de fuera del plano básico
  (U+10041); en C#, `a[0]` es la mitad alta de su par de UTF-16 (0xD800), y en BP, que guarda UTF-8, el carácter
  entero. Es la diferencia que ya decía `Length`: no se traduce sin cambiar lo que es una cadena de BP.
- **Dos reescrituras de C#.** Un método `partial` sin cuerpo: no se escribe, y su llamada, tampoco (C# la quita, con
  sus argumentos sin calcular); queda un comentario. Una local declarada en un `case` y usada en otro (en C#, su ámbito
  es el `switch`): declarada antes del `switch`, con un nombre propio (`i_s1`: en C#, un bloque hermano puede reusar el
  suyo) y un valor cualquiera (C# no deja leerla sin asignarla), y en su `case`, su asignación; una constante usada en la
  etiqueta de otro `case` no cuenta (la etiqueta lleva su valor). No dentro de un `if (true)`: JC no ve que se entra, y
  una variable que el `switch` asigna en todos sus casos le parecería sin valor (la diferencia de `test-901`). Caso
  `Ambitos`. **Mono: 325** (+5).
- **Lo privado que usa una anidada, y `lock`.** Una clase anidada es en JC una clase aparte (`A_B`), que no vería lo
  privado de `A`: lo que una anidada usa de la de fuera (privado, o protegido sin heredar) se escribe `public`; sólo
  cambia quién lo ve, no lo que se calcula (una pasada antes de escribir nada). Y `lock (x)`, como C# (`Monitor.Enter`):
  lo que se cierra se calcula una vez, antes —una asignación, como sentencia; otra cosa, en una temporal—, y si es
  `null`, `ArgumentNullException("obj")`; sin mirarlo, lo que no puede serlo (`this`, `typeof`, un `new`, una cadena).
  Casos `Accesos` (sus anidadas las construye una clase de detrás: que las construya la de fuera, declarada antes en JC,
  es `#751`) y `Varios` (su `lock`, con el `null` mirado). **Mono: 329** (+4).
- **Lo que queda, para cuando se retome** (además de lo de la lista del escalón): una propiedad de sólo lectura de
  C# (`public virtual int Y { get; }`) asignada en su constructor escribe su campo, sin setter; el traductor la pasa
  con `get; set;`, y si una hija la redefine, JC llama al setter de la hija, con su propio campo (`gtest-autoproperty-12`,
  que destapó `#744`: falla con «Main devuelve 5»). La regla: rechazarla cuando una hija la redefine, o escribirla
  de otra forma.

### Lo que puse «fuera» y no lo estaba: `int?` y `uint` (2-nov)

**La pregunta** (Eduardo, 2-nov: *«Volvemos a la revisión de la compatibilidad entre C# y JC . Hay algo que no
entiendo, ¿ por que tan pocos test migran de C# a JC ?»*), medida con el lote del 1-nov (349) y el censo del 31-oct:
- **son las pruebas de un compilador**, no programas: cada una prueba una característica de C#, y las familias más
  grandes, justo lo que JC no tiene (`gtest`, los genéricos: 585 válidas, 6 funcionan; `test-anon`, 172, y
  `test-async`, 91: ninguna); en `test-N`, la de todo, funcionan 211 de 799 (26 %);
- **todo o nada**: a 1309 de las 1967 que no se traducen les faltan dos cosas o más;
- **lo de fuera**: 1106 piden algo que se dejó fuera (las lambdas, 394; la reflexión, 305; `int?`, 144…); con eso, el
  techo era 1216 de 2322;
- **lo que le falta a JC, tres piezas grandes**: los genéricos propios (+286), los delegados (+96) y `struct` (+74); el
  traductor y su biblioteca solos, +33;
- **el informe se queda corto**: el traductor no entra en lo que rechaza (deja un hueco), así que «a las que sólo les
  falta eso» es optimista (`dtest-047`: para el traductor, sólo los genéricos; dentro, un evento con una lambda).
  Cruzado con el censo, que mira el árbol entero: de las 384 de «sólo los genéricos», 260; y 234 que no parecían pedir
  nada de fuera, sí. Las cifras de arriba ya llevan el cruce.

**Lo que no estaba fuera.** Eduardo: *«int? es Integer (la clase)»*; y *«La herencia múltiple al estilo Java ahora
entra al permitirse varias interfaces»* (H7.3, `#756`; C# no tiene otra). De lo que queda fuera, son decisión suya las
lambdas y el entero que se convierte en un enum (`#522`); lo demás lo clasifiqué yo en `#745`, y se repasa (*«OK»*, a
hacer `int?` y `uint` y repasar después la lista). `int?` lo había puesto entre «lo que la VM no tiene», y entró en la
tabla de arriba con el OK a la tabla entera.

**`int?`, hecho** (`compat/csharp/cs2jc/Nullables.cs`, y en `CsSystem` las funciones `Nullable_*`): un `T?` es la caja
de BP de su `T` —`Integer` (también la de los enteros estrechos y el `char`), `Long`, `Float`, `Double`, `Boolean`—, o
`null`. Lo que C# hace con ellos, con una función de `CsSystem` por operación y los operandos ya en su caja, así que
cada uno se calcula una vez:
- con un `null`, la aritmética da `null`, y `<`, `>`, `<=` y `>=`, `false`; `==` compara los valores, y dos `null` son
  iguales; `x == null`, la referencia; algo que no puede ser `null` (un `int`, un enum), comparado con `null`, `false`,
  como en C#;
- `&` y `|` de `bool?`, con la lógica de tres valores de C# (`null & false` es `false`; `null | true`, `true`): lo pedía
  `gtest-377`, y por eso no se quedó en «todavía no»;
- `x.Value` y `(int) x`, el valor, o `InvalidOperationException` si es `null`, como .NET; `HasValue`,
  `GetValueOrDefault`, `ToString` (vacío si es `null`); y al escribirlo, un `null` vacío;
- `x ?? y`, el ternario (la derecha, sólo si hace falta; `x`, dos veces, así que sólo si no calcula nada); `x++` y
  `x += y` como sentencia;
- las conversiones entre ellos (`int?` a `long?`; `(byte?) x`, sus bits de abajo) y con `object` (un `int?` como
  `object` es su misma `Integer`); `new int?()`, `default`; un `int?[]` con sus valores.

Caso `Nullables`. **Mono: 367** (+18), sin perder ninguna; fallan las mismas 6. Lo que queda, con su motivo en el
informe: el enum que puede ser `null` (`E?`, 9 pruebas: sacar su valor es convertir un entero en un enum, `#522`); el
`switch` sobre un `T?` (7: un `null` va al `default`, y el de JC no lo hace solo); un valor por defecto de un `T?` (3:
en JC es un objeto, y JC sólo admite un literal); y el `T?` de un `struct` (H7).

**Lo que queda de los `T?`, fuera** (7-nov). Eduardo: *«Lo de int? de C# no lo vamos a hacer, int? se traduce a
Integer»*; y, preguntado si también lo que ya se hace: *«Si el traductor traduce int? a Integer perfecto, no se toca.
n+null sinceramente no se que es ni que valor tiene, para mi eso es un error»*. Lo de arriba se queda como está; lo
que el traductor todavía no escribía de los `T?` (un `null` suelto en una cuenta, `n + null`; las cuentas de un `uint?`;
un `E?`; las asignaciones compuestas de uno estrecho…) pasa a «fuera» (`Porque.IntNullable`), y deja de contar como
trabajo por hacer. El valor por defecto de un `T?` ya salía con el tema 4 (`new Integer(5)`, en la llamada).

**`uint`, hecho** (`compat/csharp/cs2jc/SinSigno.cs`). Eduardo: *«No tenemos uint?»*: BP tiene sin signo sólo los
estrechos, `byte` y `word` (`ushort` en JC), que el traductor ya pasaba; un `uint` cabe en un `long` (propuesto, y
*«OK»*). Un `uint` es un `long` con su valor (0 … 4 294 967 295):
- C# ya sube a `long` lo que mezcla un `uint` con un `int`; el traductor sólo pone la máscara `& 4294967295L` donde C#
  da la vuelta (`+`, `-`, `*`, `<<` y `~` entre `uint`, y `++`, `--`, `+=`…), y `& 31` a la cuenta de un
  desplazamiento (C# usa 5 bits en un tipo de 32; un `long`, 6); comparar, `/`, `%` y `>>` salen bien sin nada, porque
  el valor nunca es negativo;
- `(int) u`, los 32 bits de abajo (el `(int)` de JC ya trunca); `(uint) i`, los suyos; a un tipo estrecho, sus bits de
  abajo (el cast de JC a uno estrecho mira el rango); una constante, su valor con `L`;
- un índice, con `(int)`; un array, `newLongArray`; su caja, un `Long` (y `o is uint`, todavía no: no se distingue de
  un `long`); un `switch`, con las etiquetas en `long`; `uint?`, en un `Long`, con sus comparaciones (sus cuentas,
  todavía no: piden el recorte).

Caso `SinSigno`; en el caso `Rechazos`, el `uint` que se rechazaba es ahora un `ulong`. **Mono: 376** (+9: las 7
previstas, y `test-192` y `test-cls-19`), sin perder ninguna; fallan las mismas 6. `ulong`, `decimal`, `nint` y `nuint`
siguen fuera (64 pruebas).

### El repaso de lo demás que puse «fuera» (2-nov; por decidir)

Con la pregunta de `int?` —*¿lo tiene BP con otro nombre?*—, lo que clasifiqué yo en `#745` (las lambdas y el entero a
enum, `#522`, son decisión de Eduardo). Las cuentas, con el lote de las 376 cruzado con el censo: cuántas pruebas lo
piden; en cuántas es lo único de fuera (subirían al techo si entrase); y de ésas, cuántas no piden nada a JC.

| lo de fuera | lo piden | lo único | y nada de JC | ¿lo tiene BP? |
|---|---|---|---|---|
| la reflexión | 304 | 195 | 92 | en parte: una clase de atributo es una clase (39 sólo declaran o usan atributos; `[Conditional]` quita las llamadas, como un `partial` sin cuerpo); y «¿es exactamente de esta clase?» (`o.GetType() == typeof(X)`) se escribe con `is`, porque el traductor conoce todas las clases (unas 18). `Type`, `GetMethod`, `GetCustomAttributes`, no: BP no guarda el nombre de la clase |
| `ulong`, `decimal`, `nint`, `nuint` | 64 | 35 | 16 | `ulong`: los mismos 64 bits en un `long`, con funciones para comparar, dividir, `>>` y escribir (8); `decimal` y `nint`, no |
| `goto` | 33 | 21 | 17 | no (15 van a una etiqueta; `goto case`, 2) |
| los tipos anónimos | 21 | 9 | 7 | sí: una clase que escribe el traductor (sus propiedades, `Equals` y `ToString`) |
| las fechas de .NET | 19 | 7 | 3 | poco: `now()`, en milisegundos |
| `checked` | 15 | 5 | 1 | se podría (la cuenta en `long`, y mirar si cabe), para una |
| interop, `extern`, `unsafe` | 39 | 17 | 7 | no |
| `dynamic` | 67 | 48 | 11 | no: BP es de tipos estáticos |
| `yield` | 103 | 72 | 0 | no, y todas piden además genéricos |
| LINQ | 96 | 20 | 1 | pide lambdas |
| `async` | 93 | 0 | 0 | pide `Task` y hilos |
| los hilos de .NET | 105 | 2 | 0 | BP tiene hilos (`Thread`, `Mutex`), pero el de .NET se crea con un delegado: esperan a los delegados |

De Eduardo, como dato: las lambdas, 394 / 238 / 0; el entero a enum, 55 / 21 / 13. **Lo propuesto, por lo que da:** los
atributos (hasta 39), «¿es exactamente de esta clase?» (unas 18), los tipos anónimos (7) y `ulong` (8). Por decidir
(Eduardo).

### Los eventos de C# frente a los de JC (2-nov; por decidir)

Eduardo: *«Una cosa, eventos y delegados. Eventos ya tenemos, ¿que problemas tenemos con la traducción de eventos C# a
eventos JC?»*

**Son dos mecanismos distintos con el mismo nombre.** En C#, un evento es un campo de un tipo delegado —una lista de
métodos—: dispararlo LLAMA a cada suscriptor allí mismo, en orden, antes de seguir; `+=` añade y `-=` quita; se
suscribe cualquier método (static, privado, una lambda), y el delegado puede devolver un valor. En BP y JC (§15 del
manual), un evento es un AVISO, hecho para la GUI: `raise` lo encola y sigue, y el manejador corre después, entre
cuantos. Tiene UN suscriptor (suscribirse es asignar, `b.onClick = p::pulsado`, y otro lo sustituye), que es el método
de un objeto (no uno static), marcado `function event` o público, con 4 parámetros como mucho y sin valor de vuelta; y
no se le puede preguntar si tiene suscriptor (`#771`).

**Probado** (una sonda en JC, fuera del repo, en las dos VMs, iguales): justo tras el `raise`, el manejador no ha corrido
(en C#, sí); corre tras un `sleep(1)`; y una segunda suscripción sustituye a la primera.

**Lo que hacen las pruebas** (Roslyn, sobre las 2322 válidas): de las 539 que no se traducen y piden «delegados y
eventos», sólo 82 declaran eventos; las otras 457 usan delegados sin eventos —en una variable, un parámetro,
`new D(m)`, llamándolos—, y a eso un evento de JC no llega. De las 82: 29 con lambdas (fuera), 29 con el `EventHandler`
de .NET, 23 con delegados como valor, 21 con su `add`/`remove`, 20 virtuales o abstractos, 16 en una interfaz, 12 que
devuelven un valor, 12 con un manejador static, 11 eventos static…; suscriben el método de un objeto, 3; y sólo 6 tienen
la forma de un evento de JC, y ésas ni se suscriben ni se disparan: sólo lo declaran.

**Lo que encajaría** (propuesto, sin hacer): los delegados como objetos, como Java antes de las lambdas, con lo que JC ya
tiene (las interfaces de H7.3). Un tipo delegado, una interfaz con su `Invoke`; un método usado como valor (`obj.M`,
`new D(obj.M)`, uno static), una clase pequeña que escribe el traductor, que guarda el objeto y llama a su método;
varios a la vez, una lista; y un evento de C#, un campo de ese tipo: `+=` añade, `-=` quita, y dispararlo llama a cada
uno allí mismo. Las lambdas siguen fuera. Lo que da: 53 pruebas no piden nada de fuera y de JC sólo los delegados (20
con eventos), y más detrás de los genéricos. Por decidir (Eduardo).

### `++`, `--` y `=` dentro de una expresión: partirla en dos (2-nov; por decidir)

Eduardo: *«A ver v++ si lo queremos pasar a JC tendríamos que partir la expresión en 2. Con un poco de trabajo se
podría hacer.»* JC (y BP) no deja que una expresión cambie una variable; el traductor puede sacar el cambio a una
sentencia propia —detrás, o delante con una temporal— y dejar en su sitio el valor. Es exacto (la segunda regla de
fondo: no cambia lo que se calcula ni su orden) cuando nada más de la misma sentencia lee lo que cambia y, si es un
campo, una propiedad o un elemento, ninguna otra llamada de la sentencia podría verlo; bajo `&&`, `||` o `?:`, que
sólo calculan a veces, no.

| nivel | qué | en C# → en JC |
|---|---|---|
| 1 | una variable local, en una sentencia | `int j = i++;` → `int j = i; i++;` · `return i++;` → `int t = i; i++; return t;` · `if ((x = Lee()) != null)` → `x = Lee(); if (x != null)` |
| 2 | una asignación encadenada | `a = b = 0;` → `b = 0; a = b;` (con una propiedad, por una temporal) |
| 3 | una variable, en la condición de un bucle | `while (n-- > 0) {…}` → `while (true) { int t = n; n--; if (!(t > 0)) break; … }` |
| 4 | un campo, una propiedad o un elemento, sin otras llamadas | `return this.cuenta++;` → `int t = this.cuenta; this.cuenta++; return t;` |

**Medido** (Roslyn, 2-nov; el lote de las 376): 79 pruebas lo piden, en 206 sitios; por su sitio más difícil, con los
niveles 1 a 4 quedan **54** (17 + 17 + 3 + 17). De las 16 a las que de JC sólo les falta esto (y nada de fuera), las 16.
Lo que queda: lo que se vuelve a leer en la misma sentencia (`a[i] = i++`; 9 pruebas), bajo `&&`, `||` o `?:` (1), los
parámetros `ref`/`out` y los indexadores (14), y el valor de un campo (`static int a = b = 5;`: no hay sentencia donde
ponerlo). Por decidir (Eduardo).

### H8, tema a tema (Eduardo, 2-nov)

Eduardo: *«Yo creo que deberíamos seguir un método. Mi propuesta, empezar por el tema 1 y hacer pequeños ajustes hasta
ver donde podemos llegar. Despues repetir con el tema 2 y así.»* Los temas, en el orden de la tabla de FICHAS («🧩 H6,
H7 y H8»); cada ajuste, con su algoritmo y el OK, su caso en `compat/csharp/casos` y el lote; lo que toca JC (el
lenguaje), con su forma decidida por Eduardo. La cuenta de cada tema con el lote de cada día, y lo que le falta a cada
prueba, con dos scripts de la carpeta temporal (`temas_hoy.py`, `tema1.py`).

**El tema 1, «expresiones y lo demás»** (141 válidas; 86 funcionaban). Los ajustes, por lo que da cada uno en él:

| # | ajuste | dónde | da | hecho |
|---|---|---|---|---|
| 1 | los atributos: una clase de atributo, una clase más; `[Conditional]`, quitar sus llamadas | el traductor | +9 | 2-nov: **95** |
| 2 | `++` partido, los niveles 1 a 4 (arriba) | el traductor | +2 | 2-nov: **97** |
| 3 | los formatos `{0:X}` y `{0,5}` | `CsSystem` | +2 | 2-nov: **99** |
| 4 | lo pequeño de JC: `as`, una constante estrecha en un argumento, el «Color Color», `1__000` | el traductor (JC, como Java) | +4 | 2-nov: **103** |
| 5 | el tipo exacto, `o.GetType() == typeof(X)` | el traductor | hasta +4 | |
| 6 | la E/S de .NET y lo demás de la biblioteca | `CsSystem` | +3 a +5 | |

Fuera, salvo que Eduardo decida otra cosa: `null` en una cadena, `FormattableString`, `IPAddress`, y las 20 que piden
algo de fuera de verdad (`goto`, `decimal`, interop, `dynamic`…).

**1. Los atributos, hecho** (Eduardo: *«OK, adelante»*). Una clase que hereda de `System.Attribute` es una clase más,
sin padre (sus usos, `[X(…)]`, el traductor ya los quitaba); y un método `[Conditional("S")]` se escribe, y sus
llamadas, si `S` no está definido en el fichero (`#define`, `#undef`), se quitan como en C#, sin calcular sus
argumentos, con un comentario (como las de un `partial` sin cuerpo); en otro sitio que una sentencia, todavía no. Leer
los atributos (`GetCustomAttributes`…) sigue siendo reflexión. De paso, un fallo del traductor que salió con
`test-272`: los accesores de una propiedad, con el `get` delante (JC lo pide así; C# deja `{ set {…} get {…} }`). Caso
`Atributos`. **Mono: 404** (+28: 9 del tema 1 y 19 de otros temas), sin perder ninguna; fallan las mismas 6. **El tema 1:
95 de 141.**

**2. `++` partido, hecho** (Eduardo: *«OK, adelante»*; `compat/csharp/cs2jc/Sacar.cs`). El `++`, el `--` o el `=` de
dentro de una expresión sale a su sentencia: `++x` y `x = e`, delante, y en su sitio `x`; `x++`, detrás si la
sentencia lo deja (una declaración, una asignación, una llamada), o delante con una temporal (`int valor_s1 = x; x++;`)
en un `return`, un `throw`, la condición de un `if`, un `switch` o un `foreach`. En un else if, la cadena sigue en un
`else { …; if … }`; en la condición de un bucle, `while (true) { …; if (!(cond)) { break; } cuerpo }`, y en un `do`
con `continue`, una variable que se salta la condición sólo la primera vuelta (el `continue` de C# va a la condición).
Una propiedad, sólo `P = e`, por una temporal. Lo que no cumple las condiciones de arriba sigue «le falta a JC». De
paso, un `for` sin variable que declarar se escribe sin el `if (true)` (no hace falta, y JC no veía tras él la variable
asignada: `test-25`). Caso `Partidas` (y en `Rechazos`, el `++` que se rechaza es ahora uno que se vuelve a leer).
**Mono: 414** (+10), sin perder ninguna; fallan las mismas 6. **El tema 1: 97 de 141.** De las 16 a las que de JC sólo
les faltaba esto, entran 9 (y `test-25`): 5 piden además otra cosa que antes quedaba tapada (los inicializadores fuera
de una declaración, 3; un `foreach` propio; `IDisposable`) y una es `stack[sp++] = cc` (el índice cambia dentro del
destino).

**3. Los formatos, hecho** (Eduardo: *«OK, adelante»*). Medido antes: 14 pruebas los piden, y sólo a 5 no les falta
nada más, cada una con uno distinto (el ancho, 2; `x`, 1; `0.00`, 1; un formato en una cadena, 1). En `CsSystem`,
`Format_Ancho` (`{x,N}`: espacios delante si N > 0, detrás si N < 0), `Format_Hex` (`x` y `X` con sus cifras; un
negativo, en complemento a dos: 8 cifras un int, 16 un long, como .NET; uno estrecho con signo, con sus bits) y
`Format_Decimales` (un formato a medida de ceros y un punto, redondeando a medias hacia fuera del cero). En el
traductor, en `$"…"` y en `Console.WriteLine`/`string.Format`, con la expresión sin su caja; en una cadena, un bool o
un char, que no tienen formatos, ninguno (.NET lo ignora); los demás (`N`, `F2`, `D5`, las fechas…), todavía «le falta
a JC: formatos». Caso `Formatos`. **Mono: 419** (+5: las 5), sin perder ninguna; fallan las mismas 6. **El tema 1: 99
de 141.**

**4. Lo pequeño de JC, hecho en el traductor** (Eduardo: *«si»*, a hacerlo ahí y no en el lenguaje). En los cuatro, JC
hace lo que Java, y el traductor escribe lo que quiere decir C#: `x as T`, `(x is T ? (T) x : null)` (subir, el mismo
objeto; un `T?` de una caja, la caja; `x`, sin calcular nada); una constante estrecha en un argumento o un `return`,
con su conversión escrita, `((byte) 5)`; un tipo que se llama como una variable, un campo, una propiedad o un
parámetro (el «Color Color»), con un `_` detrás y su aviso, como las palabras reservadas; y `1__000`, `1000`. Caso
`Pequenos` (y `RechazosJc`, al día: esas cuatro ya no se rechazan). **Mono: 435** (+16), sin perder ninguna; falla una
más, `test-346`, que tapaba el `as`: dos sobrecargas que encajan subiendo, «ambigua» en JC (`#772`). **El tema 1: 103
de 141.**

**Los `null` de las cadenas, de paso por todos los temas** (2-nov, tras `#774`; Eduardo, a revisar el traductor:
*«OK si»*). JC ya los tiene (una variable `string` puede valer `null`), así que el traductor deja de rechazarlos:
`string s = null`, `default(string)`, `(string) null` y `== null`, tal cual. Lo que difiere, a mano: un campo o una
propiedad `string` sin valor nace `null` en C# y `""` en JC, así que el traductor escribe el `= null` de C#; un `null`
escrito en una suma de cadenas (`"x" + null`), `""`, lo que vale en C# (en JC es un error a propósito); y
`string.IsNullOrEmpty`, en `CsSystem`. Siguen fuera un parámetro con `= null` por defecto (JC pide un literal) y `case
null`. El caso `CadenasNulas`. **Mono: 441** (+7: `test-506`, `test-772`, `test-79` y cuatro `test-xml`); falla una
que funcionaba, `gtest-autoproperty-20` (el `= null` de la hija corre tras el constructor de la base, que ya le había
dado valor: `#776`), y cuatro que no se traducían fallan ahora por otra cosa: `test-243` (`#777`), `test-163`
(`#772`), `test-77` y `test-504` (`#778`). Con `#777` (una constante no puede ser `null`; el traductor escribe la de C#
como una variable), `test-243` funciona: **Mono: 442**.

### Los temas 1 y 2, otra vez (4-nov)

Eduardo: *«Ahora me gustaría probar el primer y segundo grupo a ver cuantas pasan, y de las que no pasan a ver si podemos
hacer algo para que pasen algunas mas.»* Con el lote del 4-nov (Mono, 447, y fallan las 6 de siempre): **el tema 1, 106
de 141; el tema 2, «static, anidadas y partial», 73 de 123**, y ninguna falla. Leídas una a una las 85 que no pasan,
doce propuestas por lo que da cada una. Eduardo: *«1) al 7) si, el 8 lo hablamos, el 12 si»*; del 9 y el 11, nada, así que
no se tocan; y del 10, el `goto`: *«No vamos a ñadir labels ni goto. Pero se puede reemplazar por do ... loop y break. Lo
que igual necesitamos es si tenemos varios loops anidados poder hacer un break a un loop concreto. No se poder poner 'do'
'miLoop' ... break miLoop ... Loop»*: por analizar.

| # | ajuste | dónde | tema 1 | tema 2 | hecho |
|---|---|---|---|---|---|
| 1 | `IDisposable`, una interfaz de verdad | `CsSystem` y el traductor | | +2 | 4-nov: Mono **450** |
| 2 | un `struct` sin campos de instancia, una clase | el traductor | | +2 | 4-nov: Mono **468** |
| 3 | lo suelto de la biblioteca (`d.ToString()`, `"x" is String`, `sizeof(int)`, `StringWriter`, `StreamWriter.Null`, `String.Concat`) | `CsSystem` y el traductor | +5 | | 4-nov: Mono **475** |
| 4 | `??` con referencias | el traductor | | +1 | 4-nov: Mono **476** |
| 5 | una implementación explícita y otro miembro con su nombre | el traductor | | +1 | 4-nov: Mono **485** |
| 6 | un constructor estático sin llamadas dentro | el traductor | | +1 | 4-nov: Mono **487** |
| 7 | dos sueltas: una local sin usar ni valor, `static bool x = x = F();` | el traductor | +1 | +1 | 4-nov: Mono **489** |
| 8 | el tipo de un objeto, sin `Type` al ejecutar (propuesto: una tabla con las clases; hablado: lo escribe el traductor) | el traductor | +1 | +8 | 4-nov: Mono **516** |
| 9 | los tipos anónimos | sin decidir | +2 | +2 | |
| 10 | `goto`, con `continue` a un bucle con nombre | el compilador y el traductor | +4 | | 4-nov: Mono **535** |
| 11 | `ulong`, sólo lo que da los mismos bits en un `long` | el traductor | +2 | | 5-nov: Mono **570** |
| 12 | `&`, `\|` y `^` entre `bool`, en JC como en Java | el compilador | | +1 | 4-nov: Mono **495** |

**1. `IDisposable`, hecho.** Una interfaz de `CsSystem`, `function void Dispose()`: una clase del programa la implementa
como las suyas (`class C : CsSystem.IDisposable`, y en su método, `@implement{ IDisposable }`, sin el módulo), una interfaz
del programa hereda de ella, una variable puede ser de su tipo, y `is`, `as`, una conversión y `d.Dispose()` funcionan. Así
`using` vale con cualquier tipo (antes, sólo con una clase del programa con su `Dispose`, no heredado); con varias
variables, una dentro de otra, como dice C#. Caso `Recursos`. **Mono: 450** (+3: `test-713`, `test-727` y `test-53`), sin
perder ninguna; fallan las mismas 6. **El tema 2: 75.**

**2. Un `struct` sin estado, hecho.** Sin campos de instancia (ni propiedades automáticas o eventos, que los llevan), un
`struct` se escribe como una clase: sin estado, que se copie al asignarlo no se nota, y un objeto vale tanto como otro. Lo que
hay que escribir es su valor por defecto, que en C# existe y en una clase sería `null`: una local sin valor, un campo o una
propiedad nacen con `new S()`, y `default(S)` es `new S()`. Meterlo en una caja no hace nada (ya es un objeto), y sacarlo
es una conversión; si declara constructores, se escribe además uno sin argumentos, que en C# siempre está; y `this = x`, que
sólo existe en un `struct`, no tiene nada que copiar: se calcula `x`, si calcula algo. El que tiene campos sigue sin
traducirse (`S?`, también). Caso `Structs`. **Mono: 468** (+18), sin perder ninguna; fallan las mismas 6. **El tema 2: 76**
(`test-static-using-07` pide además `String.Concat`, en el ajuste 3).

**3. Lo suelto de la biblioteca, hecho.** `d.ToString()` de un `double` o un `float`, y `d.ToString("R")`: el número como lo
escribe BP, igual que al imprimirlo. `s is string` y `s is object` con una cadena delante: que no sea `null`; una constante,
`true`. `sizeof(int)`, de un tipo de C#: una constante, que da Roslyn. En `CsSystem`, `TextWriter` (el que no escribe en
ningún sitio, `TextWriter.Null` y `StreamWriter.Null`) y `StringWriter`, que se queda lo que le escriben; `new StreamWriter(…)`
sigue sin traducirse. Y `string.Concat`, la suma, con los argumentos sueltos. Caso `Suelto`. **Mono: 475** (+7), sin perder
ninguna; fallan las mismas 6. **El tema 1: 111; el tema 2: 77.**

**4. `??` con referencias, hecho.** `a ?? b` es `(a != null ? a : b)`: la derecha, sólo si hace falta, como C#. `null ?? b` es
`b`, y lo que no puede ser null (`new X() ?? b`), él mismo. `a` va dos veces: si calcula algo, una vez, en una temporal delante
de la sentencia (`Sacar.cs`), siempre que no se calcule antes otra cosa que tenga efecto ni esté bajo `&&`, `||`, `?:`; si no,
sigue sin traducirse. Con una interfaz a la izquierda, la derecha lleva la conversión, que da su tipo al ternario de JC
(`#761`). Y `x ??= y`, con algo que no calcula: `x = (x != null ? x : y)`. Caso `NuloOtro` (y `RechazosJc`, lo que no se puede
adelantar). **Mono: 476** (+1, `gtest-333`), sin perder ninguna; fallan las mismas 6. **El tema 2: 78.** La estimación de 12
que di contaba también `?.`: las otras 11 piden `?.`, que queda para su tema.

**5. Las explícitas que chocan, hecho** (`compat/csharp/cs2jc/Explicitas.cs`). En JC una implementación explícita es un método
público con el nombre del de la interfaz; si la clase, o un padre, tiene otro miembro que se escribiría igual (`int IFoo.Hello()`
y `public int Hello()`), u otra explícita de otra interfaz (`void IA.Stop()` y `void IB.Stop()`), son dos cuerpos para un
nombre. Propuse cambiar el nombre del método de la clase; al hacerlo, mejor al revés, y da más: lleva otro nombre **el miembro
de la interfaz**, en todo el programa (`IFoo_Hello`). La explícita se escribe con él, lo que llama por la interfaz también, y
una clase que la implementa con un miembro normal añade uno que le reenvía. Lo de la clase no cambia de nombre, así que quien
lo llama o lo sobrescribe no lo nota, y entra también «la implementación explícita de verdad» (dos interfaces, dos cuerpos), que
estaba como algo que le faltaba a JC. Sólo si la interfaz no es del programa (`IDisposable`) sigue sin traducirse. Caso
`Explicitas`. **Mono: 485** (+9), sin perder ninguna; fallan las mismas 6. **El tema 2: 79.**

**6. Un constructor estático que sólo da valores, hecho.** En C# corre la primera vez que se usa la clase; en JC, lo estático
se calcula al cargar el módulo. No se nota si sólo da valores a lo estático de su clase con lo que no tiene efectos ni mira
nada de fuera: sin llamadas, sin nada que cambie, sin leer lo estático de otra clase, y un `new` sólo de un constructor
implícito. Se escribe detrás de todo, porque en C# corre detrás de los valores de los campos: una función `Cs_Estatico`, y un
campo privado que la llama. Vacío, nada. Si hace otra cosa, se notaría cuándo corre, y sigue sin traducirse. Caso `Estaticos` (y
`Rechazos`, uno que llama a algo). **Mono: 487** (+2: `test-debug-03` y `gtest-autoproperty-18`), sin perder ninguna; fallan las
mismas 6. **El tema 2: 80.**

**7. Dos sueltas, hecho.** Una local sin valor que nadie usa no hace nada, y no se escribe, sea del tipo que sea
(`System.Net.IPAddress[] x;`, `test-55`). Y un campo que se da a sí mismo el valor, `static bool x = x = F();` (`test-422`),
con lo de la derecha: en el valor de un campo no hay sentencia a la que sacar la asignación. Caso `Sueltas`. **Mono: 489** (+2),
sin perder ninguna; fallan las mismas 6. **El tema 1: 112; el tema 2: 81**, lo previsto para los siete.

**12. `&`, `|` y `^` entre `bool`, hecho, en JC y en BP** (Eduardo: *«el 12 si»*). Calculan los dos lados, como en Java y en C#;
`and` y `or` (`&&` y `||` en JC) se siguen saltando el segundo. Sólo el compilador: un `boolean` es 0 o 1, y las instrucciones
de bits ya hacen lo lógico; la VM no cambia, y en el AOT son los de C. Sus compuestos también (`&=`, `|=`, `xor=`). El tipo:
los dos `boolean`, o un error que lo dice (`'&' requiere integer/long, o dos boolean`); un desplazamiento, sólo con números.
En el traductor, fuera sus dos rechazos. Sample `BoolBits` en el corpus, `BoolBitsMal` en los de error, `BoolBitsTest` y el
AOT en `AotCoberturaTest` (31 de 31); los mismos `.mod` en los 734 fuentes y en la stdlib.

Y salió otra cosa: dos pruebas que se traducían ya (`test-484`, `test-533`) daban un error interno, `Símbolo de datos no
declarado`. Una constante de clase de C# puede ser una cuenta (`const bool B = !A & C;`) y JC pide un literal; el traductor la
copiaba tal cual, y JC, en vez de decirlo, se paraba en el emisor. Lo primero, en el traductor: una constante de clase se
escribe con el valor que calcula Roslyn; la que no tiene literal (`NaN`, `int.MinValue`), como una variable que nadie cambia.
Caso `Constantes`. Lo segundo, en el compilador, `#789`: una const de clase sin literal dice lo que dice la de módulo. **Mono:
495** (+6, con `test-144`), sin perder ninguna; fallan las mismas 6. **El tema 2: 82.**

### `break` a un bucle con nombre, para el `goto` (4-nov; decidido y hecho en el compilador)

Eduardo: *«No vamos a ñadir labels ni goto. Pero se puede reemplazar por do ... loop y break. Lo que igual necesitamos es si
tenemos varios loops anidados poder hacer un break a un loop concreto. No se poder poner 'do' 'miLoop' ... break miLoop ...
Loop»*.

**La forma.** Un bucle `do` lleva un nombre detrás de la palabra, y `break` y `continue` pueden nombrar a uno de los que están
abiertos. Sin palabras nuevas, sin etiquetas y sin `goto`: el nombre sólo se ve dentro de su bucle.

```
do miLoop                          // BP
  for i := 1 to 3 do
    if i == 2 then
      break miLoop                 // sale de los dos
    endif
  next i
loop

do miLoop                          // JC
{
   for (int i = 1; i <= 3; i++)
   {
      if (i == 2) { break miLoop; }
   }
} while (true);
```

**Por qué basta para traducir `goto`.** En C# una etiqueta se ve en su bloque y en los de dentro, así que un `goto` nunca entra
en un bloque: se queda en el de la etiqueta o sale hacia él. Cada bloque con etiquetas se parte por ellas en trozos, y se
escribe un bucle con una variable que dice por qué trozo sigue. `if (pc <= k)` corre el trozo `k` y, al seguir, los de detrás,
en orden, como en C#; un `goto` es dar valor a `pc` y `continue` a ese bucle. `test-515`:

```csharp
int i = 0;                         // C#
goto a;
b:
   if (++ i > 1) throw new System.Exception ("infloop!!!");
   return;
a:
   goto b;
```
```
int i = 0;                         // JC
int pc = 0;
do salto
{
   if (pc <= 0) { pc = 2; continue salto; }
   if (pc <= 1) { i++; if (i > 1) { throw …; } return; }
   if (pc <= 2) { pc = 1; continue salto; }
   break salto;
} while (true);
```

El nombre hace falta porque el `goto` puede estar dentro de un `for`, de un `switch` o de un `try` (`test-626`, `test-519`): sin
él, `continue` iría a ese bucle. Y hace falta `continue` con nombre además de `break`: un `goto` hacia atrás vuelve a empezar.
Las locales de los trozos se declaran delante del bucle (lo que hace ya el `switch`, `Local`).

**Lo que toca.** Sólo el compilador: los dos parsers, el análisis (que el nombre sea de un `do` abierto; que un `finally` no salga
con él, `#674`; la asignación definida), el emisor (`loopStack` ya guarda, de cada bucle, cuántos `try` y cuántas sentencias
abiertas cierra un salto: se busca por el nombre en vez de mirar el de arriba), el AOT (en C, un `goto` a su fin o a su
principio) y los dos escritores. La VM y el formato del `.mod` no cambian: son saltos. Sirve además para quien programa: hoy
salir de dos bucles pide una variable bandera.

**Lo que da.** De las 26 que aún piden `goto`, a 19 no les falta nada más. Las del tema 1, las cuatro. Otra regla, para las etiquetas
dentro de un `case` (`test-193`, `test-854`, `test-873`) y `goto case` (`test-872`, `test-876`): el `switch` entero es el bloque, y
elegir el `case` es dar valor a `pc`.

**Una regla o dos** (hablado con Eduardo: *«tendría que ser un loop false y para ir hacia atrás necesitamos algo mas complicado. Y si
tenemos los dos gotos necesitamos 2 bucles»*). Para lo sencillo, su forma es la clara: hacia delante, `do fin … break fin … loop false`;
hacia atrás, `do ini … continue ini … break ini … loop`. Pero cuando se cruzan (`test-515`: el bloque de `goto a` va del principio a
`a`, el bucle de `goto b` de `b` al final, y ninguno contiene al otro) `goto a` tendría que entrar por la mitad de un bucle, y `break`
sólo sale. De las 19: 7 sólo hacia delante, 1 sólo hacia atrás, 5 se cruzan y 6 dentro de un `switch`. La variable sirve para las 19,
así que el traductor usa ésa, una sola regla (*«OK»*); y `do … loop false` con `break` queda para quien escribe BP a mano.

**Hecho en el compilador** (4-nov): `do miLoop`, `break miLoop` y `continue miLoop`, en BP y en JC (`do miLoop { … } while (c);`), sólo en
`do`, lo que recomendé y no se corrigió: `while` y `for`, si hacen falta, con la misma regla. Sin palabras nuevas. El nombre tiene que ser de
un `do` abierto, dos abiertos no se llaman igual, y una rama de `parallel` no ve los de fuera. No se sale de un `finally` (`#674`), y
un `continue` va a la condición, como el de siempre. En el emisor, `loopStack` busca por el nombre; en el AOT, dos etiquetas de C y un
`goto`, tras cerrar los `try` de encima; la asignación definida sigue el salto al bucle que toca. La VM y el `.mod` no cambian. Sample
`BucleConNombre`, `BucleConNombreMal`, `BucleConNombreTest`, y en `NatSwitch`, ejecutado en una `native` (`make test-aotswitch`, 16/16,
también `&` entre `boolean`). El manual, §7.5, y la gramática.

**Hecho en el traductor** (4-nov, `compat/csharp/cs2jc/Saltos.cs`). La regla de arriba: un bloque con etiquetas a las que va un `goto`
se parte por ellas, y se escribe `do salto { if (salto <= 0) { … } if (salto <= 1) { … } break salto; } while (true);`. Un `switch` con
etiquetas o con `goto case` se parte igual: cada sección empieza un trozo, y el `switch` de JC sólo elige por cuál se empieza; de una
sección no se cae en la siguiente, porque C# no lo deja. Las locales de un trozo que se usan en otro, delante, como las de un `case`.
Lo que no estaba previsto: el bucle de la máquina no es de C#, y un `break` o un `continue` de un bucle de fuera lo cruzaría
(`test-579`): salen por una variable, `fuga`, y el salto se hace detrás; el `break` del propio `switch` sale de su máquina. Y si C#
dice que no se llega al final, un `throw` detrás: JC ve el `break` del bucle y pediría un `return`. Caso `Saltos` (y `Rechazos`: el
`goto` ya no se rechaza). **Mono: 535** (+19, las 19 de la cuenta), sin perder ninguna; fallan las mismas 6. **El tema 1: 117.**

### El tipo de un objeto (ajuste 8; hecho el 4-nov, `compat/csharp/cs2jc/Tipos.cs`)

Propuse un `Type` mínimo, una clase de `CsSystem` con una tabla de las clases del programa. Eduardo, al hablarlo: *«El compilador
ya conoce el tipo entonces, no se puede comparar si un type el igual ?»*; *«El 'instanceof' no puede sustituir la expresion?»*; y de
dos `Object` con un `Integer` y un `Float` dentro: *«eso no lo puede resolver el compilador, tendría que ser la VM»*. Es mejor: **no hay
un `Type` al ejecutar.** El traductor ve el programa entero, y lo que sabe lo escribe él; lo que sólo se sabe al ejecutar, lo pregunta
a la VM.

| en C# | en JC |
|---|---|
| `i.GetType() != typeof(int)`, un valor, una cadena | `false`: su tipo no puede ser otro |
| `v.GetType() == typeof(X)`, `X` sin hijas | `v is X` |
| `v.GetType() == typeof(A)`, con una hija `B` | `(v is A && !(v is B))`: `is` también dice sí con una hija, y `GetType`, no |
| `o.GetType() == typeof(int)`, un `object` | `o is Integer` |
| `a.GetType() == b.GetType()`, dos que dependen de lo que llevan dentro | `CsSystem.Type_Misma(a, b)`: el nombre de la clase, que sabe la VM (`__className`, el builtin 301, de H9) |
| `typeof(X).IsAbstract`, `.IsSealed`, `.IsClass`… | lo que dice su declaración |
| `.Name`, `.FullName`, imprimirlo, con el tipo sabido | el nombre, como texto, con el de .NET (`X+A`, `N.A`, `System.Int32`) |
| lo mismo, cuando depende | `Cs_TipoDe(x)`: una función que escribe el traductor, una cadena de `is` de las hijas a los padres, y las cajas |

`GetType` lanza si el objeto es `null`: `CsSystem.Type_Hay(x)` delante, salvo que no pueda serlo. Lo que calcula, una vez, en una
temporal (`Sacar.cs`). **El límite** (hablado con Eduardo): una caja no guarda todos los tipos de C#: un `uint` va en un `Long`, como un
`long`, y el tipo se pierde; eso no lo sabe nadie. Si el programa compara tipos de `Object` y mete uno de ésos en una caja, no se
traduce, y lo dice: mejor eso que una respuesta equivocada. Lo que escribí el 2-nov, «BP no guarda el nombre de la clase», ya no es
cierto desde H9. Sigue sin traducirse un `Type` en una variable (`Type t = x.GetType();`) o en un array.

De paso: una local con un `typeof` que nadie usa no se escribe, como la que no tiene valor; y una constante que cabe en un tipo
estrecho y es una cuenta (`byte b = (int) 0x30;`, `test-129`), con su valor: JC sólo la acepta escrita tal cual.

Casos `Tipos` y `Constantes` (y `RechazosJc`, la caja compartida). **Mono: 516** (+21), sin perder ninguna; fallan las mismas 6.
**El tema 1: 113; el tema 2: 90.** De las 22 de la propuesta, entran 20 (y `test-733`); quedan `test-296` y `test-528`, las que
guardan el tipo en una variable.

### El tema 3, «propiedades» (4-nov)

Eduardo: *«Mientras puedes mirar las pruebas de Mono que pasan del grupo 3»*. 44 de 83, fallan 2 y 37 no se traducen. Cuatro propuestas
por lo que da cada una; Eduardo: *«1,2 y 4 OK»*.

| # | ajuste | dónde | tema 3 | hecho |
|---|---|---|---|---|
| 1 | ocultar un miembro (`new`): el de la hija lleva otro nombre en JC | el traductor | +7 | 4-nov: Mono **558** |
| 2 | una propiedad de sólo lectura asignada en su constructor: escribe su campo | el traductor | +1 | 4-nov: Mono **560** |
| 3 | un array de objetos de una clase con su tamaño (`new Derived[0]`) | JC y BP | +1 | 5-nov: Mono **563** |
| 4 | `Console.Error` | `CsSystem` | +1 | 4-nov: Mono **560** |

**1. Ocultar un miembro, hecho** (`Explicitas.cs`). En JC todo método es virtual, y lo que se llama igual en una hija sobrescribe. El
que oculta (con `new`, o sin él, con el aviso de C#) lleva otro nombre, `Hija_M`: métodos, propiedades, campos, constantes. Qué miembro usa
cada sitio lo dice Roslyn, así que `b.M()` con `b` del padre va al del padre, y con `b` de la hija, al suyo; un `override` de uno nuevo
hereda su nombre, y lo que implementa una interfaz, le reenvía (el miembro de la interfaz cambia de nombre, como en las explícitas). Sólo
con padres del programa: ocultar algo de .NET sigue sin traducirse. De paso, dos propiedades que sólo se diferencian en la primera
letra (`p` y `P`, `test-56`): en JC, las dos, `getP`; la segunda, con otro nombre. Y `test-502`: un método `get_Value` no oculta el
accesor de una propiedad. Caso `Ocultos`. **Mono: 558** (+23), sin perder ninguna; fallan las mismas 6. **El tema 3: 52.**

**2. La propiedad de sólo lectura, hecho.** En C#, dar valor en el constructor a `public virtual int Y { get; }` escribe su campo, sin pasar
por nadie, y leerla va por su `get`, que es virtual. En JC, con `{ get; set; }`, iría al setter y una hija que la redefine se lo
llevaba. Las que redefine una hija se escriben con su campo a la vista (`Y_campo`): `Y = e` lo escribe, y `Y++` y `Y += e` leen por el
`get` y escriben el campo. Caso `SoloLectura`. `gtest-autoproperty-12` deja de fallar.

**4. `Console.Error`, hecho.** BP no tiene salida de error: lo que se escribe se calcula y no va a ningún sitio; en C# tampoco va al `stdout`,
que es lo que se compara. **Mono: 560** (+2); fallan 5. **El tema 3: 54.**

**3. Un array con su tamaño, hecho** (5-nov; Eduardo: *«Las decisiones 1) Si, en BP no se var a:miObjeto[] := miObjeto[10]»*). Probado en JC:
`newObjArray(n)` da un `Object[]` y no se asigna a un `A[]`, `(A[]) x` no se puede escribir, y sólo se creaba uno con sus valores,
`[new A(), new A()]`. Ahora, `A[n]` en BP y `new A[n]` en JC, como en Java y C#, de cualquier tipo: una clase, una interfaz, un enum, `string`
y los números; las casillas nacen a cero o a null.

- **El árbol es el de indexar**, porque BP no los distingue al leer. Los distingue el análisis, por lo que es el nombre de la izquierda
  (`info.arraysNuevos`). Un nombre con valor (una variable, un parámetro, un campo) gana y se indexa, como gana en `A(…)`. En JC no se mira:
  tras `new` sólo va un tipo, sin él es un error que dice cómo se escribe, y `new A[3][2]` también (un array de arrays no existe).
- **Las palabras de tipo de BP** (`integer`, `string`, `byte`…) entran en una expresión seguidas de `[`: si no, JC no se podría escribir en BP.
- **La VM y el `.mod` no cambian:** sale lo mismo que el principio de un literal de array, `NEWARRAY…` o el builtin `NEW_REF_ARRAY`; un tamaño
  negativo o que no cabe da el error de siempre. Los `.mod` de 734 fuentes, iguales. También en una `native` (`NatSwitch`, 17/17).
- **El traductor** escribe así todos sus `new T[n]`, y su tabla de funciones de Core se fue. Sigue rechazando dos: un array de arrays, y uno de
  `struct`, cuyas casillas en C# nacen con su valor y aquí serían null.

Sample `ArrayNuevo` y `ArrayNuevoMal`, `ArrayNuevoTest`, caso `ArraysNuevos`. **Mono: 563** (+3, las previstas; a las otras 21 de las 24 que lo
pedían les falta algo más: genéricos, `struct`, LINQ, reflexión…). **El tema 3: 55.**

### `ulong`, sólo en el traductor (5-nov)

Eduardo: *«A ver ulong. No soportamos operaciones con ulong. Lo que podemos es permitir declarar ulong, leer y escribir haciendo conversiones de
long <-> ulong»*; y con lo medido, *«OK, hacemos 1 y 2, solo en el traductor»*, *«las 2 primeras filas»*. JC sigue sin tener `ulong`: para el
traductor es un `long` con sus 64 bits tal cual (`ulong.MaxValue` es `-1L`), como el `uint` es un `long` con su valor.

- **Sale sin nada** lo que da los mismos bits en un `long`, también de 2^63 para arriba y dando la vuelta: declararlo, pasarlo, leerlo y
  escribirlo; `+`, `-`, `*`, `&`, `|`, `^`, `~`, `<<`, `++`, `--`, `==`, `!=` y un `switch`; y las conversiones a los enteros, con sus bits de
  abajo. Medido antes: un `long` da la vuelta sin error en las dos VMs, y `<<` usa 6 bits de la cuenta, como C#.
- **No se traduce**, con su motivo, lo que de 2^63 para arriba daría otro resultado en silencio: comparar, `/`, `%`, `>>`, escribirlo, pasarlo a un
  real o a una caja, `ulong?`, y lo de .NET que lo recibe (`Math.Max(u, v)`, `u.ToString()`). Lo que C# ya calcula (`ulong.MaxValue > 5`) es una
  constante y sí va. Aquí se queda la tercera fila de lo medido: `test-146`, `test-441` y `test-729`, que sólo piden comparar.

Casos `ULong` y `RechazosULong`. **Mono: 570** (+7: `test-446`, `test-757`, `test-615`, `test-683` y `test-499`; y `test-binaryliteral` y
`test-cls-10`, que no había contado). `gtest-optional-36`, que sí conté, no pasa: lleva un `params` tras un valor por defecto, que JC no deja, y
hasta el ajuste 1 del tema 4 no se traduce. Las «unas 14» que dije antes contaban también `decimal` y `nint`, que iban en el mismo motivo; ahora
van aparte. **El tema 1: 119.**

### Los temas 1 y 2, completados (5-nov)

Eduardo: *«Del tema 1 queda algo mas que podamos arreglar de forma sencilla? Si no queda nada mas sencillo lo damos por completado con sus fallos
actuales. Del tema 2) lo mismo»*. Leídas una a una las que quedaban, tres eran sencillas, todas del traductor:

- **Un método `[Conditional]` sin definir no se escribe:** C# quita todas sus llamadas, y nadie llega a su cuerpo (`test-238`, y de paso `test-239` y `test-477`).
- **`checked (M) (x)` es la llamada:** sobre un nombre de método no cuenta nada (`test-665`).
- **Un `$"…"` que se pasa como `FormattableString` es su texto** (`test-interpolation-10`). Sus miembros (`.Format`) siguen sin traducirse.

Caso `Sueltas`. **Mono: 575.** **El tema 1, completado: 121 de 141. El tema 2, completado: 91 de 123.** Ninguna falla en los dos. Lo que queda:

| motivo | tema 1 | tema 2 |
|---|---|---|
| reflexión | 9 | 5 |
| genéricos | | 13 |
| tipos anónimos (por decidir) | 3 | 2 |
| `yield` | | 4 |
| lambdas, LINQ, delegados | 1 | 2 |
| hilos, `async`, fechas | 3 | |
| `decimal`, `nint` | 1 | 1 |
| interop | 1 | 1 |
| una sola cosa, que no es sencilla | 2 | 4 |

Las de una sola cosa: `test-74` (un `StreamReader` que lee su propio fuente), `test-interpolation-02` (los miembros de `FormattableString`), `gtest-512` (`(bool) o != null`, que
siempre da cierto, pero con un cast delante que calcula), `test-145` (un array de arrays, que BP no tiene), `test-441` (comparar un `ulong`: decidido) y `test-614` (`value++` bajo
`&&`: no se puede sacar a su sentencia, y haría falta una función por campo).

De las 13 de genéricos, alguna no usa sus clases genéricas (`gtest-partial-01` tiene el `Main` vacío). Pasarían quitando lo que nadie usa, pero sin probar nada: no se hace.

### `while` y `for` con nombre (5-nov; Eduardo: *«4) si»*)

Como `do`, con la misma regla y sin palabras nuevas: `break` y `continue` nombran a un bucle abierto, y `continue` sigue en su condición o en su paso. **En BP, tras el `do` del bucle,
solo en la línea:** `while c do miLoop`, `for i := 1 to 3 do miLoop`, `for x in lista do miLoop`; `while c do f()` sigue siendo la forma de una línea. **En JC, tras la palabra**, como `do miLoop`:
`while miLoop (c) { … }`, `for miLoop (…) { … }`, `foreach miLoop (…) { … }`. El `.mod` no cambia: los bucles sin nombre salen iguales. `BucleConNombre`, `BucleConNombreTest`, `AotCoberturaTest` y `NatSwitch`.

### Los temas 3 y 5, repasados (5-nov: propuestas, ya hechas)

Eduardo: *«revisa el tema 3, debería haber bastantes que no entran, hay que revisar a ver que se puede hacer. Y despues de todo empezamos a mirar el tema 5. El tema
4 lo miraremos despacio»*. Hechas de paso cuatro sueltas del traductor: una local con un null que nadie usa no se escribe (`test-624`), `System.Version` como tipo, sin sus miembros
(`test-447`), un enum a un tipo estrecho, con sus bits de abajo (`test-642`), y el `ToString` de un enum, su nombre (`test-68`). Caso `Sueltas`. **Mono: 579. El tema 3: 57 de 83. El
tema 5: 35 de 70.**

| # | ajuste | dónde | piden | sólo eso | tema 3 | tema 5 |
|---|---|---|---|---|---|---|
| A | `foreach` sobre una clase propia, y `ArrayList` y `Hashtable` | el traductor y `CsSystem` | 141 | 11 | 2 | |
| B | un enum que se opera, como constantes enteras | el traductor | 66 | unas 14 | | unas 13 |
| C | un inicializador de objeto fuera de una declaración, a una temporal | el traductor | 33 | 7 | 1 | |
| D | el constructor primario (C# 12) | el traductor | 6 | 2 | 1 | |
| E | un `struct` con estado, como una clase que se copia | el traductor | 180 | 46 | 2 | |
| F | los tipos anónimos (ya propuesto) | el traductor | 15 | 7 | 3 | |

- **A, hecho.** `foreach (T x in c)` con una clase del programa es `E e = c.GetEnumerator(); while (e.MoveNext()) { T x = e.Current; … }`, y el enumerador se suelta al salir si es
  `IDisposable`, o puede serlo una hija suya (`test-188`). También sobre una cadena. `IEnumerable` e `IEnumerator`, en `CsSystem`. **`ArrayList` es la `List` de Core** (Eduardo: *«ArrayList es
  nuestra List»*): `add`, `get`, `set`, `length` y el `foreach` de JC; en `CsSystem`, sólo lo que ella no tiene: el índice de `Add`, `Clear` y el enumerador. `Hashtable` e `IDictionary` no
  pueden ser el `Map`, que ordena sus llaves, y aquí una llave es cualquier objeto: una clase de `CsSystem`, sobre dos `List`, que busca una a una. Casos `ForeachPropio` y `Colecciones`.
  **Mono: 595** (+9). **El tema 3: 61.** Los 141 eran casi todos de `IEnumerable<T>`, que pide genéricos.
- **B, hecho, como dijo Eduardo** (*«no podemos dejarlos como enum y convertirlos a enteros?»*), que es mejor que lo propuesto (una clase de constantes). `#522` no cambia: JC sigue sin dejar
  convertir un entero en un enum. **El enum se queda como enum, y lo que se opera se convierte a entero:** sus variables son `int` (`long`, si su tipo de debajo no cabe) y un miembro es
  `((int) Permiso.Leer)`, que vale en un `|` y en un `case`: sólo se convierte de enum a entero, nunca al revés. Se hace con los enums del programa que se operan (`A.x | A.y`, `~e`, `e + 1`, `e++`,
  `(E) n`, `E e = 0`); los demás, como hoy. Un enum con valores que uno de JC no lleva (negativos, o que no caben en un int: «el valor de un enum es un entero escrito, sin signo», punto 25) no se escribe, y
  sus miembros van con su número. Su nombre en una cadena, con una función del enum que escribe el traductor, `Cs_Nombre_E`, también el de un `[Flags]`. Y dos sobrecargas que en JC serían la
  misma (`Test (int)` y `Test (E)`; también `f (long)` y `f (uint)`): Roslyn dice a cuál va cada llamada, y la segunda lleva otro nombre. `EnumsEnteros.cs`; caso `EnumsEnteros`. **Mono: 609**
  (+14: más de las «unas 14» que dije de sólo eso, no menos). **El tema 5: 49.**
- **F, no.** Eduardo: *«no se lo que son tipos anónimos ni para que sirven, por mi no»*. Siguen fuera.
- **C, hecho** (Eduardo: *«1) si»*: C, D, A y B, por ese orden). `new T () { A = 1 }` en una expresión: el objeto a una temporal y sus asignaciones, en orden, delante de la sentencia,
  y en su sitio la temporal; así `f = new Foo () { P = f.P }` lee el `f` de antes. No bajo `&&`, `||`, `??` o `?:`, ni en el valor de un campo, que no tiene sentencia. Caso
  `InicializaFuera`. **Mono: 584** (+5). **El tema 3: 58.**
- **D, hecho.** `class D (string arg) : Base (arg)`: un constructor con esos parámetros, que llama a la base y da los valores que los miran, en orden; `class X ()`, el implícito. Un
  parámetro usado en un método (C# lo guarda en un campo escondido), no. Caso `Primario`. **Mono: 586** (+2). **El tema 3: 59.**
- **E.** Es lo grande: un `struct` es un valor. Como clase hay que copiarlo al asignarlo y al pasarlo, darle valor al declararlo y llenar sus arrays. Es su propio tema.
- **Lo que no se puede:** en el tema 3, 13 son de reflexión, `dynamic`, interop, fechas o `yield`, y `gtest-autoproperty-20` falla por una diferencia decidida (`#776`); en el tema 5, 17 son de
  reflexión (`typeof`, `Enum.GetValues`, atributos), y otras piden `decimal` o `checked`.

### El rumbo de H8, desde 609 (5-nov)

Eduardo: *«600 Test de Mono pasados es un gran logro, el doble con que empezamos. Al principio aparecieron algunos bugs ahora casi no hay, lo que es muy buen síntoma. Algunas cosas que no entran
de C# me resisto a incorporarlas porque hacen al lenguaje mas complejo y no creo que aporten tanto en el ámbito en el que estamos de micro-controladores. Lo que tenemos que ver es a partir de ahora que
incorporamos y si realmente suponen una mejora. No se trata de hacer JC un C# sino de hacer JC mejor y mas probado.»*

La cuenta de Mono deja de ser el objetivo: es un instrumento. Los datos le dan la razón: de la batería han salido doce fallos reales de BP, de JC o de las VMs (`#744`, `#751`, `#752`, `#753`, `#755`, `#760`, `#761`,
`#772`, `#777`, `#789`, `#790` y `#791`), y desde 447 hasta 609, sólo los tres últimos. Lo que queda, con el lote de hoy («piden» / a las que no les falta nada más):

| motivo | piden | sólo eso |
|---|---|---|
| genéricos propios | 987 | 281 |
| lambdas (fuera, decidido) | 569 | 137 |
| delegados y eventos de C# | 538 | 53 |
| reflexión | 243 | 27 |
| `struct` | 229 | 21 |
| `yield` | 136 | 19 |
| `ref` y `out` | 121 | 13 |
| sobrecarga de operadores | 120 | 29 |
| `?.` y patrones | 108 | 13 |
| indexadores | 103 | 20 |

### Los genéricos: una orden al compilador (5-nov, `#793`)

Eduardo: *«A ver, los genéricos yo lo haría así. List y Map se declaran igual que ahora, con Object. Otras clases nuevas se pueden declarar con un tipo o con una interface. Por ejemplo SortList se podría
declarar con un IComparable. Ahora, a la hora de utilizar la Lista, declaramos var a:List<miTipo> . <miTipo> es una instrucción para el compilador, para que sepa que tipo de objetos guarda. Entonces los
métodos add y get no hacen falta sobreescribirlos ( ni List ) pero el compilador restringe el tipo. Es como una orden semántica para el compilador, pero no afecta nada a la VM ni al código generado.»*

**De dónde viene.** Le propuse los de Java, por pasos: los contenedores, las clases propias con una variable `T` (borrado de tipos, restricciones, métodos puente) y los métodos genéricos. Lo suyo es más
simple: la clase no lleva `T` y se compila como hoy. Desaparecen la variable, la sustitución dentro de las clases, los métodos puente y las restricciones (el tipo base ya lo es), y los métodos genéricos no
hacen falta.

**La regla.** En `C<X>`, donde la firma de `C` dice su tipo base, el compilador lee `X`: lo que entra tiene que ser un `X` y lo que sale es un `X`.

**Lo medido** (lote a28):
- Las 281 pruebas de «sólo eso» engañan: 109 tienen el `Main` vacío (`gtest-215` son tres declaraciones) y sólo unas 67 comprueban algo (`Main` con contenido, y escriben, lanzan o devuelven un error; por
  patrones sobre el fuente). Son pruebas del compilador de Mono: miran si acepta una declaración. La razón de los genéricos es lo que gana quien programa, no la cuenta.
- 1024 números en una `List` gastan 28 272 bytes en la VM-C: unos 28 cada uno (la caja, 12; la referencia, 8; el handle, unos 8). En un array, 4 096.
- Sin el cast de salida (quitado de un `.mod` de prueba, en las dos VMs, lo mismo): con un `Pequeno` de un campo donde se espera un `Grande` de cuatro, `GET_FIELD` lee el campo del objeto de al lado sin error,
  `SET_FIELD` escribe encima de ese objeto sin error, y el fallo sale después, en el vecino («el receptor no es un objeto»). Con el cast, para en la línea: «conversion invalida: el valor no es un 'Grande'».
- Las firmas: en `List`, `SyncList` y `OwnerList`, cada `Object` es el elemento (`add`, `get`, `set`, `pop`, `popBlocking`). En `Map`, no: unas veces es la llave y otras el valor.
- Entre módulos (su pregunta: *«como va a saber el compilador el tipo del cast?»*): el cast va en el módulo que lee, no en `List`, y ése sabe el tipo por la sección INTERFACE del `.mod` del que declara, que hoy
  pierde los argumentos (`func mete(l:Core.List):void public`). La clave de un método no los lleva, así que el enlace no cambia. Para usar un `Sensor` hay que importar su módulo, como hoy («el tipo 'Cc.Sensor'
  no tiene miembros»). La lista en sí no sabe nada: `<Sensor>` va en las declaraciones, no en el objeto; por un `Object` o una `List` a secas se pierde, y quien la recibe lo declara otra vez.

**Lo decidido** (*«OK a las 4»*):
1. El cast de salida se queda: el compilador escribe el `CHECKCAST` que hoy se escribe a mano. El compilador ve lo que entra por `add`, pero no lo que hace una clase por dentro, donde todo es `Object`, ni lo que
   llega dando la vuelta por un `Object`. Lo que se genera es lo mismo que con el cast a mano, y la VM no cambia.
2. Una clase nueva dice su tipo base con una anotación, `@element{IComparable}`: es justo una orden para el compilador. Sin marca, `Thread<Sensor>` compilaría sin significar nada. `class SortList<IComparable>`
   no: en Java y C# eso declara un parámetro llamado así.
3. `List<Perro>` no pasa a `List<Animal>` (como en Java y C#: por esa variable se metería un `Gato` entre los perros). Y la `List` a secas se mezcla con `List<Sensor>`, como los raw types de Java, para que no
   cambie ningún programa.
4. `#792` se arregla lo primero (hecho).

De su diseño sale además que entre `<` y `>` va una clase, una interfaz o `string`, como en Java: `List<Integer>`. `List<integer>` obligaría a cambiar `get` por `getInteger`, y eso ya no es sólo una orden.

**El algoritmo** (5-nov; Eduardo: *«OK, adelante»*; hecho el mismo día):
1. La marca. `@element{B}` en una clase dice su tipo base, y la heredan sus hijas. `List` y sus hijas lo tienen sin marca: `Object`. `Map`, aparte. Se lee antes de resolver las firmas (hoy las anotaciones se
   validan después del cuerpo).
2. El tipo. `C<X>` vale si `C` tiene tipo base y `X` es una clase, una interfaz o `string` que desciende de él. Si no, error: `Thread<Sensor>`, `List<integer>`, `SortList<Motor>`. Al resolver un tipo ya están la
   jerarquía y las interfaces, así que se comprueba ahí.
3. Una llamada `r.m(args)` con `r` de tipo `C<X>`: se mira la clase que declara `m` y su tipo base `B`. Un parámetro que dice `B` tiene que recibir un `X`. Un parámetro primitivo, si otra sobrecarga de `m` tiene
   `B` en ese sitio, tiene que recibir algo cuya caja sea un `X`. Si devuelve `B`, la expresión es de tipo `X` y queda anotada para el cast. Lo mismo con las `property`.
4. `for x in l`: `x` es un `X`, con su cast en cada vuelta.
5. `Map<K, V>`: `put` recibe `V`; `get` y `valueAt` devuelven `V`. La llave, como hoy. `keyAt` no se toca.
6. Asignar: `C<A>` a `C<B>` pide los argumentos iguales. Si un lado no los lleva, vale.
7. Emitir: una expresión anotada emite el `CHECKCAST` de `X` con el mismo código que el cast a mano. Si `X` es `B` no emite nada: `List<Object>` sale como hoy. En una `native`, el mismo error que el cast a mano
   (`#757`: un cast no se traduce ahí).
8. Entre módulos: la interfaz escribe `Core.List<Cc.Sensor>` y, en la clase, `element <tipo>`, con `bpi 11` sólo en quien lo use: los demás, byte a byte como hoy. Al importar se lee y se resuelve.
9. `List<List<Sensor>>`. En JC, `>>` es un desplazamiento y al cerrar un tipo se parte en dos. En BP ya son dos `>`: su desplazamiento es `shr`.

**Lo que no cubre:** dos tipos en una clase propia (`Par<A, B>`: sólo `Map` tiene dos), una clase que fija el tipo al heredar (`class Sensores extends List<Sensor>`) y un parámetro del tipo base que no sea el
elemento (en una clase marcada también se restringe). Para un micro no parecen hacer falta. A las pruebas que los usan se les puede dar servicio desde el traductor, que conoce los tipos y escribe los casts.

**Lo que salió al hacerlo:**
- `new List<Sensor>()`, en JC, da el tipo con sus argumentos, comprobados como los de una declaración: antes se ignoraban.
- Le dije que si `Aa` pasa de `List<Sensor>` a `List<Motor>`, un `Bd` sin recompilar se rechazaba al cargar. **No es así en MOD9**, medido con proyecto (`gen` 7) y sin él: las dos VMs avisan («compilado contra otra interfaz …;
  enlace tardio») y cargan, y lo para el cast, «conversion invalida: el valor no es un 'Sensor'». Otra razón para que se quede.
- `Genericos.mod` crece 14 bytes: su `List<Object>` público ahora se escribe en la interfaz (`bpi 11`). Los otros 737 `.mod`, iguales.

### `lista[i]`: una property con índice (5-nov, `#794`)

Eduardo: *«Lo de lista[i] me parece bien, en realidad es la notación de una propiedad ( en C# las propiedades pueden tener índice )»*.

**Lo que hay.** `x[i]` sólo vale en un array (*«el tipo 'List' no es indexable»*). Una `property p` son dos métodos, `getP()` y `setP(v)`. Una con índice y sin nombre serían `get(i)` y `set(i, v)`, y ésos ya los tiene `List`. C# hace
lo mismo: `this[int i]` se compila como `get_Item(i)` y `set_Item(i, v)`, una por clase y sin nombre, sobrecargable por el tipo del índice. Con nombre las tienen VB y Delphi; Java no tiene.

**Lo decidido** (Eduardo: *«a ver get normal no tiene índice, get(i) sería una sobrecarga y no veo el problema que necesite que se cambie de nombre»*; *«OK»*). `x[i]` es `x.get(i)`, y `x[i] := v`, `x.set(i, v)`, en cualquier clase que los
tenga: nada nuevo que declarar, y nada que tocar en Core, como `for x in lista`. En un `Map`, `m["a"] := v` es `put`. Y con nombre, por el mismo camino: `x.pin[i]` es `x.getPin(i)`, y `x.pin[i] := v`, `x.setPin(i, v)`. Una property `pin` puede
convivir con `getPin(i)`: son sobrecargas, y el compilador las rechazaba sólo porque miraba el nombre. Yo había propuesto que `pin` devolviera un objeto con `get` y `set`: costaba una clase y RAM por cada colección. No hace falta `this[…]`.

**Un matiz que salió al implementarlo:** dentro de la clase, si hay un campo o una property `pin`, `pin[i]` es indexar ese miembro. Si no, en una clase con un campo `pin` y un `getPin(i)`, patrón normal, su cuerpo `this.pin[i]` se llamaría a sí mismo.
Desde fuera, `x.pin[i]` es el método. Si `pin` no es un miembro, también dentro, y sin `this`.

**Lo medido:**
- De las 103 pruebas que piden indexadores, 99 declaran el suyo: con índice `int`, 67; `string`, 20; sobrecargados, 18; en una interfaz, 23; `x[i] += 1`, 14; con dos índices, 14. A 20 no les falta nada más, y 11 de ellas comprueban algo.
  El traductor puede escribir su `this[…]` como `get` y `set`: JC no necesita esa forma para pasarlas.
- `l[i]` parece un array y es una llamada. 3 millones de lecturas, con su bucle, en la VM-C del PC: un array, 83 ms; `get`, 169; `get` con su cast, 185.

**El algoritmo:**
1. `x[i]` con `x` de una clase: se construye la llamada `x.get(i)`, sin tocar el árbol que leen los escritores, y se analiza como cualquier llamada: sobrecargas, visibilidad, la llave de un Map y el `C<X>` de `#793`.
2. `x[i] := v`: lo mismo con `x.set(i, v)`, o `put` en un Map.
3. `x[i] += e`: el receptor y el índice, una vez, con los locales ocultos de `#746`; luego `get`, la cuenta y `set`.
4. Sin `get`: «el tipo 'Tabla' no es indexable». Sin `set`, se lee y no se escribe.
5. Una `native`: lo mismo que la llamada escrita a mano.
6. No cambian ni los opcodes, ni el formato del `.mod`, ni la interfaz, ni Core.

**Lo que no cubre:** `x[i, j]` (el traductor escribe `x.get(i, j)`) y `s[i]` en una cadena, que sigue sin valer.

**El traductor.** `T this[…] { get {…} set {…} }` pasa a ser los dos métodos, también en una interfaz, y lo de la `ArrayList` y la `Hashtable` se escribe `x[i]`. Rechaza lo que no cabe: un explícito, con opcionales o `params`, con un miembro llamado
`get` o `set`, y el `new` que oculta al del padre (`test-147`). Mono, de 609 a 627.

**Lo que salió de las pruebas:** `#795`, `List.get` y `set` miraban la capacidad y no el tamaño (`l.get(3)` en una lista de 2 daba `null`); y `#796`, de `test-834`: la visibilidad de un método sobrecargado se miraba en la primera sobrecarga, no en la
elegida, y `a.t(1)` llamaba desde fuera a un `private`. Las dos, hechas (*«1) si 2) si»*). Y uno mío, en el compuesto: un `get` privado no está en la vtable (`test-655`, `test-590`).

### Los genéricos en el traductor (5-nov)

Eduardo: *«¿Por que el traductor no utiliza genéricos? Ahora ya no hay nada que lo impida»*. Sólo el traductor (`cs2jc/Genericos.cs`): JC no cambia, salvo `#797`, que salió de aquí. Se borra `T` y se escribe lo que hay:

| en C# | en JC |
|---|---|
| `class Caja<T>`, `class Medida<T> where T : IValor` | `@element{ Object } class Caja`, `@element{ IValor } class Medida`, con el tipo base donde dice `T` |
| `Caja<Sensor>`, `Caja<int>`, `List<int>` | `Caja<Sensor>`, `Caja<Integer>`, `List<Integer>` |
| `Par<A, B>`, `T Igual<T> (T x)` | la clase a secas y `Object`: no hay nada que escribir, y el cast va en cada uso |
| un número que entra en un `T` | en su caja: `new Integer(5)` |
| un número que sale de un `T` | `n.Saca().value()`; si JC no sabe el tipo, con su cast: `((Integer) p.Segundo).value()` |
| `(Caja<int>) o` | `(Caja) o`: una `Caja` a secas entra en una `Caja<Integer>` |

**Cuándo una clase genérica va a secas, sin `@element`.** JC lee X donde la firma dice el tipo base. Si ahí hay algo que en C# no es `T`, lo leería mal, y no se escribe `C<X>`: el cast va en cada uso, como con dos parámetros.
- Un `object` de verdad, o un `U` de un método genérico, que sin escribirlo es `Object` (`gtest-039`, `gtest-331`).
- `Test (int)` junto a `Test (T)`: a `C<X>.Test (3)`, JC le pide que 3 quepa en un X, como el `add(5)` de una lista (`gtest-237`).
- Un antepasado que guarda otra cosa: `class Mezcla<T> : Caja<int>`.

**Dos sobrecargas que sin `T` son la misma** (`F (IFoo<T>)` y `F<U> (IFoo<U>)`): como las de un enum que se opera, Roslyn dice a cuál va cada llamada y la segunda lleva otro nombre (`BuscaSobrecargasIguales`).

**Lo que no se traduce, con su motivo:**
- Lo que pide saber `T` al ejecutar: `new T()`, `default(T)`, `x is T`, `x is C<int>`, `T[]`, el `GetType` de una `C<X>`. Es lo que da el diseño: *«no afecta nada a la VM ni al código generado»*. 105 pruebas, y 35 sólo por eso. De ellas se podrían ganar algunas en
  el traductor, si valiera la pena: `T[]` dentro de su clase, como `Object[]` (ya funciona), unas 10; `default(T)`, mirando con qué se usa `T` en todo el programa, unas 6. `new T()` no.
- Lo estático de una clase genérica, porque en C# hay uno por cada `C<X>` (17).
- Dos clases con el mismo nombre y distintos parámetros: `C` y `C<T>` (20).
- Implementar o redefinir con otra firma lo que se declaró con `T`: el `int Get ()` de una `class X : I<int>`, frente al `Object Get ()` de la interfaz. En JC no es el mismo método, y haría falta uno puente (25 y 14).
- Un parámetro de tipo con varias restricciones (11), un array como `T` (en JC, un array no es un objeto), y un compuesto sobre lo que se declaró con `T`.
- Lo de la biblioteca de .NET: `IEnumerable<T>`, `Dictionary<K, V>`, `IComparable<T>`… A JC ya no le faltan los genéricos: lo que queda de ese motivo es de .NET, y así se dice (182, y 22 sólo por eso).

**Lo medido.** Mono, de 628 a **805**, sin perder ninguna. De las 177 nuevas, 30 se comprueban solas, 16 imprimen algo y 131 corren sin imprimir nada: traducirlas y compilarlas prueba algo, pero poco. Fallan 6: las 5 de antes y `gtest-535`, que no se puede arreglar aquí:
C# descarta la sobrecarga que no cumple su `where T : class`, y a JC, sin restricciones, un `null` le lleva a la más específica.

**Lo que salió:**
- `#797`: `(Caja) x` no convertía si el constructor de `Caja` aceptaba `x`. Eduardo: *«en JC los constructores van con new Caja(x)»*.
- `Cs_TipoDe` no reconocía una cadena en un `Object` y decía «System.Object», sin avisar; JC no escribe `o is string`, y se le pregunta a la VM (`Type_Clase`). Y tampoco ve las clases genéricas: si las hay, no se traduce.
- Leídos uno a uno los 25 fallos del primer lote, ninguno era de JC salvo `#797`: el traductor escribía lo que JC, con razón, no da por bueno.

### El tema 4, los parámetros (5-nov: propuestas; hechas el 7-nov)

27 de 130, ninguna falla; 44 piden algo de fuera (reflexión, `dynamic`, `ulong`…) y 59 no. Medido con el lote de hoy: «piden», las que lo piden en
todo el lote; «sólo eso», a las que no les falta nada más. Es una medida por motivo: leídas una a una, alguna se queda por el camino.

| # | ajuste | dónde | piden | sólo eso | del tema 4 |
|---|---|---|---|---|---|
| 1 | los valores por defecto, escritos en cada llamada, que es lo que hace C#: `null`, un enum, un `T?` | el traductor | 19 | 6 | 6 |
| 2 | un array de valores de otro tipo (`params object[]` con un 8 y una cadena): una función por tipo y tamaño, con `new T[n]` | el traductor | 20 | 3 | 3 |
| 3 | `ref` y `out`: la variable vive en una caja, y lo que se pasa es la caja | el traductor | 122 | 14 | 14 |
| 4 | argumentos con nombre que cambian el orden de lo que se calcula | el traductor | 5 | 2 | 2 |
| 5 | `(X) null`, para elegir una sobrecarga | el traductor | 3 | 2 | 1 |

Juntas: 1 y 2, +9; con 4 y 5, +13; con 3, +27. Ninguna toca JC.

- **1.** Hoy el valor por defecto va en la declaración, y JC sólo admite ahí un literal. Pero en C# no es de la función: lo pone el compilador en
  cada llamada, y Roslyn lo sabe. `foo()` se escribe `foo(Foo.None)`. Ojo: `gtest-optional-39` llama a `GetValues(null)` con dos sobrecargas, y
  con los dos argumentos escritos sigue sin decidirse: es del ajuste 5.
- **3.** Copiar el valor a la entrada y a la salida no vale: las pruebas llaman dentro de una condición (`if (Prueba(out x) && x == 0)`, `test-893`,
  `test-904`). Por eso la variable vive en una caja desde que se declara: `Ref_int x = new Ref_int();`, sus usos son `x.v`, y la llamada es
  `Prueba(x)`. Una clase por tipo, escrita por el traductor. En un primer paso, las variables y los parámetros; lo que se pasa de un campo o de
  una casilla (`ref a[i]`) y lo de C# 7 (`ref int r = ref x`, `test-ref-11`), aparte.

**La idea de Eduardo para `ref` y `out`** (6-nov; sin decidir ni programar, y cambia el ajuste 3): *«Aquí no separaremos un poco de C#. A ver pasar datos por referencia solamente tiene sentido con variable escalares, los objetos y arrays ya se pasan por referencia y los strings son inmutables así que no se pueden pasar. La diferencia que quiero hacer es que lo único que vamos autorizar es pasar propiedades, con lo cual pasaremos 'delegados' del setter y el getter.»*

- **Cómo la entiendo:** en `suma(ref c.valor, 5)`, lo que llega a `suma` no es un entero sino el objeto `c` con el getter y el setter de `valor`, y dentro `n := n + k` se compila como leer con el getter y escribir con el setter. `suma(ref x, 5)` con una variable, o `suma(ref a[i], 5)`, son un error de compilación.
- **Es lo contrario de C#**, que permite variables, campos y casillas y prohíbe justo las propiedades (CS0206). Y encaja con la VM: una referencia a una variable sería un puntero a una pila, que aquí no existe; una property es un objeto, con su handle, y dos métodos. La VM no ve nada nuevo: seguramente sin tocarla ni tocar el `.mod`, a comprobar.
- **`out`:** sólo el setter, así que leer antes de escribir es un error de compilación, y una property sin setter no se puede pasar. La palabra `out` no puede reservarse del todo: hay muestras con variables que se llaman así (`GpioLoop`, los `NB2`).
- **Censo, por texto y aproximado** (128 de las 2742 pruebas): de 244 parámetros declarados, 113 son escalares, 18 cadenas y 5 arrays, y los 108 restantes, clases, `struct`, enums o `T`. De 161 argumentos, 150 son un nombre suelto (una variable o un campo), 6 una casilla y 1 un miembro.
- **Decidido el 7-nov (H7.4; Eduardo: *«OK, adelante»*), tras afinarlo con él:**
  - **La palabra es `var`**, en el parámetro, como en Pascal: *«en vez de Ref y Out emplearemos la palabra reservada 'var' que en los parámetros será equivalente a 'ref'. 'out' no lo vamos a utilizar»*. `function suma(var n: integer, m: integer)`; en JC, `void suma(var int n, int m)`.
  - **En la llamada, `objeto::property`** (Eduardo): `suma(miObjeto::miProperty, 5)`. `::` ya significa en BP «la función, no el valor».
  - **Sin clases ni objetos nuevos** (Eduardo: *«estamos hablando de delegados, aunque no los nombremos como tales, se pasan las direcciones del getter y del setter, juntos en un long»*): quien llama pasa el objeto (una referencia, que ve el GC) y un `long` con las ranuras del getter y del setter; dentro, leer es `INVOKE_SLOT` con la del getter, y escribir, con la del setter. Es lo que guardan los eventos: el receptor y la ranura. *«si hace falta pasar el objeto se pasa»*. Sin tocar la VM.
  - **Las reglas:** a un `var` sólo se le pasa `objeto::property` u otro parámetro `var`, por su nombre; la property, con getter y setter, del mismo tipo, y legible y escribible desde quien llama; de cualquier tipo; el objeto se calcula una vez, al llamar; sin valor por defecto, sin sobrecargas que difieran sólo en `var` y sin `var` en una `native`. De momento, error: las properties de un módulo y las estáticas (no tienen objeto), y un objeto declarado como interfaz (en la llamada no se sabe su ranura).
  - **El traductor:** `ref` y `out` son `var`; una variable local que C# pasa por `ref` vive en una caja, una clase con una property `v`, y se pasa `caja::v`. Los campos y `ref a[i]`, después.
- **Hecho en el compilador (7-nov):** `var` en `Param` y en los dos parsers (`var n: integer`, `var int n`) y en los dos escritores; en la interfaz del módulo, `var nombre:tipo` (bpi 12, sólo si se usa) y la marca `&` en la clave canónica, para que un compilador que no lo sepa no enlace en vez de descuadrar la pila; en el semántico, `checkArgVar` y `refAProperty`, con un mensaje por regla; en el emisor, dos parámetros ocultos por cada `var` (`__vo_n`, el objeto, y `__vr_n`, el long), leer con `INVOKE_SLOT` y la ranura de abajo y escribir con la de arriba, y cuatro huecos en `argSlotCount`; el envoltorio de un método llamado desde otro módulo reenvía los dos. Lo probado: `samples/VarParam.bp` en las dos VMs (sumar, asignar, reenviar un var, leerlo, `this::valor`, una cadena, un long, la property de una hija, un getter y un setter escritos, que corren una vez cada uno en `n += m`, y un objeto que se crea al llamar), su ida y vuelta por JC, y `VarParamTest`. Con el compilador de antes y el nuevo, 693 de 748 fuentes dan los mismos `.mod`; la única nueva, `VarParam`.
- **Hecho en el traductor (7-nov, `cs2jc/Cajas.cs`):** `ref` y `out` son `var` (`in`, un parámetro más); las locales que se pasan así, en una caja desde que se declaran, una clase por tipo con su property `v` escrita al final del módulo (`CsRef_int`), sus usos `x.v` y la llamada `x::v`; un parámetro `ref` que se reenvía, tal cual. **Mono: de 810 a 827**, y fallan las mismas 6. En el primer lote fallaron cinco más, y cada una salió con su arreglo: un `ref` en un constructor (`test-371` y el constructor primario de `test-primary-ctor-04`: JC no lleva `var` ahí) y dos métodos que sólo difieren en `ref` u `out`, también entre una clase y su padre (`test-798`: en JC, uno sobreescribiría al otro), se rechazan con su motivo; la caja de un `sbyte` recibe su valor convertido (`test-41`); y una clase llamada `main` choca con la función de entrada (`gtest-241`, que tapaba el `ref`). Lo que queda, con su motivo: un campo, una casilla, un parámetro por valor u `out var` (9), dos que sólo difieren en `ref` (5), un constructor (3), con nombre o `params` (3).
- **Dos cosas que señalé:** (1) con una cadena o un objeto, `ref` no cambia el texto ni el objeto: cambia a cuál apunta la variable del que llama (`out string error`, `ref Nodo raiz`). Tiene sentido, y con este mecanismo sale sin hacer nada: una property de tipo `string` tiene su setter igual. Recomiendo no limitarlo a los escalares. (2) Como casi todo lo que pasa C# son variables, en las pruebas lo salva el traductor con la caja del ajuste 3, que tendría una property `v`: se pasaría `ref caja.v`. Las dos ideas encajan.

**Los otros cuatro ajustes, hechos en el traductor (7-nov; Eduardo: *«OK, sigue con los 4 ajustes»*; `cs2jc/Argumentos.cs`).** Ninguno toca JC:
- **1, los valores por defecto:** los que JC no admite en la declaración (`null`, un enum, un `T?` con valor) y los de un método con un `params` detrás de uno se escriben en cada llamada, como hace C#, y la declaración se queda sin ninguno: `foo()` es `foo(Foo.None)`. Un `null` va con su tipo si el método tiene sobrecargas; un `T?`, en su caja (`new Integer(5)`); un `params` que falta, `[]`. También en la llamada a la base que C# hace sola.
- **5, `(X) null`:** una función que lo devuelve, `CsNull_X()`, escrita al final del módulo. La usan también los `null` del ajuste 1.
- **2, un array de valores de otro tipo:** una función por tipo y tamaño, que lo crea con `new T[n]` y llena sus casillas en orden: `CsArray_Animal_3(new Perro(), new Gato(), new Animal())`. Vale también para uno que sólo tiene `null`.
- **4, los argumentos con nombre que cambian el orden:** lo que calcula algo pasa a una temporal delante de la sentencia, en el orden escrito, como un `++` partido; lo que no cambia (un literal, una local, un parámetro) se queda en su sitio. No se hace bajo `&&`, `||`, `??` o `?:`, ni detrás de algo de la sentencia que se calcule antes.
- **Lo que salió por el camino:**
  - El `super()` implícito de JC y de BP sólo encuentra un constructor de la base sin parámetros, no uno que los tiene todos con valor (`#804`, para que lo decida Eduardo). El traductor escribe la llamada con los valores, `: base(2)`.
  - Con un `uint` o un `ulong`, que en JC son un `long`, JC elegía otra sobrecarga que C#: `gtest-optional-36` no se traducía, y en el primer lote falló. En un método con sobrecargas, un número que va a uno de ellos lleva su conversión escrita, como con los argumentos con nombre (`#783`).
  - El valor que una sobrescritura le pone a un `params` heredado (`int[] args = null`, `gtest-optional-32`) no se escribe: C# no lo usa.
- **Mono: de 827 a 843**, y fallan las mismas 6. **El tema 4: de 41 a 56 de 130.** Lo que queda del tema pide casi siempre otra cosa: un `ref` de un campo o de una casilla (6), un `++` dentro de una expresión (5), indexadores (5), delegados (5)…
- **Los casos del traductor:** `Llamadas`, nuevo. `Rechazos` y `RechazosJc` se pusieron al día; el primero se había quedado atrás con `ref`, que ya se traduce.

### Los temas 6, 7 y 8 (7-nov)

Con el lote de ese día (Mono, 805): casi todo lo que falta en los tres pide algo que está fuera por decisión.

| tema | válidas | funcionan | fallan | no se traducen | de ellas, con algo de fuera |
|---|---|---|---|---|---|
| 6, excepciones | 69 | 42 | 0 | 27 | 23 (async/await e hilos de .NET, `yield`, `dynamic`, reflexión, `checked`, `decimal`) |
| 7, herencia y virtuales | 48 | 38 | 2 | 8 | 6 (reflexión, `dynamic`, interop, el entero a enum de `#522`) |
| 8, interfaces simples | 78 | 60 | 1 | 17 | 12 (reflexión, sobre todo) |

- **El 6:** lo único de fondo, que un error de la VM tenga su tipo (`catch (NullReferenceException)`): lo piden 14 en todo el lote. Eduardo: *«los runtime error se puede poner una excepción dentro como excepción específica. En Java ya lo hacen así»*: es **H7.5** (FICHAS). Con él y el filtro `when` en el traductor, entrarían dos del tema. **Hecho el 7-nov** (abajo, «H7.5»).
- **El 7:** `test-29` es una regla distinta a propósito (C# prefiere el método de la hija aunque convierta peor; JC, el más exacto, como Java). Los métodos no virtuales (`new` en C#; los piden 9, 4 sólo eso) irían contra el modelo de Java: no.
- **El 8, hecho** (Eduardo: *«OK 1) y 2)»*): **`==` y `!=` entre una clase y una antecesora suya** (`Object` incluido), la identidad, como en Java, en JC y en BP (`comparableForEquality`; `samples/IgualdadRefs.bp`, en la red; `IgualdadRefsTest`, que dos clases sin parentesco sigan sin compararse; con el compilador de antes y el nuevo, los mismos `.mod` en los 694 que ya compilaban); en el traductor, también un `T` de un genérico con una referencia, que en JC es un `Object`. Una cadena con un `Object` sigue fuera (`test-111`). Y **`ICloneable`** en `CsSystem`. **Mono: 810** (+5: `gtest-294`, `gtest-455`, `gtest-466`, `test-316` y `test-585`); fallan las mismas 6. Lo de implementar con un tipo concreto lo que la interfaz declara con `T` (25 lo piden, 12 sólo eso), con los genéricos.

### El tema 9, cadenas, caracteres y conversiones (7-nov; cerrado)

Con el lote de ese día (Mono, 844): 85 válidas, 40 funcionan, ninguna falla, y de las 45 que no se traducen, 41 piden
algo de fuera (la reflexión, 13; `yield`, 11; `decimal`, 7; `dynamic`, 5; los tipos anónimos, 3…). De las otras cuatro,
dos pedían la librería, y entraron (Eduardo: *«OK»*): **`bool.Parse`** (`gtest-419`) y **`string.Split`** (`test-88`),
en `CsSystem` (caso `SplitParse`). `Split` con un separador elige en .NET 10 `Split(char, StringSplitOptions)` (sin las
opciones; con ellas, todavía no), y con varios o ninguno, un `params ReadOnlySpan<char>`, que JC no tiene: los dos son
`CsSystem.String_Split(s, …)`, con los separadores en un array. Las otras dos, fuera: `test-111` (`==` entre una cadena
y un `Object`, como se dejó en el tema 8) y `test-330` (`IFormattable foo = -1`: el `Integer` de BP tendría que
implementar una interfaz de .NET). **El tema 9: 42 de 85; Mono, 846.**

### H6, H7 y H8, cerrados (7-nov)

Con el tema 11, los once temas de H8 están repasados (Eduardo, 6-nov: *«Cuando lleguemos a los grupos 10 u 11 lo
daremos por terminado, con la pruebas que pasen como buenas»*), y con H8 se cierran H6 y H7, que existían para él.
Eduardo: *«OK. Y con eso el lote de pruebas de C# pasan a ser pruebas de BP y JC. Nuestras pruebas son test de caja gris o blancas (pruebas que se hacen conociendo el código) mientras que las pruebas de C# son de caja negra»*.
- **H8:** 848 de las 2322 válidas funcionan, y fallan las mismas 6. Desde hoy, **la prueba de caja negra de BP y JC**:
  las 848 que funcionan; una que deje de funcionar es una regresión. Sigue fuera del repositorio (las pruebas son de
  Mono), y se pasa con `cs2jc --lote`, comparado con el lote anterior (`compara_lotes.py`).
- **H6, el traductor:** hecho, en `compat/csharp/cs2jc`, con 52 casos propios.
- **H7, las mejoras de JC:** de la H7.1 a la H7.5. Lo que quedaba de la lista de `#745` no pasa a V9 como compromiso.
  Eduardo: *«A V9 no nos comprometemos a hacer nada del lenguaje. Cuando estemos en V9 ya estudiaremos lo que es razonable migrar a JC. Hay muchas cosas de C# que no quiero migrar, hacen al lenguaje complicado y en mi opinión aportan poco»*. Queda como lista para estudiar entonces: los delegados, los `struct` con estado, los
  operadores propios, los indexadores, `?.`, las tuplas, los patrones, los arrays de varias dimensiones y los de
  arrays, el `++` dentro de una expresión.
- **Lo que salió y sigue abierto en V8**, para los pendientes: `#804` (el `super()` implícito y los valores por
  defecto), `#805` (tres caminos de error en los que las VMs difieren), `#806` (`throw` de una excepción null) y `#800`
  (la prueba de `test-cola`, que se simplifica ahora: cerrada esa tarde, con 1 run y con 2).

### El tema 11, los arrays (7-nov; cerrado)

Con el lote de ese día (Mono, 846): 42 válidas, 13 funcionan, una falla (`test-673`, una de las 6 de siempre: mira las
mitades UTF-16 de un carácter de fuera del plano básico, y una cadena de BP es UTF-8) y 28 no se traducen; de ellas, 14
piden algo de fuera. De las otras 14, a V9 los arrays de varias dimensiones (11, 6 sólo eso), los arrays de arrays (3) y
`fc = stack[sp++] = cc` (`test-527`, con el `++` dentro de una expresión). Hechas en el traductor (Eduardo: *«OK, la 1
es correcta»*; caso `ForeachConvierte`):
- **un `foreach` con la variable de otro tipo que los elementos** (`gtest-140`): C# convierte cada elemento; en JC, el
  `foreach` con el tipo de los elementos y, dentro, la conversión: de una caja a otra, `Long x =
  CsSystem.Nullable_ALong(e);` (en JC, `Long x = e;` y `(Long) e` no compilan: `Integer` y `Long` no tienen parentesco,
  como en Java; que JC lo acepte sería un cambio del lenguaje, para V9); a una clase hija, `(Perro) e`; a un número más
  ancho, `(long) e`;
- **`ReferenceEquals(a, b)` y el `Equals` de un array** (`test-934`): el mismo objeto, `a == b`, con referencias que JC
  compare (un array, sólo con otro de su tipo).

**El tema 11: 15 de 42; Mono, 848.**

### El tema 10, los métodos de extensión (7-nov; cerrado sin cambios)

Con el lote de ese día (Mono, 846): 36 válidas, 16 funcionan, ninguna falla, y 20 no se traducen. Lo que es del tema
ya se traduce (la llamada estática que son: `x.Doble(a)` es `Ext.Doble(x, a)`). De las 20, 8 piden algo de fuera (LINQ,
la reflexión, saber `T` al ejecutar, las lambdas), y las otras 12, alguna de las mejoras grandes que quedaron para V9:
los delegados (6), las colecciones genéricas de .NET, `IEnumerable<T>` y las demás (6), y un `struct` con estado (1);
a las que sólo les falta una, `gtest-exmethod-47` (los delegados) y `gtest-exmethod-38` (el `struct`). Eduardo: *«OK,
cierra el tema 10»*. **El tema 10: 16 de 36.**

### H7.5, los errores de la VM con su tipo (7-nov; hecho)

Eduardo: *«A ver los runtime error se puede poner una excepción dentro como excepción específica. En Java ya lo hacen
así. Habría que añadir algunas excepciones y modificar la VM. No se, podemos analizar que no sea costoso para las VMs»*.
Decidido en tres pasos: las clases (*«OK, 1) 2) y 3)»*), el algoritmo (*«OK, adelante»*) y, con el inventario de los
sitios de las dos VMs, lo que quedaba (*«1) a, 2) ok, 3) ok»*).

- **Las clases**, en `Core`, hijas de `RuntimeError` (`catch e: RuntimeError` las sigue atrapando todas; el mensaje, el
  de siempre): `ArithmeticError` (división o módulo enteros por cero, exponente negativo, un número que no cabe:
  `byte(300)`, `factorial(13)`, y `remap` con el rango vacío), `NullReferenceError` (usar null, también un elemento de
  un array null, y un objeto eliminado), `IndexError` (un índice o un rango fuera: arrays, `charAt`, `List`, `move`),
  `CastError` e `IOError` (los ficheros). `EncodingError`, hija de `IOError`, como en Java (3). Lo demás —pines, buses,
  red, heap lleno, pila— sigue siendo `RuntimeError`. Lo dudoso, como en Java (2).
- **Las VMs:** la misma tabla (`claseDeError`, `bpvm_clase_de_error`) y cada sitio con su clase: 71 en miVM, y en la
  VM-C, las macros del intérprete con su clase (`BPVM_RT_THROW_DE`) y cuatro ayudantes en los builtins. Ningún opcode
  nuevo, ni nada en el camino normal: la clase se busca por su nombre al lanzar. Con un `Core` de antes, `RuntimeError`.
- **El builtin 308, `__throwError(tipo, msg)`:** lo usan el compilador (el `this` null de un método privado, `#752`) y
  `Core` (`List`). **En una `native`** (1, la a): el C del AOT lo llama por `call_builtin`, que ya estaba (la división
  de 32 bits, la conversión estrecha y el `this` null); lo que lanzan los helpers de la VM lleva su clase al límite
  (`bpvm_aot_throw_tipo`). Sin tocar la ABI del `.mdn`.
- **Lo probado:** `samples/ErroresConTipo.bp` (23 comprobaciones: la suya, aunque antes vaya otra; `RuntimeError`;
  `EncodingError` por `IOError`) y `bpgenvm-c/samples/NatErrores.bp` en la red de paridad; `make test-aoterrores`, las
  natives con el AOT y sin él, la misma clase y el mismo texto (10/10); `test-aotprivado` mira ya la llamada nueva.
- **El traductor:** `NullReferenceException`, `IndexOutOfRangeException`, `DivideByZeroException`,
  `ArithmeticException`, `OverflowException` e `InvalidCastException` son sus clases de Core, con el mensaje de .NET si
  se crean sin él; y `catch … when (c)`, el catch con `if (!(c)) throw e;` delante, si ningún catch de detrás la
  atraparía (en C#, la que el filtro deja pasar la mira ése). `throw null` se rechaza (`#806`). **Mono, 844**; el
  tema 6, 43 de 69.
- **Lo que salió:** `#805` (tres caminos de error en los que las dos VMs ya diferían: el texto de un fallo de `mkdir` y
  compañía, escribir una cadena eliminada, un `Mutex` con un id que no vale) y `#806` (`throw` de una excepción null).

## E10 · Las interfaces de clase, en JC y en BP (Eduardo, 31-oct; `#756`; el hito H7.3, decidido entero el 31-oct; HECHO el 1-nov)

Eduardo (31-oct): *«El problema de la interfaz, quizás deberiamos añadir una interfaz a JC. En los módulos ya estamos
guardando la interfaz de las clases así que no deberia costar mucho incluirla en el compilador.»*; *«Tenemos pendiente
en análisis de la Interfaz en JC.»*; y *«Yo creo que una clase y una interfaz se guardan igual o casi igual. Con un id
que indique que una es una clase y otro id que indique que es una interfaz debería valer»*. Aquí, el análisis y el
algoritmo, para su OK. Nada programado.

### Lo que ya estaba decidido, y lo que ha cambiado

- **El 6-jun, descartadas** (`V2_BACKLOG.md` §5, Eduardo): *«No se harán interfaces de clase. El polimorfismo por
  herencia simple (L2 v3) cubre el caso OO habitual»*; para un contrato, una clase concreta con el método esperado. El
  30-ago se retiraron también las de módulo (`manual.html` §10.4: *«Ni para clases —que sería lo defendible— ni para
  módulos»*; su código sigue, sin uso). Y en `#745` (31-oct): *«A priori, herencia múltiples y expresiones landa no
  entrarían»*: el traductor hace de una interfaz sola la clase base, y lo demás es «fuera: interfaces (herencia
  múltiple)».
- **Lo que ha cambiado desde junio** (lo que pide revisarlo):
  1. **JC** (H1, 18-oct): su razón de ser es el código de Java y de C#, y ahí las interfaces están en todas partes. La
     herencia simple sigue cubriendo lo habitual de un programa de BP; lo de C#, no.
  2. **MOD9** (`#523`, sep.): el módulo ya guarda cada clase con sus métodos **por clave** (`area()d`), y el cargador ya
     construye las vtables buscando por clave. La tabla que pide una interfaz se construye igual: es lo que dice Eduardo.
  3. **Lo medido**: 47 pruebas de Mono las piden, y a 13 no les falta nada más.
- **Es herencia múltiple de tipo, no de estado ni de código** (Eduardo, 31-oct: *«Pero es posible que una clase
  implemente varias interfaces? Y eso no es herencia múltiple (al menos como la entiende Java)?»*; lo había escrito mal:
  «no es herencia múltiple»). Java distingue tres: la de **estado** (heredar campos de dos clases: no la permite, una
  clase extiende una sola), la de **código** (dos cuerpos para un mismo método: sólo desde Java 8, con los métodos
  `default` de las interfaces) y la de **tipo** (ser a la vez de varios tipos: sí, con varias interfaces). Lo que se
  propone es sólo la de tipo: una interfaz no trae campos ni cuerpos, así que no hay diamante de campos, ni dos
  vtables, ni ajuste de `this` (lo que `V2_BACKLOG.md` §6 desaconsejaba, con razón), ni dos cuerpos entre los que
  elegir. Es el modelo de Java y de C#: un padre y las interfaces que se quiera. Con lo que dijo Eduardo en `#745`
  (*«herencia múltiples … no entrarían»*) sólo encaja si aquello era la de clases: es la primera decisión.

### Qué sería una interfaz

- Un tipo con **métodos sin cuerpo** y **properties**, que puede **heredar de varias interfaces**. Sin campos, sin
  constructores, sin miembros estáticos, sin cuerpos por defecto (los de C# 8 y Java 8) y sin eventos.
- Una clase tiene **un padre**, como hoy, y **las interfaces que quiera**.
- **Implementarla** es tener, propio o heredado del padre, un método público con el mismo nombre y la misma firma
  —parámetros y retorno: la clave de MOD9— por cada método de la interfaz y de las que hereda. Si falta uno, error de
  compilación con su nombre. Una property pide sus dos accesores, `getX()t` y `setX(t)V` (en BP una property siempre
  los tiene).
- **Es un tipo, como una clase**: variables, parámetros, retornos, campos. Una clase que la implementa se le asigna sin
  escribir nada, y ella se asigna a `Object`; al revés, `(I) x` en JC e `I(x)` en BP, comprobado al ejecutar como hoy
  (si no, el RuntimeError que nombra el tipo); `x is I` y `x instanceof I`; `==` con cualquier referencia (la
  identidad); y los métodos de `Object` (`toString()`…) sobre cualquier interfaz.

En JC:

```
interface IForma
{
   function double area();
}

interface IPoligono : IForma
{
   function int lados();
}

class Cuadrado : Figura, IPoligono
{
   …
   public function double area() { return this.lado * this.lado; }
   public function int lados() { return 4; }
}

IForma f = new Cuadrado(2.0);
println(f.area());
if (f is IPoligono)
{
   println(((IPoligono) f).lados());
}
```

y en BP:

```
interface Forma
  function area(): double
end Forma

interface Poligono extends Forma
  function lados(): integer
end Poligono

class Cuadrado extends Figura implements Poligono
  …
  public function area(): double
    return this.lado * this.lado
  end area
  public function lados(): integer
    return 4
  end lados
end Cuadrado

var f: Forma := Cuadrado(2.0)
print f.area()
if f instanceof Poligono then
  print Poligono(f).lados()
endif
```

`interface`, `extends` e `implements` ya son palabras de los dos lenguajes (las de los módulos retirados): **ninguna
palabra nueva**.

### Cómo se guarda: como una clase, con su marca (lo que dice Eduardo)

En la subtabla `CLASES` (4.5) de MOD9 una interfaz es **una fila más**, con un bit nuevo en sus `flags` (el bit 2,
**interfaz**: el «id» de Eduardo; hoy se usan el 0, abierta, y el 1, pública). Lleva su nombre y sus métodos por clave,
sin código (`codeOff` = −1); no lleva campos ni eventos, y en lugar del padre, las interfaces de las que hereda. Cada
clase añade a su fila **las interfaces que declara** (`depIdx` y nombre, como su padre). La fila cambia de forma, así que
la subtabla sube a su **versión 2** (lleva la suya, `ver:u16`), **sólo en los módulos que usan interfaces**: los demás
siguen en la 1 y corren en el firmware de hoy. Uno con interfaces en un firmware anterior se rechaza al cargar,
nombrándolo (`'App': subtabla CLASES de version 2 (esperada 1)`): ruidoso, nada en silencio. La sección `interface` del
`.mod`, la que lee el compilador al importar, lleva dos directivas nuevas, `iface` e `implements`, que un lector
anterior salta (§4.5).

### Lo que no se guarda igual: la llamada

Con una clase, un método tiene **la misma ranura** de vtable en todas sus hijas: una hija añade detrás, y lo que
sobrescribe se queda en la ranura del padre. Por eso `obj.m()` lleva su ranura escrita (en MOD9, la que el cargador
pone en la ext-table). Con una interfaz no: dos clases sin parentesco la implementan cada una en su sitio.

```
class Cuadrado : Figura, IForma   → area, en la ranura 4 (detrás de lo de Figura)
class Circulo : IForma            → area, en la ranura 2
IForma f = …;  f.area()           → ¿la 4 o la 2? Depende del objeto, no del tipo de f
```

La pieza nueva es **una tabla por clase y por interfaz** («`IForma.area` → 4» en `Cuadrado`, «→ 2» en `Circulo`), que
el cargador construye **al cargar, buscando por clave**, como ya construye las vtables. Y tres instrucciones nuevas
(hay sitio desde `0xBF`):
- **llamar por una interfaz**: la clase del objeto, su tabla de esa interfaz (la busca en su lista, de una a tres
  entradas lo normal), la ranura, y desde ahí **lo mismo que una llamada virtual** (`bpvm_vt_resolver` /
  `resolverVirtual`, con su subida por los padres). Es lo que ya hace `INVOKE_SLOT` (`0xBE`, `#607`) con la ranura
  calculada al ejecutar;
- **`is` con una interfaz**: si está en la lista de la clase del objeto;
- **convertir a una interfaz**: lo mismo, y si no está, el RuntimeError de siempre, que nombra el tipo.

El `is` y la conversión a una clase siguen como hoy, por la cadena de padres.

### El algoritmo del cargador (el mismo en las dos VMs; tras el paso 2 de `MOD_FORMAT.md` §13.4)

- **2b. Las interfaces.** Registrarlas como las clases (si falta una, E1; si heredan en ciclo, E7) y, de cada una, su
  lista **aplanada** de claves: las de las interfaces de las que hereda, en el orden en que las declara, y después las
  suyas; una clave repetida, una vez. Su «descriptor» es un bloque en la arena, y su dirección, su identidad.
- **2c. Cada clase, el padre antes que la hija.** Su lista de interfaces es **la del padre, tal cual** (la hija conserva
  las ranuras de lo que sobrescribe, así que las tablas del padre le valen), más cada interfaz que declara **y las que
  ésa hereda**, si no estaban ya. De cada una nueva, su tabla: por cada clave, `buscarClave` en la clase (propio o
  heredado y no privado, la búsqueda de hoy) → la ranura. Si no está, **E11**, nuevo: `'App.Cuadrado' implementa
  'Lib.IForma' y no tiene 'area()d': recompila 'App'`. La lista va detrás de la vtable del descriptor que construye el
  cargador.
- **3. Las refs.** La interfaz, por `CLASE` (la dirección de su bloque). Un método de una interfaz, por una ref nueva
  (tipo 7), que el cargador resuelve **por clave** en la lista aplanada → su posición; la instrucción lleva las dos.
  Nada horneado entre módulos: si `Lib` añade un método a `IForma`, `App` sigue enlazando sin recompilar (y si una
  clase de `App` no lo tiene, E11). Y `SUBTIPO` (tipo 5) mira también las interfaces: «`S` sigue implementando `T`».

Una clase con interfaces **siempre la construye el cargador** (es abierta). En MOD9 casi todas lo son ya —toda clase sin
`extends` desciende de `Core.Object`, que es de otro módulo—; sólo cambia para las de `Core`, si un día implementan una.
Así la VM sabe qué clases llevan lista (un detalle de la implementación: por ejemplo, las que construye el cargador
viven en la arena; las horneadas no tendrían ninguna).

### El compilador

- **Los dos parsers**: `interface` dentro de un módulo; la cabecera de la clase, con su lista (JC: `class C : B, I, J`,
  la clase primero, como en C#; BP: `class C extends B implements I, J`).
- **El símbolo**: una interfaz es un `ClassSymbol` con su marca, sin campos y con sus interfaces padre.
- **Los tipos**: asignar (una clase a la interfaz que implementa, una interfaz a la que hereda y a `Object`), `is`,
  convertir, `==`; la comprobación de «implementa»; y las llamadas, resueltas contra la lista aplanada (las sobrecargas,
  también entre las interfaces padre).
- **El emisor**: las tres instrucciones, las refs, las filas de `CLASES` versión 2 y las directivas de la interfaz del
  módulo; y en el `.dbg`, la interfaz como tipo de una variable.
- **Los escritores BP ↔ JC** (`EscritorJc`, `EscritorBp`), el manual y la gramática; y el traductor, que deja de hacer
  de la interfaz una clase base.

### Lo demás

- **El AOT**: una native que llama por una interfaz, o convierte a una, o pregunta `is`: rechazada con su mensaje (como
  hoy `instanceof`). Hay que rechazarlo **a propósito**: el AOT ya llama a métodos virtuales por clave y, sin el
  rechazo, buscaría la clave en la interfaz como si fuera la clase. Más adelante, la llamada podría ir por el puente,
  como las virtuales.
- **El depurador**: el wire no cambia (enseña la clase del objeto); el IDE, al enseñar una variable de tipo interfaz,
  tiene que mirar la clase del objeto: a comprobar.
- **La memoria en la placa**: por clase con interfaces, 4 bytes, más 8 por interfaz y 2 por cada método de ella; por
  interfaz, unos 8 + 2·n. Diez clases con una o dos interfaces de cuatro métodos: unos 300 B de arena. El enlace, una
  búsqueda por clave por cada método de cada interfaz de cada clase, como las vtables. Y la llamada, la de una virtual
  más recorrer una lista corta.

### Lo que da

- **Mono**: de las 13 a las que sólo les faltan las interfaces, leídas una a una, entrarían 10 (`test-80`, `-134`,
  `-137`, `-197`, `-245`, `-372`, `-421`, `-667`, `-855` y `test-partial-05`: varias interfaces, interfaces que heredan
  de otras, properties en una interfaz, la implementación heredada del padre). `test-30` pide la implementación
  explícita (`void IA.Draw()`: dos interfaces con el mismo método y dos cuerpos); `test-457`, dos explícitas con el
  mismo nombre y distinta firma, que el traductor podría pasar como dos sobrecargas; y `test-746` junta una property y
  un método con el mismo nombre. De las otras 34, a 16 les faltan además los genéricos y a 8 los métodos no virtuales
  (`new`).
- **En BP**: el polimorfismo sin una base común (un `Sensor` que implementan un `Bme280` y un `Dht22` que ya heredan de
  otra clase), y el contrato con su tipo, en lugar de la clase-callback de junio.
- **Más adelante, en la stdlib** (cada una, su decisión): `Comparable` para ordenar, `Iterable` para el `foreach` sobre
  colecciones propias (lo piden 49 pruebas) y las interfaces genéricas, con los genéricos de H7.

### Lo que no entraría en el primer paso

La implementación explícita de C# (el traductor la sigue pasando como un método normal cuando no choca con otro, y
rechazándola cuando choca), los cuerpos por defecto, los miembros estáticos, las interfaces genéricas y el `catch` de
una interfaz (se atrapan clases que heredan de `Exception`).

### Lo que hay que decidir (Eduardo)

1. **¿Se reabre la decisión del 6-jun?** Y con ella, la de `#745`: si aquella «herencia múltiple» era la de clases
   (campos y código), las interfaces caben; si era cualquier tipo con varios supertipos, son justo lo que quedó fuera.
   Y si se reabre, ¿en JC y en BP? Propuesta: en los dos; es el mismo compilador, y una interfaz de un módulo JC tiene
   que poder implementarse desde uno BP.
2. **El primer paso**: métodos, properties, herencia entre interfaces y la implementación heredada del padre; fuera, lo
   del apartado anterior.
3. **La sintaxis**: la de arriba.
4. **`Thread` y sus hijas.** En BP un `Thread` no es un `Object` (es `root class`). Propuesta: que no puedan implementar
   interfaces, para que el valor de una interfaz sea siempre un `Object` (y `toString()` tenga qué despachar).
5. **Si una clase deja de tener un método de su interfaz** (otra versión de la librería): Java lo descubre al llamarlo
   (`AbstractMethodError`); MOD9 lo diría **al enlazar**, nombrándolo (E11). Propuesta: al enlazar, como el resto de
   §13.
6. **La compatibilidad**: la versión 2 de `CLASES`, sólo en los módulos que usan interfaces (arriba).

### Decidido (Eduardo, 31-oct)

*«A ver que yo vengo de programar sobre todo en Java así que si se pueden utilizar interfaces estoy encantado. Y si, la
herencia múltiple de tipos. En BP ( y JC ) no se puede exportar nada mas que métodos y propiedades así que para la
interfaces casi no hay restricciones.»* La 1 y la 2: **las interfaces, sí, en BP y en JC**, con la herencia múltiple de
tipos; y **métodos y properties**, que es lo que una clase exporta (desde otro módulo, quien usa un objeto sólo ve eso;
los campos sólo los ve una clase hija, que es el objeto: `manual.html` §9.6). Y después la 3, la 4 y la 6 (*«3 y 4 OK,
6 también OK»*): la sintaxis de arriba; `Thread` y sus hijas, sin interfaces; y la versión 2 de `CLASES`, sólo en los
módulos que las usan. **La 5, abierta** (*«Veamos 5. Tenemos un método declarado, si ese método está implementando un
método de una interfaz debería indicarse.»*): que el método que implementa uno de una interfaz lo diga.

### La 5: el método que implementa lo dice (Eduardo, 31-oct)

Con una anotación, como el `@Override` de Java (*«En java utilizan una directiva, el equivalente nuestro es @(...)»*),
pero **propia, aparte de la de redefinir** (*«Prefiero separar impements de override porque no es exactamente lo
mismo. Sería algo así @implement( intefaz1, intefaz 2)»*). BP y JC ya tienen anotaciones (V5/H5, las del ORM): el parser
las acepta delante de cualquier declaración y el semántico decide dónde valen; hoy, sólo `@BD`, y sobre una función se
rechaza. `implement` no es palabra reservada (`implements` sí), así que vale de nombre sin tocar el lexer.

```
BP:  class Cuadrado extends Figura implements Poligono
       @implement{ Poligono }
       public function area(): double
         return this.lado * this.lado
       end area
     end Cuadrado

JC:  @implement{ IPoligono }
     public function double area() { … }
```

**Decidido** (Eduardo, 31-oct: *«1 llaves, 2 una o varias de las que implementa la clase, 3 obligatoria si, 4 se puede
utilizar @override para sobreescribir métodos de la clase padre aunque no es obligatorio pero es mas claro y los que
vengan de Java lo podrian hechar de menos,a nosotros nos sale casi gratis.»*):
- **La forma**: la de las anotaciones de hoy, con llaves (`@BD{ tabla = "medidas" }`, la gramática de V5/H5).
- **Qué nombra**: una o varias de las interfaces que implementa la clase (las de su cabecera, las que éstas heredan y
  las de su padre), y cada una tiene que tener ese método. `class C implements A, B`, con `X()` en las dos:
  `@implement{ A }`, `@implement{ B }` o `@implement{ A, B }`; en `Cuadrado`, `area` puede decir `Forma` o `Poligono`.
- **Obligatoria** en un método declarado en la clase que implementa uno de una interfaz de su cabecera: si falta, error
  (`'area' implementa 'Poligono.area': márcalo con @implement{ … }`); y si nombra una interfaz que la clase no
  implementa, o que no tiene ese método, también.
- **Lo heredado no se marca**: en `class Y : X, A`, el `Add` lo pone `X`. Y una hija que sobrescribe un método que su
  padre ya implementaba, sin repetir la interfaz en su cabecera, está redefiniendo: `@implement` le vale, pero no le
  hace falta.
- **`@override{}`, para redefinir un método del padre**: opcional («es mas claro y los que vengan de Java lo podrian
  hechar de menos»); si la lleva y no redefine nada, error (la errata de Java). Lo escrito sin ella sigue valiendo.
- **Varias anotaciones por declaración**, como en Java (Eduardo: *«Si, varias notaciones»*; hoy el parser guarda una):
  una property de una entidad del ORM que implementa una interfaz lleva `@BD{ pk }` y `@implement{ IConId }`, y un
  método que implementa y redefine a la vez puede llevar `@implement` y `@override`. Un cambio pequeño en los dos
  parsers, en los escritores BP ↔ JC y en quien las lee (la validación del ORM y su generador de DAO).
- **En el módulo**: el bit «implementa» en la fila del método (el 2; hoy, el 0 es `override` y el 1, privado); al
  enlazar, E11 si la interfaz gana un método que la clase no tiene, y E12 si ninguna interfaz de la clase tiene ya el
  que la clase dice implementar. `@override` no llega al módulo: el bit `override` ya lo pone el compilador siempre.
- **El traductor de C#**: `@implement{ … }` donde Roslyn dice qué implementa cada método, y `@override{}` donde C#
  escribe `override`.

### El orden, si sale el OK

El de siempre: el compilador y la VM-Java, con los samples buenos y los de error (no implementa, ciclo, conversión
inválida); después, la VM-C, con la paridad, el caso de dos módulos (una interfaz en una librería) y los del enlace
tardío en `mezcla.py` (la librería añade un método a la interfaz → E11; una clase deja de implementarla → E10); el
traductor; la red; y la placa, en la próxima tanda.

### Hecho (1-nov), en ese orden

- **El compilador.** Los dos parsers leen `interface … extends …` y `implements`, y varias anotaciones por declaración
  (`Node.annotations`, una lista; `@BD` la lee igual). El semántico comprueba lo que lleva una interfaz (métodos sin
  cuerpo y properties cortas; lo demás, un error que dice qué sobra), que no haya ciclos (`interface A extends B`, `B
  extends A`), que la clase tenga cada miembro de sus interfaces —con la clave de MOD9, la misma que casará el
  cargador—, y las dos anotaciones. Una interfaz desciende de `Object`, así que `f.toString()` vale. En JC, `class C :
  A, B` no dice si `A` es la clase base o una interfaz: el parser lo marca y el semántico lo decide. Los escritores BP ↔
  JC y la comparación de árboles, con todo eso; y la sección de interfaz del `.mod`, `bpi 10` (`iface … end iface`,
  `implements`) sólo en los módulos con interfaces (los demás siguen en la 9).
- **El módulo** (`MOD_FORMAT.md` §13): la interfaz es una fila de CLASES con el bit 2, y la v2 de la subtabla añade a
  cada fila sus interfaces; el bit 2 de la fila del método es «implementa»; la ref `IFAZ_MET` (7) es la posición de una
  clave en la lista aplanada de la interfaz. Tres opcodes, `0xBF`–`0xC1` (`INVOKE_IFAZ_REF`, `INSTANCEOF_IFAZ_REF` y
  `CHECKCAST_IFAZ_REF`; `OPCODES.md`).
- **Las dos VMs**, el mismo algoritmo: al enlazar, cada clase con interfaces tiene en la arena su lista (interfaz →
  tabla) y cada tabla las ranuras de su vtable; una tabla aparte, ordenada, lleva de un descriptor a su lista, y se
  busca por bisección (§13.9). La clase comparte las tablas de su padre. E11, E12 y las variantes de E1 (la interfaz
  que ya no está, o que ahora es una clase), con el mismo texto en las dos.
- **El traductor** (`compat/csharp/cs2jc`): las interfaces de C# salen como interfaces de JC —ya no como clases base—,
  con `@implement{ … }` donde Roslyn dice qué implementa cada método y `@override{}` donde C# escribe `override`. Lo que
  no tiene JC lo dice: dos implementaciones explícitas con la misma firma (`ExplicitaDoble`), `new` de una interfaz COM.
- **Verificado:** el compilador nuevo y el de antes dan los mismos `.mod` en los 663 samples que compilan (los 39 que no
  compilan, en los dos igual) y en los 31 de la stdlib: nada de lo que existía cambia. `Interfaces` e `InterfacesUsa`
  (con su librería, `InterfacesLib`) en el corpus, con la paridad en las dos VMs y la vuelta por JC EQUIVALENTE;
  `InterfacesMal`, doce errores en `h13-errores.sh` (84 de 84); `mezcla.py`, el bloque F (la librería cambia sus
  interfaces sin recompilar el programa: E11, E12, E1 y `SUBTIPO`), 203 de 203; los casos del traductor, 9 de 9; y
  Mono, de 275 a **290 que funcionan**, y las mismas 4 que fallan (de las 13 a las que sólo les faltaban las
  interfaces, 12 funcionan).
- **La red, la primera vez, en ROJO**, y con razón: `test-tablasim` (`#639`, el enlace con el heap troceado: ningún
  bloque de más de 3000 B) decía «enlace tardío: sin memoria para construir las clases». El registro del enlace —una
  entrada por clase de todos los módulos, en UN bloque— había crecido con lo de las interfaces, también en los programas
  que no las usan: de 112 a 176 B por clase en el PC y de 72 a 108 en un micro de 32 bits (medido con el compilador de
  ARM); con las 25 clases que carga la prueba (`Json` y lo suyo), de 2 800 a 4 400 B, medido con una copia
  instrumentada del enlace de antes y del de ahora (ninguna otra reserva pasa de 3000 B). **Arreglado:** el registro,
  como antes (2 800 B, lo mismo que antes de H7.3), y lo de
  las interfaces en una tabla aparte (40 B por clase en el PC, 24 en el micro) que sólo se reserva si algún módulo trae
  interfaces; las de cada fila no se guardan, se leen de la fila. Un programa sin interfaces no paga nada, como la v2
  de `CLASES`. Y de paso, el arnés: en la pasada MOD8, la marca `// mod8: fuera` se mira antes de compilar, para que
  `Interfaces` salga con su razón y no como «no compila». La segunda red, VERDE (la paridad, 179); y `check-jc`, 175
  (los 4 SKIP de siempre).
- **Lo que no está** (como en Java 1.4): los métodos con cuerpo en una interfaz, las implementaciones explícitas de
  verdad (`I.M()` de C#: el traductor pasa una sola a un método normal, y dos con la misma firma no se traducen), las
  interfaces genéricas y las clases abstractas. Una native no llama por una interfaz (`AOT_LIMITES.md` §6). Y la placa,
  en la próxima tanda (H7.3 no cambia la stdlib).

### Hallazgos de paso (con ficha)

- `#757` — el AOT compila una conversión de referencia como si construyera un objeto: en una native,
  `var p: Perro := Perro(a)` (con `a` un `Animal`) llama a la factoría de `Perro` con `a` como argumento si el
  constructor pide un entero. Código equivocado sin un error; visto en el C que genera (una sonda, `CastNat`).
- `#758` — una ranura que no se resuelve al llamar (un `.mod` corrupto o un fallo del cargador; tras un enlace bueno,
  no pasa) para el programa en la VM-C y es un error atrapable en la VM-Java.

## E11 · Las colecciones, con interfaces: `Comparable` y `Collection` (las ideas de Eduardo, 2-nov; sin empezar)

> Eduardo, 2-nov (noche), ya con las interfaces de clase (H7.3, `#756`): *«Ideas: Una lista ordenada necesita que los
> objetos implementen comparable, nada mas. foreach debería admitir cualquier contenedor que implemente collection.»*
> Apuntadas para analizarlas cuando toque; nada hecho.

- **Una lista ordenada pide `Comparable`, y nada más.** Hoy `Comparable` es una CLASE de Core (`#446`, 18-ago: la base
  de los envoltorios `Integer`, `Long`, `Double`…, con `compareTo(other: Object): integer`, que devuelve 0 si no se
  sobrescribe), y el `Map` de `Collections` ordena sus claves con un objeto aparte, un `Comparator`
  (`StringComparator` por defecto, `NaturalComparator` para las que son `Comparable`). Con las interfaces, `Comparable`
  podría ser una interfaz, y una lista ordenada (o el `Map`) sólo pediría que sus elementos la implementen, como en
  Java (`Comparable<T>`) y en C# (`IComparable`). Eduardo, el mismo día: *«Comparable debería ser una interfaz que
  pueda implementar cualquier clase.»* El rumbo, decidido; lo que cambia (los envoltorios, que hoy heredan de la
  clase; el `NaturalComparator`, que la convierte; lo ya compilado contra la clase), por analizar.
- **`foreach` sobre cualquier contenedor que implemente `Collection`.** Hoy `for x in` recorre un array o una `List`, y
  nada más («se esperaba un array o una List en 'for in'»). Con una interfaz (en Java, `Iterable` y su `iterator()`;
  en C#, `IEnumerable` y su `GetEnumerator()`), una clase del programa también se podría recorrer. En el lote de Mono,
  «foreach sobre colecciones propias» lo piden 48 pruebas (2 sólo eso).
- **Las interfaces del lenguaje, juntas** (Eduardo, 7-nov): *«Podriamos tener un módulo de interfaces algo como Module BP y dentro solamente interfaces básicas del lenguaje como IComparable, etc.»* Y tras hablarlo: *«De momento si quieres en Core. A mi me gusta la Idea de Java donde separan las interfaces de las implementaciones, dejando abierto a que una interfaz la puedan implementar diferentes librerías. Pero quizás eso sea algo para mas adelante, no ahora en la V8.»* Lo propuesto: los nombres de Java, sin la `I` (`Comparable`, `Iterable` o `Collection`, `Cloneable`, `Closeable`), y el traductor convierte los de C# (`IComparable` → `Comparable`, `ICloneable` → `Cloneable`); hoy `ICloneable`, `IDisposable` e `IEnumerable` viven en la biblioteca del traductor, `CsSystem`. **En V8, si hace falta, en `Core`; el módulo aparte, con las interfaces separadas de las implementaciones, para después de V8.**
- **Por analizar:** lo que pide cada una al lenguaje (el `for … in` con una interfaz; si `Comparable` pasa de clase a
  interfaz, lo que cambia para los envoltorios y para lo ya compilado), a la stdlib (`List`, `Map`) y a las dos VMs.

## E12 · Los literales grandes, como *resources* que se cargan al heap (la idea de Eduardo, 3-nov; sin empezar)

> Eduardo, 3-nov, con `#768` (el bloque de datos de un módulo, de 32 a 64 KB con el desplazamiento como un *word*):
> *«Los literales grandes deberían ser resources, que se cargan al heap.»*

- **Por qué:** el bloque de datos de un `.mod` (sus literales distintos, sus globales, sus constantes) está en la
  memoria del programa desde que se carga, y tiene un tope (64 KB desde `#768`). Un texto grande —una página web para
  el servidor de la WiFi, un JSON, una tabla— no necesita estar ahí: puede ser un fichero aparte que el programa carga
  en el heap cuando lo usa, y que se libera cuando ya no.
- **Lo que ya hay parecido:** la marca `// recurso:` de los samples (un fichero que va con el programa, para el arnés),
  y `readFile` (que lo lee al heap). Lo que faltaría: que el compilador o el IDE lleven el *resource* con la aplicación
  (al workdir del micro, como los módulos de la app) y una forma de nombrarlo desde el código.
- **Por analizar** cuando toque: cómo se declara, cómo viaja con la app al micro y cómo se lee.

## E13 · Lanzar un programa desde otro: cargar módulos en marcha, o un relevo (Eduardo, 4-nov; `#786`; decidida la A, con la clase `Overlay`; es el hito H9, hecho en el PC: al final de esta sección)

> Eduardo, 4-nov: *«Si lo tuviésemos implementado podríamos hacer una programa genérico al que le podemos pasar como
> argumento el módulo/pack a ejecutar y el se encargaría que ejecutarlo. Y si lo hiciémos dentro de un bloque try ...
> catch exception atraparíamos cualquier error. Lo único que veo un poco delicado sería liberar los módulos que ya no
> se usan pero tampoco debería ser demasiado problema.»* Y después: *«Haz el análisis a ver que tal.»*

### Para qué se quiere

Cuatro usos, que no piden lo mismo:

1. **Un lanzador** (4-nov): un programa genérico que corre otro, ve cómo acabó y sigue.
2. **Una opción «Configurar el Bluetooth»** que lance el programa de configuración y, al terminar, vuelva (14-oct, E1).
3. **Partir un programa grande en pedazos** y aprovechar mejor la memoria (`#595`; `V2_BACKLOG` §17).
4. **Un complemento**: un programa que sigue vivo, carga un módulo, llama a sus funciones y comparte objetos con él.
   Nadie lo ha pedido; lo pongo porque es el único de los cuatro que sólo se resuelve con la opción A.

Los tres primeros son el mismo gesto: **corre aquél y, cuando acabe, sigo yo**. Entre los dos programas pasa poco: un
argumento de ida y, de vuelta, cómo acabó. No comparten objetos.

### Cómo está hecho hoy (leído en el código)

- **La memoria:** `[ módulos ][ heap → ][ tabla de handles ][ pila del main ][ pilas ]`. Los módulos van uno detrás de
  otro y el heap empieza donde acaba el último (`loader.c`, `heap_start = next_free_address`); el enlace tardío pone
  su arena en ese mismo sitio y mueve el heap (`link.c`). Con el programa en marcha, ahí ya hay objetos: un módulo nuevo
  no cabe donde hoy van todos.
- **Un literal no es un objeto del heap:** es una dirección en los datos de su módulo, sin cabecera y sin handle. Seis
  sitios (`interp.c`, `builtins.c`, `bpvm_internal.h`, y sus espejos en miVM) distinguen un literal de una cadena del
  heap mirando si la dirección queda por debajo de `heap_start`.
- **Un objeto apunta a su clase**, y la clase vive en su módulo.
- **El enlace es de una vez:** `bpvm_link_all` recorre todos los módulos, antes de que corra nada.
- **Un programa no se puede «llamar»:** `__startup` llama a `__init` y a `Main` y acaba en `HALT`, que termina la VM (y
  en un hilo que no sea el main, es un error). Y BP no tiene `exit`: un programa acaba cuando acaba su `Main`.
- **Hay cosas que se resuelven una vez por RUN** y no se vuelven a mirar: los despachadores de la GUI (`#486`), el
  registro de las natives (`bpvm_aot_clear`), y el cursor de las cadenas guarda la dirección de un literal, que hoy no
  cambia nunca (`bpvm_strcur_t`).
- **Tope de 16 módulos** (`BPVM_MAX_MODULES`), y hoy los cuentan el programa y la stdlib que importa.
- **Acabar un programa es lo más probado que hay:** `bpvm_destroy` lo suelta todo, y cada prueba de la red pasa por ahí.

### Opción A: cargar y descargar en marcha (la de `V2_BACKLOG` §17)

```basic
module Lanzador
  import Core
  import Machine

  function Main(arg: string)
    var m: integer := Machine.loadModule(arg)
    try
      Machine.runModule(m, "")
    catch e: Exception
      print "falló: " + e.msg
    endtry
    Machine.unloadModule(m)
  end Main
end Lanzador
```

**Lo que hace falta, en las dos VMs:**

1. **Un sitio para el módulo.** Un bloque clavado del heap, como las pilas de los hilos (`#740`), o encima de la tabla
   de handles. En los dos casos el literal deja de estar por debajo de `heap_start`: hay que rehacer aquellos seis
   sitios, que están en el camino rápido de las cadenas. Con el bloque clavado, además, la docena de sitios que
   preguntan si una dirección está en el heap: un literal de ese módulo diría que sí.
2. **Cargar y enlazar en marcha:** el cargador y `bpvm_link_all`, módulo a módulo; leer el fichero por `io`; el mundo
   parado mientras cambian `modules[]` y los símbolos; y subir el tope de 16, o un lanzador con `Gui` y `Json` se queda
   sin sitio para el programa.
3. **Los `.mdn` y las natives de los packs**, cargados y soltados con el programa en marcha.
4. **Entrar y volver:** `__startup` acaba en `HALT`. O cambia lo que emite el compilador, o `HALT` pasa a significar
   «vuelve» dentro de un programa lanzado. (`__init` no da problema: ya es idempotente, y lo que ya se inició no se
   repite. El argumento tampoco: `__runArg`.)
5. **Descargar.** Con las condiciones de `Heap.compact` (sólo desde `Main` y sin otros hilos vivos), que resuelven de
   golpe los marcos de la pila y los hilos:
   1. quitar sus globales de las raíces;
   2. una colecta, y correr los destructores con su código aún en memoria;
   3. recorrer lo vivo: ni un objeto de sus clases ni una palabra que apunte a su zona, en el heap, en las pilas o en
      las globales de quien se queda; y ni un manejador de la GUI ni un evento suyos;
   4. si queda algo, un error con nombre; si no, devolver la memoria, los símbolos y los `.mdn`, y dar por caducado lo
      resuelto (`str_epoca`, la caché de `cs→cb`, los despachadores de la GUI).
6. **El depurador:** un `.dbg` que aparece con el programa en marcha, las trazas de `#582` y el IDE.

El tamaño: el del enlace tardío (`#523`). Y toca lo que más nos ha costado dejar fiable: el heap, el GC, el cargador.

**Lo delicado es liberar, como dice Eduardo, y es más de lo que parece.** Cinco casos concretos:

- **La excepción.** El programa lanzado falla con un `Demo.ErrorDeSensor`. El lanzador lo tiene en `e`, y `e` sigue en
  su pila cuando llama a `unloadModule`, que encuentra un objeto vivo de una clase que se va. Tiene arreglo: en la
  frontera, convertirla en una `RuntimeError` con su texto, para que no cruce ninguna clase.
- **El literal.** El programa lanzado llama a `Ajustes.setNombre("Demo")` en un módulo que el lanzador también usa. Esa
  global se queda con la dirección de un literal del módulo que se va. Los handles aquí no protegen, porque un literal
  no tiene: el paso 3 lo encontraría y se negaría a descargar.
- **El hilo.** El programa arranca un hilo y falla ahí. La regla del manual: la excepción acaba *su* hilo y los demás
  siguen, así que el `try` del lanzador no se entera.
- **El bucle.** `while true` o dos hilos que se esperan: el `try` no vuelve nunca.
- **El hueco.** Tras varias vueltas, el programa siguiente necesita un bloque seguido que el heap puede no tener.

**Y la consecuencia:** si la descarga se niega, el lanzador no tiene salida. Su razón de ser es sobrevivir al programa
lanzado, y lo que decide si se puede descargar es lo que ese programa dejó. Lo único seguro en ese punto es terminar
la VM y volver a empezar, que es la opción B. **A necesita a B detrás; B no necesita a A.**

### Opción B: el relevo

El programa no carga a otro: **pide que, cuando él acabe, el sistema arranque aquél**, y que al terminar vuelva a
arrancar a quien lo pidió, diciéndole cómo acabó. Cada programa corre en una VM limpia, y entre uno y otro actúa
`bpvm_destroy`. Sigue habiendo un programa cada vez.

```basic
module Lanzador
  import Core
  import Machine

  function Main(arg: string)
    if Machine.lastProgram() == "" then
      Machine.nextProgram(arg, "")     // al acabar yo, ése; y al acabar ése, otra vez yo
    else
      print Machine.lastProgram() + " acabó: " + Machine.lastStatus()
      print Machine.lastMessage()
    endif
  end Main
end Lanzador
```

(Los nombres son de ejemplo.) Si le pasamos `/app/Demo.mod` y `Demo` divide por cero, pasa esto: `Lanzador` apunta la
petición y acaba; la VM se destruye; `Demo` arranca en una VM nueva y falla; la VM se destruye; `Lanzador` arranca otra
vez, desde `Main`, y lee `RUNTIME_ERROR` y `División por cero`.

**Lo que hace falta:**

1. **En `Machine`, una función que apunta la petición** y las que leen cómo acabó el anterior: el estado, el código de
   salida, el mensaje y la traza de `#582`. Apuntar y nada más: no hay una forma nueva de acabar un programa.
2. **El RUN común** (`bpvm_repl.c`, el mismo para todas las familias): tras `bpvm_destroy`, si hay petición, otra vuelta
   con la ruta nueva. La misma sesión para el IDE: un `RUN_REPLY`, un `EXITED` al final y la salida seguida. Vale para
   un `.mod` y para un pack, porque es `bpvm_load_entry`.
3. **Lo mismo en los otros sitios que arrancan un programa:** el CLI de la VM-C, `Main.java` y `DebugServer`.
4. **Menos de 1 KB fuera de la VM**, porque tiene que sobrevivir a `bpvm_destroy`: las dos rutas, el argumento y el
   resultado. Un nivel: quien pide es a quien se vuelve.
5. **Las reglas:** un KILL o un RESET cortan la cadena, porque el `Stop` del IDE quiere decir parar. Un programa que no
   existe o no enlaza vuelve al lanzador con ese error. Con el depurador, se depura el primero.
6. **Si se quiere, un tiempo máximo:** el RUN común pide el KILL a los N ms, y vuelve con «tiempo agotado». Es lo que
   hace que el bucle sin fin también se atrape.
7. **La prueba en la red:** una cadena normal, un error, un error en un hilo, un programa que no enlaza y un KILL, con
   las dos VMs byte a byte.

No toca el heap, ni el GC, ni el cargador, ni el enlace, ni los literales, ni los `.mdn`, ni los packs: esos caminos
siguen como están y ya los prueba cada ejecución. El tamaño: el de la etapa 1 de `#582`.

**Lo que cuesta, medido el 4-nov** (`Corto` sólo importa `Core`; `Medio`, `Core`, `Collections`, `Json`, `Math` y
`Machine`; mediana de 9, del RUN enviado al EXITED recibido, cable incluido; el programa en sí, de 6 a 53 ms):

| Placa | `Corto` | `Medio` |
|---|---|---|
| Nucleo | 63 ms | 181 ms |
| Metro | 153 ms | 368 ms |
| S3 | 217 ms | 550 ms |
| P4 | 236 ms | 390 ms |

Una vuelta entera son dos arranques, el del programa y el del lanzador, y el del programa lo paga también la opción A:
**menos de un segundo en todas**. (La primera tanda salió redondeada a 0,2 s porque leí con `w.lines`, que espera ese
plazo; es el aviso de `#495`. Estos números son con `w.esperar`.)

**Lo que se pierde:**

- **Quien lanza vuelve a empezar desde `Main`.** Lo que tenía en memoria se va, y lo que quiera conservar lo escribe en
  un fichero o se lo pasa por el argumento. Para un menú no es nada; para la aplicación con «Configurar el Bluetooth»
  es volver a abrir su pantalla, y `lastProgram()` le dice que viene de ahí.
- **De vuelta sólo llega texto:** el resultado, y lo que se deje en un fichero. El argumento tiene 127 bytes de tope.
- **No tiene forma de `try … catch`:** es un `if` al principio de `Main`.
- **No hace complementos** (el uso 4).

**Y lo que da sin haberlo pedido:** con `/sys/auto.txt` apuntando al lanzador, una placa sola se vigila a sí misma. Hoy,
un programa que falla sin nadie delante se queda en el REPL; con el relevo, el lanzador lo apunta y lo vuelve a
arrancar, o arranca otro. Y en el uso 3 cada pedazo tiene toda la memoria, sin huecos.

### Opción C: otra VM dentro (descartada)

Una segunda `bpvm_t` en un bloque del heap del lanzador daría las dos cosas: aislamiento total y que el lanzador siga
vivo. Pero hay unas 200 variables de fichero en la VM-C (contadas con `grep`, a ojo), y entre ellas la cola de los hilos
(`bpvm_la_cola`, 55 usos en la VM), la GUI y el registro de las natives; la memoria habría que partirla por adelantado; la
stdlib iría dos veces en la RAM de las placas que no la corren desde la flash; y rompe *una app por VM*.

### Qué atrapa cada una

| Lo que pasa en el programa lanzado | A: `try … catch` | B: relevo |
|---|---|---|
| Un error en su hilo principal | sí | sí |
| No carga o no enlaza | sí | sí |
| Se queda sin memoria | sí | sí, y vuelve toda la memoria |
| Un error en un hilo suyo | **no** | sí |
| Un bucle sin fin, dos hilos que se esperan | **no** | con el tiempo máximo |
| El heap descarrilado (`#494`) | llega la excepción, pero el lanzador sigue en esa VM | sí: VM nueva |
| Deja algo vivo al acabar | **no se puede descargar** | no pasa |
| Un fallo del propio firmware | no | no, pero si la placa se reinicia, el autorun arranca el lanzador |

### Recomendación

**La opción B.** Da los usos 1, 2 y 3, atrapa más que el `try … catch`, cuesta una fracción y no toca nada de lo que
ya es fiable. Lo que no da es que quien lanza conserve su estado, y para un lanzador eso no pesa.

La opción A se queda escrita aquí para cuando aparezca un complemento de verdad (el uso 4). Si llega, dos cosas:
**una sola llamada** que cargue, corra y descargue (`Machine.runModule(ruta, arg)`), para que las cargas vayan en pila
por construcción y no se pueda olvidar la descarga; y **que descargue el GC, no el programa**, como un `ClassLoader` de
Java: el módulo vive mientras algo lo alcance. Así, lo que quede vivo retiene memoria en vez de ser un error.

### Lo que hay que decidir (Eduardo)

1. **¿B, A o las dos?** La recomendación, B.
2. Con B, **¿vale que quien lanza vuelva a empezar desde `Main`?** Es lo único que cambia respecto a lo que pedía.
3. **¿El tiempo máximo entra de primeras?** Son pocas líneas, y es lo que cubre el bucle sin fin.
4. **Los nombres** (`Machine.nextProgram`, `lastProgram`, `lastStatus`, `lastMessage`…).

### Decidido (Eduardo, 4-nov)

> *«1) La A. Si hay problemas lo vamos resolviendo poco a poco. 3) Si hay un bucle sin fin mala suerte, eso también puede
> pasar en un programa normal. Mira lo que yo haría es Crear una clase Overlay y ahí guardamos toda la información que
> haga falta. Un método 'Load' y otro 'Unload' . Y luego un método tryExe para ejecutar el 'main' del módulo cargado. Mas
> adelante podemos investigar una versión asíncrona que cree un hilo a parte.»*

- **La opción A**, con una clase `Overlay` que guarda lo que haga falta, y tres métodos: `load`, `tryExe` y `unload` (en
  minúsculas, por la convención de nombres).
- **Sin tiempo máximo:** un bucle sin fin es lo que le pasaría a un programa normal.
- **Más adelante:** una versión asíncrona, en un hilo aparte.

### El algoritmo (por aprobar)

```basic
module Lanzador
  import Core
  import Overlays

  function Main(arg: string)
    var ov: Overlays.Overlay := Overlays.Overlay()
    ov.load(arg)                     // si no se puede, RuntimeError
    if ov.tryExe("") then
      print "acabó bien"
    else
      print "falló: " + ov.getError()
    endif
    ov.unload()                      // si no se puede, RuntimeError
  end Main
end Lanzador
```

**Dónde va el módulo: se aparta el heap.** Esto mejora lo que puse arriba en «Lo que hace falta», punto 1. Ni un bloque
clavado del heap ni encima de la tabla: el módulo va donde van todos, detrás del último, y lo que se mueve son los
objetos.

```
antes       [ módulos ][ objetos ············· libre ············· ][ tabla ][ pilas ]
apartar     [ módulos ][ ············· libre ············· objetos ][ tabla ][ pilas ]
cargar      [ módulos ][ Demo, Json ][ ······· libre ······ objetos ][ tabla ][ pilas ]
devolver    [ módulos ][ Demo, Json ][ objetos ······ libre ······· ][ tabla ][ pilas ]
descargar   [ módulos ][ hueco libre ][ objetos ····· libre ······· ][ tabla ][ pilas ]
```

- **Apartar** es `Heap.compact` (`#595`), que deja todos los objetos seguidos, más un `memmove` de ese trozo hasta arriba
  y sumar lo mismo a cada handle. **Devolver** es lo contrario. Mover objetos ya está hecho y probado, y con las mismas
  condiciones.
- **El cargador no cambia:** con el heap apartado, ve lo mismo que al arrancar un programa, un heap vacío detrás del
  último módulo. Se le baja `heap_top` para que no pise lo apartado, y su «no cabe» de siempre vale.
- **Las cadenas no cambian:** el módulo sigue por debajo del heap, así que un literal sigue por debajo de `heap_start`.
  Los seis sitios del camino rápido y la docena que pregunta si una dirección está en el heap se quedan como están.
- **Al descargar no se mueve nada:** `heap_start` vuelve a bajar y lo que ocupaba el módulo es un bloque libre más.
- **Una sola carga a la vez**, de momento: una segunda `load` es un error. Así, descargar es deshacer lo último.

**`load(ruta)`**

Las condiciones, con un texto cada una, el mismo en las dos VMs:

- **llamado desde `Main`**, directamente: una expresión a medias en otro marco podría tener la dirección de un objeto, y
  los objetos se van a mover. `load` no devuelve nada, para que no pueda estar dentro de una expresión;
- **sin otros hilos vivos**, como `Heap.compact`;
- **sin ficheros abiertos:** su bloque de estado no se puede mover (`bpvm_files_clavar`);
- **sin otro overlay cargado.**

Y los pasos, con el mundo parado de principio a fin:

1. apuntar cómo está todo: cuántos módulos y símbolos hay, dónde acaba lo cargado, la tabla de interfaces, el pack en
   ejecución, el registro de las natives;
2. colecta y compactación; apartar;
3. `bpvm_load_entry(ruta)`, tal cual: un `.mod` o un pack, y las dependencias que no estén ya cargadas. Lo ya cargado se
   reutiliza: si `Demo` usa `Core`, es el `Core` del lanzador;
4. enlazar sólo lo nuevo:
   - imports y fixups, desde el primer módulo nuevo;
   - **el enlace tardío:** su registro de clases no se guarda, porque serían unos 80 B fijos de RAM por clase. Así que se
     rehace para las clases de siempre, sobre su arena, escribiendo lo mismo en el mismo sitio, y se comprueba que acaba
     en la dirección de entonces. Las clases nuevas van a una arena nueva, detrás de los módulos nuevos. En miVM el
     registro sigue vivo y sólo se añade;
   - la tabla de interfaces, nueva y con todas; la vieja se queda para la descarga;
5. los `.mdn` sueltos de los módulos nuevos, con el bucle de siempre;
6. devolver los objetos, y dar por caducado lo que se resuelve una vez por RUN (los despachadores de la GUI).

Si algo falla en 3, 4 o 5, se deshace todo lo apuntado en 1 y se lanza una `RuntimeError` con el motivo de siempre («falta
el módulo X», «recompila Y», «no cabe»). El programa sigue como estaba. Y un módulo con el mismo nombre que uno ya
cargado también es error, para que el símbolo no resuelva al que no es.

**`tryExe(arg)`**

Está escrito en BP, en la clase, con un `try … catch` de verdad. Devuelve `true` si acabó bien; si no, `false`, y el
error se queda en el `Overlay`:

```basic
public function tryExe(arg: string): boolean
  try
    __overlayExe(arg)             // vuelve cuando acaba el Main del programa cargado
    return true
  catch e: Exception
    this.fallo := __overlayCopia(e.msg)
    return false
  endtry
end tryExe
```

- **`__overlayExe`** pone `arg` donde lo lee `__runArg`, apila un marco como el de `CALL_EXT` y salta al `__startup` del
  módulo. Corre en el hilo de quien llama. `__init` inicia sólo lo nuevo, y después va `Main`.
- **`HALT` vuelve.** `__startup` acaba en `HALT`, el único sitio que lo emite. En un programa normal, ahí `bp` es el fondo
  de la pila; si no lo es, a `__startup` lo llamaron, y `HALT` hace lo que un `RET`. No hace falta estado nuevo, y un
  `.mod` ya compilado vale tal cual.
- **La excepción no sale del `Overlay`**, que es lo que resuelve el caso de arriba: el objeto muere con el marco de
  `tryExe`, y al lanzador sólo le llega texto. `__overlayCopia` copia al heap, porque `e.msg` puede ser un literal del
  módulo que se va.
- **Lo que queda guardado:** el mensaje, el nombre de la clase y la traza de `#582`, que se compone cuando la excepción
  sale del módulo. Se leen con `getError()`, `getErrorClass()` y `getTrace()`.

**`unload()`**

Las condiciones: que esté cargado, que no esté corriendo y que no haya otros hilos vivos. No hace falta estar en `Main`,
porque no se mueve ningún objeto. Y los pasos:

1. sus globales dejan de ser raíces;
2. colecta y `Core.__destruyePendientes()`, con su código aún en memoria, hasta que no quede nada pendiente: sus
   destructores corren como siempre (`#668`);
3. recorrer el heap: ni un objeto de una clase suya, ni un campo o un elemento de array que apunte a su zona (los bitmaps
   de las clases dicen cuáles son referencias, así que es exacto). Si queda algo, `RuntimeError` con el nombre: *«sigue
   vivo un objeto de Demo.Cosa»*. El overlay se queda cargado pero cerrado: no se puede ejecutar, y se puede volver a
   intentar la descarga;
4. soltar: la memoria de cada módulo, los símbolos, las natives, la tabla de interfaces, el pack en ejecución y la zona.
   Y dar por caducado lo que guarda direcciones: `str_epoca` (el cursor de las cadenas), la caché de `cs→cb`, los
   despachadores de la GUI y la marca por clase de los destructores. Si no, otra carga en la misma dirección
   heredaría datos del anterior.

**Cerrado el 7-nov** (H9). Eduardo: *«Si, cierralo. H9 necesita rodaje, que se utilice y que vayan apareciendo los problemas. Esto es muy útil en dispositivos con poca memoria. Por ejemplo en un programa, las opciones de configuración se utilizan pocas veces. Si se descarga a una librería aparte y que se carga solamente cuando hace falta, liberamos memoria. Lo mismo pasa con una opción de exportar o importar datos. De esta forma, en el programa dejamos solamente lo esencial que se utiliza habitualmente y nos ahorramos memoria. Esto era muy habitual en MS-DOS donde estaba la barrera de los 640K.»* Lo de abajo queda como límites conocidos (`PENDIENTES`, «Los
overlays»), para cuando el uso los pida.

Y otros usos, ese mismo día: *«Hay otros usos. Por ejemplo podemos hacer una línea de comandos que permita ejecutar otros programas. Y también podriamos hacer un programa extensible, con algún tipo de interface que permita intercambiar datos»*. La línea de comandos, con lo de hoy y sin
probar, sería un bucle en `Main` que lee la orden y hace `load`, `tryExe` con sus argumentos y `unload`. Intercambiar
datos con una interfaz no está hecho: hoy, de ida, un `string`, y de vuelta, si acabó bien o su error. Lo nuevo, a V9.

**Lo que se deja para después, a sabiendas:**

- **Un hilo que el programa deje vivo** impide descargar: el error lo dice, pero el lanzador no puede matarlo, sólo
  esperar. Un programa normal sigue hasta que acaban todos sus hilos (`#346`); `tryExe`, de momento, vuelve al acabar su
  `Main`.
- **Un error en un hilo suyo** no llega a `tryExe`, y el programa entero acabará con código 1 (`hilo_sin_atrapar`). Se
  puede recoger en `tryExe`.
- **Una global de un módulo que se queda** con un literal del que se va no se detecta: las globales no llevan bitmap, y
  en el bloque de datos hay de todo (los descriptores de las clases, por ejemplo), así que mirarlas a ojo daría falsos
  positivos. En la stdlib sólo hay una `string`, `Log.ruta` (`Log.bp:66`), pero el módulo puede ser del usuario: es el
  caso de abajo, «El ejemplo de Eduardo».
- **La GUI:** si el programa cargado la usó, sus widgets siguen en el modelo, y sus manejadores impedirán descargar. Si
  `Gui` vino con el overlay, habrá que hacer `bpvm_gui_reset`.
- **Los eventos** que queden en cola para sus objetos también impiden descargar.
- **El depurador:** no pone puntos de ruptura en un módulo cargado así, aunque las trazas de `#582` sí dicen su nombre.
- **`load` desde un manejador de la GUI:** no se puede. El manejador apunta qué quiere y `Main` lo hace al salir del bucle.
- **Dentro del programa cargado** no valen ni `Heap.compact` ni otro `load`: los dos se niegan solos, porque su `Main` no
  es el del fondo de la pila.
- **El tope de 16 módulos** cuenta los del lanzador más los del programa. Se dice al llegar.

**Las etapas:**

1. **El PC, en las dos VMs:** los tres métodos, con `.mod` y packs. La red, con un `overlay_smoke` y los sabotajes que
   hagan falta.
2. **Las placas:** la Metro, la P4 y la Nucleo. Lo que hay que mirar: la pila de C de la tarea `run` (16 KB), porque el
   cargador hoy corre en otra; el registro del enlace (`bpvm_calloc`) con el programa en marcha; y el `.mod` desde un pack
   en flash.
3. **Lo de la lista de arriba**, según vaya haciendo falta.

**Lo que tiene que probar la etapa 1**, byte a byte en las dos VMs:

- un programa que acaba bien, y otro que falla con un error de la VM, con un `throw` y con una clase de excepción propia y
  un mensaje literal;
- un programa con clases abiertas que extienden las de `Core` y las de un módulo que trae él, y con interfaces;
- que los objetos del lanzador sobreviven: valores, cadenas y `gc()` antes y después;
- cien veces cargar, ejecutar y descargar, con `Heap.getFree()` igual en cada vuelta;
- un destructor del programa, que corre en `unload`;
- un objeto suyo en una global de un módulo que se queda, y que la descarga lo nombra;
- cada condición de `load` y `unload`, con su texto;
- no existe, no enlaza, no tiene `Main`, se llama como uno cargado, no cabe.

### El ejemplo de Eduardo: el módulo compartido (4-nov)

> *«Tenemos un programa A que ha cargado los módulos M1,M2 y M3. Ahora con Overlay ejecutamos un programa B que cargaría
> los módulos M1 y M4. Como M1 ya está cargado no debería volverlo a cargar, en cambio M4 se cargaría. Ahora veamos el
> stack, tenemos A, M1, M2, M3 y el stack de A. Ahora cargamos B, tenemos A, M1, M2, M3 y el stack de A, M4 y stack de B.
> Ahora cerramos B, primero el stack de B desaparece, despues M4 se descarga, y devolvemos la ejecución al stack de A.»*

Es el algoritmo de arriba, con tres matices:

- **La pila de B son sus marcos encima de los de A**, en el mismo hilo. Se van al acabar B, y comparten el tamaño de la
  pila del main: si B se pasa, es el «desbordamiento de pila» de siempre, y lo atrapa `tryExe`. Una pila propia es lo que
  daría la versión asíncrona.
- **Primero la ejecución vuelve a A, y es A quien llama a `unload`.** Antes de descargar, A puede leer el error, y los
  destructores de B corren en su hilo.
- **El heap es de los dos,** y no sale en el dibujo. La memoria es `[ módulos ][ heap ][ tabla ][ pila de A ]`: B y M4 no
  pueden ir encima de la pila de A, y tienen que quedar por debajo del heap, porque así distingue la VM un literal de una
  cadena del heap. Por eso se apartan los objetos.

**Lo que trae compartir M1:**

- **B ve M1 como lo dejó A:** sus globales, y ya iniciado (`__init` no se repite). Lo que B cambie en M1 se queda para A.
  Así que B como overlay puede portarse distinto que B solo.
- **Si B se compiló contra otro M1,** `load` falla con el error de siempre (`gen`, o el enlace tardío).
- **Lo que B deje dentro de M1** es por donde se escapa algo a la descarga:
  - un objeto de una clase de B o de M4: `unload` se niega y lo nombra;
  - un objeto de M1 con un literal de B en un campo: se niega y lo nombra, porque los campos llevan bitmap;
  - **un literal de B en una global de M1:** `M1.setNombre("Demo")`, si M1 se lo guarda en una `var`. Esto no se detecta, y
    tras la descarga la global apunta a memoria que ya es otra cosa. Y un literal no tiene handle, así que no falla ahí
    mismo.

**El arreglo de lo último:** el compilador sabe qué globales son referencias (`declareGlobalRef`). Una tabla con sus
posiciones en el `.mod`, aditiva como las otras subtablas, le daría a `unload` el mismo recorrido exacto que tiene en los
objetos. El error sería *«queda un literal de B en una global de M1»*. Un `.mod` compilado antes de la tabla no se podría
comprobar.

### Los literales de un overlay, en el heap (la idea de Eduardo, 4-nov; por aprobar)

> *«Un literal de B, si se descarga desaparece. Ahora un literal de B una vez cargado se convierte en un string en el heap.
> Si ese literal está referenciado en A, lo que está referenciado es el string y está en el heap. Si desaparece B, no
> debería haber ningún problema, sigue en el heap y GC no lo borra porque hay referencias vivas en A.»*

**Hoy no es así.** Un literal se queda en el bloque de datos de su módulo, y la referencia es su dirección (`LEA_GLOBAL`,
`MOD_FORMAT` §5). Medido en las dos VMs, con un literal de 130 B: asignarlo a una variable gasta 0 B de heap, guardarlo en
una global gasta 0 B, y una cadena de esa medida hecha en marcha gasta 144 B.

**Pero se puede hacer que sea así, y es mejor que la tabla de arriba.** Sólo para los módulos de un overlay:

- el compilador emite una instrucción nueva, `LEA_STR`, en lugar de `LEA_GLOBAL`, cuando el símbolo es una cadena (lo sabe
  `ModWriter`: `addConstantString`);
- en un módulo normal, `LEA_STR` hace lo mismo que `LEA_GLOBAL`, con una comparación de más (`cs` frente al primer módulo
  del overlay);
- en un módulo del overlay, crea el string en el heap, con el camino de siempre, y empuja su handle.

Con `M1.setNombre("Demo")`, a M1 le llega un string del heap. Se descarga B, el string sigue ahí, y el GC lo suelta cuando
M1 deje de apuntarlo. Es lo que decía Eduardo.

**Al evaluarlo, no al cargar.** Al cargar habría que duplicar en el heap todos los literales de B, se usen o no, y `LEA_STR`
necesitaría una tabla para encontrar cada uno, que además serían raíces del GC. Al evaluarlo no hay tabla.

**Lo que desaparece:** la tabla de las globales, `__overlayCopia` (`e.msg` ya es del heap), mirar literales en los campos
y el caso de `Log.ruta`. `unload` sólo busca objetos de las clases que se van.

**Lo que cuesta:**

- cada vez que B evalúa un literal, una reserva: un bucle con literales irá más lento y dará más trabajo al GC. Sólo en un
  overlay; A y M1 no lo notan. Sin medir. Si duele, dos arreglos: no copiar donde el literal no se guarda (`print`, `==`,
  `+`) o una caché por literal;
- todos los `.mod` se recompilan, y un `.mod` de antes en un overlay sigue con literales sin proteger;
- **las natives de B:** su código tiene los literales en crudo. En la etapa 1, un overlay no carga sus `.mdn` y corre
  interpretado, que es lo que ya hace un módulo sin ellos.

**Lo que no cubre:** escribí aquí que los arrays del bloque de datos de B también son direcciones. Al programarlo se vio
que un programa BP no puede tener ninguno: `tipo[N]` sólo vale en una local o en un campo, una `const` sólo es un valor
simple o una cadena, y `var t: integer[] := [1, 2, 3]` nace en el heap. Así que las cadenas son lo único de un bloque de
datos que un programa puede pasar a otro módulo, y con `LEA_STR` no queda ninguna en crudo. `unload` sigue mirando las
referencias crudas de los campos: un `.mod` compilado antes de `LEA_STR` las tendría.

### Lo que había que decidir sobre el algoritmo

1. **¿OK al algoritmo?** Sobre todo a las condiciones de `load` (desde `Main`, sin hilos, sin ficheros abiertos) y a una
   sola carga a la vez.
2. **El nombre del módulo:** la clase `Overlay` en un módulo nuevo `Overlays`, como `Collections.Map`. En `Core` la
   pagarían todos los programas.
3. **Los literales de un overlay, en el heap** (arriba), en lugar de la tabla de las globales.

**Decidido** (Eduardo, 4-nov): *«1, 2 y 3 Si. Crea un hito nuevo lo vamos haciendo. Añade los apartados y las etapas que
consideres necesarios. Necesitaremos un par de programas de ejemplo ( A y B )»*. Es el hito H9, en `FICHAS`.

### Hecho en el PC (4-nov)

El algoritmo de arriba, sin cambios de fondo, en las dos VMs; el ejemplo, en `samples/overlay`, con A, B y M1 a M4 como en
el ejemplo de Eduardo. Lo que salió al programarlo:

- **Las intrínsecas de `Overlays` son públicas:** una clase sólo alcanza lo público de su módulo. Empiezan por `__`.
- **`tryExe` atrapa también sus propios errores:** un argumento de más de 127 bytes vuelve como `false`, con su texto, no
  como una excepción.
- **Un módulo sin `Main` se carga y no hace nada,** como cuando se ejecuta solo: el compilador siempre emite `__startup`.
- **`Core.mod` no cambia:** las primitivas nuevas no son públicas y su cuerpo va en línea.
- **La carpeta del programa principal,** la que se mira primero en `load`, la guarda la VM-C al cargar (`raiz_dir`): el CLI
  del PC no tiene `basedir`, y Java ya la tenía (`rootModuleDir`).

`overlay_smoke`, en la red: dieciséis casos en las dos VMs y el sim, byte a byte, y ocho sabotajes (`BPVM_SABOTAJE_OVERLAY`)
que tienen que cambiar la salida. La marca por clase de los destructores no tiene sabotaje: hacen falta dos clases en la
misma dirección, y una clase abierta va a la arena, detrás del código, así que no se puede forzar desde un programa.

### En las placas (4-nov)

Con la imagen de `761e9c00` y el `Stdlib.pack` nuevo en la Metro, la P4 y la Nucleo, los quince programas de `overlay_smoke`,
con la salida comparada con la de la VM-C del PC (y después, con la de `35297826`, que lleva lo de abajo y `#787`, los
dieciséis, los dieciséis bien en las tres):

| | Metro | P4 | Nucleo |
|---|---|---|---|
| los quince casos, la primera vez | 14 (`OvCargaFalla` no llega a cargar: `#787`) | 15 | 15 |
| los dieciséis, con la imagen de `35297826` | 16 | 16 | 16 |
| la pila de C de `run`: lo que sobra de 16 KB | 8,4 KB | 8,0 KB | no lo dice (`runStackFree` = -1) |
| `malloc`, el pico de cargar con `Json` en marcha | 59 KB de 298 (55 sin overlay) | — | 41 KB de 130 |
| un módulo de la zona de packs | `Json`, «cargado XIP desde pack», tres vueltas | sí | sí |
| el mismo bucle de 200 000 vueltas con un literal, solo | 1295 ms | 1060 ms | 1079 ms |
| y dentro de un overlay | 6620 ms: 27 µs por literal | 3225 ms: 11 µs | 2713 ms: 8 µs |

Fuera de un overlay, un literal cuesta lo mismo que leer una variable. Dentro, lo que cuesta crear una cadena.

La Nucleo pasó esos dieciséis con el `Overlays.mod` suelto, porque su zona no daba para el pack nuevo. Con la zona formateada y
el pack grabado, otra vez los dieciséis, sin nada suelto y con `Overlays` cargado XIP desde el pack.

**Una falsa alarma del GC, y era del diseño.** Al añadir la regla de los hilos, `tryExe` creció una local, todo se movió unos
bytes y el caso 2 empezó a fallar en el sim: *«Overlay.unload: sigue vivo un objeto de OvComun.Caja»*, de un objeto que nadie
alcanzaba. Quien lo marcaba era una palabra del bloque de datos de `Core`: un desplazamiento de una vtable que valía justo
su dirección, 4612. El recorrido es conservador: una palabra sin `TAG` que cae en un objeto lo retiene de más. Normalmente da
igual, pero `unload` pregunta qué sigue vivo de verdad, y esto lo hace mentir. En el PC no pasa: `Core` está en la memoria de
la VM, el heap empieza detrás y ningún desplazamiento llega. En las placas sí, porque `Core` está en el pack y el heap empieza
muy abajo (en 3008 en la Metro), en los mismos números que los desplazamientos: una lotería con cada objeto de una clase del
programa. **El arreglo:** con el programa cerrado, mientras se descarga, el GC sólo se fía de los handles. Una palabra sin
`TAG` nunca es una referencia a un objeto del heap: desde V4 sólo lo son los handles (`bpref_deref`). El resto del tiempo se
deja como está: las natives pueden tener direcciones crudas en su pila de C, y ahí retener de más no hace daño. La prueba, el
caso 6b: 3000 objetos de B, tres vueltas; su sabotaje va en el sim, que es donde `Core` está en un pack.

Queda un caso, raro: una local de una función que llama a `unload` por debajo de `Main`, aún sin valor, puede tener un handle
de lo que había antes en esa zona de la pila. Eso sí es un handle, y retiene el objeto. El `unload` se puede volver a intentar.

**Los hilos** (Eduardo, 4-nov: *«Debería ser un error, el programa B debería asegurarse de cerrar sus hilos antes de
salir»*). Si el `Main` de B acaba bien y deja hilos vivos, `tryExe` da `false`, con *«Overlay.tryExe: el programa ha acabado
con hilos vivos»*: compara los hilos vivos de antes y de después, con el builtin 302 (`__overlayThreads`). Los hilos de A no
cuentan. Si uno de A acaba mientras B deja uno suyo, la cuenta no lo ve, pero `unload` sigue negándose con otros hilos vivos.

## E14 · Micro32 Suite: una organización en GitHub y el proyecto en dos (Eduardo, 6-nov; `#802`; sin decidir)

Eduardo: *«con JC llamar al proyecto BasicPlus se queda un poco corto. Así que la idea es en GitHub cambiar el nombre de proyecto y convertirlo en un grupo de proyectos ( no se si 'grupo' es el término correcto ) . El nombre sería algo como 'Micro32 Suite' . Y luego para que sea un grupo de proyectos tenemos que dividir el nuestro en 2. Por un lado el IDE, el compilador y la VM-Java . Por el otro la VM-C y las VM de los micros. Hay que verlo pero creo que puede ser una separación bastante limpia, una se hace casi todo en Java y la otra se hace casi todo en C.»*

**El término** es *organización*. `legio-e/BasicPlus` se transfiere a ella, y los enlaces y los `git` de antes siguen funcionando por redirección, mientras nadie cree otro repositorio con el nombre viejo. Dividir, en cambio, crea repositorios nuevos, sin redirección, sin etiquetas y con otros hashes.

**El nombre.** El 6-nov, `github.com/micro32` y `micro-32` estaban cogidos; `micro32-suite` y `micro32suite` daban 404. No es una reserva, ni se ha mirado si es marca registrada. BasicPlus y JC siguen siendo los nombres de los lenguajes, y lo de dentro (`bpvm_*`, `BpIde`) no hace falta cambiarlo.

**Lo que dice el código: la separación es limpia.** Ninguna línea de Java enlaza con C, ni al revés: se hablan por contratos (el `.mod`, los opcodes, el wire, el `.mdn`, los packs).

**Lo que queda en medio, que no es de Java ni de C:**

| qué | por qué está en medio |
|---|---|
| `bpstdlib` (185 ficheros) | la necesita el compilador y va dentro de las imágenes; de `Core.bp` sale la tabla de builtins de las dos VMs (`scripts/gen_builtins.py`) |
| `compat` y `samples` (982 ficheros) | la red es el invariante: necesita las dos VMs por definición |
| el AOT | en los dos sentidos: el compilador escribe C contra las cabeceras y los `.ld` de `bpgenvm-c`, y el `Makefile` de la VM-C llama al compilador (el jar y `AotMain`) |
| lo que usa el IDE | `bpvm-sim` y la VM-C de host; y el ZIP se monta con todo |
| `docs`, con `FICHAS.md` | la fuente única de verdad es de las dos mitades |

**Lo que dicen los commits: la forma de trabajar no es tan limpia.** Sin contar los que sólo tocan documentos, tocan Java y C a la vez 341 de 1785 en toda la historia (19 %), y 51 de 161 en V8 (32 %). No es un defecto: es el invariante, la cascada hecha commit. Un builtin nuevo hoy es un commit; dividido, dos en dos repositorios que tienen que ir a la par.

**Y la historia.** 3185 commits y 1625 hashes citados en los documentos. `git filter-repo` conserva la historia de cada mitad, pero con otros hashes: `BasicPlus` tendría que quedarse, archivado, como la referencia de esos hashes y de las etiquetas `v1.0`–`v7.0`.

**Recomendación:**

1. La organización y el nombre, al publicar V8: entonces se hace el push de todos modos, y no se pierde nada.
2. La división, no antes de empezar V9, y cuanto más quieto esté el lenguaje, mejor: lo que cuesta son los commits que cruzan.
3. Antes, dentro del repositorio de hoy y con la red de testigo: quitar las rutas escritas a mano (`../lexer-java`, `C:\lenguajes\pm\bpgenvm-c`), y dejar fuera lo viejo (`lexer`, `vm`…).
4. Lo de en medio, decidido antes de cortar. La opción más sencilla: tres proyectos, el de Java, el de C y uno pequeño con la red y los contratos, que no es de ninguno de los dos.
