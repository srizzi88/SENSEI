/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEllipseArcSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkActor.h>
#include <svtkEllipseArcSource.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

int TestEllipseArcSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkEllipseArcSource> source;
  source->SetCenter(0.0, 0.0, 0.0);
  source->SetRatio(0.25);
  source->SetNormal(0., 0., 1.);
  source->SetMajorRadiusVector(10, 0., 0.);
  source->SetStartAngle(20);
  source->SetSegmentAngle(250);
  source->SetResolution(80);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .6, .3);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindowInteractor->Start();
  return EXIT_SUCCESS;
}
