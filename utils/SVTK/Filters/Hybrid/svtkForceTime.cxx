/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkForceTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkForceTime.h"

#include "svtkDataObject.h"
#include "svtkDataObjectTypes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkForceTime);

//----------------------------------------------------------------------------
svtkForceTime::svtkForceTime()
{
  this->ForcedTime = 0.0;
  this->IgnorePipelineTime = true;
  this->Cache = nullptr;
  this->PipelineTime = -1;
  this->PipelineTimeFlag = false;
}

//----------------------------------------------------------------------------
svtkForceTime::~svtkForceTime()
{
  if (this->Cache != nullptr)
  {
    this->Cache->Delete();
    this->Cache = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkForceTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ForcedTime: " << this->ForcedTime << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
}

//----------------------------------------------------------------------------
int svtkForceTime::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double range[2];
    inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range);
    if (this->IgnorePipelineTime)
    {
      range[0] = this->ForcedTime;
      range[1] = this->ForcedTime;
    }
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
  }

  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* outTimes;
    if (this->IgnorePipelineTime)
    {
      outTimes = new double[numTimes];
      for (int i = 0; i < numTimes; i++)
      {
        outTimes[i] = this->ForcedTime;
      }
    }
    else
    {
      outTimes = inTimes;
    }
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), outTimes, numTimes);

    if (this->IgnorePipelineTime)
    {
      delete[] outTimes;
    }
  }

  // Upstream filters changed, invalidate cache
  if (this->IgnorePipelineTime && this->Cache)
  {
    this->Cache->Delete();
    this->Cache = nullptr;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkForceTime::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* inData = svtkDataObject::GetData(inputVector[0], 0);
  svtkDataObject* outData = svtkDataObject::GetData(outputVector, 0);

  if (!inData)
  {
    return 1;
  }

  // Filter is "disabled", just pass input data
  if (!this->IgnorePipelineTime)
  {
    outData->ShallowCopy(inData);
    return 1;
  }

  // Create Cache
  if (!this->Cache)
  {
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->Cache = svtkDataObjectTypes::NewDataObject(inData->GetClassName());
    this->Cache->DeepCopy(inData);
    this->PipelineTimeFlag = true;
  }
  else if (this->PipelineTimeFlag)
  {
    // Stop the pipeline loop
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->PipelineTimeFlag = false;
  }
  outData->ShallowCopy(this->Cache);
  return 1;
}

//----------------------------------------------------------------------------
int svtkForceTime::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->IgnorePipelineTime && !this->Cache)
  {
    double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      // Save current pipeline time step
      this->PipelineTime = inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->ForcedTime);
    }
  }
  else if (this->PipelineTimeFlag)
  {
    // Restore pipeline time
    double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->PipelineTime);
    }
  }

  return 1;
}
