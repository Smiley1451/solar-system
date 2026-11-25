@echo off
title Solar System Benchmark Dashboard & Execution Server
cd /d "%~dp0"
echo ==============================================================================
echo   SOLAR SYSTEM BENCHMARK DASHBOARD & LIVE EXECUTION SERVER
echo ==============================================================================
echo Starting local benchmark server on http://localhost:5050...
python benchmark_server.py
pause
