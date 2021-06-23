/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAnnotationLink.h"
#include "svtkChartXY.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestLinePlotSelection(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  svtkNew<svtkAnnotationLink> link;
  chart->SetAnnotationLink(link);
  chart->SetActionToButton(svtkChart::SELECT_POLYGON, svtkContextMouseEvent::LEFT_BUTTON);
  chart->SetSelectionMethod(svtkChart::SELECTION_ROWS);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  svtkNew<svtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  svtkNew<svtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, sin(i * inc) + 0.5);
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);

  view->Update();
  view->Render();

  // Inject some mouse events to perform selection.
  chart->SetSelectionMode(svtkContextScene::SELECTION_ADDITION);
  svtkContextMouseEvent event;
  event.SetInteractor(view->GetInteractor());
  event.SetPos(svtkVector2f(80, 50));
  event.SetButton(svtkContextMouseEvent::RIGHT_BUTTON);
  chart->MouseButtonPressEvent(event);
  event.SetPos(svtkVector2f(200, 200));
  chart->MouseButtonReleaseEvent(event);

  // Polygon now.
  event.SetPos(svtkVector2f(260, 50));
  event.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  chart->MouseButtonPressEvent(event);
  event.SetPos(svtkVector2f(220, 250));
  chart->MouseMoveEvent(event);
  event.SetPos(svtkVector2f(350, 90));
  chart->MouseButtonReleaseEvent(event);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
