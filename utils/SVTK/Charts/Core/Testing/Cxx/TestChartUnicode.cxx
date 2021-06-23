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
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"

#include <string>

//----------------------------------------------------------------------------
int TestChartUnicode(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  std::string fontFile(argv[1]);

  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Exercise the support for extended characters using UTF8 encoded strings.
  chart->GetTitleProperties()->SetFontFamily(SVTK_FONT_FILE);
  chart->GetTitleProperties()->SetFontFile(fontFile.c_str());
  chart->SetTitle("\xcf\x85\xcf\x84\xce\xba");

  svtkAxis* axis1 = chart->GetAxis(0);
  axis1->GetTitleProperties()->SetFontFamily(SVTK_FONT_FILE);
  axis1->GetTitleProperties()->SetFontFile(fontFile.c_str());
  axis1->SetTitle("\xcf\x87(m)");

  svtkAxis* axis2 = chart->GetAxis(1);
  axis2->GetTitleProperties()->SetFontFamily(SVTK_FONT_FILE);
  axis2->GetTitleProperties()->SetFontFile(fontFile.c_str());
  axis2->SetTitle("\xcf\x80\xcf\x86");

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + sin(i * (inc - svtkMath::Pi())));
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(42, 55, 69, 255);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
