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

// Test that no scalar overflow occurs with ButterflySubdivision

#include "svtkSmartPointer.h"

#include "svtkButterflySubdivisionFilter.h"
#include "svtkCylinderSource.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkLoopSubdivisionFilter.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTriangleFilter.h"
#include "svtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
int TestButterflyScalars(int argc, char* argv[])
{
  // Defining a cylinder source.
  svtkSmartPointer<svtkCylinderSource> cylinderSource = svtkSmartPointer<svtkCylinderSource>::New();
  cylinderSource->Update();

  svtkSmartPointer<svtkTriangleFilter> triangles = svtkSmartPointer<svtkTriangleFilter>::New();
  triangles->SetInputConnection(cylinderSource->GetOutputPort());
  triangles->Update();

  svtkSmartPointer<svtkPolyData> originalMesh;
  originalMesh = triangles->GetOutput();

  svtkSmartPointer<svtkUnsignedCharArray> colors = svtkSmartPointer<svtkUnsignedCharArray>::New();
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(originalMesh->GetNumberOfPoints());
  colors->SetName("Colors");

  // Loop to select colors for each of the points in the polydata.
  for (int i = 0; i < originalMesh->GetNumberOfPoints(); i++)
  {
    if (i > 0 && i < 5)
    {
      // Black
      colors->InsertTuple3(i, 255, 255, 0);
    }
    else if (i > 4 && i < 10)
    {
      // Blue
      colors->InsertTuple3(i, 0, 0, 255);
    }
    else if (i > 9 && i < 300)
    {
      // Red
      colors->InsertTuple3(i, 255, 0, 0);
    }
    else
    {
      colors->InsertTuple3(i, 255, 0, 0);
    }
  }

  originalMesh->GetPointData()->SetScalars(colors);

  // Subdivision.
  int numberOfSubdivisions = 4;
  svtkSmartPointer<svtkButterflySubdivisionFilter> subdivisionFilter =
    svtkSmartPointer<svtkButterflySubdivisionFilter>::New();

  subdivisionFilter->SetNumberOfSubdivisions(numberOfSubdivisions);
  subdivisionFilter->SetInputData(originalMesh);
  subdivisionFilter->Update();

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create a mapper and actor
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(subdivisionFilter->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(0, 0, 0);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);
  renderWindow->Render();

  int testStatus = svtkRegressionTestImage(renderWindow);
  if (testStatus == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return (testStatus ? EXIT_SUCCESS : EXIT_FAILURE);
}
