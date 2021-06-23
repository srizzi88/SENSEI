/*=========================================================================

  Prograxq:   Visualization Toolkit
  Module:    svtkOSPRayPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtk_glew.h>

#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkFrameBufferObjectBase.h"
#include "svtkLightsPass.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayViewNodeFactory.h"
#include "svtkObjectFactory.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOverlayPass.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkSequencePass.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkVolumetricPass.h"

#include "RTWrapper/RTWrapper.h"

class svtkOSPRayPassInternals : public svtkRenderPass
{
public:
  static svtkOSPRayPassInternals* New();
  svtkTypeMacro(svtkOSPRayPassInternals, svtkRenderPass);

  svtkOSPRayPassInternals() = default;

  ~svtkOSPRayPassInternals() override { delete this->QuadHelper; }

  void Init(svtkOpenGLRenderWindow* context)
  {
    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
      "uniform sampler2D colorTexture;\n"
      "uniform sampler2D depthTexture;\n");

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
      "gl_FragData[0] = texture(colorTexture, texCoord);\n"
      "gl_FragDepth = texture(depthTexture, texCoord).r;\n");

    this->QuadHelper = new svtkOpenGLQuadHelper(context,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->ColorTexture->SetContext(context);
    this->ColorTexture->AutoParametersOff();
    this->DepthTexture->SetContext(context);
    this->DepthTexture->AutoParametersOff();
    this->SharedColorTexture->SetContext(context);
    this->SharedColorTexture->AutoParametersOff();
    this->SharedDepthTexture->SetContext(context);
    this->SharedDepthTexture->AutoParametersOff();
  }

  void Render(const svtkRenderState* s) override { this->Parent->RenderInternal(s); }

  svtkNew<svtkOSPRayViewNodeFactory> Factory;
  svtkOSPRayPass* Parent = nullptr;

  // OpenGL-based display
  svtkOpenGLQuadHelper* QuadHelper = nullptr;
  svtkNew<svtkTextureObject> ColorTexture;
  svtkNew<svtkTextureObject> DepthTexture;
  svtkNew<svtkTextureObject> SharedColorTexture;
  svtkNew<svtkTextureObject> SharedDepthTexture;
};

int svtkOSPRayPass::RTDeviceRefCount = 0;

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOSPRayPassInternals);

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOSPRayPass);

// ----------------------------------------------------------------------------
svtkOSPRayPass::svtkOSPRayPass()
{
  this->SceneGraph = nullptr;

  svtkOSPRayPass::RTInit();

  this->Internal = svtkOSPRayPassInternals::New();
  this->Internal->Parent = this;

  this->CameraPass = svtkCameraPass::New();
  this->LightsPass = svtkLightsPass::New();
  this->SequencePass = svtkSequencePass::New();
  this->VolumetricPass = svtkVolumetricPass::New();
  this->OverlayPass = svtkOverlayPass::New();

  this->RenderPassCollection = svtkRenderPassCollection::New();
  this->RenderPassCollection->AddItem(this->LightsPass);
  this->RenderPassCollection->AddItem(this->Internal);
  this->RenderPassCollection->AddItem(this->OverlayPass);

  this->SequencePass->SetPasses(this->RenderPassCollection);
  this->CameraPass->SetDelegatePass(this->SequencePass);

  this->PreviousType = "none";
}

// ----------------------------------------------------------------------------
svtkOSPRayPass::~svtkOSPRayPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = 0;
  if (this->CameraPass)
  {
    this->CameraPass->Delete();
    this->CameraPass = 0;
  }
  if (this->LightsPass)
  {
    this->LightsPass->Delete();
    this->LightsPass = 0;
  }
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = 0;
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->Delete();
    this->VolumetricPass = 0;
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->Delete();
    this->OverlayPass = 0;
  }
  if (this->RenderPassCollection)
  {
    this->RenderPassCollection->Delete();
    this->RenderPassCollection = 0;
  }
  svtkOSPRayPass::RTShutdown();
}

// ----------------------------------------------------------------------------
void svtkOSPRayPass::RTInit()
{
  if (RTDeviceRefCount == 0)
  {
    rtwInit();
  }
  RTDeviceRefCount++;
}

// ----------------------------------------------------------------------------
void svtkOSPRayPass::RTShutdown()
{
  --RTDeviceRefCount;
  if (RTDeviceRefCount == 0)
  {
    rtwShutdown();
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkOSPRayPass, SceneGraph, svtkOSPRayRendererNode);

// ----------------------------------------------------------------------------
void svtkOSPRayPass::Render(const svtkRenderState* s)
{
  svtkRenderer* ren = s->GetRenderer();
  if (ren)
  {
    std::string type = svtkOSPRayRendererNode::GetRendererType(ren);
    if (this->PreviousType != type && this->SceneGraph)
    {
      this->SceneGraph->Delete();
      this->SceneGraph = nullptr;
    }
    if (!this->SceneGraph)
    {
      this->SceneGraph =
        svtkOSPRayRendererNode::SafeDownCast(this->Internal->Factory->CreateNode(ren));
    }
    this->PreviousType = type;
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
void svtkOSPRayPass::RenderInternal(const svtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  if (this->SceneGraph)
  {
    svtkRenderer* ren = s->GetRenderer();

    svtkFrameBufferObjectBase* fbo = s->GetFrameBuffer();
    int viewportX, viewportY;
    int viewportWidth, viewportHeight;
    double tileViewport[4];
    int tileScale[2];
    if (fbo)
    {
      viewportX = 0;
      viewportY = 0;
      fbo->GetLastSize(viewportWidth, viewportHeight);

      tileViewport[0] = tileViewport[1] = 0.0;
      tileViewport[2] = tileViewport[3] = 1.0;
      tileScale[0] = tileScale[1] = 1;
    }
    else
    {
      ren->GetTiledSizeAndOrigin(&viewportWidth, &viewportHeight, &viewportX, &viewportY);

      svtkWindow* win = ren->GetSVTKWindow();
      win->GetTileViewport(tileViewport);
      win->GetTileScale(tileScale);
    }

    svtkOSPRayRendererNode* oren =
      svtkOSPRayRendererNode::SafeDownCast(this->SceneGraph->GetViewNodeFor(ren));

    oren->SetSize(viewportWidth, viewportHeight);
    oren->SetViewport(tileViewport);
    oren->SetScale(tileScale);

    this->SceneGraph->TraverseAllPasses();

    if (oren->GetBackend() == nullptr)
    {
      return;
    }

    // copy the result to the window

    svtkRenderWindow* rwin = svtkRenderWindow::SafeDownCast(ren->GetSVTKWindow());

    const int colorTexGL = this->SceneGraph->GetColorBufferTextureGL();
    const int depthTexGL = this->SceneGraph->GetDepthBufferTextureGL();

    svtkOpenGLRenderWindow* windowOpenGL = svtkOpenGLRenderWindow::SafeDownCast(rwin);

    if (!this->Internal->QuadHelper)
    {
      this->Internal->Init(windowOpenGL);
    }
    else
    {
      windowOpenGL->GetShaderCache()->ReadyShaderProgram(this->Internal->QuadHelper->Program);
    }

    if (!this->Internal->QuadHelper->Program || !this->Internal->QuadHelper->Program->GetCompiled())
    {
      svtkErrorMacro("Couldn't build the shader program.");
      return;
    }

    windowOpenGL->MakeCurrent();

    svtkTextureObject* usedColorTex = nullptr;
    svtkTextureObject* usedDepthTex = nullptr;

    if (colorTexGL != 0 && depthTexGL != 0 && windowOpenGL != nullptr)
    {
      // for visRTX, re-use existing OpenGL texture provided
      this->Internal->SharedColorTexture->AssignToExistingTexture(colorTexGL, GL_TEXTURE_2D);
      this->Internal->SharedDepthTexture->AssignToExistingTexture(depthTexGL, GL_TEXTURE_2D);

      usedColorTex = this->Internal->SharedColorTexture;
      usedDepthTex = this->Internal->SharedDepthTexture;
    }
    else
    {
      // upload to the texture
#ifdef SVTKOSPRAY_ENABLE_DENOISER
      this->Internal->ColorTexture->Create2DFromRaw(
        viewportWidth, viewportHeight, 4, SVTK_FLOAT, this->SceneGraph->GetBuffer());
#else
      this->Internal->ColorTexture->Create2DFromRaw(
        viewportWidth, viewportHeight, 4, SVTK_UNSIGNED_CHAR, this->SceneGraph->GetBuffer());
#endif
      this->Internal->DepthTexture->CreateDepthFromRaw(viewportWidth, viewportHeight,
        svtkTextureObject::Float32, SVTK_FLOAT, this->SceneGraph->GetZBuffer());

      usedColorTex = this->Internal->ColorTexture;
      usedDepthTex = this->Internal->DepthTexture;
    }

    usedColorTex->Activate();
    usedDepthTex->Activate();

    this->Internal->QuadHelper->Program->SetUniformi(
      "colorTexture", usedColorTex->GetTextureUnit());
    this->Internal->QuadHelper->Program->SetUniformi(
      "depthTexture", usedDepthTex->GetTextureUnit());

    svtkOpenGLState* ostate = windowOpenGL->GetState();

    svtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);
    svtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    svtkOpenGLState::ScopedglDepthFunc dfsaver(ostate);
    svtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);

    ostate->svtkglViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    ostate->svtkglScissor(viewportX, viewportY, viewportWidth, viewportHeight);

    ostate->svtkglEnable(GL_DEPTH_TEST);

    if (ren->GetLayer() == 0)
    {
      ostate->svtkglDisable(GL_BLEND);
      ostate->svtkglDepthFunc(GL_ALWAYS);
    }
    else
    {
      ostate->svtkglEnable(GL_BLEND);
      ostate->svtkglDepthFunc(GL_LESS);
      if (svtkOSPRayRendererNode::GetCompositeOnGL(ren))
      {
        ostate->svtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
      }
      else
      {
        ostate->svtkglBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
      }
    }

    this->Internal->QuadHelper->Render();

    usedDepthTex->Deactivate();
    usedColorTex->Deactivate();
  }
}

// ----------------------------------------------------------------------------
bool svtkOSPRayPass::IsBackendAvailable(const char* choice)
{
  std::set<RTWBackendType> bends = rtwGetAvailableBackends();
  if (!strcmp(choice, "OSPRay raycaster"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OSPRay pathtracer"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OptiX pathtracer"))
  {
    return (bends.find(RTW_BACKEND_VISRTX) != bends.end());
  }
  return false;
}
