<#
.SYNOPSIS
    Universal Simulation Runner for OpenCL N-Body Universe Simulation
.DESCRIPTION
    Runs the simulation with any combination of execution device (GPU, CPU, CPUGPU),
    OpenCL kernels, OpenMP thread counts, SIMD vectorization, datasets, and frame limits.
    Parameters can be passed directly as arguments or via environment variables.

.EXAMPLE
    # Run fastest OpenCL GPU simulation
    .\run_simulation.ps1 -Device GPU -Kernel nbody.cl

    # Run on CPU with 8 OpenMP threads
    .\run_simulation.ps1 -Device CPU -Threads 8

    # Run benchmark for 60 frames and auto-exit
    .\run_simulation.ps1 -Device GPU -Kernel nbody.cl -Frames 60

    # Run scaled Wikipedia dataset with N=2048 bodies
    .\run_simulation.ps1 -Device GPU -Dataset Wikipedia -N 2048

    # Pass configuration using environment variables
    $env:DEVICE = "GPU"
    $env:KERNEL = "nbody_local.cl"
    $env:FRAMES = "30"
    .\run_simulation.ps1
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("GPU", "CPU", "CPUGPU", IgnoreCase = $true)]
    [string]$Device = $env:DEVICE,

    [Parameter(Position = 1)]
    [ValidateSet("nbody.cl", "nbody_local.cl", "nbody_async.cl", IgnoreCase = $true)]
    [string]$Kernel = $env:KERNEL,

    [Parameter(Position = 2)]
    [ValidateRange(1, 64)]
    [int]$Threads = $(if ($env:OMP_NUM_THREADS) { [int]$env:OMP_NUM_THREADS } elseif ($env:THREADS) { [int]$env:THREADS } else { 0 }),

    [Parameter(Position = 3)]
    [string]$Dataset = $(if ($env:DATASET) { $env:DATASET } else { "SolarSystem" }),

    [Parameter(Position = 4)]
    [int]$N = $(if ($env:N) { [int]$env:N } else { 0 }),

    [Parameter(Position = 5)]
    [int]$Frames = $(if ($env:FRAMES) { [int]$env:FRAMES } else { -1 }),

    [Parameter()]
    [switch]$Scalar = $false,

    [Parameter()]
    [switch]$Help = $false
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Get-Help $MyInvocation.MyCommand.Path -Full
    exit 0
}

# Resolve Defaults if not set
if ([string]::IsNullOrWhiteSpace($Device)) {
    $Device = "GPU"
}
if ([string]::IsNullOrWhiteSpace($Kernel)) {
    $Kernel = "nbody.cl"
}
if ($Threads -le 0) {
    $Threads = [Environment]::ProcessorCount
}

# Find appropriate executable
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$exeOpenMP = Join-Path $scriptDir "N-Body-Simulation.exe"
if (-not (Test-Path $exeOpenMP)) {
    $exeOpenMP = Join-Path $scriptDir "build\N-Body-Simulation.exe"
}
if (-not (Test-Path $exeOpenMP)) {
    $exeOpenMP = Join-Path $scriptDir "N-Body-Simulation-openmp-simd.exe"
}

$exeScalar = Join-Path $scriptDir "build-scalar\N-Body-Simulation.exe"
if (-not (Test-Path $exeScalar)) {
    $exeScalar = Join-Path $scriptDir "N-Body-Simulation-scalar.exe"
}

$targetExe = if ($Scalar) { $exeScalar } else { $exeOpenMP }

if (-not (Test-Path $targetExe)) {
    Write-Error "Executable not found at '$targetExe'. Please build the project first using 'cmake --build build'."
    exit 1
}

# Configure Environment
$env:OMP_NUM_THREADS = $Threads.ToString()

# Build Argument List
$simArgs = @(
    "--Device", $Device
)

if ($Device -eq "GPU" -or $Device -eq "CPUGPU") {
    $simArgs += @("--Kernel", $Kernel)
}

if ($Dataset -ne "SolarSystem") {
    $simArgs += @("--Dataset", $Dataset)
}

if ($N -gt 0) {
    $simArgs += @("--N", $N.ToString())
}

if ($Frames -gt 0) {
    $simArgs += @("--Frames", $Frames.ToString())
}

# Display Banner
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "       N-BODY UNIVERSE & SOLAR SYSTEM SIMULATION RUNNER                " -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ("  Executable : {0}" -f (Split-Path -Leaf $targetExe)) -ForegroundColor Gray
Write-Host ("  Device     : {0}" -f $Device) -ForegroundColor Yellow
if ($Device -ne "CPU") {
    Write-Host ("  GPU Kernel : {0}" -f $Kernel) -ForegroundColor Yellow
}
Write-Host ("  OpenMP     : {0} Active Threads (OMP_NUM_THREADS={0})" -f $Threads) -ForegroundColor Yellow
Write-Host ("  Dataset    : {0}" -f $Dataset) -ForegroundColor Green
if ($N -gt 0) {
    Write-Host ("  Bodies (N) : {0}" -f $N) -ForegroundColor Green
} else {
    Write-Host "  Bodies (N) : Default (~1,611 celestial bodies & asteroids)" -ForegroundColor Green
}
if ($Frames -gt 0) {
    Write-Host ("  Frames     : {0} (Auto-exit benchmark mode)" -f $Frames) -ForegroundColor Magenta
} else {
    Write-Host "  Mode       : Continuous Interactive Visualization" -ForegroundColor Magenta
}
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ("Executing: & `"{0}`" {1}" -f $targetExe, ($simArgs -join " ")) -ForegroundColor DarkGray
Write-Host ""

# Execute the simulation
& $targetExe @simArgs
$exitCode = $LASTEXITCODE

if ($exitCode -eq 0) {
    Write-Host "`nSimulation completed successfully (Exit Code 0)." -ForegroundColor Green
} else {
    Write-Host "`nSimulation exited with code $exitCode." -ForegroundColor Red
}

exit $exitCode
