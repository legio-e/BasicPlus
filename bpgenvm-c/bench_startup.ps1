# bench_startup.ps1 — mide overhead de arranque (Hello.mod ≈ 0 trabajo).
# Sirve para restar al benchmark CPU y obtener el tiempo neto de fib(30).
param([int]$Runs = 5)

function Time-Run($label, $cmd, $cmdArgs) {
    $times = @()
    for ($i = 0; $i -lt $Runs; $i++) {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName  = $cmd
        $psi.Arguments = $cmdArgs
        $psi.UseShellExecute        = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError  = $true
        $proc = [System.Diagnostics.Process]::Start($psi)
        $null = $proc.StandardOutput.ReadToEnd()
        $null = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()
        $sw.Stop()
        $times += $sw.Elapsed.TotalMilliseconds
    }
    $sorted = $times | Sort-Object
    $minMs  = $sorted[0]
    $medMs  = if ($Runs % 2 -eq 1) { $sorted[[int]([math]::Floor($Runs/2))] } else { ($sorted[$Runs/2-1] + $sorted[$Runs/2]) / 2 }
    Write-Host ("[{0}] min={1:N1} ms  median={2:N1} ms" -f $label, $minMs, $medMs)
    return [pscustomobject]@{ Label=$label; Min=$minMs; Median=$medMs }
}

Write-Host ("=== Startup overhead (Hello.mod, {0} runs) ===" -f $Runs)
& "C:/lenguajes/pm/bpgenvm-c/build/bpgenvm-c.exe" "samples/Hello.mod" | Out-Null
& java -jar C:/lenguajes/pm/miVM/target/bpgenvm-1.0.jar "samples/Hello.mod" | Out-Null

$sC = Time-Run "VM-C startup"    "C:/lenguajes/pm/bpgenvm-c/build/bpgenvm-c.exe" "samples/Hello.mod"
$sJ = Time-Run "VM-Java startup" "java" "-jar C:/lenguajes/pm/miVM/target/bpgenvm-1.0.jar samples/Hello.mod"
