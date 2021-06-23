/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridClip.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkRectilinearGridClip);

//----------------------------------------------------------------------------
svtkRectilinearGridClip::svtkRectilinearGridClip()
{
  this->ClipData = 0;
  this->Initialized = 0;

  this->OutputWholeExtent[0] = this->OutputWholeExtent[2] = this->OutputWholeExtent[4] =
    -SVTK_INT_MAX;

  this->OutputWholeExtent[1] = this->OutputWholeExtent[3] = this->OutputWholeExtent[5] =
    SVTK_INT_MAX;
}

//----------------------------------------------------------------------------
void svtkRectilinearGridClip::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int idx;

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0] << ","
     << this->OutputWholeExtent[1];
  for (idx = 1; idx < 3; ++idx)
  {
    os << indent << ", " << this->OutputWholeExtent[idx * 2] << ","
       << this->OutputWholeExtent[idx * 2 + 1];
  }
  os << ")\n";
  if (this->ClipData)
  {
    os << indent << "ClipDataOn\n";
  }
  else
  {
    os << indent << "ClipDataOff\n";
  }
}

//----------------------------------------------------------------------------
void svtkRectilinearGridClip::SetOutputWholeExtent(int extent[6], svtkInformation* outInfo)
{
  int idx;
  int modified = 0;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->OutputWholeExtent[idx] != extent[idx])
    {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
    }
  }
  this->Initialized = 1;
  if (modified)
  {
    this->Modified();
    if (!outInfo)
    {
      outInfo = this->GetExecutive()->GetOutputInformation(0);
    }
    outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  }
}

//----------------------------------------------------------------------------
void svtkRectilinearGridClip::SetOutputWholeExtent(
  int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;
  extent[1] = maxX;
  extent[2] = minY;
  extent[3] = maxY;
  extent[4] = minZ;
  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}

//----------------------------------------------------------------------------
void svtkRectilinearGridClip::GetOutputWholeExtent(int extent[6])
{
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->OutputWholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int svtkRectilinearGridClip::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int idx, extent[6];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  if (!this->Initialized)
  {
    this->SetOutputWholeExtent(extent, outInfo);
  }

  // Clip the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
  {
    if (this->OutputWholeExtent[idx * 2] >= extent[idx * 2] &&
      this->OutputWholeExtent[idx * 2] <= extent[idx * 2 + 1])
    {
      extent[idx * 2] = this->OutputWholeExtent[idx * 2];
    }
    if (this->OutputWholeExtent[idx * 2 + 1] >= extent[idx * 2] &&
      this->OutputWholeExtent[idx * 2 + 1] <= extent[idx * 2 + 1])
    {
      extent[idx * 2 + 1] = this->OutputWholeExtent[idx * 2 + 1];
    }
    // make usre the order is correct
    if (extent[idx * 2] > extent[idx * 2 + 1])
    {
      extent[idx * 2] = extent[idx * 2 + 1];
    }
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void svtkRectilinearGridClip::ResetOutputWholeExtent()
{
  if (!this->GetInputConnection(0, 0))
  {
    svtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
  }

  this->GetInputConnection(0, 0)->GetProducer()->UpdateInformation();
  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  this->SetOutputWholeExtent(inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int svtkRectilinearGridClip::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int* inExt;
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkRectilinearGrid* outData =
    svtkRectilinearGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkRectilinearGrid* inData =
    svtkRectilinearGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  inExt = inData->GetExtent();

  outData->SetExtent(inExt);
  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());
  outData->SetXCoordinates(inData->GetXCoordinates());
  outData->SetYCoordinates(inData->GetYCoordinates());
  outData->SetZCoordinates(inData->GetZCoordinates());

  if (this->ClipData)
  {
    outData->Crop(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  }

  return 1;
}
