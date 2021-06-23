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

#include "svtkActor.h"
#include "svtkChartXY.h"
#include "svtkContextActor.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int TestMultipleChartRenderers(int, char*[])
{

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->SetSize(800, 640);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  // setup the 4charts view ports
  double viewports[16] = { 0.0, 0.0, 0.3, 0.5, 0.3, 0.0, 1.0, 0.5, 0.0, 0.5, 0.5, 1.0, 0.5, 0.5,
    1.0, 1.0 };

  for (int i = 0; i < 4; ++i)
  {
    SVTK_CREATE(svtkRenderer, ren);
    ren->SetBackground(1.0, 1.0, 1.0);
    ren->SetViewport(&viewports[i * 4]);
    renwin->AddRenderer(ren);

    SVTK_CREATE(svtkChartXY, chart);
    SVTK_CREATE(svtkContextScene, chartScene);
    SVTK_CREATE(svtkContextActor, chartActor);

    chartScene->AddItem(chart);
    chartActor->SetScene(chartScene);

    // both needed
    ren->AddActor(chartActor);
    chartScene->SetRenderer(ren);

    // Create a table with some points in it...
    SVTK_CREATE(svtkTable, table);
    SVTK_CREATE(svtkFloatArray, arrX);
    arrX->SetName("X Axis");
    table->AddColumn(arrX);
    SVTK_CREATE(svtkFloatArray, arrC);
    arrC->SetName("Cosine");
    table->AddColumn(arrC);
    SVTK_CREATE(svtkFloatArray, arrS);
    arrS->SetName("Sine");
    table->AddColumn(arrS);
    SVTK_CREATE(svtkFloatArray, arrS2);
    arrS2->SetName("Sine2");
    table->AddColumn(arrS2);
    // Test charting with a few more points...
    int numPoints = 69;
    float inc = 7.5 / (numPoints - 1);
    table->SetNumberOfRows(numPoints);
    for (int j = 0; j < numPoints; ++j)
    {
      table->SetValue(j, 0, j * inc);
      table->SetValue(j, 1, cos(j * inc) + 0.0);
      table->SetValue(j, 2, sin(j * inc) + 0.0);
      table->SetValue(j, 3, sin(j * inc) + 0.5);
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
  }

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
