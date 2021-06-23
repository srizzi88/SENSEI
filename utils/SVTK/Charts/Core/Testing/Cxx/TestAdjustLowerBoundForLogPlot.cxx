/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAdjustLowerBoundForLogPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAxis.h"
#include "svtkChartXY.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"

#include <cstdio>

int TestAdjustLowerBoundForLogPlot(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(300, 300);
  svtkNew<svtkChartXY> chart;
  chart->AdjustLowerBoundForLogPlotOn();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;

  svtkNew<svtkFloatArray> xArray;
  xArray->SetName("X");
  table->AddColumn(xArray);

  svtkNew<svtkFloatArray> dataArray;
  dataArray->SetName("Data");
  table->AddColumn(dataArray);

  int numRows = 100;
  table->SetNumberOfRows(numRows);
  for (int i = 0; i < numRows; ++i)
  {
    float x = 0.1 * ((-0.5 * (numRows - 1)) + i);
    table->SetValue(i, 0, x);
    float y = std::abs(x * x - 10.0);
    table->SetValue(i, 1, y);
  }

  svtkPlot* plot = chart->AddPlot(svtkChart::LINE);
  plot->SetInputData(table, 0, 1);

  svtkAxis* axis = chart->GetAxis(svtkAxis::LEFT);
  axis->LogScaleOn();

  // This sequence is necessary to invoke the logic when AdjustLowerBoundForLogPlot is enabled.
  view->GetRenderWindow()->Render();
  chart->RecalculateBounds();

  // Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
