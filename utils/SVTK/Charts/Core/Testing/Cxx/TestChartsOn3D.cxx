/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartsOn3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkChartXY.h"
#include "svtkContextActor.h"
#include "svtkContextScene.h"
#include "svtkCubeSource.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkPlotPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestChartsOn3D(int, char*[])
{

  svtkNew<svtkRenderWindow> renwin;
  renwin->SetMultiSamples(4);
  renwin->SetSize(600, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.8, 0.8, 0.8);
  renwin->AddRenderer(renderer);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetPosition(1.0, 1.0, -4.0);
  renderer->GetActiveCamera()->Azimuth(40);

  // Cube Source 1
  svtkNew<svtkCubeSource> cube;
  svtkNew<svtkPolyDataMapper> cubeMapper;
  svtkNew<svtkActor> cubeActor;

  cubeMapper->SetInputConnection(cube->GetOutputPort());
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  renderer->AddActor(cubeActor);
  cubeActor->GetProperty()->SetRepresentationToSurface();

  // Now the chart
  svtkNew<svtkChartXY> chart;
  svtkNew<svtkContextScene> chartScene;
  svtkNew<svtkContextActor> chartActor;

  chart->SetAutoSize(false);
  chart->SetSize(svtkRectf(0.0, 0.0, 300, 200));

  chartScene->AddItem(chart);
  chartActor->SetScene(chartScene);

  // both needed
  renderer->AddActor(chartActor);
  chartScene->SetRenderer(renderer);

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
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
  }

  // Add multiple line plots, setting the colors etc
  svtkPlot* points = chart->AddPlot(svtkChart::POINTS);
  points->SetInputData(table, 0, 1);
  points->SetColor(0, 0, 0, 255);
  points->SetWidth(1.0);
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

  renwin->SetMultiSamples(0);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
