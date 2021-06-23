/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <svtkIntersectionPolyDataFilter.h>

#include <svtkActor.h>
#include <svtkConeSource.h>
#include <svtkCubeSource.h>
#include <svtkLinearSubdivisionFilter.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTriangleFilter.h>

int TestIntersectionPolyDataFilter3(int, char*[])
{
  svtkSmartPointer<svtkCubeSource> cubeSource = svtkSmartPointer<svtkCubeSource>::New();
  cubeSource->SetCenter(0.0, 0.0, 0.0);
  cubeSource->SetXLength(1.0);
  cubeSource->SetYLength(1.0);
  cubeSource->SetZLength(1.0);
  cubeSource->Update();
  svtkSmartPointer<svtkTriangleFilter> cubetriangulator = svtkSmartPointer<svtkTriangleFilter>::New();
  cubetriangulator->SetInputConnection(cubeSource->GetOutputPort());
  svtkSmartPointer<svtkLinearSubdivisionFilter> cubesubdivider =
    svtkSmartPointer<svtkLinearSubdivisionFilter>::New();
  cubesubdivider->SetInputConnection(cubetriangulator->GetOutputPort());
  cubesubdivider->SetNumberOfSubdivisions(3);
  svtkSmartPointer<svtkPolyDataMapper> cubeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  cubeMapper->SetInputConnection(cubesubdivider->GetOutputPort());
  cubeMapper->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> cubeActor = svtkSmartPointer<svtkActor>::New();
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetOpacity(.3);
  cubeActor->GetProperty()->SetColor(1, 0, 0);
  cubeActor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  coneSource->SetCenter(0.0, 0.0, 0.0);
  coneSource->SetRadius(0.5);
  coneSource->SetHeight(2.0);
  coneSource->SetResolution(10.0);
  coneSource->SetDirection(1, 0, 0);
  svtkSmartPointer<svtkTriangleFilter> conetriangulator = svtkSmartPointer<svtkTriangleFilter>::New();
  conetriangulator->SetInputConnection(coneSource->GetOutputPort());
  svtkSmartPointer<svtkLinearSubdivisionFilter> conesubdivider =
    svtkSmartPointer<svtkLinearSubdivisionFilter>::New();
  conesubdivider->SetInputConnection(conetriangulator->GetOutputPort());
  conesubdivider->SetNumberOfSubdivisions(3);
  svtkSmartPointer<svtkPolyDataMapper> coneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(conesubdivider->GetOutputPort());
  coneMapper->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> coneActor = svtkSmartPointer<svtkActor>::New();
  coneActor->SetMapper(coneMapper);
  coneActor->GetProperty()->SetOpacity(.3);
  coneActor->GetProperty()->SetColor(0, 1, 0);
  coneActor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
    svtkSmartPointer<svtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SetInputConnection(0, cubesubdivider->GetOutputPort());
  intersectionPolyDataFilter->SetInputConnection(1, conesubdivider->GetOutputPort());
  intersectionPolyDataFilter->SetSplitFirstOutput(0);
  intersectionPolyDataFilter->SetSplitSecondOutput(0);
  intersectionPolyDataFilter->Update();

  svtkSmartPointer<svtkPolyDataMapper> intersectionMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection(intersectionPolyDataFilter->GetOutputPort());
  intersectionMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> intersectionActor = svtkSmartPointer<svtkActor>::New();
  intersectionActor->SetMapper(intersectionMapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddViewProp(cubeActor);
  renderer->AddViewProp(coneActor);
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
