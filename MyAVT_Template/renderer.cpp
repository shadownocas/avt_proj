//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: Jo�o Madeiras Pereira
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "renderer.h"
#include "mathUtility.h"
#include "shader.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"

using namespace std;

Renderer::Renderer() {}



bool Renderer::setRenderMeshesShaderProg(const std::string& vertShaderPath, const std::string& fragShaderPath) {

    // Shader for models
    Shader shader;
    shader.init();
    program = shader.getProgramIndex();
    shader.compileShader(Shader::VERTEX_SHADER, vertShaderPath);
    shader.compileShader(Shader::FRAGMENT_SHADER, fragShaderPath);

    // set semantics for the shader variables
    glBindFragDataLocation(program, 0, "colorOut");
    glBindAttribLocation(program, Shader::VERTEX_COORD_ATTRIB, "position");
    glBindAttribLocation(program, Shader::NORMAL_ATTRIB, "normal");
    glBindAttribLocation(program, Shader::TEXTURE_COORD_ATTRIB, "texCoord");

    glLinkProgram(program);

    printf("InfoLog for Model Shaders and Program\n%s\n\n", shader.getAllInfoLogs().c_str());
    if (!shader.isProgramValid())
        printf("GLSL Model Program Not Valid!\n");

    pvm_loc = glGetUniformLocation(program, "m_pvm");
    vm_loc = glGetUniformLocation(program, "m_viewModel");
    normal_loc = glGetUniformLocation(program, "m_normal");
    texMode_loc = glGetUniformLocation(program, "texMode"); // different modes of texturing
    lpos_loc = glGetUniformLocation(program, "l_pos");
    tex_loc[0] = glGetUniformLocation(program, "texmap");
    tex_loc[1] = glGetUniformLocation(program, "texmap1");
    tex_loc[2] = glGetUniformLocation(program, "texmap2");
    tex_loc[3] = glGetUniformLocation(program, "texmap3");
    tex_loc[4] = glGetUniformLocation(program, "texmap4");
    tex_loc[5] = glGetUniformLocation(program, "texmap5");
    tex_loc[6] = glGetUniformLocation(program, "texmap6");

    return(shader.isProgramLinked() && shader.isProgramValid());
}


void Renderer::setFog(bool on, const float color[3], float start, float end) {
    GLint pid = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
    if (!pid) return;

    GLint locOn  = glGetUniformLocation(pid, "uFogOn");
    GLint locCol = glGetUniformLocation(pid, "uFogColor");
    GLint locFS  = glGetUniformLocation(pid, "uFogStart");
    GLint locFE  = glGetUniformLocation(pid, "uFogEnd");

    if (locOn >= 0)  glUniform1i(locOn, on ? 1 : 0);
    if (locCol >= 0) glUniform3f(locCol, color[0], color[1], color[2]);
    if (locFS >= 0)  glUniform1f(locFS, start);
    if (locFE >= 0)  glUniform1f(locFE, end);
}

Renderer::~Renderer() {
    glDeleteProgram(program);
    glDeleteProgram(textProgram);
    for (auto& mesh : myMeshes) glDeleteVertexArrays(1, &(mesh.vao));
    myMeshes.clear(); myMeshes.shrink_to_fit();
}

