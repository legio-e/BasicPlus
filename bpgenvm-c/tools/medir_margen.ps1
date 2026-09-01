# medir_margen.ps1 — protocolo de medida del MARGEN DEL SISTEMA (V6/U6).
#
# QUE MIDE: lo que consume EL SISTEMA (no el programa BP) mientras la placa
# trabaja — tareas que se crean, buffers que se piden, FS, trafico del wire. Es
# el numero del que sale el MARGEN: cuanta DRAM hay que dejarle a lo que NO es
# la VM. Sale de restar el minimo historico (heap_caps_get_minimum_free_size) a
# lo que habia libre justo tras reservar el bloque de la VM.
#
# POR QUE EXISTE ESCRITO. El S3 tenia 86 KB medidos (#336) y el C3 17,5 KB, y la
# diferencia era indefendible porque las dos medidas se tomaron con CARGAS
# DISTINTAS: la del S3 dice solo «tras varios RUN», sin decir cuales. Dos numeros
# del mismo instrumento no se pueden restar si no se sabe que se ejecuto.
#
#   1. LOG_CLEAR y LUEGO reset  (arranque limpio, y log SOLO de esta pasada)
#   2. PUT de un fichero        (ejercita el buffer de bulk del wire)
#   3. RUN x2                   (el MISMO programa en las dos placas)
#   4. LIST                     (recorre el FS entero)
#   5. LOG_DUMP                 (la linea `mem:` de cada RUN)
#
# EL ORDEN DEL PASO 1 IMPORTA, y la primera version lo tenia al reves. El log es
# ACUMULATIVO entre arranques: sin limpiar salen lineas `mem:` de sesiones viejas
# con valores parecidos —una medida que no desempata—, pero limpiando DESPUES del
# reset se pierden las lineas del propio arranque, que traen «libre tras
# reservar», o sea la mitad del dato.
#
# ── LO QUE CAMBIA POR PLACA, y por que hay dos interruptores ─────────────────
#
#  -Rts      El C3 habla por su USB-Serial-JTAG NATIVO, donde un pulso de RTS
#            mete el chip en MODO DESCARGA y deja de contestar (medio dia
#            perdido con eso el 31-ago). El S3 habla por un puente USB-UART con
#            el circuito clasico de dos transistores, donde hay que dejar DTR y
#            RTS AMBOS afirmados para soltar EN e IO0: con uno solo, muda.
#
#  -Reabre   Al resetear, el USB nativo del C3 se re-enumera y el handle del
#            puerto MUERE: hay que cerrar, esperar y volver a abrir. El puente
#            del S3 no se entera, y reabrir alli es contraproducente porque cada
#            apertura vuelve a mover DTR/RTS y la placa se queda muda.
#
# Uso:   .\medir_margen.ps1 -Port COM3 -Reabre     # C3 (USB nativo)
#        .\medir_margen.ps1 -Port COM9 -Rts        # S3 (puente CH343)
param(
    [string]$Port   = "COM3",
    [switch]$Rts,
    [switch]$Reabre,
    [string]$Mod    = "C:\lenguajes\pm\samples\benchmarks\BenchMem.mod",
    [string]$Correr = "/app/Bench.mod"
)

function Nueva-Conexion($p) {
    $s = New-Object System.IO.Ports.SerialPort $p,115200,None,8,one
    $s.ReadTimeout = 6000
    $s.Open()
    if ($Rts) {
        $s.DtrEnable = $false; $s.RtsEnable = $true    # EN abajo = reset
        Start-Sleep -Milliseconds 300
        $s.DtrEnable = $true;  $s.RtsEnable = $true    # las dos sueltas = corre
        Start-Sleep -Seconds 4
    } else {
        $s.DtrEnable = $true
        Start-Sleep -Milliseconds 600
    }
    $s.DiscardInBuffer()
    return $s
}
function Pide($s, $json, $esperaSeg = 8, $marca = $null) {
    $s.Write($json + "`n")
    $sb  = New-Object System.Text.StringBuilder
    $fin = (Get-Date).AddSeconds($esperaSeg)
    while ((Get-Date) -lt $fin) {
        $null = $sb.Append($s.ReadExisting())
        if ($marca -and $sb.ToString() -match $marca) { break }
        Start-Sleep -Milliseconds 150
    }
    return $sb.ToString()
}

# ── 1. limpiar el log y LUEGO resetear ──────────────────────────────────────
$s = Nueva-Conexion $Port
Write-Output "1. LOG_CLEAR -> $((Pide $s '{""type"":""LOG_CLEAR"",""id"":9}' 4).Trim())"
Write-Output "2. RESET     -> $((Pide $s '{""type"":""RESET"",""id"":1}' 3).Trim())"
if ($Reabre) {
    $s.Close()
    Write-Output "   (el USB nativo se re-enumera: cerrando y esperando)"
    Start-Sleep -Seconds 6
    $s = Nueva-Conexion $Port
} else {
    Start-Sleep -Seconds 5
    $s.DiscardInBuffer()
}

# ── 2. PUT ──────────────────────────────────────────────────────────────────
$b   = [System.IO.File]::ReadAllBytes($Mod)
$hdr = [System.Text.Encoding]::ASCII.GetBytes(
        '{"type":"PUT","id":2,"path":"/app/_margen.mod","bulk":' + $b.Length + '}' + "`n")
$s.Write($hdr, 0, $hdr.Length); $s.Write($b, 0, $b.Length)
Start-Sleep -Milliseconds 2500
Write-Output "3. PUT ($($b.Length) B) -> $($s.ReadExisting().Trim())"

# ── 3. dos RUN del MISMO programa ───────────────────────────────────────────
foreach ($n in 1, 2) {
    $r   = Pide $s ('{"type":"RUN","id":' + (10 + $n) + ',"path":"' + $Correr + '"}') 60 '"type":"EXITED"'
    $sal = ($r -split "`n" | Select-String 'EXITED') -join ' '
    Write-Output "4.$n RUN -> $sal"
}

# ── 4. LIST ─────────────────────────────────────────────────────────────────
$r = Pide $s '{"type":"LIST","id":20}' 12 '"omitted"|"omitidas"'
Write-Output "5. LIST -> $($r.Length) bytes de respuesta"

# ── 5. el log de esta pasada ────────────────────────────────────────────────
$r = Pide $s '{"type":"LOG_DUMP","id":21}' 12
$s.Close()
Write-Output "6. LOG (solo esta pasada):"
($r -split '\\n') | Select-String 'vm: heap|mem: DRAM|fin de RUN' | ForEach-Object { "   $_" }
