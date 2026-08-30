#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragViewPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 0, binding = 3) uniform sampler2D aoMap;

layout(push_constant) uniform PushConstants {
    vec3 lightPos;
    vec3 lightColor;
    float lightIntensity;
    vec3 cameraPos;
    vec3 ambientColor;
    float ambientIntensity;
} pc;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(pc.cameraPos - fragWorldPos);
    vec3 L = normalize(pc.lightPos - fragWorldPos);
    
    vec4 albedo = texture(albedoMap, fragTexCoord);
    vec4 normalMapSample = texture(normalMap, fragTexCoord);
    vec4 metallicRoughness = texture(metallicRoughnessMap, fragTexCoord);
    float ao = texture(aoMap, fragTexCoord).r;
    
    // Unpack normal from normal map
    vec3 tangentNormal = normalize(normalMapSample.xyz * 2.0 - 1.0);
    
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    
    // Lambertian diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = albedo.rgb * pc.lightColor * NdotL * pc.lightIntensity;
    
    // Specular (GGX)
    vec3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    float D = a2 / (3.14159265359 * denom * denom);
    
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;
    
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    
    vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 0.0001);
    
    vec3 radiance = (diffuse + specular) * ao;
    vec3 ambient = pc.ambientColor * albedo.rgb * pc.ambientIntensity;
    
    outColor = vec4(radiance + ambient, albedo.a);
    outColor = vec4(pow(outColor.rgb, vec3(1.0/2.2)), outColor.a); // Gamma correction
}