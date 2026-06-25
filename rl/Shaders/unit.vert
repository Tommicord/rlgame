#version 450

// Input from vertex buffer (UnitVertex structure)
layout (location = 0) in vec4 a_Position;          // Local position (16 bytes)
layout (location = 1) in vec4 a_PolRight;          // Polygon fence right (16 bytes)
layout (location = 2) in vec4 a_PolLeft;           // Polygon fence left (16 bytes)
layout (location = 3) in vec2 a_TexCoords;         // Texture coordinates (8 bytes)
layout (location = 4) in uint a_LightingEmit;      // Lighting emit value (4 bytes)
layout (location = 5) in uint a_TransparencyLevel; // Transparency level (4 bytes)
layout (location = 6) in uint a_FaceIndex;         // Face index (4 bytes)
layout (location = 7) in float a_Roughness;        // Roughness (4 bytes)
layout (location = 8) in float a_Metallic;         // Metallic (4 bytes)
layout (location = 9) in vec4 a_PolCurve;             // Curvature (4 bytes)
layout (location = 10) in vec4 a_Albedo;           // Albedo (16 bytes)
layout (location = 11) in vec4 a_Tangent;          // Tangent (16 bytes)
layout (location = 12) in vec4 a_Bitangent;        // Bitangent (16 bytes)
layout (location = 13) in vec4 a_Normal;           // Normal (16 bytes)

// Push constants for camera matrices
layout (push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pc;

// Output to fragment shader
layout (location = 0) smooth out vec3 v_WorldPos;
layout (location = 1) smooth out vec2 v_TexCoords;
layout (location = 2) flat out uint v_LightingEmit;
layout (location = 3) flat out uint v_TransparencyLevel;
layout (location = 4) flat out uint v_FaceIndex;
layout (location = 5) smooth out vec3 v_Albedo;
layout (location = 6) smooth out float v_Metallic;
layout (location = 7) smooth out float v_Roughness;
layout (location = 8) smooth out mat3 v_TBN;
layout (location = 11) smooth out vec3 v_GeometricNormal;
layout (location = 12) smooth out vec2 v_PolCurve;

void main() {
    vec3 localPos = a_Position.xyz;
    vec2 texCoords = a_TexCoords;
    float polCurveV = a_PolCurve.x;
    float polCurveH = a_PolCurve.y;

    uint lightingEmit = a_LightingEmit;
    uint transparencyLevel = a_TransparencyLevel;
    uint faceIndex = a_FaceIndex;

    vec3 albedo = a_Albedo.xyz;
    float metallic = a_Metallic;
    float roughness = a_Roughness;

    vec3 tangent = a_Tangent.xyz;
    vec3 bitangent = a_Bitangent.xyz;
    vec3 geometricNormal = a_Normal.xyz;

    // Transform to world space
    vec4 worldPos = pc.model * vec4(localPos, 1.0);

    // Transform to clip space
    gl_Position = pc.projection * pc.view * worldPos;

    // Tangent and bitangent are in local space, transform to world space
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * geometricNormal);

    // Re-orthogonalize TBN matrix
    N = normalize(N);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(T, N));

    v_WorldPos = worldPos.xyz;
    v_TexCoords = texCoords;
    v_LightingEmit = lightingEmit;
    v_TransparencyLevel = transparencyLevel;
    v_FaceIndex = faceIndex;
    v_Albedo = albedo;
    v_Metallic = metallic;
    v_Roughness = roughness;
    v_TBN = mat3(T, B, N);
    v_GeometricNormal = normalize(mat3(pc.model) * a_Normal.xyz);
    v_FaceIndex = a_FaceIndex;
    v_PolCurve = a_PolCurve.xy;
}