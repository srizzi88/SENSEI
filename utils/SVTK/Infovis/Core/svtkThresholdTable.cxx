/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkThresholdTable.h"

#include "svtkArrayIteratorIncludes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

svtkStandardNewMacro(svtkThresholdTable);

svtkThresholdTable::svtkThresholdTable()
  : MinValue(0)
  , MaxValue(SVTK_INT_MAX)
  , Mode(0)
{
}

svtkThresholdTable::~svtkThresholdTable() = default;

void svtkThresholdTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MinValue: " << this->MinValue.ToString() << endl;
  os << indent << "MaxValue: " << this->MaxValue.ToString() << endl;
  os << indent << "Mode: ";
  switch (this->Mode)
  {
    case ACCEPT_LESS_THAN:
      os << "Accept less than";
      break;
    case ACCEPT_GREATER_THAN:
      os << "Accept greater than";
      break;
    case ACCEPT_BETWEEN:
      os << "Accept between";
      break;
    case ACCEPT_OUTSIDE:
      os << "Accept outside";
      break;
    default:
      os << "Undefined";
      break;
  }
  os << endl;
}

static bool svtkThresholdTableCompare(svtkVariant a, svtkVariant b)
{
  return a.ToDouble() <= b.ToDouble();
}

template <typename iterT>
void svtkThresholdTableThresholdRows(
  iterT* it, svtkTable* input, svtkTable* output, svtkVariant min, svtkVariant max, int mode)
{
  svtkIdType maxInd = it->GetNumberOfValues();
  for (svtkIdType i = 0; i < maxInd; i++)
  {
    bool accept = false;
    svtkVariant v(it->GetValue(i));
    if (mode == svtkThresholdTable::ACCEPT_LESS_THAN)
    {
      accept = svtkThresholdTableCompare(v, max);
    }
    else if (mode == svtkThresholdTable::ACCEPT_GREATER_THAN)
    {
      accept = svtkThresholdTableCompare(min, v);
    }
    else if (mode == svtkThresholdTable::ACCEPT_BETWEEN)
    {
      accept = (svtkThresholdTableCompare(min, v) && svtkThresholdTableCompare(v, max));
    }
    else if (mode == svtkThresholdTable::ACCEPT_OUTSIDE)
    {
      accept = (svtkThresholdTableCompare(v, min) || svtkThresholdTableCompare(max, v));
    }
    if (accept)
    {
      svtkVariantArray* row = input->GetRow(i);
      output->InsertNextRow(row);
    }
  }
}

void svtkThresholdTable::ThresholdBetween(svtkVariant lower, svtkVariant upper)
{
  if (this->MinValue != lower || this->MaxValue != upper ||
    this->Mode != svtkThresholdTable::ACCEPT_BETWEEN)
  {
    this->MinValue = lower;
    this->MaxValue = upper;
    this->Mode = svtkThresholdTable::ACCEPT_BETWEEN;
    this->Modified();
  }
}

int svtkThresholdTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == nullptr)
  {
    svtkErrorMacro("An input array must be specified.");
    return 0;
  }

  svtkTable* input = svtkTable::GetData(inputVector[0]);
  svtkTable* output = svtkTable::GetData(outputVector);

  for (int n = 0; n < input->GetNumberOfColumns(); n++)
  {
    svtkAbstractArray* col = input->GetColumn(n);
    svtkAbstractArray* ncol = svtkAbstractArray::CreateArray(col->GetDataType());
    ncol->SetName(col->GetName());
    ncol->SetNumberOfComponents(col->GetNumberOfComponents());
    output->AddColumn(ncol);
    ncol->Delete();
  }

  svtkArrayIterator* iter = arr->NewIterator();
  switch (arr->GetDataType())
  {
    svtkArrayIteratorTemplateMacro(svtkThresholdTableThresholdRows(
      static_cast<SVTK_TT*>(iter), input, output, this->MinValue, this->MaxValue, this->Mode));
  }
  iter->Delete();

  return 1;
}
