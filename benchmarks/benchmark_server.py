#!/usr/bin/env python3
"""
Solar System N-Body Simulation — Interactive Benchmark Dashboard Server
Serves the web dashboard and provides REST API to execute live benchmarks on demand.
Uses standard library only (zero external pip packages required).
"""

import http.server
import socketserver
import json
import subprocess
import os
import sys
import threading
import webbrowser
import time

PORT = 5050
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
DASHBOARD_FILE = os.path.join(ROOT_DIR, "benchmark_dashboard.html")
RESULTS_FILE = os.path.join(ROOT_DIR, "benchmark_results.json")
RUNNER_PS1 = os.path.join(ROOT_DIR, "benchmark_runner.ps1")

is_benchmarking = False
benchmark_log = ""
benchmark_lock = threading.Lock()

class DashboardHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=ROOT_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            with open(DASHBOARD_FILE, "rb") as f:
                self.wfile.write(f.read())
            return

        if self.path == "/api/status":
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            status_data = {
                "running": is_benchmarking,
                "log": benchmark_log[-2000:] if benchmark_log else "",
                "timestamp": time.time()
            }
            self.wfile.write(json.dumps(status_data).encode('utf-8'))
            return

        if self.path == "/api/results":
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            if os.path.exists(RESULTS_FILE):
                with open(RESULTS_FILE, "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.wfile.write(json.dumps({"error": "No benchmark results yet."}).encode('utf-8'))
            return

        return super().do_GET()

    def do_POST(self):
        global is_benchmarking, benchmark_log

        if self.path == "/api/run":
            if is_benchmarking:
                self.send_response(409)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"error": "A benchmark is already in progress. Please wait."}).encode('utf-8'))
                return

            content_len = int(self.headers.get('Content-Length', 0))
            post_body = self.rfile.read(content_len).decode('utf-8') if content_len > 0 else "{}"
            try:
                params = json.loads(post_body)
            except Exception:
                params = {}

            device = params.get("device", "ALL")
            threads = params.get("threads", "1,2,4,8")
            kernel = params.get("kernel", "ALL")
            frames = int(params.get("frames", 15))
            bodies = int(params.get("bodies", 1611))

            with benchmark_lock:
                is_benchmarking = True
                benchmark_log = f"[*] Starting Benchmark: Device={device}, Threads={threads}, Kernel={kernel}, Frames={frames}, Bodies={bodies}\n"

            # Execute PowerShell benchmark runner synchronously
            cmd = [
                "powershell.exe",
                "-NoProfile",
                "-ExecutionPolicy", "Bypass",
                "-File", RUNNER_PS1,
                "-Device", str(device),
                "-Threads", str(threads),
                "-Kernel", str(kernel),
                "-Frames", str(frames),
                "-Bodies", str(bodies),
                "-NoBrowser"
            ]

            try:
                proc = subprocess.run(cmd, cwd=ROOT_DIR, capture_output=True, text=True, timeout=300)
                stdout = proc.stdout
                stderr = proc.stderr
                return_code = proc.returncode
            except Exception as e:
                stdout = ""
                stderr = str(e)
                return_code = 1
            finally:
                with benchmark_lock:
                    is_benchmarking = False
                    benchmark_log = stdout + ("\nErrors:\n" + stderr if stderr else "")

            # Read latest JSON results
            results_data = {}
            if os.path.exists(RESULTS_FILE):
                try:
                    with open(RESULTS_FILE, "r", encoding="utf-8") as f:
                        results_data = json.load(f)
                except Exception:
                    pass

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            resp = {
                "success": (return_code == 0),
                "stdout": stdout,
                "stderr": stderr,
                "data": results_data
            }
            self.wfile.write(json.dumps(resp).encode('utf-8'))
            return

        self.send_response(404)
        self.end_headers()

    def log_message(self, format, *args):
        # Suppress routine log output
        pass

class ReusableServer(socketserver.TCPServer):
    allow_reuse_address = True

def main():
    print("=" * 76)
    print("  SOLAR SYSTEM N-BODY SIMULATION -- INTERACTIVE BENCHMARK DASHBOARD")
    print("=" * 76)
    print(f" [*] Server URL: http://localhost:{PORT}")
    print(" [*] You can trigger benchmarks, view live telemetry, and plot graphs")
    print("     directly from the web dashboard!")
    print("=" * 76)
    print(" [*] Server is active. Press Ctrl+C to terminate.\n")

    threading.Timer(1.2, lambda: webbrowser.open(f"http://localhost:{PORT}")).start()

    with ReusableServer(("", PORT), DashboardHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[*] Server stopped.")

if __name__ == "__main__":
    main()
