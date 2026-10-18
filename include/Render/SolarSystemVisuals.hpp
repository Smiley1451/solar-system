/**
 * @file SolarSystemVisuals.hpp
 * @brief High-quality 3D celestial rendering: procedural planets, glowing Sun,
 *        Saturn's rings, comet tails, orbital paths, motion trails, starfield,
 *        billboard labels, and HUD overlay.
 */

#ifndef __SOLAR_SYSTEM_VISUALS_HPP__
#define __SOLAR_SYSTEM_VISUALS_HPP__

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

struct SphereMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct RingMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct StarfieldMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei starCount = 0;
};

struct OrbitMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei pointCount = 0;
};

class SolarSystemVisuals {
public:
    static SolarSystemVisuals& getInstance();

    void init(float maxPosition);
    void cleanup();

    void renderStarfield(const glm::mat4 &view, const glm::mat4 &proj, float timeSec);
    void renderReferenceOrbits(const glm::mat4 &view, const glm::mat4 &proj, float maxPos);
    void renderMotionTrails(const glm::mat4 &view, const glm::mat4 &proj, const float *majorPositions, float maxPos);
    void renderCelestialBodies(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos,
                               const float *majorPositions, float maxPos, float timeSec);
    void renderNBodyParticles(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos,
                             GLuint vbo, GLuint mbo, size_t totalCount, float maxPos, float timeSec);
    void renderSaturnRings(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &saturnPos, float saturnRadius);
    void renderCometTail(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &cometPos, const glm::vec3 &sunPos);
    void renderLabels(const glm::mat4 &view, const glm::mat4 &proj, const float *majorPositions, float maxPos, int screenW, int screenH);
    void renderHUD(int screenW, int screenH, bool isPaused, float simSpeed, int totalBodies,
                   int submittedBodies, int renderedBodies, int asteroidCount, int kuiperCount, int starCount,
                   const std::string &backend, int focusIndex, float fps, float calcTimeMs);

    // State toggles
    bool showOrbits = true;
    bool showTrails = true;
    bool showLabels = true;
    bool showStarfield = true;
    bool showParticles = true;
    bool showHUD = true;

    glm::vec3 getBodyNormalizedPosition(int bodyIndex, const float *majorPositions, float maxPos) const;
    std::string getBodyName(int bodyIndex) const;
    float getBodyVisualRadius(int bodyIndex) const;

private:
    SolarSystemVisuals() = default;
    ~SolarSystemVisuals() = default;

    bool initialized = false;
    float cachedMaxPos = 6.5e12f;

    // Meshes
    SphereMesh sphere;
    RingMesh saturnRing;
    StarfieldMesh starfield;
    std::vector<OrbitMesh> orbitMeshes;

    // Trail history: 11 major bodies, ring buffers of recent positions
    static const size_t MAX_TRAIL_LENGTH = 160;
    std::vector<std::vector<glm::vec3>> trailHistory;
    GLuint trailVao = 0;
    GLuint trailVbo = 0;

    // Shaders
    GLuint sunShader = 0;
    GLuint planetShader = 0;
    GLuint ringShader = 0;
    GLuint starfieldShader = 0;
    GLuint simpleLineShader = 0;
    GLuint cometShader = 0;
    GLuint particleShader = 0;
    GLuint particleVao = 0;

    void buildSphere(int slices = 36, int stacks = 36);
    void buildSaturnRings(int segments = 80);
    void buildStarfield(int starCount = 4500);
    void buildOrbitMeshes(float maxPos);
    void buildShaders();

    GLuint compileShader(GLenum type, const char* source);
    GLuint linkProgram(GLuint vs, GLuint fs);
};

#endif // __SOLAR_SYSTEM_VISUALS_HPP__
