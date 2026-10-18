/**
 * @file SolarSystemVisuals.cpp
 * @brief High-quality 3D celestial rendering: procedural planets, glowing Sun,
 *        Saturn's rings, comet tails, orbital paths, motion trails, starfield,
 *        billboard labels, and HUD overlay.
 */

#include "../../include/Render/SolarSystemVisuals.hpp"
#include <iostream>
#include <cmath>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SolarSystemVisuals& SolarSystemVisuals::getInstance() {
    static SolarSystemVisuals instance;
    return instance;
}

static const char* SUN_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

uniform mat4 M_model;
uniform mat4 M_view;
uniform mat4 M_projection;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

void main() {
    vNormal = inNormal;
    vUV = inUV;
    vec4 worldPos = M_model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = M_projection * M_view * worldPos;
}
)";

static const char* SUN_FS = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

uniform vec3 viewPos;
uniform float time;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    // Procedural solar surface turbulence
    float n1 = sin(N.x * 22.0 + time * 1.8) * cos(N.y * 22.0 - time * 1.5) * sin(N.z * 22.0 + time * 1.2);
    float n2 = sin(N.y * 45.0 + time * 2.5) * cos(N.z * 45.0 - time * 2.0);
    float granulation = (n1 * 0.6 + n2 * 0.4) * 0.22;

    // Solar limb darkening and corona gradient
    vec3 sunCore = vec3(1.0, 0.96, 0.82);
    vec3 sunMid  = vec3(1.0, 0.62, 0.12);
    vec3 sunRim  = vec3(0.95, 0.30, 0.04);

    float t = clamp(NdotV + granulation, 0.0, 1.0);
    vec3 baseColor = mix(sunRim, sunMid, smoothstep(0.0, 0.5, t));
    baseColor = mix(baseColor, sunCore, smoothstep(0.5, 1.0, t));

    // Outer coronal glare
    float corona = pow(1.0 - NdotV, 2.5) * 1.2;
    vec3 finalColor = baseColor * 1.25 + vec3(1.0, 0.55, 0.15) * corona;

    fragColor = vec4(finalColor, 1.0);
}
)";

static const char* PLANET_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

uniform mat4 M_model;
uniform mat4 M_view;
uniform mat4 M_projection;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

void main() {
    vNormal = mat3(transpose(inverse(M_model))) * inNormal;
    vUV = inUV;
    vec4 worldPos = M_model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = M_projection * M_view * worldPos;
}
)";

static const char* PLANET_FS = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

uniform vec3 lightPos; // Sun position
uniform vec3 viewPos;
uniform int planetId;
uniform float time;

out vec4 fragColor;

// Simplex-style pseudo noise
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

    // Procedural surface details based on planetId
    if (planetId == 1) {
        // Mercury: heavily cratered gray-brown rocky surface
        float craters = fbm(vUV * 30.0);
        albedo = mix(vec3(0.48, 0.46, 0.44), vec3(0.68, 0.65, 0.62), craters * 0.5 + 0.5);
    } else if (planetId == 2) {
        // Venus: golden-yellow dense atmospheric haze
        float clouds = sin(vUV.y * 14.0 + fbm(vUV * 8.0) * 2.5);
        albedo = mix(vec3(0.88, 0.76, 0.50), vec3(0.96, 0.88, 0.68), clouds * 0.5 + 0.5);
        atmosphereColor = vec3(0.95, 0.85, 0.60) * fresnel * 0.8;
    } else if (planetId == 3) {
        // Earth: deep blue oceans, green/brown continents, polar ice caps, clouds
        float continentNoise = fbm(vUV * 8.0);
        float isLand = smoothstep(0.05, 0.15, continentNoise);
        vec3 oceanColor = vec3(0.08, 0.28, 0.68);
        vec3 landColor = mix(vec3(0.22, 0.52, 0.22), vec3(0.55, 0.48, 0.32), fbm(vUV * 16.0) * 0.5 + 0.5);
        albedo = mix(oceanColor, landColor, isLand);

        // Polar ice caps
        if (abs(N.y) > 0.82) {
            albedo = vec3(0.95, 0.98, 1.0);
            isLand = 1.0;
        }

        // Swirling dynamic white clouds
        float cloudPattern = fbm(vUV * 12.0 + vec2(time * 0.015, 0.0));
        float clouds = smoothstep(0.18, 0.45, cloudPattern);
        albedo = mix(albedo, vec3(0.98, 0.98, 1.0), clouds * 0.85);

        specularStrength = (1.0 - isLand) * (1.0 - clouds) * 0.75;
        atmosphereColor = vec3(0.25, 0.60, 1.0) * fresnel * 1.1;
    } else if (planetId == 4) {
        // Mars: rusty iron-red surface with dark basalt patches and polar caps
        float terrain = fbm(vUV * 12.0);
        albedo = mix(vec3(0.82, 0.35, 0.18), vec3(0.48, 0.22, 0.12), terrain * 0.5 + 0.5);
        if (abs(N.y) > 0.86) {
            albedo = vec3(0.95, 0.96, 0.98); // North/South polar ice caps
        }
        atmosphereColor = vec3(0.85, 0.45, 0.25) * fresnel * 0.35;
    } else if (planetId == 5) {
        // Jupiter: alternating cream/amber/terracotta bands and Great Red Spot
        float bands = sin(vUV.y * 36.0 + fbm(vec2(vUV.x * 6.0, vUV.y * 12.0)) * 1.5);
        vec3 bandLight = vec3(0.92, 0.86, 0.75);
        vec3 bandDark  = vec3(0.68, 0.45, 0.28);
        albedo = mix(bandDark, bandLight, bands * 0.5 + 0.5);

        // Great Red Spot at latitude -0.32, longitude 0.45
        vec2 spotCenter = vec2(0.45, 0.34);
        vec2 d = (vUV - spotCenter) * vec2(2.5, 1.2);
        float spotDist = length(d);
        if (spotDist < 0.065) {
            float spot = smoothstep(0.065, 0.02, spotDist);
            albedo = mix(albedo, vec3(0.82, 0.25, 0.16), spot * 0.9);
        }
    } else if (planetId == 6) {
        // Saturn: golden-butterscotch atmospheric bands
        float bands = sin(vUV.y * 42.0 + fbm(vec2(vUV.x * 4.0, vUV.y * 8.0)) * 0.8);
        albedo = mix(vec3(0.78, 0.68, 0.50), vec3(0.92, 0.84, 0.64), bands * 0.5 + 0.5);
    } else if (planetId == 7) {
        // Uranus: smooth pale cyan-aquamarine ice atmosphere
        albedo = vec3(0.58, 0.84, 0.88);
        atmosphereColor = vec3(0.50, 0.90, 0.95) * fresnel * 0.7;
    } else if (planetId == 8) {
        // Neptune: deep royal azure blue with bright cirrus streaks
        float storm = fbm(vUV * 10.0 + vec2(time * 0.02, 0.0));
        albedo = mix(vec3(0.15, 0.35, 0.85), vec3(0.22, 0.55, 0.95), storm * 0.5 + 0.5);
        atmosphereColor = vec3(0.30, 0.65, 1.0) * fresnel * 0.8;
    } else if (planetId == 9) {
        // Pluto: mottled tan, brown and white nitrogen ice heart
        float patches = fbm(vUV * 10.0);
        albedo = mix(vec3(0.62, 0.50, 0.42), vec3(0.85, 0.80, 0.75), patches * 0.5 + 0.5);
    } else if (planetId == 10) {
        // Comet nucleus: bright icy white-cyan
        albedo = vec3(0.88, 0.94, 1.0);
        specularStrength = 0.5;
    }

    vec3 ambient = vec3(0.08) * albedo; // Soft starlight on dark side
    vec3 diffuse = albedo * NdotL * 0.92;
    vec3 specular = vec3(specularStrength * spec * NdotL);

    vec3 color = ambient + diffuse + specular + atmosphereColor;
    fragColor = vec4(color, 1.0);
}
)";

