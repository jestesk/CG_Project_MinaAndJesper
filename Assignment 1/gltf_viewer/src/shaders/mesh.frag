#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform sampler2D u_texture;
// ...

// Fragment shader inputs
in vec3 N;
in vec3 V;
in vec3 L;
in float enableTexture;
in vec2 textureCoordinate;
// ...

// Fragment shader outputs
out vec4 frag_color;

const float specularPower = 40.0f;
const vec3 specularColor = vec3(0.04);
const float gamma = 2.2f;

void main()
{
    float diffuse = max(0.0, dot(N, L));
    float specular = pow(max(dot(L, N), 0.0), specularPower);
    vec3 R = reflect(-V, N);
    vec3 diffuseColor = vec3(0.0f, 1.0f, 0.0f);
    if(enableTexture >= 1.0f){
        diffuseColor = texture(u_texture, textureCoordinate).xyz;
    }
    vec3 ambientColor = 0.01 * diffuseColor;
    vec3 color = diffuse * diffuseColor + ambientColor + (specularPower + 8)/8 * specular * specularColor;
    vec3 colorGammaCorrected = pow(color, vec3(1.0 / gamma));
    frag_color = vec4(colorGammaCorrected, 1.0f);
  
}
