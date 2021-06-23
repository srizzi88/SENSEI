/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRearrangeFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRearrangeFields.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include <cctype>

svtkStandardNewMacro(svtkRearrangeFields);

typedef svtkRearrangeFields::Operation Operation;

// Used by AddOperation() and RemoveOperation() designed to be used
// from other language bindings.
char svtkRearrangeFields::OperationTypeNames[2][5] = { "COPY", "MOVE" };
char svtkRearrangeFields::FieldLocationNames[3][12] = { "DATA_OBJECT", "POINT_DATA", "CELL_DATA" };
char svtkRearrangeFields::AttributeNames[svtkDataSetAttributes::NUM_ATTRIBUTES][10] = { { 0 } };

//--------------------------------------------------------------------------

svtkRearrangeFields::svtkRearrangeFields()
{
  this->Head = nullptr;
  this->Tail = nullptr;
  this->LastId = 0;
  // convert the attribute names to uppercase for local use
  if (svtkRearrangeFields::AttributeNames[0][0] == 0)
  {
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
      int l = static_cast<int>(strlen(svtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
      {
        svtkRearrangeFields::AttributeNames[i][c] =
          toupper(svtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
      }
    }
  }
}

svtkRearrangeFields::~svtkRearrangeFields()
{
  this->DeleteAllOperations();
}

int svtkRearrangeFields::RequestData(svtkInformation* svtkNotUsed(request),
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

  // Apply all operations.
  Operation* cur = this->GetFirst();
  if (cur)
  {
    Operation* before;
    do
    {
      before = cur;
      cur = cur->Next;
      this->ApplyOperation(before, input, output);
    } while (cur);
  }

  // Pass all.
  if (output->GetFieldData() && input->GetFieldData())
  {
    output->GetFieldData()->PassData(input->GetFieldData());
  }
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

// Given location (DATA_OBJECT, CELL_DATA, POINT_DATA) return the
// pointer to the corresponding field data.
svtkFieldData* svtkRearrangeFields::GetFieldDataFromLocation(svtkDataSet* ds, int fieldLoc)
{
  svtkFieldData* fd = nullptr;

  switch (fieldLoc)
  {
    case svtkRearrangeFields::DATA_OBJECT:
      fd = ds->GetFieldData();
      break;
    case svtkRearrangeFields::POINT_DATA:
      fd = ds->GetPointData();
      break;
    case svtkRearrangeFields::CELL_DATA:
      fd = ds->GetCellData();
      break;
  }
  return fd;
}

void svtkRearrangeFields::ApplyOperation(Operation* op, svtkDataSet* input, svtkDataSet* output)
{
  svtkDebugMacro("Applying operation: " << op->Id);

  // Get the field data corresponding to the operation
  // from input and output
  svtkFieldData* inputFD = this->GetFieldDataFromLocation(input, op->FromFieldLoc);
  svtkFieldData* outputFD = this->GetFieldDataFromLocation(output, op->ToFieldLoc);
  if (!inputFD || !outputFD)
  {
    svtkWarningMacro("Can not apply operation " << op->Id
                                               << ": Inappropriate input or output location"
                                               << " specified for the operation.");
    return;
  }

  // If the source is specified by name
  if (op->FieldType == svtkRearrangeFields::NAME)
  {
    svtkDebugMacro("Copy by name:" << op->FieldName);
    // Pass the array
    outputFD->AddArray(inputFD->GetArray(op->FieldName));
    // If moving the array, make sure that it is not copied
    // with PassData()
    if (op->OperationType == svtkRearrangeFields::MOVE)
    {
      svtkFieldData* fd = this->GetFieldDataFromLocation(output, op->FromFieldLoc);
      fd->CopyFieldOff(op->FieldName);
    }
    else if (op->OperationType == svtkRearrangeFields::COPY)
    {
    }
    else
    {
      svtkWarningMacro("Can not apply operation " << op->Id << ": Inappropriate operation type.");
      return;
    }
  }
  // If source is specified as attribute
  else if (op->FieldType == svtkRearrangeFields::ATTRIBUTE)
  {
    svtkDebugMacro("Copy by attribute");
    // Get the attribute and pass it
    svtkDataSetAttributes* dsa = svtkDataSetAttributes::SafeDownCast(inputFD);
    if (!dsa)
    {
      svtkWarningMacro(
        "Can not apply operation " << op->Id << ": Input has to be svtkDataSetAttributes.");
      return;
    }
    outputFD->AddArray(dsa->GetAbstractAttribute(op->AttributeType));
    // If moving the array, make sure that it is not copied
    // with PassData()
    if (op->OperationType == svtkRearrangeFields::MOVE)
    {
      svtkFieldData* fd = this->GetFieldDataFromLocation(output, op->FromFieldLoc);
      svtkDataSetAttributes* dsa2 = svtkDataSetAttributes::SafeDownCast(fd);
      if (dsa2)
      {
        dsa2->SetCopyAttribute(op->AttributeType, 0);
      }
    }
    else if (op->OperationType == svtkRearrangeFields::COPY)
    {
    }
    else
    {
      svtkWarningMacro("Can not apply operation " << op->Id << ": Inappropriate operation type.");
      return;
    }
  }
  else
  {
    svtkWarningMacro("Can not apply operation " << op->Id << ": Inappropriate field type"
                                               << " specified for the operation.");
    return;
  }
}

// Helper method used by the bindings. Allows the caller to
// specify arguments as strings instead of enums.Returns an operation id
// which can later be used to remove the operation.
int svtkRearrangeFields::AddOperation(
  const char* operationType, const char* name, const char* fromFieldLoc, const char* toFieldLoc)
{
  int numAttr = svtkDataSetAttributes::NUM_ATTRIBUTES;
  int numOpTypes = 2;
  int numFieldLocs = 3;
  int opType = -1, i;
  // Convert strings to ints and call the appropriate AddOperation()
  for (i = 0; i < numOpTypes; i++)
  {
    if (!strcmp(operationType, OperationTypeNames[i]))
    {
      opType = i;
      break;
    }
  }
  if (opType == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return -1;
  }
  int attributeType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(name, AttributeNames[i]))
    {
      attributeType = i;
      break;
    }
  }

  int fromLoc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(fromFieldLoc, FieldLocationNames[i]))
    {
      fromLoc = i;
      break;
    }
  }
  if (fromLoc == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return -1;
  }

  int toLoc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(toFieldLoc, FieldLocationNames[i]))
    {
      toLoc = i;
      break;
    }
  }
  if (toLoc == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return -1;
  }

  if (attributeType == -1)
  {
    svtkDebugMacro("Adding operation with parameters: " << opType << " " << name << " " << fromLoc
                                                       << " " << toLoc);
    return this->AddOperation(opType, name, fromLoc, toLoc);
  }
  else
  {
    svtkDebugMacro("Adding operation with parameters: " << opType << " " << attributeType << " "
                                                       << fromLoc << " " << toLoc);
    return this->AddOperation(opType, attributeType, fromLoc, toLoc);
  }
}

