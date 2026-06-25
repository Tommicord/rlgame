#version 450

// Input from vertex shader (after compute shader culling)
layout (location = 0) smooth in vec3 v_WorldPos;
layout (location = 1) smooth in vec2 v_TexCoords;
layout (location = 2) flat in uint v_LightingEmit;
layout (location = 3) flat in uint v_TransparencyLevel;
layout (location = 4) flat in uint v_FaceIndex;
layout (location = 5) smooth in vec3 v_Albedo;
layout (location = 6) smooth in float v_Metallic;
layout (location = 7) smooth in float v_Roughness;
layout (location = 8) smooth in mat3 v_TBN;
layout (location = 11) smooth in vec3 v_GeometricNormal;

// Output color
layout (location = 0) out vec4 outColor;

// Light structure
struct Light {
    vec3 direction;
    vec3 color;
    float intensity;
    float padding;
};

// Lighting uniforms
layout (std140, set = 0, binding = 4) uniform LightingBlock {
    vec4 u_SunDirection;
    vec4 u_SunColor;
    float u_SunIntensity;
    uint u_AdditionalLightCount;
    float u_AmbientStrength;
    float u_Exposure;
    vec4 u_CameraPosition;
    Light u_AdditionalLights[4];

    // Spherical harmonics for GI (9 coefficients for RGB)
    vec4 u_SHCoefficients[9];
    vec4 u_GroundColor;
    vec4 u_SkyColor;
    mat4 u_LightSpaceMatrix;

    // LOD settings
    float u_LODDistanceNear;   // Distance threshold for high quality
    float u_LODDistanceFar;    // Distance threshold for low quality
    uint u_QualityLevel;       // 0=low, 1=medium, 2=high
} lighting;

// Ambient occlusion texture
layout (binding = 10) uniform sampler2D u_AOTexture;
layout (binding = 11) uniform sampler2D u_NormalTexture;
layout (binding = 13) uniform sampler2DShadow u_ShadowMap;

// Triplanar mapping settings
layout (binding = 12) uniform TriplanarSettings {
    float scale;
    float sharpness;
    float offsetX;
    float offsetY;
    float offsetZ;
    float blendMix;
} triplanar;

// The texture of the Unit
// The unit has 6 faces
layout (binding = 2) uniform sampler2D u_Texture[6];

// PBR Constants
const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const float MAX_REFLECTION_LOD = 4.0;

