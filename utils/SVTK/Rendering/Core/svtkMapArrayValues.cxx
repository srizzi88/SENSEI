/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapArrayValues.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMapArrayValues.h"

#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraph.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariant.h"
#include <cctype>

#include <map>
#include <utility>

svtkStandardNewMacro(svtkMapArrayValues);

typedef std::map<svtkVariant, svtkVariant, svtkVariantLessThan> MapBase;
class svtkMapType : public MapBase
{
};

svtkMapArrayValues::svtkMapArrayValues()
{
  this->InputArrayName = nullptr;
  this->OutputArrayName = nullptr;
  this->SetOutputArrayName("ArrayMap");
  this->FieldType = svtkMapArrayValues::POINT_DATA;
  this->OutputArrayType = SVTK_INT;
  this->PassArray = 0;
  this->FillValue = -1;

  this->Map = new svtkMapType;
}

svtkMapArrayValues::~svtkMapArrayValues()
{
  this->SetInputArrayName(nullptr);
  this->SetOutputArrayName(nullptr);
  delete this->Map;
}

void svtkMapArrayValues::AddToMap(const char* from, int to)
{
  svtkVariant fromVar(from);
  svtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void svtkMapArrayValues::AddToMap(int from, int to)
{
  svtkVariant fromVar(from);
  svtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void svtkMapArrayValues::AddToMap(int from, const char* to)
{
  svtkVariant fromVar(from);
  svtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void svtkMapArrayValues::AddToMap(const char* from, const char* to)
{
  svtkVariant fromVar(from);
  svtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void svtkMapArrayValues::AddToMap(svtkVariant from, svtkVariant to)
{
  svtkVariant fromVar(from);
  svtkVariant toVar(to);
  this->Map->insert(std::make_pair(fromVar, toVar));

  this->Modified();
}

void svtkMapArrayValues::ClearMap()
{
  this->Map->clear();

  this->Modified();
}

int svtkMapArrayValues::GetMapSize()
{
  return static_cast<int>(this->Map->size());
}

int svtkMapArrayValues::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  if (!this->InputArrayName)
  {
    // svtkErrorMacro(<<"Input array not specified.");
    output->ShallowCopy(input);
    return 1;
  }

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
    switch (this->FieldType)
    {
      case svtkMapArrayValues::POINT_DATA:
        ods = dsOutput->GetPointData();
        break;
      case svtkMapArrayValues::CELL_DATA:
        ods = dsOutput->GetCellData();
        break;
      default:
        svtkErrorMacro(<< "Data must be point or cell for svtkDataSet");
        return 0;
    }
  }
  else if (svtkGraph::SafeDownCast(input))
  {
    svtkGraph* graphInput = svtkGraph::SafeDownCast(input);
    svtkGraph* graphOutput = svtkGraph::SafeDownCast(output);
    graphOutput->ShallowCopy(graphInput);
    switch (this->FieldType)
    {
      case svtkMapArrayValues::VERTEX_DATA:
        ods = graphOutput->GetVertexData();
        break;
      case svtkMapArrayValues::EDGE_DATA:
        ods = graphOutput->GetEdgeData();
        break;
      default:
        svtkErrorMacro(<< "Data must be vertex or edge for svtkGraph");
        return 0;
    }
  }
  else if (svtkTable::SafeDownCast(input))
  {
    svtkTable* tableInput = svtkTable::SafeDownCast(input);
    svtkTable* tableOutput = svtkTable::SafeDownCast(output);
    tableOutput->ShallowCopy(tableInput);
    switch (this->FieldType)
    {
      case svtkMapArrayValues::ROW_DATA:
        ods = tableOutput->GetRowData();
        break;
      default:
        svtkErrorMacro(<< "Data must be row for svtkTable");
        return 0;
    }
  }
  else
  {
    svtkErrorMacro(<< "Invalid input type");
    return 0;
  }

  svtkAbstractArray* inputArray = ods->GetAbstractArray(this->InputArrayName);
  if (!inputArray)
  {
    return 1;
  }
  svtkAbstractArray* outputArray = svtkAbstractArray::CreateArray(this->OutputArrayType);
  svtkDataArray* outputDataArray = svtkArrayDownCast<svtkDataArray>(outputArray);
  svtkStringArray* outputStringArray = svtkArrayDownCast<svtkStringArray>(outputArray);
  outputArray->SetName(this->OutputArrayName);

  // Are we copying the input array values to the output array before
  // the mapping?
  if (this->PassArray)
  {
    // Make sure the DeepCopy will succeed
    if ((inputArray->IsA("svtkDataArray") && outputArray->IsA("svtkDataArray")) ||
      (inputArray->IsA("svtkStringArray") && outputArray->IsA("svtkStringArray")))
    {
      outputArray->DeepCopy(inputArray);
    }
    else
    {
      svtkIdType numComps = inputArray->GetNumberOfComponents();
      svtkIdType numTuples = inputArray->GetNumberOfTuples();
      outputArray->SetNumberOfComponents(numComps);
      outputArray->SetNumberOfTuples(numTuples);
      for (svtkIdType i = 0; i < numTuples; ++i)
      {
        for (svtkIdType j = 0; j < numComps; ++j)
        {
          outputArray->InsertVariantValue(
            i * numComps + j, inputArray->GetVariantValue(i * numComps + j));
        }
      }
    }
  }
  else
  {
    outputArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());
    outputArray->SetNumberOfTuples(inputArray->GetNumberOfTuples());

    // Fill the output array with a default value
    if (outputDataArray)
    {
      outputDataArray->FillComponent(0, this->FillValue);
    }
  }

  // Use the internal map to set the mapped values in the output array
  svtkIdList* results = svtkIdList::New();
  for (MapBase::iterator i = this->Map->begin(); i != this->Map->end(); ++i)
  {
    inputArray->LookupValue(i->first, results);
    for (svtkIdType j = 0; j < results->GetNumberOfIds(); ++j)
    {
      if (outputDataArray)
      {
        outputDataArray->SetComponent(results->GetId(j), 0, i->second.ToDouble());
      }
      else if (outputStringArray)
      {
        outputStringArray->SetValue(results->GetId(j), i->second.ToString());
      }
    }
  }

  // Finally, add the array to the appropriate svtkDataSetAttributes
  ods->AddArray(outputArray);

  results->Delete();
  outputArray->Delete();

  return 1;
}

int svtkMapArrayValues::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // This algorithm may accept a svtkPointSet or svtkGraph.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

void svtkMapArrayValues::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input array name: ";
  if (this->InputArrayName)
  {
    os << this->InputArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Output array name: ";
  if (this->OutputArrayName)
  {
    os << this->OutputArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Output array type: " << this->OutputArrayType << endl;
  os << indent << "PassArray: " << this->PassArray << endl;
  os << indent << "FillValue: " << this->FillValue << endl;
}
