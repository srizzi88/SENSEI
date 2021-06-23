/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCameraActor.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkFrustumSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlanes.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"

svtkStandardNewMacro(svtkCameraActor);
svtkCxxSetObjectMacro(svtkCameraActor, Camera, svtkCamera);

// ----------------------------------------------------------------------------
svtkCameraActor::svtkCameraActor()
{
  this->Camera = nullptr;
  this->WidthByHeightRatio = 1.0;
  this->FrustumSource = nullptr;
  this->FrustumMapper = nullptr;
  this->FrustumActor = nullptr;
}

// ----------------------------------------------------------------------------
svtkCameraActor::~svtkCameraActor()
{
  this->SetCamera(nullptr);

  if (this->FrustumActor != nullptr)
  {
    this->FrustumActor->Delete();
  }

  if (this->FrustumMapper != nullptr)
  {
    this->FrustumMapper->Delete();
  }
  if (this->FrustumSource != nullptr)
  {
    this->FrustumSource->Delete();
  }
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int svtkCameraActor::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->UpdateViewProps();

  int result = 0;
  if (this->FrustumActor != nullptr && this->FrustumActor->GetMapper() != nullptr)
  {
    result = this->FrustumActor->RenderOpaqueGeometry(viewport);
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
svtkTypeBool svtkCameraActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//-----------------------------------------------------------------------------
void svtkCameraActor::ReleaseGraphicsResources(svtkWindow* window)
{
  if (this->FrustumActor != nullptr)
  {
    this->FrustumActor->ReleaseGraphicsResources(window);
  }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkCameraActor::GetBounds()
{
  // we cannot initialize the Bounds the same way svtkBoundingBox does because
  // svtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As svtkMath::UninitializeBounds initialized finite unvalid bounds, it
  // passes silently and GetLength() returns 0.
  svtkMath::UninitializeBounds(this->Bounds);

  this->UpdateViewProps();
  if (this->FrustumActor != nullptr && this->FrustumActor->GetUseBounds())
  {
    this->FrustumActor->GetBounds(this->Bounds);
  }
  return this->Bounds;
}

//-------------------------------------------------------------------------
svtkMTimeType svtkCameraActor::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Camera != nullptr)
  {
    svtkMTimeType time;
    time = this->Camera->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }
  return mTime;
}

// ----------------------------------------------------------------------------
// Description:
// Get property of the internal actor.
svtkProperty* svtkCameraActor::GetProperty()
{
  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = svtkActor::New();
  }

  return this->FrustumActor->GetProperty();
}

// ----------------------------------------------------------------------------
// Description:
// Set property of the internal actor.
void svtkCameraActor::SetProperty(svtkProperty* p)
{
  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = svtkActor::New();
  }

  this->FrustumActor->SetProperty(p);
}

// ----------------------------------------------------------------------------
void svtkCameraActor::UpdateViewProps()
{
  if (this->Camera == nullptr)
  {
    svtkDebugMacro(<< "no camera to represent.");
    return;
  }

  svtkPlanes* planes = nullptr;
  if (this->FrustumSource == nullptr)
  {
    this->FrustumSource = svtkFrustumSource::New();
    planes = svtkPlanes::New();
    this->FrustumSource->SetPlanes(planes);
    planes->Delete();
  }
  else
  {
    planes = this->FrustumSource->GetPlanes();
  }

  double coefs[24];
  this->Camera->GetFrustumPlanes(this->WidthByHeightRatio, coefs);
  planes->SetFrustumPlanes(coefs);

  this->FrustumSource->SetShowLines(false);

  if (this->FrustumMapper == nullptr)
  {
    this->FrustumMapper = svtkPolyDataMapper::New();
  }

  this->FrustumMapper->SetInputConnection(this->FrustumSource->GetOutputPort());

  if (this->FrustumActor == nullptr)
  {
    this->FrustumActor = svtkActor::New();
  }

  this->FrustumActor->SetMapper(this->FrustumMapper);

  svtkProperty* p = this->FrustumActor->GetProperty();
  p->SetRepresentationToWireframe();
  this->FrustumActor->SetVisibility(1);
}

//-------------------------------------------------------------------------
void svtkCameraActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Camera: ";
  if (this->Camera == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->Camera->PrintSelf(os, indent);
  }

  os << indent << "WidthByHeightRatio: " << this->WidthByHeightRatio << endl;
}
