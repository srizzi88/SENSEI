/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMergeFields.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkMergeFields);

char svtkMergeFields::FieldLocationNames[3][12] = { "DATA_OBJECT", "POINT_DATA", "CELL_DATA" };

typedef svtkMergeFields::Component Component;

svtkMergeFields::svtkMergeFields()
{
  this->FieldName = nullptr;
  this->FieldLocation = -1;
  this->NumberOfComponents = 0;

  this->Head = nullptr;
  this->Tail = nullptr;
}

svtkMergeFields::~svtkMergeFields()
{
  delete[] this->FieldName;
  this->FieldName = nullptr;
  this->DeleteAllComponents();
}

void svtkMergeFields::SetOutputField(const char* name, int fieldLoc)
{
  if (!name)
  {
    return;
  }

  if ((fieldLoc != svtkMergeFields::DATA_OBJECT) && (fieldLoc != svtkMergeFields::POINT_DATA) &&
    (fieldLoc != svtkMergeFields::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name) + 1];
  strcpy(this->FieldName, name);
}

void svtkMergeFields::SetOutputField(const char* name, const char* fieldLoc)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  int numFieldLocs = 3;
  int i;

  // Convert fieldLoc to int an call the other SetOutputField()
  int loc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(fieldLoc, FieldLocationNames[i]))
    {
      loc = i;
      break;
    }
  }
  if (loc == -1)
  {
    svtkErrorMacro("Location for the field is invalid.");
    return;
  }

  this->SetOutputField(name, loc);
}

void svtkMergeFields::Merge(int component, const char* arrayName, int sourceComp)
{
  if (!arrayName)
  {
    return;
  }

  this->Modified();
  Component* comp = this->FindComponent(component);
  if (comp)
  {
    // If component already exists, replace information
    comp->SetName(arrayName);
    comp->SourceIndex = sourceComp;
  }
  else
  {
    // otherwise create a new one
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    comp->SourceIndex = sourceComp;
    this->AddComponent(comp);
  }
}

int svtkMergeFields::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // This has to be here because it initialized all field datas.
  output->CopyStructure(input);

  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  svtkFieldData* fd = nullptr;
  svtkFieldData* outputFD = nullptr;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur)
  {
    return 1;
  }

  // Get the input and output field data
  if (this->FieldLocation == svtkMergeFields::DATA_OBJECT)
  {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
  }
  else if (this->FieldLocation == svtkMergeFields::POINT_DATA)
  {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
  }
  else if (this->FieldLocation == svtkMergeFields::CELL_DATA)
  {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
  }

  if (!fd || !outputFD)
  {
    svtkErrorMacro("No field data in svtkDataObject.");
    return 1;
  }

  // Check if the data types of the input fields are the same
  // Otherwise warn the user.
  // Check if the number of tuples are the same for all arrays.
  svtkDataArray* inputArray;
  int dataType = -1;
  int sameDataType = 1;
  int numTuples = -1;
  int sameNumTuples = 1;
  do
  {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (!inputArray)
    {
      continue;
    }
    else
    {
      if (dataType == -1)
      {
        dataType = inputArray->GetDataType();
      }
      else
      {
        if (inputArray->GetDataType() != dataType)
        {
          sameDataType = 0;
        }
      }
      if (numTuples == -1)
      {
        numTuples = inputArray->GetNumberOfTuples();
      }
      else
      {
        if (inputArray->GetNumberOfTuples() != numTuples)
        {
          sameNumTuples = 0;
        }
      }
    }
  } while (cur);
  if (!sameNumTuples)
  {
    svtkErrorMacro("The number of tuples in the input arrays do not match.");
    return 1;
  }
  if (dataType == -1)
  {
    svtkErrorMacro("No input array(s) were found.");
    return 1;
  }
  svtkDataArray* outputArray;
  if (!sameDataType)
  {
    svtkWarningMacro("The input data types do not match. The output will be "
      << "float. This will potentially cause accuracy and speed.");
    outputArray = svtkFloatArray::New();
  }
  else
  {
    outputArray = svtkDataArray::CreateDataArray(dataType);
  }

  if (this->NumberOfComponents <= 0)
  {
    svtkErrorMacro("NumberOfComponents has be set prior to the execution of "
                  "this filter");
  }

  outputArray->SetNumberOfComponents(this->NumberOfComponents);
  outputArray->SetNumberOfTuples(numTuples);
  outputArray->SetName(this->FieldName);

  // Merge
  cur = this->GetFirst();
  do
  {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (inputArray)
    {
      if (!this->MergeArray(inputArray, outputArray, before->SourceIndex, before->Index))
      {
        outputArray->Delete();
        return 1;
      }
    }
    else
    {
      if (before->FieldName)
      {
        svtkWarningMacro("Input array " << before->FieldName << " does not exist.");
      }
      continue;
    }
  } while (cur);
  outputFD->AddArray(outputArray);
  outputArray->Delete();

  return 1;
}

namespace {

struct MergeFieldsWorker
{
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* input, DstArrayT* output,
                  int inComp, int outComp) const
  {
    const auto srcRange = svtk::DataArrayTupleRange(input);
    auto dstRange = svtk::DataArrayTupleRange(output);

    for (svtkIdType tupleIdx = 0; tupleIdx < srcRange.size(); ++tupleIdx)
    {
      dstRange[tupleIdx][outComp] = srcRange[tupleIdx][inComp];
    }
  }
};

} // end anon namespace

int svtkMergeFields::MergeArray(svtkDataArray* in, svtkDataArray* out, int inComp, int outComp)
{
  if ((inComp < 0) || (inComp > in->GetNumberOfComponents()) || (outComp < 0) ||
    (outComp > out->GetNumberOfComponents()))
  {
    svtkErrorMacro("Invalid component. Can not merge.");
    return 0;
  }

  // If data types match, use templated, fast method
  using Dispatcher = svtkArrayDispatch::Dispatch2SameValueType;

  MergeFieldsWorker worker;
  if (!Dispatcher::Execute(in, out, worker, inComp, outComp))
  { // otherwise fallback to svtkDataArray API:
    worker(in, out, inComp, outComp);
  }

  return 1;
}

// linked list methods
void svtkMergeFields::AddComponent(Component* op)
{
  op->Next = nullptr;

  if (!this->Head)
  {
    this->Head = op;
    this->Tail = op;
    return;
  }
  this->Tail->Next = op;
  this->Tail = op;
}

Component* svtkMergeFields::FindComponent(int index)
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return nullptr;
  }

  if (cur->Index == index)
  {
    return cur;
  }
  while (cur->Next)
  {
    if (cur->Next->Index == index)
    {
      return cur->Next;
    }
    cur = cur->Next;
  }
  return nullptr;
}

void svtkMergeFields::DeleteAllComponents()
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Component* before;
  do
  {
    before = cur;
    cur = cur->Next;
    delete before;
  } while (cur);
  this->Head = nullptr;
  this->Tail = nullptr;
}

void svtkMergeFields::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field name: ";
  if (this->FieldName)
  {
    os << this->FieldName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}

void svtkMergeFields::PrintComponent(Component* op, ostream& os, svtkIndent indent)
{
  os << indent << "Field name: " << op->FieldName << endl;
  os << indent << "Component index: " << op->Index << endl;
  os << indent << "Source component index: " << op->SourceIndex << endl;
}

void svtkMergeFields::PrintAllComponents(ostream& os, svtkIndent indent)
{
  Component* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Component* before;
  do
  {
    before = cur;
    cur = cur->Next;
    os << endl;
    this->PrintComponent(before, os, indent);
  } while (cur);
}
