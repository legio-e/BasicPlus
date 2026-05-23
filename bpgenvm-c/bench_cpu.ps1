# bench_cpu.ps1 — benchmark de tiempo de ejecución VM C vs VM Java.
# Ejecuta cada VM N veces sobre el mismo .mod CPU-bound (fib recursivo)
# y reporta min / median / mean.

param(
    [int]$Runs = 5,
    [string]$Mod = "samples/BenchCpu.mod"
)

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
        $out = $proc.StandardOutput.ReadToEnd()
        $err = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()
        $sw.Stop()
        $ms = $sw.Elapsed.TotalMilliseconds
        $times += $ms
        Write-Host ("  [{0}] run {1}: {2:N1} ms" -f $label, ($i+1), $ms)
    }
    $sorted = $times | Sort-Object
    $minMs  = $sorted[0]
    $maxMs  = $sorted[-1]
    $meanMs = ($times | Measure-Object -Average).Average
    $medMs  = if ($Runs % 2 -eq 1) { $sorted[[int]([math]::Floor($Runs/2))] } else { ($sorted[$Runs/2-1] + $sorted[$Runs/2]) / 2 }
    Write-Host ("[{0}] min={1:N1} ms  median={2:N1} ms  mean={3:N1} ms  max={4:N1} ms" -f `
        $label, $minMs, $medMs, $meanMs, $maxMs)
    return [pscustomobject]@{
        Label  = $label
        Min    = $minMs
        Median = $medMs
        Mean   = $meanMs
        Max    = $maxMs
    }
}

Write-Host ("=== Benchmark CPU-bound: {0} ({1} runs c/u) ===" -f $Mod, $Runs)

# Warm-up: 1 run extra para calentar caches y JIT.
Write-Host "-- warmup --"
& "C:/lenguajes/pm/bpgenvm-c/build/bpgenvm-c.exe" $Mod 2>&1 | Out-Null
& java -jar C:/lenguajes/pm/miVM/target/bpgenvm-1.0.jar $Mod 2>&1 | Out-Null

Write-Host "-- VM-C --"
$rC = Time-Run "VM-C" "C:/lenguajes/pm/bpgenvm-c/build/bpgenvm-c.exe" $Mod

Write-Host "-- VM-Java --"
$rJ = Time-Run "VM-Java" "java" ("-jar C:/lenguajes/pm/miVM/target/bpgenvm-1.0.jar " + $Mod)

Write-Host ""
Write-Host "=== Resultados ==="
Write-Host ("{0,-10} {1,12} {2,12} {3,12} {4,12}" -f "VM", "min(ms)", "median(ms)", "mean(ms)", "max(ms)")
Write-Host ("{0,-10} {1,12:N1} {2,12:N1} {3,12:N1} {4,12:N1}" -f $rC.Label, $rC.Min, $rC.Median, $rC.Mean, $rC.Max)
Write-Host ("{0,-10} {1,12:N1} {2,12:N1} {3,12:N1} {4,12:N1}" -f $rJ.Label, $rJ.Min, $rJ.Median, $rJ.Mean, $rJ.Max)
if ($rC.Median -gt 0) {
    $ratio = $rJ.Median / $rC.Median
    Write-Host ("`nRatio VM-Java / VM-C (median): {0:N2}x" -f $ratio)
}
