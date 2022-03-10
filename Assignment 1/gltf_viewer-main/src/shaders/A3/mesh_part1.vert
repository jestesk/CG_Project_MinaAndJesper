#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;

uniform bool u_isNormalsRGB;

//Lambertian
uniform vec3 u_diffuseColor; // The diffuse surface color of the model
uniform vec3 u_lightPosition; // The position of your light source

//BP Light
uniform vec3 u_ambientColor, u_specularColor;
uniform float u_specularPower;
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
// ...

// Vertex shader outputs
out vec3 v_NormalVec;
out vec3 v_LightVec;
out vec3 v_ViewVec;

out vec3 v_diffuseColor;
out vec3 v_ambientColor;
out vec3 v_specularColor;

out float v_specularPower;
// ...

void main()
{
    gl_Position = u_projection * u_view * u_model * a_position;

    mat4 mv = u_view * u_model;
    // Transform the vertex position to view space (eye coordinates)
    vec3 positionEye = vec3(mv * a_position);
    
    // Calculate the view-space normal
    vec3 N = normalize(mat3(mv) * a_normal);

    // Calculate the view-space light direction
    vec3 L = normalize(u_lightPosition - positionEye);
    vec3 V = normalize(-positionEye);

    v_NormalVec = N;
    v_LightVec = L;
    v_ViewVec = V;

    v_specularPower = u_specularPower;
    v_ambientColor = u_ambientColor;
    v_diffuseColor = u_diffuseColor;
    v_specularColor = u_specularColor;
}
