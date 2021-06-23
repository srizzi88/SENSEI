/*=========================================================================

   Program: ParaView
   Module:    svtkEDLShading.cxx

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

#include "svtkEDLShading.h"

#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkPropCollection.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtk_glew.h"
#include <cassert>
#include <sstream>
#include <string>

#include "svtkEDLBilateralFilterFS.h"
#include "svtkEDLComposeFS.h"
#include "svtkEDLShadeFS.h"
#include "svtkTextureObjectVS.h"

//#define SVTK_EDL_SHADING_DEBUG

namespace
{
void annotate(const std::string& str)
{
  svtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

svtkStandardNewMacro(svtkEDLShading);

// ----------------------------------------------------------------------------
svtkEDLShading::svtkEDLShading()
{

  this->ProjectionFBO = nullptr;
  this->ProjectionColorTexture = nullptr;
  this->ProjectionDepthTexture = nullptr;

  this->EDLHighFBO = nullptr;
  this->EDLHighShadeTexture = nullptr;
  this->EDLLowFBO = nullptr;
  this->EDLLowShadeTexture = nullptr;
  this->EDLLowBlurTexture = nullptr;

  this->EDLIsFiltered = true;
  // init neighbours in image space
  for (int c = 0; c < 8; c++)
  {
    float x, y;
    x = cos(2 * svtkMath::Pi() * float(c) / 8.);
    y = sin(2 * svtkMath::Pi() * float(c) / 8.);
    this->EDLNeighbours[c][0] = x / sqrt(x * x + y * y);
    this->EDLNeighbours[c][1] = y / sqrt(x * x + y * y);
    this->EDLNeighbours[c][2] = 0.;
    this->EDLNeighbours[c][3] = 0.;
  }
  this->EDLLowResFactor = 2;
  this->Zn = 0.1;
  this->Zf = 1.0;
}

// ----------------------------------------------------------------------------
svtkEDLShading::~svtkEDLShading()
{
  if (this->ProjectionFBO != nullptr)
  {
    svtkErrorMacro(<< "FrameBufferObject should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->ProjectionColorTexture != nullptr)
  {
    svtkErrorMacro(<< "ColorTexture should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->ProjectionDepthTexture != nullptr)
  {
    svtkErrorMacro(<< "DepthTexture should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->EDLHighFBO != nullptr)
  {
    svtkErrorMacro(<< "FrameBufferObject should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->EDLHighShadeTexture != nullptr)
  {
    svtkErrorMacro(<< "ColorTexture should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->EDLLowFBO != nullptr)
  {
    svtkErrorMacro(<< "FrameBufferObject should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->EDLLowShadeTexture != nullptr)
  {
    svtkErrorMacro(<< "ColorTexture should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
  if (this->EDLLowBlurTexture != nullptr)
  {
    svtkErrorMacro(<< "ColorTexture should have been deleted in "
                  << "ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void svtkEDLShading::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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

// ----------------------------------------------------------------------------
// Description:
// Initialize framebuffers and associated texture objects,
// with link to render state s
void svtkEDLShading::EDLInitializeFramebuffers(svtkRenderState& s)
{
  svtkRenderer* r = s.GetRenderer();

  svtkOpenGLCheckErrorMacro("failed before Initialize");

  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());

  //  PROJECTION FBO and TEXTURES
  //
  if (this->ProjectionFBO == nullptr)
  {
    this->ProjectionFBO = svtkOpenGLFramebufferObject::New();
    this->ProjectionFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(this->ProjectionFBO);
  renWin->GetState()->PushFramebufferBindings();
  this->ProjectionFBO->Bind();
  // Color texture
  if (this->ProjectionColorTexture == nullptr)
  {
    this->ProjectionColorTexture = svtkTextureObject::New();
    this->ProjectionColorTexture->SetContext(this->ProjectionFBO->GetContext());
  }
  if (this->ProjectionColorTexture->GetWidth() != static_cast<unsigned int>(this->W) ||
    this->ProjectionColorTexture->GetHeight() != static_cast<unsigned int>(this->H))
  {
    this->ProjectionColorTexture->Create2D(this->W, this->H, 4, SVTK_FLOAT, false);
  }
  // Depth texture
  if (this->ProjectionDepthTexture == nullptr)
  {
    this->ProjectionDepthTexture = svtkTextureObject::New();
    this->ProjectionDepthTexture->SetContext(this->ProjectionFBO->GetContext());
  }
  if (this->ProjectionDepthTexture->GetWidth() != static_cast<unsigned int>(this->W) ||
    this->ProjectionDepthTexture->GetHeight() != static_cast<unsigned int>(this->H))
  {
    this->ProjectionDepthTexture->AllocateDepth(this->W, this->H, svtkTextureObject::Float32);
  }

  // Apply textures
  // to make things clear, we write all
  this->ProjectionFBO->AddColorAttachment(0, this->ProjectionColorTexture);
  this->ProjectionFBO->ActivateDrawBuffer(0);
  this->ProjectionFBO->AddDepthAttachment(this->ProjectionDepthTexture);

  this->ProjectionDepthTexture->SetWrapS(svtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetWrapT(svtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetMinificationFilter(svtkTextureObject::Linear);
  this->ProjectionDepthTexture->SetLinearMagnification(true);
  this->ProjectionDepthTexture->Bind();
  this->ProjectionDepthTexture->SendParameters();

  renWin->GetState()->PopFramebufferBindings();

  //  EDL-RES1 FBO and TEXTURE
  //
  if (this->EDLHighFBO == nullptr)
  {
    this->EDLHighFBO = svtkOpenGLFramebufferObject::New();
    this->EDLHighFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(EDLHighFBO);
  // Color texture
  if (this->EDLHighShadeTexture == nullptr)
  {
    this->EDLHighShadeTexture = svtkTextureObject::New();
    this->EDLHighShadeTexture->SetContext(this->EDLHighFBO->GetContext());
  }
  if (this->EDLHighShadeTexture->GetWidth() != static_cast<unsigned int>(this->W) ||
    this->EDLHighShadeTexture->GetHeight() != static_cast<unsigned int>(this->H))
  {
    this->EDLHighShadeTexture->Create2D(this->W, this->H, 4, SVTK_FLOAT, false);
  }
  renWin->GetState()->PushFramebufferBindings();
  this->EDLHighFBO->Bind();
  this->EDLHighFBO->AddColorAttachment(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->ActivateDrawBuffer(0);
  this->EDLHighFBO->AddDepthAttachment();
  renWin->GetState()->PopFramebufferBindings();

  //  EDL-RES2 FBO and TEXTURE
  //
  if (this->EDLLowFBO == nullptr)
  {
    this->EDLLowFBO = svtkOpenGLFramebufferObject::New();
    this->EDLLowFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(EDLLowFBO);
  // Color texture
  if (this->EDLLowShadeTexture == nullptr)
  {
    this->EDLLowShadeTexture = svtkTextureObject::New();
    this->EDLLowShadeTexture->SetContext(this->EDLLowFBO->GetContext());
  }
  if (this->EDLLowShadeTexture->GetWidth() !=
      static_cast<unsigned int>(this->W / EDLLowResFactor) ||
    this->EDLLowShadeTexture->GetHeight() != static_cast<unsigned int>(this->H / EDLLowResFactor))
  {
    this->EDLLowShadeTexture->Create2D(
      this->W / EDLLowResFactor, this->H / EDLLowResFactor, 4, SVTK_FLOAT, false);
  }
  // Blur texture
  if (this->EDLLowBlurTexture == nullptr)
  {
    this->EDLLowBlurTexture = svtkTextureObject::New();
    this->EDLLowBlurTexture->SetContext(this->EDLLowFBO->GetContext());
  }
  if (this->EDLLowBlurTexture->GetWidth() != static_cast<unsigned int>(this->W / EDLLowResFactor) ||
    this->EDLLowBlurTexture->GetHeight() != static_cast<unsigned int>(this->H / EDLLowResFactor))
  {
    this->EDLLowBlurTexture->Create2D(
      this->W / EDLLowResFactor, this->H / EDLLowResFactor, 4, SVTK_FLOAT, false);
  }
  renWin->GetState()->PushFramebufferBindings();
  this->EDLLowFBO->Bind();
  this->EDLLowFBO->AddColorAttachment(0, this->EDLLowShadeTexture);
  this->EDLLowFBO->ActivateDrawBuffer(0);
  this->EDLLowFBO->AddDepthAttachment();

  this->EDLLowShadeTexture->SetWrapS(svtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetWrapT(svtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetMinificationFilter(svtkTextureObject::Linear);
  this->EDLLowShadeTexture->SetLinearMagnification(true);
  this->EDLLowShadeTexture->Bind();
  this->EDLLowShadeTexture->SendParameters();

  this->EDLLowBlurTexture->SetWrapS(svtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetWrapT(svtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetMinificationFilter(svtkTextureObject::Linear);
  this->EDLLowBlurTexture->SetLinearMagnification(true);
  this->EDLLowBlurTexture->Bind();
  this->EDLLowBlurTexture->SendParameters();

  renWin->GetState()->PopFramebufferBindings();

  svtkOpenGLCheckErrorMacro("failed after Initialize");
}
// ----------------------------------------------------------------------------
// Description:
// Initialize shaders
//
void svtkEDLShading::EDLInitializeShaders(svtkOpenGLRenderWindow* renWin)
{
#ifdef SVTK_EDL_SHADING_DEBUG
  cout << "EDL: INITIALIZE SHADERS" << endl;
#endif

  //  EDL SHADE
  //
  if (this->EDLShadeProgram.Program == nullptr)
  {
    this->EDLShadeProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(svtkTextureObjectVS, svtkEDLShadeFS, "");
  }

  //  EDL COMPOSE
  //
  if (this->EDLComposeProgram.Program == nullptr)
  {
    this->EDLComposeProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(svtkTextureObjectVS, svtkEDLComposeFS, "");
  }

  //  BILATERAL FILTER
  //
  if (this->BilateralProgram.Program == nullptr)
  {
    this->BilateralProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(svtkTextureObjectVS, svtkEDLBilateralFilterFS, "");
  }

#ifdef SVTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in full resolution
//
bool svtkEDLShading::EDLShadeHigh(svtkRenderState& s, svtkOpenGLRenderWindow* renWin)
{
  //  VARIABLES
  //
  float d = 1.0;
  float F_scale = 5.0;
  float SX = 1. / float(this->W);
  float SY = 1. / float(this->H);
  float L[3] = { 0., 0., -1. };

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLShadeProgram.Program);

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLHighFBO);
  this->EDLHighShadeTexture->Activate();
  renWin->GetState()->PushFramebufferBindings();
  this->EDLHighFBO->Bind();
  this->EDLHighFBO->AddColorAttachment(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->ActivateDrawBuffer(0);
  this->EDLHighFBO->Start(this->W, this->H);

  // DEPTH TEXTURE PARAMETERS
  this->ProjectionDepthTexture->Activate();
  svtkShaderProgram* prog = this->EDLShadeProgram.Program;

  // shader parameters
  prog->SetUniformi("s2_depth", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("d", d);
  prog->SetUniformf("F_scale", F_scale);
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniform3f("L", L);
  prog->SetUniform4fv("N", 8, this->EDLNeighbours);
  prog->SetUniformf("Znear", this->Zn);
  prog->SetUniformf("Zfar", this->Zf);

  // compute the scene bounding box, and set the scene size to the diagonal of it.
  double bb[6];
  svtkMath::UninitializeBounds(bb);
  bool boundsSet = false;
  for (int i = 0; i < s.GetPropArrayCount(); i++)
  {
    const double* bounds = s.GetPropArray()[i]->GetBounds();
    if (bounds)
    {
      if (!boundsSet)
      {
        bb[0] = bounds[0];
        bb[1] = bounds[1];
        bb[2] = bounds[2];
        bb[3] = bounds[3];
        bb[4] = bounds[4];
        bb[5] = bounds[5];
        boundsSet = true;
      }
      else
      {
        bb[0] = (bb[0] < bounds[0] ? bb[0] : bounds[0]);
        bb[1] = (bb[1] > bounds[1] ? bb[1] : bounds[1]);
        bb[2] = (bb[2] < bounds[2] ? bb[2] : bounds[2]);
        bb[3] = (bb[3] > bounds[3] ? bb[3] : bounds[3]);
        bb[4] = (bb[4] < bounds[4] ? bb[4] : bounds[4]);
        bb[5] = (bb[5] > bounds[5] ? bb[5] : bounds[5]);
      }
    }
  }
  float diag = (bb[1] - bb[0]) * (bb[1] - bb[0]) + (bb[3] - bb[2]) * (bb[3] - bb[2]) +
    (bb[5] - bb[4]) * (bb[5] - bb[4]);
  diag = sqrt(diag);
  prog->SetUniformf("SceneSize", diag);

  // RENDER AND FREE ALL
  this->EDLHighFBO->RenderQuad(0, this->W - 1, 0, this->H - 1, prog, this->EDLShadeProgram.VAO);

  //
  this->ProjectionDepthTexture->Deactivate();
  this->EDLHighShadeTexture->Deactivate();
  renWin->GetState()->PopFramebufferBindings();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in low resolution
//
bool svtkEDLShading::EDLShadeLow(svtkRenderState& s, svtkOpenGLRenderWindow* renWin)
{
  //  VARIABLES
  //
  float d = 2.0;
  float F_scale = 5.0;
  float SX = 1. / float(this->W / this->EDLLowResFactor);
  float SY = 1. / float(this->H / this->EDLLowResFactor);
  float L[3] = { 0., 0., -1. };

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowShadeTexture->Activate();
  this->EDLLowShadeTexture->SetLinearMagnification(true);
  this->EDLLowShadeTexture->SendParameters();
  renWin->GetState()->PushFramebufferBindings();
  this->EDLLowFBO->Bind();
  this->EDLLowFBO->AddColorAttachment(0, this->EDLLowShadeTexture);
  this->EDLLowFBO->ActivateDrawBuffer(0);
  this->EDLLowFBO->Start(this->W / this->EDLLowResFactor, this->H / this->EDLLowResFactor);

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLShadeProgram.Program);
  // DEPTH TEXTURE PARAMETERS
  svtkShaderProgram* prog = this->EDLShadeProgram.Program;
  this->ProjectionDepthTexture->Activate();
  // shader parameters
  prog->SetUniformi("s2_depth", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("d", d);
  prog->SetUniformf("F_scale", F_scale);
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniform3f("L", L);
  prog->SetUniform4fv("N", 8, this->EDLNeighbours); // USELESS, ALREADY DEFINED IN FULL RES
  prog->SetUniformf("Znear", this->Zn);
  prog->SetUniformf("Zfar", this->Zf);

  // RENDER AND FREE ALL
  //
  this->EDLLowFBO->RenderQuad(0, this->W / this->EDLLowResFactor - 1, 0,
    this->H / this->EDLLowResFactor - 1, prog, this->EDLShadeProgram.VAO);

  this->ProjectionDepthTexture->Deactivate();
  this->EDLLowShadeTexture->Deactivate();
  renWin->GetState()->PopFramebufferBindings();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Bilateral Filter low resolution shaded image
//
bool svtkEDLShading::EDLBlurLow(svtkRenderState& s, svtkOpenGLRenderWindow* renWin)
{
  // shader parameters
  float SX = 1. / float(this->W / this->EDLLowResFactor);
  float SY = 1. / float(this->H / this->EDLLowResFactor);
  int EDL_Bilateral_N = 5;
  float EDL_Bilateral_Sigma = 2.5;

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->BilateralProgram.Program);

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowBlurTexture->Activate();
  renWin->GetState()->PushFramebufferBindings();
  this->EDLLowFBO->Bind();
  this->EDLLowFBO->AddColorAttachment(0, this->EDLLowBlurTexture);
  this->EDLLowFBO->ActivateDrawBuffer(0);
  this->EDLLowFBO->Start(this->W / EDLLowResFactor, this->H / EDLLowResFactor);

  // DEPTH TEXTURE PARAMETERS
  svtkShaderProgram* prog = this->BilateralProgram.Program;

  // DEPTH TEXTURE PARAMETERS
  this->EDLLowShadeTexture->Activate();
  this->ProjectionDepthTexture->Activate();

  // shader parameters
  prog->SetUniformi("s2_I", this->EDLLowShadeTexture->GetTextureUnit());
  prog->SetUniformi("s2_D", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniformi("N", EDL_Bilateral_N);
  prog->SetUniformf("sigma", EDL_Bilateral_Sigma);

  this->EDLLowFBO->RenderQuad(0, this->W / this->EDLLowResFactor - 1, 0,
    this->H / this->EDLLowResFactor - 1, prog, this->BilateralProgram.VAO);

  this->EDLLowBlurTexture->Deactivate();
  this->EDLLowShadeTexture->Deactivate();
  this->ProjectionDepthTexture->Deactivate();

  renWin->GetState()->PopFramebufferBindings();

  return EDLIsFiltered;
}

// ----------------------------------------------------------------------------
// Description:
// Compose color and shaded images
//
bool svtkEDLShading::EDLCompose(const svtkRenderState*, svtkOpenGLRenderWindow* renWin)
{
  //  this->EDLIsFiltered = true;

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLComposeProgram.Program);
  svtkOpenGLState* ostate = renWin->GetState();

  // DEPTH TEXTURE PARAMETERS
  svtkShaderProgram* prog = this->EDLComposeProgram.Program;

  //  EDL shaded texture - full res
  this->EDLHighShadeTexture->Activate();
  prog->SetUniformi("s2_S1", this->EDLHighShadeTexture->GetTextureUnit());

  //  EDL shaded texture - low res
  // this->EDLLowBlurTexture->SetLinearMagnification(true);
  // this->EDLLowBlurTexture->SendParameters();
  if (this->EDLIsFiltered)
  {
    this->EDLLowBlurTexture->Activate();
    prog->SetUniformi("s2_S2", this->EDLLowBlurTexture->GetTextureUnit());
  }
  else
  {
    this->EDLLowShadeTexture->Activate();
    prog->SetUniformi("s2_S2", this->EDLLowShadeTexture->GetTextureUnit());
  }

  //  initial color texture
  this->ProjectionColorTexture->Activate();
  prog->SetUniformi("s2_C", this->ProjectionColorTexture->GetTextureUnit());

  //  DRAW CONTEXT - prepare blitting
  //
  // Prepare blitting
  ostate->svtkglClearColor(1., 1., 1., 1.);
  ostate->svtkglClearDepth(1.0);
  ostate->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // IMPORTANT since we enable depth writing hereafter
  ostate->svtkglDisable(GL_BLEND);
  ostate->svtkglEnable(GL_DEPTH_TEST);
  // IMPORTANT : so that depth information is propagated
  ostate->svtkglDisable(GL_SCISSOR_TEST);

  int blitSize[2] = { this->W - 1 - 2 * this->ExtraPixels, this->H - 1 - 2 * this->ExtraPixels };

  this->EDLHighShadeTexture->CopyToFrameBuffer(this->ExtraPixels, this->ExtraPixels, blitSize[0],
    blitSize[1], this->Origin[0], this->Origin[1], this->Origin[0] + blitSize[0],
    this->Origin[1] + blitSize[1], prog, this->EDLComposeProgram.VAO);

  //  FREE ALL
  //
  if (this->EDLIsFiltered)
  {
    this->EDLLowBlurTexture->Deactivate();
  }
  else
  {
    this->EDLLowShadeTexture->Deactivate();
  }
  this->EDLHighShadeTexture->Deactivate();
  this->ProjectionColorTexture->Deactivate();

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkEDLShading::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);
  annotate("Start svtkEDLShading::Render");

  this->NumberOfRenderedProps = 0;
  svtkRenderer* r = s->GetRenderer();
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());

  if (this->DelegatePass != nullptr)
  {

    //////////////////////////////////////////////////////
    //
    //  2. DEFINE SIZE and ACCORDING RENDER STATE
    //
    this->ReadWindowSize(s);
    // this->extraPixels = 20; Obsolete
    this->ExtraPixels = 0; // extra pixels to zero in the new system
    this->W = this->Width + 2 * this->ExtraPixels;
    this->H = this->Height + 2 * this->ExtraPixels;
    svtkRenderState s2(r);
    s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());

    //////////////////////////////////////////////////////
    //
    // 3. INITIALIZE FBOs and SHADERS
    //
    //  FBOs
    //
    annotate("Start svtkEDLShading Initialization");
    this->EDLInitializeFramebuffers(s2);
    //  Shaders
    //
    this->EDLInitializeShaders(renWin);
    annotate("End svtkEDLShading Initialization");

    if (this->EDLShadeProgram.Program == nullptr || this->EDLComposeProgram.Program == nullptr ||
      this->BilateralProgram.Program == nullptr)
    {
      return;
    }

    //////////////////////////////////////////////////////
    //
    // 4. DELEGATE RENDER IN PROJECTION FBO
    //
    //
    double znear, zfar;
    r->GetActiveCamera()->GetClippingRange(znear, zfar);
    this->Zf = zfar;
    this->Zn = znear;
    // cout << " -- ZNEAR/ZFAR : " << Zn << " || " << Zf << endl;
    renWin->GetState()->PushFramebufferBindings();
    this->ProjectionFBO->Bind();
    annotate("Start svtkEDLShading::RenderDelegate");
    this->RenderDelegate(s, this->Width, this->Height, this->W, this->H, this->ProjectionFBO,
      this->ProjectionColorTexture, this->ProjectionDepthTexture);
    annotate("End svtkEDLShading::RenderDelegate");

    this->ProjectionFBO->UnBind();

    // system("PAUSE");

    //////////////////////////////////////////////////////
    //
    // 5. EDL SHADING PASS - FULL RESOLUTION
    //
#if EDL_HIGH_RESOLUTION_ON
    annotate("Start svtkEDLShading::ShadeHigh");
    if (!this->EDLShadeHigh(s2, renWin))
    {
      renWin->GetState()->PopFramebufferBindings();
    }
    annotate("End svtkEDLShading::ShadeHigh");
#endif // EDL_HIGH_RESOLUTION_ON

    //////////////////////////////////////////////////////
    //
    // 6. EDL SHADING PASS - LOW RESOLUTION + blur pass
    //
#if EDL_LOW_RESOLUTION_ON
    annotate("Start svtkEDLShading::ShadeLow");
    if (!this->EDLShadeLow(s2, renWin))
    {
      renWin->GetState()->PopFramebufferBindings();
    }
    annotate("End svtkEDLShading::ShadeLow");
    if (this->EDLIsFiltered)
    {
      annotate("Start svtkEDLShading::BlurLow");
      this->EDLBlurLow(s2, renWin);
      annotate("End svtkEDLShading::BlurLow");
    }
#endif // EDL_LOW_RESOLUTION_ON

    //////////////////////////////////////////////////////
    //
    // 7. COMPOSITING PASS (in original framebuffer)
    //
    if (s->GetFrameBuffer() != nullptr)
    {
      svtkOpenGLFramebufferObject::SafeDownCast(s->GetFrameBuffer())->Bind();
    }
    renWin->GetState()->PopFramebufferBindings();

    annotate("Start svtkEDLShading::Compose");
    if (!this->EDLCompose(s, renWin))
    {
      return;
    }
    annotate("End svtkEDLShading::Compose");
  }
  else
  {
    svtkWarningMacro(<< " no delegate.");
  }

  annotate("END svtkEDLShading::Render");
}

// --------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkEDLShading::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  //  SHADERS
  this->EDLShadeProgram.ReleaseGraphicsResources(w);
  this->EDLComposeProgram.ReleaseGraphicsResources(w);
  this->BilateralProgram.ReleaseGraphicsResources(w);

  // FBOs and TOs
  //
  if (this->ProjectionFBO != nullptr)
  {
    this->ProjectionFBO->Delete();
    this->ProjectionFBO = nullptr;
  }
  if (this->ProjectionColorTexture != nullptr)
  {
    this->ProjectionColorTexture->Delete();
    this->ProjectionColorTexture = nullptr;
  }
  if (this->ProjectionDepthTexture != nullptr)
  {
    this->ProjectionDepthTexture->Delete();
    this->ProjectionDepthTexture = nullptr;
  }
  if (this->EDLHighFBO != nullptr)
  {
    this->EDLHighFBO->Delete();
    this->EDLHighFBO = nullptr;
  }
  if (this->EDLHighShadeTexture != nullptr)
  {
    this->EDLHighShadeTexture->Delete();
    this->EDLHighShadeTexture = nullptr;
  }
  if (this->EDLLowFBO != nullptr)
  {
    this->EDLLowFBO->Delete();
    this->EDLLowFBO = nullptr;
  }
  if (this->EDLLowShadeTexture != nullptr)
  {
    this->EDLLowShadeTexture->Delete();
    this->EDLLowShadeTexture = nullptr;
  }
  if (this->EDLLowBlurTexture != nullptr)
  {
    this->EDLLowBlurTexture->Delete();
    this->EDLLowBlurTexture = nullptr;
  }

  this->Superclass::ReleaseGraphicsResources(w);
}
