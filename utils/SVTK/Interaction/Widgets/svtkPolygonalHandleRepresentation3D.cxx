/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolygonalHandleRepresentation3D.h"
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkCoordinate.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointPlacer.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkPolygonalHandleRepresentation3D);

//----------------------------------------------------------------------
svtkPolygonalHandleRepresentation3D::svtkPolygonalHandleRepresentation3D()
{
  this->Offset[0] = this->Offset[1] = this->Offset[2] = 0.0;

  this->Actor = svtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->HandlePicker->AddPickList(this->Actor);
}

//-------------------------------------------------------------------------
void svtkPolygonalHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (!this->Renderer || !this->PointPlacer || this->PointPlacer->ValidateWorldPosition(p))
  {
    this->HandleTransformMatrix->SetElement(0, 3, p[0] - this->Offset[0]);
    this->HandleTransformMatrix->SetElement(1, 3, p[1] - this->Offset[1]);
    this->HandleTransformMatrix->SetElement(2, 3, p[2] - this->Offset[2]);

    this->WorldPosition->SetValue(this->HandleTransformMatrix->GetElement(0, 3),
      this->HandleTransformMatrix->GetElement(1, 3), this->HandleTransformMatrix->GetElement(2, 3));

    this->WorldPositionTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkPolygonalHandleRepresentation3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Offset: (" << this->Offset[0] << "," << this->Offset[1] << ")\n";
}
