# Rendimiento en placa: p4, p4ws frente a p4x

> 19-nov-2026, `#846` (Eduardo: *«Lo que gustaría es una prueba de rendimiento entre P4X y P4 a ver que tal. Por el
> reloj debería ser un 10% mas rápida»*). **p4ws**: la P4 de Waveshare (la revisión v1.x, 360 MHz), con la imagen de V8
> del lote (`imageCrc` `306828a5`, la de `dist/firmware`), su WiFi y su Bluetooth encendidos, la stdlib de su
> `Stdlib.pack` y el wire por la UART de su puente CH343. **p4x**: el kit ESP32-P4X-Function-EV (la v3.2, 400 MHz), con
> la suya (`9aecd6ef`), la stdlib de su imagen y el wire por el USB-Serial-JTAG. Las dos imágenes, del mismo código y
> con la misma PSRAM (HEX a 200 MHz), flash (40 MHz) y caché L2 (256 KB): sólo cambia la CPU. Medidas a la vez con
> `tools/rendimiento_versiones.py` (`mide p4 PUERTO hoy --sin-grabar`, con la carpeta de salida cambiada para no pisar
> `rendimiento/p4_hoy.json`, el del 31-oct): `rendimiento/p4_p4ws.json` y `rendimiento/p4_p4x.json`.
>
> **Lo que dice:** la P4X tarda entre un 8 y un 10 % menos en todo lo que es CPU y memoria (el reloj: 400/360, un 11 %
> más), lo nativo igual que lo interpretado; `AllocBench`, un 15 % (sin explicar: el silicio v3 o la radio encendida
> de la Waveshare). El RUN entero de `PrintBench` (2 000 líneas, 121 KB de salida) no es la CPU sino el cable: 20,5 s
> por la UART de la Waveshare y 0,28 s por el USB de la P4X.

- p4ws: 2026-11-19 18:42, [   52] === boot ESP32-P4 === | [  829] boot: imagen de DOS nucleos
- p4x: 2026-11-19 18:42, [    9] === boot ESP32-P4 === | [  547] boot: imagen de DOS nucleos
- cada programa, 2 veces; los tiempos son la media; «p4ws → p4x» y el cambio (negativo = p4x más rápido)

