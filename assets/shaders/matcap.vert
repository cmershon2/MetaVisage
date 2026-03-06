#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vViewNormal;
out vec3 vViewPos;

void main() {
    vec4 viewPos = uModelView * vec4(aPos, 1.0);
    vViewPos = viewPos.xyz;
    vViewNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uProjection * viewPos;
}
