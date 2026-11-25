#!/usr/bin/env python3
"""
Solar System N-Body Simulation — Interactive Benchmark Dashboard & 3D Graphics Server
Hosts:
  - Benchmark Dashboard & REST Runner on Port 5050 (http://localhost:5050)
  - 3D Real-Time Graphics Observatory on Port 8080 (http://localhost:8080)
  - 3D Real-Time Graphics Observatory (HTTPS) on Port 8443 (https://localhost:8443)
Standard library only (zero external pip packages required).
"""

import http.server
import socketserver
import socket
import ssl
import json
import subprocess
import os
import sys
import threading
import webbrowser
import time

DASHBOARD_PORT = 5050
GRAPHICS_HTTP_PORT = 8080
GRAPHICS_HTTPS_PORT = 8443

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
DASHBOARD_FILE = os.path.join(ROOT_DIR, "benchmark_dashboard.html")
RESULTS_FILE = os.path.join(ROOT_DIR, "benchmark_results.json")
RUNNER_PS1 = os.path.join(ROOT_DIR, "benchmark_runner.ps1")
VR_DIR = os.path.join(ROOT_DIR, "vr")
VR_WEB_DIR = os.path.join(VR_DIR, "web")
CERT_FILE = os.path.join(VR_DIR, "cert.pem")
KEY_FILE = os.path.join(VR_DIR, "key.pem")

is_benchmarking = False
benchmark_log = ""
benchmark_lock = threading.Lock()

def get_local_ip():
    """Detect local IP accessible on the local network / Wi-Fi."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

# ---------------------------------------------------------------------------
# 1. 3D Graphics Observatory Request Handler (Ports 8080 & 8443)
# ---------------------------------------------------------------------------
class VRRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=VR_WEB_DIR, **kwargs)

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def do_GET(self):
        if self.path == '/api/status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            status_data = {
                "server": "SolarSystem-3D-Graphics-Server",
                "status": "online",
                "bodies": 1611,
                "timestamp": time.time(),
                "local_ip": get_local_ip(),
                "http_port": GRAPHICS_HTTP_PORT,
                "https_port": GRAPHICS_HTTPS_PORT
            }
            self.wfile.write(json.dumps(status_data).encode('utf-8'))
            return
        return super().do_GET()

    def log_message(self, format, *args):
        pass

# ---------------------------------------------------------------------------
# 2. Benchmark Dashboard & Runner Request Handler (Port 5050)
# ---------------------------------------------------------------------------
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
        # Serve main dashboard
        if self.path == "/" or self.path == "/index.html":
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            with open(DASHBOARD_FILE, "rb") as f:
                self.wfile.write(f.read())
            return

        # Direct alias to 3D Graphics Visualizer on port 5050 (e.g. /vr or /vr/index.html)
        if self.path == "/vr" or self.path == "/vr/":
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            index_path = os.path.join(VR_WEB_DIR, "index.html")
            with open(index_path, "rb") as f:
                self.wfile.write(f.read())
            return

        if self.path.startswith("/vr/"):
            rel_path = self.path[4:].split("?")[0]
            target_path = os.path.join(VR_WEB_DIR, rel_path)
            if os.path.isfile(target_path):
                self.send_response(200)
                if target_path.endswith(".html"):
                    self.send_header('Content-Type', 'text/html; charset=utf-8')
                elif target_path.endswith(".js"):
                    self.send_header('Content-Type', 'application/javascript')
                elif target_path.endswith(".css"):
                    self.send_header('Content-Type', 'text/css')
                elif target_path.endswith(".json"):
                    self.send_header('Content-Type', 'application/json')
                self.end_headers()
                with open(target_path, "rb") as f:
                    self.wfile.write(f.read())
                return

        # API Status
        if self.path == "/api/status":
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            status_data = {
                "running": is_benchmarking,
                "log": benchmark_log[-2000:] if benchmark_log else "",
                "graphics_http_port": GRAPHICS_HTTP_PORT,
                "graphics_https_port": GRAPHICS_HTTPS_PORT,
                "dashboard_port": DASHBOARD_PORT,
                "local_ip": get_local_ip(),
                "timestamp": time.time()
            }
            self.wfile.write(json.dumps(status_data).encode('utf-8'))
            return

        # API Results
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
        pass

# ---------------------------------------------------------------------------
# Background Server Starters
# ---------------------------------------------------------------------------
def ensure_ssl_certificates():
    if os.path.exists(CERT_FILE) and os.path.exists(KEY_FILE):
        return True
    git_openssl = r"C:\Program Files\Git\usr\bin\openssl.exe"
    if os.path.exists(git_openssl):
        cmd = f'"{git_openssl}" req -x509 -newkey rsa:2048 -keyout "{KEY_FILE}" -out "{CERT_FILE}" -days 365 -nodes -subj "/CN={get_local_ip()}"'
        os.system(cmd)
        return os.path.exists(CERT_FILE)
    return False

def run_graphics_http():
    try:
        with ReusableTCPServer(("", GRAPHICS_HTTP_PORT), VRRequestHandler) as httpd:
            httpd.serve_forever()
    except OSError as e:
        print(f"[!] Graphics HTTP Port {GRAPHICS_HTTP_PORT} notice: {e}")

def run_graphics_https():
    if not ensure_ssl_certificates():
        return
    try:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)
        with ReusableTCPServer(("", GRAPHICS_HTTPS_PORT), VRRequestHandler) as httpsd:
            httpsd.socket = context.wrap_socket(httpsd.socket, server_side=True)
            httpsd.serve_forever()
    except OSError as e:
        print(f"[!] Graphics HTTPS Port {GRAPHICS_HTTPS_PORT} notice: {e}")

def main():
    local_ip = get_local_ip()
    has_https = os.path.exists(CERT_FILE) and os.path.exists(KEY_FILE)

    print("=" * 78)
    print("  SOLAR SYSTEM 3D SIMULATION: BENCHMARK DASHBOARD & 3D GRAPHICS SERVER")
    print("=" * 78)
    print(f" [*] Web Benchmark Dashboard:       http://localhost:{DASHBOARD_PORT}")
    print(f" [*] 3D Graphics Observatory (HTTP): http://localhost:{GRAPHICS_HTTP_PORT}")
    if has_https:
        print(f" [*] 3D Graphics (Mobile Gyro HTTPS): https://localhost:{GRAPHICS_HTTPS_PORT}")
        print(f" [*] Phone Observatory (Same Wi-Fi):  https://{local_ip}:{GRAPHICS_HTTPS_PORT}")
    else:
        print(f" [*] Phone Observatory (Same Wi-Fi):  http://{local_ip}:{GRAPHICS_HTTP_PORT}")
    print("=" * 78)
    print(" [*] Pop-up 3D graphics windows are ready to display while running.")
    print(" [*] Press Ctrl+C to terminate all servers.\n")

    # Start 3D Graphics HTTP server in background thread
    t_http = threading.Thread(target=run_graphics_http, daemon=True)
    t_http.start()

    # Start 3D Graphics HTTPS server in background thread
    if has_https:
        t_https = threading.Thread(target=run_graphics_https, daemon=True)
        t_https.start()

    # Open dashboard in browser after 1.2s
    threading.Timer(1.2, lambda: webbrowser.open(f"http://localhost:{DASHBOARD_PORT}")).start()

    # Serve Dashboard on main thread
    with ReusableTCPServer(("", DASHBOARD_PORT), DashboardHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[*] Servers stopped.")

if __name__ == "__main__":
    main()
