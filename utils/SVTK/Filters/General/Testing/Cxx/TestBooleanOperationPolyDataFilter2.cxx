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
#include <svtkAppendPolyData.h>
#include <svtkBooleanOperationPolyDataFilter.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkDistancePolyDataFilter.h>
#include <svtkIntersectionPolyDataFilter.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkReverseSense.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkThreshold.h>

static svtkActor* GetBooleanOperationActor(double x, int operation)
{
  double centerSeparation = 0.15;
  svtkSmartPointer<svtkSphereSource> sphere1 = svtkSmartPointer<svtkSphereSource>::New();
  sphere1->SetCenter(-centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkSphereSource> sphere2 = svtkSmartPointer<svtkSphereSource>::New();
  sphere2->SetCenter(centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkIntersectionPolyDataFilter> intersection =
    svtkSmartPointer<svtkIntersectionPolyDataFilter>::New();
  intersection->SetInputConnection(0, sphere1->GetOutputPort());
  intersection->SetInputConnection(1, sphere2->GetOutputPort());

  svtkSmartPointer<svtkDistancePolyDataFilter> distance =
    svtkSmartPointer<svtkDistancePolyDataFilter>::New();
  distance->SetInputConnection(0, intersection->GetOutputPort(1));
  distance->SetInputConnection(1, intersection->GetOutputPort(2));

  svtkSmartPointer<svtkThreshold> thresh1 = svtkSmartPointer<svtkThreshold>::New();
  thresh1->AllScalarsOn();
  thresh1->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "Distance");
  thresh1->SetInputConnection(distance->GetOutputPort(0));

  svtkSmartPointer<svtkThreshold> thresh2 = svtkSmartPointer<svtkThreshold>::New();
  thresh2->AllScalarsOn();
  thresh2->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "Distance");
  thresh2->SetInputConnection(distance->GetOutputPort(1));

  if (operation == svtkBooleanOperationPolyDataFilter::SVTK_UNION)
  {
    thresh1->ThresholdByUpper(0.0);
    thresh2->ThresholdByUpper(0.0);
  }
  else if (operation == svtkBooleanOperationPolyDataFilter::SVTK_INTERSECTION)
  {
    thresh1->ThresholdByLower(0.0);
    thresh2->ThresholdByLower(0.0);
  }
  else // Difference
  {
    thresh1->ThresholdByUpper(0.0);
    thresh2->ThresholdByLower(0.0);
  }

  svtkSmartPointer<svtkDataSetSurfaceFilter> surface1 =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surface1->SetInputConnection(thresh1->GetOutputPort());

  svtkSmartPointer<svtkDataSetSurfaceFilter> surface2 =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surface2->SetInputConnection(thresh2->GetOutputPort());

  svtkSmartPointer<svtkReverseSense> reverseSense = svtkSmartPointer<svtkReverseSense>::New();
  reverseSense->SetInputConnection(surface2->GetOutputPort());
  if (operation == 2) // difference
  {
    reverseSense->ReverseCellsOn();
    reverseSense->ReverseNormalsOn();
  }

  svtkSmartPointer<svtkAppendPolyData> appender = svtkSmartPointer<svtkAppendPolyData>::New();
  appender->SetInputConnection(surface1->GetOutputPort());
  if (operation == 2)
  {
    appender->AddInputConnection(reverseSense->GetOutputPort());
  }
  else
  {
    appender->AddInputConnection(surface2->GetOutputPort());
  }

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(appender->GetOutputPort());
  mapper->ScalarVisibilityOff();

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  return actor;
}

int TestBooleanOperationPolyDataFilter2(int, char*[])
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
