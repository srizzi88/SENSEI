/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCylinderSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCylinderSource.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>

int TestCylinderSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkCylinderSource> cylinderSource = svtkSmartPointer<svtkCylinderSource>::New();
  cylinderSource->SetResolution(8);
  cylinderSource->CappingOn();

  cylinderSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  cylinderSource->SetCenter(center);

  randomSequence->Next();
  double height = randomSequence->GetValue();
  cylinderSource->SetHeight(height);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  cylinderSource->SetRadius(radius);

  cylinderSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = cylinderSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  cylinderSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  cylinderSource->SetCenter(center);

  randomSequence->Next();
  height = randomSequence->GetValue();
  cylinderSource->SetHeight(height);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  cylinderSource->SetRadius(radius);

  cylinderSource->Update();

  polyData = cylinderSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
