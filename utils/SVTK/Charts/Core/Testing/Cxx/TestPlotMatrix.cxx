/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

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
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestPlotMatrix(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 300);
  svtkSmartPointer<svtkChartXY> chart = svtkSmartPointer<svtkChartXY>::New();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();
  svtkSmartPointer<svtkFloatArray> arrX = svtkSmartPointer<svtkFloatArray>::New();
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkSmartPointer<svtkFloatArray> arrC = svtkSmartPointer<svtkFloatArray>::New();
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  svtkSmartPointer<svtkFloatArray> arrS = svtkSmartPointer<svtkFloatArray>::New();
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  svtkSmartPointer<svtkFloatArray> arrS2 = svtkSmartPointer<svtkFloatArray>::New();
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

  // Set to fixes size, and resize to make it small.
  chart->SetAutoSize(false);
  chart->SetSize(svtkRectf(0.0, 0.0, 200.0, 150.0));

  // Now Set up another plot with cos
  svtkSmartPointer<svtkChartXY> chart2 = svtkSmartPointer<svtkChartXY>::New();
  view->GetScene()->AddItem(chart2);
  line = chart2->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);
  chart2->SetAutoSize(false);
  chart2->SetSize(svtkRectf(200.0, 0.0, 200.0, 150.0));

  // Now Set up another plot with cos
  svtkSmartPointer<svtkChartXY> chart3 = svtkSmartPointer<svtkChartXY>::New();
  view->GetScene()->AddItem(chart3);
  line = chart3->AddPlot(svtkChart::POINTS);
  line->SetInputData(table, 0, 1);
  chart3->SetAutoSize(false);
  chart3->SetSize(svtkRectf(0.0, 150.0, 200.0, 150.0));

  // Now Set up another plot with cos
  svtkSmartPointer<svtkChartXY> chart4 = svtkSmartPointer<svtkChartXY>::New();
  view->GetScene()->AddItem(chart4);
  line = chart4->AddPlot(svtkChart::BAR);
  line->SetInputData(table, 0, 1);
  chart4->GetAxis(svtkAxis::BOTTOM)->SetBehavior(svtkAxis::FIXED);
  chart4->GetAxis(svtkAxis::BOTTOM)->SetRange(0.0, 10.0);
  chart4->SetAutoSize(false);
  chart4->SetSize(svtkRectf(200.0, 150.0, 200.0, 150.0));

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
