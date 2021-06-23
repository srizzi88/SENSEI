/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBarGraphSelection.cxx

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
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

// Monthly circulation data
static int data_2008[] = { 10822, 10941, 9979, 10370, 9460, 11228, 15093, 12231, 10160, 9816, 9384,
  7892 };
static int data_2009[] = { 9058, 9474, 9979, 9408, 8900, 11569, 14688, 12231, 10294, 9585, 8957,
  8590 };
static int data_2010[] = { 9058, 10941, 9979, 10270, 8900, 11228, 14688, 12231, 10160, 9585, 9384,
  8590 };

//----------------------------------------------------------------------------
int TestBarGraphSelection(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;

  svtkNew<svtkIntArray> arrMonth;
  arrMonth->SetName("Month");
  table->AddColumn(arrMonth);

  svtkNew<svtkIntArray> arr2008;
  arr2008->SetName("2008");
  table->AddColumn(arr2008);

  svtkNew<svtkIntArray> arr2009;
  arr2009->SetName("2009");
  table->AddColumn(arr2009);

  svtkNew<svtkIntArray> arr2010;
  arr2010->SetName("2010");
  table->AddColumn(arr2010);

  table->SetNumberOfRows(12);
  for (int i = 0; i < 12; i++)
  {
    table->SetValue(i, 0, i + 1);
    table->SetValue(i, 1, data_2008[i]);
    table->SetValue(i, 2, data_2009[i]);
    table->SetValue(i, 3, data_2010[i]);
  }

  // Build a selection object.
  svtkNew<svtkIdTypeArray> selection;
  selection->InsertNextValue(1);
  selection->InsertNextValue(3);
  selection->InsertNextValue(5);

  // Add multiple bar plots, setting the colors etc
  svtkPlot* plot = nullptr;

  plot = chart->AddPlot(svtkChart::BAR);
  plot->SetInputData(table, 0, 1);
  plot->SetColor(0, 255, 0, 255);

  plot->SetSelection(selection);

  plot = chart->AddPlot(svtkChart::BAR);
  plot->SetInputData(table, 0, 2);
  plot->SetColor(255, 0, 0, 255);

  plot = chart->AddPlot(svtkChart::BAR);
  plot->SetInputData(table, 0, 3);
  plot->SetColor(0, 0, 255, 255);

  plot->SetSelection(selection);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
