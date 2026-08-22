# Propuesta del modelo de memoria — estudio previo (mayo–julio 2026)

> **Qué es esto.** El trabajo de diseño que precedió al cambio de modelo de
> memoria de BasicPlus: especificación, verificación y una maqueta ejecutable.
> Es **material histórico**, no documentación del producto. Se publica porque
> explica el *porqué* de decisiones que hoy están en el código, y ese porqué no
> cabe en un fichero de diseño escrito después.
>
> ⚠️ **No es una fuente de verdad.** Lo que hay HOY se documenta en
> [`docs/HEAP_LAYOUT.md`](../docs/HEAP_LAYOUT.md) y el estado vive en
> [`docs/FICHAS.md`](../docs/FICHAS.md). Si algo de aquí los contradice, mandan
> ellos: esto es una foto de lo que se pensaba antes de construirlo.

## En qué quedó

El estudio desembocó en **H1 de V4 — el modelo de handles**: una referencia dejó
de ser una dirección y pasó a ser un handle con contador de generación, de modo
que usar un objeto liberado falla *ahí mismo* en vez de corromper en silencio.
Eso está cerrado y en producción desde V4. La campaña de auditoría que vino
detrás tiene su propio registro en [`docs/V4_REF_AUDIT.md`](../docs/V4_REF_AUDIT.md).

Lo que el contacto con el código cambió no fue el destino sino **el método**.
`plan-h1.2a-ensanchado-refs.md` parte de un inventario de los sitios donde había
4 bytes, para irlos ensanchando; lo que acabó funcionando fue lo contrario:
introducir una **abstracción de referencia** (`readRef`/`writeRef`, un `REF_SIZE`
único) de modo que el ancho se decidiera en tres sitios y no en diecisiete. El
plan por etapas —ensanchado plano primero, tabla de handles después— sí se
respetó. También eso es parte de lo que cuenta un archivo histórico.

## Qué hay dentro

| carpeta | qué contiene |
|---|---|
| `01-especificacion/` | la semántica de `owner` (destrucción, aliasing) |
| `02-verificacion/` | plan de validación, cross-check contra el código, *spike* del coste de los handles y una **maqueta en Java** que se ejecuta |
| `03-arquitectura/` | el `ADR-0001` (allocator no-moving + compactación diferida), el análisis del modelo, el objetivo SMP y el plan de ensanchado |

## Enlaces que no llevan a ninguna parte — y por qué

Varios documentos citan una carpeta `hallazgos/` con un catálogo numerado
(`H-001` … `H-010`) y una carpeta `fuentes/`. **Ninguna de las dos está aquí ni
lo estuvo nunca**: eran material de trabajo, fuera del repositorio, y se
perdieron. Se deja la cita en su sitio en vez de borrarla porque nombra la
evidencia sobre la que se razonaba; pero no la busques, no está.
