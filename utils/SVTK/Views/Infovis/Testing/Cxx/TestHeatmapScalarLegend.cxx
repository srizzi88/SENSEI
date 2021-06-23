/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapScalarLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkHeatmapItem.h"
#include "svtkIntArray.h"
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
int TestHeatmapScalarLegend(int argc, char* argv[])
{
  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> tableNames;
  svtkNew<svtkIntArray> column;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "3");
  tableNames->SetValue(1, "2");
  tableNames->SetValue(2, "1");
  tableNames->SetName("names");

  column->SetNumberOfTuples(3);
  column->SetName("values");
  column->SetValue(0, 3);
  column->SetValue(1, 2);
  column->SetValue(2, 1);

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

  // double click to display the color legend
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
