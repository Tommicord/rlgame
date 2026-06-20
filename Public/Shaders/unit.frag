#version 450

// Input from geometry shader
layout (location = 0) flat in vec3 g_WorldPos;
layout (location = 1) flat in vec2 g_TexCoords;
layout (location = 2) flat in uint g_LightingEmit;
layout (location = 3) flat in uint g_TransparencyLevel;
layout (location = 4) flat in uint g_FaceIndex;

// Output color
layout (location = 0) out vec4 outColor;

// The texture of the Unit
// The unit has 6 faces
layout (binding = 2) uniform sampler2D u_Texture[6];

// Lighting input (if using separate lighting pass)
layout (binding = 8) uniform sampler2D u_LightingTexture;

// Use separate lighting pass
layout(set = 0, binding = 9) uniform SettingsBlock {
    bool u_UseLightingPass;
} settings;

void main() {
    // Sample texture based on face index
    vec4 texColor = texture(u_Texture[g_FaceIndex], g_TexCoords);
    // Apply transparency
    float transparency = float(g_TransparencyLevel) / 255.0;
    texColor.a = mix(texColor.a, 1.0 - transparency, transparency);
    
    // Apply lighting if using separate lighting pass
    vec3 finalColor;
    if (settings.u_UseLightingPass) {
        // Sample lighting texture (screen space)
        vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(u_LightingTexture, 0));
        vec4 lighting = texture(u_LightingTexture, screenUV);
        finalColor = texColor.rgb * lighting.rgb;
    } else {
        // Basic output without lighting pass
        finalColor = texColor.rgb;
    }
    // Output final color
    outColor = vec4(finalColor, texColor.a);
}