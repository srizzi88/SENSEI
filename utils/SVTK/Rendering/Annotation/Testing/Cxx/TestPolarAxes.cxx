/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolarAxes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "svtkBYUReader.h"
#include "svtkCamera.h"
#include "svtkLODActor.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPolarAxesActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"

#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestPolarAxes(int argc, char* argv[])
{
  svtkNew<svtkBYUReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  reader->SetGeometryFileName(fname);
  delete[] fname;

  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkPolyDataMapper> readerMapper;
  readerMapper->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkLODActor> readerActor;
  readerActor->SetMapper(readerMapper);
  readerActor->GetProperty()->SetDiffuseColor(.5, .8, .3);

  svtkNew<svtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  svtkNew<svtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(1., 1., 1.);

  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(0., .5, 0.);
  camera->SetPosition(5., 6., 14.);

  svtkNew<svtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(7., 7., 4.);

  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->AddLight(light);

  // Update normals in order to get correct bounds for polar axes
  normals->Update();

  svtkNew<svtkPolarAxesActor> polaxes;
  polaxes->SetBounds(normals->GetOutput()->GetBounds());
  polaxes->SetPole(.5, 1., 3.);
  polaxes->SetMaximumRadius(3.);
  polaxes->SetMinimumAngle(-60.);
  polaxes->SetMaximumAngle(210.);
  polaxes->SetRequestedNumberOfRadialAxes(10);
  polaxes->SetCamera(renderer->GetActiveCamera());
  polaxes->SetPolarLabelFormat("%6.1f");
  polaxes->GetLastRadialAxisProperty()->SetColor(.0, .0, 1.);
  polaxes->GetSecondaryRadialAxesProperty()->SetColor(.0, .0, 1.);
  polaxes->GetPolarArcsProperty()->SetColor(1., .0, 0.);
  polaxes->GetSecondaryPolarArcsProperty()->SetColor(1., 1., 1.);
  polaxes->GetPolarAxisProperty()->SetColor(.2, .2, .2);
  polaxes->GetPolarAxisTitleTextProperty()->SetColor(1, 1, 0);
  polaxes->GetPolarAxisLabelTextProperty()->SetColor(0, 1, 1);
  polaxes->GetSecondaryRadialAxesTextProperty()->SetColor(1, 0, 1);
  polaxes->SetNumberOfPolarAxisTicks(9);
  polaxes->SetAutoSubdividePolarAxis(false);
  polaxes->SetScreenSize(9.0);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  renWin->SetWindowName("SVTK - Polar Axes");
  renWin->SetSize(600, 600);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->SetBackground(.8, .8, .8);
  renderer->AddViewProp(readerActor);
  renderer->AddViewProp(outlineActor);
  renderer->AddViewProp(polaxes);
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
