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
#include "svtkAbstractPointLocator.h"
#include "svtkVoronoiKernel.h"

//----------------------------------------------------------------------------
svtkVoronoiKernel::svtkVoronoiKernel() {}

//----------------------------------------------------------------------------
svtkVoronoiKernel::~svtkVoronoiKernel() {}

//----------------------------------------------------------------------------
void svtkVoronoiKernel::svtkIdType ComputeWeights(
  double x[3], svtkIdList* pIds, svtkDoubleArray* weights)
{
}

//----------------------------------------------------------------------------
void svtkVoronoiKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
