# N-Body Simulation Runner Script
param (
    [int]$N = 1000,
    [string]$Device = "GPU",
    [string]$Kernel = "nbody.cl",
    [string]$Distribution = "normal"
)

$exePath = Join-Path $PSScriptRoot "N-Body-Simulation.exe"

if (-not (Test-Path $exePath)) {
    $exePath = Join-Path $PSScriptRoot "build\N-Body-Simulation.exe"
}

if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found. Please build the project first."
    exit 1
}

Write-Host "Launching N-Body Simulation:" -ForegroundColor Cyan
Write-Host "  Bodies:       $N"
Write-Host "  Device:       $Device"
Write-Host "  Kernel:       $Kernel"
Write-Host "  Distribution: $Distribution"
Write-Host ""
Write-Host "Controls:" -ForegroundColor Yellow
Write-Host "  [Esc] : Close simulation"
Write-Host "  [R]   : Toggle automatic camera rotation"
Write-Host "  [F]   : Toggle fullscreen"
Write-Host "  Mouse Drag  : Rotate/pan view"
Write-Host "  Mouse Scroll: Zoom in/out"
Write-Host ""

& $exePath --N $N --Device $Device --Kernel $Kernel --Random_Initialization $Distribution --CL_Kernel_Path "$PSScriptRoot/kernels/"
