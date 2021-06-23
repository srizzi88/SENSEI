/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <svtkSmartPointer.h>

#include <svtkActor.h>
#include <svtkDistancePolyDataFilter.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkScalarBarActor.h>
#include <svtkSphereSource.h>

int TestDistancePolyDataFilter(int, char*[])
{
  svtkSmartPointer<svtkSphereSource> model1 = svtkSmartPointer<svtkSphereSource>::New();
  model1->SetPhiResolution(11);
  model1->SetThetaResolution(11);
  model1->SetCenter(0.0, 0.0, 0.0);

  svtkSmartPointer<svtkSphereSource> model2 = svtkSmartPointer<svtkSphereSource>::New();
  model2->SetPhiResolution(11);
  model2->SetThetaResolution(11);
  model2->SetCenter(0.2, 0.3, 0.0);

  svtkSmartPointer<svtkDistancePolyDataFilter> distanceFilter =
    svtkSmartPointer<svtkDistancePolyDataFilter>::New();

  distanceFilter->SetInputConnection(0, model1->GetOutputPort());
  distanceFilter->SetInputConnection(1, model2->GetOutputPort());
  distanceFilter->Update();

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(distanceFilter->GetOutputPort());
  mapper->SetScalarRange(distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0],
    distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[1]);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkPolyDataMapper> mapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper2->SetInputConnection(distanceFilter->GetOutputPort(1));
  mapper2->SetScalarRange(
    distanceFilter->GetSecondDistanceOutput()->GetPointData()->GetScalars()->GetRange()[0],
    distanceFilter->GetSecondDistanceOutput()->GetPointData()->GetScalars()->GetRange()[1]);

  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  actor2->SetMapper(mapper2);

  svtkSmartPointer<svtkScalarBarActor> scalarBar = svtkSmartPointer<svtkScalarBarActor>::New();
  scalarBar->SetLookupTable(mapper->GetLookupTable());
  scalarBar->SetTitle("Distance");
  scalarBar->SetNumberOfLabels(5);
  scalarBar->SetTextPad(4);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renWinInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renWin);

  renderer->AddActor(actor);
  renderer->AddActor(actor2);
  renderer->AddActor2D(scalarBar);

  renWin->Render();
  distanceFilter->Print(std::cout);

  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
