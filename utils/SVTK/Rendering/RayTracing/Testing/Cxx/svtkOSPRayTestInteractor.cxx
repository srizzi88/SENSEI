/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayTestInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOSPRayTestInteractor.h"
#include "svtkObjectFactory.h"

#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkOSPRayLightNode.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"

#include <string>
#include <vector>

namespace
{
static std::vector<std::string> ActorNames;
}

//----------------------------------------------------------------------------
class svtkOSPRayTestLooper : public svtkCommand
{
  // for progressive rendering
public:
  svtkTypeMacro(svtkOSPRayTestLooper, svtkCommand);

  static svtkOSPRayTestLooper* New()
  {
    svtkOSPRayTestLooper* self = new svtkOSPRayTestLooper;
    self->RenderWindow = nullptr;
    self->ProgressiveCount = 0;
    return self;
  }

  void Execute(
    svtkObject* svtkNotUsed(caller), unsigned long eventId, void* svtkNotUsed(callData)) override
  {
    if (eventId == svtkCommand::TimerEvent)
    {
      if (this->RenderWindow)
      {
        svtkRenderer* renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
        int maxframes = svtkOSPRayRendererNode::GetMaxFrames(renderer);
        if (this->ProgressiveCount < maxframes)
        {
          this->ProgressiveCount++;
          this->RenderWindow->Render();
        }
      }
    }
    else
    {
      this->ProgressiveCount = 0;
    }
  }
  svtkRenderWindow* RenderWindow;
  int ProgressiveCount;
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOSPRayTestInteractor);

//----------------------------------------------------------------------------
svtkOSPRayTestInteractor::svtkOSPRayTestInteractor()
{
  this->SetPipelineControlPoints(nullptr, nullptr, nullptr);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
  this->Looper = svtkOSPRayTestLooper::New();
}

//----------------------------------------------------------------------------
svtkOSPRayTestInteractor::~svtkOSPRayTestInteractor()
{
  this->Looper->Delete();
}

//----------------------------------------------------------------------------
void svtkOSPRayTestInteractor::SetPipelineControlPoints(
  svtkRenderer* g, svtkRenderPass* _O, svtkRenderPass* _G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void svtkOSPRayTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == nullptr)
  {
    return;
  }

  // Get the keypress
  svtkRenderWindowInteractor* rwi = this->Interactor;
  std::string key = rwi->GetKeySym();

  if (key == "c")
  {
    svtkRenderPass* current = this->GLRenderer->GetPass();
    if (current == this->G)
    {
      cerr << "OSPRAY rendering " << this->O << endl;
      this->GLRenderer->SetPass(this->O);
      this->GLRenderer->GetRenderWindow()->Render();
    }
    else if (current == this->O)
    {
      cerr << "GL rendering " << this->G << endl;
      this->GLRenderer->SetPass(this->G);
      this->GLRenderer->GetRenderWindow()->Render();
    }
  }

  if (key == "n")
  {
    svtkActorCollection* actors = this->GLRenderer->GetActors();

    this->VisibleActor++;
    cerr << "VISIBLE " << this->VisibleActor;
    if (this->VisibleActor == actors->GetNumberOfItems())
    {
      this->VisibleActor = -1;
    }
    for (int i = 0; i < actors->GetNumberOfItems(); i++)
    {
      if (this->VisibleActor == -1 || this->VisibleActor == i)
      {
        if (i < static_cast<int>(ActorNames.size()))
        {
          cerr << " : " << ActorNames[i] << " ";
        }
        svtkActor::SafeDownCast(actors->GetItemAsObject(i))->SetVisibility(1);
      }
      else
      {
        svtkActor::SafeDownCast(actors->GetItemAsObject(i))->SetVisibility(0);
      }
    }
    cerr << endl;
    this->GLRenderer->ResetCamera();
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "l")
  {
    svtkLightCollection* lights = this->GLRenderer->GetLights();

    this->VisibleLight++;
    if (this->VisibleLight == lights->GetNumberOfItems())
    {
      this->VisibleLight = -1;
    }
    cerr << "LIGHT " << this->VisibleLight << "/" << lights->GetNumberOfItems() << endl;
    for (int i = 0; i < lights->GetNumberOfItems(); i++)
    {
      if (this->VisibleLight == -1 || this->VisibleLight == i)
      {
        svtkLight::SafeDownCast(lights->GetItemAsObject(i))->SwitchOn();
      }
      else
      {
        svtkLight::SafeDownCast(lights->GetItemAsObject(i))->SwitchOff();
      }
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "P")
  {
    int maxframes = svtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer) + 16;
    if (maxframes > 256)
    {
      maxframes = 256;
    }
    svtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "p")
  {
    int maxframes = svtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer);
    if (maxframes > 1)
    {
      maxframes = maxframes / 2;
    }
    svtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "s")
  {
    bool shadows = !(this->GLRenderer->GetUseShadows() == 0);
    cerr << "shadows now " << (!shadows ? "ON" : "OFF") << endl;
    this->GLRenderer->SetUseShadows(!shadows);
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "t")
  {
    std::string type = svtkOSPRayRendererNode::GetRendererType(this->GLRenderer);
    if (type == std::string("scivis"))
    {
      svtkOSPRayRendererNode::SetRendererType("pathtracer", this->GLRenderer);
    }
    else if (type == std::string("pathtracer"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", this->GLRenderer);
    }
    else if (type == std::string("optix pathtracer"))
    {
      svtkOSPRayRendererNode::SetRendererType("scivis", this->GLRenderer);
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "2")
  {
    int spp = svtkOSPRayRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp + 1 << endl;
    svtkOSPRayRendererNode::SetSamplesPerPixel(spp + 1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
  }
  if (key == "1")
  {
    svtkOSPRayRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "D")
  {
    int aoSamples = svtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples > 64)
    {
      aoSamples = 64;
    }
    svtkOSPRayRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "d")
  {
    int aosamples = svtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples = aosamples / 2;
    svtkOSPRayRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "I")
  {
    double intens = svtkOSPRayLightNode::GetLightScale() * 1.5;
    svtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "i")
  {
    double intens = svtkOSPRayLightNode::GetLightScale() / 1.5;
    svtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "N")
  {
    bool set = svtkOSPRayRendererNode::GetEnableDenoiser(this->GLRenderer);
    svtkOSPRayRendererNode::SetEnableDenoiser(!set, this->GLRenderer);
    cerr << "denoiser " << (!set ? "ON" : "OFF") << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  // Forward events
  svtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void svtkOSPRayTestInteractor::AddName(const char* name)
{
  ActorNames.push_back(std::string(name));
}

//------------------------------------------------------------------------------
svtkCommand* svtkOSPRayTestInteractor::GetLooper(svtkRenderWindow* rw)
{
  rw->Render();
  svtkOSPRayRendererNode::SetMaxFrames(128, this->GLRenderer);
  svtkOSPRayTestLooper::SafeDownCast(this->Looper)->RenderWindow = rw;
  return this->Looper;
}
