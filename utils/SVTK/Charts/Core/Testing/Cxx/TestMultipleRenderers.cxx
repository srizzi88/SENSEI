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
#include "svtkCubeSource.h"
#include "svtkFloatArray.h"
#include "svtkPlot.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int TestMultipleRenderers(int, char*[])
{

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetSize(800, 640);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  SVTK_CREATE(svtkRenderer, ren3d);
  ren3d->SetBackground(0.0, 0.0, 0.0);
  renwin->AddRenderer(ren3d);

  // Cube Source 1
  SVTK_CREATE(svtkCubeSource, cube);
  SVTK_CREATE(svtkPolyDataMapper, cubeMapper);
  SVTK_CREATE(svtkActor, cubeActor);

  cubeMapper->SetInputConnection(cube->GetOutputPort());
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  ren3d->AddActor(cubeActor);
  cubeActor->GetProperty()->SetRepresentationToSurface();

  // setup the 2d chart
  SVTK_CREATE(svtkRenderer, ren2d);
  ren2d->SetBackground(1.0, 1.0, 1.0);
  renwin->AddRenderer(ren2d);

  SVTK_CREATE(svtkChartXY, chart);
  SVTK_CREATE(svtkContextScene, chartScene);
  SVTK_CREATE(svtkContextActor, chartActor);

  chartScene->AddItem(chart);
  chartActor->SetScene(chartScene);

  // both needed
  ren2d->AddActor(chartActor);
  chartScene->SetRenderer(ren2d);

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

  ren3d->SetViewport(0, 0, 1, 0.5);
  ren2d->SetViewport(0, 0.5, 1, 1);

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
