/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenerateIndexArray.cxx

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include "svtkGenerateIndexArray.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkGenerateIndexArray);

svtkGenerateIndexArray::svtkGenerateIndexArray()
  : ArrayName(nullptr)
  , FieldType(ROW_DATA)
  , ReferenceArrayName(nullptr)
  , PedigreeID(false)
{
  this->SetArrayName("index");
}

svtkGenerateIndexArray::~svtkGenerateIndexArray()
{
  this->SetArrayName(nullptr);
  this->SetReferenceArrayName(nullptr);
}

void svtkGenerateIndexArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;
  os << "FieldType: " << this->FieldType << endl;
  os << "ReferenceArrayName: " << (this->ReferenceArrayName ? this->ReferenceArrayName : "(none)")
     << endl;
  os << "PedigreeID: " << this->PedigreeID << endl;
}

svtkTypeBool svtkGenerateIndexArray::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int svtkGenerateIndexArray::RequestDataObject(
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

int svtkGenerateIndexArray::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // An output array name is required ...
  if (!(this->ArrayName && strlen(this->ArrayName)))
  {
    svtkErrorMacro(<< "No array name defined.");
    return 0;
  }

  // Make a shallow-copy of our input ...
  svtkDataObject* const input = svtkDataObject::GetData(inputVector[0]);
  svtkDataObject* const output = svtkDataObject::GetData(outputVector);
  output->ShallowCopy(input);

  // Figure-out where we'll be reading/writing data ...
  svtkDataSetAttributes* output_attributes = nullptr;
  svtkIdType output_count = 0;

  switch (this->FieldType)
  {
    case ROW_DATA:
    {
      svtkTable* const table = svtkTable::SafeDownCast(output);
      output_attributes = table ? table->GetRowData() : nullptr;
      output_count = table ? table->GetNumberOfRows() : 0;
      break;
    }
    case POINT_DATA:
    {
      svtkDataSet* const data_set = svtkDataSet::SafeDownCast(output);
      output_attributes = data_set ? data_set->GetPointData() : nullptr;
      output_count = data_set ? data_set->GetNumberOfPoints() : 0;
      break;
    }
    case CELL_DATA:
    {
      svtkDataSet* const data_set = svtkDataSet::SafeDownCast(output);
      output_attributes = data_set ? data_set->GetCellData() : nullptr;
      output_count = data_set ? data_set->GetNumberOfCells() : 0;
      break;
    }
    case VERTEX_DATA:
    {
      svtkGraph* const graph = svtkGraph::SafeDownCast(output);
      output_attributes = graph ? graph->GetVertexData() : nullptr;
      output_count = graph ? graph->GetNumberOfVertices() : 0;
      break;
    }
    case EDGE_DATA:
    {
      svtkGraph* const graph = svtkGraph::SafeDownCast(output);
      output_attributes = graph ? graph->GetEdgeData() : nullptr;
      output_count = graph ? graph->GetNumberOfEdges() : 0;
      break;
    }
  }

  if (!output_attributes)
  {
    svtkErrorMacro(<< "Invalid field type for this data object.");
    return 0;
  }

  // Create our output array ...
  svtkIdTypeArray* const output_array = svtkIdTypeArray::New();
  output_array->SetName(this->ArrayName);
  output_array->SetNumberOfTuples(output_count);
  output_attributes->AddArray(output_array);
  output_array->Delete();

  if (this->PedigreeID)
    output_attributes->SetPedigreeIds(output_array);

  // Generate indices based on the reference array ...
  if (this->ReferenceArrayName && strlen(this->ReferenceArrayName))
  {
    int reference_array_index = -1;
    svtkAbstractArray* const reference_array =
      output_attributes->GetAbstractArray(this->ReferenceArrayName, reference_array_index);
    if (!reference_array)
    {
      svtkErrorMacro(<< "No reference array " << this->ReferenceArrayName);
      return 0;
    }

    typedef std::map<svtkVariant, svtkIdType, svtkVariantLessThan> index_map_t;
    index_map_t index_map;

    for (svtkIdType i = 0; i != output_count; ++i)
    {
      if (!index_map.count(reference_array->GetVariantValue(i)))
      {
#ifdef _RWSTD_NO_MEMBER_TEMPLATES
        // Deal with Sun Studio old libCstd.
        // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
        index_map.insert(
          std::pair<const svtkVariant, svtkIdType>(reference_array->GetVariantValue(i), 0));
#else
        index_map.insert(std::make_pair(reference_array->GetVariantValue(i), 0));
#endif
      }
    }

    svtkIdType index = 0;
    for (index_map_t::iterator i = index_map.begin(); i != index_map.end(); ++i, ++index)
      i->second = index;

    for (svtkIdType i = 0; i != output_count; ++i)
      output_array->SetValue(i, index_map[reference_array->GetVariantValue(i)]);
  }
  // Otherwise, generate a trivial index array ...
  else
  {
    for (svtkIdType i = 0; i != output_count; ++i)
      output_array->SetValue(i, i);
  }

  return 1;
}
