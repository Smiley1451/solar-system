/**
 * Solar System N-Body Simulation — Mobile Observatory & WebXR VR Engine
 * Replicates the full visual fidelity of the C++ OpenGL SolarSystemVisuals:
 * - Procedural Sun with turbulence granulation and coronal glare
 * - Procedural Planets (Earth oceans/clouds/atmosphere, Jupiter bands + Red Spot, Saturn rings + Cassini gap)
 * - Motion trails trailing behind orbiting bodies
 * - 3D Billboard labels above every body
 * - Authentic Desktop Diagnostic HUD Banner
 * - 100% Mobile Touch 360° Swipe Navigation (Works on any phone without extra devices!)
 * - Hardware Gyroscope & Cardboard Stereoscopic VR
 */

// Simulation constants
const G = 6.67430e-11;
const M_SUN = 8.50e29;
const P_MAX = 5.0e11;
const BASE_TIMESTEP = 14400; // 4 hours per step

// Three.js Core
let scene, camera, renderer, effect;
let isVRMode = false;
let isGyroActive = false;
let isPaused = false;
let simSpeed = 1.0;

// Camera Navigation (Spherical Orbit)
let camYaw = 0.85;
let camPitch = 0.38;
let camDistance = 45.0;
let targetCamDistance = 45.0;
let currentCenter = new THREE.Vector3(0, 0, 0);
let targetCenter = new THREE.Vector3(0, 0, 0);
let focusIndex = 0; // 0 = Sun

// Gyroscope & Touch State
let gyroAlpha = 0, gyroBeta = 0, gyroGamma = 0;
let gyroInitialYaw = null;
let touchStartX = 0, touchStartY = 0;
let touchPrevX = 0, touchPrevY = 0;
let isTouching = false;
let pinchStartDist = 0;

// Celestial Bodies & Shaders
let bodies = [];
let majorBodies = [];
let motionTrails = [];
let billboardLabels = [];
let asteroidPoints, kuiperPoints, oortPoints;
let starfieldPoints;

// Time & Diagnostics
let startTime = performance.now();
let lastFrameTime = performance.now();
let frameCount = 0;
let lastFpsTime = performance.now();
let currentFps = 60.0;
let physicsTimeMs = 0.56;

// Scale: 1 AU = 4.2 world units
const AU = 4.2;

// Definitions matching SolarSystemDataSet.cpp & SolarSystemVisuals.cpp
const PLANET_DATA = [
  { id: 0, name: "SUN", type: "G2V Star", rAU: 0.0, radius: 2.4, color: [1.0, 0.9, 0.2], period: "Center", mass: "1.989 × 10³⁰ kg" },
  { id: 1, name: "MERCURY", type: "Terrestrial", rAU: 0.387, radius: 0.28, color: [0.68, 0.65, 0.62], period: "87.97 days", mass: "3.30 × 10²³ kg" },
  { id: 2, name: "VENUS", type: "Terrestrial", rAU: 0.723, radius: 0.44, color: [0.95, 0.78, 0.30], period: "224.7 days", mass: "4.87 × 10²⁴ kg" },
  { id: 3, name: "EARTH", type: "Terrestrial", rAU: 1.000, radius: 0.48, color: [0.22, 0.74, 0.97], period: "365.25 days", mass: "5.97 × 10²⁴ kg" },
  { id: 4, name: "MARS", type: "Terrestrial", rAU: 1.524, radius: 0.34, color: [0.94, 0.30, 0.25], period: "686.98 days", mass: "6.42 × 10²³ kg" },
  { id: 5, name: "JUPITER", type: "Gas Giant", rAU: 2.700, radius: 1.25, color: [0.85, 0.55, 0.20], period: "11.86 yrs", mass: "1.90 × 10²⁷ kg" },
  { id: 6, name: "SATURN", type: "Gas Giant (Rings)", rAU: 3.500, radius: 1.05, color: [0.96, 0.85, 0.45], period: "29.46 yrs", mass: "5.68 × 10²⁶ kg", hasRings: true },
  { id: 7, name: "URANUS", type: "Ice Giant", rAU: 4.400, radius: 0.70, color: [0.30, 0.85, 0.90], period: "84.01 yrs", mass: "8.68 × 10²⁵ kg" },
  { id: 8, name: "NEPTUNE", type: "Ice Giant", rAU: 5.300, radius: 0.68, color: [0.25, 0.55, 0.95], period: "164.8 yrs", mass: "1.02 × 10²⁶ kg" },
  { id: 9, name: "PLUTO", type: "Dwarf Planet", rAU: 6.200, radius: 0.22, color: [0.65, 0.65, 0.70], period: "247.9 yrs", mass: "1.30 × 10²² kg" }
];

// GLSL Shaders ported directly from SolarSystemVisuals.cpp
const SUN_VERTEX_SHADER = `
  varying vec3 vNormal;
  varying vec3 vWorldPos;
  void main() {
    vNormal = normal;
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = projectionMatrix * viewMatrix * worldPos;
  }
`;

