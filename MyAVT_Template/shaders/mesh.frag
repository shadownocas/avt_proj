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

in vec3 normal1;       // normal in eye space
in vec3 eye;          // vector toward camera (eye space)
in vec3 posEye;       // position in eye space
in vec2 tex_coord;    // texture coordinates
in vec3 skyboxTexCoord;
in vec3 eyeDir;
in vec3 reflectedDir;

uniform Materials mat;
uniform float alpha_threshold;  // for transparency cutoff
const float reflect_factor = 0.9;

// Texture samplers
uniform sampler2D texmap;   
uniform sampler2D texmap1;  
uniform sampler2D texmap2;  
uniform sampler2D texmap3;  
uniform sampler2D texmap4;  
uniform sampler2D texmap5;  
uniform sampler2D texmap6;
uniform sampler2D texmap7;  
uniform sampler2D texmap8;  
uniform sampler2D texmap9;  
uniform sampler2D texmap10;  
uniform sampler2D texmap11;  
uniform sampler2D texmap12;  
uniform sampler2D texmap13;
uniform samplerCube texmap14;  

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

// Fog
uniform int   uFogOn;
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform float uFogStart;
uniform float uFogEnd;

uniform int texMode;

out vec4 colorOut;

// --- Phong for one light ---
vec3 calcPhong(vec3 Normal, vec3 View, vec3 LightDir, vec3 lightColor) {
    float diff = max(dot(Normal, LightDir), 0.0);
    vec3 HalfVec = normalize(LightDir + View);
    float spec = pow(max(dot(Normal, HalfVec), 0.0), mat.shininess);
    return diff * mat.diffuse.rgb * lightColor + spec * mat.specular.rgb * lightColor;
}

// --- Point light with attenuation ---
vec3 calcPointLight(PointLight light, vec3 Normal, vec3 View, vec3 posEye) {
    vec3 toLight = light.LocalPos - posEye;
    float dist = length(toLight);
    vec3 LightDir = normalize(toLight);

    float att = 1.0 / (light.atten.constant + light.atten.linear * dist + light.atten.exp * dist * dist);

    return calcPhong(Normal, View, LightDir, light.Color) * att;
}

vec3 calcSpotLight(SpotLight light, vec3 Normal, vec3 View, vec3 posEye) {
    vec3 toLight = light.Position - posEye;
    float dist = length(toLight);
    vec3 LightDir = normalize(toLight);

    // Spotlight factor: angle between light direction and vector to fragment
    float spotFactor = dot(-LightDir, normalize(light.Direction));
    if (spotFactor < light.Cutoff) {
        return vec3(0.0); // outside spotlight cone
    }

    // Attenuation
    float att = 1.0 / (light.atten.constant + light.atten.linear * dist + light.atten.exp * dist * dist);

    // Phong contribution
    return calcPhong(Normal, View, LightDir, light.Color) * att * spotFactor; // multiply by spotFactor for smooth edge
}


