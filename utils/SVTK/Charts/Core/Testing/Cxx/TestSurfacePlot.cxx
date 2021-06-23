/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfacePlot.cxx

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
#include "svtkPlotSurface.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVector.h"

int TestSurfacePlot(int, char*[])
{
  svtkNew<svtkChartXYZ> chart;
  svtkNew<svtkPlotSurface> plot;
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart);

  chart->SetGeometry(svtkRectf(75.0, 20.0, 250, 260));

  // Create a surface
  svtkNew<svtkTable> table;
  svtkIdType numPoints = 70;
  float inc = 9.424778 / (numPoints - 1);
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    svtkNew<svtkFloatArray> arr;
    table->AddColumn(arr);
  }
  table->SetNumberOfRows(static_cast<svtkIdType>(numPoints));
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    float x = i * inc;
    for (svtkIdType j = 0; j < numPoints; ++j)
    {
      float y = j * inc;
      table->SetValue(i, j, sin(sqrt(x * x + y * y)));
    }
  }

  // Set up the surface plot we wish to visualize and add it to the chart.
  plot->SetXRange(0, 9.424778);
  plot->SetYRange(0, 9.424778);
  plot->SetInputData(table);
  chart->AddPlot(plot);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  // rotate
  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  svtkVector2i pos;
  svtkVector2i lastPos;

  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  lastPos.Set(100, 50);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(150, 100);
  mouseEvent.SetScreenPos(pos);
  svtkVector2d sP(pos.Cast<double>().GetData());
  svtkVector2d lSP(lastPos.Cast<double>().GetData());
  svtkVector2d screenPos(mouseEvent.GetScreenPos().Cast<double>().GetData());
  svtkVector2d lastScreenPos(mouseEvent.GetLastScreenPos().Cast<double>().GetData());
  chart->MouseMoveEvent(mouseEvent);

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
