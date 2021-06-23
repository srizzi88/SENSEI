/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayToTable.cxx

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

#include "svtkArrayToTable.h"
#include "svtkArrayData.h"
#include "svtkCharArray.h"
#include "svtkDenseArray.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkObjectFactory.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnicodeStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"
#include "svtkUnsignedShortArray.h"

#include <sstream>
#include <stdexcept>

/// Convert a 1D array to a table with one column ...
template <typename ValueT, typename ColumnT>
static bool ConvertVector(svtkArray* Array, svtkTable* Output)
{
  if (Array->GetDimensions() != 1)
    return false;

  svtkTypedArray<ValueT>* const array = svtkTypedArray<ValueT>::SafeDownCast(Array);
  if (!array)
    return false;

  const svtkArrayRange extents = array->GetExtent(0);

  ColumnT* const column = ColumnT::New();
  column->SetNumberOfTuples(extents.GetSize());
  column->SetName(array->GetName());
  for (svtkIdType i = extents.GetBegin(); i != extents.GetEnd(); ++i)
  {
    column->SetValue(i - extents.GetBegin(), array->GetValue(i));
  }

  Output->AddColumn(column);
  column->Delete();

  return true;
}

/// Convert a 2D array to a table with 1-or-more columns ...
template <typename ValueT, typename ColumnT>
static bool ConvertMatrix(svtkArray* Array, svtkTable* Output)
{
  if (Array->GetDimensions() != 2)
    return false;

  svtkTypedArray<ValueT>* const array = svtkTypedArray<ValueT>::SafeDownCast(Array);
  if (!array)
    return false;

  svtkSparseArray<ValueT>* const sparse_array = svtkSparseArray<ValueT>::SafeDownCast(array);

  const svtkIdType non_null_count = array->GetNonNullSize();
  const svtkArrayRange columns = array->GetExtent(1);
  const svtkArrayRange rows = array->GetExtent(0);

  std::vector<ColumnT*> new_columns;
  for (svtkIdType j = columns.GetBegin(); j != columns.GetEnd(); ++j)
  {
    std::ostringstream column_name;
    column_name << j;

    ColumnT* const column = ColumnT::New();
    column->SetNumberOfTuples(rows.GetSize());
    column->SetName(column_name.str().c_str());

    if (sparse_array)
    {
      for (svtkIdType i = 0; i != rows.GetSize(); ++i)
        column->SetValue(i, sparse_array->GetNullValue());
    }

    Output->AddColumn(column);
    column->Delete();
    new_columns.push_back(column);
  }

  for (svtkIdType n = 0; n != non_null_count; ++n)
  {
    svtkArrayCoordinates coordinates;
    array->GetCoordinatesN(n, coordinates);

    new_columns[coordinates[1] - columns.GetBegin()]->SetValue(
      coordinates[0] - rows.GetBegin(), array->GetValueN(n));
  }

  return true;
}

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkArrayToTable);

// ----------------------------------------------------------------------

svtkArrayToTable::svtkArrayToTable()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkArrayToTable::~svtkArrayToTable() = default;

// ----------------------------------------------------------------------

void svtkArrayToTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int svtkArrayToTable::FillInputPortInformation(int port, svtkInformation* info)
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

int svtkArrayToTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  try
  {
    svtkArrayData* const input_array_data = svtkArrayData::GetData(inputVector[0]);
    if (!input_array_data)
      throw std::runtime_error("Missing svtkArrayData on input port 0.");
    if (input_array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error(
        "svtkArrayToTable requires a svtkArrayData containing exactly one array.");

    svtkArray* const input_array = input_array_data->GetArray(static_cast<svtkIdType>(0));
    if (input_array->GetDimensions() > 2)
      throw std::runtime_error("svtkArrayToTable input array must have 1 or 2 dimensions.");

    svtkTable* const output_table = svtkTable::GetData(outputVector);

    if (ConvertVector<double, svtkDoubleArray>(input_array, output_table))
      return 1;
    if (ConvertVector<unsigned char, svtkUnsignedCharArray>(input_array, output_table))
      return 1;
    if (ConvertVector<unsigned short, svtkUnsignedShortArray>(input_array, output_table))
      return 1;
    if (ConvertVector<unsigned int, svtkUnsignedIntArray>(input_array, output_table))
      return 1;
    if (ConvertVector<unsigned long, svtkUnsignedLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<unsigned long long, svtkUnsignedLongLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<long unsigned int, svtkUnsignedLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<char, svtkCharArray>(input_array, output_table))
      return 1;
    if (ConvertVector<short, svtkShortArray>(input_array, output_table))
      return 1;
    if (ConvertVector<long, svtkLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<long long, svtkLongLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<long int, svtkLongArray>(input_array, output_table))
      return 1;
    if (ConvertVector<svtkIdType, svtkIdTypeArray>(input_array, output_table))
      return 1;
    if (ConvertVector<svtkStdString, svtkStringArray>(input_array, output_table))
      return 1;
    if (ConvertVector<svtkUnicodeString, svtkUnicodeStringArray>(input_array, output_table))
      return 1;

    if (ConvertMatrix<double, svtkDoubleArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<unsigned char, svtkUnsignedCharArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<unsigned short, svtkUnsignedShortArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<unsigned int, svtkUnsignedIntArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<unsigned long, svtkUnsignedLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<unsigned long long, svtkUnsignedLongLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<long unsigned int, svtkUnsignedLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<char, svtkCharArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<short, svtkShortArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<int, svtkIntArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<long, svtkLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<long long, svtkLongLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<long int, svtkLongArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<svtkIdType, svtkIdTypeArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<svtkStdString, svtkStringArray>(input_array, output_table))
      return 1;
    if (ConvertMatrix<svtkUnicodeString, svtkUnicodeStringArray>(input_array, output_table))
      return 1;

    throw std::runtime_error("Unhandled input array type.");
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< "caught exception: " << e.what() << endl);
  }
  catch (...)
  {
    svtkErrorMacro(<< "caught unknown exception." << endl);
  }

  return 0;
}
