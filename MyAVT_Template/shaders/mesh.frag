#version 430
#define MAX_LAMPS 16 

struct Materials {
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emissive;
    float shininess;
    int texCount;
};

in Data {
    vec3 normal;
    vec3 eye;
    vec3 lightDir;
    vec2 tex_coord;
} DataIn;

uniform Materials mat;

// Texture samplers
uniform sampler2D texmap;   // stone
uniform sampler2D texmap1;  // checker
uniform sampler2D texmap2;  // lightwood
uniform sampler2D texmap3;  // bricks
uniform sampler2D texmap4;  // metal

// Directional light (day/night)
uniform vec3 dirLightDir;   // normalized
uniform vec3 dirLightColor; // RGB intensity

// Spotlight
uniform bool spotlight_mode;
uniform vec4 coneDir;
uniform float spotCosCutOff;

// Mesh
uniform int texMode;

out vec4 colorOut;

void main() {
    vec3 n = normalize(DataIn.normal);
    vec3 e = normalize(DataIn.eye);
    vec3 l = normalize(DataIn.lightDir);
    vec3 sd = normalize(coneDir.xyz);

    vec4 texel = vec4(1.0);
    vec4 spec = vec4(0.0);
    float intensity = 0.0;
    float intSpec = 0.0;
    float att = 0.0;
    float spotExp = 60.0;

    // --- Spotlight / Point Light ---
    if(spotlight_mode) {
        float spotCos = dot(-l, sd);
        if (spotCos > spotCosCutOff) {
            att = pow(spotCos, spotExp);
            intensity = max(dot(n, l), 0.0) * att;
            if (intensity > 0.0) {
                vec3 h = normalize(l + e);
                intSpec = max(dot(h, n), 0.0);
                spec = mat.specular * pow(intSpec, mat.shininess) * att;
            }
        }
    } else {
        intensity = max(dot(n, l), 0.0);
        if (intensity > 0.0) {
            vec3 h = normalize(l + e);
            intSpec = max(dot(h, n), 0.0);
            spec = mat.specular * pow(intSpec, mat.shininess);
        }
    }

    // --- Directional light contribution ---
    vec3 dirLight = normalize(-dirLightDir);
    float diffDir = max(dot(n, dirLight), 0.0);

    // --- Select texture ---
    if (texMode == 1) texel = texture(texmap2, DataIn.tex_coord); // lightwood
    else if (texMode == 2) texel = texture(texmap, DataIn.tex_coord); // stone
    else if (texMode == 3) texel = texture(texmap3, DataIn.tex_coord); // bricks
    else if (texMode == 4) texel = texture(texmap4, DataIn.tex_coord); // metal

    // --- Combine lights with texture ---
    vec3 finalColor = texel.rgb * (diffDir * dirLightColor + intensity * mat.diffuse.rgb) + spec.rgb;

    // --- Add minimal ambient contribution ---
    finalColor = max(finalColor, mat.ambient.rgb + 0.07 * texel.rgb);

    colorOut = vec4(finalColor, 1.0);
}