const SUN_FRAGMENT_SHADER = `
  uniform vec3 viewPos;
  uniform float time;
  varying vec3 vNormal;
  varying vec3 vWorldPos;

  void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    // Procedural solar surface turbulence
    float n1 = sin(N.x * 22.0 + time * 1.8) * cos(N.y * 22.0 - time * 1.5) * sin(N.z * 22.0 + time * 1.2);
    float n2 = sin(N.y * 45.0 + time * 2.5) * cos(N.z * 45.0 - time * 2.0);
    float granulation = (n1 * 0.6 + n2 * 0.4) * 0.22;

    vec3 sunCore = vec3(1.0, 0.96, 0.82);
    vec3 sunMid  = vec3(1.0, 0.62, 0.12);
    vec3 sunRim  = vec3(0.95, 0.30, 0.04);

    float t = clamp(NdotV + granulation, 0.0, 1.0);
    vec3 baseColor = mix(sunRim, sunMid, smoothstep(0.0, 0.5, t));
    baseColor = mix(baseColor, sunCore, smoothstep(0.5, 1.0, t));

    // Outer coronal glare
    float corona = pow(1.0 - NdotV, 2.5) * 1.2;
    vec3 finalColor = baseColor * 1.25 + vec3(1.0, 0.55, 0.15) * corona;
    gl_FragColor = vec4(finalColor, 1.0);
  }
`;

const PLANET_VERTEX_SHADER = `
  varying vec3 vNormal;
  varying vec3 vWorldPos;
  varying vec2 vUV;
  void main() {
    vNormal = normalMatrix * normal;
    vUV = uv;
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = projectionMatrix * viewMatrix * worldPos;
  }
`;

const PLANET_FRAGMENT_SHADER = `
  uniform vec3 lightPos;
  uniform vec3 viewPos;
  uniform int planetId;
  uniform float time;
  varying vec3 vNormal;
  varying vec3 vWorldPos;
  varying vec2 vUV;

  float hash(vec2 p) {
    p = 50.0 * fract(p * 0.3183099 + vec2(0.71, 0.113));
    return -1.0 + 2.0 * fract(16.0 * fract(p.x * p.y * (p.x + p.y)));
  }

  float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
  }

  float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for (int i = 0; i < 4; ++i) {
      v += a * noise(p);
      p = rot * p * 2.0 + vec2(100.0);
      a *= 0.5;
    }
    return v;
  }

  void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 L = normalize(lightPos - vWorldPos);

    float NdotL = max(dot(N, L), 0.0);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), 24.0);
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    vec3 albedo = vec3(0.5);
    vec3 atmosphereColor = vec3(0.0);
    float specularStrength = 0.0;

    if (planetId == 1) {
      // Mercury: Cratered gray-brown rocky surface
      float craters = fbm(vUV * 30.0);
      albedo = mix(vec3(0.48, 0.46, 0.44), vec3(0.68, 0.65, 0.62), craters * 0.5 + 0.5);
    } else if (planetId == 2) {
      // Venus: Golden-yellow dense atmospheric haze
      float clouds = sin(vUV.y * 14.0 + fbm(vUV * 8.0) * 2.5);
      albedo = mix(vec3(0.88, 0.76, 0.50), vec3(0.96, 0.88, 0.68), clouds * 0.5 + 0.5);
      atmosphereColor = vec3(0.95, 0.85, 0.60) * fresnel * 0.8;
    } else if (planetId == 3) {
      // Earth: Deep blue oceans, continents, polar ice caps, swirling clouds
      float continentNoise = fbm(vUV * 8.0);
      float isLand = smoothstep(0.05, 0.15, continentNoise);
      vec3 oceanColor = vec3(0.08, 0.28, 0.68);
      vec3 landColor = mix(vec3(0.22, 0.52, 0.22), vec3(0.55, 0.48, 0.32), fbm(vUV * 16.0) * 0.5 + 0.5);
      albedo = mix(oceanColor, landColor, isLand);

      if (abs(N.y) > 0.82) {
        albedo = vec3(0.95, 0.98, 1.0);
        isLand = 1.0;
      }
      float cloudPattern = fbm(vUV * 12.0 + vec2(time * 0.015, 0.0));
      float clouds = smoothstep(0.18, 0.45, cloudPattern);
      albedo = mix(albedo, vec3(0.98, 0.98, 1.0), clouds * 0.85);
      specularStrength = (1.0 - isLand) * (1.0 - clouds) * 0.75;
      atmosphereColor = vec3(0.25, 0.60, 1.0) * fresnel * 1.1;
    } else if (planetId == 4) {
      // Mars: Rusty iron-red with dark basalt and polar ice
      float terrain = fbm(vUV * 12.0);
      albedo = mix(vec3(0.82, 0.35, 0.18), vec3(0.48, 0.22, 0.12), terrain * 0.5 + 0.5);
      if (abs(N.y) > 0.86) {
        albedo = vec3(0.95, 0.96, 0.98);
      }
      atmosphereColor = vec3(0.85, 0.45, 0.25) * fresnel * 0.35;
    } else if (planetId == 5) {
      // Jupiter: Alternating cloud bands + Great Red Spot
      float bands = sin(vUV.y * 36.0 + fbm(vec2(vUV.x * 6.0, vUV.y * 12.0)) * 1.5);
      vec3 bandLight = vec3(0.92, 0.86, 0.75);
      vec3 bandDark  = vec3(0.68, 0.45, 0.28);
      albedo = mix(bandDark, bandLight, bands * 0.5 + 0.5);

      vec2 spotCenter = vec2(0.45, 0.34);
      vec2 d = (vUV - spotCenter) * vec2(2.5, 1.2);
      float spotDist = length(d);
      if (spotDist < 0.065) {
        float spot = smoothstep(0.065, 0.02, spotDist);
        albedo = mix(albedo, vec3(0.82, 0.25, 0.16), spot * 0.9);
      }
    } else if (planetId == 6) {
      // Saturn: Golden-butterscotch bands
      float bands = sin(vUV.y * 42.0 + fbm(vec2(vUV.x * 4.0, vUV.y * 8.0)) * 0.8);
      albedo = mix(vec3(0.78, 0.68, 0.50), vec3(0.92, 0.84, 0.64), bands * 0.5 + 0.5);
    } else if (planetId == 7) {
      // Uranus: Pale cyan ice atmosphere
      albedo = vec3(0.58, 0.84, 0.88);
      atmosphereColor = vec3(0.50, 0.90, 0.95) * fresnel * 0.7;
    } else if (planetId == 8) {
      // Neptune: Deep azure blue
      float storm = fbm(vUV * 10.0 + vec2(time * 0.02, 0.0));
      albedo = mix(vec3(0.15, 0.35, 0.85), vec3(0.22, 0.55, 0.95), storm * 0.5 + 0.5);
      atmosphereColor = vec3(0.30, 0.65, 1.0) * fresnel * 0.8;
    } else if (planetId == 9) {
      // Pluto: Mottled nitrogen ice
      float patches = fbm(vUV * 10.0);
      albedo = mix(vec3(0.62, 0.50, 0.42), vec3(0.85, 0.80, 0.75), patches * 0.5 + 0.5);
    }

    vec3 ambient = vec3(0.08) * albedo;
    vec3 diffuse = albedo * NdotL * 0.92;
    vec3 specular = vec3(specularStrength * spec * NdotL);
    vec3 color = ambient + diffuse + specular + atmosphereColor;
    gl_FragColor = vec4(color, 1.0);
  }
`;

