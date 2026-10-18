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
    if (mode == 0u){
        logInMass = log10(inMass);
        logMinMass = log10(minMass);
        logMaxMass = log10(maxMass);
        
        vec3 normPos = inPosition / maxPos;
        gl_Position = M_projection * M_view * M_model * vec4(normPos, 1.0);
        
        // Point size scaling: prominent and bold (from 4.0 up to 18.0 pixels)
        float massNorm = clamp((logInMass - logMinMass) / max(logMaxMass - logMinMass, 0.001f), 0.0f, 1.0f);
        gl_PointSize = clamp(4.0f + massNorm * 12.0f, 3.5f, 18.0f);
    }
    else if (mode == 1u){
        gl_Position = M_projection * M_view * M_model * vec4(lineCoordinates, 1.0f);
        lColor = lineColor;
    }
}
)"