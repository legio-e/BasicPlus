# U6 — protocolo de medida del margen del sistema (V6/U6), repetible en cualquier placa ESP32.
#
# QUE MIDE: lo que consume EL SISTEMA (no el programa BP) mientras la placa
# trabaja — tareas que se crean, buffers que se piden, FS, trafico del wire. Es
# el numero del que sale el MARGEN: cuanta DRAM hay que dejarle a lo que no es
# la VM. Sale de restar el minimo historico de heap_caps_get_minimum_free_size
# a lo que habia libre justo tras reservar el bloque de la VM.
#
# POR QUE EXISTE ESCRITO. El S3 tenia 86 KB medidos (#336) y el C3 17,5 KB, y la
# diferencia era indefendible porque las dos medidas se tomaron con CARGAS
# DISTINTAS: la del S3 dice solo «tras varios RUN», sin decir cuales. Dos
# numeros del mismo instrumento no se pueden restar si no se sabe que se ejecuto.
# Aqui la secuencia esta escrita, asi que las placas se comparan de verdad.
#
#   1. LOG_CLEAR y LUEGO reset  (arranque limpio y log solo de esta pasada)
#   2. PUT de un fichero        (ejercita el buffer de bulk del wire)
#   3. RUN Bench x2             (el MISMO programa en las dos placas)
#   4. LIST                     (recorre el FS entero)
#   5. LOG_DUMP                 (la linea `mem:` de cada RUN)
#
# Uso:  .\medir_margen.ps1 -Port COM3
param([string]$Port = "COM3")

function Nueva-Conexion($p) {
    $s = New-Object System.IO.Ports.SerialPort $p,115200,None,8,one
    $s.ReadTimeout = 5000
    $s.Open(); $s.DtrEnable = $true
    Start-Sleep -Milliseconds 500
    $s.DiscardInBuffer()
    return $s
}
function Pide($s, $json, $esperaSeg = 8, $marca = $null) {
    $s.Write($json + "`n")
    $sb = New-Object System.Text.StringBuilder
    $fin = (Get-Date).AddSeconds($esperaSeg)
    while ((Get-Date) -lt $fin) {
        $null = $sb.Append($s.ReadExisting())
        if ($marca -and $sb.ToString() -match $marca) { break }
        Start-Sleep -Milliseconds 150
    }
    return $sb.ToString()
}

# ── 1. LOG_CLEAR y LUEGO reset ─────────────────────────────────────────────
#
# ESTE ORDEN IMPORTA, y la primera version lo tenia al reves. El log es
# ACUMULATIVO entre arranques: sin limpiar salen lineas `mem:` de sesiones viejas
# mezcladas con las de ahora y con valores parecidos — una medida que no
# desempata. Pero limpiando DESPUES del reset se pierden las lineas del propio
# arranque, que son las que traen «libre tras reservar», o sea la mitad del dato.
#
# Limpiar primero y resetear despues deja el volcado con EXACTAMENTE esta pasada:
# el arranque completo y un `mem:` por RUN.
$s = Nueva-Conexion $Port
Write-Output "1. LOG_CLEAR -> $(Pide $s '{""type"":""LOG_CLEAR"",""id"":9}' 4)"
Write-Output "2. RESET     -> $(Pide $s '{""type"":""RESET"",""id"":1}' 3)"
$s.Close()
Write-Output "   (esperando a que el puerto vuelva a enumerar)"
Start-Sleep -Seconds 6

# ── 2. PUT ─────────────────────────────────────────────────────────────────
$s = Nueva-Conexion $Port
$b = [System.IO.File]::ReadAllBytes("C:\lenguajes\pm\samples\benchmarks\BenchMem.mod")
$hdr = [System.Text.Encoding]::ASCII.GetBytes('{"type":"PUT","id":2,"path":"/app/BenchMem.mod","bulk":' + $b.Length + '}' + "`n")
$s.Write($hdr, 0, $hdr.Length); $s.Write($b, 0, $b.Length)
Start-Sleep -Milliseconds 2000
Write-Output "2. PUT ($($b.Length) B) -> $($s.ReadExisting())"

# ── 3. dos RUN del MISMO programa ──────────────────────────────────────────
foreach ($n in 1, 2) {
    $r = Pide $s ('{"type":"RUN","id":' + (10 + $n) + ',"path":"/app/Bench.mod"}') 40 '"type":"EXITED"'
    $sal = ($r -split "`n" | Select-String 'EXITED|317811') -join ' | '
    Write-Output "3.$n RUN Bench -> $sal"
}

# ── 4. LIST ────────────────────────────────────────────────────────────────
$r = Pide $s '{"type":"LIST","id":20}' 10 '"omitted"'
Write-Output "4. LIST -> $(($r.Length)) bytes de respuesta"

# ── 5. el log ──────────────────────────────────────────────────────────────
$r = Pide $s '{"type":"LOG_DUMP","id":21}' 12
$s.Close()
Write-Output "5. LOG:"
($r -split '\\n') | Select-String 'vm: heap|mem: DRAM|fin de RUN' | ForEach-Object { "   $_" }
