#include "svtkOpenGLFXAAPass.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"

svtkStandardNewMacro(svtkOpenGLFXAAPass);

// ----------------------------------------------------------------------------
void svtkOpenGLFXAAPass::Render(const svtkRenderState* s)
{
  svtkOpenGLRenderer* r = svtkOpenGLRenderer::SafeDownCast(s->GetRenderer());
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
  svtkOpenGLState* ostate = renWin->GetState();

  svtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  ostate->svtkglViewport(x, y, w, h);
  ostate->svtkglScissor(x, y, w, h);

  if (this->DelegatePass == nullptr)
  {
    svtkWarningMacro("no delegate in svtkOpenGLFXAAPass.");
    return;
  }

  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();

  if (r->GetFXAAOptions())
  {
    this->FXAAFilter->UpdateConfiguration(r->GetFXAAOptions());
  }

  this->FXAAFilter->Execute(r);
}

// ----------------------------------------------------------------------------
void svtkOpenGLFXAAPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
