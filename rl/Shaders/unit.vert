#version 450

// Unit array structure (matches UnitGPUParams)
struct UnitData {
    uint unitId;
    float temperature;
    float moisture;
    float roughness;
    float metallic;
    float albedoR;
    float albedoG;
    float albedoB;
    float reflectivity;
    float refractiveIndex;
    float dirtiness;
    float hardness;
    float explosionResistance;
    float transparency;
    float emissiveIntensity;
    float subsurfaceScattering;
    float flammability;
    float lightEmit;
    float lightOpacity;
    float ambientOcclusion;
    float lightAbsorption;
    float lightScattering;
    float humidity;
    uint isLiquid;
    uint isGas;
    uint isSolid;
    float padding[3];
};

// Polygon fence structure
struct PolFence {
    float t, d, b, f; // Top, Down, Back, Front
};

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
layout (location = 14) in uint a_UnitId;          // Unit ID for array lookup (4 bytes)

// Unit array buffer
layout (std430, set = 0, binding = 18) readonly buffer UnitArray {
    uint unitCount;
    uint padding0;
    uint padding1;
    uint padding2;
    UnitData units[];
} unitArray;

// Polygon fence array buffer
layout (std430, set = 0, binding = 19) readonly buffer PolFenceArray {
    uint fenceCount;
    uint padding0;
    uint padding1;
    uint padding2;
    PolFence fences[];
} polFenceArray;

layout (std140, set = 0, binding = 3) uniform MVPBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout (push_constant) uniform PushConstants {
    uint useUnitArray; // 0 = use vertex data, 1 = use unit array
    uint singleUnitMode; // 1 = render single unit only
    uint singleUnitId; // unit ID for single unit mode
    uint padding1;
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

    // Determine which unit ID to use
    uint targetUnitId = a_UnitId;
    if (pc.singleUnitMode == 1u) {
        targetUnitId = pc.singleUnitId;
    }

    // Fetch unit data from array if enabled
    if (pc.useUnitArray == 1u) {
        // Find unit in array
        for (uint i = 0; i < unitArray.unitCount; ++i) {
            if (unitArray.units[i].unitId == targetUnitId) {
                // Override vertex data with unit array data
                roughness = unitArray.units[i].roughness;
                metallic = unitArray.units[i].metallic;
                albedo = vec3(unitArray.units[i].albedoR, unitArray.units[i].albedoG, unitArray.units[i].albedoB);
                transparencyLevel = uint(unitArray.units[i].transparency * 255.0);
                lightingEmit = uint(unitArray.units[i].lightEmit * 255.0);
                break;
            }
        }

        // Fetch polygon fence from array
        for (uint i = 0; i < polFenceArray.fenceCount; ++i) {
            if (i == targetUnitId) {
                // Apply polygon fence offsets
                vec4 polRight = vec4(polFenceArray.fences[i].t, polFenceArray.fences[i].d, 
                                     polFenceArray.fences[i].b, polFenceArray.fences[i].f);
                vec4 polLeft = vec4(polFenceArray.fences[i].t, polFenceArray.fences[i].d,
                                    polFenceArray.fences[i].b, polFenceArray.fences[i].f);
                // Apply to position based on face index
                switch (faceIndex) {
                    case 0: localPos.y += polRight.x; break; // Top
                    case 1: localPos.y += polRight.y; break; // Bottom
                    case 2: localPos.x += polLeft.x; break;  // Left
                    case 3: localPos.x += polRight.z; break; // Right
                    case 4: localPos.z += polRight.w; break; // Front
                    case 5: localPos.z += polLeft.z; break;  // Back
                }
                break;
            }
        }
    }

    // Transform to world space
    vec4 worldPos = ubo.model * vec4(localPos, 1.0);

    // Transform to clip space
    gl_Position = ubo.projection * ubo.view * worldPos;

    // Tangent and bitangent are in local space, transform to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
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
    v_GeometricNormal = normalize(mat3(ubo.model) * a_Normal.xyz);
    v_FaceIndex = a_FaceIndex;
    v_PolCurve = a_PolCurve.xy;
}