int svtkRearrangeFields::AddOperation(
  int operationType, const char* name, int fromFieldLoc, int toFieldLoc)
{
  if (!name)
  {
    return -1;
  }

  // Syntax and sanity checks.
  if ((operationType != svtkRearrangeFields::COPY) && (operationType != svtkRearrangeFields::MOVE))
  {
    svtkErrorMacro("Wrong operation type.");
    return -1;
  }
  if ((fromFieldLoc != svtkRearrangeFields::DATA_OBJECT) &&
    (fromFieldLoc != svtkRearrangeFields::POINT_DATA) &&
    (fromFieldLoc != svtkRearrangeFields::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return -1;
  }
  if ((toFieldLoc != svtkRearrangeFields::DATA_OBJECT) &&
    (toFieldLoc != svtkRearrangeFields::POINT_DATA) && (toFieldLoc != svtkRearrangeFields::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return -1;
  }

  // Create an operation with the specified parameters.
  Operation* op = new Operation;
  op->OperationType = operationType;
  op->FieldName = new char[strlen(name) + 1];
  strcpy(op->FieldName, name);
  op->FromFieldLoc = fromFieldLoc;
  op->ToFieldLoc = toFieldLoc;
  op->FieldType = svtkRearrangeFields::NAME;
  op->Id = this->LastId++;
  // assign this anyway
  op->AttributeType = 0;

  this->AddOperation(op);
  this->Modified();

  return op->Id;
}

int svtkRearrangeFields::AddOperation(
  int operationType, int attributeType, int fromFieldLoc, int toFieldLoc)
{

  // Syntax and sanity checks.
  if ((operationType != svtkRearrangeFields::COPY) && (operationType != svtkRearrangeFields::MOVE))
  {
    svtkErrorMacro("Wrong operation type.");
    return -1;
  }
  if ((fromFieldLoc != svtkRearrangeFields::DATA_OBJECT) &&
    (fromFieldLoc != svtkRearrangeFields::POINT_DATA) &&
    (fromFieldLoc != svtkRearrangeFields::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return -1;
  }
  if ((attributeType < 0) || (attributeType > svtkDataSetAttributes::NUM_ATTRIBUTES))
  {
    svtkErrorMacro("Wrong attribute type.");
    return -1;
  }
  if ((toFieldLoc != svtkRearrangeFields::DATA_OBJECT) &&
    (toFieldLoc != svtkRearrangeFields::POINT_DATA) && (toFieldLoc != svtkRearrangeFields::CELL_DATA))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return -1;
  }

  // Create an operation with the specified parameters.
  Operation* op = new Operation;
  op->OperationType = operationType;
  op->AttributeType = attributeType;
  op->FromFieldLoc = fromFieldLoc;
  op->ToFieldLoc = toFieldLoc;
  op->FieldType = svtkRearrangeFields::ATTRIBUTE;
  op->Id = this->LastId++;

  this->AddOperation(op);
  this->Modified();

  return op->Id;
}

// Helper method used by the bindings. Allows the caller to
// specify arguments as strings instead of enums.Returns an operation id
// which can later be used to remove the operation.
int svtkRearrangeFields::RemoveOperation(
  const char* operationType, const char* name, const char* fromFieldLoc, const char* toFieldLoc)
{
  if (!operationType || !name || !fromFieldLoc || !toFieldLoc)
  {
    return 0;
  }

  int numAttr = svtkDataSetAttributes::NUM_ATTRIBUTES;
  int numOpTypes = 2;
  int numFieldLocs = 3;
  int opType = -1, i;
  // Convert strings to ints and call the appropriate AddOperation()
  for (i = 0; i < numOpTypes; i++)
  {
    if (!strcmp(operationType, OperationTypeNames[i]))
    {
      opType = i;
    }
  }
  if (opType == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return 0;
  }
  int attributeType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(name, AttributeNames[i]))
    {
      attributeType = i;
    }
  }

  int fromLoc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(fromFieldLoc, FieldLocationNames[i]))
    {
      fromLoc = i;
    }
  }
  if (fromLoc == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return 0;
  }

  int toLoc = -1;
  for (i = 0; i < numFieldLocs; i++)
  {
    if (!strcmp(toFieldLoc, FieldLocationNames[i]))
    {
      toLoc = i;
    }
  }
  if (toLoc == -1)
  {
    svtkErrorMacro("Syntax error in operation.");
    return 0;
  }

  if (attributeType == -1)
  {
    svtkDebugMacro("Removing operation with parameters: " << opType << " " << name << " " << fromLoc
                                                         << " " << toLoc);
    return this->RemoveOperation(opType, name, fromLoc, toLoc);
  }
  else
  {
    svtkDebugMacro("Removing operation with parameters: " << opType << " " << attributeType << " "
                                                         << fromLoc << " " << toLoc);
    return this->RemoveOperation(opType, attributeType, fromLoc, toLoc);
  }
}

