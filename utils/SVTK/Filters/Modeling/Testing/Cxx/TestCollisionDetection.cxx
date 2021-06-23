/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButterflyScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCollisionDetectionFilter.h"
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkMatrix4x4.h"
#include "svtkNamedColors.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTransform.h"
#include <chrono>
#include <sstream>
#include <string>
#include <thread>

int TestCollisionDetection(int argc, char* argv[])
{
  int contactMode = 0;
  if (argc > 1)
  {
    contactMode = std::stoi(std::string(argv[1]));
  }
  svtkSmartPointer<svtkSphereSource> sphere0 = svtkSmartPointer<svtkSphereSource>::New();
  sphere0->SetRadius(.29);
  sphere0->SetPhiResolution(31);
  sphere0->SetThetaResolution(31);
  sphere0->SetCenter(0.0, 0, 0);

  svtkSmartPointer<svtkSphereSource> sphere1 = svtkSmartPointer<svtkSphereSource>::New();
  sphere1->SetPhiResolution(30);
  sphere1->SetThetaResolution(30);
  sphere1->SetRadius(0.3);

  svtkSmartPointer<svtkMatrix4x4> matrix1 = svtkSmartPointer<svtkMatrix4x4>::New();
  svtkSmartPointer<svtkTransform> transform0 = svtkSmartPointer<svtkTransform>::New();

  svtkSmartPointer<svtkCollisionDetectionFilter> collide =
    svtkSmartPointer<svtkCollisionDetectionFilter>::New();
  collide->SetInputConnection(0, sphere0->GetOutputPort());
  collide->SetTransform(0, transform0);
  collide->SetInputConnection(1, sphere1->GetOutputPort());
  collide->SetMatrix(1, matrix1);
  collide->SetBoxTolerance(0.0);
  collide->SetCellTolerance(0.0);
  collide->SetNumberOfCellsPerNode(2);
  if (contactMode == 0)
  {
    collide->SetCollisionModeToAllContacts();
  }
  else if (contactMode == 1)
  {
    collide->SetCollisionModeToFirstContact();
  }
  else
  {
    collide->SetCollisionModeToHalfContacts();
  }
  collide->GenerateScalarsOn();

  // Visualize
  svtkSmartPointer<svtkNamedColors> colors = svtkSmartPointer<svtkNamedColors>::New();
  svtkSmartPointer<svtkPolyDataMapper> mapper1 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper1->SetInputConnection(collide->GetOutputPort(0));
  mapper1->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> actor1 = svtkSmartPointer<svtkActor>::New();
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->BackfaceCullingOn();
  actor1->SetUserTransform(transform0);
  actor1->GetProperty()->SetDiffuseColor(colors->GetColor3d("tomato").GetData());
  actor1->GetProperty()->SetRepresentationToWireframe();

  svtkSmartPointer<svtkPolyDataMapper> mapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper2->SetInputConnection(collide->GetOutputPort(1));

  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->BackfaceCullingOn();
  actor2->SetUserMatrix(matrix1);

  svtkSmartPointer<svtkPolyDataMapper> mapper3 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper3->SetInputConnection(collide->GetContactsOutputPort());
  mapper3->SetResolveCoincidentTopologyToPolygonOffset();

  svtkSmartPointer<svtkActor> actor3 = svtkSmartPointer<svtkActor>::New();
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(0, 0, 0);
  actor3->GetProperty()->SetLineWidth(3.0);

  svtkSmartPointer<svtkTextActor> txt = svtkSmartPointer<svtkTextActor>::New();

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->UseHiddenLineRemovalOn();
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(txt);
  renderer->SetBackground(0.5, 0.5, 0.5);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->SetSize(640, 480);
  renderWindow->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> interactor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow(renderWindow);

  // Move the first object
  double dx = .1;
  int numSteps = 20;
  transform0->Translate(-numSteps * dx - .3, 0.0, 0.0);
  renderWindow->Render();
  renderer->GetActiveCamera()->Azimuth(-45);
  renderer->GetActiveCamera()->Elevation(45);
  renderer->GetActiveCamera()->Dolly(1.2);

  for (int i = 0; i < numSteps; ++i)
  {
    transform0->Translate(dx, 0.0, 0.0);
    renderer->ResetCameraClippingRange();
    std::ostringstream textStream;
    textStream << collide->GetCollisionModeAsString() << ": Number of contact cells is "
               << collide->GetNumberOfContacts();
    txt->SetInput(textStream.str().c_str());
    renderWindow->Render();
    if (collide->GetNumberOfContacts() > 0)
    {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  renderer->ResetCamera();
  renderWindow->Render();
  interactor->Start();
  collide->Print(std::cout);
  return EXIT_SUCCESS;
}
