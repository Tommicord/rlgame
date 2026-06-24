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
layout (location = 8) in mat3 v_TBN;
layout (location = 11) smooth in vec3 v_GeometricNormal;

// Output color
layout (location = 0) out vec4 outColor;

// Lighting uniforms
layout(set = 0, binding = 4) uniform LightingBlock {
    vec3 u_SunDirection;
    vec3 u_SunColor;
    float u_AmbientStrength;
    vec3 u_CameraPosition;
    float u_Exposure;
    float u_Padding1;  // Padding for 16-byte alignment
    float u_Padding2;  // Padding for 16-byte alignment
    float u_Padding3;  // Padding for 16-byte alignment
    vec3 u_GroundColor;
    vec3 u_SkyColor;
} lighting;

// Ambient occlusion texture
layout (binding = 10) uniform sampler2D u_AOTexture;
layout (binding = 11) uniform sampler2D u_NormalTexture;

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

// Ray-marched soft shadows
float CalculateSoftShadows(vec3 worldPos, vec3 lightDir, float maxDist, int steps) {
    float shadow = 1.0;
    float stepSize = maxDist / float(steps);
    
    // Simple ray marching with limited steps for performance
    for (int i = 1; i <= steps; i++) {
        vec3 samplePos = worldPos + lightDir * (stepSize * float(i));
        
        // Approximate occlusion using distance from origin (cube centered at 0,0,0)
        float distFromCenter = length(samplePos);
        float occlusion = smoothstep(0.5, 0.7, distFromCenter);
        
        shadow = min(shadow, occlusion);
        
        // Early exit if fully occluded
        if (shadow < 0.1) break;
    }
    
    return shadow;
}

// Screen-space ambient occlusion approximation
float CalculateSSAO(vec3 normal, vec3 viewDir, float roughness) {
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    // Simple AO based on viewing angle and roughness
    float ao = 1.0 - (1.0 - NdotV) * 0.3 * (1.0 - roughness * 0.3);
    
    // Add cavity approximation
    float cavity = pow(NdotV, 0.7);
    ao = mix(ao, cavity, 0.2);
    
    return clamp(ao, 0.5, 1.0);
}

// Ray-marched reflections for metallic surfaces (simplified)
vec3 CalculateReflections(vec3 worldPos, vec3 normal, vec3 viewDir, vec3 albedo, 
                           float metallic, float roughness, vec3 ambientColor) {
    if (metallic < 0.3) return vec3(0.0);
    
    // Calculate reflection direction
    vec3 R = reflect(-viewDir, normal);
    
    // Simple environment approximation based on reflection direction
    float skyFactor = max(R.y, 0.0);
    vec3 envColor = mix(lighting.u_GroundColor, lighting.u_SkyColor, skyFactor);
    
    // Roughness-based reflection blur approximation
    float roughnessFactor = 1.0 - roughness * 0.8;
    vec3 reflection = envColor * roughnessFactor * metallic;
    
    // Add albedo tint for metals
    reflection *= mix(vec3(1.0), albedo, metallic);
    
    return reflection * 0.4;
}

// Global illumination approximation with light bounces
vec3 CalculateGI(vec3 normal, vec3 albedo, float roughness, vec3 lightColor) {
    // First bounce approximation
    vec3 bounceColor = albedo * lightColor * 0.15;
    
    // Roughness affects bounce intensity
    bounceColor *= (1.0 - roughness * 0.5);
    
    // Add color bleeding
    vec3 colorBleed = albedo * lighting.u_GroundColor * 0.1;
    
    return bounceColor + colorBleed;
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

    // Ray-marched soft shadows (performance-optimized): reduced intensity
    float softShadow = CalculateSoftShadows(worldPos, L, 3.0, 4);
    colorFinal *= mix(1.0, softShadow, 0.5); // Blend to soften effect

    // Screen-space ambient occlusion approximation - more subtle
    float ssao = CalculateSSAO(N, V, material.roughness);
    colorFinal = mix(colorFinal * 0.7, colorFinal, ssao);

    // Ray-marched reflections for metallic surfaces: reduced intensity
    vec3 reflections = CalculateReflections(worldPos, N, V, material.albedo, 
                                           material.metallic, material.roughness, ambientColor);
    colorFinal += reflections * 0.5;

    // Global illumination approximation: reduced intensity
    vec3 gi = CalculateGI(N, material.albedo, material.roughness, sunLight.color);
    colorFinal += gi * 0.5;

    // Enhanced rim lighting with geometric normal for better edge definition
    vec3 geoN = normalize(v_GeometricNormal);
    float NdotV = max(dot(geoN, V), 0.0);
    vec3 rimFresnel = FresnelSchlick(NdotV, F0);

    // Multi-layered rim lighting for more realistic edge glow
    float baseRim = clamp(1.0 - NdotV, 0.0, 1.0);
    
    // Inner rim - softer, more diffuse
    float innerRim = smoothstep(0.3, 0.8, baseRim);
    innerRim = pow(innerRim, 2.0);
    
    // Outer rim - sharper, more intense
    float outerRim = smoothstep(0.6, 1.0, baseRim);
    outerRim = pow(outerRim, 4.0);
    
    // Combine rim layers
    float combinedRim = innerRim * 0.4 + outerRim * 0.6;
    
    // Sun alignment for directional rim flash
    float sunViewAlignment = max(dot(L, V), 0.0);
    float flashIntensity = pow(sunViewAlignment, 3.0);
    
    // Roughness affects rim intensity - smoother surfaces have stronger rims
    float roughnessAttenuation = 1.0 - material.roughness * 0.6;
    float cornerFlash = combinedRim * mix(0.3, 1.0, flashIntensity) * roughnessAttenuation;
    
    // Colored rim light based on material and sun
    vec3 rimColor = mix(sunLight.color, material.albedo, 0.3);
    vec3 rimLight = cornerFlash * rimFresnel * rimColor * (sunLight.intensity * 0.7);
    colorFinal += rimLight;

    // Add subtle subsurface scattering approximation for non-metallic materials
    if (material.metallic < 0.5) {
        float sssThickness = 1.0 - material.roughness;
        float backLight = max(dot(-L, N), 0.0);
        vec3 sssColor = material.albedo * sunLight.color * backLight * sssThickness * 0.15;
        colorFinal += sssColor;
    }

    // Add microfacet detail for rough surfaces
    if (material.roughness > 0.3) {
        float microDetail = material.roughness * 0.1;
        vec3 microColor = material.albedo * ambientColor * microDetail;
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
    vec3 viewDir = normalize(lighting.u_CameraPosition - v_WorldPos);

    // Setup sun light
    Light sunLight;
    sunLight.position = lighting.u_SunDirection; // Directional light direction
    sunLight.color = lighting.u_SunColor;
    sunLight.intensity = 12.25;

    // Calculate hemispheric ambient lighting
    vec3 hemisphericAmbient = CalculateHemisphericAmbient(
        normal,
        lighting.u_GroundColor,
        lighting.u_SkyColor,
        lighting.u_AmbientStrength
    );

    // Calculate PBR lighting
    vec3 pbrColor = CalculatePBR(v_WorldPos, normal, viewDir, material, sunLight, hemisphericAmbient);

    // Calculate self-emission with proper HDR handling
    float emission = float(v_LightingEmit) / 255.0;
    // Emission should be additive and not affected by AO
    vec3 emitColor = emission * material.albedo * lighting.u_SunColor * 4.0;
    
    // Add bloom-like glow for emissive materials
    if (emission > 0.1) {
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