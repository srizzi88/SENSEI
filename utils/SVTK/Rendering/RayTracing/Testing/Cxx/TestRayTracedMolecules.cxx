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
#include "svtkOSPRayPass.h"
#include "svtkPDBReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkCamera.h"
#include "svtkTimerLog.h"

// This is a clone of TestPDBBallAndStickShadows that validates RayTraced
// molecule rendering.
int TestRayTracedMolecules(int argc, char* argv[])
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
  actor->GetProperty()->SetAmbient(0.3);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetSpecular(0.4);
  actor->GetProperty()->SetSpecularPower(40);

  svtkNew<svtkRenderer> ren;
  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  ren->SetPass(ospray);

  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);
  ren->SetBackground(0.4, 0.5, 0.6);
  win->SetSize(450, 450);

  // add a plane
  svtkNew<svtkPlaneSource> plane;
  const double* bounds = molmapper->GetBounds();
  plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
  plane->SetPoint1(bounds[1], bounds[2], bounds[4]);
  plane->SetPoint2(bounds[0], bounds[2], bounds[5]);
  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  ren->AddActor(planeActor);

  svtkNew<svtkLight> light1;
  light1->SetFocalPoint(0, 0, 0);
  light1->SetPosition(0, 1, 0.2);
  light1->SetColor(0.95, 0.97, 1.0);
  light1->SetIntensity(0.8);
  ren->AddLight(light1);

  svtkNew<svtkLight> light2;
  light2->SetFocalPoint(0, 0, 0);
  light2->SetPosition(1.0, 1.0, 1.0);
  light2->SetColor(1.0, 0.8, 0.7);
  light2->SetIntensity(0.3);
  ren->AddLight(light2);

  ren->UseShadowsOn();

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
