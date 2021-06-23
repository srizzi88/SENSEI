/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeneralizedKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGeneralizedKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

//----------------------------------------------------------------------------
svtkGeneralizedKernel::svtkGeneralizedKernel()
{
  this->KernelFootprint = svtkGeneralizedKernel::RADIUS;
  this->Radius = 1.0;
  this->NumberOfPoints = 8;
  this->NormalizeWeights = true;
}

//----------------------------------------------------------------------------
svtkGeneralizedKernel::~svtkGeneralizedKernel() = default;

//----------------------------------------------------------------------------
svtkIdType svtkGeneralizedKernel::ComputeBasis(double x[3], svtkIdList* pIds, svtkIdType)
{
  if (this->KernelFootprint == svtkGeneralizedKernel::RADIUS)
  {
    this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  }
  else
  {
    this->Locator->FindClosestNPoints(this->NumberOfPoints, x, pIds);
  }

  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
void svtkGeneralizedKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Kernel Footprint: " << this->GetKernelFootprint() << "\n";
  os << indent << "Radius: " << this->GetRadius() << "\n";
  os << indent << "Number of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Normalize Weights: " << (this->GetNormalizeWeights() ? "On\n" : "Off\n");
}
