@echo off
cd /d "%~dp0"
echo =======================================================================
echo    Launching Interactive 3D Solar System and Universe Simulation
echo =======================================================================
echo Device: GPU (with automatic CPU fallback if GPU unavailable)
echo.
"%~dp0N-Body-Simulation.exe" %*
