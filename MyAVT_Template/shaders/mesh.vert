#version 430

// Uniforms
uniform mat4 m_pvm;        // projection * view * model
uniform mat4 m_viewModel;  // model -> eye space
uniform mat3 m_normal;     // normal matrix (for transforming normals)
//uniform mat4 m_Model;    //FIX

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
} DataOut;

void main() {
    // Transform vertex position to eye space
    vec4 posEye4 = m_viewModel * position;

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

    DataOut.skyboxTexCoord = position.xyz; // since translation is canceled, this works
    DataOut.skyboxTexCoord.x = -DataOut.skyboxTexCoord.x; // if needed to fix orientation
}
