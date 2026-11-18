#!/usr/bin/env python3
"""
Solar System N-Body Simulation — Mobile Observatory & VR Server
Serves the Mobile WebXR Client with zero external dependencies.
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
    """Detect local IP accessible by phones on the same Wi-Fi / Hotspot / USB."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
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
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def do_GET(self):
        if self.path == '/api/status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            status_data = {
                "server": "SolarSystem-Observatory-Server",
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
        if "404" in str(args) or "500" in str(args):
            sys.stderr.write(f"[VR Server] {args}\n")

def run_server():
    local_ip = get_local_ip()
    url_local = f"http://localhost:{PORT}"
    url_mobile = f"http://{local_ip}:{PORT}"

    print("=" * 76)
    print("   SOLAR SYSTEM 3D SIMULATION -- MOBILE OBSERVATORY & VR SERVER")
    print("=" * 76)
    print(f" [*] Local PC URL:      {url_local}")
    print(f" [*] Mobile Phone URL:   {url_mobile}")
    print("-" * 76)
    print(" [HOW TO USE ON ANY SMARTPHONE (NO EXTRA HARDWARE NEEDED)]:")
    print("  1. Connect your phone to your laptop (via Wi-Fi or USB Tethering).")
    print(f"  2. Open Chrome or Safari on your phone and go to:  {url_mobile}")
    print("  3. Navigation on phone:")
    print("     - Swipe anywhere with 1 finger: Look 360 degrees around space!")
    print("     - Pinch with 2 fingers: Zoom in and out.")
    print("     - Tap any planet: Smoothly fly right up to it.")
    print("     - Optional: Tap 'VR Cardboard' if you have a headset for stereo 3D.")
    print("=" * 76)
    print(" [*] Server is active. Press Ctrl+C to stop.")
    print()

    threading.Timer(1.2, lambda: webbrowser.open(url_local)).start()

    with socketserver.TCPServer(("", PORT), VRRequestHandler) as httpd:
        httpd.allow_reuse_address = True
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[*] VR Server stopped. Goodbye!")

if __name__ == "__main__":
    run_server()
