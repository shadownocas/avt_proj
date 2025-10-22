#version 430

// Uniforms
uniform mat4 m_pvm;        // projection * view * model
uniform mat4 m_viewModel;  // model -> eye space
uniform mat3 m_normal;     // normal matrix (for transforming normals)
uniform mat4 m_View;
uniform mat4 m_Model;   //por causa do cubo para a skybox
uniform int texMode;

// Vertex attributes
uniform vec4 l_pos;
in vec4 tangent;
in vec4 position;
in vec4 normal;    // from geometry generator
in vec4 texCoord;

// Output to fragment shader
out vec3 normal1;       // normal in eye space
out vec3 eye;          // vector toward camera (eye space)
out vec3 posEye;       // position in eye space
out vec2 tex_coord;    // texture coordinates
out vec3 skyboxTexCoord;
out vec3 eyeDir;
out vec3 reflectedDir;
out vec3 tangent1;    // tangent in eye space
out float tangentSign;

void main() {
    // Transform vertex position to eye space
    vec4 posEye4 = m_viewModel * position;

    // Transform normal to eye space
    normal1 = normalize(m_normal * normal.xyz);

    vec3 n = normalize(m_normal * normal1.xyz);

    // Eye vector in eye space
    vec3 eyeDir1 = -posEye4.xyz;
    eyeDir = eyeDir1;

    vec3 lightDir = (l_pos.xyz - posEye4.xyz);

    // Output eye-space position
    posEye = posEye4.xyz;

    // Compute vector toward camera (eye-space)
    // Eye is at origin in eye space, so vector = -posEye
    eye = normalize(-posEye4.xyz);

    // Pass through texture coordinates
    tex_coord = texCoord.st;

    // Compute clip-space position for rasterization
    gl_Position = m_pvm * position;

    skyboxTexCoord = vec3(m_Model * position);
    skyboxTexCoord.x = -skyboxTexCoord.x;

    // Transform tangent to eye space
    tangent1 = normalize(m_normal * tangent.xyz);
    tangentSign = tangent.w;

    if (texMode == 15) {
        reflectedDir = vec3(transpose(m_View) * vec4(vec3(reflect(-eyeDir, normal1)), 0.0)); //reflection vector in world coord
        reflectedDir.x= - reflectedDir.x;
    }

    if (texMode == 3) {
        vec3 tgt = normalize(m_normal * tangent.xyz);
        vec3 b = tangent.w * cross(n, tgt);

        vec3 V;
        V.x = dot(lightDir, tgt);
        V.y = dot(lightDir, b);
        V.z = dot (lightDir, n);
        lightDir = normalize(V);

        V.x = dot(eyeDir, tgt);
        V.y = dot(eyeDir, b);
        V.z = dot(eyeDir, n);
        eyeDir = normalize(V);
    }
}
