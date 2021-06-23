/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeDifferenceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkTreeDifferenceFilter.h"

//----------------------------------------------------------------------------
int TestTreeDifferenceFilter(int, char*[])
{
  // create tree 1
  svtkNew<svtkMutableDirectedGraph> graph1;
  svtkIdType root = graph1->AddVertex();
  svtkIdType internalOne = graph1->AddChild(root);
  svtkIdType internalTwo = graph1->AddChild(internalOne);
  svtkIdType a = graph1->AddChild(internalTwo);
  svtkIdType b = graph1->AddChild(internalTwo);
  svtkIdType c = graph1->AddChild(internalOne);

  svtkNew<svtkDoubleArray> weights1;
  weights1->SetNumberOfTuples(5);
  weights1->SetValue(graph1->GetEdgeId(root, internalOne), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights1->SetValue(graph1->GetEdgeId(internalTwo, a), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalTwo, b), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalOne, c), 3.0f);
  weights1->SetName("weight");
  graph1->GetEdgeData()->AddArray(weights1);

  svtkNew<svtkStringArray> names1;
  names1->SetNumberOfTuples(6);
  names1->SetValue(a, "a");
  names1->SetValue(b, "b");
  names1->SetValue(c, "c");
  names1->SetName("node name");
  graph1->GetVertexData()->AddArray(names1);

  svtkNew<svtkTree> tree1;
  tree1->ShallowCopy(graph1);

  // create tree 2.  Same topology as tree 1, but its vertices are created in
  // a different order.  Also, its edge weights are different.
  svtkNew<svtkMutableDirectedGraph> graph2;
  root = graph2->AddVertex();
  internalOne = graph2->AddChild(root);
  c = graph2->AddChild(internalOne);
  internalTwo = graph2->AddChild(internalOne);
  b = graph2->AddChild(internalTwo);
  a = graph2->AddChild(internalTwo);

  svtkNew<svtkStringArray> names2;
  names2->SetNumberOfTuples(6);
  names2->SetValue(a, "a");
  names2->SetValue(b, "b");
  names2->SetValue(c, "c");
  names2->SetName("node name");
  graph2->GetVertexData()->AddArray(names2);

  svtkNew<svtkDoubleArray> weights2;
  weights2->SetNumberOfTuples(5);
  weights2->SetValue(graph2->GetEdgeId(root, internalOne), 2.0f);
  weights2->SetValue(graph2->GetEdgeId(internalOne, internalTwo), 4.0f);
  weights2->SetValue(graph2->GetEdgeId(internalTwo, a), 4.0f);
  weights2->SetValue(graph2->GetEdgeId(internalTwo, b), 5.0f);
  weights2->SetValue(graph2->GetEdgeId(internalOne, c), 8.0f);
  weights2->SetName("weight");
  graph2->GetEdgeData()->AddArray(weights2);

  svtkNew<svtkTree> tree2;
  tree2->ShallowCopy(graph2);

  svtkNew<svtkTreeDifferenceFilter> filter;
  filter->Print(std::cout);
  filter->SetInputDataObject(0, tree1);
  filter->SetInputDataObject(1, tree2);
  filter->SetIdArrayName("node name");
  filter->SetComparisonArrayIsVertexData(false);
  filter->SetComparisonArrayName("weight");
  filter->SetOutputArrayName("weight differences");

  filter->Update();

  svtkNew<svtkTree> outputTree;
  outputTree->ShallowCopy(filter->GetOutput());
  svtkDoubleArray* diff = svtkArrayDownCast<svtkDoubleArray>(
    outputTree->GetEdgeData()->GetAbstractArray("weight differences"));

  if (diff->GetValue(0) != -1.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(1) != -2.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(2) != -3.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(3) != -4.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(4) != -5.0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
