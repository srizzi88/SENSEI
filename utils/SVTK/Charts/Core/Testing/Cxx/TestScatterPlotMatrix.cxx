/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChart.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkScatterPlotMatrix.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrix(int, char*[])
{
  // Set up a 2D scene, add a chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  svtkNew<svtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrC;
  arrC->SetName("cos(x)");
  table->AddColumn(arrC);
  svtkNew<svtkFloatArray> arrS;
  arrS->SetName("sin(x)");
  table->AddColumn(arrS);
  svtkNew<svtkFloatArray> arrS2;
  arrS2->SetName("sin(x + 0.5)");
  table->AddColumn(arrS2);
  svtkNew<svtkFloatArray> tangent;
  tangent->SetName("tan(x)");
  table->AddColumn(tangent);
  // Test the chart scatter plot matrix
  int numPoints = 100;
  float inc = 4.0 * svtkMath::Pi() / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(table);

  matrix->SetNumberOfBins(7);

  view->Render();
  matrix->GetMainChart()->SetActionToButton(
    svtkChart::SELECT_POLYGON, svtkContextMouseEvent::RIGHT_BUTTON);

  // Test animation by releasing a right click on subchart (1,2)
  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  svtkVector2f pos;

  mouseEvent.SetButton(svtkContextMouseEvent::RIGHT_BUTTON);
  pos.Set(245, 301);
  mouseEvent.SetPos(pos);
  matrix->MouseButtonReleaseEvent(mouseEvent);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
