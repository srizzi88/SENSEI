/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkDepthPeelingPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDepthPeelingPass.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkProp.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include <cassert>
#include <list>

#include "svtkRenderStepsPass.h"

#include "svtkOpenGLHelper.h"

// the 2D blending shaders we use
#include "svtkDepthPeelingPassFinalFS.h"
#include "svtkDepthPeelingPassIntermediateFS.h"

svtkStandardNewMacro(svtkDepthPeelingPass);
svtkCxxSetObjectMacro(svtkDepthPeelingPass, TranslucentPass, svtkRenderPass);

// ----------------------------------------------------------------------------
svtkDepthPeelingPass::svtkDepthPeelingPass()
  : Framebuffer(nullptr)
{
  this->TranslucentPass = nullptr;

  this->OcclusionRatio = 0.0;
  this->MaximumNumberOfPeels = 4;

  this->IntermediateBlend = nullptr;
  this->FinalBlend = nullptr;

  this->OpaqueRGBATexture = nullptr;
  this->OpaqueZTexture = nullptr;
  this->OwnOpaqueZTexture = false;
  this->OwnOpaqueRGBATexture = false;

  this->TranslucentZTexture[0] = svtkTextureObject::New();
  this->TranslucentZTexture[1] = svtkTextureObject::New();
  this->DepthFormat = svtkTextureObject::Float32;

  for (int i = 0; i < 3; i++)
  {
    this->TranslucentRGBATexture[i] = svtkTextureObject::New();
  }

  this->ViewportX = 0;
  this->ViewportY = 0;
  this->ViewportWidth = 100;
  this->ViewportHeight = 100;
}