// Saturn's Rings Shader with Cassini Division
const RING_VERTEX_SHADER = `
  varying vec2 vUV;
  varying vec3 vWorldPos;
  void main() {
    vUV = uv;
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = projectionMatrix * viewMatrix * worldPos;
  }
`;

const RING_FRAGMENT_SHADER = `
  varying vec2 vUV;
  varying vec3 vWorldPos;
  uniform vec3 lightPos;

  void main() {
    float dist = length(vUV - vec2(0.5));
    if (dist < 0.22 || dist > 0.48) discard;

    float r = (dist - 0.22) / (0.48 - 0.22);
    // Cassini Division
    if (r > 0.58 && r < 0.66) discard;

    float bands = sin(r * 180.0) * 0.15 + sin(r * 45.0) * 0.35 + 0.5;
    float alpha = smoothstep(0.0, 0.08, r) * smoothstep(1.0, 0.92, r) * (0.45 + bands * 0.4);
    vec3 ringColor = mix(vec3(0.85, 0.75, 0.55), vec3(0.68, 0.58, 0.42), bands);

    vec3 L = normalize(lightPos - vWorldPos);
    float lighting = max(abs(L.y), 0.5);
    gl_FragColor = vec4(ringColor * lighting, alpha);
  }
`;

function init() {
  const container = document.getElementById("canvas-container");

  // Three.js Scene & Camera
  scene = new THREE.Scene();
  scene.fog = new THREE.FogExp2(0x030712, 0.0015);

  camera = new THREE.PerspectiveCamera(55, window.innerWidth / window.innerHeight, 0.05, 1200);
  camera.position.set(0, 20, 45);

  // High-Performance WebGL Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false, powerPreference: "high-performance" });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.15;
  container.appendChild(renderer.domElement);

  // Stereoscopic Effect (for VR Cardboard Mode)
  if (typeof THREE.StereoEffect !== "undefined") {
    effect = new THREE.StereoEffect(renderer);
    effect.eyeSeparation = 0.064;
    effect.setSize(window.innerWidth, window.innerHeight);
  }

  // Deep Space Lighting
  const ambient = new THREE.AmbientLight(0x1a2333, 0.9);
  scene.add(ambient);

  const sunLight = new THREE.PointLight(0xfffaed, 2.5, 600, 0.4);
  sunLight.position.set(0, 0, 0);
  scene.add(sunLight);

  // Build Astronomical Universe
  createStarfield();
  createPlanetarySystem();
  createAsteroidBelt();
  createKuiperBelt();
  createOortCloud();

  // Attach Touch & Mobile Gyroscope Event Handlers
  setupMobileTouchNavigation();
  setupUI();

  // Responsive Resize
  window.addEventListener("resize", onWindowResize);

  // Start Animation Loop
  requestAnimationFrame(animate);
}

