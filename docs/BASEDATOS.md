# BasicPlus — bases de datos

> Todo lo de este documento está **ejecutado**, no supuesto: los ejemplos salen de
> `samples/SqlDemo.bp`, `samples/DaoDemo.bp` y `samples/GenDemo.bp`, que corren en
> las dos VMs y en placa. Si algo aquí no funciona, es un bug y va a `docs/FICHAS.md`.

BasicPlus habla con **SQLite**. El mismo motor, el mismo fichero `.db` y el mismo
código en el PC, en la Metro y en el ESP32-P4. Se puede usar de dos maneras, y las
dos son legítimas:

- **con SQL**, escribiendo las sentencias a mano — directo y sin sorpresas;
- **con el ORM**, que las escribe por ti a partir de tus clases.

---

## 1. El pack: instalarlo antes de nada

El motor **no está dentro de la VM**. Es un *pack* de código nativo que se graba
aparte, y se publica con el nombre `SQLI`. Esto no es un capricho: SQLite ocupa
más que toda la VM junta, y quien no use bases de datos no tiene por qué pagarlo.

**Construir el pack** (una vez, o cuando cambie la librería):

```bash
java -jar lexer-java/target/basicplus-frontend.jar --project bpstdlib/sqlite/SQLite.bpbuild --backend=mivm
```

**Grabarlo en la placa**: en el IDE, panel de *Packs* → **«Copiar pack a la placa…»**
y elige `packs/SQLite.pack`. Ahí mismo están *Refrescar*, *Borrar* y *Formatear zona*.

**Darle memoria.** SQLite trabaja sobre una **arena** propia, separada del montón de
la VM, y su tamaño se declara en el entorno de la placa:

```
SQLite=4
```

Cuatro megabytes es lo que usa el P4. Si pones `SQLite=0` o no cabe, el pack no
arranca y el arranque lo dice en su línea `bd:`.

**Si falta el pack**, el programa no devuelve ceros ni se cuelga: la primera llamada
lanza una excepción que lo nombra.

```
sql_open: falta el codigo nativo del pack 'SQLI' v1.
Este modulo no puede funcionar sin el.
```

---

## 2. Sin ORM: hablar SQL directamente

Tres verbos, y son los tres que hay.

```basic
import SQLite

var db: SQLite.Db := SQLite.Db()
if not db.connect("/sd/medidas.db") then
  print "no se pudo abrir -> ", db.errorMessage()
  return
endif
```

`connect` **devuelve `false`**, no lanza: que no haya tarjeta o que la ruta esté mal
es algo que *pasa*, y tu programa decide qué hacer. Hay también `connectRead(path)`
para abrir en sólo lectura.

### 2.1 Mandar y ya

```basic
db.exec("CREATE TABLE medidas (id INTEGER PRIMARY KEY, sensor TEXT, valor REAL, t INTEGER)")
db.exec("INSERT INTO medidas(sensor, valor) VALUES('temp', 21.5)")
print "el ultimo id: ", db.lastInsertId()    // long
print "filas tocadas:", db.changes()
```

Admite varias sentencias en una llamada, separadas por `;`. Aquí **un SQL mal escrito
sí es excepción**: eso es un fallo del programa, no del mundo.

### 2.2 Pedir UN dato

```basic
print db.execInt("SELECT count(*) FROM medidas")
print db.execDouble("SELECT avg(valor) FROM medidas WHERE sensor = 'temp'")
print db.execStr("SELECT sensor FROM medidas ORDER BY id LIMIT 1")
```

Hay uno por tipo de respuesta porque BP es de tipado estático: el tipo de retorno
tiene que estar decidido al compilar. El segundo parámetro es **qué contestar cuando
no hay ninguna fila**, que no es lo mismo que «vale cero»:

```basic
db.execInt("SELECT valor FROM medidas WHERE sensor = 'viento'", -1)   // -1
```

### 2.3 Recorrer filas

