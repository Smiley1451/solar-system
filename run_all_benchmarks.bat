@echo off
title Solar System N-Body Simulation - Parallel Architecture Benchmark
cd /d "%~dp0"
echo ==============================================================================
echo       SOLAR SYSTEM N-BODY SIMULATION -- AUTOMATED BENCHMARK SUITE
echo ==============================================================================
echo Running all available architecture combinations (CPU, OpenMP, SIMD, GPU, Hybrid)...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0benchmark_runner.ps1"
pause
