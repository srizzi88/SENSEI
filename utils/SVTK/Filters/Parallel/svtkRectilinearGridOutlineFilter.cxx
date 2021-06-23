/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridOutlineFilter.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkRectilinearGridOutlineFilter);

int svtkRectilinearGridOutlineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkRectilinearGrid* input =
    svtkRectilinearGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  float bounds[6];
  double* range;
  float x[3];
  svtkIdType pts[2];
  svtkPoints* newPts;
  svtkCellArray* newLines;

  svtkDataArray* xCoords = input->GetXCoordinates();
  svtkDataArray* yCoords = input->GetYCoordinates();
  svtkDataArray* zCoords = input->GetZCoordinates();
  int* ext = input->GetExtent();
  int* wholeExt = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  if (xCoords == nullptr || yCoords == nullptr || zCoords == nullptr ||
    input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // We could probably use just the input bounds ...
  range = xCoords->GetRange();
  bounds[0] = range[0];
  bounds[1] = range[1];
  range = yCoords->GetRange();
  bounds[2] = range[0];
  bounds[3] = range[1];
  range = zCoords->GetRange();
  bounds[4] = range[0];
  bounds[5] = range[1];

  //
  // Allocate storage and create outline
  //
  newPts = svtkPoints::New();
  newPts->Allocate(24);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(12, 2);

  // xMin yMin
  if (ext[0] == wholeExt[0] && ext[2] == wholeExt[2])
  {
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMin yMax
  if (ext[0] == wholeExt[0] && ext[3] == wholeExt[3])
  {
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMin zMin
  if (ext[0] == wholeExt[0] && ext[4] == wholeExt[4])
  {
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMin zMax
  if (ext[0] == wholeExt[0] && ext[5] == wholeExt[5])
  {
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMax yMin
  if (ext[1] == wholeExt[1] && ext[2] == wholeExt[2])
  {
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMax yMax
  if (ext[1] == wholeExt[1] && ext[3] == wholeExt[3])
  {
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMax zMin
  if (ext[1] == wholeExt[1] && ext[4] == wholeExt[4])
  {
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // xMax zMax
  if (ext[1] == wholeExt[1] && ext[5] == wholeExt[5])
  {
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // yMin zMin
  if (ext[2] == wholeExt[2] && ext[4] == wholeExt[4])
  {
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // yMin zMax
  if (ext[2] == wholeExt[2] && ext[5] == wholeExt[5])
  {
    x[0] = bounds[0];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[2];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // yMax zMin
  if (ext[3] == wholeExt[3] && ext[4] == wholeExt[4])
  {
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[4];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }
  // yMax zMax
  if (ext[3] == wholeExt[3] && ext[5] == wholeExt[5])
  {
    x[0] = bounds[0];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[0] = newPts->InsertNextPoint(x);
    x[0] = bounds[1];
    x[1] = bounds[3];
    x[2] = bounds[5];
    pts[1] = newPts->InsertNextPoint(x);
    newLines->InsertNextCell(2, pts);
  }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

int svtkRectilinearGridOutlineFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

void svtkRectilinearGridOutlineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
