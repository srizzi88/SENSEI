/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRegularPolygonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkRegularPolygonSource.h>
#include <svtkSmartPointer.h>

int TestRegularPolygonSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkRegularPolygonSource> regularPolygonSource =
    svtkSmartPointer<svtkRegularPolygonSource>::New();
  regularPolygonSource->SetNumberOfSides(8);
  regularPolygonSource->GeneratePolygonOn();
  regularPolygonSource->GeneratePolylineOn();

  regularPolygonSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  regularPolygonSource->SetRadius(radius);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  regularPolygonSource->SetCenter(center);

  regularPolygonSource->Update();

  double normal[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  regularPolygonSource->SetNormal(normal);

  regularPolygonSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = regularPolygonSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  regularPolygonSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  regularPolygonSource->SetRadius(radius);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  regularPolygonSource->SetCenter(center);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  regularPolygonSource->SetNormal(normal);

  regularPolygonSource->Update();

  polyData = regularPolygonSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
