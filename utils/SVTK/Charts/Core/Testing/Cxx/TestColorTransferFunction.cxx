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

#include "svtkChartXY.h"
#include "svtkColorTransferControlPointsItem.h"
#include "svtkColorTransferFunction.h"
#include "svtkColorTransferFunctionItem.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkRangeHandlesItem.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
int TestColorTransferFunction(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  svtkSmartPointer<svtkChartXY> chart = svtkSmartPointer<svtkChartXY>::New();
  chart->SetTitle("Chart");
  view->GetScene()->AddItem(chart);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    svtkSmartPointer<svtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(50., 0., 1., 1., 85., 0.3333, 1., 1.);
  colorTransferFunction->AddHSVSegment(85., 0.3333, 1., 1., 170., 0.6666, 1., 1.);
  colorTransferFunction->AddHSVSegment(170., 0.6666, 1., 1., 200., 0., 1., 1.);

  colorTransferFunction->Build();

  svtkSmartPointer<svtkColorTransferFunctionItem> colorTransferItem =
    svtkSmartPointer<svtkColorTransferFunctionItem>::New();
  colorTransferItem->SetColorTransferFunction(colorTransferFunction);
  chart->AddPlot(colorTransferItem);

  svtkSmartPointer<svtkColorTransferControlPointsItem> controlPointsItem =
    svtkSmartPointer<svtkColorTransferControlPointsItem>::New();
  controlPointsItem->SetColorTransferFunction(colorTransferFunction);
  controlPointsItem->SetUserBounds(0., 255., 0., 1.);
  chart->AddPlot(controlPointsItem);

  svtkNew<svtkRangeHandlesItem> rangeHandlesItem;
  rangeHandlesItem->SetColorTransferFunction(colorTransferFunction);
  // Use very large handles to make sure the test fails if the handles fail to render
  rangeHandlesItem->SetHandleWidth(40.0);
  chart->AddPlot(rangeHandlesItem);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(1);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
