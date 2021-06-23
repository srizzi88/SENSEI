/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTexturedButtonRepresentation.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkCoordinate.h"
#include "svtkFollower.h"
#include "svtkImageData.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include <map>

svtkStandardNewMacro(svtkTexturedButtonRepresentation);

svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation, Property, svtkProperty);
svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation, HoveringProperty, svtkProperty);
svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation, SelectingProperty, svtkProperty);

// Map of textures
class svtkTextureArray : public std::map<int, svtkSmartPointer<svtkImageData> >
{
};
typedef std::map<int, svtkSmartPointer<svtkImageData> >::iterator svtkTextureArrayIterator;

//----------------------------------------------------------------------
svtkTexturedButtonRepresentation::svtkTexturedButtonRepresentation()
{
  this->Mapper = svtkPolyDataMapper::New();
  this->Texture = svtkTexture::New();
  this->Texture->SetBlendingMode(svtkTexture::SVTK_TEXTURE_BLENDING_MODE_ADD);
  this->Actor = svtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetTexture(this->Texture);
  this->Follower = svtkFollower::New();
  this->Follower->SetMapper(this->Mapper);
  this->Follower->SetTexture(this->Texture);

  // Following
  this->FollowCamera = 0;

  // Set up the initial properties
  this->CreateDefaultProperties();

  // List of textures
  this->TextureArray = new svtkTextureArray;

  this->Picker = svtkCellPicker::New();
  this->Picker->AddPickList(this->Actor);
  this->Picker->AddPickList(this->Follower);
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------
svtkTexturedButtonRepresentation::~svtkTexturedButtonRepresentation()
{
  this->Actor->Delete();
  this->Follower->Delete();
  this->Mapper->Delete();
  this->Texture->Delete();

  if (this->Property)
  {
    this->Property->Delete();
    this->Property = nullptr;
  }

  if (this->HoveringProperty)
  {
    this->HoveringProperty->Delete();
    this->HoveringProperty = nullptr;
  }

  if (this->SelectingProperty)
  {
    this->SelectingProperty->Delete();
    this->SelectingProperty = nullptr;
  }

  delete this->TextureArray;

  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation::SetButtonGeometry(svtkPolyData* pd)
{
  this->Mapper->SetInputData(pd);
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation::SetButtonGeometryConnection(svtkAlgorithmOutput* algOutput)
{
  this->Mapper->SetInputConnection(algOutput);
}

//-------------------------------------------------------------------------
svtkPolyData* svtkTexturedButtonRepresentation::GetButtonGeometry()
{
  return this->Mapper->GetInput();
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation::SetButtonTexture(int i, svtkImageData* image)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  (*this->TextureArray)[i] = image;
}

//-------------------------------------------------------------------------
svtkImageData* svtkTexturedButtonRepresentation::GetButtonTexture(int i)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  svtkTextureArrayIterator iter = this->TextureArray->find(i);
  if (iter != this->TextureArray->end())
  {
    return (*iter).second;
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation::PlaceWidget(double scale, double xyz[3], double normal[3])
{
  // Translate the center
  double bds[6], center[3];
  this->Actor->GetBounds(bds);
  center[0] = (bds[0] + bds[1]) / 2.0;
  center[1] = (bds[2] + bds[3]) / 2.0;
  center[2] = (bds[4] + bds[5]) / 2.0;

  this->Actor->AddPosition(center[0] - xyz[0], center[1] - xyz[1], center[2] - xyz[2]);
  this->Follower->AddPosition(center[0] - xyz[0], center[1] - xyz[1], center[2] - xyz[2]);

  // Scale the button
  this->Actor->SetScale(scale, scale, scale);
  this->Follower->SetScale(scale, scale, scale);

  // Rotate the button to align with the normal Cross the z axis with the
  // normal to get a rotation vector. Then rotate around it.
  double zAxis[3];
  zAxis[0] = zAxis[1] = 0.0;
  zAxis[2] = 1.0;
  double rotAxis[3], angle;

  svtkMath::Normalize(normal);
  svtkMath::Cross(zAxis, normal, rotAxis);
  angle = acos(svtkMath::Dot(zAxis, normal));
  this->Actor->RotateWXYZ(svtkMath::DegreesFromRadians(angle), rotAxis[0], rotAxis[1], rotAxis[2]);
  this->Follower->RotateWXYZ(
    svtkMath::DegreesFromRadians(angle), rotAxis[0], rotAxis[1], rotAxis[2]);
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation::PlaceWidget(double bds[6])
{
  double bounds[6], center[3], aBds[6], aCenter[3];

  this->AdjustBounds(bds, bounds, center);
  for (int i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  // Get the bounds of the actor
  this->Actor->GetBounds(aBds);
  aCenter[0] = (aBds[0] + aBds[1]) / 2.0;
  aCenter[1] = (aBds[2] + aBds[3]) / 2.0;
  aCenter[2] = (aBds[4] + aBds[5]) / 2.0;

  // Now fit the actor bounds in the place bounds by tampering with its
  // transform.
  this->Actor->AddPosition(center[0] - aCenter[0], center[1] - aCenter[1], center[2] - aCenter[2]);
  this->Follower->AddPosition(
    center[0] - aCenter[0], center[1] - aCenter[1], center[2] - aCenter[2]);

  double s[3], sMin;
  for (int i = 0; i < 3; ++i)
  {
    if ((bounds[2 * i + 1] - bounds[2 * i]) <= 0.0 || (aBds[2 * i + 1] - aBds[2 * i]) <= 0.0)
    {
      s[i] = SVTK_FLOAT_MAX;
    }
    else
    {
      s[i] = (bounds[2 * i + 1] - bounds[2 * i]) / (aBds[2 * i + 1] - aBds[2 * i]);
    }
  }
  sMin = (s[0] < s[1] ? (s[0] < s[2] ? s[0] : s[2]) : (s[1] < s[2] ? s[1] : s[2]));

  this->Actor->SetScale(sMin, sMin, sMin);
  this->Follower->SetScale(sMin, sMin, sMin);
}

//-------------------------------------------------------------------------
int svtkTexturedButtonRepresentation ::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  this->VisibilityOn(); // actor must be on to be picked

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if (path != nullptr)
  {
    this->InteractionState = svtkButtonRepresentation::Inside;
  }
  else
  {
    this->InteractionState = svtkButtonRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::Highlight(int highlight)
{
  this->Superclass::Highlight(highlight);

  svtkProperty* initialProperty = this->Actor->GetProperty();
  svtkProperty* selectedProperty;

  if (highlight == svtkButtonRepresentation::HighlightHovering)
  {
    this->Actor->SetProperty(this->HoveringProperty);
    this->Follower->SetProperty(this->HoveringProperty);
    selectedProperty = this->HoveringProperty;
  }
  else if (highlight == svtkButtonRepresentation::HighlightSelecting)
  {
    this->Actor->SetProperty(this->SelectingProperty);
    this->Follower->SetProperty(this->SelectingProperty);
    selectedProperty = this->SelectingProperty;
  }
  else // if ( highlight == svtkButtonRepresentation::HighlightNormal )
  {
    this->Actor->SetProperty(this->Property);
    this->Follower->SetProperty(this->Property);
    selectedProperty = this->Property;
  }

  if (selectedProperty != initialProperty)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::CreateDefaultProperties()
{
  this->Property = svtkProperty::New();
  this->Property->SetColor(1, 1, 1);

  this->HoveringProperty = svtkProperty::New();
  this->HoveringProperty->SetAmbient(1.0);

  this->SelectingProperty = svtkProperty::New();
  this->SelectingProperty->SetAmbient(0.2);
  this->SelectingProperty->SetAmbientColor(0.2, 0.2, 0.2);
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    // In case follower is being used
    if (this->FollowCamera)
    {
      this->Follower->VisibilityOn();
      this->Actor->VisibilityOff();
      this->Follower->SetCamera(this->Renderer->GetActiveCamera());
    }
    else
    {
      this->Follower->VisibilityOff();
      this->Actor->VisibilityOn();
    }

    svtkTextureArrayIterator iter = this->TextureArray->find(this->State);
    if (iter != this->TextureArray->end())
    {
      this->Texture->SetInputData((*iter).second);
    }
    else
    {
      this->Texture->SetInputData(nullptr);
    }

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkTexturedButtonRepresentation* rep = svtkTexturedButtonRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->Mapper->ShallowCopy(rep->Mapper);
    this->Property->DeepCopy(rep->Property);
    this->HoveringProperty->DeepCopy(rep->HoveringProperty);
    this->SelectingProperty->DeepCopy(rep->SelectingProperty);

    svtkTextureArrayIterator iter;
    for (iter = rep->TextureArray->begin(); iter != rep->TextureArray->end(); ++iter)
    {
      (*this->TextureArray)[(*iter).first] = (*iter).second;
    }
    this->FollowCamera = rep->FollowCamera;
  }

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->Follower->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkTexturedButtonRepresentation::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();

  if (this->FollowCamera)
  {
    return this->Follower->RenderOpaqueGeometry(viewport);
  }
  else
  {
    return this->Actor->RenderOpaqueGeometry(viewport);
  }
}

//-----------------------------------------------------------------------------
int svtkTexturedButtonRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();

  if (this->FollowCamera)
  {
    return this->Follower->RenderTranslucentPolygonalGeometry(viewport);
  }
  else
  {
    return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
  }
}
//-----------------------------------------------------------------------------
svtkTypeBool svtkTexturedButtonRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  if (this->FollowCamera)
  {
    return this->Follower->HasTranslucentPolygonalGeometry();
  }
  else
  {
    return this->Actor->HasTranslucentPolygonalGeometry();
  }
}

//----------------------------------------------------------------------
double* svtkTexturedButtonRepresentation::GetBounds()
{
  return this->Actor->GetBounds();
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::GetActors(svtkPropCollection* pc)
{
  if (this->FollowCamera)
  {
    this->Follower->GetActors(pc);
  }
  else
  {
    this->Actor->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Button Geometry: " << this->GetButtonGeometry() << "\n";

  os << indent << "Follow Camera: " << (this->FollowCamera ? "On\n" : "Off\n");

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->HoveringProperty)
  {
    os << indent << "Hovering Property: " << this->HoveringProperty << "\n";
  }
  else
  {
    os << indent << "Hovering Property: (none)\n";
  }

  if (this->SelectingProperty)
  {
    os << indent << "Selecting Property: " << this->SelectingProperty << "\n";
  }
  else
  {
    os << indent << "Selecting Property: (none)\n";
  }
}
