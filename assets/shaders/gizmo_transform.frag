#version 430 core

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uViewPos;

void main() {
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);

    // Simple hemisphere lighting for clean gizmo look
    float ndotv = dot(norm, viewDir);
    float lighting = 0.4 + 0.6 * max(abs(ndotv), 0.0);

    vec3 result = uColor * lighting;
    FragColor = vec4(result, 1.0);
}
