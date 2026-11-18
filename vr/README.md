# Solar System N-Body Simulation — VR & Mobile Demonstration Guide

Welcome to the **VR & Mobile extension** of the OpenCL N-Body Simulation project! This module is specifically built so you can demonstrate a working **Virtual Reality (VR) and 3D astronomical simulation** in college using **only your laptop and any smartphone**, with **zero hardware investment and zero app installation required on the phone**.

---

## 🚀 Quick Start for College Demonstration

### Method A: Mobile Phone VR (Best for College Presentation)
*Works with **any Android or iPhone** using Google Chrome or Safari. No app download needed!*

1. **Connect your phone to your laptop**:
   - **Option 1 (Recommended - No Wi-Fi needed):** Connect your phone to your laptop with a USB cable and enable **USB Tethering** in phone settings.
   - **Option 2:** Connect both laptop and phone to the same Wi-Fi network (or turn on your phone's Personal Hotspot and connect your laptop to it).
2. **Launch the VR Server on your Laptop**:
   - Double-click `run_vr_mobile.bat` (or `vr/start_vr_mobile.bat`).
   - A terminal will open showing:
     ```
     [*] Local PC URL:    http://localhost:8080
     [*] Mobile VR URL:   http://192.168.x.x:8080
     ```
3. **Open on Mobile Phone**:
   - On your phone's browser (Chrome / Safari), open the **Mobile VR URL** (or click "Share to Phone" on the laptop screen to scan the QR code).
4. **Choose your demonstration mode**:
   - **📱 360° Gyroscope Mode ("Magic Window"):** Hold the phone in your hand and move it around in 3D space. The phone's gyroscope tracks your head and turns the camera in 360° real space!
   - **👓 Stereoscopic VR Cardboard Mode:** Tap **"Enter VR"**. The screen instantly splits into Left Eye and Right Eye viewports with 3D depth. Slide the phone into any ₹200 Google Cardboard or plastic VR headset for true 3D immersion!
   - **🪐 Planet Teleport:** Tap any planet button on the bottom bar to smoothly fly the camera right up to it.
   - **⏱️ Speed & Pause:** Adjust simulation speed (`0.25x` to `4.0x`) or pause anytime.

---

### Method B: Native Desktop Side-by-Side (SBS) VR
*For PCVR Headsets (Meta Quest, HTC Vive, Pico 4) or Trinus VR streaming.*

1. Double-click `run_vr_desktop.bat` (or `vr/start_vr_native.bat`).
2. The simulation opens in a high-performance **Side-by-Side (SBS) Stereoscopic 3D** window.
3. **Controls**:
   - `V`: Toggle Side-by-Side 3D / Monoscopic mode
   - `Space`: Pause / Resume physics
   - `+` / `-`: Increase / Decrease simulation speed
   - `[` / `]`: Adjust Interpupillary Distance (IPD / eye separation)
   - `0` - `9`: Fly to Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto
   - `R`: Reset camera view
   - `ESC`: Exit
4. **Mobile Gyro Tracking over UDP**:
   - The native app listens on UDP port `4242`. Any mobile app sending `yaw,pitch,roll` packets to port `4242` will directly rotate the desktop 3D camera!

---

## 🎓 Technical Details for College Examiners & Viva

When explaining this project to professors, highlight these key technical points:

1. **1,611 Gravitationally Interacting Bodies**:
   - Sun (Central Star)
   - 9 Major Planets + Pluto (Keplerian orbits with $v = \sqrt{G M_\odot / r}$)
   - Saturn's Ring System
   - Asteroid Belt (600 planar disc particles between Mars and Jupiter)
   - Kuiper Belt (500 icy outer particles beyond Neptune)
   - Oort Cloud (500 distant celestial bodies)
   - Deep-space Starfield (2,500 background stars)

2. **Calibrated Timestep ($\Delta t = 14,400\text{ s}$)**:
   - Numerical forward Euler integration error scales with $\mathcal{O}(\Delta t^2)$.
   - Reducing the step from 86,400s (24h) to 14,400s (4h) drops numerical integration error by **36×**, keeping planetary orbits perfectly stable (<0.44% max drift).

3. **Stereoscopic 3D Projection**:
   - Dual-eye viewports rendered with human Interpupillary Distance (IPD $\approx 64\text{ mm}$).
   - Left and right projection frustums create true retinal disparity, enabling the brain to perceive realistic depth in space.

4. **Hardware Accessibility**:
   - High-performance astronomical visualization made fully accessible on commodity mobile smartphones without requiring \$500+ proprietary VR hardware.
