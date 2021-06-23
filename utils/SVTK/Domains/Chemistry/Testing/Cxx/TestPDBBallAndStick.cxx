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
#include "svtkLight.h"
#include "svtkMolecule.h"
#include "svtkMoleculeMapper.h"
#include "svtkNew.h"
#include "svtkPDBReader.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkCamera.h"
#include "svtkTimerLog.h"

int TestPDBBallAndStick(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2LYZ.pdb");

  // read protein from pdb
  svtkNew<svtkPDBReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkNew<svtkMoleculeMapper> molmapper;
  molmapper->SetInputConnection(reader->GetOutputPort(1));

  cerr << "Class: " << molmapper->GetClassName() << endl;
  cerr << "Atoms: " << molmapper->GetInput()->GetNumberOfAtoms() << endl;
  cerr << "Bonds: " << molmapper->GetInput()->GetNumberOfBonds() << endl;

  molmapper->UseBallAndStickSettings();

  svtkNew<svtkActor> actor;
  actor->SetMapper(molmapper);
  actor->GetProperty()->SetAmbient(0.0);
  actor->GetProperty()->SetDiffuse(1.0);
  actor->GetProperty()->SetSpecular(0.0);
  actor->GetProperty()->SetSpecularPower(40);

  svtkNew<svtkLight> light;
  light->SetLightTypeToCameraLight();
  light->SetPosition(1.0, 1.0, 1.0);

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);
  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  /*
    int numRenders = 500;
    timer->StartTimer();
    for (int i = 0; i < numRenders; ++i)
      {
      ren->GetActiveCamera()->Azimuth(85.0/numRenders);
      ren->GetActiveCamera()->Elevation(85.0/numRenders);
      win->Render();
      }
    timer->StopTimer();
    double elapsed = timer->GetElapsedTime();
    cerr << "interactive render time: " << elapsed / numRenders << endl;
  */

  ren->GetActiveCamera()->SetPosition(0, 0, 1);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);

  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