static const char* RING_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

uniform mat4 M_model;
uniform mat4 M_view;
uniform mat4 M_projection;

out vec2 vUV;
out vec3 vWorldPos;

void main() {
    vUV = inUV;
    vec4 worldPos = M_model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = M_projection * M_view * worldPos;
}
)";

static const char* RING_FS = R"(#version 330 core
in vec2 vUV;
in vec3 vWorldPos;

uniform vec3 lightPos;
out vec4 fragColor;

void main() {
    // Distance from center of ring in [0, 1] mapped UV
    float dist = length(vUV - vec2(0.5));
    // Valid ring range in UV radius: 0.22 to 0.48
    if (dist < 0.22 || dist > 0.48) {
        discard;
    }

    // Normalized ring radius coordinate [0, 1]
    float r = (dist - 0.22) / (0.48 - 0.22);

    // Cassini Division gap between r=0.58 and r=0.66
    if (r > 0.58 && r < 0.66) {
        discard;
    }

    // Multiple concentric ring bands
    float bands = sin(r * 180.0) * 0.15 + sin(r * 45.0) * 0.35 + 0.5;
    float alpha = smoothstep(0.0, 0.08, r) * smoothstep(1.0, 0.92, r) * (0.45 + bands * 0.4);

    vec3 ringColor = mix(vec3(0.85, 0.75, 0.55), vec3(0.68, 0.58, 0.42), bands);

    // Directional lighting
    vec3 L = normalize(lightPos - vWorldPos);
    float lighting = max(abs(L.y), 0.5);

    fragColor = vec4(ringColor * lighting, alpha);
}
)";

static const char* STARFIELD_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in float inSize;

uniform mat4 M_view;
uniform mat4 M_projection;
uniform float time;

out vec3 vColor;
out float vTwinkle;

void main() {
    // Keep stars infinitely far by zeroing translation in view matrix
    mat4 viewNoTrans = M_view;
    viewNoTrans[3] = vec4(0.0, 0.0, 0.0, 1.0);

    gl_Position = M_projection * viewNoTrans * vec4(inPos, 1.0);
    // Smooth multi-frequency twinkle
    float twinkle = sin(time * 2.8 + inPos.x * 14.0 + inPos.y * 24.0) * 0.22 + 0.78;
    gl_PointSize = clamp(inSize * twinkle, 2.5, 7.5);
    vColor = inColor;
    vTwinkle = twinkle;
}
)";

static const char* STARFIELD_FS = R"(#version 330 core
in vec3 vColor;
in float vTwinkle;
out vec4 fragColor;

void main() {
    float alpha = vTwinkle;
    if (gl_PointCoord != vec2(0.0, 0.0)) {
        vec2 circ = gl_PointCoord - vec2(0.5);
        float dist = length(circ);
        if (dist > 0.5) {
            discard;
        }
        alpha = smoothstep(0.5, 0.05, dist) * vTwinkle;
    }
    fragColor = vec4(vColor, alpha);
}
)";

static const char* PARTICLE_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in float inMass;

uniform mat4 M_view;
uniform mat4 M_projection;
uniform vec3 camPos;
uniform float maxPos;
uniform float time;

out vec3 vColor;
out float vType; // 0: Asteroid, 1: Kuiper, 2: Simulated Star

