#version 430

// Uniforms
uniform mat4 m_pvm;        // projection * view * model
uniform mat4 m_viewModel;  // model -> eye space
uniform mat3 m_normal;     // normal matrix (for transforming normals)
uniform mat4 m_View;
uniform mat4 m_Model;   //por causa do cubo para a skybox
uniform int texMode;

// Vertex attributes
in vec4 position;
in vec4 normal;    // from geometry generator
in vec4 texCoord;

// Output to fragment shader
out Data {
    vec3 normal;    // normal in eye space
    vec3 eye;       // vector toward camera (eye space)
    vec3 posEye;    // position in eye space
    vec2 tex_coord; // texture coordinates
    vec3 skyboxTexCoord;
    vec3 eyeDir;
    vec3 reflectedDir;
} DataOut;

void main() {
    // Transform vertex position to eye space
    vec4 posEye4 = m_viewModel * position;

    // Eye vector in eye space
    vec3 eyeDir1 = -posEye4.xyz;
    DataOut.eyeDir = eyeDir1;

    // Output eye-space position
    DataOut.posEye = posEye4.xyz;

    // Transform normal to eye space
    DataOut.normal = normalize(m_normal * normal.xyz);

    // Compute vector toward camera (eye-space)
    // Eye is at origin in eye space, so vector = -posEye
    DataOut.eye = normalize(-posEye4.xyz);

    // Pass through texture coordinates
    DataOut.tex_coord = texCoord.st;

    // Compute clip-space position for rasterization
    gl_Position = m_pvm * position;

    DataOut.skyboxTexCoord = vec3(m_Model * position); // since translation is canceled, this works
    DataOut.skyboxTexCoord.x = -DataOut.skyboxTexCoord.x; // if needed to fix orientation

    if (texMode == 15) {
        DataOut.reflectedDir = vec3(transpose(m_View) * vec4(vec3(reflect(-DataOut.eyeDir, DataOut.normal)), 0.0)); //reflection vector in world coord
        DataOut.reflectedDir.x= - DataOut.reflectedDir.x;
    }
}
