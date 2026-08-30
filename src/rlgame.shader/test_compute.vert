#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat3 normalMatrix;
} pc;

void main() {
    vec4 worldPos = pc.model * inPosition;
    outNormal = pc.normalMatrix * inNormal;
    outTexCoord = inTexCoord;
    outWorldPos = worldPos.xyz;
    gl_Position = pc.proj * pc.view * worldPos;
}