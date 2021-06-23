/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotDouble.cxx

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
#include "svtkFloatingPointExceptions.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"
//----------------------------------------------------------------------------
int TestLinePlotDouble(int, char*[])
{
  // Thus test will cause floating point because it uses inf and nan for some
  // numbers
  svtkFloatingPointExceptions::Disable();

  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkDoubleArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkDoubleArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  svtkNew<svtkDoubleArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  svtkNew<svtkDoubleArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, 1.0e-80 * cos(i * inc - 1.0) * 1.0e-8);
    table->SetValue(i, 2, 1.0e-80 * sin(i * inc) * 1.0e-8);
    table->SetValue(i, 3, 1.0e80 * sin(i * inc - 1.0));
  }
  table->SetValue(66, 2, svtkMath::Nan());
  table->SetValue(4, 3, svtkMath::Inf());

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
  chart->SetPlotCorner(line, 1);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
