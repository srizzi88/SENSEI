/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeTables.cxx

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

#include "svtkMergeTables.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergeColumns.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkMergeTables);
//---------------------------------------------------------------------------
svtkMergeTables::svtkMergeTables()
{
  this->FirstTablePrefix = nullptr;
  this->SecondTablePrefix = nullptr;
  this->MergeColumnsByName = true;
  this->PrefixAllButMerged = false;
  this->SetFirstTablePrefix("Table1.");
  this->SetSecondTablePrefix("Table2.");
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
svtkMergeTables::~svtkMergeTables()
{
  this->SetFirstTablePrefix(nullptr);
  this->SetSecondTablePrefix(nullptr);
}

//---------------------------------------------------------------------------
int svtkMergeTables::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get input tables
  svtkInformation* table1Info = inputVector[0]->GetInformationObject(0);
  svtkTable* table1 = svtkTable::SafeDownCast(table1Info->Get(svtkDataObject::DATA_OBJECT()));
  svtkInformation* table2Info = inputVector[1]->GetInformationObject(0);
  svtkTable* table2 = svtkTable::SafeDownCast(table2Info->Get(svtkDataObject::DATA_OBJECT()));

  // Get output table
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->FirstTablePrefix || !this->SecondTablePrefix)
  {
    svtkErrorMacro("FirstTablePrefix and/or SecondTablePrefix must be non-null.");
    return 0;
  }
  if (!strcmp(this->FirstTablePrefix, this->SecondTablePrefix))
  {
    svtkErrorMacro("FirstTablePrefix and SecondTablePrefix must be different.");
    return 0;
  }

  // Add columns from table 1
  for (int c = 0; c < table1->GetNumberOfColumns(); c++)
  {
    svtkAbstractArray* col = table1->GetColumn(c);
    char* name = col->GetName();
    char* newName = name;
    if (this->PrefixAllButMerged)
    {
      int len = static_cast<int>(strlen(name));
      int prefixLen = static_cast<int>(strlen(this->FirstTablePrefix));
      newName = new char[prefixLen + len + 1];
      strcpy(newName, this->FirstTablePrefix);
      strcat(newName, name);
    }
    svtkAbstractArray* newCol = svtkAbstractArray::CreateArray(col->GetDataType());
    newCol->DeepCopy(col);
    newCol->SetName(newName);
    if (newName != name)
    {
      delete[] newName;
    }
    // svtkWarningMacro("adding column " << newCol->GetName() << " of size " <<
    // newCol->GetNumberOfTuples());
    output->AddColumn(newCol);
    newCol->Delete();
  }

  // Add empty values
  for (int r = 0; r < table2->GetNumberOfRows(); r++)
  {
    output->InsertNextBlankRow();
  }

  // Add columns from table 2
  svtkStringArray* toMerge = svtkStringArray::New();
  svtkTable* tempTable = svtkTable::New();
  for (int c = 0; c < table2->GetNumberOfColumns(); c++)
  {
    svtkAbstractArray* col = table2->GetColumn(c);
    char* name = col->GetName();
    svtkAbstractArray* newCol = svtkAbstractArray::CreateArray(col->GetDataType());
    if (table1->GetColumnByName(name) != nullptr)
    {
      // We have a naming conflict.
      // Rename both columns using the prefixes.
      int len = static_cast<int>(strlen(name));
      char* newName1 = new char[len + strlen(this->FirstTablePrefix) + 1];
      strcpy(newName1, this->FirstTablePrefix);
      strcat(newName1, name);
      if (!this->PrefixAllButMerged)
      {
        svtkAbstractArray* col1 = output->GetColumnByName(name);
        col1->SetName(newName1);
      }
      char* newName2 = new char[len + strlen(this->SecondTablePrefix) + 1];
      strcpy(newName2, this->SecondTablePrefix);
      strcat(newName2, name);
      newCol->SetName(newName2);
      toMerge->InsertNextValue(newName1);
      toMerge->InsertNextValue(newName2);
      toMerge->InsertNextValue(name);
      delete[] newName1;
      delete[] newName2;
    }
    else
    {
      char* newName = name;
      if (this->PrefixAllButMerged)
      {
        int len = static_cast<int>(strlen(name));
        int prefixLen = static_cast<int>(strlen(this->SecondTablePrefix));
        newName = new char[prefixLen + len + 1];
        strcpy(newName, this->SecondTablePrefix);
        strcat(newName, name);
      }
      newCol->SetName(newName);
      if (newName != name)
      {
        delete[] newName;
      }
    }
    tempTable->AddColumn(newCol);
    newCol->Delete();
  }

  // Add empty values
  for (int r = 0; r < table1->GetNumberOfRows(); r++)
  {
    tempTable->InsertNextBlankRow();
  }

  // Add values from table 2
  for (int r = 0; r < table2->GetNumberOfRows(); r++)
  {
    for (int c = 0; c < tempTable->GetNumberOfColumns(); c++)
    {
      svtkAbstractArray* tempCol = tempTable->GetColumn(c);
      svtkAbstractArray* col = table2->GetColumn(c);
      tempCol->InsertNextTuple(r, col);
    }
  }

  // Move the columns from the temp table to the output table
  for (int c = 0; c < tempTable->GetNumberOfColumns(); c++)
  {
    svtkAbstractArray* col = tempTable->GetColumn(c);
    // svtkWarningMacro("adding column " << col->GetName() << " of size " <<
    // col->GetNumberOfTuples());
    output->AddColumn(col);
  }
  tempTable->Delete();

  // Merge any arrays that have the same name
  svtkMergeColumns* mergeColumns = svtkMergeColumns::New();
  svtkTable* temp = svtkTable::New();
  temp->ShallowCopy(output);
  mergeColumns->SetInputData(temp);
  if (this->MergeColumnsByName)
  {
    for (svtkIdType i = 0; i < toMerge->GetNumberOfValues(); i += 3)
    {
      mergeColumns->SetInputArrayToProcess(
        0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, toMerge->GetValue(i).c_str());
      mergeColumns->SetInputArrayToProcess(
        1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, toMerge->GetValue(i + 1).c_str());
      mergeColumns->SetMergedColumnName(toMerge->GetValue(i + 2).c_str());
      mergeColumns->Update();
      temp->ShallowCopy(mergeColumns->GetOutput());
    }
  }
  mergeColumns->Delete();
  toMerge->Delete();

  output->ShallowCopy(temp);
  temp->Delete();

  // Clean up pipeline information
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  output->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(svtkDataObject::DATA_PIECE_NUMBER(), piece);

  return 1;
}

//---------------------------------------------------------------------------
void svtkMergeTables::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "FirstTablePrefix: " << (this->FirstTablePrefix ? this->FirstTablePrefix : "(null)")
     << endl;
  os << indent
     << "SecondTablePrefix: " << (this->SecondTablePrefix ? this->SecondTablePrefix : "(null)")
     << endl;
  os << indent << "MergeColumnsByName: " << (this->MergeColumnsByName ? "on" : "off") << endl;
  os << indent << "PrefixAllButMerged: " << (this->PrefixAllButMerged ? "on" : "off") << endl;
}
