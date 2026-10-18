/**
 * @file render.cpp
 * @brief Realistic 3D Solar System and Universe OpenGL rendering loop
 */

typedef unsigned int uint;

#include "../../include/Render/render.hpp"
// clang-format off
#include <GL/glew.h>
#include <GL/freeglut.h>
#ifdef __unix__
#include <GL/glx.h>
#elif _WIN32
#include <GL/GL.h>
#endif
// clang-format on
#include "../../include/glm/gtc/type_ptr.hpp"
#include "../../include/glm/mat4x4.hpp"
#include "./Simulation/CPUCalc.hpp"
#include "./Simulation/GPUCalc.hpp"

#include <iostream>
#include <algorithm>

// cl imports
#include <../../lib/OpenCL/Device.hpp>
#include <../../lib/OpenCL/Event.hpp>
#include <../../lib/OpenCL/Program.hpp>
#include <../../lib/OpenCL/cl-patched.hpp>

#include "../../include/Data/SolarSystemDataSet.hpp"
#include "../../include/Data/WikipediaDataSet.hpp"
#include "../../include/PerformanceMetrics/PerformanceMetric.hpp"
#include "../../include/PerformanceMetrics/PerformanceMetricsCollector.hpp"
#include "../../include/Render/SolarSystemVisuals.hpp"
#include "../../include/Simulation/CompareResults.hpp"
#include "../../include/callbacks.hpp"
#include "../../include/constants.hpp"
#include "../../include/glm/gtc/matrix_transform.hpp"
#include "../../lib/Core/Time.hpp"

bool initialRun = true;

extern bool useGPU;
extern bool useCPU;

GLuint vbo;
GLuint mbo;
GLuint lbo;
GLuint lco;
extern GLuint vao;
extern GLuint shaderProgram;
extern int mainWindow;
extern AbstractData *dataSet;
extern std::size_t numRenderElements;

extern glm::vec3 eyeVector;
extern glm::vec3 centerVector;
extern glm::vec3 upVector;

extern int M_model_loc;
extern int M_view_loc;
extern int M_projection_loc;
extern int modeLoc;
extern int zoomFactorLoc;
int isSolarSystemLoc = 0;

extern glm::mat4 M_model;
extern glm::mat4 M_view;
extern glm::mat4 M_projection;

extern float zoomFactor;
extern bool copyMatricesToGPU;
extern bool automaticCameraRotation;

extern const std::vector<float> coordinateSystemLines;
extern const std::vector<float> lineColors;

// benchmark
double executionTime = 0.0;
size_t nFrames = 0;
extern size_t benchmarkLength;
extern size_t benchmarkWarmUp;
extern BenchmarkMode benchmark;
extern PerformanceMetricsCollector *performanceMetricsCollector;

void calcSimulationStep(bool cpu, bool gpu) {
    if (gpu) {
        executionTime = simulateGPU();
    }
    if (cpu) {
        executionTime = simulateCPU();
    }
    if (gpu && cpu) {
        compareResults();
    }

    performanceMetricsCollector->addCalcTime(executionTime);
    nFrames++;
    if (benchmark == BenchmarkMode::OFF) {
        performanceMetricsCollector->printResult();
    } else if (nFrames == benchmarkLength) {
        if (performanceMetricsCollector->getCalcTimes().getAvgTime() < 1.0) {
            std::cout << "Avg calc time is below 1 second => using warm up" << std::endl;
        } else {
            initialRun = true;
            nFrames = 0;
            glutLeaveMainLoop();
        }
    } else if (nFrames == benchmarkWarmUp) {
        delete performanceMetricsCollector;
        performanceMetricsCollector = new PerformanceMetricsCollector();
    } else if (nFrames == (benchmarkWarmUp + benchmarkLength)) {
        initialRun = true;
        nFrames = 0;
        glutLeaveMainLoop();
    }
}

extern bool exitRenderLoop;

