/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageToImageStencil.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageToImageStencil);

//----------------------------------------------------------------------------
svtkImageToImageStencil::svtkImageToImageStencil()
{
  this->UpperThreshold = SVTK_FLOAT_MAX;
  this->LowerThreshold = -SVTK_FLOAT_MAX;
}

//----------------------------------------------------------------------------
svtkImageToImageStencil::~svtkImageToImageStencil() = default;

//----------------------------------------------------------------------------
void svtkImageToImageStencil::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
}

//----------------------------------------------------------------------------
void svtkImageToImageStencil::SetInputData(svtkImageData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageToImageStencil::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void svtkImageToImageStencil::ThresholdByUpper(double thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < SVTK_FLOAT_MAX)
  {
    this->LowerThreshold = thresh;
    this->UpperThreshold = SVTK_FLOAT_MAX;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void svtkImageToImageStencil::ThresholdByLower(double thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -SVTK_FLOAT_MAX)
  {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -SVTK_FLOAT_MAX;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void svtkImageToImageStencil::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
  {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkImageToImageStencil::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageStencilData* data =
    svtkImageStencilData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int extent[6];
  inData->GetExtent(extent);
  // output extent is always the input extent
  this->AllocateOutputData(data, extent);

  svtkDataArray* inScalars = inData->GetPointData()->GetScalars();
  double upperThreshold = this->UpperThreshold;
  double lowerThreshold = this->LowerThreshold;

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target =
    static_cast<unsigned long>((extent[5] - extent[4] + 1) * (extent[3] - extent[2] + 1) / 50.0);
  target++;

  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
  {
    for (int idY = extent[2]; idY <= extent[3]; idY++)
    {
      if (count % target == 0)
      {
        this->UpdateProgress(count / (50.0 * target));
      }
      count++;

      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      // index into scalar array
      svtkIdType idS = (static_cast<svtkIdType>(extent[1] - extent[0] + 1) *
        (static_cast<svtkIdType>(extent[3] - extent[2] + 1) *
            static_cast<svtkIdType>(idZ - extent[4]) +
          static_cast<svtkIdType>(idY - extent[2])));

      for (int idX = extent[0]; idX <= extent[1]; idX++)
      {
        int newstate = 1;
        double value = inScalars->GetComponent(idS++, 0);
        if (value >= lowerThreshold && value <= upperThreshold)
        {
          newstate = -1;
          if (newstate != state)
          { // sub extent starts
            r1 = idX;
          }
        }
        else if (newstate != state)
        { // sub extent ends
          r2 = idX - 1;
          data->InsertNextExtent(r1, r2, idY, idZ);
        }
        state = newstate;
      } // for idX
      if (state < 0)
      { // if inside at end, cap off the sub extent
        data->InsertNextExtent(r1, extent[1], idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToImageStencil::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inInfo->Get(svtkDataObject::SPACING(), spacing);
  inInfo->Get(svtkDataObject::ORIGIN(), origin);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::UNRESTRICTED_UPDATE_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToImageStencil::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToImageStencil::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int extent[6], wholeExtent[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  // clip UpdateExtent with WholeExtent
  extent[0] = (extent[0] > wholeExtent[0] ? extent[0] : wholeExtent[0]);
  extent[1] = (extent[1] < wholeExtent[1] ? extent[1] : wholeExtent[1]);
  extent[2] = (extent[2] > wholeExtent[2] ? extent[2] : wholeExtent[2]);
  extent[3] = (extent[3] < wholeExtent[3] ? extent[3] : wholeExtent[3]);
  extent[4] = (extent[4] > wholeExtent[4] ? extent[4] : wholeExtent[4]);
  extent[5] = (extent[5] < wholeExtent[5] ? extent[5] : wholeExtent[5]);

  // if invalid, use the current data extent if allocated
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    for (int j = 0; j < 6; j += 2)
    {
      extent[j] = extent[j + 1] = wholeExtent[j];
    }
    svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (inData)
    {
      inData->GetExtent(extent);
    }
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  return 1;
}
