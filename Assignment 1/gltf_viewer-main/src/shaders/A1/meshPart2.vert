#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
// ...

// Vertex shader outputs
out vec4 v_color;
// ...

void main()
{
	v_color = a_color;
    gl_Position = a_position;
}
