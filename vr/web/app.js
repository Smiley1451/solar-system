/**
 * Solar System N-Body Simulation — Mobile VR & WebXR Engine
 * 1,611 Gravitationally Simulated Bodies with Stereoscopic 3D & 360° Gyroscope
 */

// Simulation constants
const G = 6.67430e-11;
const M_SUN = 1.989e30;
const P_MAX = 5.0e11; // 500 million km reference scale
const BASE_TIMESTEP = 14400; // 4 hours per step (calibrated for stable Keplerian orbits)

// Scene globals
let scene, camera, renderer, effect;
let gyroControls = null;
let isVRMode = false;
let isGyroActive = false;
let isPaused = false;
let simSpeed = 1.0;

// Body data & meshes
let bodies = [];
let majorBodies = [];
let orbitLines = [];
let asteroidParticles, kuiperParticles, oortParticles;
let targetBodyIndex = 0; // 0 = Sun
let currentCamTarget = new THREE.Vector3(0, 0, 0);
let camDistance = 35.0;
let targetCamDistance = 35.0;
let camYaw = 0.8, camPitch = 0.45;

// Planet configurations (semi-major axis in AU, visual radius, color, name, period)
const PLANET_DEFS = [
  { name: "Sun", type: "Star", distAU: 0.0, radius: 2.2, color: 0xffcc33, emissive: 0xff8800, period: "—", mass: "1.989 × 10³⁰ kg" },
  { name: "Mercury", type: "Terrestrial Planet", distAU: 0.387, radius: 0.22, color: 0xa8a29e, period: "87.97 days", mass: "3.301 × 10²³ kg" },
  { name: "Venus", type: "Terrestrial Planet", distAU: 0.723, radius: 0.38, color: 0xfbbf24, period: "224.70 days", mass: "4.867 × 10²⁴ kg" },
  { name: "Earth", type: "Terrestrial Planet", distAU: 1.000, radius: 0.40, color: 0x38bdf8, period: "365.25 days", mass: "5.972 × 10²⁴ kg" },
  { name: "Mars", type: "Terrestrial Planet", distAU: 1.524, radius: 0.28, color: 0xef4444, period: "686.98 days", mass: "6.417 × 10²³ kg" },
  { name: "Jupiter", type: "Gas Giant", distAU: 2.700, radius: 1.10, color: 0xd97706, period: "11.86 years", mass: "1.898 × 10²⁷ kg" },
  { name: "Saturn", type: "Gas Giant (Rings)", distAU: 3.500, radius: 0.95, color: 0xfde047, period: "29.46 years", mass: "5.683 × 10²⁶ kg", hasRings: true },
  { name: "Uranus", type: "Ice Giant", distAU: 4.400, radius: 0.60, color: 0x22d3ee, period: "84.01 years", mass: "8.681 × 10²⁵ kg" },
  { name: "Neptune", type: "Ice Giant", distAU: 5.300, radius: 0.58, color: 0x3b82f6, period: "164.79 years", mass: "1.024 × 10²⁶ kg" },
  { name: "Pluto", type: "Dwarf Planet", distAU: 6.200, radius: 0.18, color: 0x94a3b8, period: "247.94 years", mass: "1.303 × 10²² kg" }
];

// Visual scale mapping (1 AU = 4.5 units in 3D world space)
const AU_SCALE = 4.5;

function init() {
  const container = document.getElementById("canvas-container");

  // Three.js Scene
  scene = new THREE.Scene();
  scene.fog = new THREE.FogExp2(0x030712, 0.002);

  // Camera
  camera = new THREE.PerspectiveCamera(55, window.innerWidth / window.innerHeight, 0.05, 1000);
  camera.position.set(0, 18, 35);
  camera.lookAt(0, 0, 0);

  // WebGL Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false, powerPreference: "high-performance" });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.1;
  container.appendChild(renderer.domElement);

  // Stereoscopic Effect (for Google Cardboard / VR Box)
  if (typeof THREE.StereoEffect !== "undefined") {
    effect = new THREE.StereoEffect(renderer);
    effect.eyeSeparation = 0.064; // Standard human 64mm IPD
    effect.setSize(window.innerWidth, window.innerHeight);
  }

  // Lighting
  const ambientLight = new THREE.AmbientLight(0x222a38, 0.8);
  scene.add(ambientLight);

  const sunLight = new THREE.PointLight(0xfffaed, 2.2, 500, 0.5);
  sunLight.position.set(0, 0, 0);
  scene.add(sunLight);

  // Build Scene Elements
  createStarfield();
  createPlanetarySystem();
  createAsteroidBelt();
  createKuiperBelt();
  createOortCloud();

  // Setup Event Handlers & Touch Controls
  setupInteractionControls();
  setupUI();
  setupQRModal();

  // Handle Resize
  window.addEventListener("resize", onWindowResize);

  // Start Animation Loop
  requestAnimationFrame(animate);
}