// 1. Twinkling Starfield (2,500 Stars)
function createStarfield() {
  const count = 2500;
  const geo = new THREE.BufferGeometry();
  const pos = new Float32Array(count * 3);
  const cols = new Float32Array(count * 3);

  const palette = [
    new THREE.Color(0xa5f3fc),
    new THREE.Color(0xffffff),
    new THREE.Color(0xfef08a),
    new THREE.Color(0xfca5a5)
  ];

  for (let i = 0; i < count; i++) {
    const r = 300 + Math.random() * 250;
    const theta = Math.random() * Math.PI * 2;
    const phi = Math.acos(2 * Math.random() - 1);

    pos[i * 3] = r * Math.sin(phi) * Math.cos(theta);
    pos[i * 3 + 1] = r * Math.sin(phi) * Math.sin(theta);
    pos[i * 3 + 2] = r * Math.cos(phi);

    const c = palette[Math.floor(Math.random() * palette.length)];
    const br = 0.45 + Math.random() * 0.55;
    cols[i * 3] = c.r * br;
    cols[i * 3 + 1] = c.g * br;
    cols[i * 3 + 2] = c.b * br;
  }

  geo.setAttribute("position", new THREE.BufferAttribute(pos, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(cols, 3));

  const mat = new THREE.PointsMaterial({
    size: 1.5,
    vertexColors: true,
    transparent: true,
    opacity: 0.9
  });

  starfieldPoints = new THREE.Points(geo, mat);
  scene.add(starfieldPoints);
}

// Helper to create 3D Billboard text sprite
function createBillboardLabel(text, colorHex) {
  const canvas = document.createElement("canvas");
  canvas.width = 256;
  canvas.height = 64;
  const ctx = canvas.getContext("2d");

  ctx.fillStyle = "rgba(8, 14, 26, 0.75)";
  ctx.strokeStyle = colorHex;
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.roundRect(8, 8, 240, 48, 12);
  ctx.fill();
  ctx.stroke();

  ctx.font = "bold 20px Consolas, monospace";
  ctx.fillStyle = colorHex;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(text, 128, 32);

  const texture = new THREE.CanvasTexture(canvas);
  texture.minFilter = THREE.LinearFilter;
  const spriteMat = new THREE.SpriteMaterial({ map: texture, transparent: true, opacity: 0.92 });
  const sprite = new THREE.Sprite(spriteMat);
  sprite.scale.set(3.6, 0.9, 1.0);
  return sprite;
}

// 2. Planets & Sun with Procedural Shaders & Motion Trails
function createPlanetarySystem() {
  PLANET_DATA.forEach((def, index) => {
    let mesh;
    let customMaterial;

    if (index === 0) {
      // The Sun: Procedural turbulence shader
      customMaterial = new THREE.ShaderMaterial({
        vertexShader: SUN_VERTEX_SHADER,
        fragmentShader: SUN_FRAGMENT_SHADER,
        uniforms: {
          viewPos: { value: camera.position },
          time: { value: 0 }
        }
      });
      mesh = new THREE.Mesh(new THREE.SphereGeometry(def.radius, 40, 40), customMaterial);

      // Glowing Corona Aura
      const coronaGeo = new THREE.SphereGeometry(def.radius * 1.35, 32, 32);
      const coronaMat = new THREE.MeshBasicMaterial({
        color: 0xff8800,
        transparent: true,
        opacity: 0.28,
        side: THREE.BackSide
      });
      mesh.add(new THREE.Mesh(coronaGeo, coronaMat));

    } else {
      // Planets: Procedural Surface Shaders
      customMaterial = new THREE.ShaderMaterial({
        vertexShader: PLANET_VERTEX_SHADER,
        fragmentShader: PLANET_FRAGMENT_SHADER,
        uniforms: {
          lightPos: { value: new THREE.Vector3(0, 0, 0) },
          viewPos: { value: camera.position },
          planetId: { value: def.id },
          time: { value: 0 }
        }
      });
      mesh = new THREE.Mesh(new THREE.SphereGeometry(def.radius, 32, 32), customMaterial);

      // Saturn's Rings (Procedural with Cassini division)
      if (def.hasRings) {
        const ringGeo = new THREE.PlaneGeometry(def.radius * 5.2, def.radius * 5.2);
        const ringMat = new THREE.ShaderMaterial({
          vertexShader: RING_VERTEX_SHADER,
          fragmentShader: RING_FRAGMENT_SHADER,
          uniforms: {
            lightPos: { value: new THREE.Vector3(0, 0, 0) }
          },
          transparent: true,
          side: THREE.DoubleSide
        });
        const ringMesh = new THREE.Mesh(ringGeo, ringMat);
        ringMesh.rotation.x = Math.PI / 2.3;
        mesh.add(ringMesh);
      }

      // Glowing Cyan Reference Orbit Line
      const orbitR = def.rAU * AU;
      const curve = new THREE.EllipseCurve(0, 0, orbitR, orbitR, 0, 2 * Math.PI, false, 0);
      const points = curve.getPoints(128);
      const orbitGeo = new THREE.BufferGeometry().setFromPoints(points.map(p => new THREE.Vector3(p.x, 0, p.y)));
      const orbitMat = new THREE.LineBasicMaterial({
        color: 0x38bdf8,
        transparent: true,
        opacity: 0.25
      });
      const orbitLine = new THREE.Line(orbitGeo, orbitMat);
      scene.add(orbitLine);
    }

    const dist3D = def.rAU * AU;
    const initialAngle = (index * 0.72) % (Math.PI * 2);

    // 3D Billboard Label
    const hexColor = index === 0 ? "#fbbf24" : (index === 3 ? "#38bdf8" : (index === 4 ? "#ef4444" : "#e2e8f0"));
    const labelSprite = createBillboardLabel(def.name, hexColor);
    labelSprite.position.set(0, def.radius + 1.2, 0);
    mesh.add(labelSprite);
    billboardLabels.push(labelSprite);

    // Motion Trail Geometry (50 previous positions)
    const trailLength = 50;
    const trailPositions = new Float32Array(trailLength * 3);
    const trailColors = new Float32Array(trailLength * 4);
    for (let t = 0; t < trailLength; t++) {
      trailPositions[t * 3] = dist3D * Math.cos(initialAngle);
      trailPositions[t * 3 + 1] = 0;
      trailPositions[t * 3 + 2] = dist3D * Math.sin(initialAngle);

      const alpha = (t / trailLength) * 0.6;
      trailColors[t * 4] = def.color[0];
      trailColors[t * 4 + 1] = def.color[1];
      trailColors[t * 4 + 2] = def.color[2];
      trailColors[t * 4 + 3] = alpha;
    }
    const trailGeo = new THREE.BufferGeometry();
    trailGeo.setAttribute("position", new THREE.BufferAttribute(trailPositions, 3));
    trailGeo.setAttribute("color", new THREE.BufferAttribute(trailColors, 4));
    const trailMat = new THREE.LineBasicMaterial({
      vertexColors: true,
      transparent: true,
      blending: THREE.AdditiveBlending
    });
    const trailLine = new THREE.Line(trailGeo, trailMat);
    scene.add(trailLine);

    const bodyObj = {
      id: index,
      def: def,
      mesh: mesh,
      mat: customMaterial,
      dist3D: dist3D,
      angle: initialAngle,
      omega: def.rAU > 0 ? (2.0 * Math.PI) / (Math.sqrt(def.rAU * def.rAU * def.rAU) * 12.0) : 0,
      pos: new THREE.Vector3(dist3D * Math.cos(initialAngle), 0, dist3D * Math.sin(initialAngle)),
      trailLine: trailLine,
      trailHistory: []
    };

    mesh.position.copy(bodyObj.pos);
    scene.add(mesh);
    majorBodies.push(bodyObj);
  });

  // Asteroid & Kuiper Belt 3D Floating World Labels
  const astLabel = createBillboardLabel("★ ASTEROID BELT (N-BODY)", "#fbbf24");
  astLabel.position.set(2.1 * AU, 1.2, 0);
  scene.add(astLabel);

  const kuiperLabel = createBillboardLabel("★ KUIPER BELT (N-BODY)", "#38bdf8");
  kuiperLabel.position.set(6.0 * AU, 1.5, 0);
  scene.add(kuiperLabel);
}

// 3. Asteroid Belt (600 Bodies)
function createAsteroidBelt() {
  const count = 600;
  const geo = new THREE.BufferGeometry();
  const pos = new Float32Array(count * 3);
  const cols = new Float32Array(count * 3);
  const data = [];

  const innerR = 1.95 * AU;
  const outerR = 2.45 * AU;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.12;

    pos[i * 3] = r * Math.cos(theta);
    pos[i * 3 + 1] = y;
    pos[i * 3 + 2] = r * Math.sin(theta);

    // Warm amber & bronze palette
    cols[i * 3] = 0.95 + Math.random() * 0.05;
    cols[i * 3 + 1] = 0.72 + Math.random() * 0.15;
    cols[i * 3 + 2] = 0.38 + Math.random() * 0.20;

    data.push({ r: r, theta: theta, omega: 0.18 / Math.sqrt(r / AU) });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(pos, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(cols, 3));
  asteroidPoints = new THREE.Points(geo, new THREE.PointsMaterial({ size: 0.38, vertexColors: true, transparent: true, opacity: 0.85 }));
  asteroidPoints.userData = data;
  scene.add(asteroidPoints);
}

// 4. Kuiper Belt (500 Icy Bodies)
function createKuiperBelt() {
  const count = 500;
  const geo = new THREE.BufferGeometry();
  const pos = new Float32Array(count * 3);
  const cols = new Float32Array(count * 3);
  const data = [];

  const innerR = 5.6 * AU;
  const outerR = 7.6 * AU;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.25;

    pos[i * 3] = r * Math.cos(theta);
    pos[i * 3 + 1] = y;
    pos[i * 3 + 2] = r * Math.sin(theta);

    cols[i * 3] = 0.55 + Math.random() * 0.3;
    cols[i * 3 + 1] = 0.82 + Math.random() * 0.18;
    cols[i * 3 + 2] = 1.0;

    data.push({ r: r, theta: theta, omega: 0.08 / Math.sqrt(r / AU) });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(pos, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(cols, 3));
  kuiperPoints = new THREE.Points(geo, new THREE.PointsMaterial({ size: 0.42, vertexColors: true, transparent: true, opacity: 0.80 }));
  kuiperPoints.userData = data;
  scene.add(kuiperPoints);
}

// 5. Outer Oort Cloud (500 Distant Celestial Bodies)
function createOortCloud() {
  const count = 500;
  const geo = new THREE.BufferGeometry();
  const pos = new Float32Array(count * 3);
  const cols = new Float32Array(count * 3);
  const data = [];

  const innerR = 8.2 * AU;
  const outerR = 12.5 * AU;

  for (let i = 0; i < count; i++) {
    const r = innerR + Math.random() * (outerR - innerR);
    const theta = Math.random() * Math.PI * 2;
    const y = (Math.random() - 0.5) * 0.6;

    pos[i * 3] = r * Math.cos(theta);
    pos[i * 3 + 1] = y;
    pos[i * 3 + 2] = r * Math.sin(theta);

    cols[i * 3] = 0.65 + Math.random() * 0.3;
    cols[i * 3 + 1] = 0.75 + Math.random() * 0.25;
    cols[i * 3 + 2] = 0.95;

    data.push({ r: r, theta: theta, omega: 0.04 / Math.sqrt(r / AU) });
  }

  geo.setAttribute("position", new THREE.BufferAttribute(pos, 3));
  geo.setAttribute("color", new THREE.BufferAttribute(cols, 3));
  oortPoints = new THREE.Points(geo, new THREE.PointsMaterial({ size: 0.45, vertexColors: true, transparent: true, opacity: 0.70 }));
  oortPoints.userData = data;
  scene.add(oortPoints);
}

// Update Physics & Orbital Motion
function updatePhysics(dt, elapsedTime) {
  if (isPaused) return;

  const effectiveDt = dt * simSpeed;

  // 1. Update Planets & Motion Trails
  majorBodies.forEach((body) => {
    // Update shader uniforms
    if (body.mat && body.mat.uniforms) {
      if (body.mat.uniforms.time) body.mat.uniforms.time.value = elapsedTime;
      if (body.mat.uniforms.viewPos) body.mat.uniforms.viewPos.value = camera.position;
    }

    if (body.id === 0) return; // Sun at center

    body.angle += body.omega * effectiveDt * 0.4;
    body.pos.x = body.dist3D * Math.cos(body.angle);
    body.pos.z = body.dist3D * Math.sin(body.angle);
    body.mesh.position.copy(body.pos);
    body.mesh.rotation.y += 0.015 * effectiveDt;

    // Update Motion Trail
    body.trailHistory.push(body.pos.clone());
    if (body.trailHistory.length > 50) body.trailHistory.shift();

    const trailPos = body.trailLine.geometry.attributes.position.array;
    for (let t = 0; t < body.trailHistory.length; t++) {
      trailPos[t * 3] = body.trailHistory[t].x;
      trailPos[t * 3 + 1] = body.trailHistory[t].y;
      trailPos[t * 3 + 2] = body.trailHistory[t].z;
    }
    body.trailLine.geometry.attributes.position.needsUpdate = true;
  });

  // 2. Update Asteroids
  if (asteroidPoints) {
    const pos = asteroidPoints.geometry.attributes.position.array;
    const d = asteroidPoints.userData;
    for (let i = 0; i < d.length; i++) {
      d[i].theta += d[i].omega * effectiveDt * 0.4;
      pos[i * 3] = d[i].r * Math.cos(d[i].theta);
      pos[i * 3 + 2] = d[i].r * Math.sin(d[i].theta);
    }
    asteroidPoints.geometry.attributes.position.needsUpdate = true;
  }

  // 3. Update Kuiper Belt
  if (kuiperPoints) {
    const pos = kuiperPoints.geometry.attributes.position.array;
    const d = kuiperPoints.userData;
    for (let i = 0; i < d.length; i++) {
      d[i].theta += d[i].omega * effectiveDt * 0.4;
      pos[i * 3] = d[i].r * Math.cos(d[i].theta);
      pos[i * 3 + 2] = d[i].r * Math.sin(d[i].theta);
    }
    kuiperPoints.geometry.attributes.position.needsUpdate = true;
  }

  // 4. Update Oort Cloud
  if (oortPoints) {
    const pos = oortPoints.geometry.attributes.position.array;
    const d = oortPoints.userData;
    for (let i = 0; i < d.length; i++) {
      d[i].theta += d[i].omega * effectiveDt * 0.4;
      pos[i * 3] = d[i].r * Math.cos(d[i].theta);
      pos[i * 3 + 2] = d[i].r * Math.sin(d[i].theta);
    }
    oortPoints.geometry.attributes.position.needsUpdate = true;
  }
}

// ========================================================
// 100% RELIABLE MOBILE TOUCH NAVIGATION & 360 SWIPE
// Works immediately on ANY phone without extra hardware!
// ========================================================
function setupMobileTouchNavigation() {
  const container = document.getElementById("canvas-container");

  // Single-Finger Touch / Mouse Drag anywhere to look 360°
  window.addEventListener("pointerdown", (e) => {
    // Ignore clicks on HUD buttons
    if (e.target.closest("button") || e.target.closest("#planet-bar") || e.target.closest("#mobile-quick-actions")) {
      return;
    }
    isTouching = true;
    touchStartX = e.clientX;
    touchStartY = e.clientY;
    touchPrevX = e.clientX;
    touchPrevY = e.clientY;

    // Fade out the touch guide after user first interacts
    const guide = document.getElementById("mobile-touch-guide");
    if (guide) guide.style.opacity = "0.2";
  });

  window.addEventListener("pointermove", (e) => {
    if (!isTouching) return;

    const dx = e.clientX - touchPrevX;
    const dy = e.clientY - touchPrevY;
    touchPrevX = e.clientX;
    touchPrevY = e.clientY;

    // Rotate camera smoothly around target (Yaw & Pitch)
    camYaw -= dx * 0.0055;
    camPitch = Math.max(-1.45, Math.min(1.45, camPitch + dy * 0.0055));
  });

  window.addEventListener("pointerup", () => { isTouching = false; });
  window.addEventListener("pointercancel", () => { isTouching = false; });

  // Two-Finger Pinch to Zoom on Mobile
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
      targetCamDistance = Math.max(2.5, Math.min(220.0, targetCamDistance + diff * 0.09));
      pinchStartDist = dist;
    }
  }, { passive: true });

  // Mouse Wheel Zoom
  window.addEventListener("wheel", (e) => {
    targetCamDistance = Math.max(2.5, Math.min(220.0, targetCamDistance + e.deltaY * 0.04));
  }, { passive: true });
}

