#version 450

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec4 tcPosition[];
layout(location = 1) in vec3 tcNormal[];
layout(location = 2) in vec2 tcTexCoord[];

layout(location = 0) out vec3 teNormal;
layout(location = 1) out vec2 teTexCoord;
layout(location = 2) out vec3 teWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat3 normalMatrix;
} pc;

void main() {
    // Interpolate using barycentric coordinates
    vec4 worldPos = tcPosition[0] * gl_TessCoord.x + 
                    tcPosition[1] * gl_TessCoord.y + 
                    tcPosition[2] * gl_TessCoord.z;
    vec3 normal = tcNormal[0] * gl_TessCoord.x + 
                  tcNormal[1] * gl_TessCoord.y + 
                  tcNormal[2] * gl_TessCoord.z;
    vec2 texCoord = tcTexCoord[0] * gl_TessCoord.x + 
                    tcTexCoord[1] * gl_TessCoord.y + 
                    tcTexCoord[2] * gl_TessCoord.z;
    
    vec4 viewPos = pc.view * pc.model * worldPos;
    
    teNormal = pc.normalMatrix * normal;
    teTexCoord = texCoord;
    teWorldPos = worldPos.xyz;
    
    gl_Position = pc.proj * viewPos;
}