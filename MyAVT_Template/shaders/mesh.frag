#version 430
#define MAX_LAMPS 7

struct Materials {
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emissive;
    float shininess;
    int texCount;
};

struct Attenuation {
    float constant;
    float linear;
    float exp;
};

struct PointLight {
    vec3 LocalPos;
    Attenuation atten;
    vec3 Color;
};

struct SpotLight {
    vec3 Position;
    vec3 Direction;
    float Cutoff;
    Attenuation atten;
    vec3 Color;
};

in Data {
    vec3 normal;
    vec3 eye;       // vector toward camera
    vec3 posEye;    // position in eye space
    vec2 tex_coord;
} DataIn;

uniform Materials mat;
uniform float alpha_threshold;  // for transparency cutoff

// Texture samplers
uniform sampler2D texmap;   
uniform sampler2D texmap1;  
uniform sampler2D texmap2;  
uniform sampler2D texmap3;  
uniform sampler2D texmap4;  
uniform sampler2D texmap5;  
uniform sampler2D texmap6;

// Directional light
uniform vec3 dirLightDir;   
uniform vec3 dirLightColor; 

// Point lights
uniform int numLamps;
uniform PointLight pointLights[MAX_LAMPS];
uniform bool lampsOn;

// SpotLight
uniform bool spotlightsOn;
uniform SpotLight spotLights[2];

uniform int texMode;

out vec4 colorOut;

// --- Phong for one light ---
vec3 calcPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor) {
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), mat.shininess);
    return diff * mat.diffuse.rgb * lightColor + spec * mat.specular.rgb * lightColor;
}

// --- Compute point light with attenuation ---
vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 posEye) {
    vec3 toLight = light.LocalPos - posEye;
    float dist = length(toLight);
    vec3 L = normalize(toLight);

    float att = 1.0 / (light.atten.constant + light.atten.linear * dist + light.atten.exp * dist * dist);

    return calcPhong(N, V, L, light.Color) * att;
}

vec3 calcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 posEye) {
    vec3 toLight = light.Position - posEye;
    float dist = length(toLight);
    vec3 L = normalize(toLight);

    // Spotlight factor: angle between light direction and vector to fragment
    float spotFactor = dot(-L, normalize(light.Direction));
    if (spotFactor < light.Cutoff) {
        return vec3(0.0); // outside spotlight cone
    }

    // Attenuation
    float att = 1.0 / (light.atten.constant + light.atten.linear * dist + light.atten.exp * dist * dist);

    // Phong contribution
    return calcPhong(N, V, L, light.Color) * att * spotFactor; // multiply by spotFactor for smooth edge
}


void main() {
    vec3 N = normalize(DataIn.normal);
    vec3 V = normalize(DataIn.eye); // eye-space view vector
    vec3 intensity = mat.ambient.rgb; // start with ambient

    // --- Directional light ---
    vec3 Ldir = normalize(-dirLightDir);
    intensity += calcPhong(N, V, Ldir, dirLightColor);

    // --- Point lights ---
    if (lampsOn) {
        for (int i = 0; i < numLamps; ++i) {
            intensity += calcPointLight(pointLights[i], N, V, DataIn.posEye);
        }
    }

	if (spotlightsOn) {
		for (int i = 0; i < 2; ++i) {
			intensity += calcSpotLight(spotLights[i], N, V, DataIn.posEye);
		}
	}

    // --- Texture application ---
    vec4 texel, texel1;
    vec4 spec = vec4(0.0);

    if (texMode == 0)
        colorOut = vec4(max(intensity * mat.diffuse.rgb + spec.rgb, mat.ambient.rgb), 1.0);
    else if (texMode == 1) {
        texel = texture(texmap2, DataIn.tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 2) {
        texel = texture(texmap, DataIn.tex_coord);
        if (texel.a <= alpha_threshold)
            discard;
        vec3 finalColor = max(intensity * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, texel.a);
    }

    else if (texMode == 3) {
        texel = texture(texmap3, DataIn.tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 4) {
        texel = texture(texmap4, DataIn.tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 5) {
        texel = texture(texmap5, DataIn.tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
     else if (texMode == 6) {
        texel = texture(texmap6, DataIn.tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else {
        texel = texture(texmap, DataIn.tex_coord);
        float stripeStart = 0.45;
        float stripeEnd   = 0.6;

        vec3 finalTexture;
        if (DataIn.tex_coord.x > stripeStart && DataIn.tex_coord.x < stripeEnd) {
            float stripeU = (DataIn.tex_coord.x - stripeStart) / (stripeEnd - stripeStart);
            vec2 stripeUV = vec2(stripeU, DataIn.tex_coord.y);

            texel1 = texture(texmap6, stripeUV);
            finalTexture = texel1.rgb;
        } 
        else {
            finalTexture = texel.rgb;
        }
        vec3 finalColor = max(intensity * finalTexture + spec.rgb, 0.07 * texel.rgb * texel1.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
}
