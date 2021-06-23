/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFramebufferPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkFramebufferPass.h"
#include "svtkObjectFactory.h"
#include <cassert>

// #include "svtkCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkTextureObject.h"
// #include "svtkShaderProgram.h"
// #include "svtkOpenGLShaderCache.h"
// #include "svtkOpenGLRenderWindow.h"
// #include "svtkOpenGLVertexArrayObject.h"

#include "svtkOpenGLHelper.h"

svtkStandardNewMacro(svtkFramebufferPass);

// ----------------------------------------------------------------------------
svtkFramebufferPass::svtkFramebufferPass()
{
  this->FrameBufferObject = nullptr;
  this->ColorTexture = svtkTextureObject::New();
  this->DepthTexture = svtkTextureObject::New();
  this->DepthFormat = svtkTextureObject::Float32;
  this->ColorFormat = svtkTextureObject::Fixed8;
}

// ----------------------------------------------------------------------------
svtkFramebufferPass::~svtkFramebufferPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    svtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
  }
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->Delete();
    this->DepthTexture = nullptr;
  }
}

// ----------------------------------------------------------------------------
void svtkFramebufferPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkFramebufferPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  svtkRenderer* r = s->GetRenderer();
  svtkOpenGLRenderWindow* renWin = static_cast<svtkOpenGLRenderWindow*>(r->GetRenderWindow());
  svtkOpenGLState* ostate = renWin->GetState();

  if (this->DelegatePass == nullptr)
  {
    svtkWarningMacro(<< " no delegate.");
    return;
  }

  // 1. Create a new render state with an FO.
  if (s->GetFrameBuffer() == nullptr)
  {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(
      &this->ViewportWidth, &this->ViewportHeight, &this->ViewportX, &this->ViewportY);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth = size[0];
    this->ViewportHeight = size[1];
    this->ViewportX = 0;
    this->ViewportY = 0;
  }

  this->ColorTexture->SetContext(renWin);
  if (!this->ColorTexture->GetHandle())
  {
    if (this->ColorFormat == svtkTextureObject::Float16)
    {
      this->ColorTexture->SetInternalFormat(GL_RGBA16F);
      this->ColorTexture->SetDataType(GL_FLOAT);
    }
    if (this->ColorFormat == svtkTextureObject::Float32)
    {
      this->ColorTexture->SetInternalFormat(GL_RGBA32F);
      this->ColorTexture->SetDataType(GL_FLOAT);
    }
    this->ColorTexture->Create2D(
      this->ViewportWidth, this->ViewportHeight, 4, SVTK_UNSIGNED_CHAR, false);
  }
  this->ColorTexture->Resize(this->ViewportWidth, this->ViewportHeight);

  // Depth texture
  this->DepthTexture->SetContext(renWin);
  if (!this->DepthTexture->GetHandle())
  {
    this->DepthTexture->AllocateDepth(this->ViewportWidth, this->ViewportHeight, this->DepthFormat);
  }
  this->DepthTexture->Resize(this->ViewportWidth, this->ViewportHeight);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = svtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  ostate->PushFramebufferBindings();
  this->RenderDelegate(s, this->ViewportWidth, this->ViewportHeight, this->ViewportWidth,
    this->ViewportHeight, this->FrameBufferObject, this->ColorTexture, this->DepthTexture);

  ostate->PopFramebufferBindings();

  // now copy the result to the outer FO
  ostate->PushReadFramebufferBinding();
  this->FrameBufferObject->Bind(this->FrameBufferObject->GetReadMode());

  ostate->svtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  ostate->svtkglScissor(this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);

  glBlitFramebuffer(0, 0, this->ViewportWidth, this->ViewportHeight, this->ViewportX,
    this->ViewportY, this->ViewportX + this->ViewportWidth, this->ViewportY + this->ViewportHeight,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);

  ostate->PopReadFramebufferBinding();

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkFramebufferPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->ReleaseGraphicsResources(w);
  }
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->ReleaseGraphicsResources(w);
  }
}
