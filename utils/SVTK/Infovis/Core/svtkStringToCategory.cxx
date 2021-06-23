/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToCategory.cxx

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

#include "svtkStringToCategory.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkGraph.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <set>

svtkStandardNewMacro(svtkStringToCategory);

svtkStringToCategory::svtkStringToCategory()
{
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "label");
  this->CategoryArrayName = nullptr;
  this->SetCategoryArrayName("category");
  this->SetNumberOfOutputPorts(2);
}

svtkStringToCategory::~svtkStringToCategory()
{
  this->SetCategoryArrayName(nullptr);
}

int svtkStringToCategory::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* outKeyInfo = outputVector->GetInformationObject(1);

  // Get the input and output objects
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  // This second output stores a list of the unique strings, in the same order
  // as used in the first output.
  svtkTable* stringTable = svtkTable::SafeDownCast(outKeyInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkStringArray* strings =
    svtkArrayDownCast<svtkStringArray>(stringTable->GetColumnByName("Strings"));
  if (strings)
  {
    strings->SetNumberOfTuples(0);
  }
  else
  {
    strings = svtkStringArray::New();
    strings->SetName("Strings");
    stringTable->AddColumn(strings);
    strings->Delete();
  }

  svtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, 0, inputVector);
  svtkStringArray* stringArr = svtkArrayDownCast<svtkStringArray>(arr);
  if (!stringArr)
  {
    svtkErrorMacro("String array input could not be found");
    return 0;
  }

  svtkInformation* arrayInfo = this->GetInputArrayInformation(0);
  // Find where the input array came from
  svtkFieldData* fd =
    output->GetAttributesAsFieldData(arrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION()));
  if (!fd)
  {
    svtkErrorMacro("Could not find where the input array came from");
    return 0;
  }

  // Perform the conversion
  svtkIdType numTuples = stringArr->GetNumberOfTuples();
  int numComp = stringArr->GetNumberOfComponents();
  svtkIntArray* catArr = svtkIntArray::New();
  if (this->CategoryArrayName)
  {
    catArr->SetName(this->CategoryArrayName);
  }
  else
  {
    catArr->SetName("category");
  }
  catArr->SetNumberOfComponents(numComp);
  catArr->SetNumberOfTuples(numTuples);
  fd->AddArray(catArr);
  catArr->Delete();
  svtkIdList* list = svtkIdList::New();
  std::set<svtkStdString> s;
  int category = 0;
  for (svtkIdType i = 0; i < numTuples * numComp; i++)
  {
    if (s.find(stringArr->GetValue(i)) == s.end())
    {
      s.insert(stringArr->GetValue(i));
      strings->InsertNextValue(stringArr->GetValue(i));
      stringArr->LookupValue(stringArr->GetValue(i), list);
      for (svtkIdType j = 0; j < list->GetNumberOfIds(); j++)
      {
        catArr->SetValue(list->GetId(j), category);
      }
      ++category;
    }
  }
  list->Delete();

  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStringToCategory::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkStringToCategory::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataObject* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkStringToCategory::FillOutputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  }
  else
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
  }
  return 1;
}

void svtkStringToCategory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "CategoryArrayName: " << (this->CategoryArrayName ? this->CategoryArrayName : "(null)")
     << endl;
}