// PBR Material structure
struct PBRMaterial {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
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

// ACES filmic tone mapping with better HDR handling
vec3 ACESToneMapping(vec3 color) {
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

// Enhanced tone mapping with exposure adaptation
vec3 EnhancedToneMapping(vec3 color, float exposure) {
    // Apply exposure
    color *= exposure;

    // ACES tone mapping
    vec3 mapped = ACESToneMapping(color);

    // Subtle contrast adjustment
    mapped = pow(mapped, vec3(1.1));

    return clamp(mapped, 0.0, 1.0);
}

// Hemispheric ambient lighting with raytracing-like approximation
vec3 CalculateHemisphericAmbient(vec3 normal, vec3 groundColor, vec3 skyColor, float intensity) {
    float hemi = 0.5 + 0.5 * normal.y;

    // Add occlusion factor for more realistic ambient
    float occlusion = 1.0 - max(0.0, -normal.y) * 0.3;

    return mix(groundColor, skyColor, hemi) * intensity * occlusion;
}

// Spherical harmonics ambient lighting for improved GI
vec3 CalculateSHAmbient(vec3 normal, vec4 shCoefficients[9]) {
    // Simplified spherical harmonics evaluation (3 bands)
    vec3 result = shCoefficients[0].xyz; // L0

    // L1 band
    result += shCoefficients[1].xyz * normal.y;
    result += shCoefficients[2].xyz * normal.z;
    result += shCoefficients[3].xyz * normal.x;

    // L2 band (simplified)
    float xx = normal.x * normal.x;
    float yy = normal.y * normal.y;
    float zz = normal.z * normal.z;
    result += shCoefficients[4].xyz * (3.0 * yy - 1.0);
    result += shCoefficients[5].xyz * (normal.y * normal.z);
    result += shCoefficients[6].xyz * (normal.x * normal.z);
    result += shCoefficients[7].xyz * (normal.x * normal.y);
    result += shCoefficients[8].xyz * (xx - zz);

    return max(result, vec3(0.0));
}

// PCF shadow calculation with 3x3 kernel
float CalculateShadow(vec3 fragPosWorldSpace, vec3 lightDir, vec3 normal) {
    vec4 fragPosLightSpace = lighting.u_LightSpaceMatrix * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (
    projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0
    ) {
        return 1.0;
    }

    float minBias = 0.0005;
    float maxBias = 0.005;
    float bias = max(maxBias * (1.0 - dot(normal, lightDir)), minBias);

    float currentDepth = projCoords.z - bias;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

    for (int x = 0; x <= 2; ++x) {
        for (int y = 0; y <= 2; ++y) {
            vec3 shadowCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth);
            shadow += texture(u_ShadowMap, shadowCoord);
        }
    }
    shadow /= 4.0;
    return shadow;
}

// Soft shadows with organic falloff based on surface orientation
float CalculateSoftShadows(vec3 worldPos, vec3 normal, vec3 lightDir, vec3 geometricNormal) {
    // Self-shadowing: surfaces facing away from light are in shadow
    float NdotL = max(dot(normal, lightDir), 0.0);
    float selfShadow = smoothstep(0.0, 0.25, NdotL);

    // Ambient occlusion-like shadow based on surface concavity
    // Surfaces facing inward get more shadow
    float inwardFactor = 1.0 - max(geometricNormal.y, 0.0);
    float cavityShadow = 1.0 - inwardFactor * 0.4;

    // Combine shadow factors with organic blending
    float shadow = selfShadow * cavityShadow;

    // Add subtle penumbra effect
    shadow = mix(shadow, 1.0, 0.25);

    return pow(shadow, 2.0);
}

// Ambient occlusion based on surface orientation and cavities
float CalculateAmbientOcclusion(vec3 normal, vec3 geometricNormal) {
    // AO based on how much the surface faces away from the camera
    float NdotV = max(dot(normal, geometricNormal), 0.0);
    float ao = 1.0 - (1.0 - NdotV) * 0.4;

    // Add cavity effect - surfaces facing inward get more AO
    float cavity = 1.0 - max(0.0, dot(geometricNormal, vec3(0, 1, 0))) * 0.3;
    ao *= cavity;

    return clamp(ao, 0.2, 1.0);
}

// Ray-traced reflections approximation with environment mapping
vec3 CalculateReflections(vec3 worldPos, vec3 normal, vec3 viewDir, vec3 albedo,
                          float metallic, float roughness, float lodFactor) {
    // Skip reflections for low quality or low metallic surfaces
    if (lighting.u_QualityLevel == 0 || metallic < 0.25) return vec3(0.0);

    // Calculate reflection direction
    vec3 R = reflect(-viewDir, normal);

    // Simplified reflections at distance (skip parallax correction)
    vec3 cameraPos = lighting.u_CameraPosition.xyz;
    float distance = length(worldPos - cameraPos);

    // Only apply parallax correction for near objects
    if (lodFactor < 0.5) {
        vec3 toCamera = normalize(cameraPos - worldPos);
        vec3 parallaxR = reflect(-toCamera, normal);
        float NdotV = max(dot(normal, viewDir), 0.0);
        float parallaxBlend = pow(1.0 - NdotV, 2.0);
        R = mix(R, parallaxR, parallaxBlend * 0.3 * (1.0 - lodFactor));
    }

    // Environment mapping based on reflection direction
    float skyFactor = smoothstep(-0.1, 0.1, R.y);
    vec3 skyReflect = lighting.u_SkyColor.xyz;
    vec3 groundReflect = lighting.u_GroundColor.xyz;
    vec3 envColor = mix(groundReflect, skyReflect, skyFactor);

    // Add sun reflection (simplified at distance)
    vec3 sunDir = normalize(lighting.u_SunDirection.xyz);
    float sunReflect = max(dot(R, sunDir), 0.0);
    float sunSpecularPower = mix(256.0, 512.0, 1.0 - lodFactor) - roughness * 400.0;
    float sunSpecular = pow(sunReflect, sunSpecularPower);
    float sunIntensity = mix(1.5, 3.0, 1.0 - lodFactor);
    envColor += lighting.u_SunColor.xyz * sunSpecular * sunIntensity;

    // Roughness-based reflection blur
    float roughnessBlur = roughness * roughness;
    float sharpness = 1.0 - roughnessBlur;
    sharpness = pow(sharpness, 2.0);

    // Distance-based fade: reflections fade with distance
    float distanceFade = smoothstep(lighting.u_LODDistanceNear, lighting.u_LODDistanceFar, distance);
    distanceFade = 1.0 - distanceFade * 0.7;

    // Fresnel-based reflection intensity
    float NdotV = max(dot(normal, viewDir), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = FresnelSchlick(NdotV, F0);

    // Combine all factors
    float reflectionIntensity = sharpness * distanceFade * metallic;
    vec3 reflection = envColor * fresnel * reflectionIntensity;

    // Add albedo tint for metals
    vec3 tintedReflection = reflection * mix(vec3(1.0), albedo, metallic * 0.8);

    // Skip secondary bounce at distance
    if (lodFactor < 0.7) {
        vec3 secondaryR = reflect(R, normal * 0.5);
        float secondarySky = max(secondaryR.y, 0.0);
        vec3 secondaryEnv = mix(groundReflect, skyReflect, secondarySky);
        float secondaryIntensity = reflectionIntensity * 0.15 * (1.0 - roughness) * (1.0 - lodFactor);
        tintedReflection += secondaryEnv * secondaryIntensity;
    }

    return tintedReflection * 0.9;
}

// Global illumination approximation with light bounces
vec3 CalculateGI(vec3 normal, vec3 albedo, float roughness, vec3 lightColor, float lodFactor) {
    // Primary bounce - direct light reflected from environment
    vec3 bounceColor = albedo * lightColor * 0.5;
    bounceColor *= (1.0 + roughness * 0.5);

    // Add color bleeding from ground and sky with improved weighting
    float skyBounce = max(normal.y, 0.0);
    float groundBounce = max(-normal.y, 0.0);

    // Sky GI with atmospheric scattering approximation
    float skyIntensity = mix(0.15, 0.25, 1.0 - lodFactor);
    vec3 skyGI = lighting.u_SkyColor.xyz * albedo * skyBounce * skyIntensity;
    skyGI *= mix(vec3(1.0), vec3(0.8, 0.85, 1.0), skyBounce * 0.3 * (1.0 - lodFactor));

    // Ground GI with color bleeding from ground color
    float groundIntensity = mix(0.2, 0.35, 1.0 - lodFactor);
    vec3 groundGI = lighting.u_GroundColor.xyz * albedo * groundBounce * groundIntensity;
    groundGI *= mix(vec3(1.0), vec3(1.05, 0.95, 0.9), groundBounce * 0.2 * (1.0 - lodFactor));

    // Secondary bounce approximation - skip at distance
    vec3 secondaryBounce = vec3(0.0);

    if (lodFactor < 0.6) {
        // Approximate inter-reflection from sun
        vec3 sunDir = normalize(lighting.u_SunDirection.xyz);
        float sunBounce = max(dot(normal, sunDir), 0.0);
        vec3 sunGI = lighting.u_SunColor.xyz * albedo * sunBounce * 0.15 * (1.0 - roughness) * (1.0 - lodFactor);
        secondaryBounce += sunGI;

        // Add color bleeding from opposite surfaces
        vec3 oppositeColor = mix(lighting.u_GroundColor.xyz, lighting.u_SkyColor.xyz, skyBounce);
        float oppositeFactor = 0.1 * (1.0 - roughness) * (1.0 - lodFactor);
        secondaryBounce += oppositeColor * albedo * oppositeFactor;
    }

    // Tertiary bounce - only for near objects
    vec3 tertiaryBounce = vec3(0.0);
    if (lodFactor < 0.3) {
        tertiaryBounce = (skyGI + groundGI) * 0.05;
        tertiaryBounce *= (1.0 - roughness * 0.5);
    }

    // Combine all bounces with energy conservation
    vec3 totalGI = bounceColor + skyGI + groundGI + secondaryBounce + tertiaryBounce;

    // Apply occlusion based on surface orientation
    float occlusion = 1.0 - max(0.0, -normal.y) * 0.2;
    totalGI *= occlusion;

    return totalGI;
}

// Calculate PBR lighting for a single light
vec3 CalculatePBRLight(vec3 N, vec3 V, vec3 F0, PBRMaterial material, Light light) {
    vec3 L = normalize(light.direction);
    vec3 H = normalize(V + L);

    // Calculate radiance
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

    // Enhanced indirect specular with roughness-based falloff
    vec3 R = reflect(-V, N);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, material.roughness);

    // Better indirect specular approximation based on roughness
    float roughnessFactor = 1.0 - material.roughness * 0.7;
    vec3 indirectSpecular = F * roughnessFactor * ambientColor * 0.6;

    // Add color bleeding from environment
    vec3 envBleeding = ambientColor * material.albedo * 0.1 * (1.0 - material.roughness);
    ambient += indirectSpecular + envBleeding;

    return ambient;
}

// Triplanar mapping function
vec4 TriplanarMapping(vec3 worldPos, vec3 geometricNormal, int faceIndex) {
    // Calculate blend weights based on geometric normal
    vec3 blend = abs(geometricNormal);
    blend = pow(blend, vec3(triplanar.sharpness));
    float b = blend.x + blend.y + blend.z;
    blend /= vec3(b, b, b);

    // Apply blend mix to control blending strength
    blend *= triplanar.blendMix;

    // Calculate triplanar UV coordinates for each plane with offsets
    vec2 uvX = (worldPos.zy + vec2(triplanar.offsetX, triplanar.offsetY)) * triplanar.scale;
    vec2 uvY = (worldPos.xz + vec2(triplanar.offsetX, triplanar.offsetZ)) * triplanar.scale;
    vec2 uvZ = (worldPos.xy + vec2(triplanar.offsetX, triplanar.offsetY)) * triplanar.scale;

    // Sample texture from each plane using triplanar
    vec4 colX = texture(u_Texture[faceIndex], uvX);
    vec4 colY = texture(u_Texture[faceIndex], uvY);
    vec4 colZ = texture(u_Texture[faceIndex], uvZ);

    // Blend the three triplanar samples
    vec4 triplanarResult = colX * blend.x + colY * blend.y + colZ * blend.z;

    // Also sample with original UVs
    vec4 originalUVResult = texture(u_Texture[faceIndex], v_TexCoords);

    // Blend between original UV and triplanar based on how "curved" the surface is
    // Use the deviation from axis-aligned normals as a proxy for curvature
    float curvatureFactor = 1.0 - max(max(abs(geometricNormal.x), abs(geometricNormal.y)), abs(geometricNormal.z));
    curvatureFactor = pow(curvatureFactor, 2.0); // Make it more pronounced

    // Blend: use original UVs on flat surfaces, triplanar on curved surfaces
    vec4 result = mix(originalUVResult, triplanarResult, curvatureFactor);

    return result;
}

// Triplanar mapping for single-channel textures like AO
float TriplanarMappingSingle(vec3 worldPos, vec3 geometricNormal, sampler2D tex, int faceIndex) {
    // Calculate blend weights based on geometric normal
    vec3 blend = abs(geometricNormal);
    blend = pow(blend, vec3(triplanar.sharpness));
    float b = blend.x + blend.y + blend.z;
    blend /= vec3(b, b, b);

    // Apply blend mix to control blending strength
    blend *= triplanar.blendMix;

    // Calculate triplanar UV coordinates for each plane with offsets
    vec2 uvX = (worldPos.zy + vec2(triplanar.offsetX, triplanar.offsetY)) * triplanar.scale;
    vec2 uvY = (worldPos.xz + vec2(triplanar.offsetX, triplanar.offsetZ)) * triplanar.scale;
    vec2 uvZ = (worldPos.xy + vec2(triplanar.offsetX, triplanar.offsetY)) * triplanar.scale;

    // Sample texture from each plane using triplanar
    float colX = texture(tex, uvX).r;
    float colY = texture(tex, uvY).r;
    float colZ = texture(tex, uvZ).r;

    // Blend the three triplanar samples
    float triplanarResult = colX * blend.x + colY * blend.y + colZ * blend.z;

    // Also sample with original UVs
    float originalUVResult = texture(tex, v_TexCoords).r;

    // Blend between original UV and triplanar based on how "curved" the surface is
    float curvatureFactor = 1.0 - max(max(abs(geometricNormal.x), abs(geometricNormal.y)), abs(geometricNormal.z));
    curvatureFactor = pow(curvatureFactor, 2.0);

    // Blend: use original UVs on flat surfaces, triplanar on curved surfaces
    float result = mix(originalUVResult, triplanarResult, curvatureFactor);

    return result;
}

// Main PBR calculation with raytracing-like effects
vec3 CalculatePBR(vec3 worldPos, vec3 normal, vec3 viewDir, PBRMaterial material,
                  vec3 ambientColor) {
    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);

