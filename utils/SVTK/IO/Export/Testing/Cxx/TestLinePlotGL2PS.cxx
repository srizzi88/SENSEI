/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotGL2PS.cxx

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
#include "svtkGL2PSExporter.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkPlotLine.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTestingInteractor.h"

//----------------------------------------------------------------------------
int TestLinePlotGL2PS(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
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
  svtkNew<svtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  svtkNew<svtkFloatArray> arr1;
  arr1->SetName("One");
  table->AddColumn(arr1);
  svtkNew<svtkFloatArray> arr0;
  arr0->SetName("Zero");
  table->AddColumn(arr0);
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
    table->SetValue(i, 4, 1.0);
    table->SetValue(i, 5, 0.0);
  }

  // Add multiple line plots, setting the colors etc
  svtkPlotLine* line = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line->SetMarkerStyle(svtkPlotLine::CIRCLE);
  line = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line->SetMarkerStyle(svtkPlotLine::SQUARE);
  line = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(svtkPlotLine::DIAMOND);
  line = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  line->SetInputData(table, 0, 4);
  line->SetColor(0, 255, 255, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(svtkPlotLine::CROSS);
  line = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  line->SetInputData(table, 0, 5);
  line->SetColor(255, 255, 0, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(svtkPlotLine::PLUS);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->UsePainterSettings();
  exp->CompressOff();
  exp->DrawBackgroundOn();

  std::string fileprefix = svtkTestingInteractor::TempDirectory + std::string("/TestLinePlotGL2PS");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  exp->SetFileFormatToPDF();
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
