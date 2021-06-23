/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeColumns.cxx

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

#include "svtkMergeColumns.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnicodeStringArray.h"

svtkStandardNewMacro(svtkMergeColumns);

svtkMergeColumns::svtkMergeColumns()
{
  this->MergedColumnName = nullptr;
}

svtkMergeColumns::~svtkMergeColumns()
{
  this->SetMergedColumnName(nullptr);
}

template <typename T>
void svtkMergeColumnsCombine(T* col1, T* col2, T* merged, svtkIdType size)
{
  for (svtkIdType i = 0; i < size; i++)
  {
    merged[i] = col1[i] + col2[i];
  }
}

int svtkMergeColumns::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get input tables
  svtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  svtkTable* input = svtkTable::SafeDownCast(inputInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Get output table
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outputInfo->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  svtkAbstractArray* col1 = this->GetInputAbstractArrayToProcess(0, 0, inputVector);
  svtkAbstractArray* col2 = this->GetInputAbstractArrayToProcess(1, 0, inputVector);
  if (!col1)
  {
    svtkErrorMacro("Could not find first column to process.");
    return 0;
  }
  if (!col2)
  {
    svtkErrorMacro("Could not find second column to process.");
    return 0;
  }
  if (col1->GetDataType() != col2->GetDataType())
  {
    svtkErrorMacro("The columns must be of the same type.");
    return 0;
  }

  output->RemoveColumnByName(col1->GetName());
  output->RemoveColumnByName(col2->GetName());

  svtkAbstractArray* merged = svtkAbstractArray::CreateArray(col1->GetDataType());
  merged->SetName(this->MergedColumnName);
  merged->SetNumberOfTuples(col1->GetNumberOfTuples());

  switch (merged->GetDataType())
  {
    case SVTK_STRING:
    {
      svtkStringArray* col1Str = svtkArrayDownCast<svtkStringArray>(col1);
      svtkStringArray* col2Str = svtkArrayDownCast<svtkStringArray>(col2);
      svtkStringArray* mergedStr = svtkArrayDownCast<svtkStringArray>(merged);
      for (svtkIdType i = 0; i < merged->GetNumberOfTuples(); i++)
      {
        svtkStdString combined = col1Str->GetValue(i);
        if (col1Str->GetValue(i).length() > 0 && col2Str->GetValue(i).length() > 0)
        {
          combined += " ";
        }
        combined += col2Str->GetValue(i);
        mergedStr->SetValue(i, combined);
      }
      break;
    }
    case SVTK_UNICODE_STRING:
    {
      svtkUnicodeStringArray* col1Str = svtkArrayDownCast<svtkUnicodeStringArray>(col1);
      svtkUnicodeStringArray* col2Str = svtkArrayDownCast<svtkUnicodeStringArray>(col2);
      svtkUnicodeStringArray* mergedStr = svtkArrayDownCast<svtkUnicodeStringArray>(merged);
      for (svtkIdType i = 0; i < merged->GetNumberOfTuples(); i++)
      {
        svtkUnicodeString combined = col1Str->GetValue(i);
        if (!col1Str->GetValue(i).empty() && !col2Str->GetValue(i).empty())
        {
          combined += svtkUnicodeString::from_utf8(" ");
        }
        combined += col2Str->GetValue(i);
        mergedStr->SetValue(i, combined);
      }
      break;
    }
      svtkTemplateMacro(svtkMergeColumnsCombine(static_cast<SVTK_TT*>(col1->GetVoidPointer(0)),
        static_cast<SVTK_TT*>(col2->GetVoidPointer(0)),
        static_cast<SVTK_TT*>(merged->GetVoidPointer(0)), merged->GetNumberOfTuples()));
  }

  output->AddColumn(merged);
  merged->Delete();

  return 1;
}

void svtkMergeColumns::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "MergedColumnName: " << (this->MergedColumnName ? this->MergedColumnName : "(null)")
     << endl;
}
