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
            float dist = sqrt(r2);
            // Solid bright core with radiant outer glow
            float core = clamp(1.0 - dist * 1.4, 0.0, 1.0);
            float glow = pow(1.0 - dist, 1.6);
            float alpha = clamp(core * 0.85 + glow * 0.65, 0.0, 1.0);

            float t = clamp((logInMass - logMinMass) / max(logMaxMass - logMinMass, 0.001), 0.0, 1.0);

            // Rich vibrant colors across celestial categories:
            // High mass: electric cyan & diamond blue-white
            // Mid mass: fiery gold & emerald
            // Low mass: warm amber & ruby-bronze
            vec3 particleColor;
            if (t > 0.60) {
                particleColor = mix(vec3(0.4, 0.9, 1.0), vec3(1.0, 1.0, 1.0), (t - 0.60) * 2.5);
            } else if (t > 0.30) {
                particleColor = mix(vec3(0.3, 1.0, 0.7), vec3(1.0, 0.85, 0.3), (t - 0.30) * 3.33);
            } else {
                particleColor = mix(vec3(1.0, 0.45, 0.15), vec3(1.0, 0.85, 0.3), t * 3.33);
            }

            fragColor = vec4(particleColor, alpha);
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