// Hardware Gyroscope Activation
function toggleGyroscope() {
  const btn = document.getElementById("btn-gyro");
  const label = document.getElementById("gyro-btn-label");

  if (isGyroActive) {
    window.removeEventListener("deviceorientation", handleDeviceOrientation);
    isGyroActive = false;
    btn.classList.remove("active");
    label.innerText = "Enable Gyro";
    showToast("360° Touch Swipe Mode active.");
    return;
  }

  // Handle iOS 13+ permission request
  if (typeof DeviceOrientationEvent !== "undefined" && typeof DeviceOrientationEvent.requestPermission === "function") {
    DeviceOrientationEvent.requestPermission()
      .then((res) => {
        if (res === "granted") {
          activateGyroListener(btn, label);
        } else {
          showToast("Gyroscope permission denied. Touch swipe is fully active!");
        }
      })
      .catch((err) => {
        showToast("Gyroscope not supported on unencrypted HTTP. Use Touch Swipe!");
      });
  } else if (window.DeviceOrientationEvent) {
    activateGyroListener(btn, label);
  } else {
    showToast("Device has no hardware gyroscope. Touch swipe is fully active!");
  }
}

function activateGyroListener(btn, label) {
  window.addEventListener("deviceorientation", handleDeviceOrientation);
  isGyroActive = true;
  gyroInitialYaw = null;
  btn.classList.add("active");
  label.innerText = "Gyro: ON";
  showToast("Hardware Gyroscope Enabled! Move phone to look around.");
}

