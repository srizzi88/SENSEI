/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalCacheTemporal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkContourFilter.h"
#include "svtkInformation.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTemporalDataSetCache.h"
#include "svtkTemporalFractal.h"
#include "svtkTemporalInterpolator.h"
#include "svtkThreshold.h"

class svtkTestTemporalCacheTemporalExecuteCallback : public svtkCommand
{
public:
  static svtkTestTemporalCacheTemporalExecuteCallback* New()
  {
    return new svtkTestTemporalCacheTemporalExecuteCallback;
  }

  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    // count the number of timesteps requested
    svtkTemporalFractal* f = svtkTemporalFractal::SafeDownCast(caller);
    svtkInformation* info = f->GetExecutive()->GetOutputInformation(0);
    int Length = info->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) ? 1 : 0;
    this->Count += Length;
  }

  unsigned int Count;
};

//-------------------------------------------------------------------------
int TestTemporalCacheTemporal(int, char*[])
{
  // we have to use a composite pipeline
  svtkCompositeDataPipeline* prototype = svtkCompositeDataPipeline::New();
  svtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // create temporal fractals
  svtkSmartPointer<svtkTemporalFractal> fractal = svtkSmartPointer<svtkTemporalFractal>::New();
  fractal->SetMaximumLevel(2);
  fractal->DiscreteTimeStepsOn();
  fractal->GenerateRectilinearGridsOn();
  fractal->SetAdaptiveSubdivision(0);

  svtkTestTemporalCacheTemporalExecuteCallback* executecb =
    svtkTestTemporalCacheTemporalExecuteCallback::New();
  executecb->Count = 0;
  fractal->AddObserver(svtkCommand::StartEvent, executecb);
  executecb->Delete();

  // cache the data to prevent regenerating some of it
  svtkSmartPointer<svtkTemporalDataSetCache> cache = svtkSmartPointer<svtkTemporalDataSetCache>::New();
  cache->SetInputConnection(fractal->GetOutputPort());
  cache->SetCacheSize(2);

  // interpolate if needed
  svtkSmartPointer<svtkTemporalInterpolator> interp = svtkSmartPointer<svtkTemporalInterpolator>::New();
  // interp->SetInputConnection(fractal->GetOutputPort());
  interp->SetInputConnection(cache->GetOutputPort());
  interp->SetCacheData(false);

  // cache the data coming out of the interpolator
  svtkSmartPointer<svtkTemporalDataSetCache> cache2 = svtkSmartPointer<svtkTemporalDataSetCache>::New();
  cache2->SetInputConnection(interp->GetOutputPort());
  cache2->SetCacheSize(11);

  svtkSmartPointer<svtkThreshold> contour = svtkSmartPointer<svtkThreshold>::New();
  // contour->SetInputConnection(interp->GetOutputPort());
  contour->SetInputConnection(cache2->GetOutputPort());
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

  double time = 0;
  int i;
  int j;
  for (j = 0; j < 5; ++j)
  {
    for (i = 0; i < 11; ++i)
    {
      time = i / 2.0;
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
      mapper->Modified();
      renderer->ResetCameraClippingRange();
      renWin->Render();
    }
  }

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);

  if (executecb->Count == 8)
  {
    return 0;
  }
  return 1;
}