int svtkRearrangeFields::RemoveOperation(int operationId)
{
  Operation* before;
  Operation* op;

  op = this->FindOperation(operationId, before);
  if (!op)
  {
    return 0;
  }
  this->DeleteOperation(op, before);
  return 1;
}

int svtkRearrangeFields::RemoveOperation(
  int operationType, const char* name, int fromFieldLoc, int toFieldLoc)
{
  Operation* before;
  Operation* op;
  op = this->FindOperation(operationType, name, fromFieldLoc, toFieldLoc, before);
  if (!op)
  {
    return 0;
  }
  this->Modified();
  this->DeleteOperation(op, before);
  return 1;
}

int svtkRearrangeFields::RemoveOperation(
  int operationType, int attributeType, int fromFieldLoc, int toFieldLoc)
{
  Operation* before;
  Operation* op;
  op = this->FindOperation(operationType, attributeType, fromFieldLoc, toFieldLoc, before);
  if (!op)
  {
    return 0;
  }
  this->Modified();
  this->DeleteOperation(op, before);
  return 1;
}

void svtkRearrangeFields::AddOperation(Operation* op)
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

void svtkRearrangeFields::DeleteOperation(Operation* op, Operation* before)
{
  if (!op)
  {
    return;
  }
  if (!before)
  {
    this->Head = op->Next;
  }
  else
  {
    before->Next = op->Next;
    if (!before->Next)
    {
      this->Tail = before;
    }
  }
  delete op;
}

Operation* svtkRearrangeFields::FindOperation(int id, Operation*& before)
{
  Operation* cur = this->GetFirst();
  if (!cur)
  {
    return nullptr;
  }

  before = nullptr;
  if (cur->Id == id)
  {
    return cur;
  }
  while (cur->Next)
  {
    before = cur;
    if (cur->Next->Id == id)
    {
      return cur->Next;
    }
    cur = cur->Next;
  }
  return nullptr;
}

