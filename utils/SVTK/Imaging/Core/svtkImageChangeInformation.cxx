/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageChangeInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageChangeInformation.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageChangeInformation);

//----------------------------------------------------------------------------
svtkImageChangeInformation::svtkImageChangeInformation()
{
  this->CenterImage = 0;

  for (int i = 0; i < 3; i++)
  {
    this->OutputExtentStart[i] = SVTK_INT_MAX;
    this->ExtentTranslation[i] = 0;
    this->FinalExtentTranslation[i] = SVTK_INT_MAX;

    this->OutputSpacing[i] = SVTK_DOUBLE_MAX;
    this->SpacingScale[i] = 1.0;

    this->OutputOrigin[i] = SVTK_DOUBLE_MAX;
    this->OriginScale[i] = 1.0;
    this->OriginTranslation[i] = 0.0;
  }

  // There is an optional second input.
  this->SetNumberOfInputPorts(2);
}

// Specify a source object at a specified table location.
void svtkImageChangeInformation::SetInformationInputData(svtkImageData* pd)
{
  this->SetInputData(1, pd);
}

// Get a pointer to a source object at a specified table location.
svtkImageData* svtkImageChangeInformation::GetInformationInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
svtkImageChangeInformation::~svtkImageChangeInformation() = default;

//----------------------------------------------------------------------------
void svtkImageChangeInformation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CenterImage : " << (this->CenterImage ? "On" : "Off") << endl;

  os << indent << "OutputExtentStart: (" << this->OutputExtentStart[0] << ","
     << this->OutputExtentStart[1] << "," << this->OutputExtentStart[2] << ")" << endl;

  os << indent << "ExtentTranslation: (" << this->ExtentTranslation[0] << ","
     << this->ExtentTranslation[1] << "," << this->ExtentTranslation[2] << ")" << endl;

  os << indent << "OutputSpacing: (" << this->OutputSpacing[0] << "," << this->OutputSpacing[1]
     << "," << this->OutputSpacing[2] << ")" << endl;

  os << indent << "SpacingScale: (" << this->SpacingScale[0] << "," << this->SpacingScale[1] << ","
     << this->SpacingScale[2] << ")" << endl;

  os << indent << "OutputOrigin: (" << this->OutputOrigin[0] << "," << this->OutputOrigin[1] << ","
     << this->OutputOrigin[2] << ")" << endl;

  os << indent << "OriginScale: (" << this->OriginScale[0] << "," << this->OriginScale[1] << ","
     << this->OriginScale[2] << ")" << endl;

  os << indent << "OriginTranslation: (" << this->OriginTranslation[0] << ","
     << this->OriginTranslation[1] << "," << this->OriginTranslation[2] << ")" << endl;
}

//----------------------------------------------------------------------------
// Change the information
int svtkImageChangeInformation::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int i;
  int extent[6], inExtent[6];
  double spacing[3], origin[3];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExtent);

  svtkImageData* infoInput = this->GetInformationInput();
  if (infoInput)
  {
    // If there is an InformationInput, it is set as a second input
    svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
    infoInput->GetOrigin(origin);
    infoInput->GetSpacing(spacing);
    in2Info->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    for (i = 0; i < 3; i++)
    {
      extent[2 * i + 1] = extent[2 * i] - inExtent[2 * i] + inExtent[2 * i + 1];
    }
  }
  else
  {
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    inInfo->Get(svtkDataObject::ORIGIN(), origin);
    inInfo->Get(svtkDataObject::SPACING(), spacing);
  }

  for (i = 0; i < 3; i++)
  {
    if (this->OutputSpacing[i] != SVTK_DOUBLE_MAX)
    {
      spacing[i] = this->OutputSpacing[i];
    }

    if (this->OutputOrigin[i] != SVTK_DOUBLE_MAX)
    {
      origin[i] = this->OutputOrigin[i];
    }

    if (this->OutputExtentStart[i] != SVTK_INT_MAX)
    {
      extent[2 * i + 1] += this->OutputExtentStart[i] - extent[2 * i];
      extent[2 * i] = this->OutputExtentStart[i];
    }
  }

  if (this->CenterImage)
  {
    for (i = 0; i < 3; i++)
    {
      origin[i] = -(extent[2 * i] + extent[2 * i + 1]) * spacing[i] / 2;
    }
  }

  for (i = 0; i < 3; i++)
  {
    spacing[i] = spacing[i] * this->SpacingScale[i];
    origin[i] = origin[i] * this->OriginScale[i] + this->OriginTranslation[i];
    extent[2 * i] = extent[2 * i] + this->ExtentTranslation[i];
    extent[2 * i + 1] = extent[2 * i + 1] + this->ExtentTranslation[i];
    this->FinalExtentTranslation[i] = extent[2 * i] - inExtent[2 * i];
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int svtkImageChangeInformation::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->FinalExtentTranslation[0] == SVTK_INT_MAX)
  {
    svtkErrorMacro("Bug in code, RequestInformation was not called");
    return 0;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int extent[6];

  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (int i = 0; i < 3; ++i)
  {
    extent[i * 2] += this->FinalExtentTranslation[i];
    extent[i * 2 + 1] += this->FinalExtentTranslation[i];
  }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageChangeInformation::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (this->FinalExtentTranslation[0] == SVTK_INT_MAX)
  {
    svtkErrorMacro("Bug in code.");
    return 0;
  }

  int inExt[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);

  inExt[0] -= this->FinalExtentTranslation[0];
  inExt[1] -= this->FinalExtentTranslation[0];
  inExt[2] -= this->FinalExtentTranslation[1];
  inExt[3] -= this->FinalExtentTranslation[1];
  inExt[4] -= this->FinalExtentTranslation[2];
  inExt[5] -= this->FinalExtentTranslation[2];

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

int svtkImageChangeInformation::FillInputPortInformation(int port, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}
