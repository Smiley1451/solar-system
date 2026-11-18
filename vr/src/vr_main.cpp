/**
 * @file vr_main.cpp
 * @brief Standalone Stereoscopic 3D & Mobile VR N-Body Solar System Engine
 * Reuses SolarSystemDataSet (1,611 bodies) with Side-by-Side (SBS) dual-eye rendering
 * and UDP mobile phone gyroscope head tracking.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <string>
#include <algorithm>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "../../include/glm/glm.hpp"
#include "../../include/glm/gtc/matrix_transform.hpp"
#include "../../include/glm/gtc/type_ptr.hpp"

#include "../../include/Data/SolarSystemDataSet.hpp"
#include "vr_tracker.hpp"

// Globals
int windowWidth = 1600;
int windowHeight = 900;
bool isVRMode = true; // Default to Side-by-Side VR
bool isPaused = false;
float simSpeed = 1.0f;
float ipd = 0.045f; // Interpupillary distance in world units

// Camera & Navigation
int focusIndex = 0; // 0 = Sun
glm::vec3 currentCenter(0.0f);
glm::vec3 targetCenter(0.0f);
float camDistance = 3.2f;
float targetDistance = 3.2f;
float camYaw = 0.8f;
float camPitch = 0.35f;

// Mouse Drag
bool isDragging = false;
int lastMouseX = 0, lastMouseY = 0;

// Simulation dataset & particles
SolarSystemDataSet *solarData = nullptr;
std::vector<float> positions;
std::vector<float> velocities;
std::vector<float> masses;
size_t numBodies = 0;
double pMax = 5.0e11;

// Background Starfield
struct Star {
    glm::vec3 pos;
    glm::vec3 col;
    float size;
};
std::vector<Star> stars;

// UDP Mobile Tracker
VRTracker tracker(4242);

// Time
auto lastFrameTime = std::chrono::high_resolution_clock::now();
float currentFps = 60.0f;
int frameCounter = 0;
auto lastFpsUpdate = std::chrono::high_resolution_clock::now();

void initStarfield() {
    stars.resize(2000);
    for (auto &s : stars) {
        float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
        float phi = std::acos(2.0f * ((float)rand() / RAND_MAX) - 1.0f);
        float r = 50.0f + ((float)rand() / RAND_MAX) * 40.0f;

        s.pos = glm::vec3(r * std::sin(phi) * std::cos(theta),
                          r * std::sin(phi) * std::sin(theta),
                          r * std::cos(phi));
        float br = 0.4f + ((float)rand() / RAND_MAX) * 0.6f;
        s.col = glm::vec3(br * 0.9f, br * 0.95f, br * 1.0f);
        s.size = 1.0f + ((float)rand() / RAND_MAX) * 1.5f;
    }
}

void initSimulation() {
    std::cout << "[VR Engine] Initializing Realistic Solar System N-Body dataset (1,600 bodies)...\n";
    solarData = new SolarSystemDataSet(1600);
    numBodies = solarData->getSize();
    pMax = solarData->getMaxPosition();

    positions = solarData->getFlatPositions();
    velocities = solarData->getFlatVelocities();
    masses = solarData->getMasses();

    std::cout << "[VR Engine] Successfully loaded " << numBodies << " gravitationally interacting bodies.\n";
    std::cout << "  - Major Bodies:  " << solarData->getMajorBodyCount() << "\n";
    std::cout << "  - Asteroid Belt: " << solarData->getAsteroidCount() << "\n";
    std::cout << "  - Kuiper Belt:   " << solarData->getKuiperCount() << "\n";
    std::cout << "  - Oort Cloud:    " << solarData->getSimulatedStarCount() << "\n";
}

void updatePhysics(float dt) {
    if (isPaused) return;

    const double G = 6.67430e-11;
    const double timestep = 14400.0 * simSpeed; // 4 hours per step
    const double softening = 1e9 * 1e9;

    // Fast O(N) central gravity + major body gravitational updates
    const size_t majorCount = std::min<size_t>(11, numBodies);

    // Update positions and velocities
    for (size_t i = 0; i < numBodies; ++i) {
        if (i == 0) continue; // Sun at center

        double ax = 0.0, ay = 0.0, az = 0.0;
        double px = positions[i * 3];
        double py = positions[i * 3 + 1];
        double pz = positions[i * 3 + 2];

        // Gravitational force from Sun and major planets
        for (size_t j = 0; j < majorCount; ++j) {
            if (i == j) continue;
            double dx = positions[j * 3] - px;
            double dy = positions[j * 3 + 1] - py;
            double dz = positions[j * 3 + 2] - pz;
            double distSqr = dx * dx + dy * dy + dz * dz + softening;
            double invDist = 1.0 / std::sqrt(distSqr);
            double invDist3 = invDist * invDist * invDist;
            double f = G * masses[j] * invDist3;

            ax += f * dx;
            ay += f * dy;
            az += f * dz;
        }

        velocities[i * 3] += (float)(ax * timestep);
        velocities[i * 3 + 1] += (float)(ay * timestep);
        velocities[i * 3 + 2] += (float)(az * timestep);

        positions[i * 3] += (float)(velocities[i * 3] * timestep);
        positions[i * 3 + 1] += (float)(velocities[i * 3 + 1] * timestep);
        positions[i * 3 + 2] += (float)(velocities[i * 3 + 2] * timestep);
    }
}

void renderSceneEye(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos) {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    // 1. Deep Space Starfield
    glPointSize(1.2f);
    glBegin(GL_POINTS);
    for (const auto &s : stars) {
        glColor3fv(glm::value_ptr(s.col));
        glVertex3fv(glm::value_ptr(s.pos));
    }
    glEnd();

    // 2. Reference Orbits (Lines)
    glColor4f(0.22f, 0.74f, 0.97f, 0.28f);
    glLineWidth(1.0f);
    const auto &metadata = solarData->getMetadata();
    for (size_t b = 1; b < std::min<size_t>(10, metadata.size()); ++b) {
        float r = (float)(metadata[b].semiMajorAxis / pMax);
        glBegin(GL_LINE_LOOP);
        for (int a = 0; a < 64; ++a) {
            float ang = (float)a / 64.0f * 6.2831853f;
            glVertex3f(r * std::cos(ang), 0.0f, r * std::sin(ang));
        }
        glEnd();
    }

    // 3. Sun (Glowing Gold/Orange)
    glPushMatrix();
    glColor3f(1.0f, 0.82f, 0.20f);
    glutSolidSphere(0.055f, 24, 24);
    // Corona
    glColor4f(1.0f, 0.55f, 0.0f, 0.35f);
    glutSolidSphere(0.075f, 16, 16);
    glPopMatrix();

    // 4. Major Planets
    const glm::vec3 planetColors[] = {
        glm::vec3(1.0f, 0.85f, 0.2f),  // Sun
        glm::vec3(0.68f, 0.65f, 0.63f), // Mercury
        glm::vec3(0.95f, 0.78f, 0.30f), // Venus
        glm::vec3(0.22f, 0.74f, 0.97f), // Earth
        glm::vec3(0.94f, 0.30f, 0.25f), // Mars
        glm::vec3(0.85f, 0.55f, 0.20f), // Jupiter
        glm::vec3(0.96f, 0.85f, 0.45f), // Saturn
        glm::vec3(0.30f, 0.85f, 0.90f), // Uranus
        glm::vec3(0.25f, 0.55f, 0.95f), // Neptune
        glm::vec3(0.65f, 0.65f, 0.70f)  // Pluto
    };

    for (size_t i = 1; i < std::min<size_t>(10, numBodies); ++i) {
        float x = (float)(positions[i * 3] / pMax);
        float y = (float)(positions[i * 3 + 1] / pMax);
        float z = (float)(positions[i * 3 + 2] / pMax);

        glPushMatrix();
        glTranslatef(x, y, z);
        glColor3fv(glm::value_ptr(planetColors[i % 10]));
        float size = (i == 5 || i == 6) ? 0.035f : 0.018f;
        glutSolidSphere(size, 16, 16);

        // Saturn's Ring
        if (i == 6) {
            glColor4f(0.88f, 0.82f, 0.70f, 0.65f);
            glBegin(GL_LINE_LOOP);
            for (int a = 0; a < 36; ++a) {
                float ang = (float)a / 36.0f * 6.2831853f;
                glVertex3f(0.07f * std::cos(ang), 0.0f, 0.07f * std::sin(ang));
            }
            glEnd();
        }
        glPopMatrix();
    }

    // 5. Asteroid Belt & Kuiper Belt N-Body Particles
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (size_t i = 11; i < numBodies; ++i) {
        float x = (float)(positions[i * 3] / pMax);
        float y = (float)(positions[i * 3 + 1] / pMax);
        float z = (float)(positions[i * 3 + 2] / pMax);

        if (i < 611) {
            // Asteroid (Warm grey/brown)
            glColor3f(0.85f, 0.75f, 0.65f);
        } else if (i < 1111) {
            // Kuiper Belt (Icy cyan/white)
            glColor3f(0.65f, 0.85f, 1.0f);
        } else {
            // Oort Cloud (Pale blue)
            glColor3f(0.55f, 0.70f, 0.90f);
        }
        glVertex3f(x, y, z);
    }
    glEnd();
}

void display() {
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;
    if (dt > 0.1f) dt = 0.1f;

    // Update FPS
    frameCounter++;
    if (std::chrono::duration<float>(now - lastFpsUpdate).count() >= 1.0f) {
        currentFps = (float)frameCounter;
        frameCounter = 0;
        lastFpsUpdate = now;
    }

    // Update simulation
    updatePhysics(dt);

    // Apply mobile gyro tracking if connected
    if (tracker.isConnected()) {
        camYaw = glm::radians(tracker.getYaw());
        camPitch = glm::radians(tracker.getPitch());
    }

    // Update Camera Target
    if (focusIndex >= 0 && focusIndex < (int)numBodies) {
        targetCenter = glm::vec3((float)(positions[focusIndex * 3] / pMax),
                                 (float)(positions[focusIndex * 3 + 1] / pMax),
                                 (float)(positions[focusIndex * 3 + 2] / pMax));
    }
    currentCenter = glm::mix(currentCenter, targetCenter, 0.10f);
    camDistance = glm::mix(camDistance, targetDistance, 0.10f);

    float cx = camDistance * std::cos(camPitch) * std::sin(camYaw);
    float cy = camDistance * std::sin(camPitch);
    float cz = camDistance * std::cos(camPitch) * std::cos(camYaw);
    glm::vec3 camPos = currentCenter + glm::vec3(cx, cy, cz);

    glm::vec3 forward = glm::normalize(currentCenter - camPos);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    glClearColor(0.012f, 0.015f, 0.028f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (isVRMode) {
        // SIDE-BY-SIDE (SBS) STEREOSCOPIC 3D
        int halfW = windowWidth / 2;

        // 1. Left Eye
        glViewport(0, 0, halfW, windowHeight);
        glm::vec3 leftEyePos = camPos - right * (ipd * 0.5f);
        glm::mat4 viewLeft = glm::lookAt(leftEyePos, currentCenter, up);
        glm::mat4 projLeft = glm::perspective(glm::radians(60.0f), (float)halfW / (float)windowHeight, 0.01f, 150.0f);
        renderSceneEye(viewLeft, projLeft, leftEyePos);

        // 2. Right Eye
        glViewport(halfW, 0, halfW, windowHeight);
        glm::vec3 rightEyePos = camPos + right * (ipd * 0.5f);
        glm::mat4 viewRight = glm::lookAt(rightEyePos, currentCenter, up);
        glm::mat4 projRight = glm::perspective(glm::radians(60.0f), (float)halfW / (float)windowHeight, 0.01f, 150.0f);
        renderSceneEye(viewRight, projRight, rightEyePos);

        // Center Divider Line
        glViewport(0, 0, windowWidth, windowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, windowWidth, 0, windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor4f(0.2f, 0.2f, 0.3f, 0.5f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2i(halfW, 0);
        glVertex2i(halfW, windowHeight);
        glEnd();

    } else {
        // Standard Monoscopic Full-Screen Viewport
        glViewport(0, 0, windowWidth, windowHeight);
        glm::mat4 view = glm::lookAt(camPos, currentCenter, up);
        glm::mat4 proj = glm::perspective(glm::radians(55.0f), (float)windowWidth / (float)windowHeight, 0.01f, 150.0f);
        renderSceneEye(view, proj, camPos);
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = (h > 0) ? h : 1;
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case 'v':
        case 'V':
            isVRMode = !isVRMode;
            std::cout << "[VR Mode] " << (isVRMode ? "Side-by-Side (SBS) Stereoscopic 3D Enabled" : "Monoscopic Mode") << "\n";
            break;
        case ' ':
            isPaused = !isPaused;
            std::cout << "[Simulation] " << (isPaused ? "PAUSED" : "RUNNING") << "\n";
            break;
        case '+':
        case '=':
            simSpeed = std::min(simSpeed * 2.0f, 8.0f);
            std::cout << "[Speed] " << simSpeed << "x\n";
            break;
        case '-':
        case '_':
            simSpeed = std::max(simSpeed / 2.0f, 0.125f);
            std::cout << "[Speed] " << simSpeed << "x\n";
            break;
        case '[':
            ipd = std::max(0.01f, ipd - 0.005f);
            std::cout << "[Stereo IPD] " << ipd << "\n";
            break;
        case ']':
            ipd = std::min(0.12f, ipd + 0.005f);
            std::cout << "[Stereo IPD] " << ipd << "\n";
            break;
        case '0': focusIndex = 0; targetDistance = 3.2f; std::cout << "[Focus] Sun\n"; break;
        case '1': focusIndex = 1; targetDistance = 0.5f; std::cout << "[Focus] Mercury\n"; break;
        case '2': focusIndex = 2; targetDistance = 0.6f; std::cout << "[Focus] Venus\n"; break;
        case '3': focusIndex = 3; targetDistance = 0.6f; std::cout << "[Focus] Earth\n"; break;
        case '4': focusIndex = 4; targetDistance = 0.5f; std::cout << "[Focus] Mars\n"; break;
        case '5': focusIndex = 5; targetDistance = 1.2f; std::cout << "[Focus] Jupiter\n"; break;
        case '6': focusIndex = 6; targetDistance = 1.4f; std::cout << "[Focus] Saturn\n"; break;
        case '7': focusIndex = 7; targetDistance = 1.0f; std::cout << "[Focus] Uranus\n"; break;
        case '8': focusIndex = 8; targetDistance = 1.0f; std::cout << "[Focus] Neptune\n"; break;
        case '9': focusIndex = 9; targetDistance = 0.4f; std::cout << "[Focus] Pluto\n"; break;
        case 'r':
        case 'R':
            focusIndex = 0;
            targetCenter = glm::vec3(0.0f);
            targetDistance = 3.2f;
            camYaw = 0.8f;
            camPitch = 0.35f;
            std::cout << "[Camera] Reset\n";
            break;
    }
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isDragging = true;
            lastMouseX = x;
            lastMouseY = y;
        } else {
            isDragging = false;
        }
    } else if (button == 3) { // Scroll Up
        targetDistance = std::max(0.2f, targetDistance * 0.9f);
    } else if (button == 4) { // Scroll Down
        targetDistance = std::min(25.0f, targetDistance * 1.1f);
    }
}

void motion(int x, int y) {
    if (!isDragging) return;
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;
    lastMouseX = x;
    lastMouseY = y;

    camYaw -= dx * 0.005f;
    camPitch = std::max(-1.5f, std::min(1.5f, camPitch + dy * 0.005f));
}

int main(int argc, char *argv[]) {
    std::cout << "========================================================================\n";
    std::cout << "       N-BODY SOLAR SYSTEM — STEREOSCOPIC 3D & MOBILE VR ENGINE\n";
    std::cout << "========================================================================\n";
    std::cout << " Controls:\n";
    std::cout << "   [V]      Toggle Side-by-Side (SBS) Stereoscopic 3D / Monoscopic\n";
    std::cout << "   [Space]  Pause / Resume Physics\n";
    std::cout << "   [+] / [-] Speed Multiplier (0.125x - 8.0x)\n";
    std::cout << "   [[] / []] Adjust Eye Separation (IPD) for VR\n";
    std::cout << "   [0 - 9]  Focus Camera on Sun / Planet\n";
    std::cout << "   [R]      Reset Camera View\n";
    std::cout << "   [ESC]    Exit\n";
    std::cout << "------------------------------------------------------------------------\n";

    // Start UDP Mobile Gyro Tracker
    tracker.start();

    // Init GLUT & OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Solar System N-Body Simulation — Side-by-Side VR Mode");

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initStarfield();
    initSimulation();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    std::cout << "[VR Engine] Window initialized in Side-by-Side 3D mode. Enjoy the simulation!\n";
    glutMainLoop();

    tracker.stop();
    delete solarData;
    return 0;
}
