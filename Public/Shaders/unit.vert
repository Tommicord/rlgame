#version 450

// Input from compute shader output buffer (world space)
layout (location = 0) in vec4 a_Position;          // Clip space position (16 bytes)
layout (location = 1) in vec4 a_WorldPosAndUV;
layout (location = 2) in vec4 a_UVAndLighting;
layout (location = 3) in vec4 a_Material;
layout (location = 4) in vec4 a_RoughnessAndTan;
layout (location = 5) in vec4 a_Bitangent;
layout (location = 6) in vec4 a_GeometricNormal;

// Push constants for camera matrices
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pc;

// Output to fragment shader
layout (location = 0) out vec3 v_WorldPos;
layout (location = 1) smooth out vec2 v_TexCoords;
layout (location = 2) flat out uint v_LightingEmit;
layout (location = 3) flat out uint v_TransparencyLevel;
layout (location = 4) flat out uint v_FaceIndex;
layout (location = 5) flat out vec3 v_Albedo;
layout (location = 6) flat out float v_Metallic;
layout (location = 7) flat out float v_Roughness;
layout (location = 8) out mat3 v_TBN;
layout (location = 9) smooth out vec3 v_GeometricNormal;

void main() {
    vec3 worldPos = a_WorldPosAndUV.xyz;
    vec2 texCoords = vec2(a_WorldPosAndUV.w, a_UVAndLighting.x);

    uint lightingEmit     = uint(a_UVAndLighting.y);
    uint transparencyLevel = uint(a_UVAndLighting.z);
    uint faceIndex        = uint(a_UVAndLighting.w);

    vec3 albedo = a_Material.xyz;
    float metallic = a_Material.w;

    float roughness = a_RoughnessAndTan.x;
    vec3 tangent = a_RoughnessAndTan.yzw;
    vec3 bitangent = a_Bitangent.xyz;
    vec3 geometricNormal = a_GeometricNormal.xyz;

    gl_Position = a_Position;

    // Tangent and bitangent are already in world space from compute shader
    // No need to transform them with model matrix
    vec3 T = normalize(tangent);
    vec3 B = normalize(bitangent);
    vec3 N = normalize(cross(B, T));

    N = normalize(N);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(T, N));

    v_WorldPos = worldPos;
    v_TexCoords = texCoords;
    v_LightingEmit = lightingEmit;
    v_TransparencyLevel = transparencyLevel;
    v_FaceIndex = faceIndex;
    v_Albedo = albedo;
    v_Metallic = metallic;
    v_Roughness = roughness;
    v_TBN = mat3(T, B, N);
    v_GeometricNormal = normalize(geometricNormal);
}