void render() {
    auto &visuals = SolarSystemVisuals::getInstance();
    bool isSolar = (dynamic_cast<SolarSystemDataSet*>(dataSet) != nullptr);

    if (initialRun) {
        modeLoc = glGetUniformLocation(shaderProgram, "mode");
        isSolarSystemLoc = glGetUniformLocation(shaderProgram, "isSolarSystem");
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);

        visuals.init(dataSet->getMaxPosition());

        performanceMetricsCollector = new PerformanceMetricsCollector;
        initialRun = false;
    }

    // Measure FPS
    static int frameCounter = 0;
    static float lastFpsTime = 0.0f;
    static float currentFps = 60.0f;
    frameCounter++;
    float now = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    if (now - lastFpsTime >= 0.5f) {
        currentFps = (float)frameCounter / (now - lastFpsTime);
        frameCounter = 0;
        lastFpsTime = now;
    }

    // Retrieve positions of the major bodies (Sun, 9 planets, Comet)
    float majorPositions[11 * 3] = {0};
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(majorPositions), majorPositions);

    // Camera target tracking
    if (focusIndex >= 0 && focusIndex <= 10 && isSolar) {
        targetCenter = visuals.getBodyNormalizedPosition(focusIndex, majorPositions, dataSet->getMaxPosition());
    } else if (focusIndex == -1) {
        targetCenter = glm::vec3(0.0f);
    }

    // Smooth camera interpolation
    currentCenter = glm::mix(currentCenter, targetCenter, 0.12f);
    camDistance = glm::mix(camDistance, targetDistance, 0.12f);

    if (automaticCameraRotation) {
        camYaw += 0.0035f;
    }

    float cx = camDistance * std::cos(camPitch) * std::sin(camYaw);
    float cy = camDistance * std::sin(camPitch);
    float cz = camDistance * std::cos(camPitch) * std::cos(camYaw);
    glm::vec3 camPos = currentCenter + glm::vec3(cx, cy, cz);

    M_view = glm::lookAt(camPos, currentCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    int screenW = glutGet(GLUT_WINDOW_WIDTH);
    int screenH = glutGet(GLUT_WINDOW_HEIGHT);
    if (screenH <= 0) screenH = 1;
    glViewport(0, 0, screenW, screenH);
    M_projection = glm::perspective(glm::radians(55.0f), (float)screenW / (float)screenH, 0.0002f, 250.0f);

    // Update uniform matrices in the legacy shader
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(M_model_loc, 1, GL_FALSE, glm::value_ptr(M_model));
    glUniformMatrix4fv(M_view_loc, 1, GL_FALSE, glm::value_ptr(M_view));
    glUniformMatrix4fv(M_projection_loc, 1, GL_FALSE, glm::value_ptr(M_projection));
    glUniform1i(isSolarSystemLoc, isSolar ? 1 : 0);
    glUniform1f(zoomFactorLoc, zoomFactor);

    // Deep space clear color
    glClearColor(0.010f, 0.012f, 0.024f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (isSolar) {
        // 1. Starfield background
        visuals.renderStarfield(M_view, M_projection, now);

        // 2. Reference orbits
        visuals.renderReferenceOrbits(M_view, M_projection, dataSet->getMaxPosition());

        // 3. Historical motion trails
        visuals.renderMotionTrails(M_view, M_projection, majorPositions, dataSet->getMaxPosition());

        // 4. 3D Celestial bodies: glowing Sun with corona, 8 planets + Pluto, Saturn's rings, Comet tail
        visuals.renderCelestialBodies(M_view, M_projection, camPos, majorPositions, dataSet->getMaxPosition(), now);

        // 5. Asteroid belt, Kuiper belt, and simulated star N-body particles
        if (numRenderElements > 11) {
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_PROGRAM_POINT_SIZE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous glow for celestial points

            glUseProgram(shaderProgram);
            glUniformMatrix4fv(M_model_loc, 1, GL_FALSE, glm::value_ptr(M_model));
            glUniformMatrix4fv(M_view_loc, 1, GL_FALSE, glm::value_ptr(M_view));
            glUniformMatrix4fv(M_projection_loc, 1, GL_FALSE, glm::value_ptr(M_projection));
            glUniform1f(glGetUniformLocation(shaderProgram, "maxPos"), dataSet->getMaxPosition());
            glUniform1f(glGetUniformLocation(shaderProgram, "minMass"), dataSet->getMinMass());
            glUniform1f(glGetUniformLocation(shaderProgram, "maxMass"), dataSet->getMaxMass());
            glUniform1i(isSolarSystemLoc, 1);
            glUniform1ui(modeLoc, 0);

            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 11, (GLsizei)(numRenderElements - 11));

            // 3D Coordinate system axes (Red, Green, Blue) from original project
            glUniform1ui(modeLoc, 1);
            glDrawArrays(GL_LINES, 0, 6);

            glBindVertexArray(0);
            glDisable(GL_BLEND);
        }

        // 6. Billboard labels
        visuals.renderLabels(M_view, M_projection, majorPositions, dataSet->getMaxPosition(), screenW, screenH);

        // 7. Interactive HUD overlay
        std::string backend = useGPU ? "OpenCL GPU" : (useCPU ? "CPU OpenMP" : "Simulation");
        visuals.renderHUD(screenW, screenH, isPaused, simSpeedMultiplier, (int)numRenderElements,
                          backend, focusIndex, currentFps, (float)(executionTime * 1000.0));
    } else {
        // Fallback for Wikipedia dataset
        visuals.renderStarfield(M_view, M_projection, now);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glUniform1ui(modeLoc, 0);
        glDrawArrays(GL_POINTS, 0, (GLsizei)numRenderElements);
        glUniform1ui(modeLoc, 1);
        glDrawArrays(GL_LINES, 0, 6);

        std::string backend = useGPU ? "OpenCL GPU" : "CPU OpenMP";
        visuals.renderHUD(screenW, screenH, isPaused, simSpeedMultiplier, (int)numRenderElements,
                          backend, -1, currentFps, (float)(executionTime * 1000.0));
    }

    glutSwapBuffers();

    // Numerical gravitational physics integration
    if (!isPaused) {
        for (int step = 0; step < simStepsPerFrame; ++step) {
            calcSimulationStep(useCPU, useGPU);
        }
    }
}

