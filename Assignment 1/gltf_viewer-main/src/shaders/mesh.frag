#version 330
#extension GL_ARB_explicit_attrib_location : require

const int maxCelLevels = 3;

// Uniform constants
uniform bool u_useGammaCompensation;
uniform bool u_useCubemapColor;
uniform bool u_showTextureCoords;
uniform bool u_showTexture;
uniform bool u_hasTexture;

uniform samplerCube u_cubemap;
uniform sampler2D u_texture;
// ...

// Fragment shader inputs
in vec3 v_NormalVec;
in vec3 v_LightVec;
in vec3 v_ViewVec;

in vec3 v_diffuseColor;
in vec3 v_ambientColor;
in vec3 v_specularColor;

in float v_specularPower;
in vec2 v_texcoord_0;

// ...

// Fragment shader outputs
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec3 normalColor;
layout(location = 2) out vec3 depthColor;

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

float depthToLinear(float depth, float near, float far){
    return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

void main()
{
    vec3 V = normalize(v_ViewVec);
    vec3 L = normalize(v_LightVec);
    vec3 N = normalize(v_NormalVec);
    vec3 H = normalize(L + V);

    float brightness = max(0.0, dot(N,L));
    float celLevel = floor(brightness * maxCelLevels);
    brightness = celLevel / maxCelLevels;

    //Specular brightness
    float specBrightness = specular_normalized(N, H, v_specularPower);
    float celLevelSpec = floor(specBrightness * maxCelLevels);
    specBrightness = celLevelSpec / maxCelLevels;
    
    // Cubemap
    vec3 R = reflect(-V, N);
    vec3 cubeColored = texture(u_cubemap, R).rgb;

    // Get texture color or the manual input color
    vec3 diffuseColor;

    if(u_showTexture && u_hasTexture)
    {
        diffuseColor = texture(u_texture, v_texcoord_0).rgb;
    }
    else
    {
        diffuseColor = v_diffuseColor;
    }

    vec3 bpShading = v_ambientColor + diffuseColor * brightness + v_specularColor * specBrightness;

    if(u_useGammaCompensation)
    {
        bpShading = linear_to_gamma(bpShading);
        cubeColored = linear_to_gamma(cubeColored);
    }

    frag_color = vec4(bpShading, 1);

    if(u_useCubemapColor)
    {
        frag_color = vec4(cubeColored, 1);
    }

    if(u_showTextureCoords)
    {
        frag_color = vec4(v_texcoord_0, 0, 1);
    }

    normalColor = v_NormalVec;
    depthColor = vec3(depthToLinear(gl_FragCoord.z, 0.1f, 10f) / 10f);
}