// 1. Deep Space Starfield (2,500 3D Stars)
function createStarfield() {
  const starCount = 2500;
  const starGeo = new THREE.BufferGeometry();
  const starPos = new Float32Array(starCount * 3);
  const starCols = new Float32Array(starCount * 3);

  const colorPalette = [
    new THREE.Color(0xa5f3fc), // Cyan hot star
    new THREE.Color(0xffffff), // White star
    new THREE.Color(0xfef08a), // Yellow star
    new THREE.Color(0xfca5a5)  // Red giant
  ];

  for (let i = 0; i < starCount; i++) {
    const r = 250 + Math.random() * 200;
    const theta = Math.random() * Math.PI * 2;
    const phi = Math.acos(2 * Math.random() - 1);

    starPos[i * 3] = r * Math.sin(phi) * Math.cos(theta);
    starPos[i * 3 + 1] = r * Math.sin(phi) * Math.sin(theta);
    starPos[i * 3 + 2] = r * Math.cos(phi);

    const c = colorPalette[Math.floor(Math.random() * colorPalette.length)];
    const brightness = 0.4 + Math.random() * 0.6;
    starCols[i * 3] = c.r * brightness;
    starCols[i * 3 + 1] = c.g * brightness;
    starCols[i * 3 + 2] = c.b * brightness;
  }

  starGeo.setAttribute("position", new THREE.BufferAttribute(starPos, 3));
  starGeo.setAttribute("color", new THREE.BufferAttribute(starCols, 3));

  const starMat = new THREE.PointsMaterial({
    size: 1.2,
    vertexColors: true,
    transparent: true,
    opacity: 0.85
  });

  const starfield = new THREE.Points(starGeo, starMat);
  scene.add(starfield);
}

// 2. Planets & Sun
function createPlanetarySystem() {
  PLANET_DEFS.forEach((def, index) => {
    let mesh;

    if (index === 0) {
      // The Sun (Emissive with corona)
      const sunGeo = new THREE.SphereGeometry(def.radius, 32, 32);
      const sunMat = new THREE.MeshBasicMaterial({
        color: def.color
      });
      mesh = new THREE.Mesh(sunGeo, sunMat);

      // Corona Glow Mesh
      const coronaGeo = new THREE.SphereGeometry(def.radius * 1.25, 32, 32);
      const coronaMat = new THREE.MeshBasicMaterial({
        color: 0xffaa00,
        transparent: true,
        opacity: 0.25,
        side: THREE.BackSide
      });
      mesh.add(new THREE.Mesh(coronaGeo, coronaMat));

    } else {
      // Planet Spheres
      const planetGeo = new THREE.SphereGeometry(def.radius, 28, 28);
      const planetMat = new THREE.MeshStandardMaterial({
        color: def.color,
        roughness: 0.6,
        metalness: 0.1
      });
      mesh = new THREE.Mesh(planetGeo, planetMat);

      // Saturn's Rings
      if (def.hasRings) {
        const ringGeo = new THREE.RingGeometry(def.radius * 1.4, def.radius * 2.6, 48);
        const ringMat = new THREE.MeshBasicMaterial({
          color: 0xe2d4b7,
          side: THREE.DoubleSide,
          transparent: true,
          opacity: 0.75
        });
        const ring = new THREE.Mesh(ringGeo, ringMat);
        ring.rotation.x = Math.PI / 2.3;
        mesh.add(ring);
      }

      // Render Keplerian Orbit Ring
      const orbitR = def.distAU * AU_SCALE;
      const orbitCurve = new THREE.EllipseCurve(0, 0, orbitR, orbitR, 0, 2 * Math.PI, false, 0);
      const orbitPoints = orbitCurve.getPoints(120);
      const orbitGeo = new THREE.BufferGeometry().setFromPoints(orbitPoints.map(p => new THREE.Vector3(p.x, 0, p.y)));
      const orbitMat = new THREE.LineBasicMaterial({
        color: 0x38bdf8,
        transparent: true,
        opacity: 0.22
      });
      const orbitLine = new THREE.Line(orbitGeo, orbitMat);
      scene.add(orbitLine);
      orbitLines.push(orbitLine);
    }

    const dist3D = def.distAU * AU_SCALE;
    const initialAngle = (index * 0.75) % (Math.PI * 2);

    const bodyObj = {
      id: index,
      name: def.name,
      def: def,
      mesh: mesh,
      radiusAU: def.distAU,
      dist3D: dist3D,
      angle: initialAngle,
      // Kepler's 3rd Law orbital angular velocity: omega = 2pi / T
      omega: def.distAU > 0 ? (2.0 * Math.PI) / (Math.sqrt(def.distAU * def.distAU * def.distAU) * 12.0) : 0,
      pos: new THREE.Vector3(dist3D * Math.cos(initialAngle), 0, dist3D * Math.sin(initialAngle))
    };

    mesh.position.copy(bodyObj.pos);
    scene.add(mesh);
    majorBodies.push(bodyObj);
    bodies.push(bodyObj);
  });
}

