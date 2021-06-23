/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestZoomAxis.cxx

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
int TestZoomAxis(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  svtkNew<svtkAnnotationLink> link;
  chart->SetAnnotationLink(link);
  chart->SetActionToButton(svtkChart::ZOOM_AXIS, svtkContextMouseEvent::LEFT_BUTTON);
  chart->SetSelectionMethod(svtkChart::SELECTION_PLOTS);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  // Test charting with a few more points...
  int numPoints = 100;
  float inc = 9.5f / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, sin(i * inc));
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* plot = chart->AddPlot(svtkChart::POINTS);
  plot->SetInputData(table, 0, 1);
  plot->SetColor(0, 255, 0, 255);
  plot->SetWidth(1.0);

  view->Update();
  view->Render();

  // Inject some mouse events to perform zooming
  svtkContextMouseEvent event;
  event.SetLastPos(svtkVector2f(0.0f));
  event.SetPos(svtkVector2f(0.0f));
  event.SetLastScenePos(svtkVector2f(0.0f));
  event.SetScenePos(svtkVector2f(0.0f));
  event.SetLastScreenPos(svtkVector2i(0));
  event.SetInteractor(view->GetInteractor());
  event.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  event.SetScreenPos(svtkVector2i(350, 250));
  chart->MouseButtonPressEvent(event);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetScreenPos(svtkVector2i(10, 10));
  chart->MouseMoveEvent(event);
  chart->MouseButtonReleaseEvent(event);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
