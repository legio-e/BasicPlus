# Consumo de RAM del sistema por programa — esp32c6 (imagen 7c18cbb7, Oct  8 2026 15:00:22)

Medido con `tools/medir_consumo.py`: la línea `mem:` del firmware tras cada RUN, antes de liberar el programa. «Libre», lo que quedaba con el programa aún vivo; «mínimo», el peor momento desde el reset de la serie.

## destructor — un objeto con destructor vivo al acabar (#668), y un fichero cerrado y uno abierto (8-oct)

| programa | veredicto | libre | mínimo | bloque mayor |
|---|---|---|---|---|
| Nada | NO-PETA | 80588 | 75268 | 63488 |
| ConDestructor | NO-PETA | 78500 | 74164 | 59392 |
| Nada | NO-PETA | 80588 | 74164 | 63488 |
| Limpia | NO-PETA | 77620 | 72160 | 61440 |
| FicheroCerrado | NO-PETA | 75540 | 71196 | 56320 |
| Nada | NO-PETA | 80588 | 71196 | 63488 |
| FicheroAbierto | NO-PETA | 75564 | 71196 | 56320 |
| Nada | NO-PETA | 80588 | 71196 | 63488 |
| Limpia | NO-PETA | 77620 | 71196 | 61440 |

