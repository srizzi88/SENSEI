/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalSnapToTimeStep.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTemporalSnapToTimeStep.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkTemporalSnapToTimeStep);

//----------------------------------------------------------------------------
svtkTemporalSnapToTimeStep::svtkTemporalSnapToTimeStep()
{
  this->HasDiscrete = 0;
  this->SnapMode = 0;
}

//----------------------------------------------------------------------------
svtkTemporalSnapToTimeStep::~svtkTemporalSnapToTimeStep() = default;

//----------------------------------------------------------------------------
svtkTypeBool svtkTemporalSnapToTimeStep::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // modify the time in either of these passes
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()) ||
    request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Change the information
int svtkTemporalSnapToTimeStep::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  this->HasDiscrete = 0;

  // unset the time steps if they are set
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int numTimes = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->InputTimeValues.resize(numTimes);
    inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->InputTimeValues[0]);
    this->HasDiscrete = 1;
  }

  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double* inRange = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
    double outRange[2];
    outRange[0] = inRange[0];
    outRange[1] = inRange[1];
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), outRange, 2);
  }
  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int svtkTemporalSnapToTimeStep::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* inData = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* outData = outInfo->Get(svtkDataObject::DATA_OBJECT());

  // shallow copy the data
  if (inData && outData)
  {
    outData->ShallowCopy(inData);

    // fill in the time steps
    double inTime = inData->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());

    if (inData->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
    {
      double outTime = inTime;
      outData->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), outTime);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkTemporalSnapToTimeStep::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // find the nearest timestep in the input
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    double* inTimes = new double[1];

    if (!this->HasDiscrete || this->InputTimeValues.empty())
    {
      inTimes[0] = upTime;
    }
    else
    {
      double dist = SVTK_DOUBLE_MAX;
      int index = -1;
      for (unsigned int t = 0; t < this->InputTimeValues.size(); t++)
      {
        double thisdist = fabs(upTime - this->InputTimeValues[t]);
        if (this->SnapMode == SVTK_SNAP_NEAREST && thisdist < dist)
        {
          index = t;
          dist = thisdist;
        }
        else if (this->SnapMode == SVTK_SNAP_NEXTBELOW_OR_EQUAL)
        {
          if (this->InputTimeValues[t] == upTime)
          {
            index = t;
            break;
          }
          else if (this->InputTimeValues[t] < upTime)
          {
            index = t;
          }
          else if (this->InputTimeValues[t] > upTime)
          {
            break;
          }
        }
        else if (this->SnapMode == SVTK_SNAP_NEXTABOVE_OR_EQUAL)
        {
          if (this->InputTimeValues[t] == upTime)
          {
            index = t;
            break;
          }
          else if (this->InputTimeValues[t] > upTime)
          {
            index = t;
            break;
          }
        }
      }
      upTime = this->InputTimeValues[index == -1 ? 0 : index];
    }
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), upTime);
    delete[] inTimes;
  }

  return 1;
}
//----------------------------------------------------------------------------
void svtkTemporalSnapToTimeStep::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SnapMode: " << this->SnapMode << endl;
}
//----------------------------------------------------------------------------
