/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that caching of time varying data works as expected.
// In particular if the svtkOSPRayCache is working, repeated passes through
// an animation should be much faster than the first because all of the
// ospray data structures are reused.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCutter.h"
#include "svtkInformation.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPlane.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkResampleToImage.h"
#include "svtkSmartPointer.h"
#include "svtkSmartVolumeMapper.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTemporalDataSetCache.h"
#include "svtkTemporalFractal.h"
#include "svtkTimerLog.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestOSPRayCache(int argc, char* argv[])
{
  auto iren = svtkSmartPointer<svtkRenderWindowInteractor>::New();
  auto renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  auto renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(400, 400);

  auto ospray = svtkSmartPointer<svtkOSPRayPass>::New();

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  renderer->SetPass(ospray);

  // a well behaved time varying data source
  auto fractal = svtkSmartPointer<svtkTemporalFractal>::New();
  fractal->SetMaximumLevel(4);
  fractal->DiscreteTimeStepsOn();
  fractal->GenerateRectilinearGridsOff();
  fractal->SetAdaptiveSubdivision(1);
  fractal->TwoDimensionalOff();

  // a slice to test geometry caching
  auto plane = svtkSmartPointer<svtkPlane>::New();
  plane->SetOrigin(0, 0, 0.25);
  plane->SetNormal(0, 0, 1);
  auto cutter = svtkSmartPointer<svtkCutter>::New();
  cutter->SetCutFunction(plane);
  cutter->SetInputConnection(fractal->GetOutputPort());
  auto geom = svtkSmartPointer<svtkCompositeDataGeometryFilter>::New();
  geom->SetInputConnection(cutter->GetOutputPort());

  // exercise SVTK's filter caching too
  auto tcache1 = svtkSmartPointer<svtkTemporalDataSetCache>::New();
  tcache1->SetInputConnection(geom->GetOutputPort());
  tcache1->SetCacheSize(11);

  // draw the slice
  auto mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(tcache1->GetOutputPort());
  auto actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  // a resample to test volume caching
  auto resample = svtkSmartPointer<svtkResampleToImage>::New();
  resample->SetInputConnection(fractal->GetOutputPort());
  resample->SetSamplingDimensions(50, 50, 50);

  // exercise SVTK's filter caching too
  auto tcache2 = svtkSmartPointer<svtkTemporalDataSetCache>::New();
  tcache2->SetInputConnection(resample->GetOutputPort());
  tcache2->SetCacheSize(11);

  // draw the volume
  auto volmap = svtkSmartPointer<svtkSmartVolumeMapper>::New();
  volmap->SetInputConnection(tcache2->GetOutputPort());
  volmap->SetScalarModeToUsePointFieldData();
  volmap->SelectScalarArray("Fractal Volume Fraction");
  auto volprop = svtkSmartPointer<svtkVolumeProperty>::New();
  auto compositeOpacity = svtkSmartPointer<svtkPiecewiseFunction>::New();
  compositeOpacity->AddPoint(0.0, 0.0);
  compositeOpacity->AddPoint(3.0, 1.0);
  volprop->SetScalarOpacity(compositeOpacity);
  auto color = svtkSmartPointer<svtkColorTransferFunction>::New();
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(6.0, 1.0, 1.0, 1.0);
  volprop->SetColor(color);
  auto vol = svtkSmartPointer<svtkVolume>::New();
  vol->SetMapper(volmap);
  vol->SetProperty(volprop);
  renderer->AddViewProp(vol);

  // make the camera sensible
  auto cam = renderer->GetActiveCamera();
  cam->SetPosition(-.37, 0, 8);
  cam->SetFocalPoint(-.37, 0, 0);
  cam->SetViewUp(1, 0, 0);
  cam->Azimuth(-35);

  // now set up the animation over time
  auto info1 = tcache1->GetOutputInformation(0);
  tcache1->UpdateInformation();
  double* tsteps = info1->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int ntsteps = info1->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  auto info2 = tcache2->GetOutputInformation(0);
  tcache2->UpdateInformation();

  // the thing we are trying to test, ospray interface's caching
  svtkOSPRayRendererNode::SetTimeCacheSize(11, renderer);

  // first pass, expected to be comparatively slow
  auto timer = svtkSmartPointer<svtkTimerLog>::New();
  timer->StartTimer();
  for (int i = 0; i < ntsteps; i += 5)
  {
    double updateTime = tsteps[i];
    cout << "t=" << updateTime << endl;

    info1->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    info2->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    svtkOSPRayRendererNode::SetViewTime(updateTime, renderer);
    renWin->Render();
  }
  timer->StopTimer();
  double etime0 = timer->GetElapsedTime();
  cout << "Elapsed time first renders " << etime0 << endl;

  // subsequent passes, expected to be faster
  timer->StartTimer();
  for (int j = 0; j < 5; ++j)
  {
    for (int i = 0; i < ntsteps; i += 5)
    {
      double updateTime = tsteps[i];
      cout << "t=" << updateTime << endl;

      info1->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
      info2->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
      svtkOSPRayRendererNode::SetViewTime(updateTime, renderer);
      renWin->Render();
    }
  }
  timer->StopTimer();
  double etime1 = timer->GetElapsedTime();
  cout << "Elapsed time for 5 cached render loops " << etime1 << endl;

  if (etime1 > etime0 * 3)
  {
    cerr << "Test failed, 5 rerenders are expected to be faster." << endl;
    cerr << "first render " << etime0 << " vs " << etime1 << " for 5x rerender" << endl;
    return 1;
  }
  iren->Start();

  return 0;
}
