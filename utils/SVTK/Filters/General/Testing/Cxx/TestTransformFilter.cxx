/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkFloatArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkSmartPointer.h>
#include <svtkTransform.h>
#include <svtkTransformFilter.h>

namespace
{
void InitializePointSet(svtkPointSet* pointSet, int dataType)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  const int numPoints = 4;

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < numPoints; ++i)
    {
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      points->InsertNextPoint(point);
    }
  }
  else
  {
    points->SetDataType(SVTK_FLOAT);
    for (unsigned int i = 0; i < numPoints; ++i)
    {
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      points->InsertNextPoint(point);
    }
  }

  // Add texture coordinates. Values don't matter, we just want to make sure
  // they are passed through the transform filter.
  svtkSmartPointer<svtkFloatArray> tcoords = svtkSmartPointer<svtkFloatArray>::New();
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(numPoints);
  tcoords->FillComponent(0, 0.0);
  tcoords->FillComponent(1, 1.0);
  pointSet->GetPointData()->SetTCoords(tcoords);

  points->Squeeze();
  pointSet->SetPoints(points);
}

void InitializeTransform(svtkTransform* transform)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  double elements[16];
  for (unsigned int i = 0; i < 16; ++i)
  {
    randomSequence->Next();
    elements[i] = randomSequence->GetValue();
  }
  transform->SetMatrix(elements);
}
}

svtkSmartPointer<svtkPointSet> TransformPointSet(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPointSet> inputPointSet = svtkSmartPointer<svtkPolyData>::New();
  InitializePointSet(inputPointSet, dataType);

  svtkSmartPointer<svtkTransform> transform = svtkSmartPointer<svtkTransform>::New();
  InitializeTransform(transform);

  svtkSmartPointer<svtkTransformFilter> transformFilter = svtkSmartPointer<svtkTransformFilter>::New();
  transformFilter->SetTransformAllInputVectors(true);
  transformFilter->SetOutputPointsPrecision(outputPointsPrecision);

  transformFilter->SetTransform(transform);
  transformFilter->SetInputData(inputPointSet);

  transformFilter->Update();

  svtkSmartPointer<svtkPointSet> outputPointSet = transformFilter->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPointSet->GetPoints();

  return outputPointSet;
}

int TestTransformFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkPointSet> pointSet =
    TransformPointSet(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  pointSet = TransformPointSet(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  pointSet = TransformPointSet(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  if (pointSet->GetPointData()->GetTCoords() == nullptr)
  {
    std::cerr << "TCoords were not passed through svtkTransformFilter." << std::endl;
    return EXIT_FAILURE;
  }

  pointSet = TransformPointSet(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  pointSet = TransformPointSet(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  pointSet = TransformPointSet(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (pointSet->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
