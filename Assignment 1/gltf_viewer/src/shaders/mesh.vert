  #version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_projection;
uniform float u_enableTexture;
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord_0;
// ...

// Vertex shader outputs
//the view-space normal
out vec3 N;
//view-space light vector
out vec3 L;
// view vector
out vec3 V;
// diffiuseColor
out vec2 textureCoordinate;

out float enableTexture;
// ...

void main()
{
    mat4 mvp = u_projection * u_model * u_view;
    
    mat4 mv = u_model * u_view;
    // Transform the vertex position to view space (eye coordinates)
    vec3 positionEye = vec3(mv * a_position);
    V = - positionEye;
    // Calculate the view-space normal
    N = normalize(mat3(mv) * a_normal);
    // Calculate the view-space light direction
    vec3 u_lightPosition = vec3(0.0f);
    L = normalize(u_lightPosition - positionEye);
    enableTexture = u_enableTexture;
    textureCoordinate = a_texcoord_0;
    gl_Position = mvp * a_position;
    
}
