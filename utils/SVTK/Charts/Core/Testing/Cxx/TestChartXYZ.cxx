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

#include "svtkAxis.h"
#include "svtkChartXYZ.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkPlotPoints3D.h"

#include "svtkCallbackCommand.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"

// Need a timer so that we can animate, and then take a snapshot!
namespace
{
static double angle = 0;

void ProcessEvents(svtkObject* caller, unsigned long, void* clientData, void* callerData)
{
  svtkChartXYZ* chart = reinterpret_cast<svtkChartXYZ*>(clientData);
  svtkRenderWindowInteractor* interactor = reinterpret_cast<svtkRenderWindowInteractor*>(caller);
  angle += 2;
  chart->SetAngle(angle);
  interactor->Render();
  if (angle >= 90)
  {
    int timerId = *reinterpret_cast<int*>(callerData);
    interactor->DestroyTimer(timerId);
  }
}
} // End of anonymous namespace.

int TestChartXYZ(int, char*[])
{
  // Now the chart
  svtkNew<svtkChartXYZ> chart;
  chart->SetAutoRotate(true);
  chart->SetFitToScene(false);
  chart->SetDecorateAxes(false);
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart);
  svtkNew<svtkChartXYZ> chart2;
  chart2->SetAutoRotate(true);
  chart2->SetFitToScene(false);
  chart->SetDecorateAxes(false);
  view->GetScene()->AddItem(chart2);

  chart->SetGeometry(svtkRectf(75.0, 20.0, 250, 260));
  chart2->SetGeometry(svtkRectf(75.0, 20.0, 250, 260));

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
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

  // chart->SetAroundX(true);
  // Add the three dimensions we are interested in visualizing.
  svtkNew<svtkPlotPoints3D> plot;
  plot->SetInputData(table, "X Axis", "Sine", "Cosine");
  chart->AddPlot(plot);
  const svtkColor4ub axisColor(20, 200, 30);
  chart->SetAxisColor(axisColor);
  chart->GetAxis(0)->SetUnscaledRange(-0.1, 7.6);
  chart->GetAxis(1)->SetUnscaledRange(-1.1, 1.1);
  chart->GetAxis(2)->SetUnscaledRange(-1.1, 1.1);
  chart->RecalculateTransform();

  // We want a duplicate, that does not move.
  svtkNew<svtkPlotPoints3D> plot2;
  plot2->SetInputData(table, "X Axis", "Sine", "Cosine");
  chart2->AddPlot(plot2);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  // Set up the timer, and be sure to incrememt the angle.
  svtkNew<svtkCallbackCommand> callback;
  callback->SetClientData(chart);
  callback->SetCallback(::ProcessEvents);
  view->GetInteractor()->AddObserver(svtkCommand::TimerEvent, callback, 0);
  view->GetInteractor()->CreateRepeatingTimer(1000 / 25);

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
