/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCameraPass.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include <cassert>

svtkStandardNewMacro(svtkCameraPass);
svtkCxxSetObjectMacro(svtkCameraPass, DelegatePass, svtkRenderPass);

// ----------------------------------------------------------------------------
svtkCameraPass::svtkCameraPass()
{
  this->DelegatePass = nullptr;
  this->AspectRatioOverride = 1.0;
}

// ----------------------------------------------------------------------------
svtkCameraPass::~svtkCameraPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkCameraPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AspectRatioOverride: " << this->AspectRatioOverride << endl;
  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

void svtkCameraPass::GetTiledSizeAndOrigin(
  const svtkRenderState* render_state, int* width, int* height, int* originX, int* originY)
{
  svtkRenderer* ren = render_state->GetRenderer();
  ren->GetTiledSizeAndOrigin(width, height, originX, originY);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkCameraPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  svtkRenderer* ren = s->GetRenderer();

  if (!ren->IsActiveCameraCreated())
  {
    svtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet.
    // If is very unlikely that this can occur - if this
    // renderer is part of a svtkRenderWindow, the camera
    // will already have been created as part of the
    // DoStereoRender() method.

    // this is ren->GetActiveCameraAndResetIfCreated();
    ren->GetActiveCamera();
    ren->ResetCamera();
  }

  int lowerLeft[2];
  int usize;
  int vsize;
  svtkOpenGLFramebufferObject* fbo = svtkOpenGLFramebufferObject::SafeDownCast(s->GetFrameBuffer());

  svtkOpenGLRenderWindow* win = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  win->MakeCurrent();
  svtkOpenGLState* ostate = win->GetState();

  if (fbo == nullptr)
  {
    this->GetTiledSizeAndOrigin(s, &usize, &vsize, lowerLeft, lowerLeft + 1);
  }
  else
  {
    // FBO size. This is the renderer size as a renderstate is per renderer.
    int size[2];
    fbo->GetLastSize(size);
    usize = size[0];
    vsize = size[1];
    lowerLeft[0] = 0;
    lowerLeft[1] = 0;
    // we assume the drawbuffer state is already initialized before.
  }

  // Save the current viewport and camera matrices.
  svtkOpenGLState::ScopedglViewport vsaver(ostate);
  svtkOpenGLState::ScopedglScissor ssaver(ostate);
  svtkOpenGLState::ScopedglEnableDisable stsaver(ostate, GL_SCISSOR_TEST);

  ostate->svtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->svtkglEnable(GL_SCISSOR_TEST);
  ostate->svtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  // Done with camera initialization. The delegate can be called.
  svtkOpenGLCheckErrorMacro("failed after camera initialization");

  if (this->DelegatePass != nullptr)
  {
    svtkOpenGLRenderUtilities::MarkDebugEvent("Start svtkCameraPass delegate");
    this->DelegatePass->Render(s);
    svtkOpenGLRenderUtilities::MarkDebugEvent("End svtkCameraPass delegate");
    this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();
  }
  else
  {
    svtkWarningMacro(<< " no delegate.");
  }
  svtkOpenGLCheckErrorMacro("failed after delegate pass");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkCameraPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
