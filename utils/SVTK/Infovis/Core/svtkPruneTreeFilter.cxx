/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPruneTreeFilter.cxx

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

#include "svtkPruneTreeFilter.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"

#include <utility>
#include <vector>

svtkStandardNewMacro(svtkPruneTreeFilter);

svtkPruneTreeFilter::svtkPruneTreeFilter()
{
  this->ParentVertex = 0;
  this->ShouldPruneParentVertex = true;
}

svtkPruneTreeFilter::~svtkPruneTreeFilter() = default;

void svtkPruneTreeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Parent: " << this->ParentVertex << endl;
}

int svtkPruneTreeFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTree* inputTree = svtkTree::GetData(inputVector[0]);
  svtkTree* outputTree = svtkTree::GetData(outputVector);

  if (this->ParentVertex < 0 || this->ParentVertex >= inputTree->GetNumberOfVertices())
  {
    svtkErrorMacro("Parent vertex must be part of the tree "
      << this->ParentVertex << " >= " << inputTree->GetNumberOfVertices());
    return 0;
  }

  // Structure for building the tree.
  svtkSmartPointer<svtkMutableDirectedGraph> builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();

  // Child iterator.
  svtkSmartPointer<svtkOutEdgeIterator> it = svtkSmartPointer<svtkOutEdgeIterator>::New();

  // Get the input and builder vertex and edge data.
  svtkDataSetAttributes* inputVertexData = inputTree->GetVertexData();
  svtkDataSetAttributes* inputEdgeData = inputTree->GetEdgeData();
  svtkDataSetAttributes* builderVertexData = builder->GetVertexData();
  svtkDataSetAttributes* builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Copy field data
  builder->GetFieldData()->DeepCopy(inputTree->GetFieldData());

  // Build a copy of the tree, skipping the parent vertex to remove.
  std::vector<std::pair<svtkIdType, svtkIdType> > vertStack;
  if (inputTree->GetRoot() != this->ParentVertex)
  {
    vertStack.push_back(std::make_pair(inputTree->GetRoot(), builder->AddVertex()));
  }
  while (!vertStack.empty())
  {
    svtkIdType tree_v = vertStack.back().first;
    svtkIdType v = vertStack.back().second;
    builderVertexData->CopyData(inputVertexData, tree_v, v);
    vertStack.pop_back();
    inputTree->GetOutEdges(tree_v, it);
    while (it->HasNext())
    {
      svtkOutEdgeType tree_e = it->Next();
      svtkIdType tree_child = tree_e.Target;
      if (this->ShouldPruneParentVertex)
      {
        if (tree_child != this->ParentVertex)
        {
          svtkIdType child = builder->AddVertex();
          svtkEdgeType e = builder->AddEdge(v, child);
          builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
          vertStack.push_back(std::make_pair(tree_child, child));
        }
      }
      else
      {
        svtkIdType child = builder->AddVertex();
        svtkEdgeType e = builder->AddEdge(v, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        if (tree_child != this->ParentVertex)
        {
          vertStack.push_back(std::make_pair(tree_child, child));
        }
        else
        {
          builderVertexData->CopyData(inputVertexData, tree_child, child);
        }
      }
    }
  }

  // Copy the structure into the output.
  if (!outputTree->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid tree structure.");
    return 0;
  }

  return 1;
}
