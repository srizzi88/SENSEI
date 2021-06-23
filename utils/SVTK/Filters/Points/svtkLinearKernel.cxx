/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinearKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkLinearKernel);

//----------------------------------------------------------------------------
svtkLinearKernel::svtkLinearKernel() = default;

//----------------------------------------------------------------------------
svtkLinearKernel::~svtkLinearKernel() = default;

//----------------------------------------------------------------------------
svtkIdType svtkLinearKernel::ComputeWeights(
  double*, svtkIdList* pIds, svtkDoubleArray* prob, svtkDoubleArray* weights)
{
  svtkIdType numPts = pIds->GetNumberOfIds();
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  weights->SetNumberOfTuples(numPts);
  double* w = weights->GetPointer(0);
  double weight = 1.0 / static_cast<double>(numPts);

  if (!prob) // standard linear interpolation
  {
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      w[i] = weight;
    }
  }

  else // weight by probability
  {
    double sum = 0.0;
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      w[i] = weight * p[i];
      sum += w[i];
    }
    // Now normalize
    if (this->NormalizeWeights && sum != 0.0)
    {
      for (svtkIdType i = 0; i < numPts; ++i)
      {
        w[i] /= sum;
      }
    }
  }

  return numPts;
}

//----------------------------------------------------------------------------
void svtkLinearKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
