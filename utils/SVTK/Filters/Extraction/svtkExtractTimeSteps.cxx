/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTimeSteps.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractTimeSteps.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cmath>
#include <vector>

svtkStandardNewMacro(svtkExtractTimeSteps);

svtkExtractTimeSteps::svtkExtractTimeSteps()
  : UseRange(false)
  , TimeStepInterval(1)
  , TimeEstimationMode(PREVIOUS_TIMESTEP)
{
  this->Range[0] = 0;
  this->Range[1] = 0;
}

//----------------------------------------------------------------------------
void svtkExtractTimeSteps::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  int count = static_cast<int>(this->TimeStepIndices.size());
  os << indent << "Number of Time Steps: " << count << std::endl;
  if (count > 0)
  {
    std::set<int>::iterator it = this->TimeStepIndices.begin();
    os << indent << "Time Step Indices: " << *it++;
    for (int i = 1; i < std::min(count, 4); ++i)
    {
      os << ", " << *it++;
    }
    if (count > 9)
    {
      std::advance(it, count - 8);
      os << ", ... ";
    }
    while (it != this->TimeStepIndices.end())
    {
      os << ", " << *it++;
    }
    os << std::endl;
  }

  os << indent << "UseRange: " << (this->UseRange ? "true" : "false") << std::endl;
  os << indent << "Range: " << this->Range[0] << ", " << this->Range[1] << std::endl;
  os << indent << "TimeStepInterval: " << this->TimeStepInterval << std::endl;
  os << indent << "TimeEstimationMode: ";
  switch (this->TimeEstimationMode)
  {
    case PREVIOUS_TIMESTEP:
      os << "Previous Timestep" << std::endl;
      break;
    case NEXT_TIMESTEP:
      os << "Next Timestep" << std::endl;
      break;
    case NEAREST_TIMESTEP:
      os << "Nearest Timestep" << std::endl;
      break;
  }
}

//----------------------------------------------------------------------------
void svtkExtractTimeSteps::AddTimeStepIndex(int timeStepIndex)
{
  if (this->TimeStepIndices.insert(timeStepIndex).second)
  {
    this->Modified();
  }
}

void svtkExtractTimeSteps::SetTimeStepIndices(int count, const int* timeStepIndices)
{
  this->TimeStepIndices.clear();
  this->TimeStepIndices.insert(timeStepIndices, timeStepIndices + count);
  this->Modified();
}

void svtkExtractTimeSteps::GetTimeStepIndices(int* timeStepIndices) const
{
  std::copy(this->TimeStepIndices.begin(), this->TimeStepIndices.end(), timeStepIndices);
}

void svtkExtractTimeSteps::GenerateTimeStepIndices(int begin, int end, int step)
{
  if (step != 0)
  {
    this->TimeStepIndices.clear();
    for (int i = begin; i < end; i += step)
    {
      this->TimeStepIndices.insert(i);
    }
    this->Modified();
  }
}

namespace
{

void getTimeSteps(svtkInformation* inInfo, const std::set<int>& timeStepIndices, bool useRange,
  int* range, int timeStepInterval, std::vector<double>& outTimes)
{
  double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int numTimes = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (!useRange)
  {
    for (std::set<int>::iterator it = timeStepIndices.begin(); it != timeStepIndices.end(); ++it)
    {
      if (*it >= 0 && *it < numTimes)
      {
        outTimes.push_back(inTimes[*it]);
      }
    }
  }
  else
  {
    for (int i = 0; i < numTimes; ++i)
    {
      if (i >= range[0] && i <= range[1])
      {
        if ((i - range[0]) % timeStepInterval == 0)
        {
          outTimes.push_back(inTimes[i]);
        }
      }
    }
  }
}

}

//----------------------------------------------------------------------------
int svtkExtractTimeSteps::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!this->TimeStepIndices.empty() && inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {

    std::vector<double> outTimes;
    getTimeSteps(
      inInfo, this->TimeStepIndices, this->UseRange, this->Range, this->TimeStepInterval, outTimes);

    if (!outTimes.empty())
    {
      outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &outTimes[0],
        static_cast<int>(outTimes.size()));

      double range[2] = { outTimes.front(), outTimes.back() };
      outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractTimeSteps::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double updateTime = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    std::vector<double> outTimes;
    getTimeSteps(
      inInfo, this->TimeStepIndices, this->UseRange, this->Range, this->TimeStepInterval, outTimes);

    if (outTimes.size() == 0)
    {
      svtkErrorMacro("Input has no time steps.");
      return 0;
    }

    double inputTime;
    if (updateTime >= outTimes.back())
    {
      inputTime = outTimes.back();
    }
    else if (updateTime <= outTimes.front())
    {
      inputTime = outTimes.front();
    }
    else
    {
      auto gtindex = std::upper_bound(outTimes.begin(), outTimes.end(), updateTime);
      auto leindex = gtindex - 1;
      if (updateTime == *leindex)
      {
        inputTime = updateTime;
      }
      else
      {
        switch (this->TimeEstimationMode)
        {
          default:
          case PREVIOUS_TIMESTEP:
            inputTime = *leindex;
            break;
          case NEXT_TIMESTEP:
            inputTime = *gtindex;
            break;
          case NEAREST_TIMESTEP:
            if (std::abs(updateTime - *leindex) <= std::abs(*gtindex - updateTime))
            {
              inputTime = *leindex;
            }
            else
            {
              inputTime = *gtindex;
            }
            break;
        }
      }
    }
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inputTime);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractTimeSteps::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* inData = svtkDataObject::GetData(inputVector[0], 0);
  svtkDataObject* outData = svtkDataObject::GetData(outputVector, 0);

  if (inData && outData)
  {
    outData->ShallowCopy(inData);
  }
  return 1;
}
