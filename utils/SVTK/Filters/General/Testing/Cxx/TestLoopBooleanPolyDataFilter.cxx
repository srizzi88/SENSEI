/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLoopOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkCubeSource.h"
#include "svtkCylinderSource.h"
#include "svtkIntersectionPolyDataFilter.h"
#include "svtkLinearSubdivisionFilter.h"
#include "svtkLoopBooleanPolyDataFilter.h"
#include "svtkMath.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkTriangleFilter.h"

static svtkSmartPointer<svtkActor> GetCubeBooleanOperationActor(double x, int operation)
{
  svtkSmartPointer<svtkCubeSource> cube1 = svtkSmartPointer<svtkCubeSource>::New();
  cube1->SetCenter(x, 4.0, 0.0);
  cube1->SetXLength(1.0);
  cube1->SetYLength(1.0);
  cube1->SetZLength(1.0);
  cube1->Update();

  svtkSmartPointer<svtkTriangleFilter> triangulator1 = svtkSmartPointer<svtkTriangleFilter>::New();
  triangulator1->SetInputData(cube1->GetOutput());
  triangulator1->Update();

  svtkSmartPointer<svtkLinearSubdivisionFilter> subdivider1 =
    svtkSmartPointer<svtkLinearSubdivisionFilter>::New();
  subdivider1->SetInputData(triangulator1->GetOutput());
  subdivider1->Update();

  svtkSmartPointer<svtkCubeSource> cube2 = svtkSmartPointer<svtkCubeSource>::New();
  cube2->SetCenter(x + 0.3, 4.3, 0.3);
  cube2->SetXLength(1.0);
  cube2->SetYLength(1.0);
  cube2->SetZLength(1.0);
  cube2->Update();

  svtkSmartPointer<svtkTriangleFilter> triangulator2 = svtkSmartPointer<svtkTriangleFilter>::New();
  triangulator2->SetInputData(cube2->GetOutput());
  triangulator2->Update();

  svtkSmartPointer<svtkLinearSubdivisionFilter> subdivider2 =
    svtkSmartPointer<svtkLinearSubdivisionFilter>::New();
  subdivider2->SetInputData(triangulator2->GetOutput());
  subdivider2->Update();

  svtkSmartPointer<svtkLoopBooleanPolyDataFilter> boolFilter =
    svtkSmartPointer<svtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, subdivider1->GetOutputPort());
  boolFilter->SetInputConnection(1, subdivider2->GetOutputPort());
  boolFilter->Update();

  svtkSmartPointer<svtkPolyData> output = svtkSmartPointer<svtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

