#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragViewPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat3 normalMatrix;
} pc;

void main() {
    vec4 worldPos = pc.model * inPosition;
    vec4 viewPos = pc.view * worldPos;
    
    fragNormal = pc.normalMatrix * inNormal;
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPos.xyz;
    fragViewPos = viewPos.xyz;
    
    gl_Position = pc.proj * viewPos;
}