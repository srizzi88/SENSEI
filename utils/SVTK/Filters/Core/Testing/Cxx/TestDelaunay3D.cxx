/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkDelaunay3D.h>
#include <svtkMinimalStandardRandomSequence.h>
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
      cells->InsertCellPoint(points->InsertNextPoint(point));
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
      cells->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(SVTK_VERTEX, cells);
}

int Delaunay3D(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkUnstructuredGrid> inputUnstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(inputUnstructuredGrid, dataType);

  svtkSmartPointer<svtkDelaunay3D> delaunay = svtkSmartPointer<svtkDelaunay3D>::New();
  delaunay->SetOutputPointsPrecision(outputPointsPrecision);
  delaunay->SetInputData(inputUnstructuredGrid);

  delaunay->Update();

  svtkSmartPointer<svtkUnstructuredGrid> outputUnstructuredGrid = delaunay->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputUnstructuredGrid->GetPoints();

  return points->GetDataType();
}
}

int TestDelaunay3D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = Delaunay3D(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
