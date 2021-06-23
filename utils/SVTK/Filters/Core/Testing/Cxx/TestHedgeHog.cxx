/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHedgeHog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkFloatArray.h>
#include <svtkHedgeHog.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPointData.h>
#include <svtkSmartPointer.h>
#include <svtkUnstructuredGrid.h>

namespace
{
void InitializeUnstructuredGrid(svtkUnstructuredGrid* unstructuredGrid, int dataType)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();
  cells->InsertNextCell(4);
  svtkSmartPointer<svtkFloatArray> vectors = svtkSmartPointer<svtkFloatArray>::New();
  vectors->SetNumberOfComponents(3);

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      double vector[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        vector[j] = randomSequence->GetValue();
      }
      vectors->InsertNextTuple(vector);
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      cells->InsertCellPoint(points->InsertNextPoint(point));
    }
  }
  else
  {
    points->SetDataType(SVTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      float vector[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        vector[j] = static_cast<float>(randomSequence->GetValue());
      }
      vectors->InsertNextTuple(vector);
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      cells->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  vectors->Squeeze();
  unstructuredGrid->GetPointData()->SetVectors(vectors);
  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(SVTK_VERTEX, cells);
}

int HedgeHog(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(unstructuredGrid, dataType);

  svtkSmartPointer<svtkHedgeHog> hedgeHog = svtkSmartPointer<svtkHedgeHog>::New();
  hedgeHog->SetOutputPointsPrecision(outputPointsPrecision);
  hedgeHog->SetInputData(unstructuredGrid);

  hedgeHog->Update();

  svtkSmartPointer<svtkPolyData> polyData = hedgeHog->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  return points->GetDataType();
}
}

int TestHedgeHog(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = HedgeHog(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = HedgeHog(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = HedgeHog(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = HedgeHog(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = HedgeHog(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = HedgeHog(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
