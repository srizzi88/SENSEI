/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVoronoiKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVoronoiKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkVoronoiKernel);

//----------------------------------------------------------------------------
svtkVoronoiKernel::svtkVoronoiKernel() = default;

//----------------------------------------------------------------------------
svtkVoronoiKernel::~svtkVoronoiKernel() = default;

//----------------------------------------------------------------------------
svtkIdType svtkVoronoiKernel::ComputeBasis(double x[3], svtkIdList* pIds, svtkIdType)
{
  pIds->SetNumberOfIds(1);
  svtkIdType pId = this->Locator->FindClosestPoint(x);
  pIds->SetId(0, pId);

  return 1;
}

//----------------------------------------------------------------------------
svtkIdType svtkVoronoiKernel::ComputeWeights(double*, svtkIdList*, svtkDoubleArray* weights)
{
  weights->SetNumberOfTuples(1);
  weights->SetValue(0, 1.0);

  return 1;
}

//----------------------------------------------------------------------------
void svtkVoronoiKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
