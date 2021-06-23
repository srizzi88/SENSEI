/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSuperquadricSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>
#include <svtkSuperquadricSource.h>

int TestSuperquadricSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkSuperquadricSource> superquadricSource =
    svtkSmartPointer<svtkSuperquadricSource>::New();
  superquadricSource->SetThetaResolution(8);
  superquadricSource->SetPhiResolution(8);
  superquadricSource->SetThetaRoundness(1.0);
  superquadricSource->SetPhiRoundness(1.0);
  superquadricSource->SetYAxisOfSymmetry();
  superquadricSource->ToroidalOff();

  superquadricSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  superquadricSource->SetCenter(center);

  double scale[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    scale[i] = randomSequence->GetValue();
  }
  superquadricSource->SetScale(scale);

  superquadricSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = superquadricSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  superquadricSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  superquadricSource->SetCenter(center);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    scale[i] = randomSequence->GetValue();
  }
  superquadricSource->SetScale(scale);

  superquadricSource->Update();

  polyData = superquadricSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
