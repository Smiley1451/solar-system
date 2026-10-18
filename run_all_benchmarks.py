"""
Comprehensive N-Body Simulation Benchmark Runner
Tests all combinations: OpenCL GPU kernels, CPU OpenMP thread scaling, SIMD vectorization, and CPU-GPU hybrid.
Measures execution time, frame rate, computational throughput, CPU utilization, and RAM usage.
"""

import os
import re
import sys
import time
import json
import csv
import subprocess
import ctypes
from ctypes import wintypes
from datetime import datetime

# Windows API for process memory and CPU utilization
kernel32 = ctypes.windll.kernel32
psapi = ctypes.windll.psapi

class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
    _fields_ = [
        ('cb', wintypes.DWORD),
        ('PageFaultCount', wintypes.DWORD),
        ('PeakWorkingSetSize', ctypes.c_size_t),
        ('WorkingSetSize', ctypes.c_size_t),
        ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
        ('QuotaPagedPoolUsage', ctypes.c_size_t),
        ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
        ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
        ('PagefileUsage', ctypes.c_size_t),
        ('PeakPagefileUsage', ctypes.c_size_t),
    ]

class FILETIME(ctypes.Structure):
    _fields_ = [('dwLowDateTime', wintypes.DWORD), ('dwHighDateTime', wintypes.DWORD)]

def ft_to_sec(ft):
    val = (ft.dwHighDateTime << 32) | ft.dwLowDateTime
    return val / 10000000.0

NUM_LOGICAL_CORES = os.cpu_count() or 8
EXECUTABLE_OPENMP = os.path.abspath("N-Body-Simulation-openmp-simd.exe")
EXECUTABLE_SCALAR = os.path.abspath("N-Body-Simulation-scalar.exe")

def run_test(name, exe_path, args, env_overrides=None, num_frames=30):
    env = os.environ.copy()
    if env_overrides:
        env.update(env_overrides)

    cmd = [exe_path, "--Frames", str(num_frames)] + args
    print(f"\n>>> Running: {name}")
    print(f"    Command: {' '.join(cmd)}")
    if env_overrides:
        print(f"    Env: {env_overrides}")

    start_wall = time.time()
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
    
    stdout, _ = proc.communicate()
    end_wall = time.time()
    total_wall_time = max(0.001, end_wall - start_wall)

    # Process resource consumption via Windows API
    peak_ram_mb = 0.0
    cpu_time_s = 0.0
    cpu_util_pct = 0.0
    try:
        h_proc = wintypes.HANDLE(proc._handle)
        pmc = PROCESS_MEMORY_COUNTERS()
        pmc.cb = ctypes.sizeof(pmc)
        if psapi.GetProcessMemoryInfo(h_proc, ctypes.byref(pmc), pmc.cb):
            peak_ram_mb = round(pmc.PeakWorkingSetSize / (1024 * 1024), 2)
        
        c, e, k, u = FILETIME(), FILETIME(), FILETIME(), FILETIME()
        if kernel32.GetProcessTimes(h_proc, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u)):
            user_s = ft_to_sec(u)
            kernel_s = ft_to_sec(k)
            cpu_time_s = round(user_s + kernel_s, 3)
            # Aggregate CPU %: 100% means 1 core fully loaded, 400% means 4 cores fully loaded
            cpu_util_pct = round((cpu_time_s / total_wall_time) * 100.0, 1)
    except Exception as ex:
        print(f"    [Warning] Failed to read process metrics: {ex}")

    res = {
        "Name": name,
        "Executable": os.path.basename(exe_path),
        "ExitCode": proc.returncode,
        "WallTime_s": round(total_wall_time, 3),
        "Peak_RAM_MB": peak_ram_mb,
        "Total_CPU_Time_s": cpu_time_s,
        "CPU_Util_Pct": cpu_util_pct,
    }

    # Match benchmark block
    match_calc_avg = re.search(r"Calc_Time_Avg_ms:\s*([0-9\.]+)", stdout)
    match_calc_min = re.search(r"Calc_Time_Min_ms:\s*([0-9\.]+)", stdout)
    match_calc_max = re.search(r"Calc_Time_Max_ms:\s*([0-9\.]+)", stdout)
    match_fps_avg  = re.search(r"FPS_Avg:\s*([0-9\.]+)", stdout)
    match_through  = re.search(r"Throughput_MegaInteractions_PerSec:\s*([0-9\.]+)", stdout)
    match_bodies   = re.search(r"Bodies:\s*([0-9]+)", stdout)
    match_device   = re.search(r"Device:\s*(\w+)", stdout)
    match_kernel   = re.search(r"Kernel:\s*([\w\.\-]+)", stdout)
    match_threads  = re.search(r"OpenMP_Threads:\s*([0-9]+)", stdout)

    if match_calc_avg:
        res["Calc_Time_Avg_ms"] = float(match_calc_avg.group(1))
        res["Calc_Time_Min_ms"] = float(match_calc_min.group(1))
        res["Calc_Time_Max_ms"] = float(match_calc_max.group(1))
        res["FPS_Avg"]          = float(match_fps_avg.group(1))
        res["Throughput_M_int_sec"] = float(match_through.group(1))
        res["Bodies"]           = int(match_bodies.group(1))
        res["Device"]           = match_device.group(1)
        res["Kernel"]           = match_kernel.group(1) if match_kernel else "N/A"
        res["OpenMP_Threads"]   = int(match_threads.group(1)) if match_threads else 1
    else:
        print("    [WARNING] Benchmark output block not detected. Stdout tail:")
        for line in stdout.splitlines()[-10:]:
            print("      ", line)

    print(f"    Result: Calc = {res.get('Calc_Time_Avg_ms', 'N/A')} ms | FPS = {res.get('FPS_Avg', 'N/A')} | Throughput = {res.get('Throughput_M_int_sec', 'N/A')} M-inter/s | RAM = {res.get('Peak_RAM_MB', 'N/A')} MB | CPU = {res.get('CPU_Util_Pct', 'N/A')}%")
    return res