int openGlInit(int argc, char *argv[]) {
    // OpenGL initialization
    glutInit(&argc, argv);
    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    mainWindow = glutCreateWindow(WINDOW_TITLE);
    glutDisplayFunc(render);
    glutKeyboardFunc(glKeyboardCallback);
    glutMotionFunc(glMouseMovementClickCallback);
    glutCloseFunc(glutCleanup);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
#ifdef __unix__
    glutMouseFunc(glMouseWheelCallback);
#elif _WIN32
    glutMouseWheelFunc(glMouseWheelCallback);
#endif
    glutIdleFunc(glutPostRedisplay);

    glewInit();

    // Initialize Shaders
    std::string vertexShaderCode =
    #include VERTEX_SHADER_PATH
    ;
    std::string fragmentShaderCode =
    #include FRAGMENT_SHADER_PATH
    ;

    GLint isCompiled = -1;
    char shaderInfoLog[512];

    const GLchar *shaderCode = vertexShaderCode.c_str();
    assert(nullptr != shaderCode);
    uint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &shaderCode, nullptr);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
    if (GL_FALSE == isCompiled) {
        glGetShaderInfoLog(vertexShader, 512, NULL, shaderInfoLog);
        std::cerr << "Vertex Shader compilation failed. Error: " << shaderInfoLog << "\n";
        glDeleteShader(vertexShader);
        return isCompiled;
    }

    shaderCode = fragmentShaderCode.c_str();
    assert(nullptr != shaderCode);
    uint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &shaderCode, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
    if (GL_FALSE == isCompiled) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, shaderInfoLog);
        std::cerr << "Fragment Shader compilation failed. Error: " << shaderInfoLog << "\n";
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return isCompiled;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glValidateProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate Vertex Buffer
    vbo = 0;
    glGenBuffers(1, &vbo);

    // Generate Color Buffer
    mbo = 0;
    glGenBuffers(1, &mbo);

    // Generate Line Buffer
    lbo = 0;
    glGenBuffers(1, &lbo);

    // Generate Line Color Buffer
    lco = 0;
    glGenBuffers(1, &lco);

    vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * dataSet->getFlatPositions().size(), dataSet->getFlatPositions().data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void *>(0));
    glEnableVertexArrayAttrib(vao, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * dataSet->getMasses().size(), dataSet->getMasses().data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, static_cast<void *>(0));
    glEnableVertexArrayAttrib(vao, 1);

    glBindBuffer(GL_ARRAY_BUFFER, lbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * coordinateSystemLines.size(), coordinateSystemLines.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void *>(0));
    glEnableVertexArrayAttrib(vao, 2);

    glBindBuffer(GL_ARRAY_BUFFER, lco);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lineColors.size(), lineColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void *>(0));
    glEnableVertexArrayAttrib(vao, 3);

    glLineWidth(COORD_AXIS_WIDTH);

    GLenum errCode = glGetError();
    if (GL_NO_ERROR != errCode) {
        std::cerr << "OpenGL encountered the following error: " << errCode << "\n";
        return errCode;
    }

    // Init Vertex Matrices
    M_model = glm::mat4(1.0f);
    M_view = glm::lookAt(glm::vec3(0.0f, 1.2f, 1.8f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    M_projection = glm::perspective(glm::radians(FOV), static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, 0.001f, 300.0f);

    M_model_loc = glGetUniformLocation(shaderProgram, "M_model");
    M_view_loc = glGetUniformLocation(shaderProgram, "M_view");
    M_projection_loc = glGetUniformLocation(shaderProgram, "M_projection");
    isSolarSystemLoc = glGetUniformLocation(shaderProgram, "isSolarSystem");
    glUniformMatrix4fv(M_model_loc, 1, GL_FALSE, glm::value_ptr(M_model));
    glUniformMatrix4fv(M_view_loc, 1, GL_FALSE, glm::value_ptr(M_view));
    glUniformMatrix4fv(M_projection_loc, 1, GL_FALSE, glm::value_ptr(M_projection));

    int minMass_loc = glGetUniformLocation(shaderProgram, "minMass");
    int maxMass_loc = glGetUniformLocation(shaderProgram, "maxMass");
    glUniform1f(minMass_loc, dataSet->getMinMass());
    glUniform1f(maxMass_loc, dataSet->getMaxMass());

    int maxPos_loc = glGetUniformLocation(shaderProgram, "maxPos");
    glUniform1f(maxPos_loc, dataSet->getMaxPosition());
    numRenderElements = dataSet->getSize();

    zoomFactorLoc = glGetUniformLocation(shaderProgram, "zoomFactor");
    glUniform1f(zoomFactorLoc, zoomFactor);
    return 0;
}

void glutCleanup() {
    SolarSystemVisuals::getInstance().cleanup();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &mbo);
    glDeleteBuffers(1, &lbo);
    glDeleteBuffers(1, &lco);
}