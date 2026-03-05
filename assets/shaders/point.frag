#version 430 core

in vec3 vColor;
out vec4 FragColor;

void main() {
    // Draw circle from point sprite using gl_PointCoord
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    if (dist > 0.5) discard;

    // Soft edge
    float alpha = 1.0 - smoothstep(0.35, 0.5, dist);

    // Slight outline effect
    float outline = smoothstep(0.25, 0.35, dist);
    vec3 color = mix(vColor, vColor * 0.6, outline);

    FragColor = vec4(color, alpha);
}
