/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlatonicSolidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkPlatonicSolidSource.h>
#include <svtkSmartPointer.h>

int TestPlatonicSolidSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkPlatonicSolidSource> platonicSolidSource =
    svtkSmartPointer<svtkPlatonicSolidSource>::New();

  platonicSolidSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  platonicSolidSource->SetSolidTypeToCube();
  platonicSolidSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = platonicSolidSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToDodecahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToIcosahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToOctahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToTetrahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  platonicSolidSource->SetSolidTypeToCube();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToDodecahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToIcosahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToOctahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  platonicSolidSource->SetSolidTypeToTetrahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
