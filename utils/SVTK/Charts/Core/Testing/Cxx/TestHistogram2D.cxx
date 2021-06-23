/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHistogram2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartHistogram2D.h"
#include "svtkColorTransferFunction.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlotHistogram2D.h"
#include "svtkPlotLine.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"
#include "svtkVector.h"

//----------------------------------------------------------------------------
int TestHistogram2D(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  int size = 400;
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(size, size);

  // Define a chart
  svtkNew<svtkChartHistogram2D> chart;
  view->GetScene()->AddItem(chart);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  // Add only a plot without an image data
  svtkNew<svtkTable> table;
  svtkNew<svtkDoubleArray> X;
  X->SetName("X");
  X->SetNumberOfComponents(1);
  X->SetNumberOfTuples(size);
  svtkNew<svtkDoubleArray> Y;
  Y->SetName("Y");
  Y->SetNumberOfComponents(1);
  Y->SetNumberOfTuples(size);

  for (int i = 0; i < size; i++)
  {
    X->SetTuple1(i, i);
    Y->SetTuple1(i, i);
  }
  table->AddColumn(X);
  table->AddColumn(Y);

  svtkPlotLine* plot = svtkPlotLine::SafeDownCast(chart->AddPlot(svtkChart::LINE));
  plot->SetInputData(table, 0, 1);
  plot->SetColor(1.0, 0.0, 0.0);
  plot->SetWidth(5);

  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  svtkVector2i mousePosition;

  // Test interactions when there is only a plot and no image data
  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  int x = chart->GetPoint1()[0] + 4;
  int y = chart->GetPoint1()[1] + 10;
  mousePosition.Set(x, y);
  mouseEvent.SetScreenPos(mousePosition);
  mouseEvent.SetPos(svtkVector2f(0.0, 0.0));
  chart->MouseButtonPressEvent(mouseEvent);
  chart->MouseButtonReleaseEvent(mouseEvent);

  // Remove the plot and add an image data
  svtkIdType id = chart->GetPlotIndex(plot);
  chart->RemovePlot(id);

  svtkNew<svtkImageData> data;
  data->SetExtent(0, size - 1, 0, size - 1, 0, 0);
  data->AllocateScalars(SVTK_DOUBLE, 1);

  data->SetOrigin(100.0, 0.0, 0.0);
  data->SetSpacing(2.0, 1.0, 1.0);

  double* dPtr = static_cast<double*>(data->GetScalarPointer(0, 0, 0));
  for (int i = 0; i < size; ++i)
  {
    for (int j = 0; j < size; ++j)
    {
      dPtr[i * size + j] = sin(svtkMath::RadiansFromDegrees(double(2 * i))) *
        cos(svtkMath::RadiansFromDegrees(double(j)));
    }
  }
  chart->SetInputData(data);

  svtkNew<svtkColorTransferFunction> transferFunction;
  transferFunction->AddHSVSegment(0.0, 0.0, 1.0, 1.0, 0.3333, 0.3333, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.3333, 0.3333, 1.0, 1.0, 0.6666, 0.6666, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.6666, 0.6666, 1.0, 1.0, 1.0, 0.2, 1.0, 0.3);
  transferFunction->Build();
  chart->SetTransferFunction(transferFunction);

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
