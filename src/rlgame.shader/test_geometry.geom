#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 fragNormal[];
layout(location = 1) in vec2 fragTexCoord[];
layout(location = 2) in vec3 fragWorldPos[];
layout(location = 3) in vec3 fragViewPos[];

layout(location = 0) out vec3 geoNormal;
layout(location = 1) out vec2 geoTexCoord;
layout(location = 2) out vec3 geoWorldPos;
layout(location = 3) out vec3 geoViewPos;

void main() {
    for (int i = 0; i < 3; i++) {
        geoNormal = fragNormal[i];
        geoTexCoord = fragTexCoord[i];
        geoWorldPos = fragWorldPos[i];
        geoViewPos = fragViewPos[i];
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}