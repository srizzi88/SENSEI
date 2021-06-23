/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDoubleArray.h"
#include "svtkHeatmapItem.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

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
int TestHeatmapItem(int argc, char* argv[])
{
  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> tableNames;
  svtkNew<svtkDoubleArray> m1;
  svtkNew<svtkDoubleArray> m2;
  svtkNew<svtkDoubleArray> m3;
  svtkNew<svtkStringArray> m4;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetName("name");

  m1->SetNumberOfTuples(3);
  m2->SetNumberOfTuples(3);
  m3->SetNumberOfTuples(3);
  m4->SetNumberOfTuples(3);

  m1->SetName("m1");
  m2->SetName("m2");
  m3->SetName("m3");
  m4->SetName("m4");

  m1->SetValue(0, 1.0f);
  m1->SetValue(1, 3.0f);
  m1->SetValue(2, 1.0f);

  m2->SetValue(0, 2.0f);
  m2->SetValue(1, 2.0f);
  m2->SetValue(2, 2.0f);

  m3->SetValue(0, 3.0f);
  m3->SetValue(1, 1.0f);
  m3->SetValue(2, 3.0f);

  m4->SetValue(0, "a");
  m4->SetValue(1, "b");
  m4->SetValue(2, "c");

  table->AddColumn(tableNames);
  table->AddColumn(m1);
  table->AddColumn(m2);
  table->AddColumn(m3);
  table->AddColumn(m4);

  svtkNew<svtkContextActor> actor;

  svtkNew<svtkHeatmapItem> heatmap;
  heatmap->SetTable(table);
  heatmap->SetPosition(20, 5);

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(heatmap);
  trans->Scale(2, 2);
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

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
