/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cube.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example shows how to manually create svtkPolyData.

// For a python version, please see:
// [Cube](https://lorensen.github.io/SVTKExamples/site/Python/DataManipulation/Cube/)

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCellArray.h>
#include <svtkFloatArray.h>
#include <svtkNamedColors.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <array>

int main()
{
  svtkNew<svtkNamedColors> colors;

  std::array<std::array<double, 3>, 8> pts = { { { { 0, 0, 0 } }, { { 1, 0, 0 } }, { { 1, 1, 0 } },
    { { 0, 1, 0 } }, { { 0, 0, 1 } }, { { 1, 0, 1 } }, { { 1, 1, 1 } }, { { 0, 1, 1 } } } };
  // The ordering of the corner points on each face.
  std::array<std::array<svtkIdType, 4>, 6> ordering = { { { { 0, 1, 2, 3 } }, { { 4, 5, 6, 7 } },
    { { 0, 1, 5, 4 } }, { { 1, 2, 6, 5 } }, { { 2, 3, 7, 6 } }, { { 3, 0, 4, 7 } } } };

  // We'll create the building blocks of polydata including data attributes.
  svtkNew<svtkPolyData> cube;
  svtkNew<svtkPoints> points;
  svtkNew<svtkCellArray> polys;
  svtkNew<svtkFloatArray> scalars;

  // Load the point, cell, and data attributes.
  for (auto i = 0ul; i < pts.size(); ++i)
  {
    points->InsertPoint(i, pts[i].data());
    scalars->InsertTuple1(i, i);
  }
  for (auto&& i : ordering)
  {
    polys->InsertNextCell(svtkIdType(i.size()), i.data());
  }

  // We now assign the pieces to the svtkPolyData.
  cube->SetPoints(points);
  cube->SetPolys(polys);
  cube->GetPointData()->SetScalars(scalars);

  // Now we'll look at it.
  svtkNew<svtkPolyDataMapper> cubeMapper;
  cubeMapper->SetInputData(cube);
  cubeMapper->SetScalarRange(cube->GetScalarRange());
  svtkNew<svtkActor> cubeActor;
  cubeActor->SetMapper(cubeMapper);

  // The usual rendering stuff.
  svtkNew<svtkCamera> camera;
  camera->SetPosition(1, 1, 1);
  camera->SetFocalPoint(0, 0, 0);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(cubeActor);
  renderer->SetActiveCamera(camera);
  renderer->ResetCamera();
  renderer->SetBackground(colors->GetColor3d("Cornsilk").GetData());

  renWin->SetSize(600, 600);

  // interact with data
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
