/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplitField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplitField.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include <cctype>

svtkStandardNewMacro(svtkSplitField);

char svtkSplitField::FieldLocationNames[3][12] = { "DATA_OBJECT", "POINT_DATA", "CELL_DATA" };

char svtkSplitField::AttributeNames[svtkDataSetAttributes::NUM_ATTRIBUTES][10] = { { 0 } };

typedef svtkSplitField::Component Component;

svtkSplitField::svtkSplitField()
{
  this->FieldName = nullptr;
  this->FieldLocation = -1;
  this->AttributeType = -1;
  this->FieldType = -1;

  this->Head = nullptr;
  this->Tail = nullptr;

  // convert the attribute names to uppercase for local use
  if (svtkSplitField::AttributeNames[0][0] == 0)
  {
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
      int l = static_cast<int>(strlen(svtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
      {
        svtkSplitField::AttributeNames[i][c] =
          toupper(svtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
      }
    }
  }
}

svtkSplitField::~svtkSplitField()
{
  delete[] this->FieldName;
  this->FieldName = nullptr;
  this->DeleteAllComponents();
}

void svtkSplitField::SetInputField(const char* name, int fieldLoc)
{
  if (!name)
  {
    return;
  }

  if ((fieldLoc != svtkSplitField::DATA_OBJECT) && (fieldLoc != svtkSplitField::POINT_DATA) &&
    (fieldLoc != svtkSplitField::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = svtkSplitField::NAME;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name) + 1];
  strcpy(this->FieldName, name);
}

void svtkSplitField::SetInputField(int attributeType, int fieldLoc)
{
  if ((fieldLoc != svtkSplitField::POINT_DATA) && (fieldLoc != svtkSplitField::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = svtkSplitField::ATTRIBUTE;
  this->AttributeType = attributeType;
}

void svtkSplitField::SetInputField(const char* name, const char* fieldLoc)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  int numAttr = svtkDataSetAttributes::NUM_ATTRIBUTES;
  int numFieldLocs = 3;
  int i;

  // Convert strings to ints and call the appropriate SetInputField()
  int attrType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(name, AttributeNames[i]))
    {
      attrType = i;
      break;
    }
  }

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

  if (attrType == -1)
  {
    this->SetInputField(name, loc);
  }
  else
  {
    this->SetInputField(attrType, loc);
  }
}

void svtkSplitField::Split(int component, const char* arrayName)
{
  if (!arrayName)
  {
    return;
  }

  this->Modified();
  Component* comp = this->FindComponent(component);
  // If component is already there, just reset the information
  if (comp)
  {
    comp->SetName(arrayName);
  }
  // otherwise add a new one
  else
  {
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    this->AddComponent(comp);
  }
}

int svtkSplitField::RequestData(svtkInformation* svtkNotUsed(request),
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

  svtkDataArray* outputArray;
  svtkDataArray* inputArray = nullptr;
  svtkFieldData* fd = nullptr;
  svtkFieldData* outputFD = nullptr;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur)
  {
    return 1;
  }

  // find the input and output field data
  if (this->FieldLocation == svtkSplitField::DATA_OBJECT)
  {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
    if (!fd || !outputFD)
    {
      svtkErrorMacro("No field data in svtkDataObject.");
      return 1;
    }
  }
  else if (this->FieldLocation == svtkSplitField::POINT_DATA)
  {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
  }
  else if (this->FieldLocation == svtkSplitField::CELL_DATA)
  {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
  }

  if (this->FieldType == svtkSplitField::NAME)
  {
    inputArray = fd->GetArray(this->FieldName);
  }
  else if (this->FieldType == svtkSplitField::ATTRIBUTE)
  {
    // If we are working with attributes, we also need to have
    // access to svtkDataSetAttributes methods.
    svtkDataSetAttributes* dsa = svtkDataSetAttributes::SafeDownCast(fd);
    if (!dsa)
    {
      svtkErrorMacro("Sanity check failed, returning.");
      return 1;
    }
    inputArray = dsa->GetAttribute(this->AttributeType);
  }

  if (!inputArray)
  {
    svtkErrorMacro("Sanity check failed, returning.");
    return 1;
  }

  // iterate over all components in the linked list and
  // generate them
  do
  {
    before = cur;
    cur = cur->Next;
    if (before->FieldName)
    {
      outputArray = this->SplitArray(inputArray, before->Index);
      if (outputArray)
      {
        outputArray->SetName(before->FieldName);
        outputFD->AddArray(outputArray);
        outputArray->UnRegister(this);
      }
    }
  } while (cur);

  return 1;
}

namespace
{

struct ExtractComponentWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* input, svtkDataArray* outputDA, svtk::ComponentIdType comp)
  {
    // We know these are the same array type:
    ArrayT* output = svtkArrayDownCast<ArrayT>(outputDA);
    assert(output);

    const auto inTuples = svtk::DataArrayTupleRange(input);
    auto outComps = svtk::DataArrayValueRange<1>(output);

    using TupleCRefT = typename decltype(inTuples)::ConstTupleReferenceType;
    using CompT = typename decltype(outComps)::ValueType;

    std::transform(inTuples.cbegin(), inTuples.cend(), outComps.begin(),
      [&](const TupleCRefT tuple) -> CompT { return tuple[comp]; });
  }
};

} // end anon namespace

svtkDataArray* svtkSplitField::SplitArray(svtkDataArray* da, int component)
{
  if ((component < 0) || (component > da->GetNumberOfComponents()))
  {
    svtkErrorMacro("Invalid component. Can not split");
    return nullptr;
  }

  svtkDataArray* output = da->NewInstance();
  output->SetNumberOfComponents(1);
  svtkIdType numTuples = da->GetNumberOfTuples();
  output->SetNumberOfTuples(numTuples);

  using Dispatcher = svtkArrayDispatch::Dispatch;
  ExtractComponentWorker worker;
  if (!Dispatcher::Execute(da, worker, output, component))
  { // fallback:
    worker(da, output, component);
  }

  return output;
}

// Linked list methods
void svtkSplitField::AddComponent(Component* op)
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

Component* svtkSplitField::FindComponent(int index)
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

void svtkSplitField::DeleteAllComponents()
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

void svtkSplitField::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}

void svtkSplitField::PrintComponent(Component* op, ostream& os, svtkIndent indent)
{
  os << indent << "Field name: " << op->FieldName << endl;
  os << indent << "Component index: " << op->Index << endl;
}

void svtkSplitField::PrintAllComponents(ostream& os, svtkIndent indent)
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
