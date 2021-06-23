/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPBRPrefilterTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPBRPrefilterTexture.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"

#include "svtk_glew.h"

#include <sstream>

svtkStandardNewMacro(svtkPBRPrefilterTexture);

svtkCxxSetObjectMacro(svtkPBRPrefilterTexture, InputTexture, svtkOpenGLTexture);

//------------------------------------------------------------------------------
svtkPBRPrefilterTexture::~svtkPBRPrefilterTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void svtkPBRPrefilterTexture::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PrefilterSize: " << this->PrefilterSize << "\n";
  os << indent << "PrefilterLevels: " << this->PrefilterLevels << "\n";
  os << indent << "PrefilterSamples: " << this->PrefilterSamples << endl;
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void svtkPBRPrefilterTexture::ReleaseGraphicsResources(svtkWindow* win)
{
  if (this->InputTexture)
  {
    this->InputTexture->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void svtkPBRPrefilterTexture::Load(svtkRenderer* ren)
{
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    svtkErrorMacro("No render window.");
  }

  if (!this->InputTexture)
  {
    svtkErrorMacro("No input cubemap specified.");
  }

  this->InputTexture->Render(ren);

  if (this->GetMTime() > this->LoadTime.GetMTime() ||
    this->InputTexture->GetMTime() > this->LoadTime.GetMTime())
  {
    if (this->TextureObject == nullptr)
    {
      this->TextureObject = svtkTextureObject::New();
    }
    this->TextureObject->SetContext(renWin);
    this->TextureObject->SetFormat(GL_RGB);
    this->TextureObject->SetInternalFormat(GL_RGB16F);
    this->TextureObject->SetDataType(GL_FLOAT);
    this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapR(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetMinificationFilter(svtkTextureObject::LinearMipmapLinear);
    this->TextureObject->SetMagnificationFilter(svtkTextureObject::Linear);
    this->TextureObject->SetGenerateMipmap(true);
    this->TextureObject->SetMaxLevel(this->PrefilterLevels - 1);
    this->TextureObject->CreateCubeFromRaw(
      this->PrefilterSize, this->PrefilterSize, 3, SVTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    svtkOpenGLState* state = renWin->GetState();
    svtkOpenGLState::ScopedglViewport svp(state);
    svtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    svtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    svtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
      "//SVTK::TEXTUREINPUT::Decl\n"
      "uniform float roughness;\n"
      "const float PI = 3.14159265359;\n"
      "vec3 GetSampleColor(vec3 dir)\n"
      "{\n"
      "  //SVTK::SAMPLING::Decl\n"
      "  //SVTK::COLORSPACE::Decl\n"
      "}\n"
      "float RadicalInverse_VdC(uint bits)\n"
      "{\n"
      "  bits = (bits << 16u) | (bits >> 16u);\n"
      "  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
      "  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
      "  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
      "  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
      "  return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
      "}\n"
      "vec2 Hammersley(uint i, uint N)\n"
      "{\n"
      "  return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
      "}\n"
      "vec3 ImportanceSampleGGX(vec2 rd, vec3 N, float roughness)\n"
      "{\n"
      "  float a = roughness*roughness;\n"
      "  float phi = 2.0 * PI * rd.x;\n"
      "  float cosTheta = sqrt((1.0 - rd.y) / (1.0 + (a*a - 1.0) * rd.y));\n"
      "  float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
      "  vec3 H;\n"
      "  H.x = cos(phi) * sinTheta;\n"
      "  H.y = sin(phi) * sinTheta;\n"
      "  H.z = cosTheta;\n"
      "  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
      "  vec3 tangent = normalize(cross(up, N));\n"
      "  vec3 bitangent = cross(N, tangent);\n"
      "  vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
      "  return normalize(sampleVec);\n"
      "}\n");

    if (this->ConvertToLinear)
    {
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::COLORSPACE::Decl", "return pow(col, vec3(2.2));");
    }
    else
    {
      svtkShaderProgram::Substitute(FSSource, "//SVTK::COLORSPACE::Decl", "return col;");
    }

    if (this->InputTexture->GetCubeMap())
    {
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::TEXTUREINPUT::Decl", "uniform samplerCube inputTex;");

      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::SAMPLING::Decl", "vec3 col = texture(inputTex, dir).rgb;");
    }
    else
    {
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::TEXTUREINPUT::Decl", "uniform sampler2D inputTex;");

      svtkShaderProgram::Substitute(FSSource, "//SVTK::SAMPLING::Decl",
        "  dir = normalize(dir);\n"
        "  float theta = atan(dir.z, dir.x);\n"
        "  float phi = asin(dir.y);\n"
        "  vec2 p = vec2(theta * 0.1591 + 0.5, phi * 0.3183 + 0.5);\n"
        "  vec3 col = texture(inputTex, p).rgb;\n");
    }

    std::stringstream fsImpl;
    fsImpl
      << "vec3 n_px = normalize(vec3(1.0, 1.0 - 2.0 * texCoord.y, 1.0 - 2.0 * texCoord.x));\n"
         "  vec3 n_nx = normalize(vec3(-1.0, 1.0 - 2.0 * texCoord.y, 2.0 * texCoord.x - 1.0));\n"
         "  vec3 n_py = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0, 2.0 * texCoord.y - 1.0));\n"
         "  vec3 n_ny = normalize(vec3(2.0 * texCoord.x - 1.0, -1.0, 1.0 - 2.0 * texCoord.y));\n"
         "  vec3 n_pz = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0 - 2.0 * texCoord.y, 1.0));\n"
         "  vec3 n_nz = normalize(vec3(1.0 - 2.0 * texCoord.x, 1.0 - 2.0 * texCoord.y, -1.0));\n"
         "  vec3 p_px = vec3(0.0);\n"
         "  vec3 p_nx = vec3(0.0);\n"
         "  vec3 p_py = vec3(0.0);\n"
         "  vec3 p_ny = vec3(0.0);\n"
         "  vec3 p_pz = vec3(0.0);\n"
         "  vec3 p_nz = vec3(0.0);\n"
         "  float w_px = 0.0;\n"
         "  float w_nx = 0.0;\n"
         "  float w_py = 0.0;\n"
         "  float w_ny = 0.0;\n"
         "  float w_pz = 0.0;\n"
         "  float w_nz = 0.0;\n"
         "  for (uint i = 0u; i < "
      << this->PrefilterSamples
      << "u; i++)\n"
         "  {\n"
         "    vec2 rd = Hammersley(i, "
      << this->PrefilterSamples
      << "u);\n"
         "    vec3 h_px = ImportanceSampleGGX(rd, n_px, roughness);\n"
         "    vec3 h_nx = ImportanceSampleGGX(rd, n_nx, roughness);\n"
         "    vec3 h_py = ImportanceSampleGGX(rd, n_py, roughness);\n"
         "    vec3 h_ny = ImportanceSampleGGX(rd, n_ny, roughness);\n"
         "    vec3 h_pz = ImportanceSampleGGX(rd, n_pz, roughness);\n"
         "    vec3 h_nz = ImportanceSampleGGX(rd, n_nz, roughness);\n"
         "    vec3 l_px = normalize(2.0 * dot(n_px, h_px) * h_px - n_px);\n"
         "    vec3 l_nx = normalize(2.0 * dot(n_nx, h_nx) * h_nx - n_nx);\n"
         "    vec3 l_py = normalize(2.0 * dot(n_py, h_py) * h_py - n_py);\n"
         "    vec3 l_ny = normalize(2.0 * dot(n_ny, h_ny) * h_ny - n_ny);\n"
         "    vec3 l_pz = normalize(2.0 * dot(n_pz, h_pz) * h_pz - n_pz);\n"
         "    vec3 l_nz = normalize(2.0 * dot(n_nz, h_nz) * h_nz - n_nz);\n"
         "    float d_px = max(dot(n_px, l_px), 0.0);\n"
         "    float d_nx = max(dot(n_nx, l_nx), 0.0);\n"
         "    float d_py = max(dot(n_py, l_py), 0.0);\n"
         "    float d_ny = max(dot(n_ny, l_ny), 0.0);\n"
         "    float d_pz = max(dot(n_pz, l_pz), 0.0);\n"
         "    float d_nz = max(dot(n_nz, l_nz), 0.0);\n"
         "    p_px += GetSampleColor(l_px) * d_px;\n"
         "    p_nx += GetSampleColor(l_nx) * d_nx;\n"
         "    p_py += GetSampleColor(l_py) * d_py;\n"
         "    p_ny += GetSampleColor(l_ny) * d_ny;\n"
         "    p_pz += GetSampleColor(l_pz) * d_pz;\n"
         "    p_nz += GetSampleColor(l_nz) * d_nz;\n"
         "    w_px += d_px;\n"
         "    w_nx += d_nx;\n"
         "    w_py += d_py;\n"
         "    w_ny += d_ny;\n"
         "    w_pz += d_pz;\n"
         "    w_nz += d_nz;\n"
         "  }\n"
         "  gl_FragData[0] = vec4(p_px / w_px, 1.0);\n"
         "  gl_FragData[1] = vec4(p_nx / w_nx, 1.0);\n"
         "  gl_FragData[2] = vec4(p_py / w_py, 1.0);\n"
         "  gl_FragData[3] = vec4(p_ny / w_ny, 1.0);\n"
         "  gl_FragData[4] = vec4(p_pz / w_pz, 1.0);\n"
         "  gl_FragData[5] = vec4(p_nz / w_nz, 1.0);\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl", fsImpl.str());

    svtkOpenGLQuadHelper quadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      svtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("inputTex", this->InputTexture->GetTextureUnit());

      svtkNew<svtkOpenGLFramebufferObject> fbo;
      fbo->SetContext(renWin);

      renWin->GetState()->PushFramebufferBindings();
      fbo->Bind();

      for (unsigned int mip = 0; mip < this->PrefilterLevels; mip++)
      {
        fbo->RemoveColorAttachments(6);
        for (int i = 0; i < 6; i++)
        {
          fbo->AddColorAttachment(
            i, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
        }
        fbo->ActivateDrawBuffers(6);
        unsigned currentSize = this->PrefilterSize >> mip;
        fbo->Start(currentSize, currentSize);

        float roughness = static_cast<float>(mip) / static_cast<float>(this->PrefilterLevels - 1);
        quadHelper.Program->SetUniformf("roughness", roughness);

        quadHelper.Render();
      }

      renWin->GetState()->PopFramebufferBindings();

      this->InputTexture->GetTextureObject()->Deactivate();
    }
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
