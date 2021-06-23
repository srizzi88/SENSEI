/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStripper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellArray.h"
#include "svtkIntersectionPolyDataFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStripper.h"
#include <cassert>

bool TestSpherePlaneIntersection(bool joinSegments)
{
  // Sphere
  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(2.0f);
  sphereSource->SetPhiResolution(20);
  sphereSource->SetThetaResolution(20);
  sphereSource->Update();

  // Plane
  svtkSmartPointer<svtkPoints> PlanePoints = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> PlaneCells = svtkSmartPointer<svtkCellArray>::New();
  // 4 points
  PlanePoints->InsertNextPoint(-3, -1, 0);
  PlanePoints->InsertNextPoint(3, -1, 0);
  PlanePoints->InsertNextPoint(-3, 1, 0);
  PlanePoints->InsertNextPoint(3, 1, 0);
  // 2 triangles
  PlaneCells->InsertNextCell(3);
  PlaneCells->InsertCellPoint(0);
  PlaneCells->InsertCellPoint(1);
  PlaneCells->InsertCellPoint(2);

  PlaneCells->InsertNextCell(3);
  PlaneCells->InsertCellPoint(1);
  PlaneCells->InsertCellPoint(3);
  PlaneCells->InsertCellPoint(2);

  // Create the polydata from points and faces
  svtkSmartPointer<svtkPolyData> Plane = svtkSmartPointer<svtkPolyData>::New();
  Plane->SetPoints(PlanePoints);
  Plane->SetPolys(PlaneCells);

  // Intersect plane with sphere, get lines
  svtkSmartPointer<svtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
    svtkSmartPointer<svtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SplitFirstOutputOff();
  intersectionPolyDataFilter->SplitSecondOutputOff();
  intersectionPolyDataFilter->SetInputConnection(0, sphereSource->GetOutputPort());
  intersectionPolyDataFilter->SetInputData(1, Plane);
  intersectionPolyDataFilter->Update();

  // Get the polylines from the segments
  svtkSmartPointer<svtkStripper> stripper = svtkSmartPointer<svtkStripper>::New();
  stripper->SetInputConnection(intersectionPolyDataFilter->GetOutputPort());

  if (joinSegments)
  {
    stripper->SetJoinContiguousSegments(true);
  }

  stripper->Update();
  svtkSmartPointer<svtkPolyDataMapper> intersectionMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection(stripper->GetOutputPort());

  if (joinSegments)
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 2)
    {
      return true;
    }
  }
  else
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 6)
    {
      return false;
    }
  }

  return true;
}

int TestStripper(int, char*[])
{
  if (!TestSpherePlaneIntersection(false))
  {
    return EXIT_FAILURE;
  }

  if (!TestSpherePlaneIntersection(true))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
