#version 450

// Input from geometry shader
layout (location = 0) in vec3 g_WorldPos;
layout (location = 1) in vec2 g_TexCoords;
layout (location = 2) in uint g_LightingEmit;
layout (location = 3) in uint g_TransparencyLevel;
layout (location = 4) in uint g_FaceIndex;

// Output color
layout (location = 0) out vec4 outColor;

// PBR Material inputs
layout (location = 5) in vec3 a_Albedo;
layout (location = 6) in float a_Metallic;
layout (location = 7) in float a_Roughness;

// Lighting uniforms
layout (binding = 4) uniform vec3 u_SunDirection;
layout (binding = 5) uniform vec3 u_SunColor;
layout (binding = 6) uniform float u_AmbientStrength;
layout (binding = 7) uniform vec3 u_CameraPosition;

// Ambient occlusion texture
layout (binding = 10) uniform sampler2D u_AOTexture;

// PBR Constants
const float PI = 3.14159265359;
const float EPSILON = 0.0001;

// PBR Material structure
struct PBRMaterial {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
};

// Light structure
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

// Distribution function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / max(denom, EPSILON);
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

// Geometry function (Smith)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel function (Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel function with roughness (for IBL)
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate PBR lighting for a single light
vec3 CalculatePBRLight(vec3 N, vec3 V, vec3 F0, PBRMaterial material, Light light) {
    vec3 L = normalize(light.position - N);
    vec3 H = normalize(V + L);
    
    // Calculate radiance
    float distance = length(light.position - N);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.color * light.intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, material.roughness);
    float G = GeometrySmith(N, V, L, material.roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    // Calculate specular and diffuse components
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    vec3 specular = numerator / denominator;
    
    // Calculate outgoing radiance
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * material.albedo / PI + specular) * radiance * NdotL;
    
    return Lo;
}

// Calculate ambient lighting with IBL (simplified)
vec3 CalculateAmbient(vec3 N, vec3 V, vec3 F0, PBRMaterial material, vec3 ambientColor) {
    vec3 kS = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, material.roughness);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - material.metallic;
    
    vec3 irradiance = ambientColor;
    vec3 ambient = kD * material.albedo * irradiance * material.ao;
    
    return ambient;
}

// Face normals for lighting calculation
const vec3 faceNormals[6] = vec3[](
    vec3( 0.0,  1.0,  0.0 ),  // Top
    vec3( 0.0, -1.0,  0.0 ), // Bottom
    vec3(-1.0,  0.0,  0.0 ), // Left
    vec3( 1.0,  0.0,  0.0 ),  // Right
    vec3( 0.0,  0.0,  1.0 ),  // Front
    vec3( 0.0,  0.0, -1.0 )  // Back
);

void main() {
    // Get face normal based on face index
    vec3 normal = faceNormals[g_FaceIndex];
    // Setup PBR material
    PBRMaterial material;
    material.albedo = a_Albedo;
    material.metallic = a_Metallic;
    material.roughness = a_Roughness;
    
    // Sample ambient occlusion
    float ao = texture(u_AOTexture, g_TexCoords).r;
    material.ao = ao;
    
    // Calculate view direction
    vec3 viewDir = normalize(u_CameraPosition - g_WorldPos);
    
    // Setup sun light
    Light sunLight;
    sunLight.position = u_SunDirection * 1000.0; // Directional light at distance
    sunLight.color = u_SunColor;
    sunLight.intensity = 1.0;
    
    // Calculate ambient color
    vec3 ambientColor = u_AmbientStrength * u_SunColor;
    
    // Calculate PBR lighting
    vec3 pbrColor = CalculatePBR(g_WorldPos, normal, viewDir, material, sunLight, ambientColor);
    
    // Calculate self-emission
    float emission = float(g_LightingEmit) / 255.0;
    vec3 emitColor = emission * u_SunColor;
    
    // Combine PBR lighting with emission
    vec3 finalColor = pbrColor + emitColor;
    
    // Output lighting result (will be multiplied with texture in unit.frag)
    outColor = vec4(finalColor, 1.0);
}