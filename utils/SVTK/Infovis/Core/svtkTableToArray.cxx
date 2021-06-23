/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToArray.cxx

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

#include "svtkTableToArray.h"
#include "svtkAbstractArray.h"
#include "svtkDenseArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkVariant.h"

#include <algorithm>

class svtkTableToArray::implementation
{
public:
  /// Store the list of columns as an ordered set of variants.  The type
  /// of each variant determines which columns will be inserted into the
  /// output matrix:
  ///
  /// svtkStdString - stores the name of a column to be inserted.
  /// int - stores the index of a column to be inserted.
  /// char 'A' - indicates that every table column should be inserted.
  std::vector<svtkVariant> Columns;
};

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkTableToArray);

// ----------------------------------------------------------------------

svtkTableToArray::svtkTableToArray()
  : Implementation(new implementation())
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkTableToArray::~svtkTableToArray()
{
  delete this->Implementation;
}

// ----------------------------------------------------------------------

void svtkTableToArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (size_t i = 0; i != this->Implementation->Columns.size(); ++i)
    os << indent << "Column: " << this->Implementation->Columns[i] << endl;
}

void svtkTableToArray::ClearColumns()
{
  this->Implementation->Columns.clear();
  this->Modified();
}

void svtkTableToArray::AddColumn(const char* name)
{
  if (!name)
  {
    svtkErrorMacro(<< "cannot add column with nullptr name");
    return;
  }

  this->Implementation->Columns.push_back(svtkVariant(svtkStdString(name)));
  this->Modified();
}

void svtkTableToArray::AddColumn(svtkIdType index)
{
  this->Implementation->Columns.push_back(svtkVariant(static_cast<int>(index)));
  this->Modified();
}

void svtkTableToArray::AddAllColumns()
{
  this->Implementation->Columns.push_back(svtkVariant(static_cast<char>('A')));
  this->Modified();
}

int svtkTableToArray::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

int svtkTableToArray::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* const table = svtkTable::GetData(inputVector[0]);

  std::vector<svtkAbstractArray*> columns;

  for (size_t i = 0; i != this->Implementation->Columns.size(); ++i)
  {
    if (this->Implementation->Columns[i].IsString())
    {
      columns.push_back(
        table->GetColumnByName(this->Implementation->Columns[i].ToString().c_str()));
      if (!columns.back())
      {
        svtkErrorMacro(<< "Missing table column: "
                      << this->Implementation->Columns[i].ToString().c_str());
        return 0;
      }
    }
    else if (this->Implementation->Columns[i].IsInt())
    {
      columns.push_back(table->GetColumn(this->Implementation->Columns[i].ToInt()));
      if (!columns.back())
      {
        svtkErrorMacro(<< "Missing table column: " << this->Implementation->Columns[i].ToInt());
        return 0;
      }
    }
    else if (this->Implementation->Columns[i].IsChar() &&
      this->Implementation->Columns[i].ToChar() == 'A')
    {
      for (svtkIdType j = 0; j != table->GetNumberOfColumns(); ++j)
      {
        columns.push_back(table->GetColumn(j));
      }
    }
  }

  svtkDenseArray<double>* const array = svtkDenseArray<double>::New();
  array->Resize(table->GetNumberOfRows(), static_cast<svtkIdType>(columns.size()));
  array->SetDimensionLabel(0, "row");
  array->SetDimensionLabel(1, "column");

  for (svtkIdType i = 0; i != table->GetNumberOfRows(); ++i)
  {
    for (size_t j = 0; j != columns.size(); ++j)
    {
      array->SetValue(i, static_cast<svtkIdType>(j), columns[j]->GetVariantValue(i).ToDouble());
    }
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}
