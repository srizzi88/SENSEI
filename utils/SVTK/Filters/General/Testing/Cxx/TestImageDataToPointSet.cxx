/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataToPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkImageDataToPointSet.h>

#include <svtkImageData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkStructuredGrid.h>

#include <svtkNew.h>

int TestImageDataToPointSet(int, char*[])
{
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  wavelet->SetCenter(0, 0, 0);
  wavelet->SetMaximum(255);
  wavelet->SetStandardDeviation(.5);
  wavelet->SetXFreq(60);
  wavelet->SetYFreq(30);
  wavelet->SetZFreq(40);
  wavelet->SetXMag(10);
  wavelet->SetYMag(18);
  wavelet->SetZMag(5);
  wavelet->SetSubsampleRate(1);
  wavelet->Update();

  svtkImageData* image = wavelet->GetOutput();
  image->SetDirectionMatrix(1, 0, 0, 0, -1, 0, 0, 0, -1);
  image->SetSpacing(0.5, 1, 1.2);
  image->SetOrigin(100, -3.3, 0);

  svtkNew<svtkImageDataToPointSet> image2points;
  image2points->SetInputData(image);
  image2points->Update();

  svtkDataSet* inData = wavelet->GetOutput();
  svtkDataSet* outData = image2points->GetOutput();

  svtkIdType numPoints = inData->GetNumberOfPoints();
  if (numPoints != outData->GetNumberOfPoints())
  {
    std::cout << "Got wrong number of points: " << numPoints << " vs "
              << outData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  svtkIdType numCells = inData->GetNumberOfCells();
  if (numCells != outData->GetNumberOfCells())
  {
    std::cout << "Got wrong number of cells: " << numCells << " vs " << outData->GetNumberOfCells()
              << std::endl;
    return EXIT_FAILURE;
  }

  for (svtkIdType pointId = 0; pointId < numPoints; pointId++)
  {
    double inPoint[3];
    double outPoint[3];

    inData->GetPoint(pointId, inPoint);
    outData->GetPoint(pointId, outPoint);

    if ((inPoint[0] != outPoint[0]) || (inPoint[1] != outPoint[1]) || (inPoint[2] != outPoint[2]))
    {
      std::cout << "Got mismatched point coordinates." << std::endl;
      std::cout << "Input: " << inPoint[0] << " " << inPoint[1] << " " << inPoint[2] << std::endl;
      std::cout << "Output: " << outPoint[0] << " " << outPoint[1] << " " << outPoint[2]
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
