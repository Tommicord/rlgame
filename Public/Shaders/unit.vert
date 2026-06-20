#version 450

// Projection-View-Model matrix
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pc;

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoords;

// The total lightning emit
// TODO: Implement the Sun light emit when possible
layout (location = 2) in uint a_LightingEmit;

// 255 = Max transparency level, 0 = No transparency
layout (location = 3) in uint a_TransparencyLevel;

// Face index: 0 = Top, 1 = Bottom, 2 = Left, 3 = Right, 4 = Front, 5 = Back
layout (location = 4) in uint a_FaceIndex;

// Polygon fence offsets for non-cube geometry
// Right side polygons (Tr, Dr, Br, Fr)
layout (location = 5) in vec4 a_PolRight;
// Left side polygons (Tl, Dl, Bl, Fl)
layout (location = 6) in vec4 a_PolLeft;

// This is used for calculate the render distance beetwen the player
// and the Unit to render in the correct position the unit
// in the camera
layout(set = 0, binding = 0) uniform CoordinatesBlock {
    vec3 u_UnitCoord;
    vec3 u_PlayerCoord;
} coords;


// The texture of the Unit
// The unit has 6 faces
layout (binding = 2) uniform sampler2D u_Texture[6];

// Output to geometry shader
layout (location = 0) out vec3 v_WorldPos;
layout (location = 1) out vec2 v_TexCoords;
layout (location = 2) out uint v_LightingEmit;
layout (location = 3) out uint v_TransparencyLevel;
layout (location = 4) out uint v_FaceIndex;
layout (location = 5) out vec3 v_Albedo;
layout (location = 6) out float v_Metallic;
layout (location = 7) out float v_Roughness;

// Apply polygon fence offset based on face index
vec3 ApplyPolygonOffset(vec3 position, uint faceIndex, vec4 polRight, vec4 polLeft) {
    vec3 offset = vec3(0.0);
    
    // Polygon fence values: t, d, b, f (top, down, back, front)
    // Right side: polTr, polDr, polBr, polFr
    // Left side: polTl, polDl, polBl, polFl
    switch(faceIndex) {
        case 0: // Top face
            offset.y = polRight.x; // Tr
            break;
        case 1: // Bottom face
            offset.y = polRight.y; // Dr
            break;
        case 2: // Left face
            offset.x = polLeft.z; // Bl
            break;
        case 3: // Right face
            offset.x = polRight.z; // Br
            break;
        case 4: // Front face
            offset.z = polRight.w; // Fr
            break;
        case 5: // Back face
            offset.z = polLeft.w; // Fl
            break;
    }
    return position + offset;
}

void main() {
    // Apply polygon geometry offsets for non-cube shapes
    vec3 modifiedPosition = ApplyPolygonOffset(a_Position, a_FaceIndex, a_PolRight, a_PolLeft);
    
    // Calculate world position
    vec4 worldPos = pc.model * vec4(modifiedPosition, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // Pass texture coordinates
    v_TexCoords = a_TexCoords;
    
    // Pass lighting and transparency
    v_LightingEmit = a_LightingEmit;
    v_TransparencyLevel = a_TransparencyLevel;
    
    // Pass face index for texture selection
    v_FaceIndex = a_FaceIndex;
    
    // Pass PBR material properties
    v_Albedo = vec3(1.0, 1.0, 1.0); // Default white albedo
    v_Metallic = 0.0; // Default non-metallic
    v_Roughness = 0.5; // Default medium roughness
    
    // Calculate final position
    gl_Position = pc.projection * pc.view * worldPos;
}