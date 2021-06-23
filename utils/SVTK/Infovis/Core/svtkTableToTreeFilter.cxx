/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToTreeFilter.cxx

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

#include "svtkTableToTreeFilter.h"

#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"

#include <algorithm>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkTableToTreeFilter);

svtkTableToTreeFilter::svtkTableToTreeFilter() = default;

svtkTableToTreeFilter::~svtkTableToTreeFilter() = default;

void svtkTableToTreeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int svtkTableToTreeFilter::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTree");
  return 1;
}

int svtkTableToTreeFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

int svtkTableToTreeFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTable and outputTree handles
  svtkTable* table = svtkTable::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* tree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkSmartPointer<svtkTable> new_table = svtkSmartPointer<svtkTable>::New();
  new_table->DeepCopy(table);

  // Create a mutable graph for building the tree
  svtkSmartPointer<svtkMutableDirectedGraph> builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();

  // Check for a corner case where we have a table with 0 rows
  if (new_table->GetNumberOfRows() != 0)
  {

    // The tree will have one more vertex than the number of rows
    // in the table (the extra vertex is the new root).
    for (svtkIdType v = 0; v <= new_table->GetNumberOfRows(); ++v)
    {
      builder->AddVertex();
    }

    // Make a star, originating at the new root (the last vertex).
    svtkIdType root = new_table->GetNumberOfRows();
    for (svtkIdType v = 0; v < new_table->GetNumberOfRows(); ++v)
    {
      builder->AddEdge(root, v);
    }

    // Insert a row in the table for the new root.
    new_table->InsertNextBlankRow(-1);
  }

  // Move the structure of the mutable graph into the tree.
  if (!tree->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Built graph is not a valid tree!");
    return 0;
  }

  // Copy the table data into the tree vertex data
  tree->GetVertexData()->PassData(new_table->GetRowData());

  // The edge data should at least have a pedigree id array.
  svtkSmartPointer<svtkIdTypeArray> edgeIds = svtkSmartPointer<svtkIdTypeArray>::New();
  edgeIds->SetName("TableToTree edge");
  svtkIdType numEdges = tree->GetNumberOfEdges();
  edgeIds->SetNumberOfTuples(numEdges);
  for (svtkIdType i = 0; i < numEdges; ++i)
  {
    edgeIds->SetValue(i, i);
  }
  tree->GetEdgeData()->SetPedigreeIds(edgeIds);

  return 1;
}