// ----------------------------------------------------------------------------
svtkDepthPeelingPass::~svtkDepthPeelingPass()
{
  if (this->TranslucentPass != nullptr)
  {
    this->TranslucentPass->Delete();
  }
  if (this->OpaqueZTexture)
  {
    this->OpaqueZTexture->UnRegister(this);
    this->OpaqueZTexture = nullptr;
  }
  if (this->TranslucentZTexture[0])
  {
    this->TranslucentZTexture[0]->UnRegister(this);
    this->TranslucentZTexture[0] = nullptr;
  }
  if (this->TranslucentZTexture[1])
  {
    this->TranslucentZTexture[1]->UnRegister(this);
    this->TranslucentZTexture[1] = nullptr;
  }
  if (this->OpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->UnRegister(this);
    this->OpaqueRGBATexture = nullptr;
  }
  for (int i = 0; i < 3; i++)
  {
    if (this->TranslucentRGBATexture[i])
    {
      this->TranslucentRGBATexture[i]->UnRegister(this);
      this->TranslucentRGBATexture[i] = nullptr;
    }
  }
  if (this->Framebuffer)
  {
    this->Framebuffer->UnRegister(this);
    this->Framebuffer = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
void svtkDepthPeelingPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  if (this->FinalBlend != nullptr)
  {
    delete this->FinalBlend;
    this->FinalBlend = nullptr;
  }
  if (this->IntermediateBlend != nullptr)
  {
    delete this->IntermediateBlend;
    this->IntermediateBlend = nullptr;
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->ReleaseGraphicsResources(w);
  }
  if (this->OpaqueZTexture)
  {
    this->OpaqueZTexture->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentZTexture[0])
  {
    this->TranslucentZTexture[0]->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentZTexture[1])
  {
    this->TranslucentZTexture[1]->ReleaseGraphicsResources(w);
  }
  if (this->OpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->ReleaseGraphicsResources(w);
  }
  for (int i = 0; i < 3; i++)
  {
    if (this->TranslucentRGBATexture[i])
    {
      this->TranslucentRGBATexture[i]->ReleaseGraphicsResources(w);
    }
  }
  if (this->Framebuffer)
  {
    this->Framebuffer->ReleaseGraphicsResources(w);
    this->Framebuffer->UnRegister(this);
    this->Framebuffer = nullptr;
  }
}

void svtkDepthPeelingPass::SetOpaqueZTexture(svtkTextureObject* to)
{
  if (this->OpaqueZTexture == to)
  {
    return;
  }
  if (this->OpaqueZTexture)
  {
    this->OpaqueZTexture->Delete();
  }
  this->OpaqueZTexture = to;
  if (to)
  {
    to->Register(this);
  }
  this->OwnOpaqueZTexture = false;
  this->Modified();
}

void svtkDepthPeelingPass::SetOpaqueRGBATexture(svtkTextureObject* to)
{
  if (this->OpaqueRGBATexture == to)
  {
    return;
  }
  if (this->OpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->Delete();
  }
  this->OpaqueRGBATexture = to;
  if (to)
  {
    to->Register(this);
  }
  this->OwnOpaqueRGBATexture = false;
  this->Modified();
}

// ----------------------------------------------------------------------------
void svtkDepthPeelingPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OcclusionRatio: " << this->OcclusionRatio << endl;

  os << indent << "MaximumNumberOfPeels: " << this->MaximumNumberOfPeels << endl;

  os << indent << "TranslucentPass:";
  if (this->TranslucentPass != nullptr)
  {
    this->TranslucentPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

void svtkDepthPeelingPassCreateTexture(svtkTextureObject* to, svtkOpenGLRenderWindow* context,
  int width, int height, int numComponents, bool isDepth, int depthFormat)
{
  to->SetContext(context);
  if (isDepth == true)
  {
    to->AllocateDepth(width, height, depthFormat);
  }
  else
  {
    to->Allocate2D(width, height, numComponents, SVTK_UNSIGNED_CHAR);
  }

  to->SetMinificationFilter(svtkTextureObject::Nearest);
  to->SetMagnificationFilter(svtkTextureObject::Nearest);
  to->SetWrapS(svtkTextureObject::ClampToEdge);
  to->SetWrapT(svtkTextureObject::ClampToEdge);
}

void svtkDepthPeelingPass::BlendIntermediatePeels(svtkOpenGLRenderWindow* renWin, bool done)
{
  // take the TranslucentRGBA texture and blend it with the current frame buffer
  if (!this->IntermediateBlend)
  {
    this->IntermediateBlend =
      new svtkOpenGLQuadHelper(renWin, nullptr, svtkDepthPeelingPassIntermediateFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->IntermediateBlend->Program);
  }
  this->IntermediateBlend->Program->SetUniformi("translucentRGBATexture",
    this->TranslucentRGBATexture[(this->ColorDrawCount - 2) % 3]->GetTextureUnit());
  this->IntermediateBlend->Program->SetUniformi("currentRGBATexture",
    this->TranslucentRGBATexture[(this->ColorDrawCount - 1) % 3]->GetTextureUnit());
  this->IntermediateBlend->Program->SetUniformi("lastpass", done ? 1 : 0);

  this->State->svtkglDisable(GL_DEPTH_TEST);

  this->Framebuffer->AddColorAttachment(0, this->TranslucentRGBATexture[this->ColorDrawCount % 3]);
  this->ColorDrawCount++;

  this->IntermediateBlend->Render();
}

void svtkDepthPeelingPass::BlendFinalPeel(svtkOpenGLRenderWindow* renWin)
{
  if (!this->FinalBlend)
  {
    this->FinalBlend = new svtkOpenGLQuadHelper(renWin, nullptr, svtkDepthPeelingPassFinalFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->FinalBlend->Program);
  }

  if (this->FinalBlend->Program)
  {
    this->FinalBlend->Program->SetUniformi("translucentRGBATexture",
      this->TranslucentRGBATexture[(this->ColorDrawCount - 1) % 3]->GetTextureUnit());

    // Store the current active texture
    svtkOpenGLState::ScopedglActiveTexture(this->State);

    this->OpaqueRGBATexture->Activate();
    this->FinalBlend->Program->SetUniformi(
      "opaqueRGBATexture", this->OpaqueRGBATexture->GetTextureUnit());

    this->OpaqueZTexture->Activate();
    this->FinalBlend->Program->SetUniformi(
      "opaqueZTexture", this->OpaqueZTexture->GetTextureUnit());

    this->Framebuffer->AddColorAttachment(
      0, this->TranslucentRGBATexture[this->ColorDrawCount % 3]);
    this->ColorDrawCount++;

    // blend in OpaqueRGBA
    this->State->svtkglEnable(GL_DEPTH_TEST);
    this->State->svtkglDepthFunc(GL_ALWAYS);

    // do we need to set the viewport
    this->FinalBlend->Render();
  }
  this->State->svtkglDepthFunc(GL_LEQUAL);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkDepthPeelingPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;

  if (this->TranslucentPass == nullptr)
  {
    svtkWarningMacro(<< "No TranslucentPass delegate set. Nothing can be rendered.");
    return;
  }

  // Any prop to render?
  bool hasTranslucentPolygonalGeometry = false;
  int i = 0;
  while (!hasTranslucentPolygonalGeometry && i < s->GetPropArrayCount())
  {
    hasTranslucentPolygonalGeometry = s->GetPropArray()[i]->HasTranslucentPolygonalGeometry() == 1;
    ++i;
  }
  if (!hasTranslucentPolygonalGeometry)
  {
    return; // nothing to render.
  }

  // check driver support
  svtkOpenGLRenderWindow* renWin =
    svtkOpenGLRenderWindow::SafeDownCast(s->GetRenderer()->GetRenderWindow());
  this->State = renWin->GetState();

  // we need alpha planes
  int rgba[4];
  renWin->GetColorBufferSizes(rgba);

  if (rgba[3] < 8)
  {
    // just use alpha blending
    this->TranslucentPass->Render(s);
    return;
  }

  // Depth peeling.
  svtkRenderer* r = s->GetRenderer();

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

  // create textures we need if not done already
  if (this->TranslucentRGBATexture[0]->GetHandle() == 0)
  {
    for (i = 0; i < 3; i++)
    {
      svtkDepthPeelingPassCreateTexture(this->TranslucentRGBATexture[i], renWin, this->ViewportWidth,
        this->ViewportHeight, 4, false, 0);
    }
    svtkDepthPeelingPassCreateTexture(this->TranslucentZTexture[0], renWin, this->ViewportWidth,
      this->ViewportHeight, 1, true, this->DepthFormat);
    svtkDepthPeelingPassCreateTexture(this->TranslucentZTexture[1], renWin, this->ViewportWidth,
      this->ViewportHeight, 1, true, this->DepthFormat);
    if (!this->OpaqueZTexture)
    {
      this->OwnOpaqueZTexture = true;
      this->OpaqueZTexture = svtkTextureObject::New();
      svtkDepthPeelingPassCreateTexture(this->OpaqueZTexture, renWin, this->ViewportWidth,
        this->ViewportHeight, 1, true, this->DepthFormat);
    }
    if (!this->OpaqueRGBATexture)
    {
      this->OwnOpaqueRGBATexture = true;
      this->OpaqueRGBATexture = svtkTextureObject::New();
      svtkDepthPeelingPassCreateTexture(
        this->OpaqueRGBATexture, renWin, this->ViewportWidth, this->ViewportHeight, 4, false, 0);
    }
  }

  for (i = 0; i < 3; i++)
  {
    this->TranslucentRGBATexture[i]->Resize(this->ViewportWidth, this->ViewportHeight);
  }
  this->TranslucentZTexture[0]->Resize(this->ViewportWidth, this->ViewportHeight);
  this->TranslucentZTexture[1]->Resize(this->ViewportWidth, this->ViewportHeight);

  if (this->OwnOpaqueZTexture)
  {
    this->OpaqueZTexture->Resize(this->ViewportWidth, this->ViewportHeight);
    this->OpaqueZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY, this->ViewportX,
      this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  }

  if (this->OwnOpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->Resize(this->ViewportWidth, this->ViewportHeight);
    this->OpaqueRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY, this->ViewportX,
      this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  }

  if (!this->Framebuffer)
  {
    this->Framebuffer = svtkOpenGLFramebufferObject::New();
    this->Framebuffer->SetContext(renWin);
  }
  this->State->PushFramebufferBindings();
  this->Framebuffer->Bind();
  this->Framebuffer->AddDepthAttachment(this->TranslucentZTexture[0]);
  this->Framebuffer->AddColorAttachment(0, this->TranslucentRGBATexture[0]);

  this->State->svtkglViewport(0, 0, this->ViewportWidth, this->ViewportHeight);
  bool saveScissorTestState = this->State->GetEnumState(GL_SCISSOR_TEST);
  this->State->svtkglDisable(GL_SCISSOR_TEST);

  this->State->svtkglClearDepth(static_cast<GLclampf>(0.0));
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  this->State->svtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  this->State->svtkglClearColor(0.0, 0.0, 0.0, 0.0); // always clear to black
  this->State->svtkglClearDepth(static_cast<GLclampf>(1.0));
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  this->Framebuffer->AddDepthAttachment(this->TranslucentZTexture[1]);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GL_MULTISAMPLE
  bool multiSampleStatus = this->State->GetEnumState(GL_MULTISAMPLE);
  this->State->svtkglDisable(GL_MULTISAMPLE);
#endif
  this->State->svtkglDisable(GL_BLEND);

  // Store the current active texture
  svtkOpenGLState::ScopedglActiveTexture(this->State);

  this->TranslucentZTexture[0]->Activate();
  this->OpaqueZTexture->Activate();

  this->TranslucentRGBATexture[0]->Activate();
  this->TranslucentRGBATexture[1]->Activate();
  this->TranslucentRGBATexture[2]->Activate();

  // Setup property keys for actors:
  this->PreRender(s);

  // Enable the depth buffer (otherwise it's disabled for translucent geometry)
  assert("Render state valid." && s);
  int numProps = s->GetPropArrayCount();
  for (int j = 0; j < numProps; ++j)
  {
    svtkProp* prop = s->GetPropArray()[j];
    svtkInformation* info = prop->GetPropertyKeys();
    if (!info)
    {
      info = svtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Set(svtkOpenGLActor::GLDepthMaskOverride(), 1);
  }

  // Do render loop until complete
  unsigned int threshold =
    static_cast<unsigned int>(this->ViewportWidth * this->ViewportHeight * OcclusionRatio);

#ifndef GL_ES_VERSION_3_0
  GLuint queryId;
  glGenQueries(1, &queryId);
#endif

  bool done = false;
  GLuint nbPixels = threshold + 1;
  this->PeelCount = 0;
  this->ColorDrawCount = 0;
  this->State->svtkglDepthFunc(GL_LEQUAL);
  while (!done)
  {
    this->State->svtkglDepthMask(GL_TRUE);
    this->State->svtkglEnable(GL_DEPTH_TEST);

    this->Framebuffer->AddColorAttachment(
      0, this->TranslucentRGBATexture[this->ColorDrawCount % 3]);
    this->ColorDrawCount++;

    // clear the zbuffer and color buffers
    this->State->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render the translucent geometry
#ifndef GL_ES_VERSION_3_0
    glBeginQuery(GL_SAMPLES_PASSED, queryId);
#endif

    // check if we are going to exceed the max number of peels or if we
    // exceeded the pixel threshold last time
    this->PeelCount++;
    if ((this->MaximumNumberOfPeels && this->PeelCount >= this->MaximumNumberOfPeels) ||
      nbPixels <= threshold)
    {
      done = true;
      // if so we do this last render using alpha blending for all
      // the stuff that is left
      this->State->svtkglEnable(GL_BLEND);
      this->State->svtkglDepthFunc(GL_ALWAYS);
    }
    this->TranslucentPass->Render(s);
    this->State->svtkglDepthFunc(GL_LEQUAL);
    this->State->svtkglDisable(GL_BLEND);

#ifndef GL_ES_VERSION_3_0
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &nbPixels);
#endif
    // cerr << "Pass " << peelCount << " pixels Drawn " << nbPixels << "\n";

    // if something was drawn, blend it in
    if (nbPixels > 0)
    {
      // update translucentZ pingpong the textures
      if (this->PeelCount % 2)
      {
        this->TranslucentZTexture[0]->Deactivate();
        this->Framebuffer->AddDepthAttachment(this->TranslucentZTexture[0]);
        this->TranslucentZTexture[1]->Activate();
      }
      else
      {
        this->TranslucentZTexture[1]->Deactivate();
        this->Framebuffer->AddDepthAttachment(this->TranslucentZTexture[1]);
        this->TranslucentZTexture[0]->Activate();
      }

      // blend the last two peels together
      if (this->PeelCount > 1)
      {
        this->BlendIntermediatePeels(renWin, done);
      }
    }
    else // if we drew nothing we are done
    {
      this->ColorDrawCount--;
      done = true;
    }
  }

  //  std::cout << "Number of peels: " << peelCount << "\n";

  // do the final blend if anything was drawn
  // something is drawn only when ColorDrawCount
  // is not zero or PeelCount is > 1
  if (this->PeelCount > 1 || this->ColorDrawCount != 0)
  {
    this->BlendFinalPeel(renWin);
  }

  this->State->PopFramebufferBindings();

  // Restore the original viewport and scissor test settings
  this->State->svtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  if (saveScissorTestState)
  {
    this->State->svtkglEnable(GL_SCISSOR_TEST);
  }
  else
  {
    this->State->svtkglDisable(GL_SCISSOR_TEST);
  }

  // blit if we drew something
  if (this->PeelCount > 1 || this->ColorDrawCount != 0)
  {
    this->State->PushReadFramebufferBinding();
    this->Framebuffer->Bind(this->Framebuffer->GetReadMode());

    glBlitFramebuffer(0, 0, this->ViewportWidth, this->ViewportHeight, this->ViewportX,
      this->ViewportY, this->ViewportX + this->ViewportWidth,
      this->ViewportY + this->ViewportHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    this->State->PopReadFramebufferBinding();
  }

#ifdef GL_MULTISAMPLE
  if (multiSampleStatus)
  {
    this->State->svtkglEnable(GL_MULTISAMPLE);
  }
#endif

  // unload the textures
  this->OpaqueZTexture->Deactivate();
  this->OpaqueRGBATexture->Deactivate();
  this->TranslucentRGBATexture[0]->Deactivate();
  this->TranslucentRGBATexture[1]->Deactivate();
  this->TranslucentRGBATexture[2]->Deactivate();
  this->TranslucentZTexture[0]->Deactivate();
  this->TranslucentZTexture[1]->Deactivate();

  // restore blending
  this->State->svtkglEnable(GL_BLEND);

  this->PostRender(s);
  for (int j = 0; j < numProps; ++j)
  {
    svtkProp* prop = s->GetPropArray()[j];
    svtkInformation* info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(svtkOpenGLActor::GLDepthMaskOverride());
    }
  }

  this->NumberOfRenderedProps = this->TranslucentPass->GetNumberOfRenderedProps();

  svtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool svtkDepthPeelingPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, svtkAbstractMapper*, svtkProp*)
{
  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Dec",
    "uniform vec2 vpSize;\n"
    "uniform sampler2D opaqueZTexture;\n"
    "uniform sampler2D translucentZTexture;\n");

  // Set gl_FragDepth if it isn't set already. It may have already been replaced
  // by the mapper, in which case the substitution will fail and the previously
  // set depth value will be used.
  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z;");

  // the .0000001 below is an epsilon.  It turns out that
  // graphics cards can render the same polygon two times
  // in a row with different z values. I suspect it has to
  // do with how rasterization of the polygon is broken up.
  // A different breakup across fragment shaders can result in
  // very slightly different z values for some of the pixels.
  // The end result is that with depth peeling, you can end up
  // counting/accumulating pixels of the same surface twice
  // simply due to this randomness in z values. So we introduce
  // an epsilon into the transparent test to require some
  // minimal z separation between pixels
  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Impl",
    "vec2 dpTexCoord = gl_FragCoord.xy / vpSize;\n"
    "  float odepth = texture2D(opaqueZTexture, dpTexCoord).r;\n"
    "  if (gl_FragDepth >= odepth) { discard; }\n"
    "  float tdepth = texture2D(translucentZTexture, dpTexCoord).r;\n"
    "  if (gl_FragDepth <= tdepth + .0000001) { discard; }\n");

  return true;
}

//------------------------------------------------------------------------------
bool svtkDepthPeelingPass::SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper*,
  svtkProp*, svtkOpenGLVertexArrayObject* svtkNotUsed(VAO))
{
  program->SetUniformi("opaqueZTexture", this->OpaqueZTexture->GetTextureUnit());
  program->SetUniformi(
    "translucentZTexture", this->TranslucentZTexture[(this->PeelCount + 1) % 2]->GetTextureUnit());

  float vpSize[2] = { static_cast<float>(this->ViewportWidth),
    static_cast<float>(this->ViewportHeight) };
  program->SetUniform2f("vpSize", vpSize);

  return true;
}
