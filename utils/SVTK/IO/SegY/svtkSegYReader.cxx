/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSegYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSegYReader.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkSegYReaderInternal.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"

#include <algorithm>
#include <iostream>
#include <iterator>

svtkStandardNewMacro(svtkSegYReader);

//-----------------------------------------------------------------------------
svtkSegYReader::svtkSegYReader()
{
  this->SetNumberOfInputPorts(0);
  this->Reader = new svtkSegYReaderInternal();
  this->FileName = nullptr;
  this->Is3D = false;
  this->Force2D = false;
  std::fill(this->DataOrigin, this->DataOrigin + 3, 0.0);
  std::fill(this->DataSpacing[0], this->DataSpacing[0] + 3, 1.0);
  std::fill(this->DataSpacing[1], this->DataSpacing[1] + 3, 1.0);
  std::fill(this->DataSpacing[2], this->DataSpacing[2] + 3, 1.0);
  std::fill(this->DataSpacingSign, this->DataSpacingSign + 3, 1);
  std::fill(this->DataExtent, this->DataExtent + 6, 0);

  this->XYCoordMode = SVTK_SEGY_SOURCE;
  this->StructuredGrid = 1;
  this->XCoordByte = 73;
  this->YCoordByte = 77;

  this->VerticalCRS = SVTK_SEGY_VERTICAL_HEIGHTS;
}

//-----------------------------------------------------------------------------
svtkSegYReader::~svtkSegYReader()
{
  delete this->Reader;
  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
void svtkSegYReader::SetXYCoordModeToSource()
{
  this->SetXYCoordMode(SVTK_SEGY_SOURCE);
}

//-----------------------------------------------------------------------------
void svtkSegYReader::SetXYCoordModeToCDP()
{
  this->SetXYCoordMode(SVTK_SEGY_CDP);
}

//-----------------------------------------------------------------------------
void svtkSegYReader::SetXYCoordModeToCustom()
{
  this->SetXYCoordMode(SVTK_SEGY_CUSTOM);
}

//-----------------------------------------------------------------------------
void svtkSegYReader::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int svtkSegYReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    return 0;
  }

  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!output)
  {
    return 0;
  }

  this->Reader->SetVerticalCRS(this->VerticalCRS);
  switch (this->XYCoordMode)
  {
    case SVTK_SEGY_SOURCE:
    {
      this->Reader->SetXYCoordBytePositions(72, 76);
      break;
    }
    case SVTK_SEGY_CDP:
    {
      this->Reader->SetXYCoordBytePositions(180, 184);
      break;
    }
    case SVTK_SEGY_CUSTOM:
    {
      this->Reader->SetXYCoordBytePositions(this->XCoordByte - 1, this->YCoordByte - 1);
      break;
    }
    default:
    {
      svtkErrorMacro(<< "Unknown value for XYCoordMode " << this->XYCoordMode);
      return 1;
    }
  }
  this->Reader->LoadTraces(this->DataExtent);
  this->UpdateProgress(0.5);
  if (this->Is3D && !this->StructuredGrid)
  {
    svtkImageData* imageData = svtkImageData::SafeDownCast(output);
    this->Reader->ExportData(
      imageData, this->DataExtent, this->DataOrigin, this->DataSpacing, this->DataSpacingSign);
  }
  else
  {
    svtkStructuredGrid* grid = svtkStructuredGrid::SafeDownCast(output);
    this->Reader->ExportData(grid, this->DataExtent, this->DataOrigin, this->DataSpacing);
    grid->Squeeze();
  }
  this->Reader->In.close();
  return 1;
}

//-----------------------------------------------------------------------------
int svtkSegYReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    svtkErrorMacro("Invalid output information object");
    return 0;
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);
  if (this->Is3D && !this->StructuredGrid)
  {
    double spacing[3] = { svtkMath::Norm(this->DataSpacing[0]), svtkMath::Norm(this->DataSpacing[1]),
      svtkMath::Norm(this->DataSpacing[2]) };
    outInfo->Set(svtkDataObject::ORIGIN(), this->DataOrigin, 3);
    outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkSegYReader::RequestDataObject(svtkInformation*,
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkDataSet* output = svtkDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    svtkErrorMacro("Requires valid input file name");
    return 0;
  }

  if (this->Reader->In.is_open())
  {
    this->Reader->In.seekg(0, this->Reader->In.beg);
  }
  else
  {
    this->Reader->In.open(this->FileName, std::ios::binary);
  }
  if (!this->Reader->In)
  {
    svtkErrorMacro("File not found:" << this->FileName);
    return 0;
  }
  this->Is3D = this->Reader->Is3DComputeParameters(
    this->DataExtent, this->DataOrigin, this->DataSpacing, this->DataSpacingSign, this->Force2D);
  const char* outputTypeName =
    (this->Is3D && !this->StructuredGrid) ? "svtkImageData" : "svtkStructuredGrid";

  if (!output || !output->IsA(outputTypeName))
  {
    svtkDataSet* newOutput = nullptr;
    if (this->Is3D && !this->StructuredGrid)
    {
      newOutput = svtkImageData::New();
    }
    else
    {
      newOutput = svtkStructuredGrid::New();
    }
    info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
  }
  return 1;
}
