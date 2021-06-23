/*=========================================================================

   Program: ParaView
   Module:    svtkDepthImageProcessingPass.cxx

  Copyright (c) Sandia Corporation, Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/

#include "svtkDepthImageProcessingPass.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkTextureObject.h"
#include <cassert>

#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkPixelBufferObject.h"

// ----------------------------------------------------------------------------
svtkDepthImageProcessingPass::svtkDepthImageProcessingPass()
{
  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Width = 0;
  this->Height = 0;
  this->W = 0;
  this->H = 0;
  this->ExtraPixels = 0;
}

// ----------------------------------------------------------------------------
svtkDepthImageProcessingPass::~svtkDepthImageProcessingPass() = default;

// ----------------------------------------------------------------------------
void svtkDepthImageProcessingPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void svtkDepthImageProcessingPass::RenderDelegate(const svtkRenderState* s, int width, int height,
  int newWidth, int newHeight, svtkOpenGLFramebufferObject* fbo, svtkTextureObject* colortarget,
  svtkTextureObject* depthtarget)
{
  assert("pre: s_exists" && s != nullptr);
  assert("pre: fbo_exists" && fbo != nullptr);
  assert("pre: fbo_has_context" && fbo->GetContext() != nullptr);
  assert("pre: colortarget_exists" && colortarget != nullptr);
  assert("pre: colortarget_has_context" && colortarget->GetContext() != nullptr);
  assert("pre: depthtarget_exists" && depthtarget != nullptr);
  assert("pre: depthtarget_has_context" && depthtarget->GetContext() != nullptr);

  svtkRenderer* r = s->GetRenderer();
  svtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());

  // Adapt camera to new window size
  svtkCamera* savedCamera = r->GetActiveCamera();
  savedCamera->Register(this);
  svtkCamera* newCamera = svtkCamera::New();
  newCamera->DeepCopy(savedCamera);

  r->SetActiveCamera(newCamera);

  if (newCamera->GetParallelProjection())
  {
    newCamera->SetParallelScale(
      newCamera->GetParallelScale() * newHeight / static_cast<double>(height));
  }
  else
  {
    double large;
    double small;
    if (newCamera->GetUseHorizontalViewAngle())
    {
      large = newWidth;
      small = width;
    }
    else
    {
      large = newHeight;
      small = height;
    }
    double angle = svtkMath::RadiansFromDegrees(newCamera->GetViewAngle());
    angle = 2.0 * atan(tan(angle / 2.0) * large / static_cast<double>(small));

    newCamera->SetViewAngle(svtkMath::DegreesFromRadians(angle));
  }

  s2.SetFrameBuffer(fbo);
  fbo->Bind();
  fbo->AddColorAttachment(0, colortarget);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateDrawBuffer(0);

  fbo->AddDepthAttachment(depthtarget);
  fbo->StartNonOrtho(newWidth, newHeight);

  // 2. Delegate render in FBO
  // glEnable(GL_DEPTH_TEST);
  svtkOpenGLRenderUtilities::MarkDebugEvent("Start svtkDepthImageProcessingPass delegate render");
  this->DelegatePass->Render(&s2);
  svtkOpenGLRenderUtilities::MarkDebugEvent("End svtkDepthImageProcessingPass delegate render");

  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  newCamera->Delete();
  r->SetActiveCamera(savedCamera);
  savedCamera->UnRegister(this);
}

// ----------------------------------------------------------------------------
// Description:
// Read window size from parent
// \pre s_exists: s!=0
//
void svtkDepthImageProcessingPass::ReadWindowSize(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkOpenGLFramebufferObject* fbo = svtkOpenGLFramebufferObject::SafeDownCast(s->GetFrameBuffer());
  svtkRenderer* r = s->GetRenderer();
  if (fbo == nullptr)
  {
    r->GetTiledSizeAndOrigin(&this->Width, &this->Height, &this->Origin[0], &this->Origin[1]);
  }
  else
  {
    int size[2];
    fbo->GetLastSize(size);
    this->Origin[0] = 0;
    this->Origin[1] = 0;
    this->Width = size[0];
    this->Height = size[1];
  }
}