void main() {
    vec3 normPos = inPos / maxPos;
    gl_Position = M_projection * M_view * vec4(normPos, 1.0);

    float rOrbit = length(normPos.xz);
    float absY = abs(normPos.y);
    float distToCam = length(camPos - normPos);
    float distAtten = 1.75 / max(distToCam, 0.35);

    // 1. Asteroid Belt (r ~ 0.33 to 0.49, near orbital plane)
    if (rOrbit >= 0.33 && rOrbit <= 0.49 && absY < 0.06) {
        vType = 0.0;
        float hash = fract(sin(dot(normPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
        if (hash < 0.38) {
            vColor = vec3(1.0, 0.82, 0.42); // Golden amber
        } else if (hash < 0.72) {
            vColor = vec3(0.95, 0.62, 0.28); // Ochre bronze
        } else {
            vColor = vec3(0.88, 0.82, 0.72); // Silicate stone
        }
        gl_PointSize = clamp(5.4 * distAtten, 4.0, 13.0);
    }
    // 2. Kuiper Belt (r >= 0.98, near orbital plane)
    else if (rOrbit >= 0.98 && absY < 0.12) {
        vType = 1.0;
        float hash = fract(sin(dot(normPos.xz, vec2(37.123, 91.456))) * 28461.123);
        if (hash < 0.42) {
            vColor = vec3(0.40, 0.88, 1.0); // Electric cyan
        } else if (hash < 0.76) {
            vColor = vec3(0.72, 0.94, 1.0); // Icy blue-white
        } else {
            vColor = vec3(0.50, 1.0, 0.82); // Aquamarine
        }
        gl_PointSize = clamp(4.8 * distAtten, 3.6, 11.5);
    }
    // 3. Simulated Stars / Outer halo bodies
    else {
        vType = 2.0;
        float hash = fract(sin(dot(normPos.xy, vec2(53.412, 19.821))) * 61829.341);
        if (hash < 0.35) {
            vColor = vec3(1.0, 0.96, 0.90); // Diamond white
        } else if (hash < 0.70) {
            vColor = vec3(0.55, 0.82, 1.0); // Sapphire star
        } else {
            vColor = vec3(1.0, 0.85, 0.48); // Golden star
        }
        float twinkle = sin(time * 3.2 + normPos.x * 12.0 + normPos.z * 18.0) * 0.18 + 0.82;
        gl_PointSize = clamp(6.2 * distAtten * twinkle, 4.5, 15.0);
    }
}
)";

static const char* PARTICLE_FS = R"(#version 330 core
in vec3 vColor;
in float vType;
out vec4 fragColor;

void main() {
    float alpha = 1.0;
    if (gl_PointCoord != vec2(0.0, 0.0)) {
        vec2 circ = 2.0 * gl_PointCoord - vec2(1.0);
        float r2 = dot(circ, circ);
        if (r2 > 1.0) {
            discard;
        }
        float dist = sqrt(r2);
        // Radiant bright core with smooth glowing halo
        float core = clamp(1.0 - dist * 1.5, 0.0, 1.0);
        float glow = pow(clamp(1.0 - dist, 0.0, 1.0), 1.5);
        alpha = clamp(core * 0.90 + glow * 0.70, 0.0, 1.0);
    }
    fragColor = vec4(vColor, alpha);
}
)";

static const char* LINE_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

uniform mat4 M_view;
uniform mat4 M_projection;

out vec4 vColor;

void main() {
    vColor = inColor;
    gl_Position = M_projection * M_view * vec4(inPos, 1.0);
}
)";

static const char* LINE_FS = R"(#version 330 core
in vec4 vColor;
out vec4 fragColor;

void main() {
    fragColor = vColor;
}
)";

static const char* COMET_VS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

uniform mat4 M_view;
uniform mat4 M_projection;

out vec4 vColor;

void main() {
    vColor = inColor;
    gl_Position = M_projection * M_view * vec4(inPos, 1.0);
}
)";

static const char* COMET_FS = R"(#version 330 core
in vec4 vColor;
out vec4 fragColor;

void main() {
    fragColor = vColor;
}
)";

GLuint SolarSystemVisuals::compileShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation failed:\n" << log << std::endl;
    }
    return s;
}

GLuint SolarSystemVisuals::linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cerr << "Program linking failed:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

void SolarSystemVisuals::buildShaders() {
    sunShader = linkProgram(compileShader(GL_VERTEX_SHADER, SUN_VS),
                            compileShader(GL_FRAGMENT_SHADER, SUN_FS));

    planetShader = linkProgram(compileShader(GL_VERTEX_SHADER, PLANET_VS),
                              compileShader(GL_FRAGMENT_SHADER, PLANET_FS));

    ringShader = linkProgram(compileShader(GL_VERTEX_SHADER, RING_VS),
                            compileShader(GL_FRAGMENT_SHADER, RING_FS));

    starfieldShader = linkProgram(compileShader(GL_VERTEX_SHADER, STARFIELD_VS),
                                compileShader(GL_FRAGMENT_SHADER, STARFIELD_FS));

    simpleLineShader = linkProgram(compileShader(GL_VERTEX_SHADER, LINE_VS),
                                  compileShader(GL_FRAGMENT_SHADER, LINE_FS));

    cometShader = linkProgram(compileShader(GL_VERTEX_SHADER, COMET_VS),
                             compileShader(GL_FRAGMENT_SHADER, COMET_FS));

    particleShader = linkProgram(compileShader(GL_VERTEX_SHADER, PARTICLE_VS),
                                 compileShader(GL_FRAGMENT_SHADER, PARTICLE_FS));
}

