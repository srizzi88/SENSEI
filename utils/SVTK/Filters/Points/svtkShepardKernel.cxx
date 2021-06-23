/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShepardKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkShepardKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkShepardKernel);

//----------------------------------------------------------------------------
svtkShepardKernel::svtkShepardKernel()
{
  this->PowerParameter = 2.0;
}

//----------------------------------------------------------------------------
svtkShepardKernel::~svtkShepardKernel() = default;

//----------------------------------------------------------------------------
svtkIdType svtkShepardKernel::ComputeWeights(
  double x[3], svtkIdList* pIds, svtkDoubleArray* prob, svtkDoubleArray* weights)
{
  svtkIdType numPts = pIds->GetNumberOfIds();
  double d, y[3], sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  double* w = weights->GetPointer(0);

  for (svtkIdType i = 0; i < numPts; ++i)
  {
    svtkIdType id = pIds->GetId(i);
    this->DataSet->GetPoint(id, y);
    if (this->PowerParameter == 2.0)
    {
      d = svtkMath::Distance2BetweenPoints(x, y);
    }
    else
    {
      d = pow(sqrt(svtkMath::Distance2BetweenPoints(x, y)), this->PowerParameter);
    }
    if (svtkMathUtilities::FuzzyCompare(
          d, 0.0, std::numeric_limits<double>::epsilon() * 256.0)) // precise hit on existing point
    {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0, id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0, 1.0);
      return 1;
    }
    else
    {
      w[i] = (p ? p[i] / d : 1.0 / d); // take into account probability if provided
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
void svtkShepardKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Power Parameter: " << this->GetPowerParameter() << "\n";
}
