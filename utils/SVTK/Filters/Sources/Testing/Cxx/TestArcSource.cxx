/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArcSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkArcSource.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>

int TestArcSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkArcSource> arcSource = svtkSmartPointer<svtkArcSource>::New();
  arcSource->SetAngle(90.0);
  arcSource->SetResolution(8);
  arcSource->NegativeOff();
  arcSource->UseNormalAndAngleOn();

  arcSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double normal[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  arcSource->SetNormal(normal);

  double polarVector[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    polarVector[i] = randomSequence->GetValue();
  }
  arcSource->SetPolarVector(polarVector);

  arcSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = arcSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  arcSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  arcSource->SetNormal(normal);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    polarVector[i] = randomSequence->GetValue();
  }
  arcSource->SetPolarVector(polarVector);

  arcSource->Update();

  polyData = arcSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