void SolarSystemVisuals::buildSphere(int slices, int stacks) {
    std::vector<float> data;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = (float)i / (float)stacks * (float)M_PI; // [0, PI]
        float y = std::cos(phi);
        float r = std::sin(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j / (float)slices * 2.0f * (float)M_PI; // [0, 2PI]
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            float u = (float)j / (float)slices;
            float v = (float)i / (float)stacks;

            // pos(3), normal(3), uv(2)
            data.push_back(x);
            data.push_back(y);
            data.push_back(z);

            data.push_back(x);
            data.push_back(y);
            data.push_back(z);

            data.push_back(u);
            data.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int first = i * (slices + 1) + j;
            unsigned int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    sphere.indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &sphere.vao);
    glGenBuffers(1, &sphere.vbo);
    glGenBuffers(1, &sphere.ebo);

    glBindVertexArray(sphere.vao);

    glBindBuffer(GL_ARRAY_BUFFER, sphere.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SolarSystemVisuals::buildSaturnRings(int segments) {
    std::vector<float> data;
    std::vector<unsigned int> indices;

    float rInner = 1.35f;
    float rOuter = 2.45f;

    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        // Inner vertex (x, y, z, u, v)
        float xIn = rInner * cosT;
        float zIn = rInner * sinT;
        float uIn = 0.5f + 0.22f * cosT;
        float vIn = 0.5f + 0.22f * sinT;
        data.push_back(xIn); data.push_back(0.0f); data.push_back(zIn);
        data.push_back(uIn); data.push_back(vIn);

        // Outer vertex (x, y, z, u, v)
        float xOut = rOuter * cosT;
        float zOut = rOuter * sinT;
        float uOut = 0.5f + 0.48f * cosT;
        float vOut = 0.5f + 0.48f * sinT;
        data.push_back(xOut); data.push_back(0.0f); data.push_back(zOut);
        data.push_back(uOut); data.push_back(vOut);
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int in1 = i * 2;
        unsigned int out1 = in1 + 1;
        unsigned int in2 = (i + 1) * 2;
        unsigned int out2 = in2 + 1;

        indices.push_back(in1);
        indices.push_back(out1);
        indices.push_back(in2);

        indices.push_back(out1);
        indices.push_back(out2);
        indices.push_back(in2);
    }

    saturnRing.indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &saturnRing.vao);
    glGenBuffers(1, &saturnRing.vbo);
    glGenBuffers(1, &saturnRing.ebo);

    glBindVertexArray(saturnRing.vao);

    glBindBuffer(GL_ARRAY_BUFFER, saturnRing.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, saturnRing.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void SolarSystemVisuals::buildStarfield(int starCount) {
    if (starCount < 8000) starCount = 8000;
    starfield.starCount = starCount;
    std::vector<float> data; // pos(3), color(3), size(1) -> 7 floats

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * (float)M_PI);
    std::uniform_real_distribution<float> zDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> rDist(115.0f, 215.0f);
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(2.5f, 6.5f);
    std::uniform_real_distribution<float> milkyDist(-0.25f, 0.25f);

    for (int i = 0; i < starCount; ++i) {
        float phi = angleDist(rng);
        float z = (i < starCount / 3) ? milkyDist(rng) : zDist(rng);
        float theta = std::asin(std::clamp(z, -0.999f, 0.999f));
        float radius = rDist(rng);

        float x = radius * std::cos(theta) * std::cos(phi);
        float y = radius * std::sin(theta);
        float zz = radius * std::cos(theta) * std::sin(phi);

        float c = colorDist(rng);
        glm::vec3 col;
        if (c < 0.22f) {
            col = glm::vec3(0.68f, 0.82f, 1.0f); // Class O/B Hot Blue-white
        } else if (c < 0.65f) {
            col = glm::vec3(0.96f, 0.98f, 1.0f); // Class A Pure brilliant white
        } else if (c < 0.85f) {
            col = glm::vec3(1.0f, 0.94f, 0.78f); // Class F/G Solar warm yellow
        } else if (c < 0.95f) {
            col = glm::vec3(1.0f, 0.70f, 0.42f); // Class K Orange giant
        } else {
            col = glm::vec3(1.0f, 0.45f, 0.32f); // Class M Red giant
        }

        float sz = sizeDist(rng);

        data.push_back(x); data.push_back(y); data.push_back(zz);
        data.push_back(col.r); data.push_back(col.g); data.push_back(col.b);
        data.push_back(sz);
    }

    glGenVertexArrays(1, &starfield.vao);
    glGenBuffers(1, &starfield.vbo);

    glBindVertexArray(starfield.vao);
    glBindBuffer(GL_ARRAY_BUFFER, starfield.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // size
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SolarSystemVisuals::buildOrbitMeshes(float maxPos) {
    // Scaled radii for Mercury, Venus, Earth, Mars, Asteroid belt, Jupiter, Saturn, Uranus, Neptune, Pluto
    std::vector<float> normRadii = {
        0.095f, // Mercury (well outside Sun at 0.038)
        0.155f, // Venus
        0.225f, // Earth
        0.305f, // Mars
        0.410f, // Asteroid Belt center
        0.540f, // Jupiter
        0.690f, // Saturn
        0.840f, // Uranus
        0.980f, // Neptune
        1.120f  // Pluto
    };

    orbitMeshes.clear();
    const int segments = 128;

    for (float rNorm : normRadii) {
        std::vector<float> points;

        for (int i = 0; i <= segments; ++i) {
            float theta = (float)i / (float)segments * 2.0f * (float)M_PI;
            float x = rNorm * std::cos(theta);
            float z = rNorm * std::sin(theta);
            points.push_back(x);
            points.push_back(0.0f);
            points.push_back(z);
        }

        OrbitMesh om;
        om.pointCount = segments + 1;
        glGenVertexArrays(1, &om.vao);
        glGenBuffers(1, &om.vbo);

        glBindVertexArray(om.vao);
        glBindBuffer(GL_ARRAY_BUFFER, om.vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        orbitMeshes.push_back(om);
    }
}

void SolarSystemVisuals::init(float maxPosition) {
    if (initialized) return;
    cachedMaxPos = maxPosition;

    buildShaders();
    buildSphere();
    buildSaturnRings();
    buildStarfield();
    buildOrbitMeshes(maxPosition);

    // Initialize trail history for 11 major bodies
    trailHistory.resize(11);

    glGenVertexArrays(1, &trailVao);
    glGenBuffers(1, &trailVbo);
    glBindVertexArray(trailVao);
    glBindBuffer(GL_ARRAY_BUFFER, trailVbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_TRAIL_LENGTH * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenVertexArrays(1, &particleVao);

    initialized = true;
}

void SolarSystemVisuals::cleanup() {
    if (!initialized) return;

    glDeleteVertexArrays(1, &sphere.vao);
    glDeleteBuffers(1, &sphere.vbo);
    glDeleteBuffers(1, &sphere.ebo);

    glDeleteVertexArrays(1, &saturnRing.vao);
    glDeleteBuffers(1, &saturnRing.vbo);
    glDeleteBuffers(1, &saturnRing.ebo);

    glDeleteVertexArrays(1, &starfield.vao);
    glDeleteBuffers(1, &starfield.vbo);

    for (auto &om : orbitMeshes) {
        glDeleteVertexArrays(1, &om.vao);
        glDeleteBuffers(1, &om.vbo);
    }
    orbitMeshes.clear();

    glDeleteVertexArrays(1, &trailVao);
    glDeleteBuffers(1, &trailVbo);

    glDeleteVertexArrays(1, &particleVao);

    glDeleteProgram(sunShader);
    glDeleteProgram(planetShader);
    glDeleteProgram(ringShader);
    glDeleteProgram(starfieldShader);
    glDeleteProgram(simpleLineShader);
    glDeleteProgram(cometShader);
    glDeleteProgram(particleShader);

    initialized = false;
}

glm::vec3 SolarSystemVisuals::getBodyNormalizedPosition(int bodyIndex, const float *majorPositions, float maxPos) const {
    if (bodyIndex < 0 || bodyIndex > 10 || !majorPositions) {
        return glm::vec3(0.0f);
    }
    // Simulation coordinates are (x, y, z) where orbital plane is X-Z
    float x = majorPositions[bodyIndex * 3] / maxPos;
    float y = majorPositions[bodyIndex * 3 + 1] / maxPos;
    float z = majorPositions[bodyIndex * 3 + 2] / maxPos;
    return glm::vec3(x, y, z);
}

std::string SolarSystemVisuals::getBodyName(int bodyIndex) const {
    static const char* names[] = {
        "SUN", "MERCURY", "VENUS", "EARTH", "MARS",
        "JUPITER", "SATURN", "URANUS", "NEPTUNE", "PLUTO", "COMET 1P/HALLEY"
    };
    if (bodyIndex >= 0 && bodyIndex <= 10) return names[bodyIndex];
    if (bodyIndex == 11) return "ASTEROID BELT REGION";
    if (bodyIndex == 12) return "KUIPER BELT REGION";
    return "SOLAR SYSTEM OVERVIEW";
}

float SolarSystemVisuals::getBodyVisualRadius(int bodyIndex) const {
    static const float radii[] = {
        0.038f, // Sun (clear space before Mercury at 0.095)
        0.009f, // Mercury
        0.013f, // Venus
        0.014f, // Earth
        0.011f, // Mars
        0.028f, // Jupiter
        0.024f, // Saturn
        0.018f, // Uranus
        0.017f, // Neptune
        0.008f, // Pluto
        0.007f  // Comet
    };
    if (bodyIndex >= 0 && bodyIndex <= 10) return radii[bodyIndex];
    return 0.01f;
}

void SolarSystemVisuals::renderStarfield(const glm::mat4 &view, const glm::mat4 &proj, float timeSec) {
    if (!showStarfield) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_POINT_SMOOTH);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glUseProgram(starfieldShader);
    glUniformMatrix4fv(glGetUniformLocation(starfieldShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(starfieldShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1f(glGetUniformLocation(starfieldShader, "time"), timeSec);

    glBindVertexArray(starfield.vao);
    glDrawArrays(GL_POINTS, 0, starfield.starCount);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void SolarSystemVisuals::renderReferenceOrbits(const glm::mat4 &view, const glm::mat4 &proj, float maxPos) {
    if (!showOrbits) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.4f);

    glUseProgram(simpleLineShader);
    glUniformMatrix4fv(glGetUniformLocation(simpleLineShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleLineShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));

    // Draw orbits with subtle dashed-style cyan tint
    for (size_t i = 0; i < orbitMeshes.size(); ++i) {
        glm::vec4 col(0.20f, 0.55f, 0.85f, 0.35f);
        if (i == 4) {
            col = glm::vec4(0.70f, 0.65f, 0.45f, 0.22f); // Asteroid belt guide
        }
        glVertexAttrib4fv(1, glm::value_ptr(col));

        glBindVertexArray(orbitMeshes[i].vao);
        glDrawArrays(GL_LINE_STRIP, 0, orbitMeshes[i].pointCount);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void SolarSystemVisuals::renderMotionTrails(const glm::mat4 &view, const glm::mat4 &proj, const float *majorPositions, float maxPos) {
    if (!showTrails || !majorPositions) return;

    // Update trail history
    for (int i = 0; i <= 10; ++i) {
        glm::vec3 pos = getBodyNormalizedPosition(i, majorPositions, maxPos);
        trailHistory[i].push_back(pos);
        if (trailHistory[i].size() > MAX_TRAIL_LENGTH) {
            trailHistory[i].erase(trailHistory[i].begin());
        }
    }

    static const glm::vec3 bodyColors[] = {
        glm::vec3(1.0f, 0.85f, 0.30f), // Sun
        glm::vec3(0.65f, 0.62f, 0.60f), // Mercury
        glm::vec3(0.92f, 0.82f, 0.55f), // Venus
        glm::vec3(0.25f, 0.65f, 1.00f), // Earth
        glm::vec3(0.95f, 0.38f, 0.20f), // Mars
        glm::vec3(0.85f, 0.65f, 0.42f), // Jupiter
        glm::vec3(0.88f, 0.78f, 0.52f), // Saturn
        glm::vec3(0.55f, 0.88f, 0.90f), // Uranus
        glm::vec3(0.25f, 0.52f, 0.95f), // Neptune
        glm::vec3(0.65f, 0.55f, 0.48f), // Pluto
        glm::vec3(0.40f, 0.95f, 0.85f)  // Comet
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.8f);

    glUseProgram(simpleLineShader);
    glUniformMatrix4fv(glGetUniformLocation(simpleLineShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleLineShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(trailVao);
    glBindBuffer(GL_ARRAY_BUFFER, trailVbo);

    for (int i = 1; i <= 10; ++i) { // Skip Sun trail
        const auto &pts = trailHistory[i];
        if (pts.size() < 2) continue;

        std::vector<float> data;
        data.reserve(pts.size() * 7);

        for (size_t j = 0; j < pts.size(); ++j) {
            float alpha = (float)j / (float)(pts.size() - 1);
            alpha = std::pow(alpha, 1.8f) * 0.75f; // Exponential fade
            data.push_back(pts[j].x);
            data.push_back(pts[j].y);
            data.push_back(pts[j].z);
            data.push_back(bodyColors[i].r);
            data.push_back(bodyColors[i].g);
            data.push_back(bodyColors[i].b);
            data.push_back(alpha);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
        glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pts.size());
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void SolarSystemVisuals::renderSaturnRings(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &saturnPos, float saturnRadius) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glUseProgram(ringShader);
    glUniformMatrix4fv(glGetUniformLocation(ringShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(ringShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(glGetUniformLocation(ringShader, "lightPos"), 0.0f, 0.0f, 0.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, saturnPos);
    // Tilt Saturn's rings by ~27 degrees
    model = glm::rotate(model, glm::radians(27.0f), glm::vec3(1.0f, 0.0f, 0.5f));
    model = glm::scale(model, glm::vec3(saturnRadius));
    glUniformMatrix4fv(glGetUniformLocation(ringShader, "M_model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(saturnRing.vao);
    glDrawElements(GL_TRIANGLES, saturnRing.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void SolarSystemVisuals::renderCometTail(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &cometPos, const glm::vec3 &sunPos) {
    glm::vec3 dir = cometPos - sunPos;
    float dist = glm::length(dir);
    if (dist < 0.0001f) return;
    dir = glm::normalize(dir);

    // Tail length is inversely proportional to distance to sun (longer when closer to Sun!)
    float tailLen = 0.08f / std::max(dist, 0.15f);
    tailLen = std::min(tailLen, 0.35f);

    // Build triangular ribbon / cone of tail particles
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (glm::length(right) < 0.01f) right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::normalize(glm::cross(dir, right));

    std::vector<float> verts;
    int segments = 16;
    float width = 0.016f;

    for (int i = 0; i < segments; ++i) {
        float f1 = (float)i / (float)segments;
        float f2 = (float)(i + 1) / (float)segments;

        float d1 = f1 * tailLen;
        float d2 = f2 * tailLen;

        float w1 = (0.002f + f1 * width);
        float w2 = (0.002f + f2 * width);

        float a1 = (1.0f - f1) * 0.75f;
        float a2 = (1.0f - f2) * 0.75f;

        glm::vec3 c1 = cometPos + dir * d1;
        glm::vec3 c2 = cometPos + dir * d2;

        // Quad in 2 triangles
        glm::vec3 p1 = c1 - right * w1;
        glm::vec3 p2 = c1 + right * w1;
        glm::vec3 p3 = c2 - right * w2;
        glm::vec3 p4 = c2 + right * w2;

        glm::vec4 col1(0.95f, 0.85f, 0.65f, a1);
        glm::vec4 col2(0.95f, 0.85f, 0.65f, a2);

        auto addV = [&](const glm::vec3 &p, const glm::vec4 &c) {
            verts.push_back(p.x); verts.push_back(p.y); verts.push_back(p.z);
            verts.push_back(c.r); verts.push_back(c.g); verts.push_back(c.b); verts.push_back(c.a);
        };

        addV(p1, col1); addV(p2, col1); addV(p3, col2);
        addV(p2, col1); addV(p4, col2); addV(p3, col2);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous glow

    glUseProgram(cometShader);
    glUniformMatrix4fv(glGetUniformLocation(cometShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(cometShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 7));

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_BLEND);
}

void SolarSystemVisuals::renderCelestialBodies(const glm::mat4 &view, const glm::mat4 &proj,
                                              const glm::vec3 &camPos, const float *majorPositions,
                                              float maxPos, float timeSec) {
    if (!majorPositions) return;

    // 1. Render the Sun
    glm::vec3 sunPos = getBodyNormalizedPosition(0, majorPositions, maxPos);
    float sunRad = getBodyVisualRadius(0);

    glUseProgram(sunShader);
    glUniformMatrix4fv(glGetUniformLocation(sunShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(sunShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(sunShader, "viewPos"), 1, glm::value_ptr(camPos));
    glUniform1f(glGetUniformLocation(sunShader, "time"), timeSec);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, sunPos);
    model = glm::scale(model, glm::vec3(sunRad));
    glUniformMatrix4fv(glGetUniformLocation(sunShader, "M_model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(sphere.vao);
    glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

    // 2. Render Planets (1: Mercury to 10: Comet)
    glUseProgram(planetShader);
    glUniformMatrix4fv(glGetUniformLocation(planetShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(planetShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(planetShader, "lightPos"), 1, glm::value_ptr(sunPos));
    glUniform3fv(glGetUniformLocation(planetShader, "viewPos"), 1, glm::value_ptr(camPos));
    glUniform1f(glGetUniformLocation(planetShader, "time"), timeSec);

    for (int i = 1; i <= 10; ++i) {
        glm::vec3 pos = getBodyNormalizedPosition(i, majorPositions, maxPos);
        float rad = getBodyVisualRadius(i);

        model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(rad));

        glUniformMatrix4fv(glGetUniformLocation(planetShader, "M_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(planetShader, "planetId"), i);

        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

        // Saturn's Rings
        if (i == 6) {
            renderSaturnRings(view, proj, pos, rad);
            // Re-bind planet shader & sphere VAO
            glUseProgram(planetShader);
            glBindVertexArray(sphere.vao);
        }

        // Comet Tail
        if (i == 10) {
            renderCometTail(view, proj, pos, sunPos);
            // Re-bind planet shader & sphere VAO
            glUseProgram(planetShader);
            glBindVertexArray(sphere.vao);
        }
    }

    glBindVertexArray(0);
}

void SolarSystemVisuals::renderNBodyParticles(const glm::mat4 &view, const glm::mat4 &proj,
                                             const glm::vec3 &camPos, GLuint vbo, GLuint mbo,
                                             size_t totalCount, float maxPos, float timeSec) {
    if (!showParticles || totalCount <= 11) return;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Allow particles to glow without blocking each other
    glDisable(GL_POINT_SMOOTH);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous glow for celestial points

    glUseProgram(particleShader);
    glUniformMatrix4fv(glGetUniformLocation(particleShader, "M_view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(particleShader, "M_projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(particleShader, "camPos"), 1, glm::value_ptr(camPos));
    glUniform1f(glGetUniformLocation(particleShader, "maxPos"), maxPos);
    glUniform1f(glGetUniformLocation(particleShader, "time"), timeSec);

    glBindVertexArray(particleVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, mbo);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // Render bodies 11 to totalCount-1 (Asteroid Belt, Kuiper Belt, Simulated Stars)
    glDrawArrays(GL_POINTS, 11, (GLsizei)(totalCount - 11));

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SolarSystemVisuals::renderLabels(const glm::mat4 &view, const glm::mat4 &proj,
                                     const float *majorPositions, float maxPos,
                                     int screenW, int screenH) {
    if (!showLabels || !majorPositions) return;

    static const char* labels[] = {
        "SUN", "MERCURY", "VENUS", "EARTH", "MARS",
        "JUPITER", "SATURN", "URANUS", "NEPTUNE", "PLUTO", "1P/HALLEY (COMET)"
    };

    static const glm::vec3 colors[] = {
        glm::vec3(1.0f, 0.90f, 0.40f), // Sun
        glm::vec3(0.85f, 0.85f, 0.85f), // Mercury
        glm::vec3(1.0f, 0.92f, 0.65f), // Venus
        glm::vec3(0.40f, 0.80f, 1.0f),  // Earth
        glm::vec3(1.0f, 0.50f, 0.35f),  // Mars
        glm::vec3(0.95f, 0.80f, 0.60f), // Jupiter
        glm::vec3(1.0f, 0.88f, 0.65f),  // Saturn
        glm::vec3(0.65f, 0.95f, 1.0f),  // Uranus
        glm::vec3(0.45f, 0.70f, 1.0f),  // Neptune
        glm::vec3(0.80f, 0.75f, 0.70f), // Pluto
        glm::vec3(0.60f, 1.0f, 0.90f)   // Comet
    };

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 vp = proj * view;

    for (int i = 0; i <= 10; ++i) {
        glm::vec3 p = getBodyNormalizedPosition(i, majorPositions, maxPos);
        float rad = getBodyVisualRadius(i);
        p.y += rad * 1.55f; // Position text slightly above planet

        glm::vec4 clip = vp * glm::vec4(p, 1.0f);
        if (clip.w <= 0.001f) continue;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1.1f || ndc.x > 1.1f || ndc.y < -1.1f || ndc.y > 1.1f) continue;

        float winX = (ndc.x * 0.5f + 0.5f) * screenW;
        float winY = (ndc.y * 0.5f + 0.5f) * screenH;

        glColor4f(colors[i].r, colors[i].g, colors[i].b, 0.92f);
        glRasterPos2f(winX - 18.0f, winY);

        std::string text = labels[i];
        for (char c : text) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }
    }

    // Belt labels
    glm::vec4 astClip = vp * glm::vec4(0.41f, 0.015f, 0.0f, 1.0f);
    if (astClip.w > 0.001f) {
        glm::vec3 ndc = glm::vec3(astClip) / astClip.w;
        float winX = (ndc.x * 0.5f + 0.5f) * screenW;
        float winY = (ndc.y * 0.5f + 0.5f) * screenH;
        glColor4f(0.95f, 0.85f, 0.60f, 0.85f);
        glRasterPos2f(winX - 45.0f, winY);
        std::string txt = "★ ASTEROID BELT (N-BODY)";
        for (char c : txt) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }

    glm::vec4 kuiperClip = vp * glm::vec4(1.20f, 0.015f, 0.0f, 1.0f);
    if (kuiperClip.w > 0.001f) {
        glm::vec3 ndc = glm::vec3(kuiperClip) / kuiperClip.w;
        float winX = (ndc.x * 0.5f + 0.5f) * screenW;
        float winY = (ndc.y * 0.5f + 0.5f) * screenH;
        glColor4f(0.75f, 0.85f, 1.0f, 0.85f);
        glRasterPos2f(winX - 45.0f, winY);
        std::string txt = "★ KUIPER BELT (N-BODY)";
        for (char c : txt) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void SolarSystemVisuals::renderHUD(int screenW, int screenH, bool isPaused, float simSpeed,
                                  int totalBodies, int submittedBodies, int renderedBodies,
                                  int asteroidCount, int kuiperCount, int starCount,
                                  const std::string &backend, int focusIndex,
                                  float fps, float calcTimeMs) {
    if (!showHUD) return;

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Top Header Banner (semi-transparent dark gradient with 3 lines of diagnostic info)
    glColor4f(0.03f, 0.05f, 0.10f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, screenH - 72.0f);
    glVertex2f(screenW, screenH - 72.0f);
    glVertex2f(screenW, screenH);
    glVertex2f(0.0f, screenH);
    glEnd();

    // Top accent border line (cyan)
    glColor4f(0.20f, 0.60f, 0.95f, 0.65f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, screenH - 72.0f);
    glVertex2f(screenW, screenH - 72.0f);
    glEnd();

    // Bottom Controls Banner
    glColor4f(0.03f, 0.05f, 0.10f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(screenW, 0.0f);
    glVertex2f(screenW, 36.0f);
    glVertex2f(0.0f, 36.0f);
    glEnd();

    // Bottom accent border line
    glColor4f(0.20f, 0.60f, 0.95f, 0.50f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 36.0f);
    glVertex2f(screenW, 36.0f);
    glEnd();

    // Top Title & Info
    glColor4f(0.35f, 0.85f, 1.0f, 1.0f);
    glRasterPos2f(16.0f, screenH - 18.0f);
    std::string title = "SOLAR SYSTEM 3D SIMULATION - REAL-TIME OPENCL N-BODY GRAVITY";
    for (char c : title) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    // Line 2: Comprehensive Simulation and Pipeline Metrics
    std::ostringstream ssTop;
    ssTop << "Status: " << (isPaused ? "[PAUSED]" : "[RUNNING]")
          << " | Step: " << std::fixed << std::setprecision(1) << simSpeed << "x (1 day/step)"
          << " | Total Bodies: " << totalBodies
          << " | Submitted: " << submittedBodies
          << " | Rendered: " << renderedBodies;
    if (asteroidCount > 0 || kuiperCount > 0) {
        ssTop << " (Major: 11, Asteroids: " << asteroidCount
              << ", Kuiper: " << kuiperCount << ", Stars: " << starCount << ")";
    }
    ssTop << " | Engine: " << backend;
    std::string infoTop = ssTop.str();

    glColor4f(0.85f, 0.90f, 0.98f, 0.95f);
    glRasterPos2f(16.0f, screenH - 36.0f);
    for (char c : infoTop) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    // Line 3: Visual Classification Legend
    std::string legendStr = "Legend: [Sun/Planets: 3D Meshes] [Asteroids: Warm Amber Points] [Kuiper Belt: Crystalline Cyan] [Simulated Stars: Diamond White] [Deep Sky: Starfield (" +
                            std::to_string(starfield.starCount) + " Stars)]";
    glColor4f(1.0f, 0.82f, 0.45f, 0.90f);
    glRasterPos2f(16.0f, screenH - 54.0f);
    for (char c : legendStr) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    // Right-aligned performance & focus stats
    std::ostringstream ssPerf;
    ssPerf << "FPS: " << std::fixed << std::setprecision(1) << fps
           << " | Calc: " << std::setprecision(2) << calcTimeMs << " ms";
    std::string perfStr = ssPerf.str();

    glColor4f(0.35f, 1.0f, 0.65f, 1.0f);
    glRasterPos2f(screenW - 220.0f, screenH - 18.0f);
    for (char c : perfStr) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    std::string targetStr = "Focus: [" + getBodyName(focusIndex) + "]";
    glColor4f(1.0f, 0.85f, 0.35f, 1.0f);
    glRasterPos2f(screenW - 220.0f, screenH - 36.0f);
    for (char c : targetStr) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    std::string pipelineStr = "Active Pipeline: " + std::to_string(renderedBodies) + " / " + std::to_string(totalBodies) + " (100% Rendered)";
    glColor4f(0.50f, 0.90f, 1.0f, 0.95f);
    glRasterPos2f(screenW - 220.0f, screenH - 54.0f);
    for (char c : pipelineStr) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    // Bottom controls guide
    glColor4f(0.80f, 0.88f, 0.96f, 0.90f);
    glRasterPos2f(16.0f, 13.0f);
    std::string guide = "[0-9] Target Planet  |  [A] Asteroid Belt  |  [K] Kuiper Belt  |  [Space] Pause  |  [+/-] Speed  |  [P] Particles  |  [T] Trails  |  [O] Orbits  |  [L] Labels  |  [B] Stars  |  [R] Auto-Rotate  |  [F] Fullscreen  |  [Esc] Exit";
    for (char c : guide) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}