    // Calculate LOD factor based on distance and quality level
    float distance = length(worldPos - lighting.u_CameraPosition.xyz);
    float lodFactor = smoothstep(lighting.u_LODDistanceNear, lighting.u_LODDistanceFar, distance);
    lodFactor = clamp(lodFactor, 0.0, 1.0);

    // Override LOD factor based on quality level
    if (lighting.u_QualityLevel == 0) {
        lodFactor = 1.0; // Always low quality
    } else if (lighting.u_QualityLevel == 2) {
        lodFactor *= 0.5; // Always high quality
    }

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);

    // Calculate ambient lighting using spherical harmonics
    vec3 shAmbient = CalculateSHAmbient(N, lighting.u_SHCoefficients);
    vec3 ambient = CalculateAmbient(N, V, F0, material, shAmbient);

    // Accumulate lighting from all sources
    vec3 Lo = vec3(0.0);

    // Primary sun light
    vec3 L = normalize(-lighting.u_SunDirection.xyz);
    vec3 H = normalize(V + L);

    vec3 radiance = lighting.u_SunColor.xyz * lighting.u_SunIntensity;
    float NDF = DistributionGGX(N, H, material.roughness);
    float G = GeometrySmith(N, V, L, material.roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    vec3 specular = numerator / denominator;

    // Add a bit of lighting to the other faces
    float NdotL = max(dot(N, L), 0.0);
    vec3 sunLightDirect = (kD * material.albedo / PI + specular) * radiance * NdotL;
    float shadowMap = CalculateShadow(worldPos, L, N);

    // Get geometric normal for shadow calculations
    vec3 geoN = normalize(v_GeometricNormal);

    float softShadow = CalculateSoftShadows(worldPos, N, L, geoN);
    float totalShadow = shadowMap * softShadow;
    vec3 directLighting = sunLightDirect * totalShadow;

    // Additional lights
    for (uint i = 0u; i < lighting.u_AdditionalLightCount && i < 4u; ++i) {
        vec3 lightDir = normalize(-lighting.u_AdditionalLights[i].direction);
        vec3 lightH = normalize(V + lightDir);

        vec3 lightRadiance = lighting.u_AdditionalLights[i].color * lighting.u_AdditionalLights[i].intensity;
        float lightNDF = DistributionGGX(N, lightH, material.roughness);
        float lightG = GeometrySmith(N, V, lightDir, material.roughness);
        vec3 lightF = FresnelSchlick(max(dot(lightH, V), 0.0), F0);

        vec3 lightKS = lightF;
        vec3 lightKD = vec3(1.0) - lightKS;
        lightKD *= 1.0 - material.metallic;

        vec3 lightNumerator = lightNDF * lightG * lightF;
        float lightDenominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, lightDir), 0.0) + EPSILON;
        vec3 lightSpecular = lightNumerator / lightDenominator;

        float lightNdotL = max(dot(N, lightDir), 0.0);
        Lo += (lightKD * material.albedo / PI + lightSpecular) * lightRadiance * lightNdotL;
    }
    // Ambient occlusion
    float ao = CalculateAmbientOcclusion(N, geoN);

    vec3 gi = CalculateGI(N, material.albedo, material.roughness, lighting.u_SunColor.xyz, lodFactor);

    vec3 indirectLighting = (ambient + gi) * ao;

    // Ray-traced reflections for metallic surfaces
    vec3 reflections = CalculateReflections(worldPos, N, V, material.albedo, material.metallic, material.roughness, lodFactor);

    vec3 colorFinal = indirectLighting + directLighting + Lo + reflections;

    // Enhanced rim lighting with geometric normal for better edge definition
    // Simplified rim lighting at distance
    if (lodFactor < 0.8) {
        float NdotV = max(dot(geoN, V), 0.0);
        vec3 rimFresnel = FresnelSchlick(NdotV, F0);

        // Multi-layered rim lighting for more realistic edge glow
        float baseRim = clamp(1.0 - NdotV, 0.0, 1.0);

        // Inner rim - softer, more diffuse
        float innerRim = smoothstep(0.3, 0.8, baseRim);
        innerRim = pow(innerRim, 2.0);

        // sharper, more intense (skip at distance)
        float outerRim = 0.0;
        if (lodFactor < 0.4) {
            outerRim = smoothstep(0.6, 1.0, baseRim);
            outerRim = outerRim * outerRim * outerRim * outerRim;
        }

        // Combine rim layers
        float combinedRim = innerRim * 0.4 + outerRim * 0.05;

        // Sun alignment for directional rim flash
        float sunViewAlignment = max(dot(L, V), 0.0);
        float flashIntensity = sunViewAlignment * sunViewAlignment * sunViewAlignment;

        // Roughness affects rim intensity - smoother surfaces have stronger rims
        float roughnessAttenuation = 1.0 - material.roughness * 0.6;
        float cornerFlash = combinedRim * mix(0.3, 1.0, flashIntensity) * roughnessAttenuation;

        // Colored rim light based on material and sun
        vec3 rimColor = mix(lighting.u_SunColor.xyz, material.albedo, 0.3);
        float rimIntensity = mix(0.3, 0.7, 1.0 - lodFactor);
        vec3 rimLight = cornerFlash * rimFresnel * rimColor * (lighting.u_SunIntensity * rimIntensity);
        colorFinal += rimLight;
    }

    // Add subtle subsurface scattering approximation for non-metallic materials
    // Skip at distance
    if (material.metallic < 0.25 && lodFactor < 0.7) {
        float sssThickness = 1.0 - material.roughness;
        float backLight = max(dot(-L, N), 0.0);
        float sssIntensity = mix(0.05, 0.15, 1.0 - lodFactor);
        vec3 sssColor = material.albedo * lighting.u_SunColor.xyz * backLight * sssThickness * sssIntensity;
        colorFinal += sssColor;
    }

    // Add microfacet detail for rough surfaces (skip at distance)
    if (material.roughness > 0.3 && lodFactor < 0.5) {
        float microDetail = material.roughness * 0.025 * (1.0 - lodFactor);
        vec3 microColor = material.albedo * shAmbient * microDetail;
        colorFinal += microColor;
    }

    return colorFinal;
}

