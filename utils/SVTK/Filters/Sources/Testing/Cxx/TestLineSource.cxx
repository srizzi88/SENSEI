/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkLineSource.h>
#include <svtkLogger.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>
#include <svtkVector.h>
#include <svtkVectorOperators.h>

int TestLineSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkLineSource> lineSource = svtkSmartPointer<svtkLineSource>::New();
  lineSource->SetResolution(8);

  lineSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double point1[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint1(point1);

  double point2[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint2(point2);

  lineSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = lineSource->GetOutput();
  svtkSmartPointer<svtkPoints> outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint1(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint2(point2);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
  inputPoints->SetDataType(SVTK_DOUBLE);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point2);

  lineSource->SetPoints(inputPoints);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  inputPoints->Reset();

  lineSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point2);

  lineSource->SetPoints(inputPoints);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();
  if (outputPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetPoints(nullptr);
  lineSource->SetPoint1(0, 0, 0);
  lineSource->SetPoint2(1, 1, 2);
  lineSource->SetNumberOfRefinementRatios(3);
  lineSource->SetRefinementRatio(0, 0.1);
  lineSource->SetRefinementRatio(1, 0.7);
  lineSource->SetRefinementRatio(2, 1.0);
  lineSource->SetUseRegularRefinement(false);
  lineSource->SetResolution(10);
  lineSource->Update();
  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();
  if (outputPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  if (outputPoints->GetNumberOfPoints() != 3)
  {
    svtkLogF(ERROR, "incorrect number of points (expected 3: got: %d)",
      static_cast<int>(outputPoints->GetNumberOfPoints()));
    return EXIT_FAILURE;
  }

  const svtkVector3d expected(0.7, 0.7, 1.4);
  svtkVector3d pt;
  outputPoints->GetPoint(1, pt.GetData());
  if (pt != expected)
  {
    svtkLogF(ERROR, "incorrect point (expected (%g, %g, %g): got: (%g, %g, %g))", expected[0],
      expected[1], expected[2], pt[0], pt[1], pt[2]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
