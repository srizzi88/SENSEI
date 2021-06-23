/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeHeatmapAutoCollapse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDendrogramItem.h"
#include "svtkDoubleArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkTreeHeatmapItem.h"

#include "svtkContextActor.h"
#include "svtkContextInteractorStyle.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestTreeHeatmapAutoCollapse(int argc, char* argv[])
{
  svtkNew<svtkMutableDirectedGraph> graph;
  svtkIdType root = graph->AddVertex();
  svtkIdType internalOne = graph->AddChild(root);
  svtkIdType internalTwo = graph->AddChild(internalOne);
  svtkIdType a = graph->AddChild(internalTwo);
  svtkIdType b = graph->AddChild(internalTwo);
  svtkIdType c = graph->AddChild(internalOne);

  svtkNew<svtkDoubleArray> weights;
  weights->SetNumberOfTuples(5);
  weights->SetValue(graph->GetEdgeId(root, internalOne), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, a), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, b), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, c), 3.0f);

  weights->SetName("weight");
  graph->GetEdgeData()->AddArray(weights);

  svtkNew<svtkStringArray> names;
  names->SetNumberOfTuples(6);
  names->SetValue(a, "a");
  names->SetValue(b, "b");
  names->SetValue(c, "c");

  names->SetName("node name");
  graph->GetVertexData()->AddArray(names);

  svtkNew<svtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfTuples(6);
  nodeWeights->SetValue(root, 0.0f);
  nodeWeights->SetValue(internalOne, 1.0f);
  nodeWeights->SetValue(internalTwo, 3.0f);
  nodeWeights->SetValue(a, 4.0f);
  nodeWeights->SetValue(b, 4.0f);
  nodeWeights->SetValue(c, 4.0f);
  nodeWeights->SetName("node weight");
  graph->GetVertexData()->AddArray(nodeWeights);

  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> tableNames;
  svtkNew<svtkDoubleArray> m1;
  svtkNew<svtkDoubleArray> m2;
  svtkNew<svtkDoubleArray> m3;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetName("name");

  m1->SetNumberOfTuples(3);
  m2->SetNumberOfTuples(3);
  m3->SetNumberOfTuples(3);

  m1->SetName("m1");
  m2->SetName("m2");
  m3->SetName("m3");

  m1->SetValue(0, 1.0f);
  m1->SetValue(1, 3.0f);
  m1->SetValue(2, 1.0f);

  m2->SetValue(0, 2.0f);
  m2->SetValue(1, 2.0f);
  m2->SetValue(2, 2.0f);

  m3->SetValue(0, 3.0f);
  m3->SetValue(1, 1.0f);
  m3->SetValue(2, 3.0f);

  table->AddColumn(tableNames);
  table->AddColumn(m1);
  table->AddColumn(m2);
  table->AddColumn(m3);

  svtkNew<svtkContextActor> actor;

  svtkNew<svtkTree> tree;
  tree->ShallowCopy(graph);

  svtkNew<svtkTreeHeatmapItem> treeItem;
  treeItem->SetTree(tree);
  treeItem->SetTable(table);
  treeItem->GetDendrogram()->DisplayNumberOfCollapsedLeafNodesOff();

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  // center the item within the render window
  trans->Translate(20, 30);
  trans->Scale(2.5, 2.5);
  trans->AddItem(treeItem);
  actor->GetScene()->AddItem(trans);

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 200);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  actor->GetScene()->SetRenderer(renderer);

  svtkNew<svtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(actor->GetScene());

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetInteractorStyle(interactorStyle);
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  // automatically collapse down to the two leaf nodes that are closest
  // to the root.
  treeItem->CollapseToNumberOfLeafNodes(2);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