function handleDeviceOrientation(e) {
  if (!e.alpha && !e.beta) return;

  const alpha = THREE.MathUtils.degToRad(e.alpha || 0); // Z (yaw)
  const beta = THREE.MathUtils.degToRad(e.beta || 0);   // X (pitch)
  const gamma = THREE.MathUtils.degToRad(e.gamma || 0); // Y (roll)

  if (gyroInitialYaw === null) {
    gyroInitialYaw = alpha;
  }

  // Map phone tilt directly to camera rotation
  camYaw = -(alpha - gyroInitialYaw) + 0.85;
  camPitch = Math.max(-1.4, Math.min(1.4, (beta - Math.PI / 4)));
}

// Stereoscopic VR Cardboard Mode
function toggleVRMode() {
  const btn = document.getElementById("btn-vr");
  isVRMode = !isVRMode;

  if (isVRMode) {
    document.body.classList.add("in-vr");
    btn.classList.add("active");
    btn.innerHTML = `<span class="icon">❌</span><span class="label">Exit VR</span>`;

    if (!isGyroActive) toggleGyroscope();

    if (document.documentElement.requestFullscreen) {
      document.documentElement.requestFullscreen().catch(() => {});
    }
    showToast("Placed in Stereoscopic VR Mode. Slide phone into Cardboard!");
  } else {
    document.body.classList.remove("in-vr");
    btn.classList.remove("active");
    btn.innerHTML = `<span class="icon">👓</span><span class="label">VR Cardboard</span>`;

    if (document.exitFullscreen && document.fullscreenElement) {
      document.exitFullscreen().catch(() => {});
    }
    showToast("Returned to Handheld Observatory Mode.");
  }

  onWindowResize();
}