```basic
var q: SQLite.Query := db.query("SELECT sensor, valor, t FROM medidas ORDER BY valor")
while q.nextRow() do
  print q.getStr(0), q.getDouble(1), q.getLong(2)
endwh
q.release()
```

Las columnas van **por índice, empezando en 0**, como los arrays de BP. Están además
`columnCount()`, `columnName(i)` y `columnType(i)` para recorrer una tabla que no
conoces de antemano. Y `getInt`, `getLong`, `getDouble` y `getStr` para leer.

> **La regla que hace difícil equivocarse: sólo lo que devuelve `query` hay que
> liberarlo.** Todo lo demás termina solo. Y `close()` es de la **base**, va una vez
> al final, y no tiene nada que ver con el `release()` de las consultas.

---

## 3. Definir entidades: qué se guarda y dónde

Una entidad es una clase normal con anotaciones `@BD`. El punto importante:

> **Sólo persiste lo que lleva `@BD`.** Una `property` sin anotar es tuya y no viaja
> a la base. No hay que excluir nada: hay que incluir lo que quieras guardar.

```basic
module Modelo

  @BD{ tabla = "medidas" }
  public class Medida
    @BD{ pk }                      public property id:       long
    @BD{ columna = "sensor_id" }   public property sensorId: long
    @BD{}                          public property valor:    double
    @BD{}                          public property t:        long

    public function Medida()
      this.id := 0
      this.sensorId := 0
      this.valor := 0.0d
      this.t := 0
    end Medida
  end Medida

end Modelo
```

- **`@BD{ tabla = "..." }`** sobre la clase — con qué tabla casa.
- **`@BD{ pk }`** — la clave primaria. La pone la base al insertar, y al volver de
  `insert` tu objeto ya la lleva puesta.
- **`@BD{ columna = "..." }`** — cuando la columna se llama distinto que la property.
  Sin él, se usa el nombre de la property.
- **`@BD{}`** a secas — se guarda, y la columna se llama igual.

Los tipos casan así: `long`/`integer` → `INTEGER`, `double`/`float` → `REAL`,
`string` → `TEXT`.

---

## 4. El ORM: `Dao` y `Where`

Sobre las entidades va un **DAO por tabla**. Te da los verbos de siempre sin que
escribas SQL, y `Where` construye las condiciones:

```basic
var w: Orm.Where := Orm.Where()
w.eq("sensor_id", 1L).lt("valor", 22.2d)
var frias: List := mdao.list(w)
```

`Where` tiene `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `like`, `isNull`, `isNotNull` y
`raw` para lo que no cubra; se encadenan con AND, y `orNext()` cambia el enlace a OR.
`sql()` te enseña lo que va a mandar, que es cómodo para depurar.

> **Y `Where` no es azúcar: es seguridad.** Escribe los valores **escapados**. Un
> nombre como `O'Brien` concatenado a mano parte la sentencia; por `Where` entra
> entero y no pasa nada.

### Leer los resultados

`list()` devuelve una `List`, cuyos elementos son `Object`. Para sacar la entidad,
un cast; para sacar valores sueltos, los **captadores tipados**:

```basic
var m: Modelo.Medida := Modelo.Medida(suyas.get(i))   // entidades: cast
var n: integer := otra.getInteger(0)                  // valores: convierte
```

---

## 5. Generar los DAO automáticamente

En el IDE: botón (o menú) **«DAO build»**. Por línea de comandos:

```bash
java -jar lexer-java/target/basicplus-frontend.jar --project mi/proyecto.bpbuild --backend=mivm --dao
```

Lee las `@BD` del proyecto y escribe un módulo con un DAO por entidad. El fichero
empieza avisando:

```
// ⚠️  NO EDITAR — este fichero lo genera `DAO build` [...] y se REHACE cada vez.
// @generado 50c3648e0696b052
```

Ese `@generado` es una huella: si el fichero no ha cambiado, el generador dice
`ya al dia` y no lo reescribe.

---

## 6. Las operaciones (CRUD)

