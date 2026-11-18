#!/usr/bin/env python3
"""
Solar System N-Body Simulation - VR & Mobile WebXR Server
Serves the Mobile VR Web Client and provides real-time sensor/telemetry bridge.
Zero external dependencies required (uses standard library only).
"""

import http.server
import socketserver
import socket
import os
import sys
import webbrowser
import threading
import json
import time

PORT = 8080
VR_DIR = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(VR_DIR, "web")

def get_local_ip():
    """Detect the local machine IP accessible by other devices on the LAN/Wi-Fi."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Does not actually establish a connection, but resolves outgoing interface
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

class VRRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def end_headers(self):
        # Enable CORS and disable caching for active development
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def do_GET(self):
        if self.path == '/api/status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            status_data = {
                "server": "SolarSystem-VR-Server",
                "status": "online",
                "bodies": 1611,
                "timestamp": time.time(),
                "local_ip": get_local_ip(),
                "port": PORT
            }
            self.wfile.write(json.dumps(status_data).encode('utf-8'))
            return
        return super().do_GET()

    def log_message(self, format, *args):
        # Suppress noisy standard GET logs, only log errors or important events
        if "404" in str(args) or "500" in str(args):
            sys.stderr.write(f"[VR Server Error] {args}\n")

def run_server():
    local_ip = get_local_ip()
    url_local = f"http://localhost:{PORT}"
    url_mobile = f"http://{local_ip}:{PORT}"

    print("=" * 74)
    print("      SOLAR SYSTEM N-BODY SIMULATION — VR & MOBILE PORTAL")
    print("=" * 74)
    print(f" [*] Local PC URL:    {url_local}")
    print(f" [*] Mobile VR URL:   {url_mobile}")
    print("-" * 74)
    print(" [INSTRUCTIONS FOR COLLEGE DEMONSTRATION]:")
    print("  1. Connect your phone to the same Wi-Fi or Laptop Mobile Hotspot / USB.")
    print(f"  2. Open Chrome/Safari on your phone and go to:  {url_mobile}")
    print("  3. Tap 'Enter VR' for Google Cardboard / VR Box stereoscopic 3D mode,")
    print("     or use '360° Gyro Mode' to hold the phone and look around space!")
    print("=" * 74)
    print(" [*] Server is active. Press Ctrl+C to terminate.")
    print()

    # Automatically open local browser on laptop
    threading.Timer(1.2, lambda: webbrowser.open(url_local)).start()

    with socketserver.TCPServer(("", PORT), VRRequestHandler) as httpd:
        httpd.allow_reuse_address = True
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[*] Shutting down VR Server. Goodbye!")

if __name__ == "__main__":
    run_server()