Operation* svtkRearrangeFields::FindOperation(
  int operationType, const char* name, int fromFieldLoc, int toFieldLoc, Operation*& before)
{
  if (!name)
  {
    return nullptr;
  }

  Operation op;
  op.OperationType = operationType;
  op.FieldName = new char[strlen(name) + 1];
  strcpy(op.FieldName, name);
  op.FromFieldLoc = fromFieldLoc;
  op.ToFieldLoc = toFieldLoc;

  Operation* cur = this->GetFirst();
  before = nullptr;
  if ((cur->FieldType == svtkRearrangeFields::NAME) && this->CompareOperationsByName(cur, &op))
  {
    return cur;
  }
  while (cur->Next)
  {
    before = cur;
    if ((cur->Next->FieldType == svtkRearrangeFields::NAME) &&
      this->CompareOperationsByName(cur->Next, &op))
    {
      return cur->Next;
    }
    cur = cur->Next;
  }
  return nullptr;
}

Operation* svtkRearrangeFields::FindOperation(
  int operationType, int attributeType, int fromFieldLoc, int toFieldLoc, Operation*& before)
{
  Operation op;
  op.OperationType = operationType;
  op.AttributeType = attributeType;
  op.FromFieldLoc = fromFieldLoc;
  op.ToFieldLoc = toFieldLoc;

  Operation* cur = this->GetFirst();
  before = nullptr;
  if ((cur->FieldType == svtkRearrangeFields::ATTRIBUTE) && this->CompareOperationsByType(cur, &op))
  {
    return cur;
  }
  while (cur->Next)
  {
    before = cur;
    if ((cur->Next->FieldType == svtkRearrangeFields::ATTRIBUTE) &&
      this->CompareOperationsByType(cur->Next, &op))
    {
      return cur->Next;
    }
    cur = cur->Next;
  }
  return nullptr;
}

void svtkRearrangeFields::DeleteAllOperations()
{
  Operation* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Operation* before;
  do
  {
    before = cur;
    cur = cur->Next;
    delete before;
  } while (cur);
  this->Head = nullptr;
  this->Tail = nullptr;
}

int svtkRearrangeFields::CompareOperationsByName(const Operation* op1, const Operation* op2)
{
  if (op1->OperationType != op2->OperationType)
  {
    return 0;
  }
  if (!op1->FieldName || !op2->FieldName || strcmp(op1->FieldName, op2->FieldName))
  {
    return 0;
  }
  if (op1->FromFieldLoc != op2->FromFieldLoc)
  {
    return 0;
  }
  if (op1->ToFieldLoc != op2->ToFieldLoc)
  {
    return 0;
  }
  return 1;
}

int svtkRearrangeFields::CompareOperationsByType(const Operation* op1, const Operation* op2)
{
  if (op1->OperationType != op2->OperationType)
  {
    return 0;
  }
  if (op1->AttributeType != op2->AttributeType)
  {
    return 0;
  }
  if (op1->FromFieldLoc != op2->FromFieldLoc)
  {
    return 0;
  }
  if (op1->ToFieldLoc != op2->ToFieldLoc)
  {
    return 0;
  }
  return 1;
}

void svtkRearrangeFields::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Last id: " << this->LastId << endl;
  os << indent << "Operations: " << endl;
  this->PrintAllOperations(os, indent.GetNextIndent());
}

void svtkRearrangeFields::PrintAllOperations(ostream& os, svtkIndent indent)
{
  Operation* cur = this->GetFirst();
  if (!cur)
  {
    return;
  }
  Operation* before;
  do
  {
    before = cur;
    cur = cur->Next;
    os << endl;
    this->PrintOperation(before, os, indent);
  } while (cur);
}

void svtkRearrangeFields::PrintOperation(Operation* op, ostream& os, svtkIndent indent)
{
  os << indent << "Id: " << op->Id << endl;
  os << indent << "Type: " << op->OperationType << endl;
  os << indent << "Field type: " << op->FieldType << endl;
  if (op->FieldName)
  {
    os << indent << "Field name: " << op->FieldName << endl;
  }
  else
  {
    os << indent << "Field name: (none)" << endl;
  }
  os << indent << "Attribute type: " << op->AttributeType << endl;
  os << indent << "Source field location: " << op->FromFieldLoc << endl;
  os << indent << "Target field location: " << op->ToFieldLoc << endl;
  os << indent << "Next operation: " << op->Next << endl;
  os << endl;
}
