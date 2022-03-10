#version 330
#extension GL_ARB_explicit_attrib_location : require

out vec4 fragColor;
in vec2 v_texCoords;

uniform sampler2D u_screenTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_depthTexture;

uniform float u_width;
uniform float u_height;

float kernel[9] = float[]
(
    1,  1, 1,
    1, -8, 1,
    1,  1, 1
);

void main()
{
    float offset_x = 1.0f / u_width;  
    float offset_y = 1.0f / u_height;  

    vec2 offsets[9] = vec2[]
    (
    vec2(-offset_x,  offset_y), vec2( 0.0f,    offset_y), vec2( offset_x,  offset_y),
    vec2(-offset_x,  0.0f),     vec2( 0.0f,    0.0f),     vec2( offset_x,  0.0f),
    vec2(-offset_x, -offset_y), vec2( 0.0f,   -offset_y), vec2( offset_x, -offset_y) 
    );

    vec3 color = vec3(0.0f);
    for(int i = 0; i < 9; i++)
        color += vec3(texture(u_screenTexture, v_texCoords.st + offsets[i])) * kernel[i];

    fragColor = vec4(color, 1.0f);
    //fragColor = vec4(texture(u_screenTexture, v_texCoords).rgb, 1.0f);
    //fragColor = vec4(texture(u_normalTexture, v_texCoords).rgb, 1.0f);
    fragColor = vec4(texture(u_screenTexture, v_texCoords).rgb, 1.0f);
}