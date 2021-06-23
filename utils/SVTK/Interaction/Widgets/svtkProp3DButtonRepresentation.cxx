/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProp3DButtonRepresentation.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkProp3D.h"
#include "svtkProp3DFollower.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include <map>

svtkStandardNewMacro(svtkProp3DButtonRepresentation);

struct svtkScaledProp
{
  svtkSmartPointer<svtkProp3D> Prop;
  double Origin[3];
  double Scale;
  double Translation[3];
  svtkScaledProp()
  {
    this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
    this->Scale = 1.0;
    this->Translation[0] = this->Translation[1] = this->Translation[2] = 0.0;
  }
};

// Map of textures
class svtkPropArray : public std::map<int, svtkScaledProp>
{
};
typedef std::map<int, svtkScaledProp>::iterator svtkPropArrayIterator;

//----------------------------------------------------------------------
svtkProp3DButtonRepresentation::svtkProp3DButtonRepresentation()
{
  // Current button representation
  this->CurrentProp = nullptr;

  // Following
  this->FollowCamera = 0;
  this->Follower = svtkProp3DFollower::New();

  // List of textures
  this->PropArray = new svtkPropArray;

  this->Picker = svtkPropPicker::New();
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------
svtkProp3DButtonRepresentation::~svtkProp3DButtonRepresentation()
{
  this->Follower->Delete();

  delete this->PropArray;

  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void svtkProp3DButtonRepresentation::SetState(int state)
{
  this->Superclass::SetState(state);

  this->CurrentProp = this->GetButtonProp(this->State);
  this->Follower->SetProp3D(this->CurrentProp);

  this->Picker->InitializePickList();
  if (this->CurrentProp)
  {
    this->Picker->AddPickList(this->CurrentProp);
  }
}

//-------------------------------------------------------------------------
void svtkProp3DButtonRepresentation::SetButtonProp(int i, svtkProp3D* prop)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  svtkScaledProp sprop;
  sprop.Prop = prop;

  (*this->PropArray)[i] = sprop;
}

//-------------------------------------------------------------------------
svtkProp3D* svtkProp3DButtonRepresentation::GetButtonProp(int i)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  svtkPropArrayIterator iter = this->PropArray->find(i);
  if (iter != this->PropArray->end())
  {
    return (*iter).second.Prop;
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
void svtkProp3DButtonRepresentation::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//-------------------------------------------------------------------------
void svtkProp3DButtonRepresentation::PlaceWidget(double bds[6])
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

  this->SetState(this->State);

  svtkProp3D* prop;
  svtkPropArrayIterator iter;
  for (iter = this->PropArray->begin(); iter != this->PropArray->end(); ++iter)
  {
    prop = (*iter).second.Prop;

    prop->GetBounds(aBds);
    aCenter[0] = (aBds[0] + aBds[1]) / 2.0;
    aCenter[1] = (aBds[2] + aBds[3]) / 2.0;
    aCenter[2] = (aBds[4] + aBds[5]) / 2.0;

    // Now fit the actor bounds in the place bounds by tampering with its
    // transform.
    (*iter).second.Origin[0] = aCenter[0];
    (*iter).second.Origin[1] = aCenter[1];
    (*iter).second.Origin[2] = aCenter[2];

    (*iter).second.Translation[0] = center[0] - aCenter[0];
    (*iter).second.Translation[1] = center[1] - aCenter[1];
    (*iter).second.Translation[2] = center[2] - aCenter[2];

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

    (*iter).second.Scale = sMin;
  }
}

//-------------------------------------------------------------------------
int svtkProp3DButtonRepresentation ::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  this->InteractionState = svtkButtonRepresentation::Outside;
  if (!this->Renderer)
  {
    return this->InteractionState;
  }
  this->VisibilityOn(); // actor must be on to be picked

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if (path != nullptr)
  {
    this->InteractionState = svtkButtonRepresentation::Inside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkProp3DButtonRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    this->SetState(this->State); // side effect sets CurrentProp
    svtkPropArrayIterator iter = this->PropArray->find(this->State);
    if (this->CurrentProp == nullptr || iter == this->PropArray->end())
    {
      return;
    }

    // In case follower is being used
    if (this->FollowCamera)
    {
      this->Follower->SetCamera(this->Renderer->GetActiveCamera());
      this->Follower->SetProp3D(this->CurrentProp);
      this->Follower->SetOrigin((*iter).second.Origin);
      this->Follower->SetPosition((*iter).second.Translation);
      this->Follower->SetScale((*iter).second.Scale);
    }
    else
    {
      this->CurrentProp->SetOrigin((*iter).second.Origin);
      this->CurrentProp->SetPosition((*iter).second.Translation);
      this->CurrentProp->SetScale((*iter).second.Scale);
    }

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkProp3DButtonRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkProp3DButtonRepresentation* rep = svtkProp3DButtonRepresentation::SafeDownCast(prop);
  if (rep)
  {
    svtkPropArrayIterator iter;
    for (iter = rep->PropArray->begin(); iter != rep->PropArray->end(); ++iter)
    {
      (*this->PropArray)[(*iter).first] = (*iter).second;
    }
    this->FollowCamera = rep->FollowCamera;
  }

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkProp3DButtonRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Follower->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkProp3DButtonRepresentation::RenderVolumetricGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();

  if (!this->CurrentProp)
  {
    return 0;
  }

  if (this->FollowCamera)
  {
    return this->Follower->RenderVolumetricGeometry(viewport);
  }
  else
  {
    return this->CurrentProp->RenderVolumetricGeometry(viewport);
  }
}

//----------------------------------------------------------------------
int svtkProp3DButtonRepresentation::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();

  if (!this->CurrentProp)
  {
    return 0;
  }

  if (this->FollowCamera)
  {
    return this->Follower->RenderOpaqueGeometry(viewport);
  }
  else
  {
    return this->CurrentProp->RenderOpaqueGeometry(viewport);
  }
}

//-----------------------------------------------------------------------------
int svtkProp3DButtonRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();

  if (!this->CurrentProp)
  {
    return 0;
  }

  if (this->FollowCamera)
  {
    return this->Follower->RenderTranslucentPolygonalGeometry(viewport);
  }
  else
  {
    return this->CurrentProp->RenderTranslucentPolygonalGeometry(viewport);
  }
}
//-----------------------------------------------------------------------------
svtkTypeBool svtkProp3DButtonRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  if (this->CurrentProp)
  {
    return this->CurrentProp->HasTranslucentPolygonalGeometry();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------
double* svtkProp3DButtonRepresentation::GetBounds()
{
  if (!this->CurrentProp)
  {
    return nullptr;
  }

  if (this->FollowCamera)
  {
    return this->Follower->GetBounds();
  }
  else
  {
    return this->CurrentProp->GetBounds();
  }
}

//----------------------------------------------------------------------
void svtkProp3DButtonRepresentation::GetActors(svtkPropCollection* pc)
{
  if (this->CurrentProp)
  {
    this->CurrentProp->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void svtkProp3DButtonRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Follow Camera: " << (this->FollowCamera ? "On\n" : "Off\n");

  os << indent << "3D Props: \n";
  svtkPropArrayIterator iter;
  int i;
  for (i = 0, iter = this->PropArray->begin(); iter != this->PropArray->end(); ++iter, ++i)
  {
    os << indent << "  (" << i << "): " << (*iter).second.Prop << "\n";
  }
}
