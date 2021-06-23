/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalDataSetCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTemporalDataSetCache.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <vector>

//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkTemporalDataSetCache);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
svtkTemporalDataSetCache::svtkTemporalDataSetCache()
{
  this->CacheSize = 10;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkTemporalDataSetCache::~svtkTemporalDataSetCache()
{
  CacheType::iterator pos = this->Cache.begin();
  for (; pos != this->Cache.end();)
  {
    pos->second.second->UnRegister(this);
    this->Cache.erase(pos++);
  }
}

svtkTypeBool svtkTemporalDataSetCache::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(svtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
  }

  // set update extent
  if (request->Has(svtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkTemporalDataSetCache::FillInputPortInformation(int port, svtkInformation* info)
{
  // port 0 must be temporal data, but port 1 can be any dataset
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  return 1;
}

int svtkTemporalDataSetCache::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}
//----------------------------------------------------------------------------
void svtkTemporalDataSetCache::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CacheSize: " << this->CacheSize << endl;
}
//----------------------------------------------------------------------------
void svtkTemporalDataSetCache::SetCacheSize(int size)
{
  if (size < 1)
  {
    svtkErrorMacro("Attempt to set cache size to less than 1");
    return;
  }

  // if growing the cache, there is no need to do anything
  this->CacheSize = size;
  if (this->Cache.size() <= static_cast<unsigned long>(size))
  {
    return;
  }

  // skrinking, have to get rid of some old data, to be easy just chuck the
  // first entries
  int i = static_cast<int>(this->Cache.size()) - size;
  CacheType::iterator pos = this->Cache.begin();
  for (; i > 0; --i)
  {
    pos->second.second->UnRegister(this);
    this->Cache.erase(pos++);
  }
}

int svtkTemporalDataSetCache::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataObject* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkTemporalDataSetCache ::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // First look through the cached data to see if it is still valid.
  CacheType::iterator pos;
  svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (!ddp)
  {
    return 1;
  }

  svtkMTimeType pmt = ddp->GetPipelineMTime();
  for (pos = this->Cache.begin(); pos != this->Cache.end();)
  {
    if (pos->second.first < pmt)
    {
      pos->second.second->Delete();
      this->Cache.erase(pos++);
    }
    else
    {
      ++pos;
    }
  }

  // are there any times that we are missing from the request? e.g. times
  // that are not cached?
  std::vector<double> reqTimeSteps;
  if (!outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // no time steps were passed in the update request, so just request
    // something to keep the pipeline happy.
    if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      int NumberOfInputTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
      //
      // Get list of input time step values
      std::vector<double> InputTimeValues;
      InputTimeValues.resize(NumberOfInputTimeSteps);
      inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &InputTimeValues[0]);

      // this should be the same, just checking for debug purposes
      reqTimeSteps.push_back(InputTimeValues[0]);
    }
    else
      return 0;
  }
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // do we have this time step?
    pos = this->Cache.find(upTime);
    if (pos == this->Cache.end())
    {
      reqTimeSteps.push_back(upTime);
    }

    // if we need any data
    if (!reqTimeSteps.empty())
    {
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), reqTimeSteps[0]);
    }
    // otherwise leave the input with what it already has
    else
    {
      svtkDataObject* dobj = inInfo->Get(svtkDataObject::DATA_OBJECT());
      if (dobj)
      {
        double it = dobj->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
        if (dobj->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
        {
          inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), it);
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int svtkTemporalDataSetCache::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkMTimeType outputUpdateTime = outInfo->Get(svtkDataObject::DATA_OBJECT())->GetUpdateTime();

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  // get some time informationX
  double upTime = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double inTime = input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());

  svtkSmartPointer<svtkDataObject> output;

  // a time should either be in the Cache or in the input
  CacheType::iterator pos = this->Cache.find(upTime);
  if (pos != this->Cache.end())
  {
    svtkDataObject* cachedData = pos->second.second;
    output.TakeReference(cachedData->NewInstance());
    output->ShallowCopy(cachedData);

    // update the m time in the cache
    pos->second.first = outputUpdateTime;
  }
  // otherwise it better be in the input
  else
  {
    if (input->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()) && (inTime == upTime))
    {
      output.TakeReference(input->NewInstance());
      output->ShallowCopy(input);
    }
    else
    {
      // just shallow copy input to output
      output.TakeReference(input->NewInstance());
      output->ShallowCopy(input);
    }
  }
  // set the data times
  outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
  output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), upTime);

  // now we need to update the cache, based on the new data and the cache
  // size add the requested data to the cache first
  if (input->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
  {

    // is the input time not already in the cache?
    CacheType::iterator pos1 = this->Cache.find(inTime);
    if (pos1 == this->Cache.end())
    {
      // if we have room in the Cache then just add the new data
      if (this->Cache.size() < static_cast<unsigned long>(this->CacheSize))
      {
        svtkDataObject* cachedData = input->NewInstance();
        cachedData->ShallowCopy(input);

        this->Cache[inTime] =
          std::pair<unsigned long, svtkDataObject*>(outputUpdateTime, cachedData);
      }
      // no room in the cache, we need to get rid of something
      else
      {
        // get rid of the oldest data in the cache
        CacheType::iterator pos2 = this->Cache.begin();
        CacheType::iterator oldestpos = this->Cache.begin();
        for (; pos2 != this->Cache.end(); ++pos2)
        {
          if (pos2->second.first < oldestpos->second.first)
          {
            oldestpos = pos2;
          }
        }
        // was there old data?
        if (oldestpos->second.first < outputUpdateTime)
        {
          oldestpos->second.second->UnRegister(this);
          this->Cache.erase(oldestpos);
        }
        // if no old data and no room then we are done
      }
    }
  }
  return 1;
}
