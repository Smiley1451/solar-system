@echo off
title Solar System N-Body Simulation - Native Side-by-Side 3D VR
cd /d "%~dp0"
echo ========================================================================
echo   N-BODY SOLAR SYSTEM -- NATIVE STEREOSCOPIC 3D & HEAD TRACKING
echo ========================================================================
echo Controls:
echo   [V]      Toggle Side-by-Side (SBS) Stereoscopic 3D / Monoscopic
echo   [Space]  Pause / Resume
echo   [+] / [-] Speed Multiplier (0.125x - 8x)
echo   [[] / []] Adjust Eye Separation (IPD)
echo   [0 - 9]  Focus Camera on Sun / Planet
echo   [R]      Reset Camera View
echo   [ESC]    Exit
echo ========================================================================
start "" "%~dp0N-Body-VR.exe"
