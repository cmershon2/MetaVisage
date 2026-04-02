#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec3 vFragPos;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uViewProjection * worldPos;
    vFragPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
}
