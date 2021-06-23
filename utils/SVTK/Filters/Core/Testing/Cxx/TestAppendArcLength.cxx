/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAppendArcLength.h"
#include "svtkCellArray.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include <array>

namespace
{
void InitializePolyData(svtkPolyData* polyData)
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataType(SVTK_DOUBLE);
  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->InsertNextCell(3);
  lines->InsertCellPoint(points->InsertNextPoint(0, 0, 0));
  lines->InsertCellPoint(points->InsertNextPoint(1.1, 0, 0));
  lines->InsertCellPoint(points->InsertNextPoint(3.3, 0, 0));

  lines->InsertNextCell(2);
  lines->InsertCellPoint(points->InsertNextPoint(0, 1, 0));
  lines->InsertCellPoint(points->InsertNextPoint(2.2, 1, 0));

  polyData->SetPoints(points);
  polyData->SetLines(lines);
}
}

/**
 * Tests if svtkAppendArcLength adds a point array called arc_length which,
 * computes the distance from the first point in the polyline.
 */
int TestAppendArcLength(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkPolyData> inputData = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(inputData);

  svtkSmartPointer<svtkAppendArcLength> arcLengthFilter = svtkSmartPointer<svtkAppendArcLength>::New();
  arcLengthFilter->SetInputDataObject(inputData);
  arcLengthFilter->Update();
  svtkDataSet* data = svtkDataSet::SafeDownCast(arcLengthFilter->GetOutputDataObject(0));

  std::array<double, 5> expected = { { 0, 1.1, 3.3, 0, 2.2 } };

  svtkDataArray* arcLength = data->GetPointData()->GetArray("arc_length");
  if (arcLength == nullptr)
  {
    std::cerr << "No arc_length array.\n";
    return EXIT_FAILURE;
  }
  if (arcLength->GetNumberOfComponents() != 1 ||
    static_cast<size_t>(arcLength->GetNumberOfTuples()) != expected.size())
  {
    std::cerr << "Invalid size or number of components.\n";
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < expected.size(); ++i)
    if (arcLength->GetTuple(static_cast<svtkIdType>(i))[0] != expected[i])
    {
      std::cerr << "Invalid value: " << arcLength->GetTuple(static_cast<svtkIdType>(i))[0]
                << " expecting: " << expected[i] << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
