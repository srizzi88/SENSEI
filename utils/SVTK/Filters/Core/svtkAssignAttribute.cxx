/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAssignAttribute.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include <cctype>

#include <cassert>

static int svtkGetArrayIndex(svtkDataSetAttributes* dsa, svtkAbstractArray* array)
{
  for (int cc = 0; cc < dsa->GetNumberOfArrays(); cc++)
  {
    if (dsa->GetAbstractArray(cc) == array)
    {
      return cc;
    }
  }
  return -1;
}

svtkStandardNewMacro(svtkAssignAttribute);

char svtkAssignAttribute::AttributeLocationNames[svtkAssignAttribute::NUM_ATTRIBUTE_LOCS][12] = {
  "POINT_DATA", "CELL_DATA", "VERTEX_DATA", "EDGE_DATA"
};

char svtkAssignAttribute::AttributeNames[svtkDataSetAttributes::NUM_ATTRIBUTES][20] = { { 0 } };

svtkAssignAttribute::svtkAssignAttribute()
{
  this->FieldName = nullptr;
  this->AttributeLocationAssignment = -1;
  this->AttributeType = -1;
  this->InputAttributeType = -1;
  this->FieldTypeAssignment = -1;

  // convert the attribute names to uppercase for local use
  if (svtkAssignAttribute::AttributeNames[0][0] == 0)
  {
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
      int l = static_cast<int>(strlen(svtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 19; c++)
      {
        svtkAssignAttribute::AttributeNames[i][c] =
          toupper(svtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
      }
    }
  }
}

svtkAssignAttribute::~svtkAssignAttribute()
{
  delete[] this->FieldName;
  this->FieldName = nullptr;
}

void svtkAssignAttribute::Assign(const char* fieldName, int attributeType, int attributeLoc)
{
  if (!fieldName)
  {
    return;
  }

  if ((attributeType < 0) || (attributeType > svtkDataSetAttributes::NUM_ATTRIBUTES))
  {
    svtkErrorMacro("Wrong attribute type.");
    return;
  }

  if ((attributeLoc < 0) || (attributeLoc > svtkAssignAttribute::NUM_ATTRIBUTE_LOCS))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  delete[] this->FieldName;
  this->FieldName = new char[strlen(fieldName) + 1];
  strcpy(this->FieldName, fieldName);

  this->AttributeType = attributeType;
  this->AttributeLocationAssignment = attributeLoc;
  this->FieldTypeAssignment = svtkAssignAttribute::NAME;
}

void svtkAssignAttribute::Assign(int inputAttributeType, int attributeType, int attributeLoc)
{
  if ((attributeType < 0) || (attributeType > svtkDataSetAttributes::NUM_ATTRIBUTES) ||
    (inputAttributeType < 0) || (inputAttributeType > svtkDataSetAttributes::NUM_ATTRIBUTES))
  {
    svtkErrorMacro("Wrong attribute type.");
    return;
  }

  if ((attributeLoc < 0) || (attributeLoc > svtkAssignAttribute::NUM_ATTRIBUTE_LOCS))
  {
    svtkErrorMacro("The source for the field is wrong.");
    return;
  }

  this->Modified();
  this->AttributeType = attributeType;
  this->InputAttributeType = inputAttributeType;
  this->AttributeLocationAssignment = attributeLoc;
  this->FieldTypeAssignment = svtkAssignAttribute::ATTRIBUTE;
}

void svtkAssignAttribute::Assign(
  const char* name, const char* attributeType, const char* attributeLoc)
{
  if (!name || !attributeType || !attributeLoc)
  {
    return;
  }

  int numAttr = svtkDataSetAttributes::NUM_ATTRIBUTES;
  int numAttributeLocs = svtkAssignAttribute::NUM_ATTRIBUTE_LOCS;
  int i;

  // Convert strings to ints and call the appropriate Assign()
  int inputAttributeType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(name, AttributeNames[i]))
    {
      inputAttributeType = i;
      break;
    }
  }

  int attrType = -1;
  for (i = 0; i < numAttr; i++)
  {
    if (!strcmp(attributeType, AttributeNames[i]))
    {
      attrType = i;
      break;
    }
  }
  if (attrType == -1)
  {
    svtkErrorMacro("Target attribute type is invalid.");
    return;
  }

  int loc = -1;
  for (i = 0; i < numAttributeLocs; i++)
  {
    if (!strcmp(attributeLoc, AttributeLocationNames[i]))
    {
      loc = i;
      break;
    }
  }
  if (loc == -1)
  {
    svtkErrorMacro("Target location for the attribute is invalid.");
    return;
  }

  if (inputAttributeType == -1)
  {
    this->Assign(name, attrType, loc);
  }
  else
  {
    this->Assign(inputAttributeType, attrType, loc);
  }
}

