#version 430 core

in vec3 vViewNormal;
in vec3 vViewPos;

out vec4 FragColor;

void main() {
    // Procedural clay-like MatCap: use view-space normal to compute lighting
    vec3 N = normalize(vViewNormal);

    // Map view-space normal XY to 0..1 range for hemisphere lookup
    vec2 matcapUV = N.xy * 0.5 + 0.5;

    // Procedural clay material: warm gray with directional rim highlight
    vec3 baseColor = vec3(0.65, 0.58, 0.52);     // Warm clay base
    vec3 highlightColor = vec3(0.95, 0.90, 0.85); // Bright highlight
    vec3 shadowColor = vec3(0.25, 0.22, 0.20);    // Deep shadow

    // Hemisphere lighting: top bright, bottom dark
    float hemi = matcapUV.y;
    vec3 hemiLight = mix(shadowColor, baseColor, smoothstep(0.15, 0.55, hemi));
    hemiLight = mix(hemiLight, highlightColor, smoothstep(0.65, 0.95, hemi));

    // Rim lighting for edge definition
    vec3 viewDir = normalize(-vViewPos);
    float rim = 1.0 - max(dot(N, viewDir), 0.0);
    rim = smoothstep(0.5, 1.0, rim);
    vec3 rimColor = vec3(0.80, 0.75, 0.70) * rim * 0.4;

    // Specular highlight
    float spec = pow(max(matcapUV.y, 0.0), 8.0) * 0.3;

    vec3 finalColor = hemiLight + rimColor + vec3(spec);
    FragColor = vec4(finalColor, 1.0);
}
