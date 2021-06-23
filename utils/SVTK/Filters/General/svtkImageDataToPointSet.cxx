/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkImageDataToPointSet.h"

#include "svtkCellData.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStructuredGrid.h"

#include "svtkNew.h"

svtkStandardNewMacro(svtkImageDataToPointSet);

//-------------------------------------------------------------------------
svtkImageDataToPointSet::svtkImageDataToPointSet() = default;

svtkImageDataToPointSet::~svtkImageDataToPointSet() = default;

void svtkImageDataToPointSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
int svtkImageDataToPointSet::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//-------------------------------------------------------------------------
int svtkImageDataToPointSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve input and output
  svtkImageData* inData = svtkImageData::GetData(inputVector[0]);
  svtkStructuredGrid* outData = svtkStructuredGrid::GetData(outputVector);

  if (inData == nullptr)
  {
    svtkErrorMacro(<< "Input data is nullptr.");
    return 0;
  }
  if (outData == nullptr)
  {
    svtkErrorMacro(<< "Output data is nullptr.");
    return 0;
  }

  // Copy input point and cell data to output
  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());

  // Extract points coordinates from the image
  svtkIdType nbPoints = inData->GetNumberOfPoints();
  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nbPoints);
  for (svtkIdType i = 0; i < nbPoints; i++)
  {
    double p[3];
    inData->GetPoint(i, p);
    points->SetPoint(i, p);
  }
  outData->SetPoints(points);

  // Copy Extent
  int extent[6];
  inData->GetExtent(extent);
  outData->SetExtent(extent);

  return 1;
}
