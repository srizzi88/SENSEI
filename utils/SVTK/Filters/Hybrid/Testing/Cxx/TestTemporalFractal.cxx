/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalFractal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkActor.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkContourFilter.h"
#include "svtkInformation.h"
#include "svtkPolyDataMapper.h"
#include "svtkSmartPointer.h"
#include "svtkTemporalFractal.h"
#include "svtkTemporalInterpolator.h"
#include "svtkTemporalShiftScale.h"
#include "svtkThreshold.h"

//-------------------------------------------------------------------------
int TestTemporalFractal(int argc, char* argv[])
{
  // we have to use a composite pipeline
  svtkCompositeDataPipeline* prototype = svtkCompositeDataPipeline::New();
  svtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // create temporal fractals
  svtkSmartPointer<svtkTemporalFractal> fractal = svtkSmartPointer<svtkTemporalFractal>::New();
  fractal->SetMaximumLevel(3);
  fractal->DiscreteTimeStepsOn();
  fractal->GenerateRectilinearGridsOn();
  //  fractal->SetAdaptiveSubdivision(0);

  // shift and scale the time range to that it run from -0.5 to 0.5
  svtkSmartPointer<svtkTemporalShiftScale> tempss = svtkSmartPointer<svtkTemporalShiftScale>::New();
  tempss->SetScale(0.1);
  tempss->SetPostShift(-0.5);
  tempss->SetInputConnection(fractal->GetOutputPort());

  // interpolate if needed
  svtkSmartPointer<svtkTemporalInterpolator> interp = svtkSmartPointer<svtkTemporalInterpolator>::New();
  interp->SetInputConnection(tempss->GetOutputPort());

  svtkSmartPointer<svtkThreshold> contour = svtkSmartPointer<svtkThreshold>::New();
  contour->SetInputConnection(interp->GetOutputPort());
  contour->ThresholdByUpper(0.5);

  svtkSmartPointer<svtkCompositeDataGeometryFilter> geom =
    svtkSmartPointer<svtkCompositeDataGeometryFilter>::New();
  geom->SetInputConnection(contour->GetOutputPort());

  // map them
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(geom->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);

  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);
  iren->SetRenderWindow(renWin);

  // ask for some specific data points
  svtkInformation* info = geom->GetOutputInformation(0);
  geom->UpdateInformation();
  double time = -0.6;
  int i;
  for (i = 0; i < 10; ++i)
  {
    time = i / 25.0 - 0.5;
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
    mapper->Modified();
    renderer->ResetCameraClippingRange();
    renWin->Render();
  }

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);
  return !retVal;
}