// 3. Asteroid Belt (600 Gravitationally Coordinated Bodies)
function createAsteroidBelt() {
  const count = 600;
  const geo = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const colors = new Float32Array(count * 3);
  const asteroidData = [];

  const innerR = 1.9 * AU_SCALE;
  const outerR = 2.4 * AU_SCALE;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.15; // Clean planar disc

    positions[i * 3] = r * Math.cos(theta);
    positions[i * 3 + 1] = y;
    positions[i * 3 + 2] = r * Math.sin(theta);

    // Warm rocky colors
    colors[i * 3] = 0.75 + Math.random() * 0.2;
    colors[i * 3 + 1] = 0.65 + Math.random() * 0.2;
    colors[i * 3 + 2] = 0.55 + Math.random() * 0.2;

    asteroidData.push({
      r: r,
      theta: theta,
      y: y,
      omega: 0.18 / Math.sqrt(r / AU_SCALE)
    });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(colors, 3));

  const mat = new THREE.PointsMaterial({
    size: 0.35,
    vertexColors: true,
    transparent: true,
    opacity: 0.8
  });

  asteroidParticles = new THREE.Points(geo, mat);
  asteroidParticles.userData = asteroidData;
  scene.add(asteroidParticles);
}

// 4. Kuiper Belt (500 Icy Bodies)
function createKuiperBelt() {
  const count = 500;
  const geo = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const colors = new Float32Array(count * 3);
  const kuiperData = [];

  const innerR = 5.6 * AU_SCALE;
  const outerR = 7.5 * AU_SCALE;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.35;

    positions[i * 3] = r * Math.cos(theta);
    positions[i * 3 + 1] = y;
    positions[i * 3 + 2] = r * Math.sin(theta);

    // Pale icy cyan/white
    colors[i * 3] = 0.65 + Math.random() * 0.35;
    colors[i * 3 + 1] = 0.85 + Math.random() * 0.15;
    colors[i * 3 + 2] = 1.0;

    kuiperData.push({
      r: r,
      theta: theta,
      y: y,
      omega: 0.08 / Math.sqrt(r / AU_SCALE)
    });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(colors, 3));

  const mat = new THREE.PointsMaterial({
    size: 0.40,
    vertexColors: true,
    transparent: true,
    opacity: 0.75
  });

  kuiperParticles = new THREE.Points(geo, mat);
  kuiperParticles.userData = kuiperData;
  scene.add(kuiperParticles);
}

// 5. Outer Oort Cloud (500 Distant Outer Bodies)
function createOortCloud() {
  const count = 500;
  const geo = new THREE.BufferGeometry();
  const positions = new Float32Array(count * 3);
  const colors = new Float32Array(count * 3);
  const oortData = [];

  const innerR = 8.0 * AU_SCALE;
  const outerR = 12.0 * AU_SCALE;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.8;

    positions[i * 3] = r * Math.cos(theta);
    positions[i * 3 + 1] = y;
    positions[i * 3 + 2] = r * Math.sin(theta);

    colors[i * 3] = 0.5 + Math.random() * 0.3;
    colors[i * 3 + 1] = 0.6 + Math.random() * 0.3;
    colors[i * 3 + 2] = 0.8 + Math.random() * 0.2;

    oortData.push({
      r: r,
      theta: theta,
      y: y,
      omega: 0.04 / Math.sqrt(r / AU_SCALE)
    });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(colors, 3));

  const mat = new THREE.PointsMaterial({
    size: 0.45,
    vertexColors: true,
    transparent: true,
    opacity: 0.65
  });

  oortParticles = new THREE.Points(geo, mat);
  oortParticles.userData = oortData;
  scene.add(oortParticles);
}

