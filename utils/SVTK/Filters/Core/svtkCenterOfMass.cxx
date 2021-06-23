/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCenterOfMass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCenterOfMass.h"

#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSmartPointer.h"

#include <cassert>

svtkStandardNewMacro(svtkCenterOfMass);

svtkCenterOfMass::svtkCenterOfMass()
{
  this->UseScalarsAsWeights = false;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->SetNumberOfOutputPorts(0);
}

void svtkCenterOfMass::ComputeCenterOfMass(
  svtkPoints* points, svtkDataArray* scalars, double center[3])
{
  svtkIdType n = points->GetNumberOfPoints();
  // Initialize the center to zero
  center[0] = 0.0;
  center[1] = 0.0;
  center[2] = 0.0;

  assert("pre: no points" && n > 0);

  if (scalars)
  {
    // If weights are to be used
    double weightTotal = 0.0;

    assert("pre: wrong array size" && scalars->GetNumberOfTuples() == n);

    for (svtkIdType i = 0; i < n; i++)
    {
      double point[3];
      points->GetPoint(i, point);

      double weight = scalars->GetComponent(0, i);
      weightTotal += weight;

      svtkMath::MultiplyScalar(point, weight);
      svtkMath::Add(center, point, center);
    }

    assert("pre: sum of weights must be positive" && weightTotal > 0.0);

    if (weightTotal > 0.0)
    {
      svtkMath::MultiplyScalar(center, 1.0 / weightTotal);
    }
  }
  else
  {
    // No weights
    for (svtkIdType i = 0; i < n; i++)
    {
      double point[3];
      points->GetPoint(i, point);

      svtkMath::Add(center, point, center);
    }

    svtkMath::MultiplyScalar(center, 1.0 / n);
  }
}

int svtkCenterOfMass::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // Get the input and output
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* points = input->GetPoints();

  if (points == nullptr || points->GetNumberOfPoints() == 0)
  {
    svtkErrorMacro("Input must have at least 1 point!");
    return 1;
  }

  svtkDataArray* scalars = nullptr;
  if (this->UseScalarsAsWeights)
  {
    scalars = input->GetPointData()->GetScalars();

    if (!scalars)
    {
      svtkErrorWithObjectMacro(input, "To use weights PointData::Scalars must be set!");
      return 1;
    }
  }

  this->ComputeCenterOfMass(points, scalars, this->Center);

  return 1;
}

void svtkCenterOfMass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center[0] << " " << this->Center[1] << " " << this->Center[2]
     << endl;
  os << indent << "UseScalarsAsWeights: " << this->UseScalarsAsWeights << endl;
}
