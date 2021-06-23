/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkConnectivityFilter.h>
#include <svtkFloatArray.h>
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
      cells->InsertCellPoint(points->InsertNextPoint(point));
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
      cells->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  scalars->Squeeze();
  unstructuredGrid->GetPointData()->SetScalars(scalars);
  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(SVTK_VERTEX, cells);
}

int FilterUnstructuredGridConnectivity(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkUnstructuredGrid> inputUnstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(inputUnstructuredGrid, dataType);

  svtkSmartPointer<svtkConnectivityFilter> connectivityFilter =
    svtkSmartPointer<svtkConnectivityFilter>::New();
  connectivityFilter->SetOutputPointsPrecision(outputPointsPrecision);
  connectivityFilter->ScalarConnectivityOn();
  connectivityFilter->SetScalarRange(0.25, 0.75);
  connectivityFilter->SetInputData(inputUnstructuredGrid);

  connectivityFilter->Update();

  svtkSmartPointer<svtkPointSet> outputUnstructuredGrid = connectivityFilter->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputUnstructuredGrid->GetPoints();

  return points->GetDataType();
}
}

int TestConnectivityFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = FilterUnstructuredGridConnectivity(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterUnstructuredGridConnectivity(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterUnstructuredGridConnectivity(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterUnstructuredGridConnectivity(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterUnstructuredGridConnectivity(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterUnstructuredGridConnectivity(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
