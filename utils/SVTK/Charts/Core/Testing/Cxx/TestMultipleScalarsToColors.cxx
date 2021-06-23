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

#include "svtkAxis.h"
#include "svtkChartXY.h"
#include "svtkColorTransferFunction.h"
#include "svtkColorTransferFunctionItem.h"
#include "svtkCompositeTransferFunctionItem.h"
#include "svtkContext2D.h"
#include "svtkContextActor.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkLookupTable.h"
#include "svtkLookupTableItem.h"
#include "svtkPiecewiseControlPointsItem.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPiecewiseFunctionItem.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRenderingOpenGLConfigure.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int TestMultipleScalarsToColors(int, char*[])
{
  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->SetSize(800, 900);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  // setup the 5 charts view ports
  double viewports[20] = { 0.0, 0.0, 0.3, 0.5, 0.3, 0.0, 1.0, 0.5, 0.0, 0.33, 0.5, 0.66, 0.5, 0.33,
    1.0, 0.66, 0.0, 0.66, 1.0, 1.0 };

  // Lookup Table
  svtkSmartPointer<svtkLookupTable> lookupTable = svtkSmartPointer<svtkLookupTable>::New();
  lookupTable->SetAlpha(0.5);
  lookupTable->Build();
  // Color transfer function
  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    svtkSmartPointer<svtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(0., 0., 1., 1., 0.3333, 0.3333, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.3333, 0.3333, 1., 1., 0.6666, 0.6666, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.6666, 0.6666, 1., 1., 1., 0., 1., 1.);
  colorTransferFunction->Build();
  // Opacity function
  svtkSmartPointer<svtkPiecewiseFunction> opacityFunction =
    svtkSmartPointer<svtkPiecewiseFunction>::New();
  opacityFunction->AddPoint(0., 0.);
  opacityFunction->AddPoint(1., 1.);
  // Histogram table
  svtkNew<svtkTable> histoTable;
  svtkNew<svtkDoubleArray> binArray;
  binArray->SetName("bins");
  histoTable->AddColumn(binArray);
  svtkNew<svtkDoubleArray> valueArray;
  valueArray->SetName("values");
  histoTable->AddColumn(valueArray);

  histoTable->SetNumberOfRows(3);
  histoTable->SetValue(0, 0, 0.25);
  histoTable->SetValue(0, 1, 2);
  histoTable->SetValue(1, 0, 0.5);
  histoTable->SetValue(1, 1, 5);
  histoTable->SetValue(2, 0, 0.75);
  histoTable->SetValue(2, 1, 8);

  for (int i = 0; i < 5; ++i)
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

    switch (i)
    {
      case 0:
      {
        svtkSmartPointer<svtkLookupTableItem> item = svtkSmartPointer<svtkLookupTableItem>::New();
        item->SetLookupTable(lookupTable);
        chart->AddPlot(item);
        chart->SetAutoAxes(false);
        chart->GetAxis(0)->SetVisible(false);
        chart->GetAxis(1)->SetVisible(false);
        chart->SetTitle("svtkLookupTable");
        break;
      }
      case 1:
      {
        svtkSmartPointer<svtkColorTransferFunctionItem> item =
          svtkSmartPointer<svtkColorTransferFunctionItem>::New();
        item->SetColorTransferFunction(colorTransferFunction);
        // opacity is added on the item, not on the transfer function
        item->SetOpacity(0.8);
        chart->AddPlot(item);
        chart->SetTitle("svtkColorTransferFunction");
        break;
      }
      case 2:
      {
        svtkSmartPointer<svtkCompositeTransferFunctionItem> item =
          svtkSmartPointer<svtkCompositeTransferFunctionItem>::New();
        item->SetColorTransferFunction(colorTransferFunction);
        item->SetOpacityFunction(opacityFunction);
        item->SetMaskAboveCurve(true);
        chart->AddPlot(item);
        chart->SetTitle("svtkColorTransferFunction + svtkPiecewiseFunction");
        break;
      }
      case 3:
      {
        svtkSmartPointer<svtkPiecewiseFunctionItem> item =
          svtkSmartPointer<svtkPiecewiseFunctionItem>::New();
        item->SetPiecewiseFunction(opacityFunction);
        item->SetColor(1., 0, 0);
        chart->AddPlot(item);
        svtkSmartPointer<svtkPiecewiseControlPointsItem> controlPointsItem =
          svtkSmartPointer<svtkPiecewiseControlPointsItem>::New();
        controlPointsItem->SetPiecewiseFunction(opacityFunction);
        chart->AddPlot(controlPointsItem);
        chart->SetTitle("svtkPiecewiseFunction");
        break;
      }
      case 4:
      {
        svtkNew<svtkCompositeTransferFunctionItem> item;
        item->SetColorTransferFunction(colorTransferFunction);
        item->SetOpacityFunction(opacityFunction);
        item->SetHistogramTable(histoTable);
        item->SetMaskAboveCurve(true);
        chart->AddPlot(item);
        chart->SetTitle("histogramTable");
        break;
      }
      default:
        break;
    }
  }

  renwin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
