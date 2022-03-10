#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_time;
// ...

// Fragment shader inputs
in vec4 v_color;
// ...

// Fragment shader outputs
out vec4 frag_color;

void main()
{
    frag_color = v_color + cos(vec4(1,1,1,1)*u_time);
}
