# La prueba de resistencia de días

Días de carga variada en varias placas a la vez, para ver lo que las tandas cortas no ven (V8, 15-nov; Eduardo: *«La
prueba de resistencia es para ver si salen problemas que en las tandas cortas que hacemos no se ven. Si sale algún
problema lo estudiamos y vemos que hay que hacer.»*).

- **`Resistencia.bp`**: la prueba. Cada vuelta, el cálculo, la memoria, los ficheros, dos hilos y la pantalla, cada
  fase con su marca y su try/catch, y 10 s de espera; cada 10 minutos, una línea de estado. Todo a
  `/data/resistencia/log.txt` (y `log.1`). No importa `Gui`: es la de las placas sin pantalla.
- **`ResistenciaGui.bp`**: la misma prueba en una placa con pantalla, pintando en ella.
- **`resistencia.py`**: la prepara, la mira y la recoge, por el cable serie (`fallos`, `empezar`, `mirar`, `acabar`;
  el uso, en su cabecera).

```sh
export MSYS_NO_PATHCONV=1
PY=/c/Espressif/tools/python/v6.1/venv/Scripts/python.exe
$PY compat/resistencia/resistencia.py COM22 fallos                      # antes: un error de cada clase, atrapado
$PY compat/resistencia/resistencia.py COM22 empezar C:/tmp/r/pico2      # sin pantalla
$PY compat/resistencia/resistencia.py COM15 empezar C:/tmp/r/p4 gui     # con pantalla
$PY compat/resistencia/resistencia.py COM22 mirar                       # durante: las últimas líneas del log
$PY compat/resistencia/resistencia.py COM22 acabar C:/tmp/r/pico2       # al final: los logs, el de la placa y el INFO
```

En el PC, `corta` (tres vueltas de medio segundo) o `fallos`, con la flash en una carpeta:
`java -jar miVM/target/bpgenvm-1.0.jar --flash-dir=<carpeta> Resistencia.mod corta`.
