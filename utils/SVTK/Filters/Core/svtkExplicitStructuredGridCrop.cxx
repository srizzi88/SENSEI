/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridCrop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExplicitStructuredGridCrop.h"

#include "svtkAlgorithmOutput.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkExplicitStructuredGridCrop);

//-----------------------------------------------------------------------------
svtkExplicitStructuredGridCrop::svtkExplicitStructuredGridCrop()
{
  this->Initialized = 0;
  this->OutputWholeExtent[0] = this->OutputWholeExtent[2] = this->OutputWholeExtent[4] =
    SVTK_INT_MIN;

  this->OutputWholeExtent[1] = this->OutputWholeExtent[3] = this->OutputWholeExtent[5] =
    SVTK_INT_MAX;
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridCrop::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0] << ","
     << this->OutputWholeExtent[1];
  os << indent << ", " << this->OutputWholeExtent[2] << "," << this->OutputWholeExtent[3];
  os << indent << ", " << this->OutputWholeExtent[4] << "," << this->OutputWholeExtent[5];
  os << ")\n";
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridCrop::SetOutputWholeExtent(int extent[6], svtkInformation* outInfo)
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
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  }
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridCrop::SetOutputWholeExtent(
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
void svtkExplicitStructuredGridCrop::GetOutputWholeExtent(int extent[6])
{
  for (int idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->OutputWholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void svtkExplicitStructuredGridCrop::ResetOutputWholeExtent()
{
  if (!this->GetInput())
  {
    svtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
  }

  this->GetInputConnection(0, 0)->GetProducer()->UpdateInformation();
  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  this->SetOutputWholeExtent(inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int svtkExplicitStructuredGridCrop::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int extent[6];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  if (!this->Initialized)
  {
    this->SetOutputWholeExtent(extent, outInfo);
  }

  // Clip the OutputWholeExtent with the input WholeExtent
  for (int idx = 0; idx < 3; ++idx)
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
  }
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridCrop::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // We can handle anything.
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridCrop::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve input and output
  svtkExplicitStructuredGrid* input = svtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  svtkExplicitStructuredGrid* output = svtkExplicitStructuredGrid::GetData(outputVector, 0);

  output->Crop(input, this->OutputWholeExtent, true);

  this->UpdateProgress(1.);
  return 1;
}