void main() {
    // Use geometric (flat) normal for triplanar mapping blend calculations
    vec3 geoNormal = normalize(v_GeometricNormal);

    // Normal maps - use original UV-based sampling (normal maps are in tangent space)
    vec3 normalMapSample = texture(u_NormalTexture, v_TexCoords).rgb;
    vec3 tangentNormal = normalMapSample * 2.0 - 1.0;
    // Better normal reconstruction with proper scaling
    tangentNormal = normalize(tangentNormal);
    vec3 normal = normalize(v_TBN * tangentNormal);

    // Use triplanar mapping for texture sampling with geometric normal
    vec4 texColor = TriplanarMapping(v_WorldPos, geoNormal, int(v_FaceIndex));

    vec3 albedoLinear = pow(texColor.rgb, vec3(2.2));

    // Apply transparency
    float transparency = float(v_TransparencyLevel) / 255.0;
    texColor.a = mix(texColor.a, 1.0 - transparency, transparency);

    // Setup PBR material with texture albedo
    PBRMaterial material;
    material.albedo = albedoLinear;
    material.metallic = v_Metallic;
    material.roughness = v_Roughness;

    // Sample ambient occlusion using triplanar mapping with geometric normal
    float ao = TriplanarMappingSingle(v_WorldPos, geoNormal, u_AOTexture, int(v_FaceIndex));
    material.ao = ao;

    // Calculate view direction
    vec3 viewDir = normalize(lighting.u_CameraPosition.xyz - v_WorldPos);

    // Calculate PBR lighting (uses spherical harmonics ambient internally)
    vec3 pbrColor = CalculatePBR(v_WorldPos, normal, viewDir, material, vec3(0.0));

    // Calculate self-emission with proper HDR handling
    float emission = float(v_LightingEmit) / 255.0;
    // Emission should be additive and not affected by AO
    vec3 emitColor = emission * material.albedo * lighting.u_SunColor.xyz * 4.0;

    // Add bloom-like glow for emissive materials
    if (emission > 0.15) {
        float glowIntensity = emission * 2.0;
        emitColor += material.albedo * glowIntensity * 0.5;
    }

    vec3 finalColor = pbrColor + emitColor;

    // Enhanced tone mapping with exposure
    float exposure = lighting.u_Exposure > 0.0 ? lighting.u_Exposure : 1.0;
    vec3 mappedColor = EnhancedToneMapping(finalColor, exposure);

    // Gamma correction with slight desaturation for more realistic look
    mappedColor = pow(mappedColor, vec3(1.0 / 2.2));

    // Subtle color grading - slightly warm shadows, cool highlights
    float luminance = dot(mappedColor, vec3(0.299, 0.587, 0.114));
    mappedColor = mix(mappedColor, mappedColor * vec3(1.05, 0.98, 0.95), 1.0 - luminance * 0.5);

    // Output final color with texture alpha
    outColor = vec4(mappedColor, texColor.a);
}