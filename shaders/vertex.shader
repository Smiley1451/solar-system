R"(
#version 330 core

layout(location=0) in vec3 inPosition;
layout(location=1) in float inMass;
layout(location=2) in vec3 lineCoordinates;
layout(location=3) in vec3 lineColor;

uniform mat4 M_model;
uniform mat4 M_view;
uniform mat4 M_projection;

uniform float maxPos;
uniform float minMass;
uniform float maxMass;

uniform uint mode; // 0 = Bodies, 1 = Lines
uniform int isSolarSystem;

uniform float zoomFactor = 1.0f;

const float MAX_ADD_RADIUS = 60.0f;
const float BASE_RADIUS = 1.0f;

out vec3 lColor;

out float logInMass;
out float logMinMass;
out float logMaxMass;

float log10(float x){
    return log(max(x, 1e-10)) / 2.302585092994046;
}

void main(){
    // Used for rendering the bodies
    if (mode == 0u){
        logInMass = log10(inMass);
        logMinMass = log10(minMass);
        logMaxMass = log10(maxMass);
        if (isSolarSystem == 1) {
            vec3 normPos = inPosition / maxPos;
            gl_Position = M_projection * M_view * M_model * vec4(normPos, 1.0);
            // Particle point size for asteroid, kuiper, and simulated star bodies
            float massNorm = clamp((logInMass - logMinMass) / max(logMaxMass - logMinMass, 0.001f), 0.0f, 1.0f);
            gl_PointSize = clamp(3.2f + massNorm * 4.8f, 2.5f, 9.0f);
        } else {
            gl_Position = M_projection * M_view * M_model * vec4(inPosition.x, inPosition.z, inPosition.y, maxPos);
            gl_PointSize = float((logInMass - logMinMass) / max(logMaxMass - logMinMass, 0.001f) * MAX_ADD_RADIUS) + BASE_RADIUS;
            if(zoomFactor > 1.0f && logInMass >= 0.7f * logMaxMass){
                gl_PointSize *= zoomFactor;
            } else if (zoomFactor < 1.0f){
                gl_PointSize *= zoomFactor;
            }
        }
    }
    // Used for rendering the coordinate system axes
    else if (mode == 1u){
        gl_Position = M_projection * M_view * M_model * vec4(lineCoordinates, 1.0f);
        lColor = lineColor;
    }
}
)"