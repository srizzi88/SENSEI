/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <svtkIntersectionPolyDataFilter.h>

#include <svtkActor.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

int TestIntersectionPolyDataFilter(int, char*[])
{
  svtkSmartPointer<svtkSphereSource> sphereSource1 = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource1->SetCenter(0.0, 0.0, 0.0);
  sphereSource1->SetRadius(2.0);
  sphereSource1->SetPhiResolution(11);
  sphereSource1->SetThetaResolution(21);
  sphereSource1->Update();
  svtkSmartPointer<svtkPolyDataMapper> sphere1Mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  sphere1Mapper->SetInputConnection(sphereSource1->GetOutputPort());
  sphere1Mapper->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> sphere1Actor = svtkSmartPointer<svtkActor>::New();
  sphere1Actor->SetMapper(sphere1Mapper);
  sphere1Actor->GetProperty()->SetOpacity(.3);
  sphere1Actor->GetProperty()->SetColor(1, 0, 0);
  sphere1Actor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkSphereSource> sphereSource2 = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource2->SetCenter(1.0, 0.0, 0.0);
  sphereSource2->SetRadius(2.0);
  svtkSmartPointer<svtkPolyDataMapper> sphere2Mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  sphere2Mapper->SetInputConnection(sphereSource2->GetOutputPort());
  sphere2Mapper->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> sphere2Actor = svtkSmartPointer<svtkActor>::New();
  sphere2Actor->SetMapper(sphere2Mapper);
  sphere2Actor->GetProperty()->SetOpacity(.3);
  sphere2Actor->GetProperty()->SetColor(0, 1, 0);
  sphere2Actor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
    svtkSmartPointer<svtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SetInputConnection(0, sphereSource1->GetOutputPort());
  intersectionPolyDataFilter->SetInputConnection(1, sphereSource2->GetOutputPort());
  intersectionPolyDataFilter->Update();

  svtkSmartPointer<svtkPolyDataMapper> intersectionMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection(intersectionPolyDataFilter->GetOutputPort());
  intersectionMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> intersectionActor = svtkSmartPointer<svtkActor>::New();
  intersectionActor->SetMapper(intersectionMapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddViewProp(sphere1Actor);
  renderer->AddViewProp(sphere2Actor);
  renderer->AddViewProp(intersectionActor);
  renderer->SetBackground(.1, .2, .3);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renWinInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renderWindow);

  intersectionPolyDataFilter->Print(std::cout);

  renderWindow->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
