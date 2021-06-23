/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransposeMatrix.cxx

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

#include "svtkTransposeMatrix.h"
#include "svtkCommand.h"
#include "svtkDenseArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"

///////////////////////////////////////////////////////////////////////////////
// svtkTransposeMatrix

svtkStandardNewMacro(svtkTransposeMatrix);

svtkTransposeMatrix::svtkTransposeMatrix() = default;

svtkTransposeMatrix::~svtkTransposeMatrix() = default;

void svtkTransposeMatrix::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int svtkTransposeMatrix::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkArrayData* const input = svtkArrayData::GetData(inputVector[0]);
  if (input->GetNumberOfArrays() != 1)
  {
    svtkErrorMacro(
      << "svtkTransposeMatrix requires svtkArrayData containing exactly one array as input.");
    return 0;
  }

  if (svtkSparseArray<double>* const input_array =
        svtkSparseArray<double>::SafeDownCast(input->GetArray(static_cast<svtkIdType>(0))))
  {
    if (input_array->GetDimensions() != 2)
    {
      svtkErrorMacro(<< "svtkTransposeMatrix requires a matrix as input.");
      return 0;
    }

    const svtkArrayExtents input_extents = input_array->GetExtents();

    svtkSparseArray<double>* const output_array = svtkSparseArray<double>::New();
    output_array->Resize(svtkArrayExtents(input_extents[1], input_extents[0]));
    output_array->SetDimensionLabel(0, input_array->GetDimensionLabel(1));
    output_array->SetDimensionLabel(1, input_array->GetDimensionLabel(0));

    svtkArrayCoordinates coordinates;
    const svtkIdType element_count = input_array->GetNonNullSize();
    for (svtkIdType n = 0; n != element_count; ++n)
    {
      input_array->GetCoordinatesN(n, coordinates);
      output_array->AddValue(
        svtkArrayCoordinates(coordinates[1], coordinates[0]), input_array->GetValueN(n));
    }

    svtkArrayData* const output = svtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();
  }
  else
  {
    svtkDenseArray<double>* const input_array2 =
      svtkDenseArray<double>::SafeDownCast(input->GetArray(static_cast<svtkIdType>(0)));
    if (input_array2 != nullptr)
    {
      if (input_array2->GetDimensions() != 2)
      {
        svtkErrorMacro(<< "svtkTransposeMatrix requires a matrix as input.");
        return 0;
      }

      const svtkArrayExtents input_extents = input_array2->GetExtents();

      svtkDenseArray<double>* const output_array = svtkDenseArray<double>::New();

      output_array->Resize(svtkArrayExtents(input_extents[1], input_extents[0]));
      output_array->SetDimensionLabel(0, input_array2->GetDimensionLabel(1));
      output_array->SetDimensionLabel(1, input_array2->GetDimensionLabel(0));

      for (svtkIdType i = input_extents[0].GetBegin(); i != input_extents[0].GetEnd(); ++i)
      {
        for (svtkIdType j = input_extents[1].GetBegin(); j != input_extents[1].GetEnd(); ++j)
        {
          output_array->SetValue(
            svtkArrayCoordinates(j, i), input_array2->GetValue(svtkArrayCoordinates(i, j)));
        }
      }

      svtkArrayData* const output = svtkArrayData::GetData(outputVector);
      output->ClearArrays();
      output->AddArray(output_array);
      output_array->Delete();
    }
    else
    {
      svtkErrorMacro(<< "Unsupported input array type.");
      return 0;
    }
  }

  return 1;
}
