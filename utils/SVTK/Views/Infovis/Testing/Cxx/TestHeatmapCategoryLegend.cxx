/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapCategoryLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkHeatmapItem.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkContextView.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestHeatmapCategoryLegend(int argc, char* argv[])
{
  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> tableNames;
  svtkNew<svtkStringArray> column;

  tableNames->SetNumberOfTuples(4);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetValue(3, "a");
  tableNames->SetName("names");

  column->SetNumberOfTuples(4);
  column->SetName("values");
  column->SetValue(0, "c");
  column->SetValue(1, "b");
  column->SetValue(2, "a");
  column->SetValue(3, "a");

  table->AddColumn(tableNames);
  table->AddColumn(column);

  svtkNew<svtkHeatmapItem> heatmap;
  heatmap->SetTable(table);

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(heatmap);
  trans->Translate(125, 125);

  svtkNew<svtkContextView> contextView;
  contextView->GetScene()->AddItem(trans);

  contextView->GetRenderWindow()->SetMultiSamples(0);
  contextView->GetRenderWindow()->Render();

  // double click to display the category legend
  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(contextView->GetInteractor());
  svtkVector2f pos;
  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  pos.Set(16, 38);
  mouseEvent.SetPos(pos);
  heatmap->MouseDoubleClickEvent(mouseEvent);
  contextView->GetRenderWindow()->Render();

  int retVal = svtkRegressionTestImage(contextView->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    contextView->GetRenderWindow()->Render();
    contextView->GetInteractor()->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
