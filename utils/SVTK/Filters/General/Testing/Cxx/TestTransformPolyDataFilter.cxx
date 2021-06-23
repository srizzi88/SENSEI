/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTransformPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>
#include <svtkTransform.h>
#include <svtkTransformPolyDataFilter.h>

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

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
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
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
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

int TransformPolyData(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  svtkSmartPointer<svtkTransform> transform = svtkSmartPointer<svtkTransform>::New();
  InitializeTransform(transform);

  svtkSmartPointer<svtkTransformPolyDataFilter> transformPolyDataFilter =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  transformPolyDataFilter->SetOutputPointsPrecision(outputPointsPrecision);

  transformPolyDataFilter->SetTransform(transform);
  transformPolyDataFilter->SetInputData(inputPolyData);

  transformPolyDataFilter->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = transformPolyDataFilter->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

int TestTransformPolyDataFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = TransformPolyData(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
