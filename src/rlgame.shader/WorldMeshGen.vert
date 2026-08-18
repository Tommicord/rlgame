#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants
{
    layout(column_major) mat4 mvp;
};

void main()
{
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = mvp * worldPos;

    fragNormal = inNormal;
    fragUV = inUV;
    fragWorldPos = worldPos.xyz;
}
