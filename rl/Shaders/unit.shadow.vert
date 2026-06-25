#version 450

layout (location = 0) in vec4 a_Position;
layout (push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
} pc;

void main() {
    vec3 localPos = a_Position.xyz;
    gl_Position = pc.lightSpaceMatrix * vec4(localPos, 1.0);
}
