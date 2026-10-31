# Rendimiento en placa: rp2350, v7 frente a hoy

- v7: 2026-10-31 08:36, [    0] === boot Oct 12 2026 16:06:34 === | [    0] boot: imagen de DOS nucleos
- hoy: 2026-10-31 08:19, [    0] boot: imagen M (la VM caliente en la SRAM), pasada por P | [    0] boot: imagen de DOS nucleos
- cada programa, 2 veces; los tiempos son la media; «v7 → hoy» y el cambio (negativo = hoy más rápido)

| programa | lo que mide | v7 | hoy | cambio |
|---|---|---|---|---|
| arraybench | bubbleSort( #  elems)    no-native: # s   sum = # | 609.0 ms | 612.5 ms | +1 % |
| arraybench | bubbleSortNative( #  elems)  native: # s   sum = # | 612.5 ms | 612.0 ms | -0 % |
| arraybench | **el RUN entero** | 1248 ms | 1248 ms | +0 % |
| fibobench | fibo(#)*#      no-native: # s  total = # | 23232.5 ms | 23840.0 ms | +3 % |
| fibobench | fiboNative(#)*#    native: # s  total = # | 23278.5 ms | 23774.0 ms | +2 % |
| fibobench | **el RUN entero** | 46527 ms | 47628 ms | +2 % |
| floatbench | piLeibniz( # )    no-native: # s   π ≈ # | 2405.0 ms | 2601.5 ms | +8 % |
| floatbench | piLeibnizNative( # )  native: # s   π ≈ # | 2476.0 ms | 2603.5 ms | +5 % |
| floatbench | **el RUN entero** | 4897 ms | 5219 ms | +7 % |
| intbench | intOps(#) x #         no-native: # s  result = # | 1511.5 ms | 1372.0 ms | -9 % |
| intbench | intOpsNative(#) x #        native: # s  result = # | 1533.0 ms | 1374.0 ms | -10 % |
| intbench | **el RUN entero** | 3063 ms | 2761 ms | -10 % |
| sortbench | bubble    no-native: # s   a[#..#]= # # # | 1429.0 ms | 1379.0 ms | -3 % |
| sortbench | bubble       native: # s   a[#..#]= # # # | 1373.0 ms | 1379.0 ms | +0 % |
| sortbench | insertion no-native: # s   a[#..#]= # # # | 789.5 ms | 769.0 ms | -3 % |
| sortbench | insertion    native: # s   a[#..#]= # # # | 772.0 ms | 768.5 ms | -0 % |
| sortbench | selection no-native: # s   a[#..#]= # # # | 570.5 ms | 585.5 ms | +3 % |
| sortbench | selection    native: # s   a[#..#]= # # # | 568.0 ms | 585.0 ms | +3 % |
| sortbench | **el RUN entero** | 5544 ms | 5500 ms | -1 % |
| stringbench | strScan       x #   no-native: # s  result = # | 209449.0 ms | 183169.0 ms | -13 % |
| stringbench | strScanNative x #      native: # s  result = # | 213725.5 ms | 183169.0 ms | -14 % |
| stringbench | **el RUN entero** | 423190 ms | 366351 ms | -13 % |
| sumbench | sumArray( # ) x #       no-native: # s   sum = # | 2409.0 ms | 2474.0 ms | +3 % |
| sumbench | sumArrayNative( # ) x #     native: # s   sum = # | 2449.0 ms | 2474.5 ms | +1 % |
| sumbench | timedSumNative self-measured: # s | 2449.0 ms | 2474.5 ms | +1 % |
| sumbench | **el RUN entero** | 7376 ms | 7490 ms | +2 % |
| Bench | fib(#) interp =  #  in  #  ms | 8735.0 ms | 9091.5 ms | +4 % |
| Bench | fib(#) AOT    =  #  in  #  ms | 8751.5 ms | 9171.0 ms | +5 % |
| Bench | **el RUN entero** | 17503 ms | 18276 ms | +4 % |
| AllocBench | RESULTADO asignaciones:  # ; vivos:  # ; tiempo:  #  ms; vueltas:  # | 3044.0 ms | 1881.5 ms | -38 % |
| AllocBench | **el RUN entero** | 3082 ms | 1912 ms | -38 % |
| PrintBench | RESULTADO sin print:  #  ms; con print:  #  ms; lineas:  # ; suma:  # | 14.0 ms | 15.0 ms | +7 % |
| PrintBench | **el RUN entero** | 1456 ms | 735 ms | -50 % |
| VivoLargo | vivo  # :  #  ms | 1251.5 ms | 1341.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 2504.0 ms | 2682.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 3757.5 ms | 4024.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 5010.0 ms | 5366.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 6262.5 ms | 6707.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 7514.5 ms | 8049.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 8767.0 ms | 9390.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 10019.5 ms | 10732.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 11272.0 ms | 12073.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 12525.0 ms | 13415.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 13778.0 ms | 14757.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 15031.0 ms | 16099.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 16284.0 ms | 17441.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 17538.0 ms | 18783.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 18791.0 ms | 20125.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 20044.5 ms | 21467.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 21298.0 ms | 22810.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 22551.5 ms | 24152.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 23804.5 ms | 25494.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 25057.5 ms | 26836.0 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 26311.0 ms | 28178.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 27564.0 ms | 29520.5 ms | +7 % |
| VivoLargo | vivo  # :  #  ms | 28817.5 ms | — | — |
| VivoLargo | vivo  # :  #  ms | 29888.0 ms | — | — |
| VivoLargo | **el RUN entero** | 30015 ms | 30014 ms | -0 % |
| BenchMem | secuencial  #  lecturas =  #  ms  (suma  # ) | 5669.0 ms | 5870.0 ms | +4 % |
| BenchMem | disperso    #  lecturas =  #  ms  (suma  # ) | 5925.0 ms | 6123.0 ms | +3 % |
| BenchMem | **el RUN entero** | 11917 ms | 12319 ms | +3 % |
