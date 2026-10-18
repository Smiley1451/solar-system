/**
 * @file callbacks.hpp
 * @brief Header File declaring callback functions and camera/simulation interactive states
 */

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <glm/glm.hpp>

void glKeyboardCallback(unsigned char key, int x, int y);
void glMouseMovementClickCallback(int x, int y);
void glMouseWheelCallback(int button, int state, int x, int y);

// Interactive control variables
extern int focusIndex;
extern bool isPaused;
extern int simStepsPerFrame;
extern float simSpeedMultiplier;

extern float camYaw;
extern float camPitch;
extern float camDistance;
extern float targetDistance;
extern glm::vec3 currentCenter;
extern glm::vec3 targetCenter;

#endif