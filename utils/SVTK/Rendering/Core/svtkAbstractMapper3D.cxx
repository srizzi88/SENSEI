/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractMapper3D.h"
#include "svtkDataSet.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkPlaneCollection.h"

// Construct with initial range (0,1).
svtkAbstractMapper3D::svtkAbstractMapper3D()
{
  svtkMath::UninitializeBounds(this->Bounds);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

// Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void svtkAbstractMapper3D::GetBounds(double bounds[6])
{
  this->GetBounds();
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->Bounds[i];
  }
}

double* svtkAbstractMapper3D::GetCenter()
{
  this->GetBounds();
  for (int i = 0; i < 3; i++)
  {
    this->Center[i] = (this->Bounds[2 * i + 1] + this->Bounds[2 * i]) / 2.0;
  }
  return this->Center;
}

double svtkAbstractMapper3D::GetLength()
{
  double diff, l = 0.0;
  int i;

  this->GetBounds();
  for (i = 0; i < 3; i++)
  {
    diff = this->Bounds[2 * i + 1] - this->Bounds[2 * i];
    l += diff * diff;
  }

  return sqrt(l);
}

void svtkAbstractMapper3D::GetClippingPlaneInDataCoords(
  svtkMatrix4x4* propMatrix, int i, double hnormal[4])
{
  svtkPlaneCollection* clipPlanes = this->ClippingPlanes;
  const double* mat = *propMatrix->Element;

  if (clipPlanes)
  {
    int n = clipPlanes->GetNumberOfItems();
    if (i >= 0 && i < n)
    {
      // Get the plane
      svtkPlane* plane = clipPlanes->GetItem(i);
      double* normal = plane->GetNormal();
      double* origin = plane->GetOrigin();

      // Compute the plane equation
      double v1 = normal[0];
      double v2 = normal[1];
      double v3 = normal[2];
      double v4 = -(v1 * origin[0] + v2 * origin[1] + v3 * origin[2]);

      // Transform normal from world to data coords
      hnormal[0] = v1 * mat[0] + v2 * mat[4] + v3 * mat[8] + v4 * mat[12];
      hnormal[1] = v1 * mat[1] + v2 * mat[5] + v3 * mat[9] + v4 * mat[13];
      hnormal[2] = v1 * mat[2] + v2 * mat[6] + v3 * mat[10] + v4 * mat[14];
      hnormal[3] = v1 * mat[3] + v2 * mat[7] + v3 * mat[11] + v4 * mat[15];

      return;
    }
  }

  svtkErrorMacro("Clipping plane index " << i << " is out of range.");
}

void svtkAbstractMapper3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