void main() {
    vec3 N = normalize(normal1);
    vec3 V = normalize(eye); // eye-space view vector
    vec3 intensity = mat.ambient.rgb; // start with ambient

    // --- Directional light ---
    vec3 Ldir = normalize(-dirLightDir);
    intensity += calcPhong(N, V, Ldir, dirLightColor);

    // --- Point lights ---
    if (lampsOn) {
        for (int i = 0; i < numLamps; ++i) {
            intensity += calcPointLight(pointLights[i], N, V, posEye);
        }
    }

	if (spotlightsOn) {
		for (int i = 0; i < 2; ++i) {
			intensity += calcSpotLight(spotLights[i], N, V, posEye);
		}
	}

    // --- Texture application ---
    vec4 texel, texel1;
    vec4 spec = vec4(0.0);

    if (texMode == 0)
        colorOut = vec4(max(intensity * mat.diffuse.rgb + spec.rgb, mat.ambient.rgb), 1.0);
    else if (texMode == 1) {
        texel = texture(texmap2, tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 2) {
        texel = texture(texmap, tex_coord);
        if (texel.a <= alpha_threshold)
            discard;
        vec3 finalColor = max(intensity * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, texel.a);
    }

    else if (texMode == 3) {
        texel = texture(texmap3, tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 4) {
        texel = texture(texmap4, tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 5) {
        texel = texture(texmap5, tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
     else if (texMode == 6) {
        texel = texture(texmap6, tex_coord);
        vec3 finalColor = max(intensity * mat.diffuse.rgb * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
    else if (texMode == 7){
        texel = texture(texmap7, tex_coord);
        if (texel.a <= alpha_threshold)
            discard;
        vec3 finalColor = max(intensity * texel.rgb + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, texel.a);
    }
    else if (texMode >= 8 && texMode <= 12) {
        // --- Flare effect ---
        if (texMode == 8)
            texel = texture(texmap8, tex_coord); // crcl
        else if (texMode == 9)
            texel = texture(texmap9, tex_coord); // flar
        else if (texMode == 10)
            texel = texture(texmap10, tex_coord); // hxgn
        else if (texMode == 11)
            texel = texture(texmap11, tex_coord); // ring
        else if (texMode == 12)
            texel = texture(texmap12, tex_coord); // sun
        else
            texel = vec4(1.0);

        vec4 finalColor = texel * mat.diffuse;

        if (finalColor.a < 0.2)
            discard;

        colorOut = finalColor;
        return;
    }
    else if (texMode == 13) {
        // Shadow: black color with slight transparency
        colorOut = vec4(0.0, 0.0, 0.0, 0.5);
    }
    else if (texMode == 14) { //Skybox
        vec3 cubeColor = texture(texmap14, skyboxTexCoord).rgb;
        colorOut = vec4(cubeColor, 1.0);
    }
    else if (texMode == 15) { // Environmental cube mapping
		vec4 cube_texel = texture(texmap14, reflectedDir); // interpolated reflected vector from vertex shader
		vec3 intensity3 = vec3(intensity); // vec3 intensity
		texel = texture(texmap4, tex_coord);  // texel from metal.tga
		vec4 aux_color = mix(texel, cube_texel, reflect_factor);
		
		// Calculate final color using component-wise operations
		vec3 final_rgb = max(intensity3 * aux_color.rgb + spec.rgb, 0.1 * aux_color.rgb);
		colorOut = vec4(final_rgb, 1.0);
	}
    else {
        texel = texture(texmap, tex_coord); // base floor
        vec3 finalTexture = texel.rgb;

        vec3 finalColor = max(intensity * finalTexture + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);

        // --- Garden overlay ---
        float gardenCenterX = 105.0;
        float gardenCenterZ = 95.0;
        float gardenW = 40.0;
        float gardenD = 40.0;

        // --- Lake overlay ---
        float lakeCenterX = 45.0;
        float lakeCenterZ = 160.0;
        float lakeW = 60.0;
        float lakeD = 40.0;

        float floorWidth = 200.0;
        float floorDepth = 200.0;

        // --- Compute garden bounds ---
        float gardenStartU = (gardenCenterX - gardenW/2.0) / floorWidth;
        float gardenEndU   = (gardenCenterX + gardenW/2.0) / floorWidth;
        float gardenStartV = (gardenCenterZ - gardenD/2.0) / floorDepth;
        float gardenEndV   = (gardenCenterZ + gardenD/2.0) / floorDepth;

        // --- Compute lake bounds ---
        float lakeStartU = (lakeCenterX - lakeW/2.0) / floorWidth;
        float lakeEndU   = (lakeCenterX + lakeW/2.0) / floorWidth;
        float lakeStartV = (lakeCenterZ - lakeD/2.0) / floorDepth;
        float lakeEndV   = (lakeCenterZ + lakeD/2.0) / floorDepth;

        // --- Priority order: lake > garden > road stripe ---
        if (tex_coord.x > lakeStartU && tex_coord.x < lakeEndU &&
            tex_coord.y > lakeStartV && tex_coord.y < lakeEndV) {

            // Lake region
            vec2 lakeUV;
            lakeUV.x = (tex_coord.x - lakeStartU) / (lakeEndU - lakeStartU);
            lakeUV.y = (tex_coord.y - lakeStartV) / (lakeEndV - lakeStartV);

            vec4 lakeTex = texture(texmap13, lakeUV);
            float lakeAlpha = 0.6;

            vec3 finalColor = max(intensity * lakeTex.rgb + spec.rgb, 0.07 * lakeTex.rgb);
            colorOut = vec4(finalColor, lakeAlpha);

        } else if (tex_coord.x > gardenStartU && tex_coord.x < gardenEndU &&
                tex_coord.y > gardenStartV && tex_coord.y < gardenEndV) {

            // Garden region
            vec2 gardenUV;
            gardenUV.x = (tex_coord.x - gardenStartU) / (gardenEndU - gardenStartU);
            gardenUV.y = (tex_coord.y - gardenStartV) / (gardenEndV - gardenStartV);

            finalTexture = texture(texmap5, gardenUV).rgb;

            vec3 finalColor = max(intensity * finalTexture + spec.rgb, 0.07 * texel.rgb);
            colorOut = vec4(finalColor, 1.0);

        } else {
            // Road stripe logic
            float stripeStart = 0.45;
            float stripeEnd   = 0.6;

            if (tex_coord.x > stripeStart && tex_coord.x < stripeEnd) {
                float stripeU = (tex_coord.x - stripeStart) / (stripeEnd - stripeStart);
                vec2 stripeUV = vec2(stripeU, tex_coord.y);
                finalTexture = texture(texmap6, stripeUV).rgb;

        vec3 finalColor = max(intensity * finalTexture + spec.rgb, 0.07 * texel.rgb);
        colorOut = vec4(finalColor, 1.0);
    }
}

    }
    
    // Fog
    if (uFogOn == 1) {
        float dist = length(posEye);
        float f = (uFogEnd - dist) / (uFogEnd - uFogStart);
        f = clamp(f, 0.0, 1.0);                         // 0=fog, 1=scene
        colorOut.rgb = mix(uFogColor, colorOut.rgb, f); // linear blend
    }
}
