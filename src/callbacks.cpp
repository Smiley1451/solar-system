/**
 * @file callbacks.cpp
 * @brief Interactive keyboard and mouse callbacks for 3D universe exploration
 */

#include "../include/callbacks.hpp"
#include "../include/Render/SolarSystemVisuals.hpp"
#include "../include/glm/gtc/constants.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/mat4x4.hpp"
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

extern int mainWindow;

extern glm::mat4 M_model;
extern glm::mat4 M_view;
extern glm::mat4 M_projection;

extern glm::vec3 eyeVector;
extern glm::vec3 centerVector;
extern glm::vec3 upVector;

extern float zoomFactor;
extern bool copyMatricesToGPU;
extern bool automaticCameraRotation;
bool isFullScreen = false;

// Interactive controls state
int focusIndex = -1; // -1: Overview, 0: Sun, 1..9: Planets, 10: Comet
bool isPaused = false;
int simStepsPerFrame = 1;
float simSpeedMultiplier = 1.0f;

float camYaw = 0.0f;
float camPitch = 0.55f; // ~31 degrees elevation
float camDistance = 1.8f;
float targetDistance = 1.8f;
glm::vec3 currentCenter(0.0f, 0.0f, 0.0f);
glm::vec3 targetCenter(0.0f, 0.0f, 0.0f);

int lastMouseX = -1;
int lastMouseY = -1;

void glKeyboardCallback(unsigned char key, int x, int y) {
    auto &visuals = SolarSystemVisuals::getInstance();

    switch (key) {
        case 27: // Escape
            glutDestroyWindow(mainWindow);
            glutLeaveMainLoop();
            break;

        case ' ': // Toggle Pause
            isPaused = !isPaused;
            break;

        case '+':
        case '=': // Faster simulation
            simStepsPerFrame = std::min(simStepsPerFrame * 2, 16);
            simSpeedMultiplier = (float)simStepsPerFrame;
            break;

        case '-':
        case '_': // Slower simulation
            simStepsPerFrame = std::max(simStepsPerFrame / 2, 1);
            simSpeedMultiplier = (float)simStepsPerFrame;
            break;

        case 'r':
        case 'R': // Toggle Auto Rotation
            automaticCameraRotation = !automaticCameraRotation;
            break;

        case 'f':
        case 'F': // Fullscreen
            if (!isFullScreen) {
                glutFullScreen();
                isFullScreen = true;
            } else {
                glutLeaveFullScreen();
                isFullScreen = false;
            }
            break;

        // Visual element toggles
        case 't':
        case 'T':
            visuals.showTrails = !visuals.showTrails;
            break;

        case 'o':
        case 'O':
            visuals.showOrbits = !visuals.showOrbits;
            break;

        case 'l':
        case 'L':
            visuals.showLabels = !visuals.showLabels;
            break;

        case 'b':
        case 'B':
            visuals.showStarfield = !visuals.showStarfield;
            break;

        case 'h':
        case 'H':
            visuals.showHUD = !visuals.showHUD;
            break;

        // Target body focus
        case '0':
            focusIndex = 0; // Sun
            targetDistance = 0.22f;
            break;
        case '1':
            focusIndex = 1; // Mercury
            targetDistance = 0.045f;
            break;
        case '2':
            focusIndex = 2; // Venus
            targetDistance = 0.065f;
            break;
        case '3':
            focusIndex = 3; // Earth
            targetDistance = 0.070f;
            break;
        case '4':
            focusIndex = 4; // Mars
            targetDistance = 0.055f;
            break;
        case '5':
            focusIndex = 5; // Jupiter
            targetDistance = 0.150f;
            break;
        case '6':
            focusIndex = 6; // Saturn
            targetDistance = 0.140f;
            break;
        case '7':
            focusIndex = 7; // Uranus
            targetDistance = 0.090f;
            break;
        case '8':
            focusIndex = 8; // Neptune
            targetDistance = 0.090f;
            break;
        case '9':
            focusIndex = 9; // Pluto
            targetDistance = 0.040f;
            break;

        case 'c':
        case 'C':
            focusIndex = 10; // Comet 1P/Halley
            targetDistance = 0.035f;
            break;

        case 'z':
        case 'Z':
            focusIndex = -1; // Reset to overview
            targetDistance = 1.8f;
            targetCenter = glm::vec3(0.0f, 0.0f, 0.0f);
            break;

        default:
            break;
    }
}

void glMouseMovementClickCallback(int x, int y) {
    if (-1 == lastMouseX && -1 == lastMouseY) {
        lastMouseX = x;
        lastMouseY = y;
        return;
    }

    float dx = (float)(x - lastMouseX);
    float dy = (float)(y - lastMouseY);

    camYaw -= dx * 0.006f;
    camPitch += dy * 0.006f;

    // Prevent gimbal-flip when looking directly straight up or down
    const float PITCH_LIMIT = 1.48f; // ~85 degrees
    camPitch = std::clamp(camPitch, -PITCH_LIMIT, PITCH_LIMIT);

    lastMouseX = x;
    lastMouseY = y;
    copyMatricesToGPU = true;
}

void glMouseWheelCallback(int button, int state, int x, int y) {
#ifdef __unix__
    if (3 == button) {
        targetDistance *= 0.84f;
    } else if (4 == button) {
        targetDistance *= 1.18f;
    }
#elif _WIN32
    if (state > 0) {
        targetDistance *= 0.84f;
    } else {
        targetDistance *= 1.18f;
    }
#endif
    targetDistance = std::clamp(targetDistance, 0.008f, 10.0f);
    copyMatricesToGPU = true;
}