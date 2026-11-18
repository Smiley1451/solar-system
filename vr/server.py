#!/usr/bin/env python3
"""
Solar System N-Body Simulation — Mobile Observatory & VR Server
Serves BOTH HTTP (Port 8080) and HTTPS (Port 8443) concurrently.
HTTPS unlocks full hardware gyroscope sensors in mobile Chrome & Safari!
"""

import http.server
import socketserver
import socket
import ssl
import os
import sys
import webbrowser
import threading
import json
import time

HTTP_PORT = 8080
HTTPS_PORT = 8443
VR_DIR = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(VR_DIR, "web")
CERT_FILE = os.path.join(VR_DIR, "cert.pem")
KEY_FILE = os.path.join(VR_DIR, "key.pem")

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
                "http_port": HTTP_PORT,
                "https_port": HTTPS_PORT
            }
            self.wfile.write(json.dumps(status_data).encode('utf-8'))
            return
        return super().do_GET()

    def log_message(self, format, *args):
        # Suppress routine GET logs
        pass

# Class-level allow_reuse_address is essential on Windows socketserver to prevent WinError 10048
class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

def ensure_ssl_certificates():
    """Check if self-signed certificate exists; if not, generate using git openssl."""
    if os.path.exists(CERT_FILE) and os.path.exists(KEY_FILE):
        return True

    git_openssl = r"C:\Program Files\Git\usr\bin\openssl.exe"
    if os.path.exists(git_openssl):
        cmd = f'"{git_openssl}" req -x509 -newkey rsa:2048 -keyout "{KEY_FILE}" -out "{CERT_FILE}" -days 365 -nodes -subj "/CN={get_local_ip()}"'
        os.system(cmd)
        return os.path.exists(CERT_FILE)
    return False

def run_http_server():
    try:
        with ReusableTCPServer(("", HTTP_PORT), VRRequestHandler) as httpd:
            httpd.serve_forever()
    except OSError as e:
        print(f"[!] HTTP Port {HTTP_PORT} notice: {e}. (Another instance may already be running)")

def run_https_server():
    if not ensure_ssl_certificates():
        return

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)

    try:
        with ReusableTCPServer(("", HTTPS_PORT), VRRequestHandler) as httpsd:
            httpsd.socket = context.wrap_socket(httpsd.socket, server_side=True)
            httpsd.serve_forever()
    except OSError as e:
        print(f"[!] HTTPS Port {HTTPS_PORT} notice: {e}. (Another instance may already be running)")

def main():
    local_ip = get_local_ip()
    has_https = ensure_ssl_certificates()

    print("=" * 78)
    print("    SOLAR SYSTEM 3D SIMULATION -- MOBILE OBSERVATORY & VR SERVER")
    print("=" * 78)
    print(" [*] DO NOT TYPE 'localhost' ON YOUR PHONE! 'localhost' only works on your laptop.")
    print(" [*] On your phone (connected to the same Wi-Fi), open either:")
    print()
    print(f"     1. OPTION 1 (Standard Touch 360 & D-Pad - Zero Warnings):")
    print(f"        http://{local_ip}:{HTTP_PORT}")
    print()
    if has_https:
        print(f"     2. OPTION 2 (Hardware Gyroscope & Physical Phone Tilt):")
        print(f"        https://{local_ip}:{HTTPS_PORT}")
        print(f"        (On phone: Tap 'Advanced' -> 'Proceed to {local_ip}' to unlock gyro)")
    print("-" * 78)
    print(" [NAVIGATION INSTRUCTIONS FOR PHONE]:")
    print("  - Swipe anywhere with 1 finger to look 360 degrees around space.")
    print("  - Use the glowing 4-way D-Pad on the right to spin left/right/up/down.")
    print("  - Tap any planet button on the bottom bar to fly right next to it.")
    print("  - Pinch to zoom in and out.")
    print("=" * 78)
    print(" [*] Server is ACTIVE. Press Ctrl+C to terminate.")
    print()

    # Automatically open laptop browser to HTTP
    threading.Timer(1.2, lambda: webbrowser.open(f"http://localhost:{HTTP_PORT}")).start()

    # Start HTTP server thread
    t_http = threading.Thread(target=run_http_server, daemon=True)
    t_http.start()

    # Start HTTPS server thread if cert available
    if has_https:
        t_https = threading.Thread(target=run_https_server, daemon=True)
        t_https.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[*] Shutting down servers. Goodbye!")

if __name__ == "__main__":
    main()
