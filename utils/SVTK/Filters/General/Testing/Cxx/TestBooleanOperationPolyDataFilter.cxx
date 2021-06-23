/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBooleanOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <svtkActor.h>
#include <svtkBooleanOperationPolyDataFilter.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

static svtkActor* GetBooleanOperationActor(double x, int operation)
{
  double centerSeparation = 0.15;
  svtkSmartPointer<svtkSphereSource> sphere1 = svtkSmartPointer<svtkSphereSource>::New();
  sphere1->SetCenter(-centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkSphereSource> sphere2 = svtkSmartPointer<svtkSphereSource>::New();
  sphere2->SetCenter(centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkBooleanOperationPolyDataFilter> boolFilter =
    svtkSmartPointer<svtkBooleanOperationPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, sphere1->GetOutputPort());
  boolFilter->SetInputConnection(1, sphere2->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(boolFilter->GetOutputPort(0));
  mapper->ScalarVisibilityOff();

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  return actor;
}

int TestBooleanOperationPolyDataFilter(int, char*[])
{
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renWinInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renWin);

  svtkActor* unionActor =
    GetBooleanOperationActor(-2.0, svtkBooleanOperationPolyDataFilter::SVTK_UNION);
  renderer->AddActor(unionActor);
  unionActor->Delete();

  svtkActor* intersectionActor =
    GetBooleanOperationActor(0.0, svtkBooleanOperationPolyDataFilter::SVTK_INTERSECTION);
  renderer->AddActor(intersectionActor);
  intersectionActor->Delete();

  svtkActor* differenceActor =
    GetBooleanOperationActor(2.0, svtkBooleanOperationPolyDataFilter::SVTK_DIFFERENCE);
  renderer->AddActor(differenceActor);
  differenceActor->Delete();

  renWin->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
