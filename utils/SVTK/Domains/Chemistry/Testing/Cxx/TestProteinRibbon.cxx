/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkInteractorStyleSwitch.h"
#include "svtkNew.h"
#include "svtkPDBReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProteinRibbonFilter.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestProteinRibbon(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/3GQP.pdb");

  // read protein from pdb
  svtkNew<svtkPDBReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  // setup ribbon filter
  svtkNew<svtkProteinRibbonFilter> ribbonFilter;
  ribbonFilter->SetInputConnection(reader->GetOutputPort());
  ribbonFilter->Update();

  // setup poly data mapper
  svtkNew<svtkPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputData(ribbonFilter->GetOutput());
  polyDataMapper->Update();

  // setup actor
  svtkNew<svtkActor> actor;
  actor->SetMapper(polyDataMapper);

  // setup render window
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);
  svtkInteractorStyleSwitch* is = svtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle());
  if (is)
  {
    is->SetCurrentStyleToTrackballCamera();
  }
  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  win->SetSize(450, 450);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  ren->ResetCameraClippingRange();
  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
