@echo off
cd /d "%~dp0"
echo =======================================================================
echo    Launching Interactive 3D Solar System Simulation (CPU Mode)
echo =======================================================================
echo Device: CPU Multi-Core Parallel (OpenMP + SIMD SSE)
echo.
"%~dp0N-Body-Simulation.exe" --Device CPU %*
