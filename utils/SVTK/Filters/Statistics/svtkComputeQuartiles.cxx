/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkComputeQuartiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkComputeQuartiles.h"

#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOrderStatistics.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

#include <sstream>
#include <string>

svtkStandardNewMacro(svtkComputeQuartiles);
//-----------------------------------------------------------------------------
svtkComputeQuartiles::svtkComputeQuartiles()
{
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);
  this->FieldAssociation = -1;
}

//-----------------------------------------------------------------------------
svtkComputeQuartiles::~svtkComputeQuartiles() = default;

//-----------------------------------------------------------------------------
void svtkComputeQuartiles::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int svtkComputeQuartiles::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkComputeQuartiles::GetInputFieldAssociation()
{
  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  return inArrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION());
}

//-----------------------------------------------------------------------------
svtkFieldData* svtkComputeQuartiles::GetInputFieldData(svtkDataObject* input)
{
  if (!input)
  {
    svtkErrorMacro(<< "Cannot extract fields from null input");
    return nullptr;
  }

  if (svtkTable::SafeDownCast(input))
  {
    this->FieldAssociation = svtkDataObject::FIELD_ASSOCIATION_ROWS;
  }

  if (this->FieldAssociation < 0)
  {
    this->FieldAssociation = this->GetInputFieldAssociation();
  }

  switch (this->FieldAssociation)
  {
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
    case svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      return svtkDataSet::SafeDownCast(input)->GetPointData();

    case svtkDataObject::FIELD_ASSOCIATION_CELLS:
      return svtkDataSet::SafeDownCast(input)->GetCellData();

    case svtkDataObject::FIELD_ASSOCIATION_NONE:
      return input->GetFieldData();

    case svtkDataObject::FIELD_ASSOCIATION_VERTICES:
      return svtkGraph::SafeDownCast(input)->GetVertexData();

    case svtkDataObject::FIELD_ASSOCIATION_EDGES:
      return svtkGraph::SafeDownCast(input)->GetEdgeData();

    case svtkDataObject::FIELD_ASSOCIATION_ROWS:
      return svtkTable::SafeDownCast(input)->GetRowData();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
int svtkComputeQuartiles::RequestData(svtkInformation* /*request*/,
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkTable* outputTable = svtkTable::GetData(outputVector, 0);

  svtkCompositeDataSet* cdin = svtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    svtkCompositeDataIterator* iter = cdin->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* o = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (o)
      {
        ComputeTable(o, outputTable, iter->GetCurrentFlatIndex());
      }
    }
  }
  else if (svtkDataObject* o = svtkDataObject::SafeDownCast(input))
  {
    ComputeTable(o, outputTable, -1);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkComputeQuartiles::ComputeTable(
  svtkDataObject* input, svtkTable* outputTable, svtkIdType blockId)
{
  svtkFieldData* field = this->GetInputFieldData(input);

  if (!field || field->GetNumberOfArrays() == 0)
  {
    svtkDebugMacro(<< "No field found!");
    return;
  }

  // Fill table for descriptive statistics input.
  svtkNew<svtkTable> inDescStats;
  svtkNew<svtkOrderStatistics> os;
  os->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, inDescStats);

  for (int i = 0; i < field->GetNumberOfArrays(); i++)
  {
    svtkDataArray* dataArray = field->GetArray(i);
    if (!dataArray || dataArray->GetNumberOfComponents() != 1)
    {
      svtkDebugMacro(<< "Field " << i << " empty or not scalar");
      continue;
    }

    // If field doesn't have a name, give a default one
    if (!dataArray->GetName())
    {
      std::ostringstream s;
      s << "Field " << i;
      dataArray->SetName(s.str().c_str());
    }
    inDescStats->AddColumn(dataArray);
    os->AddColumn(dataArray->GetName());
  }

  if (inDescStats->GetNumberOfColumns() == 0)
  {
    return;
  }

  os->SetLearnOption(true);
  os->SetDeriveOption(true);
  os->SetTestOption(false);
  os->SetAssessOption(false);
  os->Update();

  // Get the output table of the descriptive statistics that contains quantiles
  // of the input data series.
  svtkMultiBlockDataSet* outputModelDS = svtkMultiBlockDataSet::SafeDownCast(
    os->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
  unsigned nbq = outputModelDS->GetNumberOfBlocks() - 1;
  svtkTable* outputQuartiles = svtkTable::SafeDownCast(outputModelDS->GetBlock(nbq));
  if (!outputQuartiles || outputQuartiles->GetNumberOfColumns() < 2)
  {
    return;
  }

  svtkIdType currLen = outputTable->GetNumberOfColumns();
  svtkIdType outLen = outputQuartiles->GetNumberOfColumns() - 1;

  // Fill the table
  for (int j = 0; j < outLen; j++)
  {
    svtkNew<svtkDoubleArray> ncol;
    ncol->SetNumberOfComponents(1);
    ncol->SetNumberOfValues(5);
    outputTable->AddColumn(ncol);
    if (blockId >= 0)
    {
      std::stringstream ss;
      ss << inDescStats->GetColumnName(j) << "_Block_" << blockId;
      ncol->SetName(ss.str().c_str());
    }
    else
    {
      ncol->SetName(inDescStats->GetColumnName(j));
    }

    svtkAbstractArray* col = outputQuartiles->GetColumnByName(inDescStats->GetColumnName(j));
    for (int k = 0; k < 5; k++)
    {
      outputTable->SetValue(k, currLen + j, col ? col->GetVariantValue(k).ToDouble() : 0.0);
    }
  }
}