| programa | lo que mide | p4ws | p4x | cambio |
|---|---|---|---|---|
| arraybench | bubbleSort( #  elems)    no-native: # s   sum = # | 507.5 ms | 458.5 ms | -10 % |
| arraybench | bubbleSortNative( #  elems)  native: # s   sum = # | 507.5 ms | 457.5 ms | -10 % |
| arraybench | **el RUN entero** | 1032 ms | 930 ms | -10 % |
| fibobench | fibo(#)*#      no-native: # s  total = # | 18130.5 ms | 16713.0 ms | -8 % |
| fibobench | fiboNative(#)*#    native: # s  total = # | 18101.0 ms | 16715.5 ms | -8 % |
| fibobench | **el RUN entero** | 36241 ms | 33437 ms | -8 % |
| floatbench | piLeibniz( # )    no-native: # s   π ≈ # | 2066.5 ms | 1890.0 ms | -9 % |
| floatbench | piLeibnizNative( # )  native: # s   π ≈ # | 2063.0 ms | 1878.5 ms | -9 % |
| floatbench | **el RUN entero** | 4138 ms | 3777 ms | -9 % |
| intbench | intOps(#) x #         no-native: # s  result = # | 1009.5 ms | 930.0 ms | -8 % |
| intbench | intOpsNative(#) x #        native: # s  result = # | 1005.5 ms | 927.0 ms | -8 % |
| intbench | **el RUN entero** | 2024 ms | 1865 ms | -8 % |
| sortbench | bubble    no-native: # s   a[#..#]= # # # | 1143.0 ms | 1029.5 ms | -10 % |
| sortbench | bubble       native: # s   a[#..#]= # # # | 1140.5 ms | 1025.5 ms | -10 % |
| sortbench | insertion no-native: # s   a[#..#]= # # # | 622.0 ms | 560.0 ms | -10 % |
| sortbench | insertion    native: # s   a[#..#]= # # # | 621.0 ms | 558.5 ms | -10 % |
| sortbench | selection no-native: # s   a[#..#]= # # # | 470.0 ms | 423.0 ms | -10 % |
| sortbench | selection    native: # s   a[#..#]= # # # | 470.0 ms | 424.0 ms | -10 % |
| sortbench | **el RUN entero** | 4490 ms | 4042 ms | -10 % |
| stringbench | strScan       x #   no-native: # s  result = # | 131965.5 ms | 120694.5 ms | -9 % |
| stringbench | strScanNative x #      native: # s  result = # | 132201.0 ms | 121108.5 ms | -8 % |
| stringbench | **el RUN entero** | 264174 ms | 241812 ms | -8 % |
| sumbench | sumArray( # ) x #       no-native: # s   sum = # | 1951.0 ms | 1765.0 ms | -10 % |
| sumbench | sumArrayNative( # ) x #     native: # s   sum = # | 1951.5 ms | 1762.5 ms | -10 % |
| sumbench | timedSumNative self-measured: # s | 1949.0 ms | 1762.0 ms | -10 % |
| sumbench | **el RUN entero** | 5900 ms | 5334 ms | -10 % |
| Bench | fib(#) interp =  #  in  #  ms | 6968.5 ms | 6432.0 ms | -8 % |
| Bench | fib(#) AOT    =  #  in  #  ms | 33.0 ms | 31.0 ms | -6 % |
| Bench | **el RUN entero** | 7010 ms | 6471 ms | -8 % |
| AllocBench | RESULTADO asignaciones:  # ; vivos:  # ; tiempo:  #  ms; vueltas:  # | 1259.0 ms | 1076.0 ms | -15 % |
| AllocBench | **el RUN entero** | 1282 ms | 1094 ms | -15 % |
| PrintBench | RESULTADO sin print:  #  ms; con print:  #  ms; lineas:  # ; suma:  # | 12.0 ms | 11.0 ms | -8 % |
| PrintBench | **el RUN entero** | 20933 ms | 300 ms | -99 % |
| VivoLargo | vivo  # :  #  ms | 1102.0 ms | 1016.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 2204.5 ms | 2033.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 3306.0 ms | 3047.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 4409.5 ms | 4064.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 5513.0 ms | 5079.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 6613.0 ms | 6092.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 7714.5 ms | 7109.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 8818.5 ms | 8124.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 9921.5 ms | 9139.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 11024.5 ms | 10153.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 12127.5 ms | 11168.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 13231.0 ms | 12183.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 14334.0 ms | 13198.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 15436.5 ms | 14213.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 16540.5 ms | 15229.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 17644.5 ms | 16244.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 18749.0 ms | 17259.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 19853.0 ms | 18273.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 20957.5 ms | 19288.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 22060.5 ms | 20304.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 23163.0 ms | 21319.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 24265.0 ms | 22334.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 25369.0 ms | 23349.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 26472.5 ms | 24365.5 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 27576.5 ms | 25382.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 28680.5 ms | 26398.0 ms | -8 % |
| VivoLargo | vivo  # :  #  ms | 29782.5 ms | 27414.0 ms | -8 % |
| VivoLargo | **el RUN entero** | 30008 ms | 30007 ms | -0 % |
| BenchMem | secuencial  #  lecturas =  #  ms  (suma  # ) | 4546.5 ms | 4141.5 ms | -9 % |
| BenchMem | disperso    #  lecturas =  #  ms  (suma  # ) | 4724.5 ms | 4305.5 ms | -9 % |
| BenchMem | **el RUN entero** | 9502 ms | 8657 ms | -9 % |
