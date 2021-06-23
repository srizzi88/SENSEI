/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectToTable.cxx

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

#include "svtkDataObjectToTable.h"

#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkDataObjectToTable);
//---------------------------------------------------------------------------
svtkDataObjectToTable::svtkDataObjectToTable()
{
  this->FieldType = POINT_DATA;
}

//---------------------------------------------------------------------------
svtkDataObjectToTable::~svtkDataObjectToTable() = default;

//---------------------------------------------------------------------------
int svtkDataObjectToTable::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//---------------------------------------------------------------------------
int svtkDataObjectToTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get input data
  svtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inputInfo->Get(svtkDataObject::DATA_OBJECT());

  // Get output table
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outputInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If the input is a table, just copy it into the output.
  if (svtkTable::SafeDownCast(input))
  {
    output->ShallowCopy(input);
    return 1;
  }

  svtkDataSetAttributes* data = svtkDataSetAttributes::New();

  switch (this->FieldType)
  {
    case FIELD_DATA:
      if (input->GetFieldData())
      {
        data->ShallowCopy(input->GetFieldData());
      }
      break;
    case POINT_DATA:
      if (svtkDataSet* const dataset = svtkDataSet::SafeDownCast(input))
      {
        if (dataset->GetPointData())
        {
          data->ShallowCopy(dataset->GetPointData());
        }
      }
      break;
    case CELL_DATA:
      if (svtkDataSet* const dataset = svtkDataSet::SafeDownCast(input))
      {
        if (dataset->GetCellData())
        {
          data->ShallowCopy(dataset->GetCellData());
        }
      }
      break;
    case VERTEX_DATA:
      if (svtkGraph* const graph = svtkGraph::SafeDownCast(input))
      {
        if (graph->GetVertexData())
        {
          data->ShallowCopy(graph->GetVertexData());
        }
      }
      break;
    case EDGE_DATA:
      if (svtkGraph* const graph = svtkGraph::SafeDownCast(input))
      {
        if (graph->GetEdgeData())
        {
          data->ShallowCopy(graph->GetEdgeData());
        }
      }
      break;
  }

  output->SetRowData(data);
  data->Delete();
  return 1;
}

//---------------------------------------------------------------------------
void svtkDataObjectToTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
}
