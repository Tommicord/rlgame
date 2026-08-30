#version 450

layout(vertices = 3) out;

layout(location = 0) in vec4 inPosition[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];

layout(location = 0) out vec4 tcPosition[];
layout(location = 1) out vec3 tcNormal[];
layout(location = 2) out vec2 tcTexCoord[];

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat3 normalMatrix;
    float tessLevelInner;
    float tessLevelOuter;
} pc;

void main() {
    tcPosition[gl_InvocationID] = inPosition[gl_InvocationID];
    tcNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    tcTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
    
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = pc.tessLevelInner;
        gl_TessLevelOuter[0] = pc.tessLevelOuter;
        gl_TessLevelOuter[1] = pc.tessLevelOuter;
        gl_TessLevelOuter[2] = pc.tessLevelOuter;
    }
    
    barrier();
}