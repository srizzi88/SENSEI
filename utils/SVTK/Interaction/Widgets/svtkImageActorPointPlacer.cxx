/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageActorPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageActorPointPlacer.h"
#include "svtkBoundedPlanePointPlacer.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkImageActorPointPlacer);

svtkCxxSetObjectMacro(svtkImageActorPointPlacer, ImageActor, svtkImageActor);

//----------------------------------------------------------------------
svtkImageActorPointPlacer::svtkImageActorPointPlacer()
{
  this->Placer = svtkBoundedPlanePointPlacer::New();
  this->ImageActor = nullptr;
  this->SavedBounds[0] = 0.0;
  this->SavedBounds[1] = 0.0;
  this->SavedBounds[2] = 0.0;
  this->SavedBounds[3] = 0.0;
  this->SavedBounds[4] = 0.0;
  this->SavedBounds[5] = 0.0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = SVTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = SVTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------
svtkImageActorPointPlacer::~svtkImageActorPointPlacer()
{
  this->Placer->Delete();
  this->SetImageActor(nullptr);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double* refWorldPos, double worldPos[3], double worldOrient[9])
{
  if (!this->UpdateInternalState())
  {
    return 0;
  }

  return this->Placer->ComputeWorldPosition(ren, displayPos, refWorldPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double worldOrient[9])
{
  if (!this->UpdateInternalState())
  {
    return 0;
  }

  return this->Placer->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::ValidateWorldPosition(double worldPos[3], double* worldOrient)
{
  if (!this->UpdateInternalState())
  {
    return 0;
  }

  return this->Placer->ValidateWorldPosition(worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::ValidateWorldPosition(double worldPos[3])
{
  if (!this->UpdateInternalState())
  {
    return 0;
  }

  return this->Placer->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::UpdateWorldPosition(
  svtkRenderer* ren, double worldPos[3], double worldOrient[9])
{
  if (!this->UpdateInternalState())
  {
    return 0;
  }

  return this->Placer->UpdateWorldPosition(ren, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkImageActorPointPlacer::UpdateInternalState()
{
  if (!this->ImageActor)
  {
    return 0;
  }

  svtkImageData* input;
  input = this->ImageActor->GetInput();
  if (!input)
  {
    return 0;
  }

  double spacing[3];
  input->GetSpacing(spacing);

  double origin[3];
  input->GetOrigin(origin);

  double bounds[6];
  this->ImageActor->GetBounds(bounds);
  if (this->Bounds[0] != SVTK_DOUBLE_MAX)
  {
    bounds[0] = (bounds[0] < this->Bounds[0]) ? this->Bounds[0] : bounds[0];
    bounds[1] = (bounds[1] > this->Bounds[1]) ? this->Bounds[1] : bounds[1];
    bounds[2] = (bounds[2] < this->Bounds[2]) ? this->Bounds[2] : bounds[2];
    bounds[3] = (bounds[3] > this->Bounds[3]) ? this->Bounds[3] : bounds[3];
    bounds[4] = (bounds[4] < this->Bounds[4]) ? this->Bounds[4] : bounds[4];
    bounds[5] = (bounds[5] > this->Bounds[5]) ? this->Bounds[5] : bounds[5];
  }

  int displayExtent[6];
  this->ImageActor->GetDisplayExtent(displayExtent);

  int axis;
  double position;
  if (displayExtent[0] == displayExtent[1])
  {
    axis = svtkBoundedPlanePointPlacer::XAxis;
    position = origin[0] + displayExtent[0] * spacing[0];
  }
  else if (displayExtent[2] == displayExtent[3])
  {
    axis = svtkBoundedPlanePointPlacer::YAxis;
    position = origin[1] + displayExtent[2] * spacing[1];
  }
  else if (displayExtent[4] == displayExtent[5])
  {
    axis = svtkBoundedPlanePointPlacer::ZAxis;
    position = origin[2] + displayExtent[4] * spacing[2];
  }
  else
  {
    svtkErrorMacro("Incorrect display extent in Image Actor");
    return 0;
  }

  if (axis != this->Placer->GetProjectionNormal() ||
    position != this->Placer->GetProjectionPosition() || bounds[0] != this->SavedBounds[0] ||
    bounds[1] != this->SavedBounds[1] || bounds[2] != this->SavedBounds[2] ||
    bounds[3] != this->SavedBounds[3] || bounds[4] != this->SavedBounds[4] ||
    bounds[5] != this->SavedBounds[5])
  {
    this->SavedBounds[0] = bounds[0];
    this->SavedBounds[1] = bounds[1];
    this->SavedBounds[2] = bounds[2];
    this->SavedBounds[3] = bounds[3];
    this->SavedBounds[4] = bounds[4];
    this->SavedBounds[5] = bounds[5];

    this->Placer->SetProjectionNormal(axis);
    this->Placer->SetProjectionPosition(position);

    this->Placer->RemoveAllBoundingPlanes();

    svtkPlane* plane;

    if (axis != svtkBoundedPlanePointPlacer::XAxis)
    {
      plane = svtkPlane::New();
      plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
      plane->SetNormal(1.0, 0.0, 0.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();

      plane = svtkPlane::New();
      plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
      plane->SetNormal(-1.0, 0.0, 0.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();
    }

    if (axis != svtkBoundedPlanePointPlacer::YAxis)
    {
      plane = svtkPlane::New();
      plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
      plane->SetNormal(0.0, 1.0, 0.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();

      plane = svtkPlane::New();
      plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
      plane->SetNormal(0.0, -1.0, 0.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();
    }

    if (axis != svtkBoundedPlanePointPlacer::ZAxis)
    {
      plane = svtkPlane::New();
      plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
      plane->SetNormal(0.0, 0.0, 1.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();

      plane = svtkPlane::New();
      plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
      plane->SetNormal(0.0, 0.0, -1.0);
      this->Placer->AddBoundingPlane(plane);
      plane->Delete();
    }

    this->Modified();
  }

  return 1;
}

//----------------------------------------------------------------------
void svtkImageActorPointPlacer::SetWorldTolerance(double tol)
{
  if (this->WorldTolerance != (tol < 0.0 ? 0.0 : (tol > SVTK_DOUBLE_MAX ? SVTK_DOUBLE_MAX : tol)))
  {
    this->WorldTolerance = (tol < 0.0 ? 0.0 : (tol > SVTK_DOUBLE_MAX ? SVTK_DOUBLE_MAX : tol));
    this->Placer->SetWorldTolerance(tol);
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkImageActorPointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  const double* bounds = this->GetBounds();
  if (bounds != nullptr)
  {
    os << indent << "Bounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", " << this->Bounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", " << this->Bounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";
  }
  else
  {
    os << indent << "Bounds: (not defined)\n";
  }

  os << indent << "Image Actor: " << this->ImageActor << "\n";
}
