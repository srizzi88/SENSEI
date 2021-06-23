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
#include "svtkColorTransferFunction.h"
#include "svtkCompositeControlPointsItem.h"
#include "svtkCompositeTransferFunctionItem.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkLookupTable.h"
#include "svtkPiecewiseControlPointsItem.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRenderingOpenGLConfigure.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
int TestScalarsToColors(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  svtkSmartPointer<svtkChartXY> chart = svtkSmartPointer<svtkChartXY>::New();
  chart->SetTitle("Chart");
  chart->ForceAxesToBoundsOn();
  view->GetScene()->AddItem(chart);

  svtkSmartPointer<svtkLookupTable> lookupTable = svtkSmartPointer<svtkLookupTable>::New();
  lookupTable->Build();

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    svtkSmartPointer<svtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(0., 0., 1., 1., 0.3333, 0.3333, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.3333, 0.3333, 1., 1., 0.6666, 0.6666, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.6666, 0.6666, 1., 1., 1., 0., 1., 1.);

  colorTransferFunction->Build();

  svtkSmartPointer<svtkPiecewiseFunction> opacityFunction =
    svtkSmartPointer<svtkPiecewiseFunction>::New();
  opacityFunction->AddPoint(0.2, 0.);
  opacityFunction->AddPoint(0.5, 0.5);
  opacityFunction->AddPoint(1., 1.);

  svtkSmartPointer<svtkCompositeTransferFunctionItem> item3 =
    svtkSmartPointer<svtkCompositeTransferFunctionItem>::New();
  item3->SetColorTransferFunction(colorTransferFunction);
  item3->SetOpacityFunction(opacityFunction);
  item3->SetMaskAboveCurve(true);
  chart->AddPlot(item3);

  svtkSmartPointer<svtkCompositeControlPointsItem> item5 =
    svtkSmartPointer<svtkCompositeControlPointsItem>::New();
  item5->SetOpacityFunction(opacityFunction);
  item5->SetColorTransferFunction(colorTransferFunction);
  chart->AddPlot(item5);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(1);

  if (view->GetContext()->GetDevice()->IsA("svtkOpenGL2ContextDevice2D"))
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }

  return EXIT_SUCCESS;
}