int svtkAssignAttribute::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if ((this->AttributeType != -1) && (this->AttributeLocationAssignment != -1) &&
    (this->FieldTypeAssignment != -1))
  {
    int fieldAssociation = svtkDataObject::FIELD_ASSOCIATION_POINTS;
    switch (this->AttributeLocationAssignment)
    {
      case POINT_DATA:
        fieldAssociation = svtkDataObject::FIELD_ASSOCIATION_POINTS;
        break;
      case CELL_DATA:
        fieldAssociation = svtkDataObject::FIELD_ASSOCIATION_CELLS;
        break;
      case VERTEX_DATA:
        fieldAssociation = svtkDataObject::FIELD_ASSOCIATION_VERTICES;
        break;
      default:
        fieldAssociation = svtkDataObject::FIELD_ASSOCIATION_EDGES;
        break;
    }
    if (this->FieldTypeAssignment == svtkAssignAttribute::NAME && this->FieldName)
    {
      svtkDataObject::SetActiveAttribute(
        outInfo, fieldAssociation, this->FieldName, this->AttributeType);
      svtkInformation* inputAttributeInfo =
        svtkDataObject::GetNamedFieldInformation(inInfo, fieldAssociation, this->FieldName);
      if (inputAttributeInfo)
      {
        const int type = inputAttributeInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
        const int numComponents =
          inputAttributeInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        const int numTuples = inputAttributeInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES());

        svtkDataObject::SetActiveAttributeInfo(outInfo, fieldAssociation, this->AttributeType,
          this->FieldName, type, numComponents, numTuples);
      }
    }
    else if (this->FieldTypeAssignment == svtkAssignAttribute::ATTRIBUTE &&
      this->InputAttributeType != -1)
    {
      svtkInformation* inputAttributeInfo = svtkDataObject::GetActiveFieldInformation(
        inInfo, fieldAssociation, this->InputAttributeType);
      if (inputAttributeInfo) // do we have an active field of requested type
      {
        const char* name = inputAttributeInfo->Get(svtkDataObject::FIELD_NAME());
        const int type = inputAttributeInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
        const int numComponents =
          inputAttributeInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        const int numTuples = inputAttributeInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES());

        svtkDataObject::SetActiveAttribute(outInfo, fieldAssociation, name, this->AttributeType);
        svtkDataObject::SetActiveAttributeInfo(
          outInfo, fieldAssociation, this->AttributeType, name, type, numComponents, numTuples);
      }
    }
  }

  return 1;
}

int svtkAssignAttribute::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkDataSetAttributes* ods = nullptr;
  if (svtkDataSet::SafeDownCast(input))
  {
    svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
    svtkDataSet* dsOutput = svtkDataSet::SafeDownCast(output);
    // This has to be here because it initialized all field datas.
    dsOutput->CopyStructure(dsInput);

    if (dsOutput->GetFieldData() && dsInput->GetFieldData())
    {
      dsOutput->GetFieldData()->PassData(dsInput->GetFieldData());
    }
    dsOutput->GetPointData()->PassData(dsInput->GetPointData());
    dsOutput->GetCellData()->PassData(dsInput->GetCellData());
    switch (this->AttributeLocationAssignment)
    {
      case svtkAssignAttribute::POINT_DATA:
        ods = dsOutput->GetPointData();
        break;
      case svtkAssignAttribute::CELL_DATA:
        ods = dsOutput->GetCellData();
        break;
      default:
        svtkErrorMacro(<< "Data must be point or cell for svtkDataSet");
        return 0;
    }
  }
  else
  {
    svtkGraph* graphInput = svtkGraph::SafeDownCast(input);
    svtkGraph* graphOutput = svtkGraph::SafeDownCast(output);
    graphOutput->ShallowCopy(graphInput);
    switch (this->AttributeLocationAssignment)
    {
      case svtkAssignAttribute::VERTEX_DATA:
        ods = graphOutput->GetVertexData();
        break;
      case svtkAssignAttribute::EDGE_DATA:
        ods = graphOutput->GetEdgeData();
        break;
      default:
        svtkErrorMacro(<< "Data must be vertex or edge for svtkGraph");
        return 0;
    }
  }

  if ((this->AttributeType != -1) && (this->AttributeLocationAssignment != -1) &&
    (this->FieldTypeAssignment != -1))
  {
    // Get the appropriate output DataSetAttributes
    if (this->FieldTypeAssignment == svtkAssignAttribute::NAME && this->FieldName)
    {
      ods->SetActiveAttribute(this->FieldName, this->AttributeType);
    }
    else if (this->FieldTypeAssignment == svtkAssignAttribute::ATTRIBUTE &&
      (this->InputAttributeType != -1))
    {
      // If labeling an attribute as another attribute, we
      // need to get it's index and call SetActiveAttribute()
      // with that index
      // int attributeIndices[svtkDataSetAttributes::NUM_ATTRIBUTES];
      // ods->GetAttributeIndices(attributeIndices);
      // if (attributeIndices[this->InputAttributeType] != -1)
      svtkAbstractArray* oaa = ods->GetAbstractAttribute(this->InputAttributeType);
      if (oaa)
      {
        // Use array index to mark this array active and not its name since SVTK
        // arrays not necessarily have names.
        int arrIndex = svtkGetArrayIndex(ods, oaa);
        assert(arrIndex >= 0); // arrIndex cannot be -1. Doesn't make sense.
        ods->SetActiveAttribute(arrIndex, this->AttributeType);
      }
    }
  }

  return 1;
}

int svtkAssignAttribute::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // This algorithm may accept a svtkPointSet or svtkGraph.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkAssignAttribute::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "Field type: " << this->FieldTypeAssignment << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Input attribute type: " << this->InputAttributeType << endl;
  os << indent << "Attribute location: " << this->AttributeLocationAssignment << endl;
}
