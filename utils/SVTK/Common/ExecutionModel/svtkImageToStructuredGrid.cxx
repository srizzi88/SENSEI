/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkImageToStructuredGrid.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkImageToStructuredGrid.h"
#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStructuredGrid.h"

#include <cassert>

//
// Standard methods
//
svtkStandardNewMacro(svtkImageToStructuredGrid);

svtkImageToStructuredGrid::svtkImageToStructuredGrid() = default;

//------------------------------------------------------------------------------
svtkImageToStructuredGrid::~svtkImageToStructuredGrid() = default;

//------------------------------------------------------------------------------
void svtkImageToStructuredGrid::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
}

//------------------------------------------------------------------------------
int svtkImageToStructuredGrid::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int svtkImageToStructuredGrid::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkImageToStructuredGrid::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  assert(inInfo != nullptr);
  assert(outInfo != nullptr);

  svtkImageData* img = svtkImageData::SafeDownCast(inInfo->Get(svtkImageData::DATA_OBJECT()));
  assert(img != nullptr);

  svtkStructuredGrid* grid =
    svtkStructuredGrid::SafeDownCast(outInfo->Get(svtkStructuredGrid::DATA_OBJECT()));
  assert(grid != nullptr);

  int dims[3];
  img->GetDimensions(dims);

  svtkPoints* gridPoints = svtkPoints::New();
  assert(gridPoints != nullptr);
  gridPoints->SetDataTypeToDouble();
  gridPoints->SetNumberOfPoints(img->GetNumberOfPoints());

  double pnt[3];
  for (int i = 0; i < img->GetNumberOfPoints(); ++i)
  {
    img->GetPoint(i, pnt);
    gridPoints->SetPoint(i, pnt);
  }
  grid->SetDimensions(dims);
  grid->SetPoints(gridPoints);
  gridPoints->Delete();

  this->CopyPointData(img, grid);
  this->CopyCellData(img, grid);

  return 1;
}

//------------------------------------------------------------------------------
void svtkImageToStructuredGrid::CopyPointData(svtkImageData* img, svtkStructuredGrid* sgrid)
{
  assert(img != nullptr);
  assert(sgrid != nullptr);

  if (img->GetPointData()->GetNumberOfArrays() == 0)
    return;

  for (int i = 0; i < img->GetPointData()->GetNumberOfArrays(); ++i)
  {
    svtkDataArray* myArray = img->GetPointData()->GetArray(i);
    sgrid->GetPointData()->AddArray(myArray);
  } // END for all node arrays
}

//------------------------------------------------------------------------------
void svtkImageToStructuredGrid::CopyCellData(svtkImageData* img, svtkStructuredGrid* sgrid)
{
  assert(img != nullptr);
  assert(sgrid != nullptr);

  if (img->GetCellData()->GetNumberOfArrays() == 0)
    return;

  for (int i = 0; i < img->GetCellData()->GetNumberOfArrays(); ++i)
  {
    svtkDataArray* myArray = img->GetCellData()->GetArray(i);
    sgrid->GetCellData()->AddArray(myArray);
  } // END for all cell arrays
}
