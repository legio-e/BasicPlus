# Reproductores de #392 y #389 (V5/H5, 10-ago)

Salieron de escribir el DAO del ORM a mano. Ninguno es un ejemplo: son
instrumentos, y se quedan aquí hasta que los bugs se cierren — hacen falta para
verificar el arreglo.

## #392 — hija + sobrecarga cross-module descoloca los slots

    ProbeBase.bp   `Base`, la clase padre, en su módulo.
    ProbeDao.bp    `CosaDao extends ProbeBase.Base`. Aquí se meten las
                   sobrecargas que disparan el fallo.
    ProbeUso.bp    tercer módulo: usa la hija desde fuera.

Cómo se reproduce: en `CosaDao`, poner TRES sobrecargas de un método y otro
método detrás. Compila sin un diagnóstico, y al ejecutar:

    [bpvm-c link] lib 'ProbeDao' presente pero no exporta
                  'ProbeDao.CosaDao#despues#8' (la usa 'ProbeUso'; version vieja?)

El dueño lo tiene en el slot 10; el llamante pide el 8 — faltan 2, que son las
2 sobrecargas extra. Con el árbol tal cual está ahora (sin sobrecargas) los tres
compilan y corren bien: ése es el CONTROL.

## #389 — `Object` es `any`, el downcast no comprueba nada

    ProbeMal.bp    un DAO que dice leer `Cosa` y devuelve `Otra`.

Compila sin un aviso y `c.v` imprime **504**: está leyendo el slot de otra
clase. Basura plausible, que es la peor clase de fallo.

## Cómo se compilan

Un `.bpbuild` con `sourceDir` apuntando aquí. Los que usé están en
`notas/v5-sqlite-prueba/` (esa carpeta no se versiona).
