/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientedPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOrientedPolygonalHandleRepresentation3D.h"
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkFollower.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkTransformPolyDataFilter.h"

svtkStandardNewMacro(svtkOrientedPolygonalHandleRepresentation3D);

//----------------------------------------------------------------------
svtkOrientedPolygonalHandleRepresentation3D ::svtkOrientedPolygonalHandleRepresentation3D()
{
  this->Actor = svtkFollower::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->HandlePicker->AddPickList(this->Actor);
}

//----------------------------------------------------------------------
svtkOrientedPolygonalHandleRepresentation3D ::~svtkOrientedPolygonalHandleRepresentation3D() =
  default;

//----------------------------------------------------------------------
void svtkOrientedPolygonalHandleRepresentation3D::UpdateHandle()
{
  this->Superclass::UpdateHandle();

  // Our handle actor is a follower. It follows the camera set on it.
  if (this->Renderer)
  {
    svtkFollower* follower = svtkFollower::SafeDownCast(this->Actor);
    if (follower)
    {
      follower->SetCamera(this->Renderer->GetActiveCamera());
    }
  }

  // Update the actor position
  double handlePosition[3];
  this->GetWorldPosition(handlePosition);
  this->Actor->SetPosition(handlePosition);
}

//----------------------------------------------------------------------
void svtkOrientedPolygonalHandleRepresentation3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
