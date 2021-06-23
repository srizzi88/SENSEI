/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointFillPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPointFillPass.h"
#include "svtkObjectFactory.h"
#include <cassert>

#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"

#include "svtkPointFillPassFS.h"
#include "svtkTextureObjectVS.h"

svtkStandardNewMacro(svtkPointFillPass);

// ----------------------------------------------------------------------------
svtkPointFillPass::svtkPointFillPass()
{
  this->FrameBufferObject = nullptr;
  this->Pass1 = nullptr;
  this->Pass1Depth = nullptr;
  this->QuadHelper = nullptr;
  this->MinimumCandidateAngle = 1.5 * svtkMath::Pi();
  this->CandidatePointRatio = 0.99;
}

// ----------------------------------------------------------------------------
svtkPointFillPass::~svtkPointFillPass()
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
void svtkPointFillPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkPointFillPass::Render(const svtkRenderState* s)
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

  if (this->Pass1 == nullptr)
  {
    this->Pass1 = svtkTextureObject::New();
    this->Pass1->SetContext(renWin);
    this->Pass1->Create2D(static_cast<unsigned int>(width), static_cast<unsigned int>(height), 4,
      SVTK_UNSIGNED_CHAR, false);
  }
  this->Pass1->Resize(width, height);

  // Depth texture
  if (this->Pass1Depth == nullptr)
  {
    this->Pass1Depth = svtkTextureObject::New();
    this->Pass1Depth->SetContext(renWin);
    this->Pass1Depth->AllocateDepth(width, height, svtkTextureObject::Float32);
  }
  this->Pass1Depth->Resize(width, height);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = svtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(
    s, width, height, width, height, this->FrameBufferObject, this->Pass1, this->Pass1Depth);
  renWin->GetState()->PopFramebufferBindings();

  // has something changed that would require us to recreate the shader?
  if (!this->QuadHelper)
  {
    // build the shader source code
    // compile and bind it if needed
    this->QuadHelper = new svtkOpenGLQuadHelper(renWin, nullptr, svtkPointFillPassFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program)
  {
    return;
  }

  renWin->GetState()->svtkglDisable(GL_BLEND);
  //  glDisable(GL_DEPTH_TEST);

  this->Pass1->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->QuadHelper->Program->SetUniformi("source", this->Pass1->GetTextureUnit());

  this->Pass1Depth->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->QuadHelper->Program->SetUniformi("depth", this->Pass1Depth->GetTextureUnit());

  svtkCamera* cam = r->GetActiveCamera();
  double* frange = cam->GetClippingRange();
  this->QuadHelper->Program->SetUniformf("nearC", frange[0]);
  this->QuadHelper->Program->SetUniformf("farC", frange[1]);
  this->QuadHelper->Program->SetUniformf("MinimumCandidateAngle", this->MinimumCandidateAngle);
  this->QuadHelper->Program->SetUniformf("CandidatePointRatio", this->CandidatePointRatio);
  float offset[2];
  offset[0] = 1.0 / width;
  offset[1] = 1.0 / height;
  this->QuadHelper->Program->SetUniform2f("pixelToTCoord", offset);

  this->QuadHelper->Render();
  this->Pass1->Deactivate();
  this->Pass1Depth->Deactivate();

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkPointFillPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper != nullptr)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
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
