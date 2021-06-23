/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEquirectangularToCubeMapTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEquirectangularToCubeMapTexture.h"
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

svtkStandardNewMacro(svtkEquirectangularToCubeMapTexture);
svtkCxxSetObjectMacro(svtkEquirectangularToCubeMapTexture, InputTexture, svtkOpenGLTexture);

//------------------------------------------------------------------------------
svtkEquirectangularToCubeMapTexture::svtkEquirectangularToCubeMapTexture()
{
  this->CubeMapOn();
}

//------------------------------------------------------------------------------
svtkEquirectangularToCubeMapTexture::~svtkEquirectangularToCubeMapTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void svtkEquirectangularToCubeMapTexture::ReleaseGraphicsResources(svtkWindow* win)
{
  if (this->InputTexture)
  {
    this->InputTexture->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void svtkEquirectangularToCubeMapTexture::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CubeMapSize: " << this->CubeMapSize << endl;
}

//------------------------------------------------------------------------------
void svtkEquirectangularToCubeMapTexture::Load(svtkRenderer* ren)
{
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    svtkErrorMacro("No render window.");
  }

  if (!this->InputTexture)
  {
    svtkErrorMacro("No input texture specified.");
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
    this->TextureObject->SetFormat(
      this->InputTexture->GetTextureObject()->GetFormat(SVTK_FLOAT, 3, true));
    this->TextureObject->SetInternalFormat(
      this->InputTexture->GetTextureObject()->GetInternalFormat(SVTK_FLOAT, 3, true));
    this->TextureObject->SetDataType(
      this->InputTexture->GetTextureObject()->GetDataType(SVTK_FLOAT));
    this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapR(svtkTextureObject::ClampToEdge);
    this->TextureObject->SetMinificationFilter(svtkTextureObject::Linear);
    this->TextureObject->SetMagnificationFilter(svtkTextureObject::Linear);
    this->TextureObject->CreateCubeFromRaw(
      this->CubeMapSize, this->CubeMapSize, 3, SVTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    svtkOpenGLState* state = renWin->GetState();
    svtkOpenGLState::ScopedglViewport svp(state);
    svtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    svtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    svtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    this->TextureObject->Activate();

    svtkNew<svtkOpenGLFramebufferObject> fbo;
    fbo->SetContext(renWin);
    state->PushFramebufferBindings();
    fbo->Bind();

    for (int i = 0; i < 6; i++)
    {
      fbo->AddColorAttachment(i, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }
    fbo->ActivateDrawBuffers(6);
    fbo->Start(this->CubeMapSize, this->CubeMapSize);

    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
      "uniform sampler2D equiTex;\n"
      "vec2 toSpherical(vec3 v)\n"
      "{\n"
      "  v = normalize(v);\n"
      "  float theta = atan(v.z, v.x);\n"
      "  float phi = asin(v.y);\n"
      "  return vec2(theta * 0.1591 + 0.5, phi * 0.3183 + 0.5);\n"
      "}\n"
      "//SVTK::FSQ::Decl");

    std::stringstream fsImpl;
    fsImpl << "  \n"
              "  float x = 2.0 * texCoord.x - 1.0;\n"
              "  float y = 1.0 - 2.0 * texCoord.y;\n"
              "  gl_FragData[0] = texture(equiTex, toSpherical(vec3(1, y, -x)));\n"
              "  gl_FragData[1] = texture(equiTex, toSpherical(vec3(-1, y, x)));\n"
              "  gl_FragData[2] = texture(equiTex, toSpherical(vec3(x, 1, -y)));\n"
              "  gl_FragData[3] = texture(equiTex, toSpherical(vec3(x, -1, y)));\n"
              "  gl_FragData[4] = texture(equiTex, toSpherical(vec3(x, y, 1)));\n"
              "  gl_FragData[5] = texture(equiTex, toSpherical(vec3(-x, y, -1)));\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl", fsImpl.str());

    svtkOpenGLQuadHelper quadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      svtkErrorMacro("Couldn't build the shader program for equirectangular to cubemap texture.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("equiTex", this->InputTexture->GetTextureUnit());
      quadHelper.Render();
      this->InputTexture->GetTextureObject()->Deactivate();
    }
    this->TextureObject->Deactivate();
    state->PopFramebufferBindings();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
