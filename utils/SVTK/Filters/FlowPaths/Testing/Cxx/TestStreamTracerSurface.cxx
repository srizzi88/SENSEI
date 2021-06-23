/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkActor.h"
#include "svtkArrayCalculator.h"
#include "svtkDataSetMapper.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPointSource.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamTracer.h"
#include "svtkWarpScalar.h"

int TestStreamTracerSurface(int argc, char* argv[])
{

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 100, -10, 100, 0, 0);

  svtkNew<svtkWarpScalar> warp;
  warp->SetScaleFactor(0.1);
  warp->SetInputConnection(wavelet->GetOutputPort());

  svtkNew<svtkArrayCalculator> calc;
  calc->AddScalarArrayName("RTData");
  calc->SetFunction("abs(RTData)*iHat + abs(RTData)*jHat");
  calc->SetInputConnection(warp->GetOutputPort());
  calc->Update();

  svtkNew<svtkPoints> points;
  svtkDataSet* calcData = svtkDataSet::SafeDownCast(calc->GetOutput());
  svtkIdType nLine =
    static_cast<svtkIdType>(sqrt(static_cast<double>(calcData->GetNumberOfPoints())));
  for (svtkIdType i = 0; i < nLine; i += 10)
  {
    points->InsertNextPoint(calcData->GetPoint(i * (nLine - 1) + nLine));
  }

  svtkNew<svtkPolyData> pointsPolydata;
  pointsPolydata->SetPoints(points);

  svtkNew<svtkStreamTracer> stream;
  stream->SurfaceStreamlinesOn();
  stream->SetMaximumPropagation(210);
  stream->SetIntegrationDirection(svtkStreamTracer::BOTH);
  stream->SetInputConnection(calc->GetOutputPort());
  stream->SetSourceData(pointsPolydata);

  svtkNew<svtkDataSetMapper> streamMapper;
  streamMapper->SetInputConnection(stream->GetOutputPort());
  streamMapper->ScalarVisibilityOff();

  svtkNew<svtkDataSetMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(calc->GetOutputPort());

  svtkNew<svtkActor> streamActor;
  streamActor->SetMapper(streamMapper);
  streamActor->GetProperty()->SetColor(1, 1, 1);
  streamActor->GetProperty()->SetLineWidth(4.);
  streamActor->SetPosition(0, 0, 1);

  svtkNew<svtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->SetRepresentationToSurface();

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(surfaceActor);
  renderer->AddActor(streamActor);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
