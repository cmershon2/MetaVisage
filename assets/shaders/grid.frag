#version 430 core

in vec3 vNearPoint;
in vec3 vFarPoint;

out vec4 FragColor;

uniform mat4 uViewProjection;
uniform float uNearPlane;
uniform float uFarPlane;

// Compute the intersection of the ray with the Y=0 plane
// Returns t parameter: point = near + t * (far - near)
float ComputeT() {
    return -vNearPoint.y / (vFarPoint.y - vNearPoint.y);
}

// Compute grid lines for a given scale
vec4 Grid(vec3 fragPos3D, float scale, float lineWidth) {
    vec2 coord = fragPos3D.xz / scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float alpha = 1.0 - min(line, 1.0);

    vec4 color = vec4(0.35, 0.35, 0.35, alpha);

    // Highlight X axis (red tint along Z=0)
    float axisLineZ = abs(fragPos3D.z) / derivative.y / scale;
    if (axisLineZ < 1.0) {
        color = vec4(0.6, 0.2, 0.2, max(alpha, 1.0 - axisLineZ));
    }

    // Highlight Z axis (blue tint along X=0)
    float axisLineX = abs(fragPos3D.x) / derivative.x / scale;
    if (axisLineX < 1.0) {
        color = vec4(0.2, 0.2, 0.6, max(alpha, 1.0 - axisLineX));
    }

    return color;
}

// Compute linear depth for depth buffer writing
float ComputeLinearDepth(vec3 pos) {
    vec4 clipPos = uViewProjection * vec4(pos, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    // Convert from [-1,1] to [0,1]
    return (ndcDepth * 0.5 + 0.5);
}

// Fade factor based on distance from camera
float ComputeFade(vec3 fragPos3D) {
    float dist = length(fragPos3D);
    // Fade starts at 20 units, fully faded at 100 units
    return 1.0 - smoothstep(20.0, 100.0, dist);
}

void main() {
    float t = ComputeT();

    // Discard fragments behind the camera (t < 0)
    if (t < 0.0) discard;

    vec3 fragPos3D = vNearPoint + t * (vFarPoint - vNearPoint);

    // Write depth so grid properly occludes/is occluded by meshes
    float depth = ComputeLinearDepth(fragPos3D);
    gl_FragDepth = depth;

    // Discard fragments outside the valid depth range
    if (depth < 0.0 || depth > 1.0) discard;

    // Multi-scale grid: fine lines (1 unit) + coarse lines (10 units)
    vec4 fineGrid = Grid(fragPos3D, 1.0, 1.0);
    vec4 coarseGrid = Grid(fragPos3D, 10.0, 1.5);

    float fade = ComputeFade(fragPos3D);

    // Blend coarse grid on top of fine grid
    vec4 color = fineGrid;
    color = mix(color, coarseGrid, coarseGrid.a);

    // Apply distance fade
    color.a *= fade;

    // Discard fully transparent fragments
    if (color.a < 0.001) discard;

    FragColor = color;
}
