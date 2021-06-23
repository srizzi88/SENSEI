/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOutlineCornerSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkOutlineCornerSource.h>
#include <svtkSmartPointer.h>

int TestOutlineCornerSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkOutlineCornerSource> outlineCornerSource =
    svtkSmartPointer<svtkOutlineCornerSource>::New();
  outlineCornerSource->SetBoxTypeToAxisAligned();
  outlineCornerSource->GenerateFacesOff();

  outlineCornerSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double bounds[6];
  for (unsigned int i = 0; i < 6; ++i)
  {
    randomSequence->Next();
    bounds[i] = randomSequence->GetValue();
  }
  if (bounds[0] > bounds[3])
  {
    std::swap(bounds[0], bounds[3]);
  }
  if (bounds[1] > bounds[4])
  {
    std::swap(bounds[1], bounds[4]);
  }
  if (bounds[2] > bounds[5])
  {
    std::swap(bounds[2], bounds[5]);
  }
  outlineCornerSource->SetBounds(bounds);

  randomSequence->Next();
  double cornerFactor = randomSequence->GetValue();
  outlineCornerSource->SetCornerFactor(cornerFactor);

  outlineCornerSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = outlineCornerSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  outlineCornerSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 6; ++i)
  {
    randomSequence->Next();
    bounds[i] = randomSequence->GetValue();
  }
  if (bounds[0] > bounds[3])
  {
    std::swap(bounds[0], bounds[3]);
  }
  if (bounds[1] > bounds[4])
  {
    std::swap(bounds[1], bounds[4]);
  }
  if (bounds[2] > bounds[5])
  {
    std::swap(bounds[2], bounds[5]);
  }
  outlineCornerSource->SetBounds(bounds);

  randomSequence->Next();
  cornerFactor = randomSequence->GetValue();
  outlineCornerSource->SetCornerFactor(cornerFactor);

  outlineCornerSource->Update();

  polyData = outlineCornerSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
