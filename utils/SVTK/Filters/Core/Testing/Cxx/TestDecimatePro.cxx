/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecimatePro.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkDecimatePro.h>
#include <svtkSmartPointer.h>

namespace
{
void InitializePolyData(svtkPolyData* polyData, int dataType)
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataType(dataType);
  points->InsertNextPoint(-1.40481710, -0.03868163, -1.01241910);
  points->InsertNextPoint(-1.41186166, 0.29086590, 0.96023101);
  points->InsertNextPoint(-0.13218975, -1.22439861, 1.21793830);
  points->InsertNextPoint(-0.12514521, -1.55394614, -0.75471181);
  points->InsertNextPoint(0.13218975, 1.22439861, -1.21793830);
  points->InsertNextPoint(0.12514521, 1.55394614, 0.75471181);
  points->InsertNextPoint(1.40481710, 0.03868163, 1.01241910);
  points->InsertNextPoint(1.41186166, -0.29086590, -0.96023101);
  points->Squeeze();

  polyData->SetPoints(points);

  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  verts->InsertNextCell(8);
  for (unsigned int i = 0; i < 8; ++i)
  {
    verts->InsertCellPoint(i);
  }
  verts->Squeeze();

  polyData->SetVerts(verts);

  svtkSmartPointer<svtkCellArray> polys = svtkSmartPointer<svtkCellArray>::New();
  svtkIdType pointIds[3];
  pointIds[0] = 0;
  pointIds[1] = 1;
  pointIds[2] = 2;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 2;
  pointIds[2] = 3;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 3;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 4;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 5;
  pointIds[2] = 1;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 7;
  pointIds[2] = 4;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 2;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 6;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 2;
  pointIds[1] = 3;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 3;
  pointIds[1] = 7;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 5;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 6;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  polys->Squeeze();

  polyData->SetPolys(polys);
}

int DecimatePro(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  svtkSmartPointer<svtkDecimatePro> decimatePro = svtkSmartPointer<svtkDecimatePro>::New();
  decimatePro->SetOutputPointsPrecision(outputPointsPrecision);
  decimatePro->SetInputData(inputPolyData);

  decimatePro->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = decimatePro->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}
}

int TestDecimatePro(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = DecimatePro(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
