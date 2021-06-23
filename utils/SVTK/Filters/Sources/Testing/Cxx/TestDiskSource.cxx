/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiskSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkDiskSource.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>

int TestDiskSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkDiskSource> diskSource = svtkSmartPointer<svtkDiskSource>::New();
  diskSource->SetCircumferentialResolution(8);
  diskSource->SetRadialResolution(8);

  diskSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double innerRadius = randomSequence->GetValue();

  randomSequence->Next();
  double outerRadius = randomSequence->GetValue();

  if (innerRadius > outerRadius)
  {
    std::swap(innerRadius, outerRadius);
  }

  diskSource->SetInnerRadius(innerRadius);
  diskSource->SetOuterRadius(outerRadius);

  diskSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = diskSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  diskSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  innerRadius = randomSequence->GetValue();

  randomSequence->Next();
  outerRadius = randomSequence->GetValue();

  if (innerRadius > outerRadius)
  {
    std::swap(innerRadius, outerRadius);
  }

  diskSource->SetInnerRadius(innerRadius);
  diskSource->SetOuterRadius(outerRadius);

  diskSource->Update();

  polyData = diskSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
