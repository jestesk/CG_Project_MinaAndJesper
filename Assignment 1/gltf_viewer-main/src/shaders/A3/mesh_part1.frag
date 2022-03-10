#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform bool u_useGammaCompensation;
uniform int u_cubemap;
// ...

// Fragment shader inputs
in vec3 v_NormalVec;
in vec3 v_LightVec;
in vec3 v_ViewVec;

in vec3 v_diffuseColor;
in vec3 v_ambientColor;
in vec3 v_specularColor;

in float v_specularPower;
// ...

// Fragment shader outputs
out vec4 frag_color;

float specular_normalized(vec3 N, vec3 H, float specular_power)
{
    float normalization = (8.0 + specular_power) / 8.0;
    return normalization * pow(max(0.0, dot(N,H)), specular_power);
}

vec3 linear_to_gamma(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 gamma_to_linear(vec3 color)
{
    return pow(color, vec3(2.2));
}

void main()
{
    vec3 V = normalize(v_ViewVec);
    vec3 L = normalize(v_LightVec);
    vec3 N = normalize(v_NormalVec);
    vec3 H = normalize(L + V);

    vec3 bpShading = v_ambientColor + v_diffuseColor * max(0.0, dot(N,L)) + v_specularColor * specular_normalized(N, H, v_specularPower);

    if(u_useGammaCompensation)
    {
        bpShading = linear_to_gamma(bpShading);
    }

    frag_color = vec4(bpShading, 1);
}
