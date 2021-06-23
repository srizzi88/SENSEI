/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAdjacencyMatrixToEdgeTable.cxx

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

#include "svtkAdjacencyMatrixToEdgeTable.h"
#include "svtkArrayData.h"
#include "svtkCommand.h"
#include "svtkDenseArray.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <algorithm>
#include <functional>
#include <map>

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkAdjacencyMatrixToEdgeTable);

// ----------------------------------------------------------------------

svtkAdjacencyMatrixToEdgeTable::svtkAdjacencyMatrixToEdgeTable()
  : SourceDimension(0)
  , ValueArrayName(nullptr)
  , MinimumCount(0)
  , MinimumThreshold(0.5)
{
  this->SetValueArrayName("value");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkAdjacencyMatrixToEdgeTable::~svtkAdjacencyMatrixToEdgeTable()
{
  this->SetValueArrayName(nullptr);
}

// ----------------------------------------------------------------------

void svtkAdjacencyMatrixToEdgeTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceDimension: " << this->SourceDimension << endl;
  os << indent << "ValueArrayName: " << (this->ValueArrayName ? this->ValueArrayName : "") << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MinimumThreshold: " << this->MinimumThreshold << endl;
}

int svtkAdjacencyMatrixToEdgeTable::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

int svtkAdjacencyMatrixToEdgeTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkArrayData* const input = svtkArrayData::GetData(inputVector[0]);
  if (input->GetNumberOfArrays() != 1)
  {
    svtkErrorMacro(<< this->GetClassName()
                  << " requires an input svtkArrayData containing one array.");
    return 0;
  }

  svtkDenseArray<double>* const input_array =
    svtkDenseArray<double>::SafeDownCast(input->GetArray(static_cast<svtkIdType>(0)));
  if (!input_array)
  {
    svtkErrorMacro(<< this->GetClassName() << " requires an input svtkDenseArray<double>.");
    return 0;
  }
  if (input_array->GetDimensions() != 2)
  {
    svtkErrorMacro(<< this->GetClassName() << " requires an input matrix.");
    return 0;
  }

  const svtkArrayExtents input_extents = input_array->GetExtents();

  const svtkIdType source_dimension =
    std::max(static_cast<svtkIdType>(0), std::min(static_cast<svtkIdType>(1), this->SourceDimension));
  const svtkIdType target_dimension = 1 - source_dimension;

  svtkTable* const output_table = svtkTable::GetData(outputVector);

  svtkIdTypeArray* const source_array = svtkIdTypeArray::New();
  source_array->SetName(input_array->GetDimensionLabel(source_dimension));

  svtkIdTypeArray* const target_array = svtkIdTypeArray::New();
  target_array->SetName(input_array->GetDimensionLabel(target_dimension));

  svtkDoubleArray* const value_array = svtkDoubleArray::New();
  value_array->SetName(this->ValueArrayName);

  // For each source in the matrix ...
  svtkArrayCoordinates coordinates(0, 0);
  for (svtkIdType i = input_extents[source_dimension].GetBegin();
       i != input_extents[source_dimension].GetEnd(); ++i)
  {
    coordinates[source_dimension] = i;

    // Create a sorted list of source values ...
    typedef std::multimap<double, svtkIdType, std::greater<double> > sorted_values_t;
    sorted_values_t sorted_values;
    for (svtkIdType j = input_extents[target_dimension].GetBegin();
         j != input_extents[target_dimension].GetEnd(); ++j)
    {
      coordinates[target_dimension] = j;

#ifdef _RWSTD_NO_MEMBER_TEMPLATES
      // Deal with Sun Studio old libCstd.
      // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
      sorted_values.insert(
        std::pair<const double, svtkIdType>(input_array->GetValue(coordinates), j));
#else
      sorted_values.insert(std::make_pair(input_array->GetValue(coordinates), j));
#endif
    }

    // Create edges for each value that meets our count / threshold criteria ...
    svtkIdType count = 0;
    for (sorted_values_t::const_iterator value = sorted_values.begin();
         value != sorted_values.end(); ++value, ++count)
    {
      if (count < this->MinimumCount || value->first >= this->MinimumThreshold)
      {
        source_array->InsertNextValue(i);
        target_array->InsertNextValue(value->second);
        value_array->InsertNextValue(value->first);
      }
    }

    double progress = static_cast<double>(i - input_extents[source_dimension].GetBegin()) /
      static_cast<double>(input_extents[source_dimension].GetSize());
    this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
  }

  output_table->AddColumn(source_array);
  output_table->AddColumn(target_array);
  output_table->AddColumn(value_array);

  source_array->Delete();
  target_array->Delete();
  value_array->Delete();

  return 1;
}
