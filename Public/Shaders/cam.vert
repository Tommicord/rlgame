#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 0) out vec3 FragColor;

void main() {
    vec4 clipPos = ubo.proj * ubo.view * ubo.model * vec4(iPosition, 1.0);
    gl_Position = vec4(clipPos.x, -clipPos.y, clipPos.z, clipPos.w);
    FragColor = iColor;
}