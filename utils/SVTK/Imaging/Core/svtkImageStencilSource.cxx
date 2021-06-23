/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageStencilSource.h"

#include "svtkGarbageCollector.h"
#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageStencilSource);
svtkCxxSetObjectMacro(svtkImageStencilSource, InformationInput, svtkImageData);

//----------------------------------------------------------------------------
svtkImageStencilSource::svtkImageStencilSource()
{
  this->InformationInput = nullptr;

  this->OutputOrigin[0] = 0;
  this->OutputOrigin[1] = 0;
  this->OutputOrigin[2] = 0;

  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  this->OutputWholeExtent[0] = 0;
  this->OutputWholeExtent[1] = -1;
  this->OutputWholeExtent[2] = 0;
  this->OutputWholeExtent[3] = -1;
  this->OutputWholeExtent[4] = 0;
  this->OutputWholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
svtkImageStencilSource::~svtkImageStencilSource()
{
  this->SetInformationInput(nullptr);
}

//----------------------------------------------------------------------------
void svtkImageStencilSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InformationInput: " << this->InformationInput << "\n";

  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " << this->OutputSpacing[1]
     << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " << this->OutputOrigin[1] << " "
     << this->OutputOrigin[2] << "\n";
  os << indent << "OutputWholeExtent: " << this->OutputWholeExtent[0] << " "
     << this->OutputWholeExtent[1] << " " << this->OutputWholeExtent[2] << " "
     << this->OutputWholeExtent[3] << " " << this->OutputWholeExtent[4] << " "
     << this->OutputWholeExtent[5] << "\n";
}

//----------------------------------------------------------------------------
void svtkImageStencilSource::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->InformationInput, "InformationInput");
}

//----------------------------------------------------------------------------
int svtkImageStencilSource::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  for (int i = 0; i < 3; i++)
  {
    wholeExtent[2 * i] = this->OutputWholeExtent[2 * i];
    wholeExtent[2 * i + 1] = this->OutputWholeExtent[2 * i + 1];
    spacing[i] = this->OutputSpacing[i];
    origin[i] = this->OutputOrigin[i];
  }

  // If InformationInput is set, then get the spacing,
  // origin, and whole extent from it.
  if (this->InformationInput)
  {
    this->InformationInput->GetExtent(wholeExtent);
    this->InformationInput->GetSpacing(spacing);
    this->InformationInput->GetOrigin(origin);
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::UNRESTRICTED_UPDATE_EXTENT(), 1);

  return 1;
}