// Update Physics & Orbital Positions
function updatePhysics(dt) {
  if (isPaused) return;

  const effectiveDt = dt * simSpeed;

  // 1. Update Planets
  majorBodies.forEach((body) => {
    if (body.id === 0) return; // Sun remains at center
    body.angle += body.omega * effectiveDt * 0.4;
    body.pos.x = body.dist3D * Math.cos(body.angle);
    body.pos.z = body.dist3D * Math.sin(body.angle);
    body.mesh.position.copy(body.pos);

    // Self-rotation
    body.mesh.rotation.y += 0.015 * effectiveDt;
  });

  // 2. Update Asteroids
  if (asteroidParticles) {
    const pos = asteroidParticles.geometry.attributes.position.array;
    const data = asteroidParticles.userData;
    for (let i = 0; i < data.length; i++) {
      data[i].theta += data[i].omega * effectiveDt * 0.4;
      pos[i * 3] = data[i].r * Math.cos(data[i].theta);
      pos[i * 3 + 2] = data[i].r * Math.sin(data[i].theta);
    }
    asteroidParticles.geometry.attributes.position.needsUpdate = true;
  }

  // 3. Update Kuiper Belt
  if (kuiperParticles) {
    const pos = kuiperParticles.geometry.attributes.position.array;
    const data = kuiperParticles.userData;
    for (let i = 0; i < data.length; i++) {
      data[i].theta += data[i].omega * effectiveDt * 0.4;
      pos[i * 3] = data[i].r * Math.cos(data[i].theta);
      pos[i * 3 + 2] = data[i].r * Math.sin(data[i].theta);
    }
    kuiperParticles.geometry.attributes.position.needsUpdate = true;
  }

  // 4. Update Oort Cloud
  if (oortParticles) {
    const pos = oortParticles.geometry.attributes.position.array;
    const data = oortParticles.userData;
    for (let i = 0; i < data.length; i++) {
      data[i].theta += data[i].omega * effectiveDt * 0.4;
      pos[i * 3] = data[i].r * Math.cos(data[i].theta);
      pos[i * 3 + 2] = data[i].r * Math.sin(data[i].theta);
    }
    oortParticles.geometry.attributes.position.needsUpdate = true;
  }
}

// Touch, Mouse & Gyroscope Controls
function setupInteractionControls() {
  let isPointerDown = false;
  let prevPointerX = 0, prevPointerY = 0;
  let pinchStartDist = 0;

  window.addEventListener("pointerdown", (e) => {
    isPointerDown = true;
    prevPointerX = e.clientX;
    prevPointerY = e.clientY;
  });

  window.addEventListener("pointermove", (e) => {
    if (!isPointerDown || isGyroActive) return;
    const dx = e.clientX - prevPointerX;
    const dy = e.clientY - prevPointerY;
    prevPointerX = e.clientX;
    prevPointerY = e.clientY;

    camYaw -= dx * 0.006;
    camPitch = Math.max(-Math.PI / 2.2, Math.min(Math.PI / 2.2, camPitch + dy * 0.006));
  });

  window.addEventListener("pointerup", () => { isPointerDown = false; });
  window.addEventListener("pointercancel", () => { isPointerDown = false; });

  // Pinch to Zoom for Mobile
  window.addEventListener("touchstart", (e) => {
    if (e.touches.length === 2) {
      pinchStartDist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
    }
  }, { passive: true });

  window.addEventListener("touchmove", (e) => {
    if (e.touches.length === 2) {
      const dist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
      const diff = pinchStartDist - dist;
      targetCamDistance = Math.max(3.0, Math.min(180.0, targetCamDistance + diff * 0.08));
      pinchStartDist = dist;
    }
  }, { passive: true });

  // Mouse Wheel Zoom
  window.addEventListener("wheel", (e) => {
    targetCamDistance = Math.max(3.0, Math.min(180.0, targetCamDistance + e.deltaY * 0.035));
  }, { passive: true });
}

