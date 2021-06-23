/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDAtaToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestImageDAtaToUniformGrid.cxx --Test svtkImageDataToUniformGrid
//
// .SECTION Description
// Serial tests for converting an image data to a uniform grid with blanking.

// SVTK includes
#include <svtkDataSet.h>
#include <svtkElevationFilter.h>
#include <svtkImageDataToUniformGrid.h>
#include <svtkMultiBlockDataGroupFilter.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkPointDataToCellData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkSphereSource.h>
#include <svtkThreshold.h>
#include <svtkUnstructuredGrid.h>

// C++ includes
#include <iostream>

namespace
{

// returns 0 for success.
int TestSingleGridBlanking(bool pointBlanking, bool reverse, int expectedNumberOfCells)
{
  svtkNew<svtkRTAnalyticSource> source;
  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(source->GetOutputPort());
  elevation->SetLowPoint(-10, 0, 0);
  elevation->SetHighPoint(10, 0, 0);
  elevation->SetScalarRange(0, 3);
  svtkNew<svtkPointDataToCellData> pointDataToCellData;
  pointDataToCellData->SetInputConnection(elevation->GetOutputPort());
  pointDataToCellData->PassPointDataOn();

  pointDataToCellData->Update();

  svtkNew<svtkImageDataToUniformGrid> imageDataToUniformGrid;
  imageDataToUniformGrid->SetInputConnection(pointDataToCellData->GetOutputPort());
  imageDataToUniformGrid->SetReverse(reverse);
  if (pointBlanking)
  {
    imageDataToUniformGrid->SetInputArrayToProcess(
      0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");
  }
  else
  {
    imageDataToUniformGrid->SetInputArrayToProcess(
      0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "Elevation");
  }
  imageDataToUniformGrid->Update();

  // the threshold filter is really meant to create an unstructured
  // grid. The threshold is set to include the range of RTData so that
  // the only cells that aren't outputted from the threshold filter
  // are the blanked cells.
  svtkNew<svtkThreshold> threshold;
  threshold->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  threshold->ThresholdBetween(-1000, 1000);
  threshold->SetInputConnection(imageDataToUniformGrid->GetOutputPort());
  threshold->Update();
  svtkUnstructuredGrid* outputGrid = threshold->GetOutput();

  if (outputGrid->GetNumberOfCells() == expectedNumberOfCells)
  {
    return 0;
  }
  svtkGenericWarningMacro(
    "Expecting " << expectedNumberOfCells << " but getting " << outputGrid->GetNumberOfCells());
  return 1;
}

// returns 0 for success. only tests point blanking
int TestMultiBlockBlanking(int expectedNumberOfCells)
{
  svtkNew<svtkRTAnalyticSource> source;
  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(source->GetOutputPort());
  elevation->SetLowPoint(-10, 0, 0);
  elevation->SetHighPoint(10, 0, 0);
  elevation->SetScalarRange(0, 3);
  svtkNew<svtkPointDataToCellData> pointDataToCellData;
  pointDataToCellData->SetInputConnection(elevation->GetOutputPort());
  pointDataToCellData->PassPointDataOn();

  svtkNew<svtkSphereSource> sphereSource;
  svtkNew<svtkMultiBlockDataGroupFilter> groupFilter;
  groupFilter->SetInputConnection(pointDataToCellData->GetOutputPort());
  groupFilter->SetInputConnection(pointDataToCellData->GetOutputPort());
  groupFilter->AddInputConnection(sphereSource->GetOutputPort());

  svtkNew<svtkImageDataToUniformGrid> imageDataToUniformGrid;
  imageDataToUniformGrid->SetInputConnection(groupFilter->GetOutputPort());
  imageDataToUniformGrid->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");
  imageDataToUniformGrid->Update();
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(imageDataToUniformGrid->GetOutput());

  svtkNew<svtkThreshold> threshold;
  threshold->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  threshold->ThresholdBetween(50, 150);
  threshold->SetInputData(output->GetBlock(0));
  threshold->Update();
  svtkUnstructuredGrid* outputGrid = threshold->GetOutput();

  if (outputGrid->GetNumberOfCells() == expectedNumberOfCells)
  {
    return 0;
  }
  svtkGenericWarningMacro(
    "Expecting " << expectedNumberOfCells << " but getting " << outputGrid->GetNumberOfCells());
  return 1;
}

} // end anonymous namespace

//------------------------------------------------------------------------------
// Program main
int TestImageDataToUniformGrid(int, char*[])
{
  int rc = TestSingleGridBlanking(true, false, 5200);
  rc += TestSingleGridBlanking(false, false, 5200);

  rc += TestSingleGridBlanking(true, true, 2400);
  // note that this and the second call to TestSingleGridBlanking
  // are opposites so they should add up to 8000 cells.
  rc += TestSingleGridBlanking(false, true, 2800);

  rc += TestMultiBlockBlanking(1102);

  return rc;
}