def main():
    print("=" * 70)
    print("  N-BODY UNIVERSE SIMULATION - COMPREHENSIVE PERFORMANCE SUITE")
    print(f"  CPU Cores: {NUM_LOGICAL_CORES} logical threads detected")
    print("=" * 70)

    results = []

    # Test Suite 1: Interactive Solar System Dataset (N = 1611) across ALL backends & kernels
    solar_tests = [
        # OpenCL GPU Kernel Configurations
        ("OpenCL GPU (Kernel: nbody.cl - Global Memory)", EXECUTABLE_OPENMP, ["--Device", "GPU", "--Kernel", "nbody.cl"], None, 30),
        ("OpenCL GPU (Kernel: nbody_local.cl - Local Memory)", EXECUTABLE_OPENMP, ["--Device", "GPU", "--Kernel", "nbody_local.cl"], None, 30),
        ("OpenCL GPU (Kernel: nbody_async.cl - Async DMA)", EXECUTABLE_OPENMP, ["--Device", "GPU", "--Kernel", "nbody_async.cl"], None, 30),
        
        # Heterogeneous CPU+GPU Mode
        ("Heterogeneous CPU+GPU Hybrid", EXECUTABLE_OPENMP, ["--Device", "CPUGPU", "--Kernel", "nbody.cl"], None, 20),

        # OpenMP CPU Multi-Threading + SIMD
        ("CPU OpenMP (8 Threads) + SIMD SSE", EXECUTABLE_OPENMP, ["--Device", "CPU"], {"OMP_NUM_THREADS": "8"}, 25),
        ("CPU OpenMP (4 Threads - Physical Cores) + SIMD", EXECUTABLE_OPENMP, ["--Device", "CPU"], {"OMP_NUM_THREADS": "4"}, 20),
        ("CPU OpenMP (2 Threads) + SIMD", EXECUTABLE_OPENMP, ["--Device", "CPU"], {"OMP_NUM_THREADS": "2"}, 15),

        # Single-Thread CPU Comparisons
        ("CPU Single-Thread + SIMD SSE (1 Thread)", EXECUTABLE_OPENMP, ["--Device", "CPU"], {"OMP_NUM_THREADS": "1"}, 15),
        ("CPU Single-Thread Scalar Baseline (No SIMD, No OpenMP)", EXECUTABLE_SCALAR, ["--Device", "CPU"], None, 15),
    ]

    for name, exe, args, env, frames in solar_tests:
        r = run_test(name, exe, args, env, frames)
        r["Dataset"] = "Solar System (N=1611)"
        results.append(r)

    # Compute speedups relative to Scalar Baseline for Solar System dataset
    scalar_baseline = next((r for r in results if "Scalar Baseline" in r["Name"] and "Calc_Time_Avg_ms" in r), None)
    if scalar_baseline and scalar_baseline.get("Calc_Time_Avg_ms"):
        base_calc = scalar_baseline["Calc_Time_Avg_ms"]
        for r in results:
            if r.get("Calc_Time_Avg_ms"):
                r["Speedup_vs_Scalar"] = round(base_calc / r["Calc_Time_Avg_ms"], 2)

    # Test Suite 2: Problem Size Scaling Tests (N = 128, 512, 1024, 4096)
    scale_sizes = [128, 512, 1024, 4096]
    for n in scale_sizes:
        # GPU test
        r_gpu = run_test(f"OpenCL GPU (N={n})", EXECUTABLE_OPENMP, ["--Device", "GPU", "--Dataset", "Wikipedia", "--N", str(n)], None, 30)
        r_gpu["Dataset"] = f"Scaled Wikipedia (N={n})"
        results.append(r_gpu)

        # CPU OpenMP 8-thread test
        r_cpu = run_test(f"CPU OpenMP 8-Threads + SIMD (N={n})", EXECUTABLE_OPENMP, ["--Device", "CPU", "--Dataset", "Wikipedia", "--N", str(n)], {"OMP_NUM_THREADS": "8"}, 20 if n <= 1024 else 10)
        r_cpu["Dataset"] = f"Scaled Wikipedia (N={n})"
        results.append(r_cpu)

        # CPU Scalar test (only up to 1024 to maintain fast suite execution)
        if n <= 1024:
            r_scalar = run_test(f"CPU Scalar Baseline (N={n})", EXECUTABLE_SCALAR, ["--Device", "CPU", "--Dataset", "Wikipedia", "--N", str(n)], None, 10)
            r_scalar["Dataset"] = f"Scaled Wikipedia (N={n})"
            results.append(r_scalar)

    # Save JSON and CSV
    os.makedirs("benchmarks", exist_ok=True)
    json_path = os.path.join("benchmarks", "comprehensive_comparison.json")
    csv_path = os.path.join("benchmarks", "comprehensive_comparison.csv")

    with open(json_path, "w") as f:
        json.dump(results, f, indent=2)

    if results:
        fieldnames = [
            "Name", "Dataset", "Bodies", "Device", "Kernel", "OpenMP_Threads",
            "Calc_Time_Avg_ms", "Calc_Time_Min_ms", "Calc_Time_Max_ms",
            "FPS_Avg", "Throughput_M_int_sec", "Speedup_vs_Scalar",
            "CPU_Util_Pct", "Total_CPU_Time_s", "Peak_RAM_MB", "WallTime_s"
        ]
        with open(csv_path, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction='ignore')
            writer.writeheader()
            for row in results:
                writer.writerow(row)

    print("\n" + "=" * 70)
    print(f"Benchmark run complete! Results written to:")
    print(f"  JSON: {json_path}")
    print(f"  CSV:  {csv_path}")
    print("=" * 70)

if __name__ == "__main__":
    main()
