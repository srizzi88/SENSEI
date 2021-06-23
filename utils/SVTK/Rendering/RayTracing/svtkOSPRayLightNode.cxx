/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayLightNode.h"

#include "svtkCamera.h"
#include "svtkCollectionIterator.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkLight.h"
#include "svtkMath.h"
#include "svtkOSPRayCameraNode.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderer.h"
#include "svtkTransform.h"

#include <vector>

svtkInformationKeyMacro(svtkOSPRayLightNode, IS_AMBIENT, Integer);
svtkInformationKeyMacro(svtkOSPRayLightNode, RADIUS, Double);

//============================================================================
double svtkOSPRayLightNode::LightScale = 1.0;

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOSPRayLightNode);

//----------------------------------------------------------------------------
svtkOSPRayLightNode::svtkOSPRayLightNode()
{
  this->OLight = nullptr;
}

//----------------------------------------------------------------------------
svtkOSPRayLightNode::~svtkOSPRayLightNode()
{
  svtkOSPRayRendererNode* orn = svtkOSPRayRendererNode::GetRendererNode(this);
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
      ospRelease((OSPLight)this->OLight);
  }
}

//----------------------------------------------------------------------------
void svtkOSPRayLightNode::SetLightScale(double s)
{
  svtkOSPRayLightNode::LightScale = s;
}

//----------------------------------------------------------------------------
double svtkOSPRayLightNode::GetLightScale()
{
  return svtkOSPRayLightNode::LightScale;
}

//----------------------------------------------------------------------------
void svtkOSPRayLightNode::SetIsAmbient(int value, svtkLight* light)
{
  if (!light)
  {
    return;
  }
  svtkInformation* info = light->GetInformation();
  info->Set(svtkOSPRayLightNode::IS_AMBIENT(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayLightNode::GetIsAmbient(svtkLight* light)
{
  if (!light)
  {
    return 0;
  }
  svtkInformation* info = light->GetInformation();
  if (info && info->Has(svtkOSPRayLightNode::IS_AMBIENT()))
  {
    return (info->Get(svtkOSPRayLightNode::IS_AMBIENT()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayLightNode::SetRadius(double value, svtkLight* light)
{
  if (!light)
  {
    return;
  }
  svtkInformation* info = light->GetInformation();
  info->Set(svtkOSPRayLightNode::RADIUS(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayLightNode::GetRadius(svtkLight* light)
{
  if (!light)
  {
    return 0.0;
  }
  svtkInformation* info = light->GetInformation();
  if (info && info->Has(svtkOSPRayLightNode::RADIUS()))
  {
    return (info->Get(svtkOSPRayLightNode::RADIUS()));
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void svtkOSPRayLightNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOSPRayLightNode::Render(bool prepass)
{
  if (prepass)
  {
    svtkOSPRayRendererNode* orn =
      static_cast<svtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("svtkOSPRayRendererNode"));

    svtkOpenGLRenderer* ren = svtkOpenGLRenderer::SafeDownCast(orn->GetRenderable());
    svtkTransform* userLightTransfo = ren->GetUserLightTransform();

    svtkNew<svtkMatrix4x4> camTransfo;
    svtkNew<svtkMatrix4x4> invCamTransfo;
    if (userLightTransfo)
    {
      svtkOSPRayCameraNode* ocam =
        static_cast<svtkOSPRayCameraNode*>(orn->GetFirstChildOfType("svtkOSPRayCameraNode"));

      svtkCamera* cam = svtkCamera::SafeDownCast(ocam->GetRenderable());

      cam->GetModelViewTransformObject()->GetMatrix(camTransfo);
      svtkMatrix4x4::Invert(camTransfo, invCamTransfo);
    }

    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
      return;
    ospRelease((OSPLight)this->OLight);
    OSPLight ospLight;

    svtkLight* light = svtkLight::SafeDownCast(this->GetRenderable());

    float color[3] = { 0.0, 0.0, 0.0 };
    if (light->GetSwitch())
    {
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
    }
    if (svtkOSPRayLightNode::GetIsAmbient(light))
    {
      ospLight = ospNewLight3("ambient");
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>(
        0.13f * svtkOSPRayLightNode::LightScale * light->GetIntensity() * svtkMath::Pi());
      ospSet1f(ospLight, "intensity", fI);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else if (light->GetPositional())
    {
      double position[4];
      light->GetPosition(position);
      position[3] = 1.0;

      if (light->LightTypeIsCameraLight())
      {
        light->TransformPoint(position, position);
      }

      if (!light->LightTypeIsSceneLight() && userLightTransfo)
      {
        camTransfo->MultiplyPoint(position, position);
        userLightTransfo->TransformPoint(position, position);
        invCamTransfo->MultiplyPoint(position, position);
      }

      float coneAngle = static_cast<float>(light->GetConeAngle());
      if (coneAngle <= 0.0 || coneAngle >= 90.0)
      {
        ospLight = ospNewLight3("PointLight");
      }
      else
      {
        ospLight = ospNewLight3("SpotLight");
        double focalPoint[4];
        light->GetFocalPoint(focalPoint);
        focalPoint[3] = 1.0;

        if (light->LightTypeIsCameraLight())
        {
          light->TransformPoint(focalPoint, focalPoint);
        }

        if (!light->LightTypeIsSceneLight() && userLightTransfo)
        {
          camTransfo->MultiplyPoint(focalPoint, focalPoint);
          userLightTransfo->TransformPoint(focalPoint, focalPoint);
          invCamTransfo->MultiplyPoint(focalPoint, focalPoint);
        }

        double direction[3];
        svtkMath::Subtract(focalPoint, position, direction);
        svtkMath::Normalize(direction);

        ospSet3f(ospLight, "direction", direction[0], direction[1], direction[2]);
        // OpenGL interprets this as a half-angle. Mult by 2 for consistency.
        ospSet1f(ospLight, "openingAngle", 2 * coneAngle);
        // TODO: penumbraAngle
      }
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI =
        static_cast<float>(svtkOSPRayLightNode::LightScale * light->GetIntensity() * svtkMath::Pi());
      ospSet1i(ospLight, "isVisible", 0);
      ospSet1f(ospLight, "intensity", fI);

      ospSet3f(ospLight, "position", position[0], position[1], position[2]);
      float r = static_cast<float>(svtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "radius", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else
    {
      double position[3];
      double focalPoint[3];
      light->GetPosition(position);
      light->GetFocalPoint(focalPoint);

      double direction[4];
      svtkMath::Subtract(focalPoint, position, direction);
      svtkMath::Normalize(direction);
      direction[3] = 0.0;

      if (light->LightTypeIsCameraLight())
      {
        light->TransformVector(direction, direction);
      }

      if (!light->LightTypeIsSceneLight() && userLightTransfo)
      {
        camTransfo->MultiplyPoint(direction, direction);
        userLightTransfo->TransformNormal(direction, direction);
        invCamTransfo->MultiplyPoint(direction, direction);
      }

      ospLight = ospNewLight3("DirectionalLight");
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI =
        static_cast<float>(svtkOSPRayLightNode::LightScale * light->GetIntensity() * svtkMath::Pi());
      ospSet1f(ospLight, "intensity", fI);
      ospSet3f(ospLight, "direction", direction[0], direction[1], direction[2]);
      float r = static_cast<float>(svtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "angularDiameter", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    this->OLight = ospLight;
  }
}
