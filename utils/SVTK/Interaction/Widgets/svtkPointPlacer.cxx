/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointPlacer.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointPlacer.h"

#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkPointPlacer);

//----------------------------------------------------------------------
svtkPointPlacer::svtkPointPlacer()
{
  this->PixelTolerance = 5;
  this->WorldTolerance = 0.001;
}

//----------------------------------------------------------------------
svtkPointPlacer::~svtkPointPlacer() = default;

//----------------------------------------------------------------------
int svtkPointPlacer::UpdateWorldPosition(
  svtkRenderer* svtkNotUsed(ren), double* svtkNotUsed(worldPos), double* svtkNotUsed(worldOrient))
{
  return 1;
}

//----------------------------------------------------------------------
int svtkPointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double svtkNotUsed(worldOrient)[9])
{
  if (ren)
  {
    svtkCoordinate* dpos = svtkCoordinate::New();
    dpos->SetCoordinateSystemToDisplay();
    dpos->SetValue(displayPos[0], displayPos[1]);
    double* p = dpos->GetComputedWorldValue(ren);
    worldPos[0] = p[0];
    worldPos[1] = p[1];
    worldPos[2] = p[2];
    dpos->Delete();
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------
int svtkPointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double svtkNotUsed(refWorldPos)[3], double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkPointPlacer::ValidateWorldPosition(double svtkNotUsed(worldPos)[3])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkPointPlacer::ValidateWorldPosition(
  double svtkNotUsed(worldPos)[3], double svtkNotUsed(worldOrient)[9])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkPointPlacer::ValidateDisplayPosition(svtkRenderer*, double svtkNotUsed(displayPos)[2])
{
  return 1;
}

//----------------------------------------------------------------------
void svtkPointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pixel Tolerance: " << this->PixelTolerance << "\n";
  os << indent << "World Tolerance: " << this->WorldTolerance << "\n";
}

//----------------------------------------------------------------------
int svtkPointPlacer::UpdateNodeWorldPosition(
  double svtkNotUsed(worldPos)[3], svtkIdType svtkNotUsed(nodePointId))
{
  return 1;
}
