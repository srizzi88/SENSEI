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

#include "svtkChartPie.h"
#include "svtkColorSeries.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkPlotPie.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#define NUM_ITEMS (5)
static int data[] = { 77938, 9109, 2070, 12806, 19514 };
// static int data[] = {200,200,200,200,200};
static const char* labels[] = { "Books", "New and Popular", "Periodical", "Audiobook", "Video" };

//----------------------------------------------------------------------------
int TestPieChart(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  svtkNew<svtkChartPie> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;

  svtkNew<svtkIntArray> arrData;
  svtkNew<svtkStringArray> labelArray;

  arrData->SetName("2008 Circulation");
  for (int i = 0; i < NUM_ITEMS; i++)
  {
    arrData->InsertNextValue(data[i]);
    labelArray->InsertNextValue(labels[i]);
  }

  table->AddColumn(arrData);

  // Create a color series to use with our stacks.
  svtkNew<svtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(svtkColorSeries::WARM);

  // Add multiple line plots, setting the colors etc
  svtkPlotPie* pie = svtkPlotPie::SafeDownCast(chart->AddPlot(0));
  pie->SetColorSeries(colorSeries);
  pie->SetInputData(table);
  pie->SetInputArray(0, "2008 Circulation");
  pie->SetLabels(labelArray);

  chart->SetShowLegend(true);

  chart->SetTitle("Circulation 2008");

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