Con `dao` construido sobre una conexión (`MedidaDao(db)`):

| verbo | qué hace |
|---|---|
| `insert(e)` | inserta; al volver, `e` lleva su clave puesta |
| `add(e)` | igual, cuando no te interesa el id |
| `loadById(id)` | trae uno por clave, o `null` |
| `load(e)` | rellena `e` desde la base usando su clave; `false` si no estaba |
| `refresh(e)` | recarga `e` (lo mismo que `load`, con nombre de intención) |
| `update(e)` | guarda los cambios; `false` si no existía |
| `delete(e)` | borra; el objeto queda con la clave a 0 |
| `deleteById(id)` | borra por clave |
| `list(w, max)` | los que cumplen la condición |
| `listAll(max)` | todos |
| `uno(cond)` | el primero que cumpla, o `null` |
| `close()` | cierra el DAO (la conexión la cierra quien la abrió) |

---

## 7. Extender un DAO

**Nunca edites el generado**: se rehace y pierdes los cambios. Hereda:

```basic
public class SensorDaoPlus extends ModeloDao.SensorDao
  public function SensorDaoPlus(base: SQLite.Db)
    super(base)
  end SensorDaoPlus

  // Buscar por la OTRA clave. Convención: por clave, `...By<algo>`.
  public function loadByNombre(nombre: string): Modelo.Sensor
    return Modelo.Sensor(this.uno("nombre = '" + Orm.esc(nombre) + "'"))
  end loadByNombre
end SensorDaoPlus
```

Así puedes regenerar cuando quieras, que es justo para lo que está pensado.

---

## 8. Verificar tus entidades contra la base de verdad

Si tu proyecto declara una base, **el compilador contrasta las `@BD` con el esquema
real** en cada build y te avisa de lo que no cuadra: tablas que no están, columnas
que faltan o sobran, tipos que no casan.

```json
{
  "sourceDir": ".",
  "outDir":    "out",
  "database":  "medidas.db"
}
```

Tres cosas que conviene saber:

- **Son avisos, nunca errores.** Una base se puede diseñar entera antes que el
  programa, y con cuarenta tablas por delante nadie escribe las cuarenta entidades
  de un tirón. Bloquear el build por eso convertiría la herramienta en un estorbo.
- **La base se abre en sólo lectura.** Es tuya y puede tener datos que no están en
  ningún otro sitio; una herramienta de compilación no tiene por qué escribir ahí.
- **La verificación es opcional por diseño.** Necesita un driver JDBC en tiempo de
  ejecución; si no está, se salta y lo dice, y el build sigue.

---

## 9. Acelerar los `SELECT` con índices

Aquí no hay nada de BasicPlus: es SQL, y se manda con `exec`.

```basic
db.exec("CREATE INDEX IF NOT EXISTS ix_medidas_sensor ON medidas(sensor_id)")
db.exec("CREATE INDEX IF NOT EXISTS ix_medidas_t      ON medidas(t)")
```

La regla práctica: **indexa las columnas por las que filtras u ordenas**, sobre todo
las claves ajenas que usas en `Where.eq(...)`. En un micro se nota antes que en un
PC, porque la tarjeta es lenta: sin índice, SQLite recorre la tabla entera leyendo
de la SD.

Y el reverso, que también importa: cada índice ocupa sitio y encarece los `INSERT`.
En un registrador que escribe mucho y consulta poco, un índice de más cuesta.

---

## 10. Trabajar por ventanas, para no comerse la memoria

Un `SELECT` de cien mil filas no cabe en un microcontrolador. Hay dos formas, y la
primera es la buena:

**Con cursor** — `query` + `nextRow` **nunca tiene más de una fila en memoria**.
Da igual que la tabla tenga un millón de registros:

```basic
var q: SQLite.Query := db.query("SELECT id, valor FROM medidas ORDER BY id")
while q.nextRow() do
  procesar(q.getLong(0), q.getDouble(1))
endwh
q.release()
```

