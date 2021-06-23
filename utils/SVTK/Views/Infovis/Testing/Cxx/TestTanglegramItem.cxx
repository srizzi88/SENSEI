/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTanglegramItem.cxx

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
#include "svtkTable.h"
#include "svtkTanglegramItem.h"
#include "svtkTree.h"

#include "svtkContextActor.h"
#include "svtkContextInteractorStyle.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestTanglegramItem(int argc, char* argv[])
{
  // tree #1
  svtkNew<svtkMutableDirectedGraph> graph1;
  svtkIdType root = graph1->AddVertex();
  svtkIdType internalOne = graph1->AddChild(root);
  svtkIdType internalTwo = graph1->AddChild(internalOne);
  svtkIdType a = graph1->AddChild(internalTwo);
  svtkIdType b = graph1->AddChild(internalTwo);
  svtkIdType c = graph1->AddChild(internalOne);

  svtkNew<svtkDoubleArray> weights;
  weights->SetNumberOfTuples(5);
  weights->SetValue(graph1->GetEdgeId(root, internalOne), 1.0f);
  weights->SetValue(graph1->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights->SetValue(graph1->GetEdgeId(internalTwo, a), 1.0f);
  weights->SetValue(graph1->GetEdgeId(internalTwo, b), 1.0f);
  weights->SetValue(graph1->GetEdgeId(internalOne, c), 3.0f);

  weights->SetName("weight");
  graph1->GetEdgeData()->AddArray(weights);

  svtkNew<svtkStringArray> names1;
  names1->SetNumberOfTuples(6);
  names1->SetValue(a, "cat");
  names1->SetValue(b, "dog");
  names1->SetValue(c, "human");

  names1->SetName("node name");
  graph1->GetVertexData()->AddArray(names1);

  svtkNew<svtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfTuples(6);
  nodeWeights->SetValue(root, 0.0f);
  nodeWeights->SetValue(internalOne, 1.0f);
  nodeWeights->SetValue(internalTwo, 3.0f);
  nodeWeights->SetValue(a, 4.0f);
  nodeWeights->SetValue(b, 4.0f);
  nodeWeights->SetValue(c, 4.0f);
  nodeWeights->SetName("node weight");
  graph1->GetVertexData()->AddArray(nodeWeights);

  // tree #2
  svtkNew<svtkMutableDirectedGraph> graph2;
  root = graph2->AddVertex();
  internalOne = graph2->AddChild(root);
  internalTwo = graph2->AddChild(internalOne);
  a = graph2->AddChild(internalTwo);
  b = graph2->AddChild(internalTwo);
  c = graph2->AddChild(internalOne);

  weights->SetName("weight");
  graph2->GetEdgeData()->AddArray(weights);

  svtkNew<svtkStringArray> names2;
  names2->SetNumberOfTuples(6);
  names2->SetValue(a, "dog food");
  names2->SetValue(b, "cat food");
  names2->SetValue(c, "steak");

  names2->SetName("node name");
  graph2->GetVertexData()->AddArray(names2);

  graph2->GetVertexData()->AddArray(nodeWeights);

  // set up correspondence table: who eats what
  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> eaters;
  svtkNew<svtkDoubleArray> hungerForSteak;
  hungerForSteak->SetName("steak");
  svtkNew<svtkDoubleArray> hungerForDogFood;
  hungerForDogFood->SetName("dog food");
  svtkNew<svtkDoubleArray> hungerForCatFood;
  hungerForCatFood->SetName("cat food");

  eaters->SetNumberOfTuples(3);
  hungerForSteak->SetNumberOfTuples(3);
  hungerForDogFood->SetNumberOfTuples(3);
  hungerForCatFood->SetNumberOfTuples(3);

  eaters->SetValue(0, "human");
  eaters->SetValue(1, "dog");
  eaters->SetValue(2, "cat");

  hungerForSteak->SetValue(0, 2.0);
  hungerForSteak->SetValue(1, 1.0);
  hungerForSteak->SetValue(2, 1.0);

  hungerForDogFood->SetValue(0, 0.0);
  hungerForDogFood->SetValue(1, 2.0);
  hungerForDogFood->SetValue(2, 0.0);

  hungerForCatFood->SetValue(0, 0.0);
  hungerForCatFood->SetValue(1, 1.0);
  hungerForCatFood->SetValue(2, 2.0);

  table->AddColumn(eaters);
  table->AddColumn(hungerForSteak);
  table->AddColumn(hungerForDogFood);
  table->AddColumn(hungerForCatFood);

  svtkNew<svtkContextActor> actor;

  svtkNew<svtkTree> tree1;
  tree1->ShallowCopy(graph1);

  svtkNew<svtkTree> tree2;
  tree2->ShallowCopy(graph2);

  svtkNew<svtkTanglegramItem> tanglegram;
  tanglegram->SetTree1(tree1);
  tanglegram->SetTree2(tree2);
  tanglegram->SetTable(table);
  tanglegram->SetTree1Label("Diners");
  tanglegram->SetTree2Label("Meals");

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(tanglegram);
  // center the item within the render window
  trans->Translate(20, 75);
  trans->Scale(1.25, 1.25);
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

  int retVal = svtkRegressionTestImageThreshold(renderWindow, 100);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
