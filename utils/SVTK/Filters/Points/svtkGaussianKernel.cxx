/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGaussianKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkGaussianKernel);

//----------------------------------------------------------------------------
svtkGaussianKernel::svtkGaussianKernel()
{
  this->Sharpness = 2.0;
  this->F2 = this->Sharpness / this->Radius;
}

//----------------------------------------------------------------------------
svtkGaussianKernel::~svtkGaussianKernel() = default;

//----------------------------------------------------------------------------
void svtkGaussianKernel::Initialize(svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* pd)
{
  this->Superclass::Initialize(loc, ds, pd);

  this->F2 = this->Sharpness / this->Radius;
  this->F2 = this->F2 * this->F2;
}

//----------------------------------------------------------------------------
svtkIdType svtkGaussianKernel::ComputeWeights(
  double x[3], svtkIdList* pIds, svtkDoubleArray* prob, svtkDoubleArray* weights)
{
  svtkIdType numPts = pIds->GetNumberOfIds();
  double d2, y[3], sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  double* w = weights->GetPointer(0);
  double f2 = this->F2;

  for (svtkIdType i = 0; i < numPts; ++i)
  {
    svtkIdType id = pIds->GetId(i);
    this->DataSet->GetPoint(id, y);
    d2 = svtkMath::Distance2BetweenPoints(x, y);

    if (svtkMathUtilities::FuzzyCompare(
          d2, 0.0, std::numeric_limits<double>::epsilon() * 256.0)) // precise hit on existing point
    {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0, id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0, 1.0);
      return 1;
    }
    else
    {
      w[i] = (p ? p[i] * exp(-f2 * d2) : exp(-f2 * d2));
      sum += w[i];
    }
  } // over all points

  // Normalize
  if (this->NormalizeWeights && sum != 0.0)
  {
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      w[i] /= sum;
    }
  }

  return numPts;
}

//----------------------------------------------------------------------------
void svtkGaussianKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sharpness: " << this->GetSharpness() << endl;
}
