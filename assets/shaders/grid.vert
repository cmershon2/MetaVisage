#version 430 core

// Fullscreen triangle (3 vertices, no VBO needed)
// Vertex IDs 0,1,2 produce a triangle that covers the entire screen

uniform mat4 uViewProjection;
uniform mat4 uInvViewProjection;

out vec3 vNearPoint;
out vec3 vFarPoint;

vec3 UnprojectPoint(float x, float y, float z) {
    vec4 clip = uInvViewProjection * vec4(x, y, z, 1.0);
    return clip.xyz / clip.w;
}

// Generate fullscreen triangle from vertex ID
vec2 GetFullscreenPosition(int vertexID) {
    // Produces positions: (-1,-1), (3,-1), (-1,3)
    // This triangle covers the entire [-1,1] clip space
    float x = float((vertexID & 1) * 4 - 1);
    float y = float((vertexID & 2) * 2 - 1);
    return vec2(x, y);
}

void main() {
    vec2 pos = GetFullscreenPosition(gl_VertexID);
    gl_Position = vec4(pos, 0.0, 1.0);

    // Unproject near and far plane points to get a ray in world space
    vNearPoint = UnprojectPoint(pos.x, pos.y, 0.0);
    vFarPoint = UnprojectPoint(pos.x, pos.y, 1.0);
}
