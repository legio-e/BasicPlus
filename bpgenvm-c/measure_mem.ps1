# measure_mem.ps1 — mide working set peak de un proceso durante su
# ejecución. Útil para comparar VM Java vs VM C ejecutando el mismo
# .mod.

param(
    [string]$Label,
    [string]$Cmd,
    [string]$CmdArgs,
    [int]$SampleMs = 100
)

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName  = $Cmd
$psi.Arguments = $CmdArgs
$psi.UseShellExecute        = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError  = $true

$proc = [System.Diagnostics.Process]::Start($psi)

$stdout = $proc.StandardOutput.ReadToEndAsync()
$stderr = $proc.StandardError.ReadToEndAsync()

$peakWS = 0L
$peakPriv = 0L
$samples = 0

while (-not $proc.HasExited) {
    try {
        $proc.Refresh()
        $ws = $proc.WorkingSet64
        $priv = $proc.PrivateMemorySize64
        if ($ws -gt $peakWS) { $peakWS = $ws }
        if ($priv -gt $peakPriv) { $peakPriv = $priv }
        $samples++
    } catch { }
    Start-Sleep -Milliseconds $SampleMs
}

try {
    if ($proc.PeakWorkingSet64 -gt $peakWS) { $peakWS = $proc.PeakWorkingSet64 }
    if ($proc.PeakPagedMemorySize64 -gt $peakPriv) { $peakPriv = $proc.PeakPagedMemorySize64 }
} catch { }

$out = $stdout.Result
$err = $stderr.Result

$wsMB   = [math]::Round($peakWS / 1MB, 2)
$wsKB   = [math]::Round($peakWS / 1024, 0)
$prKB   = [math]::Round($peakPriv / 1024, 0)
Write-Host ("[{0}] peak_WS = {1:N0} KB  ({2} MB)   peak_private = {3:N0} KB   samples={4}   exit={5}" -f `
    $Label, $wsKB, $wsMB, $prKB, $samples, $proc.ExitCode)
if ($out -and $out.Trim()) { Write-Host ("  stdout: " + $out.Trim()) }
if ($err -and $err.Trim()) { Write-Host ("  stderr: " + $err.Trim()) }
