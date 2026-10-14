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
   hace con el motivo `0x13` (el otro extremo cerró); el teclado, con un `0xE1` que no es estándar, probablemente su
   propio código al dormirse. Al moverlo o al pulsar una tecla vuelven por la dirección guardada en menos de un
   segundo, y las primeras teclas llegan incluso antes de terminar la suscripción: el teclado recuerda que estábamos
   suscritos.

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

**¿Fichero o Entorno?** Por decidir (Eduardo):
- **El Entorno** es donde vive la configuración de la placa: la pantalla de la P4 pasó de `/sys/board.json` al
  Entorno. El firmware lo lee al arrancar y el IDE lo enseña y lo deja editar. Pero hoy un programa BasicPlus sólo
  puede leerlo (`Machine.getEnv`): escribirlo pediría un `Machine.setEnv`, nuevo.
- **Un fichero** en `/sys`: BasicPlus ya escribe ficheros, y el firmware tendría que leerlo al arrancar.

⚠️ **Una trampa, medida el 14-oct: la NVS no sobrevive a una actualización del firmware.** NimBLE guarda las claves en
la NVS (`0x9000`), y la imagen fusionada con la que se actualiza (la que `INSTALAR_FIRMWARE` dice grabar en el 0) lleva
`FF` en esa zona: la de v7.0, comprobada. Cada actualización borraría las claves, y el usuario tendría que volver a
emparejarlo todo. Para que «una vez configurado» sea verdad, **las claves tienen que vivir con el resto de la
configuración, en nuestro almacenamiento** (el Entorno o el sistema de ficheros, más allá de la aplicación, donde una
actualización no llega). NimBLE deja poner un almacén propio (`ble_hs_cfg.store_read_cb`, `store_write_cb`,
`store_delete_cb`). Y como las claves son secretas, mejor en un fichero que el IDE no enseñe que en el Entorno, que sí
lo enseña.

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
2. **La GUI.** Cuando el pack de la GUI está cargado, el firmware registraría en LVGL un puntero (el ratón, con su
   cursor) y un teclado (con su grupo), donde hoy se registra la táctil.
3. **El programa sin GUI.** ¿Un módulo de BasicPlus que dé las teclas y el ratón como eventos? El nombre, por decidir.
4. **El emparejamiento es configuración de la placa, no del programa**, como el Entorno. **El camino lo dio Eduardo
   el 14-oct**: un programa BasicPlus de configuración, que guarda en un fichero o en el Entorno (arriba, «La
   configuración, un programa BasicPlus»). Queda por decidir el fichero o el Entorno. Y las claves tienen que salir de
   la NVS, porque una actualización del firmware la borra.
5. **La distribución del teclado**: una clave del Entorno, por ejemplo `teclado=es`.
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
