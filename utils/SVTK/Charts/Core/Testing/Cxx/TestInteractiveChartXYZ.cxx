/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartXYZ.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartXYZ.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlotPoints3D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVector.h"

int TestInteractiveChartXYZ(int, char*[])
{
  // Now the chart
  svtkNew<svtkChartXYZ> chart;
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart);

  chart->SetGeometry(svtkRectf(75.0, 20.0, 250, 260));

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
  svtkNew<svtkFloatArray> arrColor;
  arrColor->SetName("Color");
  table->AddColumn(arrColor);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, i);
  }

  // Add the dimensions we are interested in visualizing.
  svtkNew<svtkPlotPoints3D> plot;
  plot->SetInputData(table, "X Axis", "Sine", "Cosine", "Color");
  chart->AddPlot(plot);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  svtkVector2i pos;
  svtkVector2i lastPos;

  // rotate
  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  lastPos.Set(114, 55);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(174, 121);
  mouseEvent.SetScreenPos(pos);

  svtkVector2d sP(pos.Cast<double>().GetData());
  svtkVector2d lSP(lastPos.Cast<double>().GetData());

  svtkVector2d screenPos(mouseEvent.GetScreenPos().Cast<double>().GetData());
  svtkVector2d lastScreenPos(mouseEvent.GetLastScreenPos().Cast<double>().GetData());
  chart->MouseMoveEvent(mouseEvent);

  // spin
  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(10, 10);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // zoom
  mouseEvent.SetButton(svtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(0);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(0, 10);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // mouse wheel zoom
  chart->MouseWheelEvent(mouseEvent, -1);

  // pan
  mouseEvent.SetButton(svtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(10, 10);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(0, 0);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // remove colors
  plot->SetInputData(table, "X Axis", "Sine", "Cosine");
  view->GetRenderWindow()->Render();

  // add them back in
  plot->SetColors(arrColor);

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