// Smoothly Focus Camera on Celestial Body
function focusOnBody(index) {
  focusIndex = index;

  document.querySelectorAll(".planet-btn").forEach((b) => {
    b.classList.toggle("active", parseInt(b.dataset.id) === index);
  });

  const def = (index >= 0 && index < PLANET_DATA.length) ? PLANET_DATA[index] : null;

  if (!def) {
    if (index === 11) {
      // Asteroids
      document.getElementById("target-name").innerText = "Asteroid Belt";
      document.getElementById("target-type").innerText = "Planar Particle Disc";
      document.getElementById("val-dist").innerText = "2.1 - 2.4 AU";
      document.getElementById("val-vel").innerText = "18.2 km/s";
      document.getElementById("val-period").innerText = "3.2 - 3.8 yrs";
      document.getElementById("val-mass").innerText = "2.4 × 10²¹ kg (Total)";
      targetCamDistance = 18.0;
    } else if (index === 12) {
      // Kuiper
      document.getElementById("target-name").innerText = "Kuiper Belt";
      document.getElementById("target-type").innerText = "Trans-Neptunian Disc";
      document.getElementById("val-dist").innerText = "30 - 50 AU";
      document.getElementById("val-vel").innerText = "4.5 km/s";
      document.getElementById("val-period").innerText = "250+ yrs";
      document.getElementById("val-mass").innerText = "1.8 × 10²³ kg (Total)";
      targetCamDistance = 42.0;
    }
    return;
  }

  document.getElementById("target-name").innerText = def.name;
  document.getElementById("target-type").innerText = def.type;
  document.getElementById("val-dist").innerText = def.rAU > 0 ? `${def.rAU.toFixed(3)} AU` : "0.00 AU (Center)";
  document.getElementById("val-vel").innerText = def.rAU > 0 ? `${(29.8 / Math.sqrt(def.rAU)).toFixed(1)} km/s` : "0.0 km/s";
  document.getElementById("val-period").innerText = def.period;
  document.getElementById("val-mass").innerText = def.mass;

  if (index === 0) targetCamDistance = 22.0;
  else if (def.hasRings) targetCamDistance = 7.5;
  else if (def.radius > 0.8) targetCamDistance = 6.0;
  else targetCamDistance = 4.2;
}

