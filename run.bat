@echo off
setlocal
cd /d "%~dp0"
echo Launching N-Body Simulation on GPU...
echo Controls: [Esc]=Exit, [R]=Auto-rotate, [F]=Fullscreen, Mouse=Pan/Zoom
N-Body-Simulation.exe --N 1000 --Device GPU --Kernel nbody.cl --CL_Kernel_Path kernels/
