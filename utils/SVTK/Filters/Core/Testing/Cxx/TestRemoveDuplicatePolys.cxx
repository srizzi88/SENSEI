/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRemoveDuplicatePolys.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkRemoveDuplicatePolys.h>
#include <svtkSmartPointer.h>

int TestRemoveDuplicatePolys(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();

  for (unsigned int i = 0; i < 3; ++i)
  {
    double point[3];
    for (unsigned int j = 0; j < 3; ++j)
    {
      randomSequence->Next();
      point[j] = randomSequence->GetValue();
    }
    points->InsertNextPoint(point);
  }

  verts->InsertNextCell(3);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(2);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(0);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(1);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(1);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(0);
  verts->InsertCellPoint(2);

  verts->InsertNextCell(3);
  verts->InsertCellPoint(2);
  verts->InsertCellPoint(1);
  verts->InsertCellPoint(0);

  points->Squeeze();
  verts->Squeeze();

  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  inputPolyData->SetPoints(points);
  inputPolyData->SetPolys(verts);

  svtkSmartPointer<svtkRemoveDuplicatePolys> removePolyData =
    svtkSmartPointer<svtkRemoveDuplicatePolys>::New();
  removePolyData->SetInputData(inputPolyData);

  removePolyData->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = removePolyData->GetOutput();

  if (outputPolyData->GetNumberOfPolys() != 1)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