bool Renderer::setRenderTextShaderProg(const std::string& vertShaderPath, const std::string& fragShaderPath) {
   
    Shader shader;    // Shader for rendering True Type Font (ttf) bitmap text
    shader.init();
    textProgram = shader.getProgramIndex();
    shader.compileShader(Shader::VERTEX_SHADER, vertShaderPath);
    shader.compileShader(Shader::FRAGMENT_SHADER, fragShaderPath);

    glLinkProgram(textProgram);
    printf("InfoLog for Text Rendering Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

    if (!shader.isProgramValid()) {
        printf("GLSL Text Program Not Valid!\n");
        exit(1);
    }

    fontPvm_loc = glGetUniformLocation(textProgram, "pvm");
    textColor_loc = glGetUniformLocation(textProgram, "textColor");

    // static font atlas texture binding
    glUseProgram(textProgram);
    glActiveTexture(GL_TEXTURE16);
    glBindTexture(GL_TEXTURE_2D, font.textureId);
    glUniform1i(glGetUniformLocation(textProgram, "fontAtlasTexture"), 16);

    return(shader.isProgramLinked() && shader.isProgramValid());
}


void Renderer::activateRenderMeshesShaderProg() {   //GLSL program to draw the meshes
    glUseProgram(program);
}

void Renderer::setSpotParam(float* coneDir, const float cutOff) {
    GLint loc;
    loc = glGetUniformLocation(program, "coneDir");
    glUniform4fv(loc, 1, coneDir);
    loc = glGetUniformLocation(program, "spotCosCutOff");
    glUniform1f(loc, cutOff);
}

void Renderer::setSpotLightMode(bool spotLightMode) {
    GLint loc;
    loc = glGetUniformLocation(program, "spotlight_mode");
    if (spotLightMode)
        glUniform1i(loc, 1);
    else
        glUniform1i(loc, 0);
}

void Renderer::setLightPos(float* lightPos) {
    glUniform4fv(lpos_loc, 1, lightPos);
}

void Renderer::setDirectionalLight(float* dir, float* color) {
    GLint locDir = glGetUniformLocation(program, "dirLightDir");
    glUniform3fv(locDir, 1, dir);

    GLint locCol = glGetUniformLocation(program, "dirLightColor");
    glUniform3fv(locCol, 1, color);
}

void Renderer::setLampLights(const std::vector<PointLight> &pointLights, bool lampsOn)
{
    glUseProgram(program);

    GLint locNum = glGetUniformLocation(program, "numLamps");
    glUniform1i(locNum, (GLint)pointLights.size());

    GLint locOn = glGetUniformLocation(program, "lampsOn");
    glUniform1i(locOn, lampsOn ? 1 : 0);

    for (size_t i = 0; i < pointLights.size(); ++i)
    {
        std::string base = "pointLights[" + std::to_string(i) + "]";

        GLint locPos = glGetUniformLocation(program, (base + ".LocalPos").c_str());
        GLint locColor = glGetUniformLocation(program, (base + ".Color").c_str());
        GLint locAttenC = glGetUniformLocation(program, (base + ".atten.constant").c_str());
        GLint locAttenL = glGetUniformLocation(program, (base + ".atten.linear").c_str());
        GLint locAttenE = glGetUniformLocation(program, (base + ".atten.exp").c_str());

        glUniform3fv(locPos, 1, pointLights[i].LocalPos);
        glUniform3fv(locColor, 1, pointLights[i].Color);
        glUniform1f(locAttenC, pointLights[i].atten.constant);
        glUniform1f(locAttenL, pointLights[i].atten.linear);
        glUniform1f(locAttenE, pointLights[i].atten.exp);
    }
}

void Renderer::setDroneSpotLights(SpotLight* lights, int count, bool enabled) {
    // Tell the shader if spotlights are on
    GLint loc = glGetUniformLocation(program, "spotlightsOn");
    glUniform1i(loc, enabled ? 1 : 0);

    for(int i = 0; i < count; ++i) {
        std::string base = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, (base + ".Position").c_str()), 1, lights[i].Position);
        glUniform3fv(glGetUniformLocation(program, (base + ".Direction").c_str()), 1, lights[i].Direction);
        glUniform3fv(glGetUniformLocation(program, (base + ".Color").c_str()), 1, lights[i].Color);
        glUniform1f(glGetUniformLocation(program, (base + ".Cutoff").c_str()), lights[i].Cutoff);

        glUniform1f(glGetUniformLocation(program, (base + ".atten.constant").c_str()), lights[i].atten.constant);
        glUniform1f(glGetUniformLocation(program, (base + ".atten.linear").c_str()), lights[i].atten.linear);
        glUniform1f(glGetUniformLocation(program, (base + ".atten.exp").c_str()), lights[i].atten.exp);
    }
}



void Renderer::setTexUnit(int tuId, int texObjId) {
    glActiveTexture(GL_TEXTURE0 + tuId);
    glBindTexture(GL_TEXTURE_2D, TexObjArray.getTextureId(texObjId));
    glUniform1i(tex_loc[tuId], tuId);
}

void Renderer::renderMesh(const dataMesh& data) {
    GLint loc;
    const MyMesh& mesh = *data.mesh;

    // be aware to activate previously the Model shader program
    glUniformMatrix4fv(vm_loc, 1, GL_FALSE, data.vm);
    glUniformMatrix4fv(pvm_loc, 1, GL_FALSE, data.pvm);
    glUniformMatrix3fv(normal_loc, 1, GL_FALSE, data.normal);

    // send the material
    loc = glGetUniformLocation(program, "mat.ambient");
    glUniform4fv(loc, 1, mesh.mat.ambient);
    loc = glGetUniformLocation(program, "mat.diffuse");
    glUniform4fv(loc, 1, mesh.mat.diffuse);
    loc = glGetUniformLocation(program, "mat.specular");
    glUniform4fv(loc, 1, mesh.mat.specular);
    loc = glGetUniformLocation(program, "mat.shininess");
    glUniform1f(loc, mesh.mat.shininess);

    // Render mesh
    glUniform1i(texMode_loc, data.texMode);

    // --- Set alpha threshold for transparent objects ---
    GLint locAlpha = glGetUniformLocation(program, "alpha_threshold");
    glUniform1f(locAlpha, 0.05f);

    glBindVertexArray(mesh.vao);
    glDrawElements(mesh.type, mesh.numIndexes, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
