#version 330
#extension GL_ARB_explicit_attrib_location : require


// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoord_0;
// ...

out vec2 textureCoordinate;
// ...

void main()
{
   
    gl_Position = vec4(a_position.x, a_position.y, 0.0f, 1.0f);
    textureCoordinate = a_texcoord_0; 
     
}
