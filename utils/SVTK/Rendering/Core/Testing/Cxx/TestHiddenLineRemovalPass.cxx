/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkExodusIIReader.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTestUtilities.h"

int TestHiddenLineRemovalPass(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renderer->UseHiddenLineRemovalOn();
  renWin->AddRenderer(renderer);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  svtkNew<svtkExodusIIReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  svtkNew<svtkCompositeDataGeometryFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(geomFilter->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1., 0., 0.);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);

  // Workaround a rendering bug. See gitlab issue #16816.
  actor->GetProperty()->LightingOff();

  renWin->SetSize(500, 500);
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->SetBackground2(.3, .1, .2);
  renderer->GradientBackgroundOn();
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetPosition(-340., -70., -50.);
  renderer->GetActiveCamera()->SetFocalPoint(-2.5, 3., -5.);
  renderer->GetActiveCamera()->SetViewUp(0, 0.5, -1);
  renderer->GetActiveCamera()->SetParallelScale(12);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
