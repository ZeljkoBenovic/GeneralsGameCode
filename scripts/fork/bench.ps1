<#
.SYNOPSIS
Benchmarks a game executable: rendered replay frame timings and headless simulation time.

.EXAMPLE
./bench.ps1 -GameDir "D:\Games\Zero Hour" -Exe generalszh_enhanced.exe -Label enhanced
#>
param(
    [Parameter(Mandatory)] [string]$GameDir,
    [Parameter(Mandatory)] [string]$Exe,
    [string]$Label = $Exe,
    [string]$RenderReplay = "12-11-35_2v2_babai_ILnur_HardAI_HardAI.rep",
    [int]$RenderSeconds = 60,
    [int]$RenderStartMinute = 20,
    [string[]]$SimReplays = @("12-11-35_2v2_babai_ILnur_HardAI_HardAI.rep", "15-07-24_2v2v2_Emkill_haker_HardAI_HardAI_HardAI_HardAI.rep"),
    [int]$Runs = 3,
    [string]$Resolution = "2560x1440",
    [switch]$SkipRender,
    [switch]$SkipSim
)

$ErrorActionPreference = "Stop"
$repo = Resolve-Path "$PSScriptRoot\..\.."
$src = "$repo\GeneralsReplays\GeneralsZH\1.04"
$user = "$env:USERPROFILE\Documents\Command and Conquer Generals Zero Hour Data"
$stage = "$user\Replays\_bench"
$results = "$repo\build\bench-results.csv"

# Stage the replays and their maps. -LiteralPath because map folder names contain [brackets].
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item "$src\Replays\*" $stage -Force
$stagedMaps = @()
foreach ($map in Get-ChildItem -LiteralPath "$src\Maps" -Directory) {
    $dest = Join-Path "$user\Maps" $map.Name
    if (-not (Test-Path -LiteralPath $dest)) { Copy-Item -LiteralPath $map.FullName -Destination $dest -Recurse; $stagedMaps += $dest }
}

function Add-Result($benchmark, $metric, $value) {
    if (-not (Test-Path $results)) { "date,label,exe,benchmark,metric,value" | Set-Content $results }
    "$(Get-Date -Format s),$Label,$Exe,$benchmark,$metric,$value" | Add-Content $results
}

try {
    $xres, $yres = $Resolution -split "x"
    $exePath = Join-Path $GameDir $Exe

    if (-not $SkipRender) {
        for ($i = 1; $i -le $Runs; $i++) {
            Remove-Item "$user\FrameStats.txt" -ErrorAction SilentlyContinue
            $gameArgs = "-win -xres $xres -yres $yres -replay _bench/$RenderReplay -frameStats $RenderSeconds $RenderStartMinute"
            $p = Start-Process -FilePath $exePath -ArgumentList $gameArgs -WorkingDirectory $GameDir -PassThru
            if (-not $p.WaitForExit(($RenderSeconds + 600) * 1000)) { Stop-Process -Id $p.Id -Force; throw "render run $i timed out" }
            if (-not (Test-Path "$user\FrameStats.txt")) { throw "render run $i wrote no FrameStats.txt (exit $($p.ExitCode))" }
            $stats = @{}
            Get-Content "$user\FrameStats.txt" | ForEach-Object { $k, $v = $_ -split "\s+", 2; $stats[$k] = $v.Trim() }
            "[$Label] render run $i : fps_avg=$($stats.fps_avg) fps_1pct_low=$($stats.fps_1pct_low) frame_ms_p99=$($stats.frame_ms_p99) logic_ms_avg=$($stats.logic_ms_avg)"
            foreach ($k in "fps_avg", "fps_1pct_low", "frame_ms_avg", "frame_ms_p99", "logic_ms_avg", "logic_ms_p99") { Add-Result "render" $k $stats[$k] }
        }
    }

    if (-not $SkipSim) {
        for ($i = 1; $i -le $Runs; $i++) {
            $replayArgs = ($SimReplays | ForEach-Object { "-replay `"_bench/$_`"" }) -join " "
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $p = Start-Process -FilePath $exePath -ArgumentList "-headless -jobs 1 -frameStats 1 $replayArgs" -WorkingDirectory $GameDir -PassThru -RedirectStandardOutput "$repo\build\bench-sim.log"
            if (-not $p.WaitForExit(30 * 60 * 1000)) { Stop-Process -Id $p.Id -Force; throw "sim run $i timed out" }
            $seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1)
            "[$Label] sim run $i : $seconds s (exit $($p.ExitCode))"
            Add-Result "sim" "seconds" $seconds
        }
    }
}
finally {
    Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
    foreach ($m in $stagedMaps) { Remove-Item -LiteralPath $m -Recurse -Force }
}
