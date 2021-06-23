/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPruneTreeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPruneTreeFilter.h"
#include "svtkTree.h"

//----------------------------------------------------------------------------
int TestPruneTreeFilter(int, char*[])
{
  svtkNew<svtkMutableDirectedGraph> graph;
  svtkIdType root = graph->AddVertex();
  svtkIdType internalOne = graph->AddChild(root);
  svtkIdType internalTwo = graph->AddChild(internalOne);
  svtkIdType a = graph->AddChild(internalTwo);
  graph->AddChild(internalTwo);
  graph->AddChild(internalOne);
  graph->AddChild(a);
  graph->AddChild(a);

  svtkNew<svtkTree> tree;
  tree->ShallowCopy(graph);

  svtkNew<svtkPruneTreeFilter> filter;
  filter->SetInputData(tree);
  filter->SetParentVertex(internalTwo);
  svtkTree* prunedTree = filter->GetOutput();
  filter->Update();

  if (prunedTree->GetNumberOfVertices() == 3)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
