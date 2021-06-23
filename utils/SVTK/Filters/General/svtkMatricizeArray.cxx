/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMatricizeArray.cxx

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

#include "svtkMatricizeArray.h"
#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"

#include <numeric>

///////////////////////////////////////////////////////////////////////////////
// svtkMatricizeArray

svtkStandardNewMacro(svtkMatricizeArray);

svtkMatricizeArray::svtkMatricizeArray()
  : SliceDimension(0)
{
}

svtkMatricizeArray::~svtkMatricizeArray() = default;

void svtkMatricizeArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SliceDimension: " << this->SliceDimension << endl;
}

int svtkMatricizeArray::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkArrayData* const input = svtkArrayData::GetData(inputVector[0]);
  if (input->GetNumberOfArrays() != 1)
  {
    svtkErrorMacro(
      << "svtkMatricizeArray requires svtkArrayData containing exactly one array as input.");
    return 0;
  }

  svtkSparseArray<double>* const input_array =
    svtkSparseArray<double>::SafeDownCast(input->GetArray(static_cast<svtkIdType>(0)));
  if (!input_array)
  {
    svtkErrorMacro(<< "svtkMatricizeArray requires a svtkSparseArray<double> as input.");
    return 0;
  }

  if (this->SliceDimension < 0 || this->SliceDimension >= input_array->GetDimensions())
  {
    svtkErrorMacro(<< "Slice dimension " << this->SliceDimension << " out-of-range for array with "
                  << input_array->GetDimensions() << " dimensions.");
    return 0;
  }

  svtkSparseArray<double>* const output_array = svtkSparseArray<double>::New();

  // Compute the extents of the output array ...
  const svtkArrayExtents input_extents = input_array->GetExtents();
  svtkArrayExtents output_extents(0, 0);
  output_extents[0] = input_extents[this->SliceDimension];
  output_extents[1] =
    svtkArrayRange(0, input_extents.GetSize() / input_extents[this->SliceDimension].GetSize());
  output_array->Resize(output_extents);

  // "Map" every non-null element in the input array to its position in the output array.
  // Indices in the slice dimension map directly to the row index in the output.
  // The remaining coordinates are multiplied by a "stride" value for each dimension and
  // the results are summed to compute the output column index.
  //
  // Setting the slice-dimension stride to zero simplifies computation of column coordinates
  // later-on and eliminate an inner-loop comparison.
  std::vector<svtkIdType> strides(input_array->GetDimensions());
  for (svtkIdType i = input_array->GetDimensions() - 1, stride = 1; i >= 0; --i)
  {
    if (i == this->SliceDimension)
    {
      strides[i] = 0;
    }
    else
    {
      strides[i] = stride;
      stride *= input_extents[i].GetSize();
    }
  }

  std::vector<svtkIdType> temp(input_array->GetDimensions());

  svtkArrayCoordinates coordinates;
  svtkArrayCoordinates new_coordinates(0, 0);
  const svtkIdType element_count = input_array->GetNonNullSize();
  for (svtkIdType n = 0; n != element_count; ++n)
  {
    input_array->GetCoordinatesN(n, coordinates);

    new_coordinates[0] = coordinates[this->SliceDimension];

    for (svtkIdType i = 0; i != coordinates.GetDimensions(); ++i)
      temp[i] = (coordinates[i] - input_extents[i].GetBegin()) * strides[i];
    new_coordinates[1] = std::accumulate(temp.begin(), temp.end(), 0);

    output_array->AddValue(new_coordinates, input_array->GetValueN(n));
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}
