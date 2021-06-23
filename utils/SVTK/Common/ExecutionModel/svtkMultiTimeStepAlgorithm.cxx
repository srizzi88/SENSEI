/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkMultiTimeStepAlgorithm.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiTimeStepAlgorithm.h"

#include "svtkCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationKey.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMultiTimeStepAlgorithm);

svtkInformationKeyMacro(svtkMultiTimeStepAlgorithm, UPDATE_TIME_STEPS, DoubleVector);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkMultiTimeStepAlgorithm::svtkMultiTimeStepAlgorithm()
{
  this->RequestUpdateIndex = 0;
  this->SetNumberOfInputPorts(1);
  this->CacheData = false;
  this->NumberOfCacheEntries = 1;
}

//----------------------------------------------------------------------------
bool svtkMultiTimeStepAlgorithm::IsInCache(double time, size_t& idx)
{
  std::vector<TimeCache>::iterator it = this->Cache.begin();
  for (idx = 0; it != this->Cache.end(); ++it, ++idx)
  {
    if (time == it->TimeValue)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkMultiTimeStepAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
  {
    int retVal(1);
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (this->RequestUpdateIndex == 0)
    {
      retVal = this->RequestUpdateExtent(request, inputVector, outputVector);

      double* upTimes = inInfo->Get(UPDATE_TIME_STEPS());
      int numUpTimes = inInfo->Length(UPDATE_TIME_STEPS());
      this->UpdateTimeSteps.clear();
      for (int i = 0; i < numUpTimes; i++)
      {
        this->UpdateTimeSteps.push_back(upTimes[i]);
      }
      inInfo->Remove(UPDATE_TIME_STEPS());
    }

    size_t nTimeSteps = this->UpdateTimeSteps.size();
    if (nTimeSteps > 0)
    {
      bool inCache = true;
      for (size_t i = 0; i < nTimeSteps; i++)
      {
        size_t idx;
        if (!this->IsInCache(this->UpdateTimeSteps[i], idx))
        {
          inCache = false;
          break;
        }
      }
      if (!inCache)
      {
        inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
          this->UpdateTimeSteps[this->RequestUpdateIndex]);
      }
      else
      {
        // Ask for any time step. This should not update unless something else changed.
        inInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      }
    }
    return retVal;
  }

  // generate the data
  if (request->Has(svtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = 1;
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    svtkDataObject* inData = inInfo->Get(svtkDataObject::DATA_OBJECT());

    if (this->UpdateTimeSteps.empty())
    {
      svtkErrorMacro("No temporal data has been requested. ");
      return 0;
    }

    if (this->RequestUpdateIndex == 0) // first time step
    {
      this->MDataSet = svtkSmartPointer<svtkMultiBlockDataSet>::New();
      this->MDataSet->SetNumberOfBlocks(static_cast<unsigned int>(this->UpdateTimeSteps.size()));
    }

    svtkSmartPointer<svtkDataObject> inDataCopy;
    inDataCopy.TakeReference(inData->NewInstance());
    inDataCopy->ShallowCopy(inData);

    size_t idx;
    if (!this->IsInCache(this->UpdateTimeSteps[this->RequestUpdateIndex], idx))
    {
      this->Cache.push_back(TimeCache(this->UpdateTimeSteps[this->RequestUpdateIndex], inDataCopy));
    }

    this->RequestUpdateIndex++;

    size_t nTimeSteps = this->UpdateTimeSteps.size();
    if (this->RequestUpdateIndex == static_cast<int>(nTimeSteps)) // all the time steps are here
    {
      for (size_t i = 0; i < nTimeSteps; i++)
      {
        if (this->IsInCache(this->UpdateTimeSteps[i], idx))
        {
          this->MDataSet->SetBlock(static_cast<unsigned int>(i), this->Cache[idx].Data);
        }
        else
        {
          // This should never happen
          abort();
        }
      }

      // change the input to the multiblock data and let child class to do the work
      // make sure to set the input back to what it was to not break anything upstream
      inData->Register(this);
      inInfo->Set(svtkDataObject::DATA_OBJECT(), this->MDataSet);
      retVal = this->RequestData(request, inputVector, outputVector);
      inInfo->Set(svtkDataObject::DATA_OBJECT(), inData);
      inData->Delete();

      this->UpdateTimeSteps.clear();
      this->RequestUpdateIndex = 0;
      this->MDataSet = nullptr;
      if (!this->CacheData)
      {
        // No caching, remove all
        this->Cache.clear();
      }
      else
      {
        // Caching, erase ones outside the cache
        // Note that this is a first in first out implementation
        size_t cacheSize = this->Cache.size();
        if (cacheSize > this->NumberOfCacheEntries)
        {
          size_t nToErase = cacheSize - this->NumberOfCacheEntries;
          this->Cache.erase(this->Cache.begin(), this->Cache.begin() + nToErase);
        }
      }
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }
    else
    {
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

    return retVal;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    // Upstream changed, clear the cache.
    this->Cache.clear();
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkMultiTimeStepAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
