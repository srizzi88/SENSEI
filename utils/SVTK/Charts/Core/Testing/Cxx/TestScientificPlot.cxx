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
int TestScientificPlot(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 400);
  svtkSmartPointer<svtkChartXY> chart = svtkSmartPointer<svtkChartXY>::New();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();
  svtkSmartPointer<svtkFloatArray> arrX = svtkSmartPointer<svtkFloatArray>::New();
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkSmartPointer<svtkFloatArray> arrC = svtkSmartPointer<svtkFloatArray>::New();
  arrC->SetName("cos");
  table->AddColumn(arrC);
  svtkSmartPointer<svtkFloatArray> arrS = svtkSmartPointer<svtkFloatArray>::New();
  arrS->SetName("sin");
  table->AddColumn(arrS);
  svtkSmartPointer<svtkFloatArray> arrS2 = svtkSmartPointer<svtkFloatArray>::New();
  arrS2->SetName("x^3");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 3.0 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    double v = -1.0 + i * inc;
    table->SetValue(i, 0, v);
    table->SetValue(i, 1, cos(v));
    table->SetValue(i, 2, sin(v) + 0.0);
    table->SetValue(i, 3, v * v * v);
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line = chart->AddPlot(svtkChart::POINTS);
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);

  // Set up a scientific plot...
  chart->SetDrawAxesAtOrigin(true);
  chart->SetShowLegend(true);
  chart->GetAxis(svtkAxis::LEFT)->SetRange(1.0, -1.5);
  chart->GetAxis(svtkAxis::LEFT)->SetNotation(2);
  chart->GetAxis(svtkAxis::LEFT)->SetPrecision(1);
  chart->GetAxis(svtkAxis::LEFT)->SetBehavior(svtkAxis::FIXED);
  chart->GetAxis(svtkAxis::LEFT)->SetTitle("");
  chart->GetAxis(svtkAxis::BOTTOM)->SetRange(-1.0, 1.5);
  chart->GetAxis(svtkAxis::BOTTOM)->SetNotation(2);
  chart->GetAxis(svtkAxis::BOTTOM)->SetPrecision(1);
  chart->GetAxis(svtkAxis::BOTTOM)->SetBehavior(svtkAxis::FIXED);
  chart->GetAxis(svtkAxis::BOTTOM)->SetTitle("");

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
