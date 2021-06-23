/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartDoubleColors.cxx

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
#include "svtkDoubleArray.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPen.h"
#include "svtkPlotBar.h"
#include "svtkPlotLine.h"
#include "svtkPlotPoints.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestChartDoubleColors(int, char*[])
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
  svtkNew<svtkDoubleArray> arrColor;
  arrColor->SetName("color");
  table->AddColumn(arrColor);
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
    table->SetValue(i, 4, cos(i * inc));
  }

  svtkNew<svtkLookupTable> lut;
  lut->SetValueRange(0.0, 1.0);
  lut->SetSaturationRange(1.0, 1.0);
  lut->SetHueRange(0.4, 0.9);
  lut->SetAlphaRange(0.2, 0.8);
  lut->SetRange(-1.0, 1.0);
  lut->SetRampToLinear();
  lut->Build();

  // Add multiple line plots, setting the colors etc
  svtkNew<svtkPlotPoints> points;
  chart->AddPlot(points);
  points->SetInputData(table, 0, 1);
  points->SetMarkerSize(10.0);
  points->ScalarVisibilityOn();
  points->SelectColorArray("color");
  points->SetLookupTable(lut);
  svtkNew<svtkPlotLine> line;
  chart->AddPlot(line);
  line->SetInputData(table, 0, 2);
  line->SetColor(1.0, 0.0, 0.0);
  // Put this plot in a different corner - it is orders of magnitude smaller.
  chart->SetPlotCorner(line, 1);
  svtkNew<svtkPlotBar> bar;
  chart->AddPlot(bar);
  bar->SetInputData(table, 0, 3);
  bar->ScalarVisibilityOn();
  bar->SelectColorArray("color");
  bar->SetLookupTable(lut);
  bar->GetPen()->SetLineType(svtkPen::NO_PEN);

  chart->GetAxis(svtkAxis::LEFT)->SetTitle("A tiny range");
  chart->GetAxis(svtkAxis::BOTTOM)->SetTitle("A normal range");
  chart->GetAxis(svtkAxis::RIGHT)->SetTitle("An even tinier range");
  chart->SetBarWidthFraction(1.0f);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
