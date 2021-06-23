/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartDouble.cxx

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
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include "svtkAxis.h"

//----------------------------------------------------------------------------
int TestChartDouble(int, char*[])
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
  arrC->SetName("f1");
  table->AddColumn(arrC);
  svtkNew<svtkDoubleArray> arrS;
  arrS->SetName("f2");
  table->AddColumn(arrS);
  svtkNew<svtkDoubleArray> arrS2;
  arrS2->SetName("f3");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    double x(i * inc + 0.2);
    table->SetValue(i, 0, x);
    table->SetValue(i, 1, 1.0e-80 * (cos(x - 1.0) + sin(x - svtkMath::Pi() / 4.0)));
    table->SetValue(i, 2, 1.0e-80 * sin(x) * 1e-12);
    table->SetValue(i, 3, 1.0e-80 * sin(x - 1.0));
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* line = chart->AddPlot(svtkChart::POINTS);
  line->SetInputData(table, 0, 1);
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 2);
  // Put this plot in a different corner - it is orders of magnitude smaller.
  chart->SetPlotCorner(line, 1);
  line = chart->AddPlot(svtkChart::BAR);
  line->SetInputData(table, 0, 3);

  chart->GetAxis(svtkAxis::LEFT)->SetTitle("A tiny range");
  chart->GetAxis(svtkAxis::BOTTOM)->SetTitle("A normal range");
  chart->GetAxis(svtkAxis::RIGHT)->SetTitle("An even tinier range");

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
