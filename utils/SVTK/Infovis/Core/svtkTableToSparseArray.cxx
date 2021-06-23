/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToSparseArray.cxx

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

#include "svtkTableToSparseArray.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLongLongArray.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <algorithm>

class svtkTableToSparseArray::implementation
{
public:
  std::vector<svtkStdString> Coordinates;
  svtkStdString Values;
  svtkArrayExtents OutputExtents;
  bool ExplicitOutputExtents;
};

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkTableToSparseArray);

// ----------------------------------------------------------------------

svtkTableToSparseArray::svtkTableToSparseArray()
  : Implementation(new implementation())
{
  this->Implementation->ExplicitOutputExtents = false;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkTableToSparseArray::~svtkTableToSparseArray()
{
  delete this->Implementation;
}

// ----------------------------------------------------------------------

void svtkTableToSparseArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (size_t i = 0; i != this->Implementation->Coordinates.size(); ++i)
    os << indent << "CoordinateColumn: " << this->Implementation->Coordinates[i] << endl;
  os << indent << "ValueColumn: " << this->Implementation->Values << endl;
  os << indent << "OutputExtents: ";
  if (this->Implementation->ExplicitOutputExtents)
    os << this->Implementation->OutputExtents << endl;
  else
    os << "<none>" << endl;
}

void svtkTableToSparseArray::ClearCoordinateColumns()
{
  this->Implementation->Coordinates.clear();
  this->Modified();
}

void svtkTableToSparseArray::AddCoordinateColumn(const char* name)
{
  if (!name)
  {
    svtkErrorMacro(<< "cannot add coordinate column with nullptr name");
    return;
  }

  this->Implementation->Coordinates.push_back(name);
  this->Modified();
}

void svtkTableToSparseArray::SetValueColumn(const char* name)
{
  if (!name)
  {
    svtkErrorMacro(<< "cannot set value column with nullptr name");
    return;
  }

  this->Implementation->Values = name;
  this->Modified();
}

const char* svtkTableToSparseArray::GetValueColumn()
{
  return this->Implementation->Values.c_str();
}

void svtkTableToSparseArray::ClearOutputExtents()
{
  this->Implementation->ExplicitOutputExtents = false;
  this->Modified();
}

void svtkTableToSparseArray::SetOutputExtents(const svtkArrayExtents& extents)
{
  this->Implementation->ExplicitOutputExtents = true;
  this->Implementation->OutputExtents = extents;
  this->Modified();
}

int svtkTableToSparseArray::FillInputPortInformation(int port, svtkInformation* info)
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

int svtkTableToSparseArray::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* const table = svtkTable::GetData(inputVector[0]);

  std::vector<svtkAbstractArray*> coordinates(this->Implementation->Coordinates.size());
  for (size_t i = 0; i != this->Implementation->Coordinates.size(); ++i)
  {
    coordinates[i] = table->GetColumnByName(this->Implementation->Coordinates[i].c_str());
    if (!coordinates[i])
    {
      svtkErrorMacro(<< "missing coordinate array: "
                    << this->Implementation->Coordinates[i].c_str());
    }
  }
  // See http://developers.sun.com/solaris/articles/cmp_stlport_libCstd.html
  // Language Feature: Partial Specializations
  // Workaround
  int n = 0;
#ifdef _RWSTD_NO_CLASS_PARTIAL_SPEC
  std::count(coordinates.begin(), coordinates.end(), static_cast<svtkAbstractArray*>(0), n);
#else
  n = std::count(coordinates.begin(), coordinates.end(), static_cast<svtkAbstractArray*>(nullptr));
#endif
  if (n != 0)
  {
    return 0;
  }

  svtkAbstractArray* const values = table->GetColumnByName(this->Implementation->Values.c_str());
  if (!values)
  {
    svtkErrorMacro(<< "missing value array: " << this->Implementation->Values.c_str());
    return 0;
  }

  svtkSparseArray<double>* const array = svtkSparseArray<double>::New();
  array->Resize(
    svtkArrayExtents::Uniform(static_cast<svtkArrayExtents::DimensionT>(coordinates.size()), 0));

  for (size_t i = 0; i != coordinates.size(); ++i)
  {
    array->SetDimensionLabel(static_cast<svtkArray::DimensionT>(i), coordinates[i]->GetName());
  }

  svtkArrayCoordinates output_coordinates;
  output_coordinates.SetDimensions(
    static_cast<svtkArrayCoordinates::DimensionT>(coordinates.size()));
  for (svtkIdType i = 0; i != table->GetNumberOfRows(); ++i)
  {
    for (size_t j = 0; j != coordinates.size(); ++j)
    {
      output_coordinates[static_cast<svtkArrayCoordinates::DimensionT>(j)] =
        coordinates[j]->GetVariantValue(i).ToInt();
    }
    array->AddValue(output_coordinates, values->GetVariantValue(i).ToDouble());
  }

  if (this->Implementation->ExplicitOutputExtents)
  {
    array->SetExtents(this->Implementation->OutputExtents);
  }
  else
  {
    array->SetExtentsFromContents();
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}
