/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipOrientedPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkBoxClipDataSet.h>
#include <svtkLookupTable.h>
#include <svtkSmartPointer.h>

#include <svtkUnstructuredGrid.h>
#include <svtkUnstructuredGridReader.h>

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkDataSetMapper.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include "svtkTestUtilities.h"

int BoxClipOrientedPointData(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/hexa.svtk");

  // Read the data
  svtkSmartPointer<svtkUnstructuredGridReader> reader =
    svtkSmartPointer<svtkUnstructuredGridReader>::New();
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  double bounds[6];
  reader->GetOutput()->GetBounds(bounds);

  double range[2];
  reader->GetOutput()->GetScalarRange(range);

  double minBoxPoint[3];
  double maxBoxPoint[3];
  minBoxPoint[0] = (bounds[1] - bounds[0]) / 2.0 + bounds[0];
  minBoxPoint[1] = (bounds[3] - bounds[2]) / 2.0 + bounds[2];
  minBoxPoint[2] = (bounds[5] - bounds[4]) / 2.0 + bounds[4];
  maxBoxPoint[0] = bounds[1];
  maxBoxPoint[1] = bounds[3];
  maxBoxPoint[2] = bounds[5];

  svtkSmartPointer<svtkBoxClipDataSet> boxClip = svtkSmartPointer<svtkBoxClipDataSet>::New();
  boxClip->SetInputConnection(reader->GetOutputPort());
  boxClip->GenerateClippedOutputOn();

  const double minusx[] = { -1.0, 0.0, 0.0 };
  const double minusy[] = { 0.0, -1.0, 0.0 };
  const double minusz[] = { 0.0, 0.0, -1.0 };
  const double plusx[] = { 1.0, 0.0, 0.0 };
  const double plusy[] = { 0.0, 1.0, 0.0 };
  const double plusz[] = { 0.0, 0.0, 1.0 };
  boxClip->SetBoxClip(minusx, minBoxPoint, minusy, minBoxPoint, minusz, minBoxPoint, plusx,
    maxBoxPoint, plusy, maxBoxPoint, plusz, maxBoxPoint);

  // Define a lut
  svtkSmartPointer<svtkLookupTable> lut1 = svtkSmartPointer<svtkLookupTable>::New();
  lut1->SetHueRange(.667, 0);
  lut1->Build();

  svtkSmartPointer<svtkDataSetSurfaceFilter> surfaceIn =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surfaceIn->SetInputConnection(boxClip->GetOutputPort(0));

  svtkSmartPointer<svtkDataSetMapper> mapperIn = svtkSmartPointer<svtkDataSetMapper>::New();
  mapperIn->SetInputConnection(surfaceIn->GetOutputPort());
  mapperIn->SetScalarRange(reader->GetOutput()->GetScalarRange());
  mapperIn->SetLookupTable(lut1);

  svtkSmartPointer<svtkActor> actorIn = svtkSmartPointer<svtkActor>::New();
  actorIn->SetMapper(mapperIn);

  svtkSmartPointer<svtkDataSetSurfaceFilter> surfaceOut =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surfaceOut->SetInputConnection(boxClip->GetOutputPort(1));

  svtkSmartPointer<svtkDataSetMapper> mapperOut = svtkSmartPointer<svtkDataSetMapper>::New();
  mapperOut->SetInputConnection(surfaceOut->GetOutputPort());
  mapperOut->SetScalarRange(reader->GetOutput()->GetScalarRange());
  mapperOut->SetLookupTable(lut1);

  svtkSmartPointer<svtkActor> actorOut = svtkSmartPointer<svtkActor>::New();
  actorOut->SetMapper(mapperOut);
  actorOut->AddPosition(-.5 * (maxBoxPoint[0] - minBoxPoint[0]),
    -.5 * (maxBoxPoint[1] - minBoxPoint[1]), -.5 * (maxBoxPoint[2] - minBoxPoint[2]));

  // Create a renderer, render window, and interactor
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->SetBackground(.5, .5, .5);
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->AddActor(actorIn);
  renderer->AddActor(actorOut);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(120);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->Dolly(1.0);
  renderer->ResetCameraClippingRange();

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
