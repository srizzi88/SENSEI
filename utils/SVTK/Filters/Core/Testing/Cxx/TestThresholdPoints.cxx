/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThresholdPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkFloatArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPointData.h>
#include <svtkSmartPointer.h>
#include <svtkThresholdPoints.h>

namespace
{
void InitializePolyData(svtkPolyData* polyData, int dataType)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  verts->InsertNextCell(4);
  svtkSmartPointer<svtkFloatArray> scalars = svtkSmartPointer<svtkFloatArray>::New();

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }
  else
  {
    points->SetDataType(SVTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  scalars->Squeeze();
  polyData->GetPointData()->SetScalars(scalars);
  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

int ThresholdPolyDataPoints(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  svtkSmartPointer<svtkThresholdPoints> thresholdPoints = svtkSmartPointer<svtkThresholdPoints>::New();
  thresholdPoints->SetOutputPointsPrecision(outputPointsPrecision);
  thresholdPoints->ThresholdByUpper(0.5);
  thresholdPoints->SetInputData(inputPolyData);

  thresholdPoints->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = thresholdPoints->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}
}

int TestThresholdPoints(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = ThresholdPolyDataPoints(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = ThresholdPolyDataPoints(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = ThresholdPolyDataPoints(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = ThresholdPolyDataPoints(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = ThresholdPolyDataPoints(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = ThresholdPolyDataPoints(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
