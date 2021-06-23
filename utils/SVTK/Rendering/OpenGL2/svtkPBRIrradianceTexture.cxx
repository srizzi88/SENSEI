/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPBRIrradianceTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPBRIrradianceTexture.h"
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

svtkStandardNewMacro(svtkPBRIrradianceTexture);

svtkCxxSetObjectMacro(svtkPBRIrradianceTexture, InputTexture, svtkOpenGLTexture);

//------------------------------------------------------------------------------
svtkPBRIrradianceTexture::~svtkPBRIrradianceTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void svtkPBRIrradianceTexture::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IrradianceStep: " << this->IrradianceStep << "\n";
  os << indent << "IrradianceSize: " << this->IrradianceSize << endl;
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void svtkPBRIrradianceTexture::ReleaseGraphicsResources(svtkWindow* win)
{
  if (this->InputTexture)
  {
    this->InputTexture->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void svtkPBRIrradianceTexture::Load(svtkRenderer* ren)
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
    this->TextureObject->SetMinificationFilter(svtkTextureObject::Linear);
    this->TextureObject->SetMagnificationFilter(svtkTextureObject::Linear);
    this->TextureObject->CreateCubeFromRaw(
      this->IrradianceSize, this->IrradianceSize, 3, SVTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    svtkOpenGLState* state = renWin->GetState();
    svtkOpenGLState::ScopedglViewport svp(state);
    svtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    svtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    svtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
      "//SVTK::TEXTUREINPUT::Decl\n"
      "uniform vec3 shift;\n"
      "uniform vec3 contribX;\n"
      "uniform vec3 contribY;\n"
      "const float PI = 3.14159265359;\n"
      "vec3 GetSampleColor(vec3 dir)\n"
      "{\n"
      "  //SVTK::SAMPLING::Decl\n"
      "  //SVTK::COLORSPACE::Decl\n"
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
      << "  const vec3 x = vec3(1.0, 0.0, 0.0);\n"
         "  const vec3 y = vec3(0.0, 1.0, 0.0);\n"
         "  vec3 n = normalize(vec3(shift.x + contribX.x * texCoord.x + contribY.x * texCoord.y,\n"
         "    shift.y + contribX.y * texCoord.x + contribY.y * texCoord.y,\n"
         "    shift.z + contribX.z * texCoord.x + contribY.z * texCoord.y));\n"
         "  vec3 t = normalize(cross(n, y));\n"
         "  mat3 m = mat3(t, cross(n, t), n);\n"
         "  vec3 acc = vec3(0.0);\n"
         "  float nSamples = 0.0;\n"
         "  for (float phi = 0.0; phi < 2.0 * PI; phi += "
      << this->IrradianceStep
      << ")\n"
         "  {\n"
         "    for (float theta = 0.0; theta < 0.5 * PI; theta += "
      << this->IrradianceStep
      << ")\n"
         "    {\n"
         "      vec3 sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));\n"
         "      float factor = cos(theta) * sin(theta);\n"
         "      acc += GetSampleColor(m * sample) * factor;\n"
         "      nSamples = nSamples + 1.0;\n"
         "    }\n"
         "  }\n"
         "  gl_FragData[0] = vec4(acc * (PI / nSamples), 1.0);\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl", fsImpl.str());

    svtkOpenGLQuadHelper quadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    svtkNew<svtkOpenGLFramebufferObject> fbo;
    fbo->SetContext(renWin);
    renWin->GetState()->PushFramebufferBindings();
    fbo->Bind();

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      svtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("inputTex", this->InputTexture->GetTextureUnit());

      float shift[6][3] = { { 1.f, 1.f, 1.f }, { -1.f, 1.f, -1.f }, { -1.f, 1.f, -1.f },
        { -1.f, -1.f, 1.f }, { -1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f } };
      float contribX[6][3] = { { 0.f, 0.f, -2.f }, { 0.f, 0.f, 2.f }, { 2.f, 0.f, 0.f },
        { 2.f, 0.f, 0.f }, { 2.f, 0.f, 0.f }, { -2.f, 0.f, 0.f } };
      float contribY[6][3] = { { 0.f, -2.f, 0.f }, { 0.f, -2.f, 0.f }, { 0.f, 0.f, 2.f },
        { 0.f, 0.f, -2.f }, { 0.f, -2.f, 0.f }, { 0.f, -2.f, 0.f } };

      for (int i = 0; i < 6; i++)
      {
        fbo->AddColorAttachment(0, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
        fbo->ActivateDrawBuffers(1);
        fbo->Start(this->IrradianceSize, this->IrradianceSize);

        quadHelper.Program->SetUniform3f("shift", shift[i]);
        quadHelper.Program->SetUniform3f("contribX", contribX[i]);
        quadHelper.Program->SetUniform3f("contribY", contribY[i]);
        quadHelper.Render();
        fbo->RemoveColorAttachment(0);

        // Computing irradiance can be long depending on the GPU.
        // On Windows 7, a computation longer than 2 seconds triggers GPU timeout.
        // The following call do a glFlush() that inform the OS that the computation is finished
        // thus avoids the trigger of the GPU timeout.
        renWin->WaitForCompletion();
      }
      this->InputTexture->GetTextureObject()->Deactivate();
    }
    renWin->GetState()->PopFramebufferBindings();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
