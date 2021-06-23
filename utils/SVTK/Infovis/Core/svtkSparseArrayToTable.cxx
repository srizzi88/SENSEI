/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSparseArrayToTable.cxx

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

#include "svtkSparseArrayToTable.h"
#include "svtkArrayData.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <sstream>
#include <stdexcept>

template <typename ValueT, typename ValueColumnT>
static bool Convert(svtkArray* Array, const char* ValueColumn, svtkTable* Table)
{
  svtkSparseArray<ValueT>* const array = svtkSparseArray<ValueT>::SafeDownCast(Array);
  if (!array)
    return false;

  if (!ValueColumn)
    throw std::runtime_error("ValueColumn not specified.");

  const svtkIdType dimensions = array->GetDimensions();
  const svtkIdType value_count = array->GetNonNullSize();

  for (svtkIdType dimension = 0; dimension != dimensions; ++dimension)
  {
    svtkIdType* const array_coordinates = array->GetCoordinateStorage(dimension);

    svtkIdTypeArray* const table_coordinates = svtkIdTypeArray::New();
    table_coordinates->SetName(array->GetDimensionLabel(dimension));
    table_coordinates->SetNumberOfTuples(value_count);
    std::copy(array_coordinates, array_coordinates + value_count, table_coordinates->GetPointer(0));
    Table->AddColumn(table_coordinates);
    table_coordinates->Delete();
  }

  ValueT* const array_values = array->GetValueStorage();

  ValueColumnT* const table_values = ValueColumnT::New();
  table_values->SetName(ValueColumn);
  table_values->SetNumberOfTuples(value_count);
  std::copy(array_values, array_values + value_count, table_values->GetPointer(0));
  Table->AddColumn(table_values);
  table_values->Delete();

  return true;
}

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkSparseArrayToTable);

// ----------------------------------------------------------------------

svtkSparseArrayToTable::svtkSparseArrayToTable()
  : ValueColumn(nullptr)
{
  this->SetValueColumn("value");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkSparseArrayToTable::~svtkSparseArrayToTable()
{
  this->SetValueColumn(nullptr);
}

// ----------------------------------------------------------------------

void svtkSparseArrayToTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ValueColumn: " << (this->ValueColumn ? this->ValueColumn : "(none)") << endl;
}

int svtkSparseArrayToTable::FillInputPortInformation(int port, svtkInformation* info)
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

int svtkSparseArrayToTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  try
  {
    svtkArrayData* const input_array_data = svtkArrayData::GetData(inputVector[0]);
    if (input_array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error(
        "svtkSparseArrayToTable requires a svtkArrayData containing exactly one array.");

    svtkArray* const input_array = input_array_data->GetArray(static_cast<svtkIdType>(0));

    svtkTable* const output_table = svtkTable::GetData(outputVector);

    if (Convert<double, svtkDoubleArray>(input_array, this->ValueColumn, output_table))
      return 1;
    if (Convert<svtkStdString, svtkStringArray>(input_array, this->ValueColumn, output_table))
      return 1;
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< e.what());
  }

  return 0;
}
