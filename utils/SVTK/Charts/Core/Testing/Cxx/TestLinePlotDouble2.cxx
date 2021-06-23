/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotDouble2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartXY.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestLinePlotDouble2(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkDoubleArray> arrX;
  arrX->SetName("X");
  table->AddColumn(arrX);
  svtkNew<svtkDoubleArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  // Test charting some very small points.
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    double x(1 + 1e-11 * inc * i);
    table->SetValue(i, 0, x);
    table->SetValue(i, 1, cos((x - 1.0) * 1.0e11));
  }
  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
