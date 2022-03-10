#version 330
#extension GL_ARB_explicit_attrib_location : require

layout (location = 0) in vec2 a_vertexPos;
layout (location = 1) in vec2 a_inTexCoords;

out vec2 v_texCoords;

void main()
{
    gl_Position = vec4(a_vertexPos.x, a_vertexPos.y, 0.0, 1.0); 
    v_texCoords = a_inTexCoords;
}  