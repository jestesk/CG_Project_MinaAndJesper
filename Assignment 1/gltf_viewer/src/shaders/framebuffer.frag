#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform sampler2D screenTexture;
// ...

in vec2 textureCoordinate;
// ...

// Fragment shader outputs
out vec4 frag_color;

const float offset = 1.0 / 300.0;

vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right
    );

    float kernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );
    

void main()
{
    
    vec3 sampleTex[9];
       for(int i = 0; i < 9; i++)
       {
           sampleTex[i] = vec3(texture(screenTexture, textureCoordinate.st + offsets[i]));
       }
       vec3 col = vec3(0.0);
       for(int i = 0; i < 9; i++)
           col += sampleTex[i] * kernel[i];
       
    frag_color = vec4(col, 1.0);

  
}
