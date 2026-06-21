#version 450

// Input from vertex shader (after compute shader culling)
layout (location = 0) flat in vec3 v_WorldPos;
layout (location = 1) in vec2 v_TexCoords;
layout (location = 2) flat in uint v_LightingEmit;
layout (location = 3) flat in uint v_TransparencyLevel;
layout (location = 4) flat in uint v_FaceIndex;
layout (location = 5) flat in vec3 v_Albedo;
layout (location = 6) flat in float v_Metallic;
layout (location = 7) flat in float v_Roughness;
layout (location = 8) flat in mat3 v_TBN;
layout (location = 9) smooth in vec3 v_GeometricNormal;

// Output color
layout (location = 0) out vec4 outColor;

// Lighting uniforms
layout(set = 0, binding = 4) uniform LightingBlock {
    vec3 u_SunDirection;
    vec3 u_SunColor;
    float u_AmbientStrength;
    vec3 u_CameraPosition;
} lighting;

// Ambient occlusion texture
layout (binding = 10) uniform sampler2D u_AOTexture;
layout (binding = 11) uniform sampler2D u_NormalTexture;

// The texture of the Unit
// The unit has 6 faces
layout (binding = 2) uniform sampler2D u_Texture[6];

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
    // For directional light, L is just the normalized light direction
    vec3 L = normalize(light.position);
    vec3 H = normalize(V + L);

    // Calculate radiance (no attenuation for directional light)
    vec3 radiance = light.color * light.intensity;

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

// Main PBR calculation
vec3 CalculatePBR(vec3 worldPos, vec3 normal, vec3 viewDir, PBRMaterial material,
                  Light sunLight, vec3 ambientColor) {
    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);

    // Calculate ambient lighting
    vec3 ambient = CalculateAmbient(N, V, F0, material, ambientColor);

    vec3 L = normalize(-sunLight.position);
    vec3 H = normalize(V + L);

    vec3 radiance = sunLight.color * sunLight.intensity;
    float NDF = DistributionGGX(N, H, material.roughness);
    float G = GeometrySmith(N, V, L, material.roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * material.albedo / PI + specular) * radiance * NdotL;

    vec3 colorFinal = ambient + Lo;

    // Rim lighting: flash of sunlight when viewing corners and sun is close to view direction
    // Use geometric normal for smooth rim lighting across edges
    vec3 geoN = normalize(v_GeometricNormal);
    float rimFactor = 1.0 - max(dot(geoN, V), 0.0);
    rimFactor = pow(rimFactor, 2.0 + material.roughness * 2.0); // Smoother falloff based on roughness
    
    // Check if sun direction is close to view direction (for flash effect)
    vec3 sunDir = normalize(-sunLight.position);
    float sunViewAlignment = max(dot(sunDir, V), 0.0);
    float flashIntensity = pow(sunViewAlignment, 4.0 + material.roughness * 4.0); // Smoother falloff based on roughness
    
    // Combine rim factor with sun alignment for corner flash
    float cornerFlash = rimFactor * flashIntensity * 0.6;
    
    // Add sunlight color flash with smooth blending
    vec3 rimLight = cornerFlash * sunLight.color * sunLight.intensity;
    colorFinal += rimLight;

    return colorFinal;
}

void main() {
    // Normal maps
    vec3 normalMapSample = texture(u_NormalTexture, v_TexCoords).rgb;
    vec3 tangentNormal = normalMapSample * 2.0 - 1.0;
    vec3 normal = normalize(v_TBN * tangentNormal);

    // Sample texture based on face index
    vec4 texColor = texture(u_Texture[v_FaceIndex], v_TexCoords);

    // Apply transparency
    float transparency = float(v_TransparencyLevel) / 255.0;
    texColor.a = mix(texColor.a, 1.0 - transparency, transparency);

    // Setup PBR material with texture albedo
    PBRMaterial material;
    material.albedo = texColor.rgb;
    material.metallic = v_Metallic;
    material.roughness = v_Roughness;

    // Sample ambient occlusion
    float ao = texture(u_AOTexture, v_TexCoords).r;
    material.ao = ao;

    // Calculate view direction
    vec3 viewDir = normalize(lighting.u_CameraPosition - v_WorldPos);

    // Setup sun light
    Light sunLight;
    sunLight.position = lighting.u_SunDirection; // Directional light direction
    sunLight.color = lighting.u_SunColor;
    sunLight.intensity = 1.0;

    // Calculate ambient color
    vec3 ambientColor = clamp(lighting.u_AmbientStrength, 0.0, 1.0) * lighting.u_SunColor;

    // Calculate PBR lighting
    vec3 pbrColor = CalculatePBR(v_WorldPos, normal, viewDir, material, sunLight, ambientColor);

    // Calculate self-emission
    float emission = float(v_LightingEmit) / 255.0;
    vec3 emitColor = emission * lighting.u_SunColor;

    // Combine PBR lighting with emission
    vec3 finalColor = pbrColor + emitColor;

    // Output final color with texture alpha
    outColor = vec4(finalColor, texColor.a);
}