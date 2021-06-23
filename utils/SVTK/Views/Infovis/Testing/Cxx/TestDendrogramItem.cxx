/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDendrogramItem.cxx

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
#include "svtkTree.h"

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
int TestDendrogramItem(int argc, char* argv[])
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

  svtkNew<svtkContextActor> actor;

  svtkNew<svtkTree> tree;
  tree->ShallowCopy(graph);

  svtkNew<svtkDendrogramItem> dendrogram;
  dendrogram->SetTree(tree);
  dendrogram->SetPosition(40, 15);

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(dendrogram);
  trans->Scale(3, 3);
  actor->GetScene()->AddItem(trans);

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 200);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);

  svtkNew<svtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(actor->GetScene());

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetInteractorStyle(interactorStyle);
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
