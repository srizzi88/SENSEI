/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartMatrix.h"
#include "svtkChartXY.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestChartMatrix(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartMatrix> matrix;
  view->GetScene()->AddItem(matrix);
  matrix->SetSize(svtkVector2i(2, 2));
  matrix->SetGutter(svtkVector2f(30.0, 30.0));

  svtkChart* chart = matrix->GetChart(svtkVector2i(0, 0));

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
  svtkNew<svtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  svtkNew<svtkFloatArray> tangent;
  tangent->SetName("Tangent");
  table->AddColumn(tangent);
  // Test charting with a few more points...
  int numPoints = 42;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* line = chart->AddPlot(svtkChart::POINTS);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);

  chart = matrix->GetChart(svtkVector2i(0, 1));
  line = chart->AddPlot(svtkChart::POINTS);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);

  chart = matrix->GetChart(svtkVector2i(1, 0));
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);

  chart = matrix->GetChart(svtkVector2i(1, 1));
  line = chart->AddPlot(svtkChart::BAR);
  line->SetInputData(table, 0, 4);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