**Por páginas**, cuando necesitas los resultados juntos (para ordenarlos, mostrarlos
o mandarlos):

```basic
var q: SQLite.Query := db.query("SELECT n FROM t ORDER BY n LIMIT 2 OFFSET 2")
```

Y en el ORM, el tope va en el propio verbo — **`list` y `listAll` traen 1000 como
máximo si no dices otra cosa**, que es una red y no un capricho:

```basic
var pagina: List := mdao.list(w, 50)
```

> ⚠️ `OFFSET` grande es caro: SQLite tiene que contar las filas saltadas. Para
> recorrer una tabla entera es mejor el cursor, o paginar por clave
> (`WHERE id > ultimo ORDER BY id LIMIT 50`).

---

## 11. Varias consultas a la vez

Aquí va una que sorprende a quien viene de JDBC: **puedes tener varias consultas
abiertas sobre la misma conexión, y anidarlas**. Cada `query` lleva su propio cursor
y avanzan independientes.

```basic
var s: SQLite.Query := db.query("SELECT DISTINCT sensor FROM medidas")
while s.nextRow() do
  var nombre: string := s.getStr(0)
  // ...y una consulta DENTRO del recorrido de la otra
  var cuantas: integer := db.execInt("SELECT count(*) FROM medidas WHERE sensor = '"
                                   + Orm.esc(nombre) + "'")
  print nombre, "->", cuantas
endwh
s.release()
```

En JDBC esto obliga a abrir otra conexión, porque un `Statement` cierra su
`ResultSet` al reutilizarse. Aquí no hace falta: los `execInt` de dentro abren y
sueltan el suyo sin tocar el cursor de fuera.

---

## 12. Más de una conexión

También se puede, y a la vez:

```basic
var a: SQLite.Db := SQLite.Db()
var b: SQLite.Db := SQLite.Db()
a.connect("/sd/medidas.db")
b.connect("/sd/config.db")
```

**Pero vigila la memoria.** Las dos conexiones salen de la **misma arena**, la que
declara `SQLite=<MB>` en el entorno, y no crece: cada base abierta se lleva su caché
de páginas y sus estructuras. En un PC esto no se nota; en un micro con 4 MB de
arena, sí.

La regla: **si dos DAO trabajan sobre la misma base, comparten conexión.** Se les
pasa el mismo `Db` al construirlos y cada uno cierra lo suyo; la conexión la cierra
quien la abrió, y el último.

---

## 13. Trampas conocidas

**El sufijo `L` de los `long`.** Una marca de tiempo en milisegundos no cabe en 32
bits. Sin la `L`, el literal se trunca **en silencio** y sale un número plausible y
equivocado:

```basic
1754300000000L    // bien
1754300000000     // MAL: se convierte en 1953343232
```

**El apóstrofo.** Concatenar texto del usuario a mano parte la sentencia. Usa `Where`,
o `Orm.esc(...)` si escribes el SQL tú.

**Dónde vive la base.** En la placa, en la tarjeta: `/sd/medidas.db`. **No la pongas
en `resources/`**, que esa carpeta se copia al micro en cada ejecución y se
machacaría tu base con la del proyecto.

**El camino no puede venir por argumento.** Hoy el `arg` de `Main` llega siempre
vacío. Por eso los ejemplos ponen el trabajo en una función pública —
`correr(camino)` — y quien quiera otra base escribe un módulo de tres líneas que la
llame. Ver `samples/SqlDemoSd.bp`, que apunta a la tarjeta.

---

## Por dónde seguir

| quiero… | mira |
|---|---|
| SQL a pelo, los tres verbos | `samples/SqlDemo.bp` |
| el ORM con DAO escritos a mano | `samples/DaoDemo.bp` |
| el ORM con DAO generados | `samples/GenDemo.bp` |
| la misma base en la tarjeta | `samples/SqlDemoSd.bp` |
| cómo se construye el pack por dentro | `bpstdlib/sqlite/LEEME.md` |
