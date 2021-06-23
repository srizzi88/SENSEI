/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExternalOpenGLRenderer.h"

#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkExternalLight.h"
#include "svtkExternalOpenGLCamera.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGL.h"
#include "svtkOpenGLError.h"
#include "svtkRenderWindow.h"
#include "svtkTexture.h"

#define MAX_LIGHTS 8

svtkStandardNewMacro(svtkExternalOpenGLRenderer);

//----------------------------------------------------------------------------
svtkExternalOpenGLRenderer::svtkExternalOpenGLRenderer()
{
  this->PreserveColorBuffer = 1;
  this->PreserveDepthBuffer = 1;
  this->PreserveGLCameraMatrices = 1;
  this->PreserveGLLights = 1;
  this->SetAutomaticLightCreation(0);
  this->ExternalLights = svtkLightCollection::New();
}

//----------------------------------------------------------------------------
svtkExternalOpenGLRenderer::~svtkExternalOpenGLRenderer()
{
  this->ExternalLights->Delete();
  this->ExternalLights = nullptr;
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::Render(void)
{
  // Copy GL camera matrices
  if (this->PreserveGLCameraMatrices)
  {
    this->SynchronizeGLCameraMatrices();
  }

  // Copy GL lights
  if (this->PreserveGLLights)
  {
    this->SynchronizeGLLights();
  }

  // Forward the call to the Superclass
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::SynchronizeGLCameraMatrices()
{
  GLdouble mv[16], p[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mv);
  glGetDoublev(GL_PROJECTION_MATRIX, p);

  svtkExternalOpenGLCamera* camera =
    svtkExternalOpenGLCamera::SafeDownCast(this->GetActiveCameraAndResetIfCreated());

  camera->SetProjectionTransformMatrix(p);
  camera->SetViewTransformMatrix(mv);

  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  matrix->DeepCopy(mv);
  matrix->Transpose();
  matrix->Invert();

  // Synchronize camera viewUp
  double viewUp[4] = { 0.0, 1.0, 0.0, 0.0 }, newViewUp[4];
  matrix->MultiplyPoint(viewUp, newViewUp);
  svtkMath::Normalize(newViewUp);
  camera->SetViewUp(newViewUp);

  // Synchronize camera position
  double position[4] = { 0.0, 0.0, 0.0, 1.0 }, newPosition[4];
  matrix->MultiplyPoint(position, newPosition);

  if (newPosition[3] != 0.0)
  {
    newPosition[0] /= newPosition[3];
    newPosition[1] /= newPosition[3];
    newPosition[2] /= newPosition[3];
    newPosition[3] = 1.0;
  }
  camera->SetPosition(newPosition);

  // Synchronize focal point
  double focalPoint[4] = { 0.0, 0.0, -1.0, 1.0 }, newFocalPoint[4];
  matrix->MultiplyPoint(focalPoint, newFocalPoint);
  camera->SetFocalPoint(newFocalPoint);

  matrix->Delete();
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::SynchronizeGLLights()
{
  // Lights
  // Query lights existing in the external context
  // and tweak them based on svtkExternalLight objects added by the user
  GLenum curLight;
  for (curLight = GL_LIGHT0; curLight < GL_LIGHT0 + MAX_LIGHTS; curLight++)
  {
    GLboolean status;
    glGetBooleanv(curLight, &status);

    int l_ind = static_cast<int>(curLight - GL_LIGHT0);
    bool light_created = false;
    svtkLight* light = svtkLight::SafeDownCast(this->GetLights()->GetItemAsObject(l_ind));
    if (light)
    {
      if (!status)
      {
        // This is the case when we have a SVTK light in the scene but no
        // external light corresponding to that index in the context.
        // In this case, we remove the SVTK light as well.
        light->SwitchOff();
        this->RemoveLight(light);

        // No need to go forward
        continue;
      }
    }
    else
    {
      // No matching light found in the SVTK light collection
      if (status)
      {
        // Create a new light only if one is present in the external context
        light = svtkLight::New();
        // Headlight because SVTK will apply transform matrices
        light->SetLightTypeToHeadlight();
        light_created = true;
      }
      else
      {
        // No need to go forward as this light is not being used
        continue;
      }
    }

    // Find out if there is an external light object associated with this
    // light index.
    svtkCollectionSimpleIterator sit;
    svtkExternalLight* eLight;
    svtkExternalLight* curExtLight = nullptr;
    for (this->ExternalLights->InitTraversal(sit);
         (eLight = svtkExternalLight::SafeDownCast(this->ExternalLights->GetNextLight(sit)));)
    {
      if (eLight && (static_cast<GLenum>(eLight->GetLightIndex()) == curLight))
      {
        curExtLight = eLight;
        break;
      }
    }

    if (curExtLight && (curExtLight->GetReplaceMode() == svtkExternalLight::ALL_PARAMS))
    {
      // If the replace mode is all parameters, blatantly overwrite the
      // parameters of existing/new light
      light->DeepCopy(curExtLight);
    }
    else
    {

      GLfloat info[4];

      // Set color parameters
      if (curExtLight && curExtLight->GetIntensitySet())
      {
        light->SetIntensity(curExtLight->GetIntensity());
      }

      if (curExtLight && curExtLight->GetAmbientColorSet())
      {
        light->SetAmbientColor(curExtLight->GetAmbientColor());
      }
      else
      {
        glGetLightfv(curLight, GL_AMBIENT, info);
        light->SetAmbientColor(info[0], info[1], info[2]);
      }
      if (curExtLight && curExtLight->GetDiffuseColorSet())
      {
        light->SetDiffuseColor(curExtLight->GetDiffuseColor());
      }
      else
      {
        glGetLightfv(curLight, GL_DIFFUSE, info);
        light->SetDiffuseColor(info[0], info[1], info[2]);
      }
      if (curExtLight && curExtLight->GetSpecularColorSet())
      {
        light->SetSpecularColor(curExtLight->GetSpecularColor());
      }
      else
      {
        glGetLightfv(curLight, GL_SPECULAR, info);
        light->SetSpecularColor(info[0], info[1], info[2]);
      }

      // Position, focal point and positional
      glGetLightfv(curLight, GL_POSITION, info);

      if (curExtLight && curExtLight->GetPositionalSet())
      {
        light->SetPositional(curExtLight->GetPositional());
      }
      else
      {
        light->SetPositional(info[3] > 0.0 ? 1 : 0);
      }

      if (!light->GetPositional())
      {
        if (curExtLight && curExtLight->GetFocalPointSet())
        {
          light->SetFocalPoint(curExtLight->GetFocalPoint());
          if (curExtLight->GetPositionSet())
          {
            light->SetPosition(curExtLight->GetPosition());
          }
          else
          {
            light->SetPosition(info[0], info[1], info[2]);
          }
        }
        else
        {
          light->SetFocalPoint(0, 0, 0);
          if (curExtLight && curExtLight->GetPositionSet())
          {
            light->SetPosition(curExtLight->GetPosition());
          }
          else
          {
            light->SetPosition(-info[0], -info[1], -info[2]);
          }
        }
      }
      else
      {
        if (curExtLight && curExtLight->GetPositionSet())
        {
          light->SetPosition(curExtLight->GetPosition());
        }
        else
        {
          light->SetPosition(info[0], info[1], info[2]);
        }

        // Attenuation
        if (curExtLight && curExtLight->GetAttenuationValuesSet())
        {
          light->SetAttenuationValues(curExtLight->GetAttenuationValues());
        }
        else
        {
          glGetLightfv(curLight, GL_CONSTANT_ATTENUATION, &info[0]);
          glGetLightfv(curLight, GL_LINEAR_ATTENUATION, &info[1]);
          glGetLightfv(curLight, GL_QUADRATIC_ATTENUATION, &info[2]);
          light->SetAttenuationValues(info[0], info[1], info[2]);
        }

        // Cutoff
        if (curExtLight && curExtLight->GetConeAngleSet())
        {
          light->SetConeAngle(curExtLight->GetConeAngle());
        }
        else
        {
          glGetLightfv(curLight, GL_SPOT_CUTOFF, &info[0]);
          light->SetConeAngle(info[0]);
        }

        if (light->GetConeAngle() < 90.0)
        {
          // Exponent
          if (curExtLight && curExtLight->GetExponentSet())
          {
            light->SetExponent(curExtLight->GetExponent());
          }
          else
          {
            glGetLightfv(curLight, GL_SPOT_EXPONENT, &info[0]);
            light->SetExponent(info[0]);
          }

          // Direction
          if (curExtLight && curExtLight->GetFocalPointSet())
          {
            light->SetFocalPoint(curExtLight->GetFocalPoint());
          }
          else
          {
            glGetLightfv(curLight, GL_SPOT_DIRECTION, info);
            for (unsigned int i = 0; i < 3; ++i)
            {
              info[i] += light->GetPosition()[i];
            }
            light->SetFocalPoint(info[0], info[1], info[2]);
          }
        }
      }
    }

    // If we created a new SVTK light, add it to the collection
    if (light_created)
    {
      this->AddLight(light);
      light->Delete();
    }
  }
}

//----------------------------------------------------------------------------
svtkCamera* svtkExternalOpenGLRenderer::MakeCamera()
{
  svtkCamera* cam = svtkExternalOpenGLCamera::New();
  this->InvokeEvent(svtkCommand::CreateCameraEvent, cam);
  return cam;
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::AddExternalLight(svtkExternalLight* light)
{
  if (!light)
  {
    return;
  }

  svtkExternalLight* aLight;

  svtkCollectionSimpleIterator sit;
  for (this->ExternalLights->InitTraversal(sit);
       (aLight = svtkExternalLight::SafeDownCast(this->ExternalLights->GetNextLight(sit)));)
  {
    if (aLight && (aLight->GetLightIndex() == light->GetLightIndex()))
    {
      svtkErrorMacro(<< "Attempting to add light with index " << light->GetLightIndex()
                    << ". But light with same index already exists.");
      return;
    }
  }

  this->ExternalLights->AddItem(light);
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::RemoveExternalLight(svtkExternalLight* light)
{
  this->ExternalLights->RemoveItem(light);
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::RemoveAllExternalLights()
{
  this->ExternalLights->RemoveAllItems();
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "External Lights:\n";
  this->ExternalLights->PrintSelf(os, indent.GetNextIndent());
}
