/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkConeSource.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>

int TestConeSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  coneSource->SetResolution(8);
  coneSource->CappingOn();

  coneSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  coneSource->SetCenter(center);

  double direction[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    direction[i] = randomSequence->GetValue();
  }
  coneSource->SetDirection(direction);

  randomSequence->Next();
  double height = randomSequence->GetValue();
  coneSource->SetHeight(height);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  coneSource->SetRadius(radius);

  coneSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = coneSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  coneSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  coneSource->SetCenter(center);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    direction[i] = randomSequence->GetValue();
  }
  coneSource->SetDirection(direction);

  randomSequence->Next();
  height = randomSequence->GetValue();
  coneSource->SetHeight(height);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  coneSource->SetRadius(radius);

  coneSource->Update();

  polyData = coneSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
