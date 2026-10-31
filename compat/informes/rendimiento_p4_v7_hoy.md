# Rendimiento en placa: p4, v7 frente a hoy

- v7: 2026-10-31 08:37, [   56] === boot ESP32-P4 === | [ 2267] boot: imagen de DOS nucleos
- hoy: 2026-10-31 08:54, [   52] === boot ESP32-P4 === | [ 2159] boot: imagen de DOS nucleos
- cada programa, 2 veces; los tiempos son la media; «v7 → hoy» y el cambio (negativo = hoy más rápido)

| programa | lo que mide | v7 | hoy | cambio |
|---|---|---|---|---|
| arraybench | bubbleSort( #  elems)    no-native: # s   sum = # | 496.0 ms | 508.0 ms | +2 % |
| arraybench | bubbleSortNative( #  elems)  native: # s   sum = # | 496.5 ms | 508.0 ms | +2 % |
| arraybench | **el RUN entero** | 1008 ms | 1031 ms | +2 % |
| fibobench | fibo(#)*#      no-native: # s  total = # | 17252.0 ms | 17892.0 ms | +4 % |
| fibobench | fiboNative(#)*#    native: # s  total = # | 17219.5 ms | 17868.5 ms | +4 % |
| fibobench | **el RUN entero** | 34481 ms | 35769 ms | +4 % |
| floatbench | piLeibniz( # )    no-native: # s   π ≈ # | 1959.5 ms | 2042.5 ms | +4 % |
| floatbench | piLeibnizNative( # )  native: # s   π ≈ # | 1950.0 ms | 2035.5 ms | +4 % |
| floatbench | **el RUN entero** | 3919 ms | 4086 ms | +4 % |
| intbench | intOps(#) x #         no-native: # s  result = # | 970.0 ms | 1008.5 ms | +4 % |
| intbench | intOpsNative(#) x #        native: # s  result = # | 972.5 ms | 1008.0 ms | +4 % |
| intbench | **el RUN entero** | 1952 ms | 2024 ms | +4 % |
| sortbench | bubble    no-native: # s   a[#..#]= # # # | 1116.5 ms | 1143.0 ms | +2 % |
| sortbench | bubble       native: # s   a[#..#]= # # # | 1113.0 ms | 1138.5 ms | +2 % |
| sortbench | insertion no-native: # s   a[#..#]= # # # | 611.5 ms | 627.5 ms | +3 % |
| sortbench | insertion    native: # s   a[#..#]= # # # | 611.0 ms | 627.5 ms | +3 % |
| sortbench | selection no-native: # s   a[#..#]= # # # | 460.0 ms | 473.0 ms | +3 % |
| sortbench | selection    native: # s   a[#..#]= # # # | 462.0 ms | 474.5 ms | +3 % |
| sortbench | **el RUN entero** | 4399 ms | 4506 ms | +2 % |
| stringbench | strScan       x #   no-native: # s  result = # | 144405.0 ms | 137132.0 ms | -5 % |
| stringbench | strScanNative x #      native: # s  result = # | 144574.0 ms | 136896.5 ms | -5 % |
| stringbench | **el RUN entero** | 288988 ms | 274037 ms | -5 % |
| sumbench | sumArray( # ) x #       no-native: # s   sum = # | 1902.0 ms | 1942.5 ms | +2 % |
| sumbench | sumArrayNative( # ) x #     native: # s   sum = # | 1901.5 ms | 1946.5 ms | +2 % |
| sumbench | timedSumNative self-measured: # s | 1901.5 ms | 1946.0 ms | +2 % |
| sumbench | **el RUN entero** | 5752 ms | 5882 ms | +2 % |
| Bench | fib(#) interp =  #  in  #  ms | 6620.5 ms | 6859.0 ms | +4 % |
| Bench | fib(#) AOT    =  #  in  #  ms | 32.5 ms | 33.0 ms | +2 % |
| Bench | **el RUN entero** | 6664 ms | 6901 ms | +4 % |
| AllocBench | RESULTADO asignaciones:  # ; vivos:  # ; tiempo:  #  ms; vueltas:  # | 1130.5 ms | 1125.5 ms | -0 % |
| AllocBench | **el RUN entero** | 1152 ms | 1146 ms | -1 % |
| PrintBench | RESULTADO sin print:  #  ms; con print:  #  ms; lineas:  # ; suma:  # | 11.5 ms | 11.0 ms | -4 % |
| PrintBench | **el RUN entero** | 20934 ms | 20934 ms | +0 % |
| VivoLargo | vivo  # :  #  ms | 1071.5 ms | 1100.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 2141.5 ms | 2199.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 3210.0 ms | 3298.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 4281.0 ms | 4396.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 5351.0 ms | 5498.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 6422.0 ms | 6598.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 7492.0 ms | 7698.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 8562.5 ms | 8798.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 9632.0 ms | 9900.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 10702.5 ms | 11001.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 11772.0 ms | 12102.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 12842.0 ms | 13203.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 13913.0 ms | 14305.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 14983.0 ms | 15407.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 16053.0 ms | 16509.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 17123.0 ms | 17610.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 18193.5 ms | 18712.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 19263.5 ms | 19813.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 20334.0 ms | 20915.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 21403.5 ms | 22017.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 22474.5 ms | 23120.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 23545.0 ms | 24222.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 24615.5 ms | 25324.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 25685.5 ms | 26425.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 26755.5 ms | 27526.5 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 27825.0 ms | 28629.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 28896.0 ms | 29730.0 ms | +3 % |
| VivoLargo | vivo  # :  #  ms | 29965.5 ms | — | — |
| VivoLargo | **el RUN entero** | 30008 ms | 30007 ms | -0 % |
| BenchMem | secuencial  #  lecturas =  #  ms  (suma  # ) | 4474.5 ms | 4596.0 ms | +3 % |
| BenchMem | disperso    #  lecturas =  #  ms  (suma  # ) | 4634.5 ms | 4761.5 ms | +3 % |
| BenchMem | **el RUN entero** | 9340 ms | 9592 ms | +3 % |
