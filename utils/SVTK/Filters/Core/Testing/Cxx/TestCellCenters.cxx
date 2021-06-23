/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellCenters.h>
#include <svtkCellTypeSource.h>
#include <svtkEmptyCell.h>
#include <svtkNew.h>
#include <svtkPyramid.h>
#include <svtkTetra.h>
#include <svtkUnstructuredGrid.h>

#include <cstdlib>

namespace
{

// Copied from Filters/General/Testing/Cxx/TestCellValidator.cxx
svtkSmartPointer<svtkEmptyCell> MakeEmptyCell()
{
  svtkSmartPointer<svtkEmptyCell> anEmptyCell = svtkSmartPointer<svtkEmptyCell>::New();
  return anEmptyCell;
}

svtkSmartPointer<svtkTetra> MakeTetra()
{
  svtkSmartPointer<svtkTetra> aTetra = svtkSmartPointer<svtkTetra>::New();
  aTetra->GetPointIds()->SetId(0, 0);
  aTetra->GetPointIds()->SetId(1, 1);
  aTetra->GetPointIds()->SetId(2, 2);
  aTetra->GetPointIds()->SetId(3, 3);
  aTetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);
  return aTetra;
}

svtkSmartPointer<svtkPyramid> MakePyramid()
{
  svtkSmartPointer<svtkPyramid> aPyramid = svtkSmartPointer<svtkPyramid>::New();
  aPyramid->GetPointIds()->SetId(0, 0);
  aPyramid->GetPointIds()->SetId(1, 1);
  aPyramid->GetPointIds()->SetId(2, 2);
  aPyramid->GetPointIds()->SetId(3, 3);
  aPyramid->GetPointIds()->SetId(4, 4);

  aPyramid->GetPoints()->SetPoint(0, 0, 0, 0);
  aPyramid->GetPoints()->SetPoint(1, 1, 0, 0);
  aPyramid->GetPoints()->SetPoint(2, 1, 1, 0);
  aPyramid->GetPoints()->SetPoint(3, 0, 1, 0);
  aPyramid->GetPoints()->SetPoint(4, .5, .5, 1);

  return aPyramid;
}

} // end namespace

int TestCellCenters(int, char*[])
{
  // Check centers of cells in an unstructured grid
  svtkNew<svtkCellTypeSource> cellTypeSource;
  int blockDimensions = 2;
  cellTypeSource->SetBlocksDimensions(blockDimensions, blockDimensions, blockDimensions);
  // Use a fun cell type
  cellTypeSource->SetCellType(SVTK_QUADRATIC_HEXAHEDRON);
  cellTypeSource->Update();

  svtkNew<svtkCellCenters> cellCenters;
  cellCenters->SetInputConnection(cellTypeSource->GetOutputPort());
  cellCenters->Update();

  svtkPointSet* cellCentersOutput = cellCenters->GetOutput();

  for (svtkIdType i = 0; i < cellCentersOutput->GetNumberOfPoints(); ++i)
  {
    double pt[3];
    cellCentersOutput->GetPoint(i, pt);
  }

  double centers[2] = { 0.5, 1.5 };
  for (svtkIdType k = 0; k < blockDimensions; ++k)
  {
    for (svtkIdType j = 0; j < blockDimensions; ++j)
    {
      for (svtkIdType i = 0; i < blockDimensions; ++i)
      {
        svtkIdType cellId = blockDimensions * blockDimensions * k + blockDimensions * j + i;
        double centerPt[3];
        cellCentersOutput->GetPoint(cellId, centerPt);
        double expectedPt[3] = { centers[i], centers[j], centers[k] };
        if (svtkMath::Distance2BetweenPoints(centerPt, expectedPt) > 1e-6)
        {
          std::cerr << "Error in point center calculation" << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  // Test handling of SVTK_EMPTY_CELL
  svtkSmartPointer<svtkCell> emptyCell = MakeEmptyCell();
  svtkSmartPointer<svtkTetra> tetra = MakeTetra();
  svtkSmartPointer<svtkPyramid> pyramid = MakePyramid();

  svtkNew<svtkPoints> points;
  points->InsertNextPoint(1, 2, 3);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 1, 3);
  points->InsertNextPoint(3, 2, 1);
  points->InsertNextPoint(1, 3, 1);

  svtkNew<svtkUnstructuredGrid> ugrid;
  ugrid->Allocate(20);
  ugrid->SetPoints(points);
  ugrid->InsertNextCell(
    emptyCell->GetCellType(), emptyCell->GetNumberOfPoints(), emptyCell->GetPointIds()->begin());
  ugrid->InsertNextCell(
    emptyCell->GetCellType(), emptyCell->GetNumberOfPoints(), emptyCell->GetPointIds()->begin());
  ugrid->InsertNextCell(
    tetra->GetCellType(), tetra->GetNumberOfPoints(), tetra->GetPointIds()->begin());
  ugrid->InsertNextCell(
    emptyCell->GetCellType(), emptyCell->GetNumberOfPoints(), emptyCell->GetPointIds()->begin());
  ugrid->InsertNextCell(
    pyramid->GetCellType(), pyramid->GetNumberOfPoints(), pyramid->GetPointIds()->begin());
  ugrid->InsertNextCell(
    emptyCell->GetCellType(), emptyCell->GetNumberOfPoints(), emptyCell->GetPointIds()->begin());

  cellCenters->SetInputData(ugrid);
  cellCenters->Update();

  svtkPointSet* pointSet = cellCenters->GetOutput();

  if (pointSet->GetNumberOfPoints() != 2)
  {
    std::cerr << "Empty cells were not ignored in the output" << std::endl;
    return EXIT_FAILURE;
  }

  for (svtkIdType id = 0; id < pointSet->GetNumberOfPoints(); ++id)
  {
    double pt[3];
    pointSet->GetPoint(id, pt);
    if (pt[0] == 0.0 || pt[1] == 0.0 || pt[2] == 0.0)
    {
      std::cerr << pt[0] << ", " << pt[1] << ", " << pt[2] << std::endl;
      std::cerr << "Non-empty cells should not have coordinates of 0.0" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
