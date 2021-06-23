/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDijkstraGraphGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAppendPolyData.h"
#include "svtkDijkstraGraphGeodesicPath.h"
#include "svtkNew.h"
#include "svtkSphereSource.h"

int TestDijkstraGraphGeodesicPath(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkSphereSource> sphere1;
  sphere1->SetCenter(10, 10, 10);
  sphere1->SetRadius(5.0);

  svtkNew<svtkAppendPolyData> appendFilter;
  appendFilter->AddInputConnection(sphere1->GetOutputPort());
  appendFilter->Update();

  svtkPolyData* polyData = appendFilter->GetOutput();

  svtkNew<svtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetInputData(polyData);
  pathFilter->SetStartVertex(0);
  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  // Valid path from the first to last point on a single sphere
  svtkPolyData* path1 = pathFilter->GetOutput();
  if (!path1 || !path1->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return EXIT_FAILURE;
  }
  if (path1->GetPoints()->GetNumberOfPoints() < 1)
  {
    std::cerr << "Could not find valid a path!" << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkSphereSource> sphere2;
  sphere2->SetCenter(-10, -10, -10);
  sphere2->SetRadius(2.0);
  appendFilter->AddInputConnection(sphere2->GetOutputPort());
  appendFilter->Update();

  polyData = appendFilter->GetOutput();
  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  // No path should exist between the two separate spheres
  svtkPolyData* path2 = pathFilter->GetOutput();
  if (!path2 || !path2->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return EXIT_FAILURE;
  }
  if (path2->GetPoints()->GetNumberOfPoints() > 0)
  {
    std::cerr << "Invalid path was expected, however a valid path was found!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
