/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelCoordinatesDouble.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartParallelCoordinates.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestParallelCoordinatesDouble(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(600, 400);
  svtkNew<svtkChartParallelCoordinates> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkDoubleArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  svtkNew<svtkDoubleArray> arrC;
  arrC->SetName("cosine");
  table->AddColumn(arrC);
  svtkNew<svtkDoubleArray> arrS;
  arrS->SetName("sine");
  table->AddColumn(arrS);
  svtkNew<svtkDoubleArray> arrS2;
  arrS2->SetName("tangent");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 200;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) * 1.0e-82);
    table->SetValue(i, 2, sin(i * inc) * 1.0e+89);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
  }

  chart->GetPlot(0)->SetInputData(table);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
