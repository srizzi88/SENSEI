/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkButterflySubdivisionFilter.h"
#include "svtkLinearSubdivisionFilter.h"
#include "svtkLoopSubdivisionFilter.h"

#include "svtkCellArray.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkQuad.h"
#include "svtkTriangle.h"

#include "svtkCommand.h"
#include "svtkExecutive.h"
#include "svtkTestErrorObserver.h"

#include <sstream>

template <typename T>
int TestSubdivision();

int UnitTestSubdivisionFilters(int, char*[])
{
  int status = EXIT_SUCCESS;

  status += TestSubdivision<svtkButterflySubdivisionFilter>();
  status += TestSubdivision<svtkLinearSubdivisionFilter>();
  status += TestSubdivision<svtkLoopSubdivisionFilter>();

  return status;
}

template <typename T>
int TestSubdivision()
{
  int status = EXIT_SUCCESS;

  // Start of test
  svtkSmartPointer<T> subdivision0 = svtkSmartPointer<T>::New();
  std::cout << "Testing " << subdivision0->GetClassName() << std::endl;

  // Empty Print
  std::cout << "  Testing empty print...";
  std::ostringstream emptyPrint;
  subdivision0->Print(emptyPrint);
  std::cout << "PASSED" << std::endl;

  // Catch empty input error message
  std::cout << "  Testing empty input...";
  svtkSmartPointer<svtkTest::ErrorObserver> executiveObserver =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();

  subdivision0->GetExecutive()->AddObserver(svtkCommand::ErrorEvent, executiveObserver);
  subdivision0->Update();

  int status1 = executiveObserver->CheckErrorMessage("has 0 connections but is not optional.");
  if (status1 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  // Testing empty dataset
  std::cout << "  Testing empty dataset...";
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  subdivision0->AddObserver(svtkCommand::ErrorEvent, errorObserver);
  subdivision0->SetInputData(polyData);
  subdivision0->SetNumberOfSubdivisions(4);
  subdivision0->Update();

  int status2 = errorObserver->CheckErrorMessage("No data to subdivide");
  if (status2 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  // Create a triangle
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 1.0);
  points->InsertNextPoint(0.0, 1.0, -1.0);

  svtkSmartPointer<svtkTriangle> triangle = svtkSmartPointer<svtkTriangle>::New();
  triangle->GetPointIds()->SetId(0, 0);
  triangle->GetPointIds()->SetId(1, 1);
  triangle->GetPointIds()->SetId(2, 2);

  svtkSmartPointer<svtkCellArray> triangles = svtkSmartPointer<svtkCellArray>::New();
  triangles->InsertNextCell(triangle);

  svtkSmartPointer<svtkPolyData> trianglePolyData = svtkSmartPointer<svtkPolyData>::New();
  trianglePolyData->SetPoints(points);
  trianglePolyData->SetPolys(triangles);

  std::cout << "  Testing a triangle...";
  subdivision0->SetInputData(trianglePolyData);
  subdivision0->Update();
  std::cout << "PASSED" << std::endl;

  std::cout << "  Testing non-manifold dataset...";

  svtkSmartPointer<svtkTriangle> triangle2 = svtkSmartPointer<svtkTriangle>::New();
  triangle2->GetPointIds()->SetId(0, 0);
  triangle2->GetPointIds()->SetId(1, 1);
  triangle2->GetPointIds()->SetId(2, 3);
  svtkSmartPointer<svtkTriangle> triangle3 = svtkSmartPointer<svtkTriangle>::New();
  triangle3->GetPointIds()->SetId(0, 0);
  triangle3->GetPointIds()->SetId(1, 1);
  triangle3->GetPointIds()->SetId(2, 4);

  triangles->InsertNextCell(triangle2);
  triangles->InsertNextCell(triangle3);
  triangles->Modified();

  svtkSmartPointer<svtkPolyData> nonManifoldPolyData = svtkSmartPointer<svtkPolyData>::New();
  nonManifoldPolyData->SetPoints(points);
  nonManifoldPolyData->SetPolys(triangles);

  subdivision0->SetInputData(nonManifoldPolyData);
  subdivision0->Modified();
  subdivision0->Update();
  int status3 =
    errorObserver->CheckErrorMessage("Dataset is non-manifold and cannot be subdivided");
  if (status3 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  std::cout << "  Testing non-triangles...";
  svtkSmartPointer<svtkQuad> quad = svtkSmartPointer<svtkQuad>::New();
  quad->GetPointIds()->SetId(0, 0);
  quad->GetPointIds()->SetId(1, 1);
  quad->GetPointIds()->SetId(2, 2);
  quad->GetPointIds()->SetId(3, 3);

  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();
  cells->InsertNextCell(triangle);
  cells->InsertNextCell(quad);

  svtkSmartPointer<svtkPolyData> mixedPolyData = svtkSmartPointer<svtkPolyData>::New();
  mixedPolyData->SetPoints(points);
  mixedPolyData->SetPolys(cells);
  subdivision0->SetInputData(mixedPolyData);
  subdivision0->Update();

  int status4 = errorObserver->CheckErrorMessage(
    "only operates on triangles, but this data set has other cell types present");
  if (status4 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  std::cout << "PASSED" << std::endl;
  // End of test
  if (status)
  {
    std::cout << subdivision0->GetClassName() << " FAILED" << std::endl;
  }
  else
  {
    std::cout << subdivision0->GetClassName() << " PASSED" << std::endl;
  }

  return status;
}