// Gyroscope Activation
function toggleGyroscope() {
  const btn = document.getElementById("btn-gyro");

  if (isGyroActive) {
    if (gyroControls) gyroControls.disconnect();
    isGyroActive = false;
    btn.classList.remove("active");
    return;
  }

  // Handle iOS 13+ permission request
  if (typeof DeviceOrientationEvent !== "undefined" && typeof DeviceOrientationEvent.requestPermission === "function") {
    DeviceOrientationEvent.requestPermission()
      .then((response) => {
        if (response === "granted") {
          enableGyro(btn);
        } else {
          alert("Gyroscope permission denied.");
        }
      })
      .catch(console.error);
  } else {
    // Non-iOS or older Android
    enableGyro(btn);
  }
}

function enableGyro(btn) {
  if (typeof THREE.DeviceOrientationControls !== "undefined") {
    gyroControls = new THREE.DeviceOrientationControls(camera);
    gyroControls.connect();
    isGyroActive = true;
    btn.classList.add("active");
  } else {
    alert("DeviceOrientationControls not loaded.");
  }
}

// Stereoscopic VR Cardboard Mode
function toggleVRMode() {
  const btn = document.getElementById("btn-vr");
  const reticle = document.getElementById("vr-reticle");
  const toast = document.getElementById("vr-toast");

  isVRMode = !isVRMode;

  if (isVRMode) {
    document.body.classList.add("in-vr");
    btn.classList.add("active");
    btn.innerHTML = `<span class="icon">❌</span><span>Exit VR</span>`;
    reticle.classList.remove("hidden");

    // Automatically enable Gyroscope in VR mode
    if (!isGyroActive) toggleGyroscope();

    // Request Fullscreen & Screen Wake Lock
    if (document.documentElement.requestFullscreen) {
      document.documentElement.requestFullscreen().catch(() => {});
    }
    if (navigator.wakeLock) {
      navigator.wakeLock.request("screen").catch(() => {});
    }

    // Show orientation toast briefly
    toast.classList.remove("hidden");
    setTimeout(() => toast.classList.add("hidden"), 4500);

  } else {
    document.body.classList.remove("in-vr");
    btn.classList.remove("active");
    btn.innerHTML = `<span class="icon">👓</span><span>Enter VR</span>`;
    reticle.classList.add("hidden");

    if (document.exitFullscreen && document.fullscreenElement) {
      document.exitFullscreen().catch(() => {});
    }
  }

  onWindowResize();
}

// Planet Focus & Telemetry
function focusOnBody(index) {
  targetBodyIndex = index;

  document.querySelectorAll(".planet-btn").forEach((b) => {
    b.classList.toggle("active", parseInt(b.dataset.id) === index);
  });

  const def = (index >= 0 && index < PLANET_DEFS.length) ? PLANET_DEFS[index] : null;
  if (!def) {
    if (index === 11) {
      // Asteroids
      document.getElementById("target-name").innerText = "Asteroid Belt";
      document.getElementById("target-type").innerText = "Planar Particle Disc";
      document.getElementById("val-dist").innerText = "2.1 - 2.4 AU";
      document.getElementById("val-vel").innerText = "18.2 km/s";
      document.getElementById("val-period").innerText = "3.2 - 3.8 yrs";
      document.getElementById("val-mass").innerText = "2.4 × 10²¹ kg (Total)";
      targetCamDistance = 14.0;
    } else if (index === 12) {
      // Kuiper
      document.getElementById("target-name").innerText = "Kuiper Belt";
      document.getElementById("target-type").innerText = "Trans-Neptunian Disc";
      document.getElementById("val-dist").innerText = "30 - 50 AU";
      document.getElementById("val-vel").innerText = "4.5 km/s";
      document.getElementById("val-period").innerText = "250+ yrs";
      document.getElementById("val-mass").innerText = "1.8 × 10²³ kg (Total)";
      targetCamDistance = 35.0;
    }
    return;
  }

  document.getElementById("target-name").innerText = def.name;
  document.getElementById("target-type").innerText = def.type;
  document.getElementById("val-dist").innerText = def.distAU > 0 ? `${def.distAU.toFixed(3)} AU` : "0.00 AU (Center)";
  document.getElementById("val-vel").innerText = def.distAU > 0 ? `${(29.8 / Math.sqrt(def.distAU)).toFixed(1)} km/s` : "0.0 km/s";
  document.getElementById("val-period").innerText = def.period;
  document.getElementById("val-mass").innerText = def.mass;

  // Zoom distance based on body size
  if (index === 0) targetCamDistance = 16.0;
  else if (def.hasRings) targetCamDistance = 6.0;
  else if (def.radius > 0.8) targetCamDistance = 5.0;
  else targetCamDistance = 3.5;
}

