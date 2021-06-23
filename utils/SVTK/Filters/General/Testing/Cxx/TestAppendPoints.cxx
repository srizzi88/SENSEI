/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkAppendPoints.h>
#include <svtkCellArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>

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

int AppendPolyDataPoints(int dataType0, int dataType1, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPolyData> polyData0 = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(polyData0, dataType0);

  svtkSmartPointer<svtkPolyData> polyData1 = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(polyData1, dataType1);

  svtkSmartPointer<svtkAppendPoints> appendPoints = svtkSmartPointer<svtkAppendPoints>::New();
  appendPoints->SetOutputPointsPrecision(outputPointsPrecision);

  appendPoints->AddInputData(polyData0);
  appendPoints->AddInputData(polyData1);

  appendPoints->Update();

  svtkSmartPointer<svtkPointSet> pointSet = appendPoints->GetOutput();
  svtkSmartPointer<svtkPoints> points = pointSet->GetPoints();

  return points->GetDataType();
}
}

int TestAppendPoints(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = AppendPolyDataPoints(SVTK_FLOAT, SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_FLOAT, SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_FLOAT, SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(SVTK_DOUBLE, SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