static svtkSmartPointer<svtkActor> GetSphereBooleanOperationActor(double x, int operation)
{
  double centerSeparation = 0.15;
  svtkSmartPointer<svtkSphereSource> sphere1 = svtkSmartPointer<svtkSphereSource>::New();
  sphere1->SetCenter(-centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkSphereSource> sphere2 = svtkSmartPointer<svtkSphereSource>::New();
  sphere2->SetCenter(centerSeparation + x, 0.0, 0.0);

  svtkSmartPointer<svtkLoopBooleanPolyDataFilter> boolFilter =
    svtkSmartPointer<svtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, sphere1->GetOutputPort());
  boolFilter->SetInputConnection(1, sphere2->GetOutputPort());
  boolFilter->Update();

  svtkSmartPointer<svtkPolyData> output = svtkSmartPointer<svtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

static svtkSmartPointer<svtkActor> GetCylinderBooleanOperationActor(double x, int operation)
{
  double axis[3];
  axis[0] = 0.0;
  axis[1] = 1.0;
  axis[2] = 0.0;
  double vec[3];
  vec[0] = 0.0;
  vec[1] = 1.0;
  vec[2] = 0.0;
  double rotateaxis[3];
  svtkMath::Cross(axis, vec, rotateaxis);
  svtkSmartPointer<svtkCylinderSource> cylinder1 = svtkSmartPointer<svtkCylinderSource>::New();
  cylinder1->SetCenter(0.0, 0.0, 0.0);
  cylinder1->SetHeight(2.0);
  cylinder1->SetRadius(0.5);
  cylinder1->SetResolution(15);
  cylinder1->Update();
  double radangle = svtkMath::AngleBetweenVectors(axis, vec);
  double degangle = svtkMath::DegreesFromRadians(radangle);
  svtkSmartPointer<svtkTransform> rotator1 = svtkSmartPointer<svtkTransform>::New();
  rotator1->RotateWXYZ(degangle, rotateaxis);

  svtkSmartPointer<svtkTransformPolyDataFilter> polyDataRotator1 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  polyDataRotator1->SetInputData(cylinder1->GetOutput());
  polyDataRotator1->SetTransform(rotator1);
  polyDataRotator1->Update();

  svtkSmartPointer<svtkTransform> mover1 = svtkSmartPointer<svtkTransform>::New();
  mover1->Translate(x, -4.0, 0.0);

  svtkSmartPointer<svtkTransformPolyDataFilter> polyDataMover1 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  polyDataMover1->SetInputData(polyDataRotator1->GetOutput());
  polyDataMover1->SetTransform(mover1);
  polyDataMover1->Update();

  svtkSmartPointer<svtkTriangleFilter> triangulator1 = svtkSmartPointer<svtkTriangleFilter>::New();
  triangulator1->SetInputData(polyDataMover1->GetOutput());
  triangulator1->Update();

  axis[0] = 1.0;
  axis[1] = 0.0;
  axis[2] = 0.0;
  svtkMath::Cross(axis, vec, rotateaxis);
  svtkSmartPointer<svtkCylinderSource> cylinder2 = svtkSmartPointer<svtkCylinderSource>::New();
  cylinder2->SetCenter(0.0, 0.0, 0.0);
  cylinder2->SetHeight(2.0);
  cylinder2->SetRadius(0.5);
  cylinder2->SetResolution(15);
  cylinder2->Update();
  radangle = svtkMath::AngleBetweenVectors(axis, vec);
  degangle = svtkMath::DegreesFromRadians(radangle);
  svtkSmartPointer<svtkTransform> rotator2 = svtkSmartPointer<svtkTransform>::New();
  rotator2->RotateWXYZ(degangle, rotateaxis);

  svtkSmartPointer<svtkTransformPolyDataFilter> polyDataRotator2 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  polyDataRotator2->SetInputData(cylinder2->GetOutput());
  polyDataRotator2->SetTransform(rotator2);
  polyDataRotator2->Update();

  svtkSmartPointer<svtkTransform> mover2 = svtkSmartPointer<svtkTransform>::New();
  mover2->Translate(x, -4.0, 0.0);

  svtkSmartPointer<svtkTransformPolyDataFilter> polyDataMover2 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  polyDataMover2->SetInputData(polyDataRotator2->GetOutput());
  polyDataMover2->SetTransform(mover2);
  polyDataMover2->Update();

  svtkSmartPointer<svtkTriangleFilter> triangulator2 = svtkSmartPointer<svtkTriangleFilter>::New();
  triangulator2->SetInputData(polyDataMover2->GetOutput());
  triangulator2->Update();

  svtkSmartPointer<svtkLoopBooleanPolyDataFilter> boolFilter =
    svtkSmartPointer<svtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, triangulator1->GetOutputPort());
  boolFilter->SetInputConnection(1, triangulator2->GetOutputPort());
  boolFilter->Update();

  svtkSmartPointer<svtkPolyData> output = svtkSmartPointer<svtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

int TestLoopBooleanPolyDataFilter(int, char*[])
{
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renWinInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renWin);

  // Sphere
  svtkSmartPointer<svtkActor> unionActor =
    GetSphereBooleanOperationActor(-2.0, svtkLoopBooleanPolyDataFilter::SVTK_UNION);
  renderer->AddActor(unionActor);

  svtkSmartPointer<svtkActor> intersectionActor =
    GetSphereBooleanOperationActor(0.0, svtkLoopBooleanPolyDataFilter::SVTK_INTERSECTION);
  renderer->AddActor(intersectionActor);

  svtkSmartPointer<svtkActor> differenceActor =
    GetSphereBooleanOperationActor(2.0, svtkLoopBooleanPolyDataFilter::SVTK_DIFFERENCE);
  renderer->AddActor(differenceActor);

  // Cube
  svtkSmartPointer<svtkActor> unionCubeActor =
    GetCubeBooleanOperationActor(-2.0, svtkLoopBooleanPolyDataFilter::SVTK_UNION);
  renderer->AddActor(unionCubeActor);

  svtkSmartPointer<svtkActor> intersectionCubeActor =
    GetCubeBooleanOperationActor(0.0, svtkLoopBooleanPolyDataFilter::SVTK_INTERSECTION);
  renderer->AddActor(intersectionCubeActor);

  svtkSmartPointer<svtkActor> differenceCubeActor =
    GetCubeBooleanOperationActor(2.0, svtkLoopBooleanPolyDataFilter::SVTK_DIFFERENCE);
  renderer->AddActor(differenceCubeActor);

  // Cylinder
  svtkSmartPointer<svtkActor> unionCylinderActor =
    GetCylinderBooleanOperationActor(-2.0, svtkLoopBooleanPolyDataFilter::SVTK_UNION);
  renderer->AddActor(unionCylinderActor);

  svtkSmartPointer<svtkActor> intersectionCylinderActor =
    GetCylinderBooleanOperationActor(0.0, svtkLoopBooleanPolyDataFilter::SVTK_INTERSECTION);
  renderer->AddActor(intersectionCylinderActor);

  svtkSmartPointer<svtkActor> differenceCylinderActor =
    GetCylinderBooleanOperationActor(2.0, svtkLoopBooleanPolyDataFilter::SVTK_DIFFERENCE);
  renderer->AddActor(differenceCylinderActor);

  renderer->SetBackground(0.4392, 0.5020, 0.5647);
  renWin->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
