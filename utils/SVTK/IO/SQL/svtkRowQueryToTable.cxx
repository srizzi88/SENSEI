/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRowQueryToTable.cxx

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
#include "svtkRowQueryToTable.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRowQuery.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTypeUInt64Array.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include <sstream>

svtkStandardNewMacro(svtkRowQueryToTable);

svtkRowQueryToTable::svtkRowQueryToTable()
{
  this->SetNumberOfInputPorts(0);
  this->Query = nullptr;
}

svtkRowQueryToTable::~svtkRowQueryToTable()
{
  if (this->Query)
  {
    this->Query->Delete();
    this->Query = nullptr;
  }
}

void svtkRowQueryToTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query: " << (this->Query ? "" : "nullptr") << endl;
  if (this->Query)
  {
    this->Query->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkCxxSetObjectMacro(svtkRowQueryToTable, Query, svtkRowQuery);

svtkMTimeType svtkRowQueryToTable::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Query != nullptr)
  {
    svtkMTimeType time = this->Query->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

int svtkRowQueryToTable::RequestData(svtkInformation*, svtkInformationVector** svtkNotUsed(inputVector),
  svtkInformationVector* outputVector)
{
  if (this->Query == nullptr)
  {
    svtkErrorMacro("Query undefined.");
    return 0;
  }

  svtkTable* output = svtkTable::GetData(outputVector);

  // Set up the columns
  this->Query->Execute();

  // Check for query error
  if (this->Query->HasError())
  {
    svtkErrorMacro("Query Error: " << this->Query->GetLastErrorText());
    return 0;
  }
  int cols = this->Query->GetNumberOfFields();
  for (int c = 0; c < cols; c++)
  {
    svtkAbstractArray* arr;
    int type = this->Query->GetFieldType(c);

    // Take care of the special case of uint64
    // to ensure timepoints have specific array type
    if (type == SVTK_TYPE_UINT64)
    {
      arr = svtkTypeUInt64Array::New();
    }
    else if (type == 0)
    {
      arr = svtkAbstractArray::CreateArray(SVTK_DOUBLE);
    }
    else
    {
      arr = svtkAbstractArray::CreateArray(type);
    }

    // Make sure name doesn't clash with existing name.
    const char* name = this->Query->GetFieldName(c);
    if (output->GetColumnByName(name))
    {
      int i = 1;
      std::ostringstream oss;
      svtkStdString newName;
      do
      {
        oss.str("");
        oss << name << "_" << i;
        newName = oss.str();
        ++i;
      } while (output->GetColumnByName(newName));
      arr->SetName(newName);
    }
    else
    {
      arr->SetName(name);
    }

    // Add the column to the table.
    output->AddColumn(arr);
    arr->Delete();
  }

  // Fill the table
  int numRows = 0;
  float progressGuess = 0;
  svtkVariantArray* rowArray = svtkVariantArray::New();
  while (this->Query->NextRow(rowArray))
  {
    output->InsertNextRow(rowArray);

    // Update progress every 100 rows
    numRows++;
    if ((numRows % 100) == 0)
    {
      // 1% for every 100 rows, and then 'spin around'
      progressGuess = ((numRows / 100) % 100) * .01;
      this->UpdateProgress(progressGuess);
    }
  }
  rowArray->Delete();

  return 1;
}
