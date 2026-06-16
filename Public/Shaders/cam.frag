#version 450

layout(push_constants) uniform UniformBufferObject {
    mat4 mvp
} view;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 0) out vec3 FragColor;

void main() {
    vec4 clipPos = view.mvp * vec4(iPosition, 1.0);
    gl_Position = clipPos;
    FragColor = iColor;
}