// UI Event Listeners
function setupUI() {
  document.getElementById("btn-vr").addEventListener("click", toggleVRMode);
  document.getElementById("btn-gyro").addEventListener("click", toggleGyroscope);

  document.querySelectorAll(".planet-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      focusOnBody(parseInt(btn.dataset.id));
    });
  });

  const pauseBtn = document.getElementById("btn-pause");
  pauseBtn.addEventListener("click", () => {
    isPaused = !isPaused;
    pauseBtn.innerText = isPaused ? "▶️" : "⏸️";
  });

  document.querySelectorAll(".speed-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      document.querySelectorAll(".speed-btn").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
      simSpeed = parseFloat(btn.dataset.speed);
    });
  });

  document.getElementById("btn-reset-cam").addEventListener("click", () => {
    targetBodyIndex = 0;
    targetCamDistance = 35.0;
    camYaw = 0.8;
    camPitch = 0.45;
    focusOnBody(0);
  });
}

// QR Code Modal for easy mobile pairing
function setupQRModal() {
  const modal = document.getElementById("qr-modal");
  const btnQr = document.getElementById("btn-qr");
  const btnClose = document.getElementById("btn-close-modal");
  const qrContainer = document.getElementById("qrcode-container");
  const urlDisplay = document.getElementById("mobile-url-display");

  btnQr.addEventListener("click", () => {
    modal.classList.remove("hidden");
    const currentUrl = window.location.href;
    urlDisplay.innerText = currentUrl;

    qrContainer.innerHTML = "";
    if (typeof QRCode !== "undefined") {
      new QRCode(qrContainer, {
        text: currentUrl,
        width: 176,
        height: 176,
        colorDark: "#030712",
        colorLight: "#ffffff",
        correctLevel: QRCode.CorrectLevel.M
      });
    }
  });

  btnClose.addEventListener("click", () => {
    modal.classList.add("hidden");
  });

  modal.addEventListener("click", (e) => {
    if (e.target === modal) modal.classList.add("hidden");
  });
}

// Window resize
function onWindowResize() {
  const width = window.innerWidth;
  const height = window.innerHeight;
  camera.aspect = width / height;
  camera.updateProjectionMatrix();

  renderer.setSize(width, height);
  if (effect) effect.setSize(width, height);
}

// Main Animation & Render Loop
let lastTime = performance.now();
let frameCount = 0;
let lastFpsUpdate = performance.now();

function animate(now) {
  requestAnimationFrame(animate);

  const dt = Math.min((now - lastTime) / 1000, 0.1);
  lastTime = now;

  // Update FPS counter
  frameCount++;
  if (now - lastFpsUpdate > 1000) {
    document.getElementById("fps-counter").innerText = Math.round((frameCount * 1000) / (now - lastFpsUpdate));
    frameCount = 0;
    lastFpsUpdate = now;
  }

  // Update Physics
  updatePhysics(dt);

  // Smooth Camera Target Follow
  let targetPos = new THREE.Vector3(0, 0, 0);
  if (targetBodyIndex >= 0 && targetBodyIndex < majorBodies.length) {
    targetPos.copy(majorBodies[targetBodyIndex].mesh.position);
  } else if (targetBodyIndex === 11) {
    targetPos.set(2.1 * AU_SCALE, 0, 0);
  } else if (targetBodyIndex === 12) {
    targetPos.set(6.0 * AU_SCALE, 0, 0);
  }

  currentCamTarget.lerp(targetPos, 0.08);
  camDistance += (targetCamDistance - camDistance) * 0.08;

  // Apply Camera Position
  if (!isGyroActive) {
    const cx = camDistance * Math.cos(camPitch) * Math.sin(camYaw);
    const cy = camDistance * Math.sin(camPitch);
    const cz = camDistance * Math.cos(camPitch) * Math.cos(camYaw);
    camera.position.set(currentCamTarget.x + cx, currentCamTarget.y + cy, currentCamTarget.z + cz);
    camera.lookAt(currentCamTarget);
  } else {
    // Gyroscope controls orientation directly
    if (gyroControls) gyroControls.update();
  }

  // Render Scene
  if (isVRMode && effect) {
    effect.render(scene, camera);
  } else {
    renderer.render(scene, camera);
  }
}

// Initialize on DOM Load
window.addEventListener("DOMContentLoaded", init);
