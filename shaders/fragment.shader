R"(
#version 330 core

#define M_PI 3.14159265358979323846
#define EPSILON 0.0001

in vec3 lColor;

in float logInMass;
in float logMinMass;
in float logMaxMass;

uniform float minMass;
uniform float maxMass;

uniform uint mode; // 0 = Bodies, 1 = Lines
uniform int isSolarSystem;

out vec4 fragColor;

vec3 hsv2rgb(float h, float s, float v){
    vec3 rgb;
    float C = v * s;
    h = h * 180.0 / M_PI;
    h /= 60.0;
    float X = C * (1.0 - abs(mod(h, 2.0) - 1.0));

    if(h >= 0.0 && h < 1.0){
        rgb = vec3(C, X, 0.0);
    } else if(h >= 1.0 && h < 2.0){
        rgb = vec3(X, C, 0.0);
    } else if(h >= 2.0 && h < 3.0){
        rgb = vec3(0.0, C, X);
    } else if(h >= 3.0 && h < 4.0){
        rgb = vec3(0.0, X, C);
    } else if(h >= 4.0 && h < 5.0){
        rgb = vec3(X, 0.0, C);
    } else if(h >= 5.0 && h < 6.0){
        rgb = vec3(C, 0.0, X);
    } else {
        rgb = vec3(0.0);
    }

    float m = v - C;
    return rgb + vec3(m);
}

void main(){
    if(mode == 0u){
        if (isSolarSystem == 1) {
            vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
            float r2 = dot(circCoord, circCoord);
            if (r2 > 1.0) {
                discard;
            }
            float alpha = (1.0 - r2 * r2) * 0.85;
            float t = clamp((logInMass - logMinMass) / max(logMaxMass - logMinMass, 0.001), 0.0, 1.0);
            vec3 rockyColor = mix(vec3(0.70, 0.67, 0.62), vec3(0.88, 0.82, 0.65), t);
            fragColor = vec4(rockyColor, alpha);
        } else {
            float degree = float(2.0 * M_PI * (logInMass - logMinMass) / (logMaxMass + EPSILON - logMinMass));
            vec3 pixelColor = hsv2rgb(degree, 1.0, 1.0);
            fragColor = vec4(pixelColor, 1.0);
        }
    } else if (mode == 1u){
        fragColor = vec4(lColor, 1.0);
    }
}
)"