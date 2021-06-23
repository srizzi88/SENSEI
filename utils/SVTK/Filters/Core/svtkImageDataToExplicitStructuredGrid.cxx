/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageDataToExplicitStructuredGrid.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageDataToExplicitStructuredGrid);

//----------------------------------------------------------------------------
int svtkImageDataToExplicitStructuredGrid::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int extent[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageDataToExplicitStructuredGrid::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve input and output
  svtkImageData* input = svtkImageData::GetData(inputVector[0], 0);
  svtkExplicitStructuredGrid* output = svtkExplicitStructuredGrid::GetData(outputVector, 0);

  if (!input)
  {
    svtkErrorMacro(<< "No input!");
    return 0;
  }
  if (input->GetDataDimension() != 3)
  {
    svtkErrorMacro(<< "Cannot convert non 3D image data!");
    return 0;
  }

  // Copy input point and cell data to output
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->ShallowCopy(input->GetCellData());

  svtkIdType nbCells = input->GetNumberOfCells();
  svtkIdType nbPoints = input->GetNumberOfPoints();

  // Extract points coordinates from the image
  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nbPoints);
  for (svtkIdType i = 0; i < nbPoints; i++)
  {
    double p[3];
    input->GetPoint(i, p);
    points->SetPoint(i, p);
  }

  // Build hexahedrons cells from input voxels
  svtkNew<svtkCellArray> cells;
  cells->AllocateEstimate(nbCells, 8);
  svtkNew<svtkIdList> ptIds;
  for (svtkIdType i = 0; i < nbCells; i++)
  {
    input->GetCellPoints(i, ptIds.Get());
    assert(ptIds->GetNumberOfIds() == 8);
    // Change point order: voxels and hexahedron don't have same connectivity.
    svtkIdType ids[8];
    ids[0] = ptIds->GetId(0);
    ids[1] = ptIds->GetId(1);
    ids[2] = ptIds->GetId(3);
    ids[3] = ptIds->GetId(2);
    ids[4] = ptIds->GetId(4);
    ids[5] = ptIds->GetId(5);
    ids[6] = ptIds->GetId(7);
    ids[7] = ptIds->GetId(6);
    cells->InsertNextCell(8, ids);
  }

  int extents[6];
  input->GetExtent(extents);
  output->SetExtent(extents);
  output->SetPoints(points.Get());
  output->SetCells(cells.Get());
  output->ComputeFacesConnectivityFlagsArray();
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageDataToExplicitStructuredGrid::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}