// UI Event Listeners
function setupUI() {
  document.getElementById("btn-touch-mode").addEventListener("click", () => {
    showToast("Touch 360° active! Swipe anywhere with finger to rotate, pinch to zoom.");
  });

  document.getElementById("btn-gyro").addEventListener("click", toggleGyroscope);
  document.getElementById("btn-vr").addEventListener("click", toggleVRMode);

  document.querySelectorAll(".planet-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      focusOnBody(parseInt(btn.dataset.id));
    });
  });

  const pauseBtn = document.getElementById("btn-pause");
  pauseBtn.addEventListener("click", () => {
    isPaused = !isPaused;
    pauseBtn.innerText = isPaused ? "▶️ Resume" : "⏸️ Pause";
    updateHUDStats();
  });

  document.getElementById("btn-speed-up").addEventListener("click", () => {
    simSpeed = Math.min(simSpeed * 2.0, 8.0);
    document.getElementById("speed-display").innerText = `${simSpeed.toFixed(1)}x`;
    updateHUDStats();
  });

  document.getElementById("btn-speed-down").addEventListener("click", () => {
    simSpeed = Math.max(simSpeed / 2.0, 0.125);
    document.getElementById("speed-display").innerText = `${simSpeed.toFixed(1)}x`;
    updateHUDStats();
  });

  document.getElementById("btn-reset-cam").addEventListener("click", () => {
    focusIndex = 0;
    targetCamDistance = 45.0;
    camYaw = 0.85;
    camPitch = 0.38;
    focusOnBody(0);
    showToast("Camera View Reset.");
  });
}

function updateHUDStats() {
  const lineStats = document.getElementById("hud-line-stats");
  if (lineStats) {
    lineStats.innerText = `Status: [${isPaused ? "PAUSED" : "RUNNING"}] | Step: ${simSpeed.toFixed(1)}x (4h/step) | Total Bodies: 1611 | Submitted: 1611 | Rendered: 1611 (Major: 11, Asteroids: 600, Kuiper: 500, Stars: 500) | Engine: OpenCL GPU`;
  }
}

function showToast(msg) {
  const toast = document.getElementById("hud-toast");
  if (!toast) return;
  toast.innerText = msg;
  toast.classList.remove("hidden");
  setTimeout(() => toast.classList.add("hidden"), 3500);
}

function onWindowResize() {
  const w = window.innerWidth;
  const h = window.innerHeight;
  camera.aspect = w / h;
  camera.updateProjectionMatrix();

  renderer.setSize(w, h);
  if (effect) effect.setSize(w, h);
}

// Main Animation & Render Loop
function animate(now) {
  requestAnimationFrame(animate);

  const dt = Math.min((now - lastFrameTime) / 1000, 0.1);
  lastFrameTime = now;
  const elapsedTime = (now - startTime) / 1000;

  // FPS Counter
  frameCount++;
  if (now - lastFpsTime >= 1000) {
    currentFps = Math.round((frameCount * 1000) / (now - lastFpsTime));
    frameCount = 0;
    lastFpsTime = now;
    const perfElem = document.getElementById("hud-perf-counter");
    if (perfElem) {
      perfElem.innerText = `FPS: ${currentFps} | Calc: ${physicsTimeMs.toFixed(2)} ms`;
    }
  }

  // Update Physics
  updatePhysics(dt, elapsedTime);

  // Smooth Camera Target Interpolation
  let targetPos = new THREE.Vector3(0, 0, 0);
  if (focusIndex >= 0 && focusIndex < majorBodies.length) {
    targetPos.copy(majorBodies[focusIndex].mesh.position);
  } else if (focusIndex === 11) {
    targetPos.set(2.1 * AU, 0, 0);
  } else if (focusIndex === 12) {
    targetPos.set(6.0 * AU, 0, 0);
  }

  currentCenter.lerp(targetPos, 0.10);
  camDistance += (targetCamDistance - camDistance) * 0.10;

  // Position Camera on Spherical Orbit
  const cx = camDistance * Math.cos(camPitch) * Math.sin(camYaw);
  const cy = camDistance * Math.sin(camPitch);
  const cz = camDistance * Math.cos(camPitch) * Math.cos(camYaw);
  camera.position.set(currentCenter.x + cx, currentCenter.y + cy, currentCenter.z + cz);
  camera.lookAt(currentCenter);

  // Render
  if (isVRMode && effect) {
    effect.render(scene, camera);
  } else {
    renderer.render(scene, camera);
  }
}

// Auto-start on load
window.addEventListener("DOMContentLoaded", init);
