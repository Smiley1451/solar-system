# Interactive 3D Solar System and Universe N-Body Simulation

A high-performance, real-time 3D universe and solar system visualization built on top of an OpenCL/OpenGL gravitational N-body simulation engine.

![Solar System 3D Visualization](doc/nbody.gif)

## Key Features

- **Real Gravitational N-Body Physics:**
  All celestial bodies move strictly according to numerical gravitational physics computed via hardware-accelerated **OpenCL GPU** kernels or parallel **CPU OpenMP/SIMD** solvers. No fake hardcoded orbital paths!
- **Photorealistic 3D Celestial Bodies:**
  - **Sun:** Luminous central star with procedural solar surface turbulence, limb darkening, and radiant corona glow.
  - **8 Major Planets + Pluto:** Procedural textures featuring Earth's blue oceans, continents, and cloud cover; Jupiter's atmospheric bands and Great Red Spot; Mars's rusty terrain and polar caps; Venus's dense atmosphere; Mercury's cratered surface; Uranus and Neptune ice giants; and Pluto dwarf planet.
  - **Saturn's Ring System:** Double-sided tilted ring disc (~27° inclination) with realistic inner/outer radius and dark Cassini division.
  - **Comet 1P/Halley:** Highly eccentric orbit with dynamic luminous tail facing away from the Sun, growing longer and brighter near perihelion.
- **Asteroid Belt & Kuiper Belt:**
  Over 1,000 individually simulated asteroids and Kuiper belt objects orbiting stably between Mars and Jupiter and beyond Neptune.
- **Deep Space Environment:**
  - 4,500-star background starfield with realistic stellar temperatures (blue-white to red giants) and subtle twinkling.
  - Delicate cyan reference orbits and historical gravitational motion trails.
  - Billboard labels identifying planets and major belts.
  - Comprehensive HUD overlay displaying FPS, calculation time, simulation speed, pause status, and target focus.

---

## Interactive Controls

| Key / Input | Action |
| :--- | :--- |
| **`0` – `9`** | Focus camera on **Sun (`0`)**, **Mercury (`1`)** .. **Pluto (`9`)** |
| **`C`** | Focus camera on **Comet 1P/Halley** |
| **`Z`** | Reset camera to full **Solar System Overview** |
| **`Space`** | **Pause / Resume** physical simulation |
| **`+` / `=`** | **Increase simulation speed** (1x, 2x, 4x, 8x, 16x) |
| **`-` / `_`** | **Decrease simulation speed** |
| **`T`** | Toggle **Motion Trails** |
| **`O`** | Toggle **Reference Orbits** |
| **`L`** | Toggle **Planet Labels** |
| **`B`** | Toggle **Background Starfield** |
| **`H`** | Toggle **HUD Overlay** |
| **`R`** | Toggle **Automatic 360° Camera Orbit** |
| **`F`** | Toggle **Fullscreen Mode** |
| **`Esc`** | Exit Application |
| **Left Mouse Drag** | Orbit camera smoothly around current focus target (360° yaw & pitch) |
| **Mouse Wheel** | Zoom camera in and out smoothly |

---

## How to Run (Zero Setup Required)

No compilers or software installations are needed! All 64-bit binaries and runtime libraries are pre-packaged.

### Option 1: One-Click Launchers (Easiest)
- **Double-click `run.bat`** (or `N-Body-Simulation.exe`) to launch GPU mode with automatic CPU fallback.
- **Double-click `run_cpu.bat`** to run purely on CPU with multi-core OpenMP parallel physics.

### Option 2: Command Line / Terminal
```powershell
# Run GPU accelerated mode
.\N-Body-Simulation.exe

# Run CPU parallel mode
.\N-Body-Simulation.exe --Device CPU
```

### Run with Extra Asteroids / Kuiper Belt Bodies
```powershell
.\N-Body-Simulation.exe --Device GPU --N 2000
```

### Run Legacy Dataset
```powershell
.\N-Body-Simulation.exe --Dataset Wikipedia --Device GPU
```

### Run Benchmark Mode
```powershell
.\N-Body-Simulation.exe --Benchmark SHORT --Device GPU
```

---

## Command Line Options

- `--Device`: Simulation execution backend (`GPU`, `CPU`, or `CPUGPU`)
- `--Dataset`: Dataset preset (`SolarSystem` by default, or `Wikipedia`)
- `--N`: Additional celestial bodies to add to asteroid and Kuiper belts
- `--Benchmark`: Run automated performance benchmark (`SHORT` or `LONG`)
- `--CL_Kernel_Path`: Path to OpenCL kernel files (defaults to `../kernels/` or `./kernels/`)
- `--Kernel`: Specific OpenCL kernel file to use (`nbody.cl`, `nbody_local.cl`, or `nbody_async.cl`)

---

## Running with the Universal Runner Script (`run_simulation.ps1`)

A dedicated PowerShell script is provided to easily run any execution combination:

```powershell
# Run fastest GPU configuration (Default)
.\run_simulation.ps1

# Run on CPU with 8 OpenMP threads
.\run_simulation.ps1 -Device CPU -Threads 8

# Run specific OpenCL kernel (Global, Local, Async DMA)
.\run_simulation.ps1 -Device GPU -Kernel nbody_local.cl

# Run benchmark for 60 frames and auto-exit
.\run_simulation.ps1 -Device GPU -Kernel nbody.cl -Frames 60

# Run with environment variables
$env:DEVICE = "GPU"; $env:KERNEL = "nbody.cl"; .\run_simulation.ps1
```

---

## Performance & Architecture Dashboard

Open [`dashboard.html`](dashboard.html) or [`index.html`](index.html) in any web browser to view the interactive performance dashboard:
- **Real-time benchmark comparison charts** (Latency, Gravitational Throughput, Problem Size Scaling, and Thread Scaling).
- **Searchable and sortable performance data table** across all 18 test runs.
- **Interactive PowerShell command generator** with one-click copy.
- **Hardware profiling & architectural insights** (Intel Core i5-8350U & Intel UHD Graphics 620).

### Key Performance Highlights:
- **GPU Speedup:** OpenCL GPU achieves **35.39× speedup** over single-threaded scalar CPU execution (**0.56 ms** vs **19.94 ms**).
- **Peak Throughput:** **5,621 Mega-Interactions / sec** on Intel UHD Graphics 620.
- **CPU Multi-Core Scaling:** **2.92× multi-core scaling** with 8 OpenMP threads (**5.93 ms** per step).
- **SIMD Vectorization:** **15% performance gain** via 128-bit SSE vectorization.

---

## Building from Source

### Windows (GCC / MinGW / Ninja)
Dependencies are bundled or auto-configured with CMake:
```powershell
cmake -B build -G "Ninja"
cmake --build build
```

### Linux
```bash
cmake -B build
cmake --build build
```