/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNormalizeMatrixVectors.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkNormalizeMatrixVectors.h"
#include "svtkArrayCoordinates.h"
#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTypedArray.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// svtkNormalizeMatrixVectors

svtkStandardNewMacro(svtkNormalizeMatrixVectors);

svtkNormalizeMatrixVectors::svtkNormalizeMatrixVectors()
  : VectorDimension(1)
  , PValue(2)
{
}

svtkNormalizeMatrixVectors::~svtkNormalizeMatrixVectors() = default;

void svtkNormalizeMatrixVectors::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "PValue: " << this->PValue << endl;
}

int svtkNormalizeMatrixVectors::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int vector_dimension = std::min(1, std::max(0, this->VectorDimension));
  double p_value = std::max(1.0, this->PValue);

  svtkArrayData* const input = svtkArrayData::GetData(inputVector[0]);
  if (input->GetNumberOfArrays() != 1)
  {
    svtkErrorMacro(
      << "svtkNormalizeMatrixVectors requires svtkArrayData containing exactly one array as input.");
    return 0;
  }

  svtkTypedArray<double>* const input_array =
    svtkTypedArray<double>::SafeDownCast(input->GetArray(static_cast<svtkIdType>(0)));
  if (!input_array)
  {
    svtkErrorMacro(<< "svtkNormalizeMatrixVectors requires a svtkTypedArray<double> as input.");
    return 0;
  }
  if (input_array->GetDimensions() != 2)
  {
    svtkErrorMacro(<< "svtkNormalizeMatrixVectors requires a matrix as input.");
    return 0;
  }

  svtkTypedArray<double>* const output_array =
    svtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());

  const svtkArrayRange vectors = input_array->GetExtent(vector_dimension);
  const svtkIdType value_count = input_array->GetNonNullSize();

  // Create temporary storage for computed vector weights ...
  std::vector<double> weight(vectors.GetSize(), 0.0);

  // Store the sum of the squares of each vector value ...
  svtkArrayCoordinates coordinates;
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    output_array->GetCoordinatesN(n, coordinates);
    weight[coordinates[vector_dimension] - vectors.GetBegin()] +=
      pow(output_array->GetValueN(n), p_value);
  }

  // Convert the sums into weights, avoiding divide-by-zero ...
  for (svtkIdType i = 0; i != vectors.GetSize(); ++i)
  {
    const double length = pow(weight[i], 1.0 / p_value);
    weight[i] = length ? 1.0 / length : 0.0;
  }

  // Apply the weights to each vector ...
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    output_array->GetCoordinatesN(n, coordinates);
    output_array->SetValueN(
      n, output_array->GetValueN(n) * weight[coordinates[vector_dimension] - vectors.GetBegin()]);
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}
