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

#include "svtkChartXY.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlotPoints.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestScatterPlot(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  chart->SetShowLegend(true);

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
  svtkNew<svtkFloatArray> arrT;
  arrT->SetName("Tan");
  table->AddColumn(arrT);
  svtkNew<svtkStringArray> labels;
  labels->SetName("Labels");
  table->AddColumn(labels);

  // Test charting with a few more points...
  int numPoints = 40;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
    if (i % 2)
    {
      table->SetValue(i, 4, svtkStdString("Odd"));
    }
    else
    {
      table->SetValue(i, 4, svtkStdString("Even"));
    }
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* points = chart->AddPlot(svtkChart::POINTS);
  points->SetInputData(table, 0, 1);
  points->SetColor(0, 0, 0, 255);
  points->SetWidth(1.0);
  points->SetIndexedLabels(labels);
  points->SetTooltipLabelFormat("%i from %l (%x, %y)");
  svtkPlotPoints::SafeDownCast(points)->SetMarkerStyle(svtkPlotPoints::CROSS);
  points = chart->AddPlot(svtkChart::POINTS);
  points->SetInputData(table, 0, 2);
  points->SetColor(0, 0, 0, 255);
  points->SetWidth(1.0);
  svtkPlotPoints::SafeDownCast(points)->SetMarkerStyle(svtkPlotPoints::PLUS);
  points = chart->AddPlot(svtkChart::POINTS);
  points->SetInputData(table, 0, 3);
  points->SetColor(0, 0, 255, 255);
  points->SetWidth(4.0);
  points->SetIndexedLabels(labels);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
