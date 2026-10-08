# Consumo de RAM del sistema por programa — esp32c6 (imagen 7c18cbb7, Oct  8 2026 15:00:22)

Medido con `tools/medir_consumo.py`: la línea `mem:` del firmware tras cada RUN, antes de liberar el programa. «Libre», lo que quedaba con el programa aún vivo; «mínimo», el peor momento desde el reset de la serie.

## neopixel — dos tiras NeoPixel (#666: no se sueltan al acabar)

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| NeoUna | NO-PETA | 77468 | 72972 | 57344 |
| NeoUna | NO-PETA | 77468 | 72956 | 57344 |
| NeoTres | NO-PETA | 77388 | 72868 | 57344 |
| Nada | NO-PETA | 80480 | 72868 | 58368 |

## pwm_contador — un PWM y un contador de pulsos (#666)

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| PwmDos | NO-PETA | 77376 | 73032 | 58368 |
| PwmDos | NO-PETA | 77376 | 73032 | 57344 |
| PulsoTres | NO-PETA | 77352 | 73016 | 57344 |
| PulsoTres | NO-PETA | 77352 | 73016 | 57344 |
| Nada | NO-PETA | 80308 | 73016 | 59392 |

## modulos_fichero — cinco módulos más de la stdlib, y un fichero abierto (Limpia lo borra después)

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| Importa5 | NO-PETA | 59056 | 52672 | 43008 |
| Nada | NO-PETA | 80588 | 52672 | 63488 |
| FicheroAbierto | NO-PETA | 75564 | 52672 | 56320 |
| Nada | NO-PETA | 80588 | 52672 | 63488 |
| Limpia | NO-PETA | 77620 | 52672 | 61440 |

## gui — dos programas de GUI

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| GuiWinJson | IDENTICO | 42252 | 36008 | 23040 |
| Nada | NO-PETA | 80588 | 36008 | 63488 |
| GuiArbolG2 | IDENTICO | 40060 | 34592 | 20480 |
| Nada | NO-PETA | 80588 | 34592 | 63488 |

## natives — dos programas con natives (su código, en RAM ejecutable)

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| NatV7 | IDENTICO | 68120 | 63772 | 48128 |
| Nada | NO-PETA | 80588 | 63772 | 63488 |
| FloatNat | IDENTICO | 78404 | 63772 | 58368 |
| Nada | NO-PETA | 80588 | 63772 | 63488 |

