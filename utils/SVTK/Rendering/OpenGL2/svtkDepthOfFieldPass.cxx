/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDepthOfFieldPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDepthOfFieldPass.h"
#include "svtkObjectFactory.h"
#include <cassert>

#include "svtkCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"

#include "svtkOpenGLHelper.h"

#include "svtkDepthOfFieldPassFS.h"
#include "svtkTextureObjectVS.h"

svtkStandardNewMacro(svtkDepthOfFieldPass);

// ----------------------------------------------------------------------------
svtkDepthOfFieldPass::svtkDepthOfFieldPass()
{
  this->FrameBufferObject = nullptr;
  this->Pass1 = nullptr;
  this->Pass1Depth = nullptr;
  this->BlurProgram = nullptr;
  this->AutomaticFocalDistance = true;
}

// ----------------------------------------------------------------------------
svtkDepthOfFieldPass::~svtkDepthOfFieldPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    svtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass1 != nullptr)
  {
    svtkErrorMacro(<< "Pass1 should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass1Depth != nullptr)
  {
    svtkErrorMacro(<< "Pass1Depth should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void svtkDepthOfFieldPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkDepthOfFieldPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  svtkRenderer* r = s->GetRenderer();
  svtkOpenGLRenderWindow* renWin = static_cast<svtkOpenGLRenderWindow*>(r->GetRenderWindow());

  if (this->DelegatePass == nullptr)
  {
    svtkWarningMacro(<< " no delegate.");
    return;
  }

  // 1. Create a new render state with an FBO.

  int width;
  int height;
  int size[2];
  s->GetWindowSize(size);
  width = size[0];
  height = size[1];

  // I suggest set this to 100 for debugging, makes some errors
  // much easier to find.
  // Objects that are out of view can blur onto the image due to
  // the COC, so we render a few border pixels to prevent discontinuities
  const int extraPixels = 16;

  int w = width + extraPixels * 2;
  int h = height + extraPixels * 2;

  if (this->Pass1 == nullptr)
  {
    this->Pass1 = svtkTextureObject::New();
    this->Pass1->SetContext(renWin);
  }
  if (this->Pass1->GetWidth() != static_cast<unsigned int>(w) ||
    this->Pass1->GetHeight() != static_cast<unsigned int>(h))
  {
    this->Pass1->Create2D(
      static_cast<unsigned int>(w), static_cast<unsigned int>(h), 4, SVTK_UNSIGNED_CHAR, false);
  }

  // Depth texture
  if (this->Pass1Depth == nullptr)
  {
    this->Pass1Depth = svtkTextureObject::New();
    this->Pass1Depth->SetContext(renWin);
  }
  if (this->Pass1Depth->GetWidth() != static_cast<unsigned int>(w) ||
    this->Pass1Depth->GetHeight() != static_cast<unsigned int>(h))
  {
    this->Pass1Depth->AllocateDepth(w, h, svtkTextureObject::Float32);
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = svtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(
    s, width, height, w, h, this->FrameBufferObject, this->Pass1, this->Pass1Depth);

  renWin->GetState()->PopFramebufferBindings();

  // has something changed that would require us to recreate the shader?
  if (!this->BlurProgram)
  {
    this->BlurProgram = new svtkOpenGLHelper;
    // build the shader source code
    std::string VSSource = svtkTextureObjectVS;
    std::string FSSource = svtkDepthOfFieldPassFS;
    std::string GSSource;

    // compile and bind it if needed
    svtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(
      VSSource.c_str(), FSSource.c_str(), GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->BlurProgram->Program)
    {
      this->BlurProgram->Program = newShader;
      this->BlurProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->BlurProgram->ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlurProgram->Program);
  }

  if (!this->BlurProgram->Program)
  {
    return;
  }

  renWin->GetState()->svtkglDisable(GL_BLEND);
  renWin->GetState()->svtkglDisable(GL_DEPTH_TEST);

  this->Pass1->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->BlurProgram->Program->SetUniformi("source", this->Pass1->GetTextureUnit());

  this->Pass1Depth->Activate();
  this->BlurProgram->Program->SetUniformi("depth", this->Pass1Depth->GetTextureUnit());

  svtkCamera* cam = r->GetActiveCamera();
  double* frange = cam->GetClippingRange();
  float fdist = cam->GetDistance();
  //  fdist = fdist - 0.1*(frange[1] - frange[0]);
  float focalDisk = cam->GetFocalDisk();
  float vAngle = cam->GetViewAngle();
  double* aspect = r->GetAspect();

  float winWidth;
  float winHeight;
  if (cam->GetUseHorizontalViewAngle())
  {
    winWidth = 2.0 * tan(vAngle / 2.0) * fdist;
    winHeight = winWidth * aspect[1] / aspect[0];
  }
  else
  {
    winHeight = 2.0 * tan(vAngle / 2.0) * fdist;
    winWidth = winHeight * aspect[0] / aspect[1];
  }

  float offset[2];
  offset[0] = 1.0 / winWidth;
  offset[1] = 1.0 / winHeight;
  this->BlurProgram->Program->SetUniform2f("worldToTCoord", offset);
  offset[0] = 1.0 / w;
  offset[1] = 1.0 / h;
  this->BlurProgram->Program->SetUniform2f("pixelToTCoord", offset);
  this->BlurProgram->Program->SetUniformf("nearC", frange[0]);
  this->BlurProgram->Program->SetUniformf("farC", frange[1]);
  this->BlurProgram->Program->SetUniformf("focalDisk", focalDisk);

  if (this->AutomaticFocalDistance)
  {
    this->BlurProgram->Program->SetUniformf("focalDistance", 0.0);
  }
  else
  {
    this->BlurProgram->Program->SetUniformf("focalDistance", fdist);
  }

  this->Pass1->CopyToFrameBuffer(extraPixels, extraPixels, w - 1 - extraPixels, h - 1 - extraPixels,
    0, 0, width, height, this->BlurProgram->Program, this->BlurProgram->VAO);

  this->Pass1->Deactivate();
  this->Pass1Depth->Deactivate();

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkDepthOfFieldPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->BlurProgram != nullptr)
  {
    this->BlurProgram->ReleaseGraphicsResources(w);
    delete this->BlurProgram;
    this->BlurProgram = nullptr;
  }
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->Pass1 != nullptr)
  {
    this->Pass1->Delete();
    this->Pass1 = nullptr;
  }
  if (this->Pass1Depth != nullptr)
  {
    this->Pass1Depth->Delete();
    this->Pass1Depth = nullptr;
  }
}
