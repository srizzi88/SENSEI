/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendLocationAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkAppendLocationAttributes.h>

#include <svtkCellCenters.h>
#include <svtkCellData.h>
#include <svtkCellTypeSource.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkUnstructuredGrid.h>

#include <cstdlib>

int TestAppendLocationAttributes(int, char*[])
{
  // Reference dataset
  svtkNew<svtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetBlocksDimensions(10, 10, 10);
  cellTypeSource->Update();
  svtkUnstructuredGrid* inputUG = cellTypeSource->GetOutput();

  // Create a svtkCellCenters object and use it to test the cell centers calculation in
  // svtkAppendLocationAttributes.
  svtkNew<svtkCellCenters> cellCenters;
  cellCenters->SetInputConnection(cellTypeSource->GetOutputPort());
  cellCenters->Update();

  svtkPointSet* cellCentersOutput = cellCenters->GetOutput();

  svtkNew<svtkAppendLocationAttributes> locationAttributes;
  locationAttributes->SetInputConnection(cellTypeSource->GetOutputPort());
  locationAttributes->Update();

  svtkPointSet* appendLocationOutput = svtkPointSet::SafeDownCast(locationAttributes->GetOutput());

  svtkIdType numCells = appendLocationOutput->GetNumberOfCells();
  svtkIdType numPoints = appendLocationOutput->GetNumberOfPoints();
  if (numCells != inputUG->GetNumberOfCells())
  {
    std::cerr << "Output number of cells is incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  if (numPoints != inputUG->GetNumberOfPoints())
  {
    std::cerr << "Output number of points is incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  svtkPoints* cellCenterPoints = cellCentersOutput->GetPoints();
  svtkDataArray* cellCentersArray = appendLocationOutput->GetCellData()->GetArray("CellCenters");
  svtkDataArray* pointLocationsArray =
    appendLocationOutput->GetPointData()->GetArray("PointLocations");

  for (svtkIdType i = 0; i < numCells; ++i)
  {
    double cellCenter[3];
    cellCenterPoints->GetPoint(i, cellCenter);

    double appendLocationCenter[3];
    cellCentersArray->GetTuple(i, appendLocationCenter);

    double dist2 = svtkMath::Distance2BetweenPoints(cellCenter, appendLocationCenter);
    if (dist2 > 1e-9)
    {
      std::cerr << "Cell center mismatch for cell " << i << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    double inputPoint[3];
    inputUG->GetPoints()->GetPoint(i, inputPoint);

    double appendLocationPoint[3];
    pointLocationsArray->GetTuple(i, appendLocationPoint);

    double dist2 = svtkMath::Distance2BetweenPoints(inputPoint, appendLocationPoint);
    if (dist2 > 1e-9)
    {
      std::cerr << "Point location mismatch for point " << i << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with svtkImageData
  svtkNew<svtkImageData> image;
  image->SetDimensions(10, 10, 10);
  image->AllocateScalars(SVTK_FLOAT, 1);

  locationAttributes->SetInputData(image);
  locationAttributes->Update();
  svtkImageData* imageWithLocations = locationAttributes->GetImageDataOutput();

  svtkPointData* imagePD = imageWithLocations->GetPointData();
  if (imagePD == nullptr)
  {
    std::cerr << "'PointLocations' array not added to svtkImageData point data" << std::endl;
    return EXIT_FAILURE;
  }

  svtkCellData* imageCD = imageWithLocations->GetCellData();
  if (imageCD == nullptr)
  {
    std::cerr << "'CellCenters' array not added to svtkImageData cell data" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
